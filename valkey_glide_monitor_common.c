/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor_common.h"

#include <unistd.h>
#include <zend_exceptions.h>

#include "logger.h"

// Global monitor callback storage
static HashTable monitor_callbacks;
static bool      monitor_callbacks_initialized = false;

// Initialize monitor callbacks
void init_monitor_callbacks(void) {
    if (!monitor_callbacks_initialized) {
        zend_hash_init(&monitor_callbacks, 4, NULL, cleanup_monitor_callback_info, 0);
        monitor_callbacks_initialized = true;
    }
}

// Find monitor callback by client key
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
                efree(msg->line);
            efree(msg);
            msg = next;
        }

        mutex_destroy(&info->queue_mutex);
        cond_destroy(&info->queue_cond);
        zval_ptr_dtor(&info->callback);
        Z_DELREF(info->client_obj);

        efree(info);
    }
}

// C callback handler invoked by the FFI/Rust layer from a background thread
// when a new parsed MONITOR line arrives.
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
    if (!monitor_callbacks_initialized) {
        return;
    }

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = zend_hash_str_find(&monitor_callbacks, client_key, key_len);
    if (!callback_zv) {
        return;
    }

    monitor_callback_info* info = (monitor_callback_info*) Z_PTR_P(callback_zv);
    if (!info || !info->is_active) {
        return;
    }

    // Reconstruct monitor line in PHPRedis format:
    // "1339877440.333333 [0 127.0.0.1:6379] \"COMMAND\" \"arg1\" \"arg2\""
    // We allocate a generous buffer and build the string.
    size_t buf_size = 64 + client_addr_len + command_len + args_json_len + 128;
    char*  line_buf = (char*) emalloc(buf_size);
    if (!line_buf)
        return;

    // Format timestamp and header
    int offset = snprintf(line_buf, buf_size, "%.6f [%lld ", timestamp, (long long) db);

    // Append client address
    if (client_addr && client_addr_len > 0) {
        memcpy(line_buf + offset, client_addr, client_addr_len);
        offset += client_addr_len;
    }

    // Close the bracket and add command
    offset += snprintf(line_buf + offset,
                       buf_size - offset,
                       "] \"%.*s\"",
                       (int) command_len,
                       (const char*) command_ptr);

    // Parse and append args from JSON array: ["arg1", "arg2", ...]
    if (args_json && args_json_len > 2) {
        // Simple JSON array parsing — extract string elements
        const char* p   = (const char*) args_json;
        const char* end = p + args_json_len;

        // Skip '['
        if (*p == '[')
            p++;

        while (p < end && *p != ']') {
            // Skip whitespace and commas
            while (p < end && (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t'))
                p++;
            if (p >= end || *p == ']')
                break;

            if (*p == '"') {
                // Parse JSON string
                p++;  // skip opening quote
                const char* str_start = p;
                while (p < end && *p != '"') {
                    if (*p == '\\')
                        p++;  // skip escaped char
                    p++;
                }
                int str_len = (int) (p - str_start);
                if (p < end)
                    p++;  // skip closing quote

                // Append " \"arg\""
                if (offset + str_len + 4 < (int) buf_size) {
                    offset += snprintf(
                        line_buf + offset, buf_size - offset, " \"%.*s\"", str_len, str_start);
                }
            } else {
                // Non-string value (number, null, etc) — shouldn't normally happen for args
                const char* val_start = p;
                while (p < end && *p != ',' && *p != ']')
                    p++;
                int val_len = (int) (p - val_start);
                if (offset + val_len + 2 < (int) buf_size) {
                    offset +=
                        snprintf(line_buf + offset, buf_size - offset, " %.*s", val_len, val_start);
                }
            }
        }
    }

    // Create message node and enqueue
    monitor_message* msg = (monitor_message*) emalloc(sizeof(monitor_message));
    if (!msg) {
        efree(line_buf);
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
    info->client_obj = *client_obj;
    Z_ADDREF(info->client_obj);
    info->is_active = true;

    // Initialize message queue
    info->queue_head = NULL;
    info->queue_tail = NULL;
    mutex_init(&info->queue_mutex);
    cond_init(&info->queue_cond);

    info->in_monitor_mode = false;

    // Store the pointer in a zval using ZVAL_PTR
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
        // Delete from hashtable - this will call cleanup_monitor_callback_info
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
                    efree(msg->line);
                efree(msg);
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
                    if (msg->line)
                        efree(msg->line);
                    efree(msg);
                    break;
                }
                zval_ptr_dtor(&retval);
            }

            zval_ptr_dtor(&php_line);

            if (msg->line)
                efree(msg->line);
            efree(msg);
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

    // Cleanup: stop the monitor client (closes the dedicated connection)
    php_unregister_monitor_callback((uintptr_t) monitor_client_ptr);
    close_monitor_client(monitor_client_ptr);

    RETURN_TRUE;
}

// Shutdown monitor subsystem - called during module shutdown
void valkey_glide_monitor_shutdown(void) {
    if (monitor_callbacks_initialized) {
        zend_hash_destroy(&monitor_callbacks);
        monitor_callbacks_initialized = false;
    }
}
