/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_pubsub_common.h"

#include <time.h>
#include <unistd.h>
#include <zend_exceptions.h>

#include "logger.h"

// PubSub message type constants (from PushKind enum)
#define PUBSUB_KIND_MESSAGE 3
#define PUBSUB_KIND_PMESSAGE 4
#define PUBSUB_KIND_SMESSAGE 5

// Mutex wrapper functions
void mutex_init(mutex_t* m) {
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

void mutex_lock(mutex_t* m) {
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

void mutex_unlock(mutex_t* m) {
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

void mutex_destroy(mutex_t* m) {
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

// Condition variable wrapper functions
void cond_init(cond_t* c) {
#ifdef _WIN32
    InitializeConditionVariable(c);
#else
    pthread_cond_init(c, NULL);
#endif
}

void cond_wait(cond_t* c, mutex_t* m) {
#ifdef _WIN32
    SleepConditionVariableCS(c, m, INFINITE);
#else
    pthread_cond_wait(c, m);
#endif
}

/* M-6: project-owned names. Previously these were NSEC_PER_SEC / NSEC_PER_MSEC
 * behind #ifndef guards, which collided with macOS's mach/clock_types.h. The
 * guards resolved the -Werror build failure but let the platform's definition win
 * silently -- and the platform spells them `ull` where we mean `LL`, so the
 * timeout arithmetic below would quietly convert to unsigned. Owning the names
 * removes both the collision and the signedness ambiguity. */
#define VALKEY_GLIDE_NSEC_PER_SEC 1000000000LL
#define VALKEY_GLIDE_NSEC_PER_MSEC 1000000LL

int cond_timedwait(cond_t* c, mutex_t* m, unsigned int timeout_ms) {
#ifdef _WIN32
    BOOL ok = SleepConditionVariableCS(c, m, timeout_ms);
    return ok ? 0 : 1;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        // Clock read failed (effectively never on supported platforms). Report
        // a timeout rather than silently converting a bounded wait into an
        // unbounded one; the caller re-checks its own predicate/deadline.
        return 1;
    }
    // Work in nanoseconds so the second/nanosecond split is a single
    // divide/modulo (pthread_cond_timedwait requires tv_nsec in [0, 1e9)).
    // e.g. a current tv_nsec of 800,000,000 plus a 300 ms timeout
    // (300,000,000 ns) totals 1,100,000,000 ns -> +1 s and 100,000,000 ns.
    long long total_ns =
        (long long) ts.tv_nsec + (long long) timeout_ms * VALKEY_GLIDE_NSEC_PER_MSEC;
    ts.tv_sec += total_ns / VALKEY_GLIDE_NSEC_PER_SEC;
    ts.tv_nsec = total_ns % VALKEY_GLIDE_NSEC_PER_SEC;
    return pthread_cond_timedwait(c, m, &ts);
#endif
}

void cond_signal(cond_t* c) {
#ifdef _WIN32
    WakeConditionVariable(c);
#else
    pthread_cond_signal(c);
#endif
}

void cond_destroy(cond_t* c) {
#ifdef _WIN32
    // No cleanup needed for Windows condition variables
#else
    pthread_cond_destroy(c);
#endif
}

// Global pubsub callback storage. The table is persistent and shared by all
// PHP threads (the push callback looks entries up from a GLIDE thread); each
// entry is request memory of the thread that registered it.
static HashTable pubsub_callbacks;
static bool      pubsub_callbacks_initialized = false;
// Guards pubsub_callbacks. The push callback holds it while it uses an entry,
// so an entry is only freed (by its own PHP thread) after removal under it.
static mutex_t pubsub_callbacks_mutex;
// Process that owns the registry. After fork() the child must not take the
// inherited mutex: a GLIDE thread may have held it, and that thread is not in
// the child. The child starts an empty registry of its own instead.
static pid_t pubsub_callbacks_pid;
// Entries of clients closed from their own subscribe callback, linked via
// next_removed. They are out of the table, so neither the push callback nor a
// new client at the same address finds them; the subscribe loop frees its
// entry when the callback returns, and request shutdown frees any left by a
// bailout. Only the PHP thread that owns them touches this list.
ZEND_TLS pubsub_callback_info* pubsub_detached = NULL;

// Call before any use of the registry from a PHP thread
static void pubsub_callbacks_after_fork(void) {
    if (pubsub_callbacks_pid != getpid()) {
        mutex_init(&pubsub_callbacks_mutex);
        /* The table has no destructor: the parent's entries are only dropped */
        zend_hash_clean(&pubsub_callbacks);
        pubsub_callbacks_pid = getpid();
    }
}

static uintptr_t current_thread_id(void) {
#ifdef _WIN32
    return (uintptr_t) GetCurrentThreadId();
#else
    return (uintptr_t) pthread_self();
#endif
}

// Initialize pubsub callbacks (module startup)
void init_pubsub_callbacks(void) {
    if (!pubsub_callbacks_initialized) {
        // No destructor: entries are freed outside the mutex, by their owner
        zend_hash_init(&pubsub_callbacks, 16, NULL, NULL, 1);
        mutex_init(&pubsub_callbacks_mutex);
        pubsub_callbacks_pid         = getpid();
        pubsub_callbacks_initialized = true;
    }
}

// Find pubsub callback by client key. For PHP threads: an entry is only
// removed by the thread that registered it.
pubsub_callback_info* find_pubsub_callback(const char* client_key) {
    if (!pubsub_callbacks_initialized) {
        return NULL;
    }
    pubsub_callbacks_after_fork();
    mutex_lock(&pubsub_callbacks_mutex);
    pubsub_callback_info* info =
        zend_hash_str_find_ptr(&pubsub_callbacks, client_key, strlen(client_key));
    mutex_unlock(&pubsub_callbacks_mutex);
    return info;
}

// Remove pubsub callback by client key and return it (to be freed by the caller)
static pubsub_callback_info* take_pubsub_callback(const char* client_key, size_t key_len) {
    pubsub_callbacks_after_fork();
    mutex_lock(&pubsub_callbacks_mutex);
    pubsub_callback_info* info = zend_hash_str_find_ptr(&pubsub_callbacks, client_key, key_len);
    if (info) {
        zend_hash_str_del(&pubsub_callbacks, client_key, key_len);
    }
    mutex_unlock(&pubsub_callbacks_mutex);
    return info;
}

// Free a queued message (allocated by the push callback with malloc)
static void free_pubsub_message(pubsub_message* msg) {
    free(msg->channel);
    free(msg->message);
    free(msg->pattern);
    free(msg);
}

// Cleanup callback info
void cleanup_callback_info(pubsub_callback_info* info) {
    if (info) {
        // Free all messages in queue
        pubsub_message* msg = info->queue_head;
        while (msg) {
            pubsub_message* next = msg->next;
            free_pubsub_message(msg);
            msg = next;
        }
        // A bailout (e.g. memory_limit) in the callback skipped freeing it
        if (info->in_flight) {
            free_pubsub_message(info->in_flight);
        }

        mutex_destroy(&info->queue_mutex);
        cond_destroy(&info->queue_cond);
        zval_ptr_dtor(&info->callback);
        Z_DELREF(info->client_obj);

        if (info->subscribed_channels) {
            zend_hash_destroy(info->subscribed_channels);
            efree(info->subscribed_channels);
        }

        efree(info);
    }
}

// Count a dropped message (queue_mutex held). Only the first drop the subscribe
// loop has not reported yet wakes it, so a flood of drops does not keep waking it
static void pubsub_count_drop(pubsub_callback_info* info) {
    info->dropped_count++;
    if (info->dropped_count == info->reported_drops + 1) {
        cond_signal(&info->queue_cond);
    }
}

// Whether the subscribe loop should log dropped messages now (queue_mutex held):
// at most once per second, so sustained overload does not flood the log
static bool pubsub_drop_report_due(pubsub_callback_info* info, time_t now) {
    return info->dropped_count != info->reported_drops && now != info->last_drop_report;
}

// C callback handler for FFI
void pubsub_callback_handler(uintptr_t      client_ptr,
                             int            kind,
                             const uint8_t* message,
                             int64_t        message_len,
                             const uint8_t* channel,
                             int64_t        channel_len,
                             const uint8_t* pattern,
                             int64_t        pattern_len) {
    if (!pubsub_callbacks_initialized) {
        return;
    }

    // Only handle message types
    if (kind != PUBSUB_KIND_MESSAGE && kind != PUBSUB_KIND_PMESSAGE &&
        kind != PUBSUB_KIND_SMESSAGE) {
        return;
    }

    // Runs on a GLIDE thread: no PHP allocator or zvals here, and the entry is
    // used only while pubsub_callbacks_mutex keeps its owner from freeing it
    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    mutex_lock(&pubsub_callbacks_mutex);
    pubsub_callback_info* info = zend_hash_str_find_ptr(&pubsub_callbacks, client_key, key_len);
    if (!info) {
        mutex_unlock(&pubsub_callbacks_mutex);
        return;
    }

    // Reserve room in the queue before copying anything, or drop the message.
    // An empty queue always admits one message, so one larger than the byte
    // cap is still delivered and memory stays bounded by that one message.
    // While such a message is queued, queue_bytes is over the cap, so check
    // that before subtracting to keep the unsigned difference from wrapping
    bool   has_pattern = pattern && pattern_len > 0;
    size_t size =
        (size_t) channel_len + (size_t) message_len + (has_pattern ? (size_t) pattern_len : 0);
    mutex_lock(&info->queue_mutex);
    // is_active is written by the PHP thread under queue_mutex
    if (!info->is_active) {
        mutex_unlock(&info->queue_mutex);
        mutex_unlock(&pubsub_callbacks_mutex);
        return;
    }
    bool room = info->queue_depth == 0 || (info->queue_depth < PUBSUB_QUEUE_MAX_DEPTH &&
                                           info->queue_bytes <= PUBSUB_QUEUE_MAX_BYTES &&
                                           size <= PUBSUB_QUEUE_MAX_BYTES - info->queue_bytes);
    if (room) {
        info->queue_depth++;
        info->queue_bytes += size;
    } else {
        pubsub_count_drop(info);
    }
    mutex_unlock(&info->queue_mutex);
    if (!room) {
        mutex_unlock(&pubsub_callbacks_mutex);
        return;
    }

    // Allocate and populate message node (malloc of at least 1 byte, so an
    // empty payload is not mistaken for a failed allocation)
    pubsub_message* msg = (pubsub_message*) calloc(1, sizeof(pubsub_message));
    if (msg) {
        msg->channel = (uint8_t*) malloc(channel_len > 0 ? (size_t) channel_len : 1);
        msg->message = (uint8_t*) malloc(message_len > 0 ? (size_t) message_len : 1);
        if (has_pattern) {
            msg->pattern = (uint8_t*) malloc((size_t) pattern_len);
        }
    }
    if (!msg || !msg->channel || !msg->message || (has_pattern && !msg->pattern)) {
        if (msg) {
            free_pubsub_message(msg);
        }
        mutex_lock(&info->queue_mutex);
        info->queue_depth--;
        info->queue_bytes -= size;
        pubsub_count_drop(info);
        mutex_unlock(&info->queue_mutex);
        mutex_unlock(&pubsub_callbacks_mutex);
        return;
    }

    msg->kind = kind;
    msg->next = NULL;
    memcpy(msg->channel, channel, channel_len);
    msg->channel_len = channel_len;
    memcpy(msg->message, message, message_len);
    msg->message_len = message_len;
    if (has_pattern) {
        memcpy(msg->pattern, pattern, pattern_len);
        msg->pattern_len = pattern_len;
    }

    // Add to queue (thread-safe); its room is already reserved
    mutex_lock(&info->queue_mutex);
    if (info->queue_tail) {
        info->queue_tail->next = msg;
    } else {
        info->queue_head = msg;
    }
    info->queue_tail = msg;
    cond_signal(&info->queue_cond);
    mutex_unlock(&info->queue_mutex);
    mutex_unlock(&pubsub_callbacks_mutex);
}

// Register callback
void php_register_pubsub_callback(uintptr_t client_ptr, zval* callback, zval* client_obj) {
    init_pubsub_callbacks();

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    pubsub_callback_info* info = emalloc(sizeof(pubsub_callback_info));

    // Copy the callback and reference the client object
    ZVAL_COPY(&info->callback, callback);
    info->client_obj = *client_obj;
    Z_ADDREF(info->client_obj);
    info->is_active = true;

    // Initialize message queue
    info->queue_head       = NULL;
    info->queue_tail       = NULL;
    info->queue_depth      = 0;
    info->queue_bytes      = 0;
    info->dropped_count    = 0;
    info->reported_drops   = 0;
    info->last_drop_report = 0;
    info->in_flight        = NULL;
    mutex_init(&info->queue_mutex);
    cond_init(&info->queue_cond);

    // Initialize subscribed channels HashTable
    info->subscribed_channels = emalloc(sizeof(HashTable));
    zend_hash_init(info->subscribed_channels, 8, NULL, ZVAL_PTR_DTOR, 0);

    info->in_subscribe_mode = false;
    info->client_closed     = false;
    info->owner_thread      = current_thread_id();

    // Store the pointer, replacing any earlier entry in the same critical
    // section (so the push callback never finds the key missing in between),
    // then free the old entry outside the mutex
    pubsub_callbacks_after_fork();
    mutex_lock(&pubsub_callbacks_mutex);
    pubsub_callback_info* previous = zend_hash_str_find_ptr(&pubsub_callbacks, client_key, key_len);
    zend_hash_str_update_ptr(&pubsub_callbacks, client_key, key_len, info);
    mutex_unlock(&pubsub_callbacks_mutex);
    cleanup_callback_info(previous);
}

// Unregister callback
void php_unregister_pubsub_callback(uintptr_t client_ptr) {
    if (!pubsub_callbacks_initialized)
        return;

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    // Remove from the table first, so the push callback no longer sees it
    pubsub_callback_info* info = take_pubsub_callback(client_key, key_len);
    if (info) {
        info->is_active = false;
        cond_signal(&info->queue_cond);
        cleanup_callback_info(info);
    }
}

// Drop a closed client's entry now instead of at request end, so a message
// still in flight is not queued for nobody. A close() from inside the
// subscribe callback cannot free the entry, because the subscribe loop still
// uses it: it is detached instead, and the loop ends when the callback returns
// and frees it
void php_release_pubsub_callback(uintptr_t client_ptr) {
    if (!is_client_in_subscribe_mode(client_ptr)) {
        php_unregister_pubsub_callback(client_ptr);
        return;
    }

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    // Out of the table, the push callback can no longer reach it, and a client
    // that connect() gives the same address starts without it
    pubsub_callback_info* info = take_pubsub_callback(client_key, key_len);
    if (info) {
        info->client_closed = true;
        info->is_active     = false;
        info->next_removed  = pubsub_detached;
        pubsub_detached     = info;
    }
}

// Unlink an entry that php_release_pubsub_callback() detached
static void forget_detached_callback(pubsub_callback_info* info) {
    pubsub_callback_info** link = &pubsub_detached;
    while (*link && *link != info) {
        link = &(*link)->next_removed;
    }
    if (*link) {
        *link = info->next_removed;
    }
}

// Check if client is in subscribe mode
bool is_client_in_subscribe_mode(uintptr_t client_ptr) {
    if (!pubsub_callbacks_initialized)
        return false;

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    pubsub_callback_info* info = find_pubsub_callback(client_key);
    return info ? info->in_subscribe_mode : false;
}

// Common subscribe blocking loop
static void subscribe_blocking_loop(uintptr_t connection, enum RequestType unsub_type) {
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) connection);
    pubsub_callback_info* info = find_pubsub_callback(client_key);
    if (!info)
        return;

    info->in_subscribe_mode = true;

    while (info->is_active && zend_hash_num_elements(info->subscribed_channels) > 0) {
        pubsub_message* msg = NULL;

        mutex_lock(&info->queue_mutex);
        while (!info->queue_head && info->is_active &&
               zend_hash_num_elements(info->subscribed_channels) > 0 &&
               !pubsub_drop_report_due(info, time(NULL))) {
            if (info->dropped_count != info->reported_drops) {
                /* Drops are pending but the once-per-second limit is not up yet;
                 * no later drop signals again, so wake up in time to report them */
                cond_timedwait(&info->queue_cond, &info->queue_mutex, 1000);
            } else {
                cond_wait(&info->queue_cond, &info->queue_mutex);
            }
        }
        if (info->queue_head) {
            msg              = info->queue_head;
            info->queue_head = msg->next;
            if (!info->queue_head) {
                info->queue_tail = NULL;
            }
            info->queue_depth--;
            info->queue_bytes -= (size_t) msg->channel_len + (size_t) msg->message_len +
                                 (msg->pattern ? (size_t) msg->pattern_len : 0);
        }
        time_t now     = time(NULL);
        bool   report  = pubsub_drop_report_due(info, now);
        size_t dropped = info->dropped_count;
        if (report) {
            info->reported_drops   = dropped;
            info->last_drop_report = now;
        }
        mutex_unlock(&info->queue_mutex);

        // Native memory: keep it reachable from the entry until it is freed, so
        // request shutdown frees it if PHP bails out of the warning below (it
        // allocates) or of the callback
        if (msg) {
            info->in_flight = msg;
        }

        if (report) {
            VALKEY_LOG_WARN_FMT(
                "pubsub", "Subscriber queue full: %zu message(s) dropped so far", dropped);
        }

        if (msg) {
            zval php_channel, php_message, php_pattern;
            ZVAL_STRINGL(&php_channel, (char*) msg->channel, msg->channel_len);
            ZVAL_STRINGL(&php_message, (char*) msg->message, msg->message_len);

            if (msg->pattern && msg->pattern_len > 0) {
                ZVAL_STRINGL(&php_pattern, (char*) msg->pattern, msg->pattern_len);
            } else {
                ZVAL_NULL(&php_pattern);
            }

            zval args[4];
            args[0] = info->client_obj;
            args[1] = php_channel;
            args[2] = php_message;
            args[3] = php_pattern;

            zval retval;
            ZVAL_UNDEF(&retval);
            int arg_count = (msg->pattern && msg->pattern_len > 0) ? 4 : 3;

            if (call_user_function(NULL, NULL, &info->callback, &retval, arg_count, args) ==
                SUCCESS) {
                zval_ptr_dtor(&retval);
            }

            zval_ptr_dtor(&php_channel);
            zval_ptr_dtor(&php_message);
            if (msg->pattern && msg->pattern_len > 0) {
                zval_ptr_dtor(&php_pattern);
            }

            info->in_flight = NULL;
            free_pubsub_message(msg);
        }
    }

    // Drops not reported yet because of the once-per-second limit
    mutex_lock(&info->queue_mutex);
    size_t dropped       = info->dropped_count;
    bool   unreported    = dropped != info->reported_drops;
    info->reported_drops = dropped;
    mutex_unlock(&info->queue_mutex);
    if (unreported) {
        VALKEY_LOG_WARN_FMT(
            "pubsub", "Subscriber queue full: %zu message(s) dropped in total", dropped);
    }

    // A client closed from the callback has no connection to unsubscribe on
    if (!info->client_closed) {
        struct CommandResult* unsub_result =
            command((const void*) connection, 0, unsub_type, 0, NULL, NULL, NULL, 0, 0);
        if (unsub_result) {
            free_command_result(unsub_result);
        }
    }

    info->in_subscribe_mode = false;
    if (info->client_closed) {
        // Detached by close(): no longer in the table under this key
        forget_detached_callback(info);
        cleanup_callback_info(info);
    } else {
        php_unregister_pubsub_callback(connection);
    }
}

// Helper: Execute subscribe command
static int execute_subscribe_command(const void*      connection,
                                     zval*            items_array,
                                     zend_long        timeout_ms,
                                     enum RequestType subscribe_type,
                                     enum RequestType unsubscribe_type,
                                     const char*      command_name,
                                     const char*      error_prefix,
                                     zval*            return_value) {
    HashTable* items_ht   = Z_ARRVAL_P(items_array);
    uint32_t   item_count = zend_hash_num_elements(items_ht);

    uint32_t       total_args = item_count + 1;
    uintptr_t*     args       = emalloc(total_args * sizeof(uintptr_t));
    unsigned long* args_len   = emalloc(total_args * sizeof(unsigned long));

    uint32_t i = 0;
    zval*    item_zv;
    ZEND_HASH_FOREACH_VAL(items_ht, item_zv) {
        convert_to_string(item_zv);
        args[i]     = (uintptr_t) Z_STRVAL_P(item_zv);
        args_len[i] = Z_STRLEN_P(item_zv);
        i++;
    }
    ZEND_HASH_FOREACH_END();

    char timeout_str[32];
    int  timeout_len = snprintf(timeout_str, sizeof(timeout_str), "%lld", (long long) timeout_ms);
    args[item_count] = (uintptr_t) timeout_str;
    args_len[item_count] = timeout_len;

    struct CommandResult* result =
        command(connection, 0, subscribe_type, total_args, args, args_len, NULL, 0, 0);

    efree(args);
    efree(args_len);

    if (!result || result->command_error) {
        const char* error_msg =
            result && result->command_error && result->command_error->command_error_message
                ? result->command_error->command_error_message
                : error_prefix;
        VALKEY_LOG_ERROR(command_name, error_msg);
        if (result)
            free_command_result(result);
        php_unregister_pubsub_callback((uintptr_t) connection);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
        ZVAL_FALSE(return_value);
        return 0;
    }
    free_command_result(result);

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) connection);
    pubsub_callback_info* info = find_pubsub_callback(client_key);
    if (!info) {
        VALKEY_LOG_ERROR(command_name, "Failed to find pubsub callback after command execution");
        ZVAL_FALSE(return_value);
        return 0;
    }

    // Add channels to subscribed set
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(items_array), item_zv) {
        convert_to_string(item_zv);
        zval dummy;
        ZVAL_TRUE(&dummy);
        zend_hash_str_add(
            info->subscribed_channels, Z_STRVAL_P(item_zv), Z_STRLEN_P(item_zv), &dummy);
    }
    ZEND_HASH_FOREACH_END();

    subscribe_blocking_loop((uintptr_t) connection, unsubscribe_type);

    ZVAL_TRUE(return_value);
    return 1;
}

// Helper: Execute unsubscribe command
static void execute_unsubscribe_command(const void*      connection,
                                        zval*            items_array,
                                        enum RequestType unsubscribe_type,
                                        const char*      command_name) {
    if (items_array) {
        HashTable* items_ht   = Z_ARRVAL_P(items_array);
        uint32_t   item_count = zend_hash_num_elements(items_ht);

        uint32_t       total_args = item_count + 1;
        uintptr_t*     args       = emalloc(total_args * sizeof(uintptr_t));
        unsigned long* args_len   = emalloc(total_args * sizeof(unsigned long));

        uint32_t i = 0;
        zval*    item_zv;
        ZEND_HASH_FOREACH_VAL(items_ht, item_zv) {
            convert_to_string(item_zv);
            args[i]     = (uintptr_t) Z_STRVAL_P(item_zv);
            args_len[i] = Z_STRLEN_P(item_zv);
            i++;
        }
        ZEND_HASH_FOREACH_END();

        args[item_count]     = (uintptr_t) "0";
        args_len[item_count] = 1;

        struct CommandResult* result =
            command(connection, 0, unsubscribe_type, total_args, args, args_len, NULL, 0, 0);

        efree(args);
        efree(args_len);

        if (result) {
            if (result->command_error && result->command_error->command_error_message) {
                VALKEY_LOG_ERROR(command_name, result->command_error->command_error_message);
            }
            free_command_result(result);
        }

        // Update subscription set
        char client_key[32];
        snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) connection);
        pubsub_callback_info* info = find_pubsub_callback(client_key);
        if (info) {
            // Remove channels from subscribed set
            ZEND_HASH_FOREACH_VAL(items_ht, item_zv) {
                convert_to_string(item_zv);
                zend_hash_str_del(
                    info->subscribed_channels, Z_STRVAL_P(item_zv), Z_STRLEN_P(item_zv));
            }
            ZEND_HASH_FOREACH_END();

            if (zend_hash_num_elements(info->subscribed_channels) == 0) {
                mutex_lock(&info->queue_mutex);
                info->is_active = false;
                cond_signal(&info->queue_cond);
                mutex_unlock(&info->queue_mutex);
            }
        }
    } else {
        uintptr_t     args[1]     = {(uintptr_t) "0"};
        unsigned long args_len[1] = {1};

        struct CommandResult* result =
            command(connection, 0, unsubscribe_type, 1, args, args_len, NULL, 0, 0);
        if (result) {
            if (result->command_error && result->command_error->command_error_message) {
                VALKEY_LOG_ERROR(command_name, result->command_error->command_error_message);
            }
            free_command_result(result);
        }

        // Update subscription set
        char client_key[32];
        snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) connection);
        pubsub_callback_info* info = find_pubsub_callback(client_key);
        if (info) {
            zend_hash_clean(info->subscribed_channels);
            mutex_lock(&info->queue_mutex);
            info->is_active = false;
            cond_signal(&info->queue_cond);
            mutex_unlock(&info->queue_mutex);
        }
    }
}

// Subscribe implementation
void valkey_glide_subscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *    channels, *callback;
    zend_long timeout_ms = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
    Z_PARAM_ARRAY(channels)
    Z_PARAM_ZVAL(callback)
    Z_PARAM_OPTIONAL
    Z_PARAM_LONG(timeout_ms)
    ZEND_PARSE_PARAMETERS_END();

    if (is_client_in_subscribe_mode((uintptr_t) connection)) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "Client is in subscribe mode. Only unsubscribe commands are allowed.",
                             0);
        RETURN_FALSE;
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Callback must be callable", 0);
        RETURN_FALSE;
    }

    php_register_pubsub_callback((uintptr_t) connection, callback, ZEND_THIS);

    execute_subscribe_command(connection,
                              channels,
                              timeout_ms,
                              REQUEST_TYPE_SUBSCRIBE,
                              REQUEST_TYPE_UNSUBSCRIBE,
                              "subscribe",
                              "Subscribe command failed",
                              return_value);
}

// PSubscribe implementation
void valkey_glide_psubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *    patterns, *callback;
    zend_long timeout_ms = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
    Z_PARAM_ARRAY(patterns)
    Z_PARAM_ZVAL(callback)
    Z_PARAM_OPTIONAL
    Z_PARAM_LONG(timeout_ms)
    ZEND_PARSE_PARAMETERS_END();

    if (is_client_in_subscribe_mode((uintptr_t) connection)) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "Client is in subscribe mode. Only unsubscribe commands are allowed.",
                             0);
        RETURN_FALSE;
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        VALKEY_LOG_ERROR("psubscribe", "Callback is not callable");
        zend_throw_exception(get_valkey_glide_exception_ce(), "Callback must be callable", 0);
        RETURN_FALSE;
    }

    php_register_pubsub_callback((uintptr_t) connection, callback, ZEND_THIS);

    execute_subscribe_command(connection,
                              patterns,
                              timeout_ms,
                              REQUEST_TYPE_PSUBSCRIBE,
                              REQUEST_TYPE_PUNSUBSCRIBE,
                              "psubscribe",
                              "PSubscribe command failed",
                              return_value);
}

// Unsubscribe implementation
void valkey_glide_unsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval* channels = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(channels)
    ZEND_PARSE_PARAMETERS_END();

    execute_unsubscribe_command(connection, channels, REQUEST_TYPE_UNSUBSCRIBE, "unsubscribe");

    RETVAL_TRUE;
}

// PUnsubscribe implementation
void valkey_glide_punsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval* patterns = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(patterns)
    ZEND_PARSE_PARAMETERS_END();

    execute_unsubscribe_command(connection, patterns, REQUEST_TYPE_PUNSUBSCRIBE, "punsubscribe");

    RETVAL_TRUE;
}

// Publish implementation
void valkey_glide_publish_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zend_string *channel, *message;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STR(channel)
    Z_PARAM_STR(message)
    ZEND_PARSE_PARAMETERS_END();

    uintptr_t     args[2];
    unsigned long args_len[2];

    args[0]     = (uintptr_t) ZSTR_VAL(channel);
    args_len[0] = ZSTR_LEN(channel);
    args[1]     = (uintptr_t) ZSTR_VAL(message);
    args_len[1] = ZSTR_LEN(message);

    // Call FFI command
    struct CommandResult* result =
        command(connection, 0, REQUEST_TYPE_PUBLISH, 2, args, args_len, NULL, 0, 0);

    if (result) {
        if (result->response && !result->command_error) {
            if (result->response->response_type == Int) {
                RETVAL_LONG(result->response->int_value);
            } else {
                VALKEY_LOG_ERROR("publish", "Unexpected response type from publish command");
                RETVAL_LONG(0);
            }
        } else {
            const char* error_msg =
                result->command_error && result->command_error->command_error_message
                    ? result->command_error->command_error_message
                    : "Publish failed";
            VALKEY_LOG_ERROR("publish", error_msg);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
            RETVAL_FALSE;
        }
        free_command_result(result);
    } else {
        VALKEY_LOG_ERROR("publish", "Publish command failed");
        zend_throw_exception(get_valkey_glide_exception_ce(), "Publish command failed", 0);
        RETVAL_FALSE;
    }
}

// C callback handler for FFI - called from Rust
void valkey_glide_pubsub_callback(uintptr_t      client_adapter_ptr,
                                  enum PushKind  kind,
                                  const uint8_t* message,
                                  int64_t        message_len,
                                  const uint8_t* channel,
                                  int64_t        channel_len,
                                  const uint8_t* pattern,
                                  int64_t        pattern_len) {
    /* Runs on a GLIDE thread. Inactive entries are removed by their PHP thread */
    pubsub_callback_handler(client_adapter_ptr,
                            (int) kind,
                            message,
                            message_len,
                            channel,
                            channel_len,
                            pattern,
                            pattern_len);
}


// Request shutdown: drop the callbacks this request registered (their memory
// is released with the request). Other threads' entries stay (ZTS).
void valkey_glide_pubsub_request_shutdown(void) {
    if (!pubsub_callbacks_initialized) {
        return;
    }
    pubsub_callbacks_after_fork();
    uintptr_t             self  = current_thread_id();
    pubsub_callback_info* owned = NULL; /* removed entries, linked via next_removed */

    mutex_lock(&pubsub_callbacks_mutex);
    zend_string* key;
    void*        ptr;
    ZEND_HASH_FOREACH_STR_KEY_PTR(&pubsub_callbacks, key, ptr) {
        pubsub_callback_info* info = (pubsub_callback_info*) ptr;
        if (info && info->owner_thread == self) {
            info->next_removed = owned;
            owned              = info;
            zend_hash_del(&pubsub_callbacks, key);
        }
    }
    ZEND_HASH_FOREACH_END();
    mutex_unlock(&pubsub_callbacks_mutex);

    /* Entries of clients closed in a callback that a bailout abandoned */
    while (pubsub_detached) {
        pubsub_callback_info* next    = pubsub_detached->next_removed;
        pubsub_detached->next_removed = owned;
        owned                         = pubsub_detached;
        pubsub_detached               = next;
    }

    /* Free outside the mutex: releasing the callback can run PHP code */
    while (owned) {
        pubsub_callback_info* next = owned->next_removed;
        cleanup_callback_info(owned);
        owned = next;
    }
}

// Module shutdown: every request has ended, so the table is empty
void valkey_glide_pubsub_shutdown(void) {
    if (pubsub_callbacks_initialized) {
        /* In a forked child, drop the parent's entries and lock state first */
        pubsub_callbacks_after_fork();
        /* Clear the flag first so nothing looks entries up while the table
         * is being destroyed */
        pubsub_callbacks_initialized = false;
        zend_hash_destroy(&pubsub_callbacks);
        mutex_destroy(&pubsub_callbacks_mutex);
    }
}
