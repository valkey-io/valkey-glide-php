/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor_common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zend_exceptions.h>

#include "logger.h"

// Native monitor callback registry — accessed from background Rust thread.
// Uses a simple linked list protected by a native mutex (NOT Zend APIs).
typedef struct monitor_registry_entry {
    uintptr_t                      client_ptr;
    monitor_callback_info*         info;
    struct monitor_registry_entry* next;
} monitor_registry_entry;

static monitor_registry_entry* monitor_registry_head = NULL;
static mutex_t                 monitor_registry_mutex;
static bool                    monitor_registry_initialized = false;

// Global monitor callback storage (Zend-side, for PHP-thread-only operations)
static HashTable monitor_callbacks;
static bool      monitor_callbacks_initialized = false;

// Initialize monitor subsystems
void init_monitor_callbacks(void) {
    if (!monitor_callbacks_initialized) {
        zend_hash_init(&monitor_callbacks, 4, NULL, cleanup_monitor_callback_info, 0);
        monitor_callbacks_initialized = true;
    }
    if (!monitor_registry_initialized) {
        mutex_init(&monitor_registry_mutex);
        monitor_registry_initialized = true;
    }
}

// Note: there is intentionally no standalone native_registry_find() helper.
// valkey_glide_monitor_callback() performs the lookup inline while holding
// monitor_registry_mutex for the duration of its use of the returned `info`,
// to close a use-after-free race with concurrent unregistration/cleanup (see
// the comment on valkey_glide_monitor_callback for details).

// Native registry: add entry (thread-safe)
static void native_registry_add(uintptr_t client_ptr, monitor_callback_info* info) {
    monitor_registry_entry* entry =
        (monitor_registry_entry*) malloc(sizeof(monitor_registry_entry));
    if (!entry) {
        // Allocation failure — the callback info remains registered in the
        // Zend-side hashtable, but the background thread won't be able to
        // find it via the native registry, so monitor lines will simply not
        // be delivered rather than crashing.
        return;
    }
    entry->client_ptr = client_ptr;
    entry->info       = info;
    mutex_lock(&monitor_registry_mutex);
    entry->next           = monitor_registry_head;
    monitor_registry_head = entry;
    mutex_unlock(&monitor_registry_mutex);
}

// Native registry: remove entry (thread-safe)
static void native_registry_remove(uintptr_t client_ptr) {
    mutex_lock(&monitor_registry_mutex);
    monitor_registry_entry** pp = &monitor_registry_head;
    while (*pp) {
        if ((*pp)->client_ptr == client_ptr) {
            monitor_registry_entry* to_free = *pp;
            *pp                             = to_free->next;
            free(to_free);
            break;
        }
        pp = &((*pp)->next);
    }
    mutex_unlock(&monitor_registry_mutex);
}

// Find monitor callback by client key (Zend-side, main thread only)
zval* find_monitor_callback(const char* client_key) {
    if (!monitor_callbacks_initialized) {
        return NULL;
    }
    return zend_hash_str_find(&monitor_callbacks, client_key, strlen(client_key));
}

// Cleanup monitor callback info
void cleanup_monitor_callback_info(zval* zv) {
    monitor_callback_info* info = (monitor_callback_info*) Z_PTR_P(zv);
    if (info) {
        // Free all messages in queue
        monitor_message* msg = info->queue_head;
        while (msg) {
            monitor_message* next = msg->next;
            if (msg->line)
                free(msg->line);
            free(msg);
            msg = next;
        }

        mutex_destroy(&info->queue_mutex);
        cond_destroy(&info->queue_cond);
        zval_ptr_dtor(&info->callback);
        zval_ptr_dtor(&info->client_obj);

        efree(info);
    }
}

// C callback handler invoked by the FFI/Rust layer from a background thread
// when a new parsed MONITOR line arrives.
//
// CRITICAL: This runs on a background Rust thread — NO Zend APIs allowed here.
// We use only native C: malloc, mutex, and the native registry.
//
// This matches the MonitorCallback signature from glide_bindings.h:
//   void (*MonitorCallback)(uintptr_t client_ptr, double timestamp, int64_t db,
//                           const uint8_t *client_addr, int64_t client_addr_len,
//                           const uint8_t *command, int64_t command_len,
//                           const uint8_t *args_json, int64_t args_json_len)
//
// We reconstruct a human-readable monitor line matching the format that
// PHPRedis users expect: "timestamp [db client_addr] \"COMMAND\" \"arg1\" ..."
//
// LIFETIME SAFETY: the registry lookup and all use of `info` below happen
// while holding monitor_registry_mutex for the *entire* function, not just
// the lookup. php_unregister_monitor_callback() also acquires this same
// mutex before removing the registry entry and before the Zend-side hashtable
// deletion frees `info` (destroying its mutex/cond and efree'ing the struct).
// Serializing on the single registry mutex guarantees this callback can never
// be left holding a pointer to `info` that is concurrently freed — either this
// function runs to completion first, or the unregister/removal runs first and
// this function will simply fail the lookup.
void valkey_glide_monitor_callback(uintptr_t      client_ptr,
                                   double         timestamp,
                                   int64_t        db,
                                   const uint8_t* client_addr,
                                   int64_t        client_addr_len,
                                   const uint8_t* command_ptr,
                                   int64_t        command_len,
                                   const uint8_t* args_json,
                                   int64_t        args_json_len) {
    char*            line_buf = NULL;
    monitor_message* msg      = NULL;

    mutex_lock(&monitor_registry_mutex);

    monitor_callback_info*  info  = NULL;
    monitor_registry_entry* entry = monitor_registry_head;
    while (entry) {
        if (entry->client_ptr == client_ptr) {
            info = entry->info;
            break;
        }
        entry = entry->next;
    }

    if (!info || !info->is_active) {
        goto unlock_registry;
    }

    {
        // Reconstruct monitor line in PHPRedis format:
        // "1339877440.333333 [0 127.0.0.1:6379] \"COMMAND\" \"arg1\" \"arg2\""
        // We allocate a generous buffer and build the string.
        size_t buf_size = 64 + client_addr_len + command_len + args_json_len + 128;
        line_buf        = (char*) malloc(buf_size);
        if (!line_buf)
            goto unlock_registry;

        int    offset    = 0;
        size_t remaining = buf_size;

        // Format timestamp and header
        int n = snprintf(line_buf, remaining, "%.6f [%lld ", timestamp, (long long) db);
        if (n < 0)
            goto free_line;
        if ((size_t) n >= remaining)
            n = (int) remaining - 1;
        offset += n;
        remaining = buf_size - offset;

        // Append client address
        if (client_addr && client_addr_len > 0 && (size_t) client_addr_len < remaining) {
            memcpy(line_buf + offset, client_addr, client_addr_len);
            offset += client_addr_len;
            remaining = buf_size - offset;
        }

        // Close the bracket and add command. Guard against a NULL command_ptr —
        // "%.*s" with a NULL pointer is undefined behavior even with precision 0
        // on some libc implementations.
        const char* command_str      = command_ptr ? (const char*) command_ptr : "";
        int         command_len_safe = command_ptr ? (int) command_len : 0;
        n = snprintf(line_buf + offset, remaining, "] \"%.*s\"", command_len_safe, command_str);
        if (n < 0)
            goto free_line;
        if ((size_t) n >= remaining)
            n = (int) remaining - 1;
        offset += n;
        remaining = buf_size - offset;

        // Parse and append args from JSON array: ["arg1", "arg2", ...]
        if (args_json && args_json_len > 2 && remaining > 4) {
            // Simple JSON array parsing — extract string elements and decode escapes
            const char* p   = (const char*) args_json;
            const char* end = p + args_json_len;

            // Skip '['
            if (*p == '[')
                p++;

            while (p < end && *p != ']' && remaining > 4) {
                // Skip whitespace and commas
                while (p < end &&
                       (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t'))
                    p++;
                if (p >= end || *p == ']')
                    break;

                if (*p == '"') {
                    // Parse JSON string with escape decoding
                    p++;  // skip opening quote
                    // Decode into a temporary buffer
                    char* decoded     = (char*) malloc(end - p + 1);
                    int   decoded_len = 0;
                    if (!decoded)
                        break;

                    while (p < end && *p != '"') {
                        if (*p == '\\' && (p + 1) < end) {
                            p++;  // skip backslash
                            switch (*p) {
                                case '"':
                                    decoded[decoded_len++] = '"';
                                    break;
                                case '\\':
                                    decoded[decoded_len++] = '\\';
                                    break;
                                case 'n':
                                    decoded[decoded_len++] = '\n';
                                    break;
                                case 'r':
                                    decoded[decoded_len++] = '\r';
                                    break;
                                case 't':
                                    decoded[decoded_len++] = '\t';
                                    break;
                                case '/':
                                    decoded[decoded_len++] = '/';
                                    break;
                                default:
                                    decoded[decoded_len++] = *p;
                                    break;
                            }
                            p++;
                        } else {
                            decoded[decoded_len++] = *p++;
                        }
                    }
                    if (p < end)
                        p++;  // skip closing quote

                    // Append " \"decoded_arg\""
                    n = snprintf(line_buf + offset, remaining, " \"%.*s\"", decoded_len, decoded);
                    free(decoded);
                    if (n < 0)
                        break;
                    if ((size_t) n >= remaining)
                        n = (int) remaining - 1;
                    offset += n;
                    remaining = buf_size - offset;
                } else {
                    // Non-string value (number, null, etc) — shouldn't normally happen for args
                    const char* val_start = p;
                    while (p < end && *p != ',' && *p != ']')
                        p++;
                    int val_len = (int) (p - val_start);
                    n = snprintf(line_buf + offset, remaining, " %.*s", val_len, val_start);
                    if (n < 0)
                        break;
                    if ((size_t) n >= remaining)
                        n = (int) remaining - 1;
                    offset += n;
                    remaining = buf_size - offset;
                }
            }
        }

        // Create message node and enqueue (using malloc — this is a background thread)
        msg = (monitor_message*) malloc(sizeof(monitor_message));
        if (!msg)
            goto free_line;

        msg->line     = line_buf;
        msg->line_len = offset;
        msg->next     = NULL;

        // Add to queue (thread-safe) with bounded depth
        mutex_lock(&info->queue_mutex);
        if (info->queue_depth >= MONITOR_QUEUE_MAX_DEPTH) {
            // Queue full — drop the message to prevent unbounded memory growth,
            // but record the loss so it can be surfaced to the PHP callback
            // instead of silently disappearing.
            info->dropped_count++;
            mutex_unlock(&info->queue_mutex);
            goto free_msg_and_line;
        }
        if (info->queue_tail) {
            info->queue_tail->next = msg;
        } else {
            info->queue_head = msg;
        }
        info->queue_tail = msg;
        info->queue_depth++;
        cond_signal(&info->queue_cond);
        mutex_unlock(&info->queue_mutex);
    }

    mutex_unlock(&monitor_registry_mutex);
    return;

free_msg_and_line:
    free(msg);
free_line:
    free(line_buf);
unlock_registry:
    mutex_unlock(&monitor_registry_mutex);
}

// Register monitor callback
void php_register_monitor_callback(uintptr_t client_ptr, zval* callback, zval* client_obj) {
    init_monitor_callbacks();

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    monitor_callback_info* info = emalloc(sizeof(monitor_callback_info));

    // Copy the callback and reference the client object
    ZVAL_COPY(&info->callback, callback);
    ZVAL_COPY(&info->client_obj, client_obj);
    info->is_active = true;

    // Initialize message queue
    info->queue_head    = NULL;
    info->queue_tail    = NULL;
    info->queue_depth   = 0;
    info->dropped_count = 0;
    mutex_init(&info->queue_mutex);
    cond_init(&info->queue_cond);

    info->in_monitor_mode = false;

    // Register in native registry first (background thread uses this)
    native_registry_add(client_ptr, info);

    // Store the pointer in a zval using ZVAL_PTR (Zend-side for PHP thread cleanup)
    zval callback_zv;
    ZVAL_PTR(&callback_zv, info);
    zend_hash_str_update(&monitor_callbacks, client_key, key_len, &callback_zv);
}

// Unregister monitor callback
void php_unregister_monitor_callback(uintptr_t client_ptr) {
    if (!monitor_callbacks_initialized)
        return;

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = zend_hash_str_find(&monitor_callbacks, client_key, key_len);
    if (callback_zv) {
        monitor_callback_info* info = (monitor_callback_info*) Z_PTR_P(callback_zv);
        if (info) {
            info->is_active = false;
            cond_signal(&info->queue_cond);
        }
    }

    // Remove from native registry first (stops background thread from finding it)
    native_registry_remove(client_ptr);

    // Delete from hashtable - this will call cleanup_monitor_callback_info
    if (callback_zv) {
        zend_hash_str_del(&monitor_callbacks, client_key, key_len);
    }
}

// Invoke the PHP monitor callback with a single line of text.
// Returns true if the blocking loop should continue, false if it should stop
// (callback threw, callback returned a non-null value, or invocation failed).
static bool invoke_monitor_callback(monitor_callback_info* info,
                                    const char*            line,
                                    size_t                 line_len) {
    zval php_line;
    ZVAL_STRINGL(&php_line, line, line_len);

    zval args[2];
    args[0] = info->client_obj;
    args[1] = php_line;

    zval retval;
    ZVAL_UNDEF(&retval);

    bool should_continue = true;

    if (call_user_function(NULL, NULL, &info->callback, &retval, 2, args) == SUCCESS) {
        if (EG(exception)) {
            // Callback threw — stop the loop and let the pending exception
            // propagate once we return to PHP.
            should_continue = false;
        } else if (Z_TYPE(retval) != IS_NULL) {
            // Non-null return exits monitor mode (matches PHPRedis behavior).
            should_continue = false;
        }
        zval_ptr_dtor(&retval);
    } else {
        // call_user_function failed outright — stop regardless of exception state.
        should_continue = false;
    }

    zval_ptr_dtor(&php_line);
    return should_continue;
}

// Monitor blocking loop - dequeues messages and invokes the PHP callback
static void monitor_blocking_loop(uintptr_t monitor_client_ptr) {
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) monitor_client_ptr);
    zval* callback_zv = find_monitor_callback(client_key);
    if (!callback_zv)
        return;

    monitor_callback_info* info = (monitor_callback_info*) Z_PTR_P(callback_zv);
    info->in_monitor_mode       = true;

    while (info->is_active) {
        monitor_message* msg     = NULL;
        size_t           dropped = 0;

        mutex_lock(&info->queue_mutex);
        // Wait for messages with a timed wait to allow periodic re-checks of
        // is_active (prevents blocking forever when no traffic arrives).
        while (!info->queue_head && info->is_active) {
            // Ignore the return value here — both a clean wakeup and a timeout
            // fall through to the same re-check of queue_head/is_active below.
            (void) cond_timedwait(&info->queue_cond, &info->queue_mutex, 1000);  // 1 second timeout
            // Honor PHP-level interruption requests (e.g. max_execution_time,
            // pcntl signals, request shutdown) so an idle monitor with no
            // traffic doesn't block the request forever.
            // EG(vm_interrupt) became a zend_atomic_bool (requiring the
            // load accessor) in newer Zend Engine versions; older versions
            // expose it as a plain boolean.
#if PHP_VERSION_ID >= 80500
            if (zend_atomic_bool_load_ex(&EG(vm_interrupt))) {
#else
            if (EG(vm_interrupt)) {
#endif
                info->is_active = false;
                break;
            }
        }
        if (info->queue_head) {
            msg              = info->queue_head;
            info->queue_head = msg->next;
            if (!info->queue_head) {
                info->queue_tail = NULL;
            }
            info->queue_depth--;
        }
        // Snapshot and reset any messages dropped due to a full queue so the
        // loss can be surfaced to the PHP callback instead of vanishing silently.
        dropped             = info->dropped_count;
        info->dropped_count = 0;
        mutex_unlock(&info->queue_mutex);

        if (!info->is_active) {
            if (msg) {
                if (msg->line)
                    free(msg->line);
                free(msg);
            }
            break;
        }

        if (dropped > 0) {
            char overflow_line[128];
            int  overflow_len =
                snprintf(overflow_line,
                         sizeof(overflow_line),
                         "*** MONITOR OVERFLOW: %zu command(s) dropped (queue full) ***",
                         dropped);
            if (overflow_len > 0) {
                if (!invoke_monitor_callback(info,
                                             overflow_line,
                                             (size_t) overflow_len < sizeof(overflow_line)
                                                 ? (size_t) overflow_len
                                                 : sizeof(overflow_line) - 1)) {
                    if (msg) {
                        free(msg->line);
                        free(msg);
                    }
                    break;
                }
            }
        }

        if (msg) {
            bool should_continue = invoke_monitor_callback(info, msg->line, (size_t) msg->line_len);
            free(msg->line);
            free(msg);
            if (!should_continue) {
                break;
            }
        }
    }

    info->in_monitor_mode = false;
}

// Monitor implementation - matches PHPRedis signature: monitor(callable $cb)
//
// Creates a dedicated MonitorClient connection (separate from the main client)
// using create_monitor_client() FFI function, enters a blocking loop invoking
// the callback for each monitor line, and cleans up on exit.
void valkey_glide_monitor_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval* callback;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_ZVAL(callback)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Callback must be callable", 0);
        RETURN_FALSE;
    }

    // Get the object to access stored connection request bytes
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, ZEND_THIS);

    if (!valkey_glide->connection_request_bytes || valkey_glide->connection_request_len == 0) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "Cannot start monitor: connection request bytes not available. "
                             "Ensure the client is connected before calling monitor().",
                             0);
        RETURN_FALSE;
    }

    if (valkey_glide->in_monitor_mode) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Client is already in monitor mode", 0);
        RETURN_FALSE;
    }

    // Create a dedicated monitor client connection using the FFI function.
    // This opens a new TCP connection and sends the MONITOR command.
    const struct ConnectionResponse* mon_resp =
        create_monitor_client(valkey_glide->connection_request_bytes,
                              valkey_glide->connection_request_len,
                              valkey_glide_monitor_callback);

    if (!mon_resp) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to create monitor client: null response", 0);
        RETURN_FALSE;
    }

    if (mon_resp->connection_error_message) {
        const char* error_msg = mon_resp->connection_error_message;
        VALKEY_LOG_ERROR("monitor", error_msg);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
        free_connection_response((struct ConnectionResponse*) mon_resp);
        RETURN_FALSE;
    }

    const void* monitor_client_ptr = mon_resp->conn_ptr;
    free_connection_response((struct ConnectionResponse*) mon_resp);

    if (!monitor_client_ptr) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to create monitor client: null client ptr", 0);
        RETURN_FALSE;
    }

    // Register the PHP callback using the monitor client pointer as key
    // (not the main client pointer, since the monitor has its own connection)
    php_register_monitor_callback((uintptr_t) monitor_client_ptr, callback, ZEND_THIS);

    // Mark the client object as being in monitor mode
    valkey_glide->in_monitor_mode = true;

    // Enter blocking loop waiting for monitor messages
    monitor_blocking_loop((uintptr_t) monitor_client_ptr);

    // Cleanup: close the monitor client FIRST (stops the stream and prevents
    // further callbacks), THEN unregister (frees callback state).
    // This order ensures the background thread can't fire into freed memory.
    close_monitor_client(monitor_client_ptr);
    php_unregister_monitor_callback((uintptr_t) monitor_client_ptr);

    // Clear the monitor mode flag
    valkey_glide->in_monitor_mode = false;

    RETURN_TRUE;
}

// Shutdown monitor subsystem - called during module shutdown
void valkey_glide_monitor_shutdown(void) {
    if (monitor_registry_initialized) {
        // Snapshot active client pointers under the lock, mark inactive, then
        // release the lock BEFORE closing clients. This avoids a deadlock where
        // close_monitor_client() waits for the Rust producer task to exit while
        // that task's callback (valkey_glide_monitor_callback) is blocked trying
        // to acquire monitor_registry_mutex for its own registry lookup.
        uintptr_t* client_ptrs  = NULL;
        size_t     client_count = 0;

        mutex_lock(&monitor_registry_mutex);
        // Count entries
        monitor_registry_entry* entry = monitor_registry_head;
        while (entry) {
            client_count++;
            entry = entry->next;
        }
        // Snapshot and mark inactive
        if (client_count > 0) {
            client_ptrs = (uintptr_t*) malloc(client_count * sizeof(uintptr_t));
            entry       = monitor_registry_head;
            for (size_t i = 0; entry; i++) {
                // Always mark inactive and wake any blocked consumer, even if
                // the snapshot allocation below failed — that only affects our
                // ability to proactively close_monitor_client() further down.
                if (entry->info) {
                    entry->info->is_active = false;
                    cond_signal(&entry->info->queue_cond);
                }
                if (client_ptrs) {
                    client_ptrs[i] = entry->client_ptr;
                }
                entry = entry->next;
            }
            if (!client_ptrs) {
                // Allocation failed — skip the proactive close loop below.
                // Each monitor's blocking loop will still notice is_active is
                // false (via its timed wait) and exit on its own.
                client_count = 0;
            }
        }
        mutex_unlock(&monitor_registry_mutex);

        // Close monitor clients WITHOUT holding the registry mutex.
        // The producer callback can still safely run its registry lookup —
        // it will find info->is_active == false and return early (it also
        // acquires monitor_registry_mutex itself, so it simply waits its turn
        // rather than racing shutdown).
        for (size_t i = 0; i < client_count; i++) {
            close_monitor_client((const void*) client_ptrs[i]);
        }
        free(client_ptrs);
    }

    if (monitor_callbacks_initialized) {
        // Now safe to destroy callback state — no background thread is active
        zend_hash_destroy(&monitor_callbacks);
        monitor_callbacks_initialized = false;
    }

    if (monitor_registry_initialized) {
        // Free the native registry entries
        mutex_lock(&monitor_registry_mutex);
        monitor_registry_entry* entry = monitor_registry_head;
        while (entry) {
            monitor_registry_entry* next = entry->next;
            free(entry);
            entry = next;
        }
        monitor_registry_head = NULL;
        mutex_unlock(&monitor_registry_mutex);
        mutex_destroy(&monitor_registry_mutex);
        monitor_registry_initialized = false;
    }
}
