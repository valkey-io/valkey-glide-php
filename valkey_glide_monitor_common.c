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

// Native registry: find info by client_ptr (thread-safe, no Zend APIs)
static monitor_callback_info* native_registry_find(uintptr_t client_ptr) {
    monitor_callback_info* result = NULL;
    mutex_lock(&monitor_registry_mutex);
    monitor_registry_entry* entry = monitor_registry_head;
    while (entry) {
        if (entry->client_ptr == client_ptr) {
            result = entry->info;
            break;
        }
        entry = entry->next;
    }
    mutex_unlock(&monitor_registry_mutex);
    return result;
}

// Native registry: add entry (thread-safe)
static void native_registry_add(uintptr_t client_ptr, monitor_callback_info* info) {
    monitor_registry_entry* entry =
        (monitor_registry_entry*) malloc(sizeof(monitor_registry_entry));
    entry->client_ptr = client_ptr;
    entry->info       = info;
    mutex_lock(&monitor_registry_mutex);
    entry->next          = monitor_registry_head;
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
void valkey_glide_monitor_callback(uintptr_t      client_ptr,
                                   double         timestamp,
                                   int64_t        db,
                                   const uint8_t* client_addr,
                                   int64_t        client_addr_len,
                                   const uint8_t* command_ptr,
                                   int64_t        command_len,
                                   const uint8_t* args_json,
                                   int64_t        args_json_len) {
    // Look up callback info using native registry (no Zend APIs)
    monitor_callback_info* info = native_registry_find(client_ptr);
    if (!info || !info->is_active) {
        return;
    }

    // Reconstruct monitor line in PHPRedis format:
    // "1339877440.333333 [0 127.0.0.1:6379] \"COMMAND\" \"arg1\" \"arg2\""
    // We allocate a generous buffer and build the string.
    size_t buf_size = 64 + client_addr_len + command_len + args_json_len + 128;
    char*  line_buf = (char*) malloc(buf_size);
    if (!line_buf)
        return;

    int    offset    = 0;
    size_t remaining = buf_size;

    // Format timestamp and header
    int n = snprintf(line_buf, remaining, "%.6f [%lld ", timestamp, (long long) db);
    if (n < 0) { free(line_buf); return; }
    if ((size_t)n >= remaining) n = (int)remaining - 1;
    offset += n;
    remaining = buf_size - offset;

    // Append client address
    if (client_addr && client_addr_len > 0 && (size_t)client_addr_len < remaining) {
        memcpy(line_buf + offset, client_addr, client_addr_len);
        offset += client_addr_len;
        remaining = buf_size - offset;
    }

    // Close the bracket and add command
    n = snprintf(line_buf + offset, remaining, "] \"%.*s\"", (int) command_len, (const char*) command_ptr);
    if (n < 0) { free(line_buf); return; }
    if ((size_t)n >= remaining) n = (int)remaining - 1;
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
            while (p < end && (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t'))
                p++;
            if (p >= end || *p == ']')
                break;

            if (*p == '"') {
                // Parse JSON string with escape decoding
                p++;  // skip opening quote
                // Decode into a temporary buffer
                char*  decoded     = (char*) malloc(end - p + 1);
                int    decoded_len = 0;
                if (!decoded) break;

                while (p < end && *p != '"') {
                    if (*p == '\\' && (p + 1) < end) {
                        p++;  // skip backslash
                        switch (*p) {
                            case '"':  decoded[decoded_len++] = '"';  break;
                            case '\\': decoded[decoded_len++] = '\\'; break;
                            case 'n':  decoded[decoded_len++] = '\n'; break;
                            case 'r':  decoded[decoded_len++] = '\r'; break;
                            case 't':  decoded[decoded_len++] = '\t'; break;
                            case '/':  decoded[decoded_len++] = '/';  break;
                            default:   decoded[decoded_len++] = *p;   break;
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
                if (n < 0) break;
                if ((size_t)n >= remaining) n = (int)remaining - 1;
                offset += n;
                remaining = buf_size - offset;
            } else {
                // Non-string value (number, null, etc) — shouldn't normally happen for args
                const char* val_start = p;
                while (p < end && *p != ',' && *p != ']')
                    p++;
                int val_len = (int) (p - val_start);
                n = snprintf(line_buf + offset, remaining, " %.*s", val_len, val_start);
                if (n < 0) break;
                if ((size_t)n >= remaining) n = (int)remaining - 1;
                offset += n;
                remaining = buf_size - offset;
            }
        }
    }

    // Create message node and enqueue (using malloc — this is a background thread)
    monitor_message* msg = (monitor_message*) malloc(sizeof(monitor_message));
    if (!msg) {
        free(line_buf);
        return;
    }

    msg->line     = line_buf;
    msg->line_len = offset;
    msg->next     = NULL;

    // Add to queue (thread-safe)
    mutex_lock(&info->queue_mutex);
    if (info->queue_tail) {
        info->queue_tail->next = msg;
    } else {
        info->queue_head = msg;
    }
    info->queue_tail = msg;
    cond_signal(&info->queue_cond);
    mutex_unlock(&info->queue_mutex);
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
    info->queue_head = NULL;
    info->queue_tail = NULL;
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

// Check if client is in monitor mode
bool is_client_in_monitor_mode(uintptr_t client_ptr) {
    if (!monitor_callbacks_initialized)
        return false;

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = find_monitor_callback(client_key);
    if (!callback_zv)
        return false;

    monitor_callback_info* info = (monitor_callback_info*) Z_PTR_P(callback_zv);
    return info ? info->in_monitor_mode : false;
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
        monitor_message* msg = NULL;

        mutex_lock(&info->queue_mutex);
        // Wait for messages. Woken by:
        // 1. New message enqueued (cond_signal from callback)
        // 2. is_active set to false (cond_signal from unregister)
        while (!info->queue_head && info->is_active) {
            cond_wait(&info->queue_cond, &info->queue_mutex);
        }
        if (info->queue_head) {
            msg              = info->queue_head;
            info->queue_head = msg->next;
            if (!info->queue_head) {
                info->queue_tail = NULL;
            }
        }
        mutex_unlock(&info->queue_mutex);

        if (!info->is_active) {
            if (msg) {
                if (msg->line)
                    free(msg->line);
                free(msg);
            }
            break;
        }

        if (msg) {
            zval php_line;
            ZVAL_STRINGL(&php_line, msg->line, msg->line_len);

            zval args[2];
            args[0] = info->client_obj;
            args[1] = php_line;

            zval retval;
            ZVAL_UNDEF(&retval);

            if (call_user_function(NULL, NULL, &info->callback, &retval, 2, args) == SUCCESS) {
                // If callback returns non-null, exit monitor mode (matches PHPRedis behavior)
                if (Z_TYPE(retval) != IS_NULL) {
                    zval_ptr_dtor(&retval);
                    zval_ptr_dtor(&php_line);
                    free(msg->line);
                    free(msg);
                    break;
                }
                zval_ptr_dtor(&retval);
            } else {
                // call_user_function failed — exit loop
                zval_ptr_dtor(&php_line);
                free(msg->line);
                free(msg);
                break;
            }

            // Check if callback threw an exception
            if (EG(exception)) {
                zval_ptr_dtor(&php_line);
                free(msg->line);
                free(msg);
                break;
            }

            zval_ptr_dtor(&php_line);

            free(msg->line);
            free(msg);
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

    if (is_client_in_monitor_mode((uintptr_t) connection)) {
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

    // Enter blocking loop waiting for monitor messages
    monitor_blocking_loop((uintptr_t) monitor_client_ptr);

    // Cleanup: close the monitor client FIRST (stops the stream and prevents
    // further callbacks), THEN unregister (frees callback state).
    // This order ensures the background thread can't fire into freed memory.
    close_monitor_client(monitor_client_ptr);
    php_unregister_monitor_callback((uintptr_t) monitor_client_ptr);

    RETURN_TRUE;
}

// Shutdown monitor subsystem - called during module shutdown
void valkey_glide_monitor_shutdown(void) {
    if (monitor_callbacks_initialized) {
        zend_hash_destroy(&monitor_callbacks);
        monitor_callbacks_initialized = false;
    }
    if (monitor_registry_initialized) {
        // Free any remaining native registry entries
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
