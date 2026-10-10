/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2023-2025 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#include "valkey_glide_persistent.h"

#include <strings.h>
#include <unistd.h>

#include "ext/hash/php_hash.h"
#include "ext/hash/php_hash_sha.h"
#include "ext/standard/md5.h"
#include "logger.h"
#include "valkey_glide_commands_common.h"
#include "zend_smart_str.h"

#define VALKEY_GLIDE_PERSISTENT_RESOURCE_NAME "Valkey GLIDE persistent client"

struct valkey_glide_persistent_client {
    const void* client;      /* GLIDE client handle */
    pid_t       pid;         /* process that created the client */
    zend_long   database_id; /* configured database, restored after select() */
    zend_ulong  last_used;   /* persistent_client_clock when last attached or released */
    bool        in_use;      /* attached to an object */
    bool        listed;      /* still in EG(persistent_list) */
};

static int le_valkey_glide_persistent;

/* Kept clients in this process (EG(persistent_list) is per thread under ZTS) */
ZEND_TLS zend_long persistent_client_count = 0;
/* Orders entries by last use, to find the least recently used one */
ZEND_TLS zend_ulong persistent_client_clock = 0;

/* Free an entry, closing its client only in the process that created it: a
 * forked child does not have the Rust runtime threads behind the handle. */
static void persistent_client_free(valkey_glide_persistent_client_t* entry) {
    if (entry->pid == getpid()) {
        close_glide_client(entry->client);
    }
    pefree(entry, 1);
}

/* Called when the entry leaves EG(persistent_list): on discard, or at module
 * shutdown. A client still attached to an object is freed when released. */
static void persistent_client_dtor(zend_resource* res) {
    valkey_glide_persistent_client_t* entry = (valkey_glide_persistent_client_t*) res->ptr;
    if (!entry) {
        return;
    }
    res->ptr      = NULL;
    entry->listed = false;
    persistent_client_count--;
    if (!entry->in_use) {
        persistent_client_free(entry);
    }
}

void valkey_glide_persistent_minit(int module_number) {
    le_valkey_glide_persistent = zend_register_list_destructors_ex(
        NULL, persistent_client_dtor, VALKEY_GLIDE_PERSISTENT_RESOURCE_NAME, module_number);
}

static zend_string* persistent_key(bool        is_cluster,
                                   const char* persistent_id,
                                   size_t      persistent_id_len,
                                   const void* request_bytes,
                                   size_t      request_len) {
    smart_str key = {0};
    smart_str_appends(&key, is_cluster ? "valkey_glide:cluster:" : "valkey_glide:standalone:");
    smart_str_append_long(&key, (zend_long) persistent_id_len);
    smart_str_appendc(&key, ':');
    smart_str_appendl(&key, persistent_id, persistent_id_len);
    smart_str_appendc(&key, ':');

    /* Hash the request so the key holds no copy of the credentials */
    PHP_SHA256_CTX ctx;
    unsigned char  digest[32];
    char           hex[sizeof(digest) * 2 + 1];
    PHP_SHA256Init(&ctx);
    PHP_SHA256Update(&ctx, (const unsigned char*) request_bytes, request_len);
    PHP_SHA256Final(digest, &ctx);
    make_digest_ex(hex, digest, sizeof(digest));
    smart_str_appendl(&key, hex, sizeof(digest) * 2);
    smart_str_0(&key);
    return key.s;
}

bool valkey_glide_persistent_begin(valkey_glide_object*                      valkey_glide,
                                   valkey_glide_base_client_configuration_t* config,
                                   valkey_glide_periodic_checks_status_t     periodic_checks,
                                   bool                                      is_cluster,
                                   bool          refresh_topology_from_initial_nodes,
                                   const char*   persistent_id,
                                   size_t        persistent_id_len,
                                   zend_string** out_key) {
    *out_key = NULL;

    /* An address resolver is a request-scoped PHP callable */
    if (config->address_resolver && !Z_ISNULL_P(config->address_resolver)) {
        VALKEY_LOG_DEBUG("persistent_client",
                         "Address resolver configured; client will not be persistent");
        return false;
    }

    size_t   len           = 0;
    uint8_t* request_bytes = create_connection_request(
        &len, config, periodic_checks, is_cluster, refresh_topology_from_initial_nodes);
    if (!request_bytes) {
        return false; /* the caller checks EG(exception) */
    }
    zend_string* key =
        persistent_key(is_cluster, persistent_id, persistent_id_len, request_bytes, len);
    efree(request_bytes);

    zend_resource* res = zend_hash_find_ptr(&EG(persistent_list), key);
    if (res && res->type == le_valkey_glide_persistent && res->ptr) {
        valkey_glide_persistent_client_t* entry = (valkey_glide_persistent_client_t*) res->ptr;

        if (entry->pid != getpid()) {
            /* Inherited through fork: unusable here; replace it */
            zend_hash_del(&EG(persistent_list), key);
        } else if (entry->in_use) {
            /* Used by another object in this request: give this one its own client */
            VALKEY_LOG_DEBUG("persistent_client", "Persistent client in use; using a new client");
            zend_string_release(key);
            return false;
        } else {
            entry->in_use                     = true;
            entry->last_used                  = ++persistent_client_clock;
            valkey_glide->glide_client        = entry->client;
            valkey_glide->client_pid          = entry->pid;
            valkey_glide->persistent          = entry;
            valkey_glide->persistent_key      = key;
            valkey_glide->persistent_dirty    = false;
            valkey_glide->persistent_watching = false;
            valkey_glide->persistent_selected = false;
            VALKEY_LOG_DEBUG("persistent_client", "Reusing persistent client");
            return true;
        }
    }

    *out_key = key;
    return false;
}

/* Make room for a new client: close the least recently used client that no
 * object is using (one inherited through fork() first). An old configuration
 * (rotated password, moved host, ...) is never reused, so without this its
 * client would stay open until the process exits. Returns false if every kept
 * client is in use. */
static bool persistent_evict_idle_client(void) {
    zend_string*   oldest_key  = NULL;
    zend_ulong     oldest_used = 0;
    pid_t          pid         = getpid();
    zend_string*   key;
    zend_resource* res;

    ZEND_HASH_FOREACH_STR_KEY_PTR(&EG(persistent_list), key, res) {
        if (!key || res->type != le_valkey_glide_persistent || !res->ptr) {
            continue;
        }
        valkey_glide_persistent_client_t* entry = (valkey_glide_persistent_client_t*) res->ptr;
        if (entry->in_use && entry->pid == pid) {
            continue;
        }
        zend_ulong used = entry->pid == pid ? entry->last_used : 0;
        if (!oldest_key || used < oldest_used) {
            oldest_key  = key;
            oldest_used = used;
        }
    }
    ZEND_HASH_FOREACH_END();

    if (!oldest_key) {
        return false;
    }
    VALKEY_LOG_DEBUG("persistent_client", "Closing the least recently used persistent client");
    /* Runs persistent_client_dtor, which closes and frees the entry */
    zend_hash_del(&EG(persistent_list), oldest_key);
    return true;
}

void valkey_glide_persistent_store(valkey_glide_object* valkey_glide,
                                   zend_string*         key,
                                   int                  database_id) {
    zend_long max_clients = INI_INT("valkey_glide.max_persistent_clients");
    if (max_clients <= 0) {
        /* Persistence disabled */
        zend_string_release(key);
        return;
    }
    if (persistent_client_count >= max_clients && !persistent_evict_idle_client()) {
        VALKEY_LOG_WARN("persistent_client",
                        "valkey_glide.max_persistent_clients reached; client will not be kept");
        zend_string_release(key);
        return;
    }

    valkey_glide_persistent_client_t* entry = pemalloc(sizeof(*entry), 1);
    entry->client                           = valkey_glide->glide_client;
    entry->pid                              = getpid();
    entry->database_id                      = database_id < 0 ? 0 : database_id;
    entry->last_used                        = ++persistent_client_clock;
    entry->in_use                           = true;
    entry->listed                           = true;

    /* The list owns a persistent copy of the key; the object keeps its own. */
    zend_register_persistent_resource(
        ZSTR_VAL(key), ZSTR_LEN(key), entry, le_valkey_glide_persistent);
    persistent_client_count++;

    valkey_glide->persistent          = entry;
    valkey_glide->persistent_key      = key;
    valkey_glide->persistent_dirty    = false;
    valkey_glide->persistent_watching = false;
    valkey_glide->persistent_selected = false;
}

/* Clear WATCH before the next request inherits the client. Uses the FFI
 * directly: this may run after RSHUTDOWN, from object destruction. */
static bool persistent_client_unwatch(const void* client) {
    CommandResult* result = command(client, 0, UnWatch, 0, NULL, NULL, NULL, 0, 0);
    bool           ok     = result && !result->command_error;
    if (result) {
        free_command_result(result);
    }
    return ok;
}

/* Return the client to its configured database after select(). */
static bool persistent_client_select(const void* client, zend_long database_id) {
    char           db[MAX_LENGTH_OF_LONG];
    int            db_len     = snprintf(db, sizeof(db), ZEND_LONG_FMT, database_id);
    uintptr_t      args[1]    = {(uintptr_t) db};
    unsigned long  arg_len[1] = {(unsigned long) db_len};
    CommandResult* result     = command(client, 0, Select, 1, args, arg_len, NULL, 0, 0);
    bool           ok         = result && !result->command_error;
    if (result) {
        free_command_result(result);
    }
    return ok;
}

static void persistent_release(valkey_glide_object* valkey_glide, bool discard) {
    valkey_glide_persistent_client_t* entry = valkey_glide->persistent;

    entry->in_use = false;
    if (entry->pid != getpid()) {
        /* Object inherited through fork: never touch the client from here */
        discard = true;
    } else if (!discard) {
        if (valkey_glide->persistent_watching) {
            discard = !persistent_client_unwatch(entry->client);
        }
        if (!discard && valkey_glide->persistent_selected) {
            discard = !persistent_client_select(entry->client, entry->database_id);
        }
    }

    if (entry->listed) {
        entry->last_used = ++persistent_client_clock;
        if (discard) {
            /* Runs persistent_client_dtor, which frees the entry */
            zend_hash_del(&EG(persistent_list), valkey_glide->persistent_key);
        }
    } else {
        /* Removed from the list while attached (fork replacement, shutdown) */
        persistent_client_free(entry);
    }

    zend_string_release(valkey_glide->persistent_key);
    valkey_glide->persistent          = NULL;
    valkey_glide->persistent_key      = NULL;
    valkey_glide->persistent_dirty    = false;
    valkey_glide->persistent_watching = false;
    valkey_glide->persistent_selected = false;
    valkey_glide->glide_client        = NULL;
}

void valkey_glide_release_client(valkey_glide_object* valkey_glide, bool closing) {
    if (!valkey_glide->glide_client) {
        return;
    }

    if (valkey_glide->persistent) {
        persistent_release(valkey_glide, closing || valkey_glide->persistent_dirty);
        return;
    }

    valkey_glide_close_client(valkey_glide);
}

void valkey_glide_mark_connection_state_changed(valkey_glide_object* valkey_glide) {
    if (valkey_glide) {
        valkey_glide->persistent_dirty = true;
    }
}

#define NAME_IS(str, len, literal) \
    ((len) == sizeof(literal) - 1 && strncasecmp((str), (literal), sizeof(literal) - 1) == 0)

bool valkey_glide_command_changes_connection_state(const char* cmd,
                                                   size_t      cmd_len,
                                                   const char* sub,
                                                   size_t      sub_len) {
    if (!cmd) {
        return false;
    }

    if (NAME_IS(cmd, cmd_len, "CLIENT")) {
        /* Subcommands that only read, or act on other connections or the server */
        if (sub && (NAME_IS(sub, sub_len, "LIST") || NAME_IS(sub, sub_len, "INFO") ||
                    NAME_IS(sub, sub_len, "ID") || NAME_IS(sub, sub_len, "GETNAME") ||
                    NAME_IS(sub, sub_len, "TRACKINGINFO") || NAME_IS(sub, sub_len, "GETREDIR") ||
                    NAME_IS(sub, sub_len, "HELP") || NAME_IS(sub, sub_len, "KILL") ||
                    NAME_IS(sub, sub_len, "PAUSE") || NAME_IS(sub, sub_len, "UNPAUSE") ||
                    NAME_IS(sub, sub_len, "UNBLOCK"))) {
            return false;
        }
        return true;
    }

    return NAME_IS(cmd, cmd_len, "SELECT") || NAME_IS(cmd, cmd_len, "AUTH") ||
           NAME_IS(cmd, cmd_len, "HELLO") || NAME_IS(cmd, cmd_len, "RESET") ||
           NAME_IS(cmd, cmd_len, "READONLY") || NAME_IS(cmd, cmd_len, "READWRITE") ||
           NAME_IS(cmd, cmd_len, "MULTI") || NAME_IS(cmd, cmd_len, "WATCH") ||
           NAME_IS(cmd, cmd_len, "SUBSCRIBE") || NAME_IS(cmd, cmd_len, "PSUBSCRIBE") ||
           NAME_IS(cmd, cmd_len, "SSUBSCRIBE") || NAME_IS(cmd, cmd_len, "MONITOR") ||
           NAME_IS(cmd, cmd_len, "QUIT");
}
