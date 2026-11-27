/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */

#include "valkey_glide_credential_provider.h"

#include <ctype.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "valkey_glide_prefix.h"
#include "zend_exceptions.h"

zend_class_entry* get_valkey_glide_exception_ce(void);

/* The core's default IAM token refresh interval */
#define DEFAULT_REFRESH_INTERVAL_MS (300 * 1000)
/* Shortest time between two calls of a callable from refresh_if_due() */
#define MIN_REFRESH_GAP_MS 1000

/* ====================================================================
 * SNAPSHOT REGISTRY (process-global, read by core threads)
 * ==================================================================== */

/* The credentials the core gets for one provider. Plain malloc memory, so a
 * core thread can read it after the request that made it has ended. */
typedef struct credential_entry {
    uintptr_t                id;
    char*                    access_key_id;
    size_t                   access_key_id_len;
    char*                    secret_access_key;
    size_t                   secret_access_key_len;
    char*                    session_token; /* NULL if none */
    size_t                   session_token_len;
    int64_t                  expires_at_ms; /* Unix epoch ms, 0 if none */
    struct credential_entry* next;
} credential_entry_t;

static pthread_mutex_t     registry_mutex   = PTHREAD_MUTEX_INITIALIZER;
static credential_entry_t* registry         = NULL;
static pid_t               registry_pid     = 0;
static atomic_uintptr_t    next_provider_id = 1;

static void entry_free(credential_entry_t* entry) {
    if (!entry) {
        return;
    }
    free(entry->access_key_id);
    free(entry->secret_access_key);
    free(entry->session_token);
    free(entry);
}

/* PHP thread only, before using the registry. A forked child gets the registry
 * as it was in the parent, and the mutex may have been held by a core thread
 * that does not exist in the child: start over. The parent's entries belong to
 * clients the child never uses, so they are only dropped. */
static void registry_after_fork(void) {
    pid_t pid = getpid();
    if (registry_pid != pid) {
        if (registry_pid != 0) {
            pthread_mutex_init(&registry_mutex, NULL);
            registry = NULL;
        }
        registry_pid = pid;
    }
}

/* Must hold registry_mutex */
static credential_entry_t** registry_find(uintptr_t id) {
    credential_entry_t** link = &registry;
    while (*link && (*link)->id != id) {
        link = &(*link)->next;
    }
    return link;
}

/* Install entry as the snapshot of entry->id; frees the one it replaces */
static void registry_store(credential_entry_t* entry) {
    registry_after_fork();
    pthread_mutex_lock(&registry_mutex);
    credential_entry_t** link = registry_find(entry->id);
    credential_entry_t*  old  = *link;
    entry->next               = old ? old->next : NULL;
    *link                     = entry;
    pthread_mutex_unlock(&registry_mutex);
    entry_free(old);
}

static void registry_remove(uintptr_t id) {
    registry_after_fork();
    pthread_mutex_lock(&registry_mutex);
    credential_entry_t** link = registry_find(id);
    credential_entry_t*  old  = *link;
    if (old) {
        *link = old->next;
    }
    pthread_mutex_unlock(&registry_mutex);
    entry_free(old);
}

static int64_t wall_clock_ms(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (int64_t) now.tv_sec * 1000 + now.tv_usec / 1000;
}

static int64_t monotonic_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

uint8_t valkey_glide_credential_callback(uintptr_t  client_id,
                                         uint8_t*   access_key_id_buf,
                                         uintptr_t  access_key_id_buf_len,
                                         uintptr_t* access_key_id_len,
                                         uint8_t*   secret_access_key_buf,
                                         uintptr_t  secret_access_key_buf_len,
                                         uintptr_t* secret_access_key_len,
                                         uint8_t*   session_token_buf,
                                         uintptr_t  session_token_buf_len,
                                         uintptr_t* session_token_len,
                                         int64_t*   expires_at_epoch_millis) {
    /* Runs on a core thread: no Zend calls here */
    uint8_t status = 0;
    pthread_mutex_lock(&registry_mutex);
    credential_entry_t* entry = *registry_find(client_id);
    if (entry && (entry->expires_at_ms <= 0 || wall_clock_ms() < entry->expires_at_ms)) {
        *access_key_id_len     = entry->access_key_id_len;
        *secret_access_key_len = entry->secret_access_key_len;
        *session_token_len     = entry->session_token_len;
        if (entry->access_key_id_len <= access_key_id_buf_len &&
            entry->secret_access_key_len <= secret_access_key_buf_len &&
            entry->session_token_len <= session_token_buf_len) {
            memcpy(access_key_id_buf, entry->access_key_id, entry->access_key_id_len);
            memcpy(secret_access_key_buf, entry->secret_access_key, entry->secret_access_key_len);
            if (entry->session_token_len > 0) {
                memcpy(session_token_buf, entry->session_token, entry->session_token_len);
            }
            *expires_at_epoch_millis = entry->expires_at_ms;
            status                   = 1;
        } else {
            /* The core retries once with buffers of the full size limit */
            status = 2;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    return status;
}

/* ====================================================================
 * PROVIDERS (PHP thread)
 * ==================================================================== */

struct valkey_glide_credential_provider {
    uintptr_t id;
    zval      callable;
    bool      has_callable; /* false after request shutdown released it */
    bool      in_call;      /* the callable is running */
    bool      free_pending; /* freed while the callable ran: free when it returns */
    bool      listed;       /* in this thread's provider list */
    int64_t   interval_ms;
    int64_t   last_attempt_ms; /* monotonic */
    int64_t   last_success_ms; /* monotonic */
    int64_t   expires_at_ms;   /* of the current snapshot, Unix epoch ms, 0 if none */
    pid_t     pid;
    struct valkey_glide_credential_provider* next;
};

/* Providers created by this thread whose callable is still held */
ZEND_TLS valkey_glide_credential_provider_t* thread_providers = NULL;
/* Provider callables running on this thread */
ZEND_TLS int callable_depth = 0;

bool valkey_glide_credential_in_callable(void) {
    return callable_depth > 0;
}

uintptr_t valkey_glide_credential_provider_id(const valkey_glide_credential_provider_t* provider) {
    return provider->id;
}

zval* valkey_glide_credential_provider_callable(valkey_glide_credential_provider_t* provider) {
    return provider && provider->has_callable ? &provider->callable : NULL;
}

static void provider_unlist(valkey_glide_credential_provider_t* provider) {
    if (!provider->listed) {
        return;
    }
    valkey_glide_credential_provider_t** link = &thread_providers;
    while (*link && *link != provider) {
        link = &(*link)->next;
    }
    if (*link) {
        *link = provider->next;
    }
    provider->next   = NULL;
    provider->listed = false;
}

static char* copy_field(zval* value) {
    char* copy = malloc(Z_STRLEN_P(value) + 1);
    if (copy) {
        memcpy(copy, Z_STRVAL_P(value), Z_STRLEN_P(value));
        copy[Z_STRLEN_P(value)] = '\0';
    }
    return copy;
}

static bool is_blank(zval* value) {
    for (size_t index = 0; index < Z_STRLEN_P(value); index++) {
        if (!isspace((unsigned char) Z_STRVAL_P(value)[index])) {
            return false;
        }
    }
    return true;
}

/* Validate the callable's return value and copy it into a new entry. Returns an
 * error message, or NULL with *out set. Messages never contain credential values. */
static const char* parse_result(zval* result, uintptr_t id, credential_entry_t** out) {
    if (Z_TYPE_P(result) != IS_ARRAY) {
        return "credentialProvider must return an array";
    }
    HashTable* result_ht = Z_ARRVAL_P(result);

    zval* access_key_id = zend_hash_str_find_deref(result_ht, "accessKeyId", strlen("accessKeyId"));
    if (!access_key_id || Z_TYPE_P(access_key_id) != IS_STRING || is_blank(access_key_id)) {
        return "credentialProvider result: accessKeyId must be a non-blank string";
    }
    zval* secret_access_key =
        zend_hash_str_find_deref(result_ht, "secretAccessKey", strlen("secretAccessKey"));
    if (!secret_access_key || Z_TYPE_P(secret_access_key) != IS_STRING ||
        is_blank(secret_access_key)) {
        return "credentialProvider result: secretAccessKey must be a non-blank string";
    }
    zval* session_token =
        zend_hash_str_find_deref(result_ht, "sessionToken", strlen("sessionToken"));
    if (session_token && Z_TYPE_P(session_token) == IS_NULL) {
        session_token = NULL;
    }
    if (session_token && Z_TYPE_P(session_token) != IS_STRING) {
        return "credentialProvider result: sessionToken must be a string or null";
    }
    zval* expires_at = zend_hash_str_find_deref(result_ht, "expiresAt", strlen("expiresAt"));
    if (expires_at && Z_TYPE_P(expires_at) != IS_NULL && Z_TYPE_P(expires_at) != IS_LONG) {
        return "credentialProvider result: expiresAt must be an integer or null";
    }
    int64_t expires_at_ms =
        expires_at && Z_TYPE_P(expires_at) == IS_LONG && Z_LVAL_P(expires_at) > 0
            ? (int64_t) Z_LVAL_P(expires_at)
            : 0;
    if (expires_at_ms > 0 && expires_at_ms <= wall_clock_ms()) {
        return "credentialProvider result: the credentials have already expired";
    }

    size_t session_token_len = session_token ? Z_STRLEN_P(session_token) : 0;
    if (Z_STRLEN_P(access_key_id) > VALKEY_GLIDE_CREDENTIAL_MAX_BYTES ||
        Z_STRLEN_P(secret_access_key) > VALKEY_GLIDE_CREDENTIAL_MAX_BYTES ||
        session_token_len > VALKEY_GLIDE_CREDENTIAL_MAX_BYTES ||
        Z_STRLEN_P(access_key_id) + Z_STRLEN_P(secret_access_key) + session_token_len >
            VALKEY_GLIDE_CREDENTIAL_MAX_BYTES) {
        return "credentialProvider result: the credentials exceed 1 MiB";
    }

    credential_entry_t* entry = calloc(1, sizeof(*entry));
    if (!entry) {
        return "credentialProvider: out of memory";
    }
    entry->id                    = id;
    entry->access_key_id         = copy_field(access_key_id);
    entry->access_key_id_len     = Z_STRLEN_P(access_key_id);
    entry->secret_access_key     = copy_field(secret_access_key);
    entry->secret_access_key_len = Z_STRLEN_P(secret_access_key);
    if (session_token_len > 0) {
        entry->session_token     = copy_field(session_token);
        entry->session_token_len = session_token_len;
    }
    entry->expires_at_ms = expires_at_ms;
    if (!entry->access_key_id || !entry->secret_access_key ||
        (session_token_len > 0 && !entry->session_token)) {
        entry_free(entry);
        return "credentialProvider: out of memory";
    }
    *out = entry;
    return NULL;
}

static void provider_free_now(valkey_glide_credential_provider_t* provider) {
    provider_unlist(provider);
    registry_remove(provider->id);
    zval callable;
    ZVAL_UNDEF(&callable);
    if (provider->has_callable) {
        ZVAL_COPY_VALUE(&callable, &provider->callable);
        provider->has_callable = false;
    }
    pefree(provider, 1);
    /* Last: releasing the callable can run PHP code */
    zval_ptr_dtor(&callable);
}

bool valkey_glide_credential_provider_fetch(valkey_glide_credential_provider_t* provider,
                                            bool                                throw_on_error) {
    if (!provider->has_callable || provider->in_call) {
        if (throw_on_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), "credentialProvider cannot be called now", 0);
        }
        return false;
    }

    provider->last_attempt_ms = monotonic_ms();
    provider->in_call         = true;
    callable_depth++;
    zval retval;
    ZVAL_UNDEF(&retval);
    int call_result = call_user_function(NULL, NULL, &provider->callable, &retval, 0, NULL);

    const char*         error = NULL;
    credential_entry_t* entry = NULL;
    if (EG(exception)) {
        error = "credentialProvider threw an exception";
    } else if (call_result != SUCCESS || Z_ISUNDEF(retval)) {
        error = "credentialProvider could not be called";
    } else {
        error = parse_result(&retval, provider->id, &entry);
    }
    /* Destroying the result can run a destructor that closes the client. The guards stay up
     * until then, so the close is deferred (free_pending) and the provider stays valid */
    zval_ptr_dtor(&retval);
    callable_depth--;
    provider->in_call = false;
    /* A destructor of the result (e.g. an extra object in it) can throw */
    if (!error && EG(exception)) {
        error = "credentialProvider threw an exception";
    }

    bool installed = false;
    if (!error && !provider->free_pending) {
        provider->expires_at_ms   = entry->expires_at_ms;
        provider->last_success_ms = provider->last_attempt_ms;
        registry_store(entry);
        installed = true;
    } else {
        entry_free(entry);
    }

    if (error) {
        if (throw_on_error) {
            /* Chains the callable's exception, if any, as the previous one */
            zend_throw_exception(get_valkey_glide_exception_ce(), error, 0);
        } else if (EG(exception)) {
            zend_clear_exception();
        }
    }
    if (provider->free_pending) {
        provider_free_now(provider);
    }
    return installed;
}

valkey_glide_credential_provider_t* valkey_glide_credential_provider_create(
    zval* callable, int refresh_interval_seconds) {
    if (!zend_is_callable(callable, 0, NULL)) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "iamConfig credentialProvider must be callable", 0);
        return NULL;
    }

    valkey_glide_credential_provider_t* provider = pecalloc(1, sizeof(*provider), 1);
    provider->id                                 = atomic_fetch_add(&next_provider_id, 1);
    provider->interval_ms = refresh_interval_seconds > 0 ? (int64_t) refresh_interval_seconds * 1000
                                                         : DEFAULT_REFRESH_INTERVAL_MS;
    provider->pid         = getpid();
    ZVAL_COPY(&provider->callable, callable);
    provider->has_callable = true;
    provider->next         = thread_providers;
    provider->listed       = true;
    thread_providers       = provider;

    if (!valkey_glide_credential_provider_fetch(provider, true)) {
        valkey_glide_credential_provider_free(provider);
        return NULL;
    }
    return provider;
}

static bool provider_is_due(const valkey_glide_credential_provider_t* provider) {
    int64_t now = monotonic_ms();
    if (now - provider->last_attempt_ms < MIN_REFRESH_GAP_MS) {
        return false;
    }
    /* Half the interval, so the core's refresh tick signs with recent credentials */
    if (now - provider->last_success_ms >= provider->interval_ms / 2) {
        return true;
    }
    return provider->expires_at_ms > 0 &&
           provider->expires_at_ms - wall_clock_ms() <= provider->interval_ms;
}

void valkey_glide_credential_refresh_if_due(const void* glide_client) {
    if (!thread_providers) {
        return;
    }
    valkey_glide_object* valkey_glide = valkey_glide_prefix_find_object(glide_client);
    if (!valkey_glide || !valkey_glide->credential_provider) {
        return;
    }
    valkey_glide_credential_provider_t* provider = valkey_glide->credential_provider;
    if (!provider->has_callable || provider->in_call || provider->pid != getpid() ||
        !provider_is_due(provider)) {
        return;
    }
    /* A failure keeps the previous snapshot; the command still runs */
    valkey_glide_credential_provider_fetch(provider, false);
}

bool valkey_glide_credential_client_has_provider(const void* glide_client) {
    if (!thread_providers) {
        return false;
    }
    valkey_glide_object* valkey_glide = valkey_glide_prefix_find_object(glide_client);
    return valkey_glide && valkey_glide->credential_provider &&
           valkey_glide->credential_provider->has_callable &&
           valkey_glide->credential_provider->pid == getpid();
}

void valkey_glide_credential_provider_free(valkey_glide_credential_provider_t* provider) {
    if (!provider) {
        return;
    }
    if (provider->in_call) {
        provider->free_pending = true;
        return;
    }
    provider_free_now(provider);
}

void valkey_glide_credential_request_shutdown(void) {
    /* Releasing a callable can run PHP code that frees another provider */
    while (thread_providers) {
        valkey_glide_credential_provider_t* provider = thread_providers;
        provider_unlist(provider);
        registry_remove(provider->id);
        if (provider->has_callable) {
            zval callable;
            ZVAL_COPY_VALUE(&callable, &provider->callable);
            provider->has_callable = false;
            /* Can free this provider (its client is freed) or others */
            zval_ptr_dtor(&callable);
        }
    }
    callable_depth = 0;
}
