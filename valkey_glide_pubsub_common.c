/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_pubsub_common.h"
#include <zend_exceptions.h>
#include <unistd.h>

// Mutex wrapper functions
void mutex_init(mutex_t *m) {
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

void mutex_lock(mutex_t *m) {
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

void mutex_unlock(mutex_t *m) {
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

void mutex_destroy(mutex_t *m) {
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

// Global pubsub callback storage
static HashTable pubsub_callbacks;
static bool pubsub_callbacks_initialized = false;

// Initialize pubsub callbacks
void init_pubsub_callbacks(void) {
    if (!pubsub_callbacks_initialized) {
        zend_hash_init(&pubsub_callbacks, 16, NULL, cleanup_callback_info, 0);
        pubsub_callbacks_initialized = true;
    }
}

// Find pubsub callback by client key
zval* find_pubsub_callback(const char *client_key) {
    if (!pubsub_callbacks_initialized) {
        return NULL;
    }
    return zend_hash_str_find(&pubsub_callbacks, client_key, strlen(client_key));
}

// Remove pubsub callback by client key
void remove_pubsub_callback(const char *client_key) {
    if (pubsub_callbacks_initialized) {
        zend_hash_str_del(&pubsub_callbacks, client_key, strlen(client_key));
    }
}

// Cleanup callback info
void cleanup_callback_info(zval *zv) {
    pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(zv);
    if (info) {
        // Free all messages in queue
        pubsub_message *msg = info->queue_head;
        while (msg) {
            pubsub_message *next = msg->next;
            if (msg->channel) free(msg->channel);
            if (msg->message) free(msg->message);
            if (msg->pattern) free(msg->pattern);
            free(msg);
            msg = next;
        }
        
        mutex_destroy(&info->queue_mutex);
        if (Z_TYPE(info->callback) != IS_UNDEF) {
            zval_ptr_dtor(&info->callback);
        }
        if (Z_TYPE(info->client_obj) != IS_UNDEF) {
            zval_ptr_dtor(&info->client_obj);
        }
        efree(info);
    }
}

// Cleanup callback info for pointer-based storage
void cleanup_callback_info_ptr(void *ptr) {
    pubsub_callback_info *info = (pubsub_callback_info *)ptr;
    if (info) {
        zval_dtor(&info->callback);
        zval_dtor(&info->client_obj);
        efree(info);
    }
}

// C callback handler for FFI
void pubsub_callback_handler(
    uintptr_t client_ptr,
    int kind,
    const uint8_t *message,
    int64_t message_len,
    const uint8_t *channel,
    int64_t channel_len,
    const uint8_t *pattern,
    int64_t pattern_len
) {
    if (!pubsub_callbacks_initialized) {
        return;
    }

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)client_ptr);
    
    zval *callback_zv = zend_hash_str_find(&pubsub_callbacks, client_key, strlen(client_key));
    if (!callback_zv) {
        return;
    }

    pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(callback_zv);
    if (!info || !info->is_active) {
        return;
    }

    // Only handle message types
    if (kind != 3 && kind != 4 && kind != 5) {
        return;
    }

    // Allocate and populate message node
    pubsub_message *msg = (pubsub_message *)malloc(sizeof(pubsub_message));
    if (!msg) return;
    
    msg->kind = kind;
    msg->next = NULL;
    
    // Copy channel
    msg->channel = (uint8_t *)malloc(channel_len);
    if (msg->channel) {
        memcpy(msg->channel, channel, channel_len);
        msg->channel_len = channel_len;
    } else {
        free(msg);
        return;
    }
    
    // Copy message
    msg->message = (uint8_t *)malloc(message_len);
    if (msg->message) {
        memcpy(msg->message, message, message_len);
        msg->message_len = message_len;
    } else {
        free(msg->channel);
        free(msg);
        return;
    }
    
    // Copy pattern if present
    if (pattern && pattern_len > 0) {
        msg->pattern = (uint8_t *)malloc(pattern_len);
        if (msg->pattern) {
            memcpy(msg->pattern, pattern, pattern_len);
            msg->pattern_len = pattern_len;
        } else {
            free(msg->message);
            free(msg->channel);
            free(msg);
            return;
        }
    } else {
        msg->pattern = NULL;
        msg->pattern_len = 0;
    }
    
    // Add to queue (thread-safe)
    mutex_lock(&info->queue_mutex);
    if (info->queue_tail) {
        info->queue_tail->next = msg;
    } else {
        info->queue_head = msg;
    }
    info->queue_tail = msg;
    mutex_unlock(&info->queue_mutex);
}

// Register callback
void register_pubsub_callback(uintptr_t client_ptr, zval *callback, zval *client_obj) {
    init_pubsub_callbacks();

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)client_ptr);

    pubsub_callback_info *info = emalloc(sizeof(pubsub_callback_info));
    
    // Copy the callback and client object
    ZVAL_COPY(&info->callback, callback);
    ZVAL_COPY(&info->client_obj, client_obj);
    info->is_active = true;
    
    // Initialize message queue
    info->queue_head = NULL;
    info->queue_tail = NULL;
    mutex_init(&info->queue_mutex);

    // Store the pointer in a zval using ZVAL_PTR
    zval callback_zv;
    ZVAL_PTR(&callback_zv, info);
    zend_hash_str_update(&pubsub_callbacks, client_key, strlen(client_key), &callback_zv);
}

// Unregister callback
void unregister_pubsub_callback(uintptr_t client_ptr) {
    if (!pubsub_callbacks_initialized) return;

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)client_ptr);

    zval *callback_zv = zend_hash_str_find(&pubsub_callbacks, client_key, strlen(client_key));
    if (callback_zv) {
        pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(callback_zv);
        if (info) {
            info->is_active = false;
        }
        // Delete from hashtable - this will call cleanup_callback_info
        zend_hash_str_del(&pubsub_callbacks, client_key, strlen(client_key));
    }
}

// Subscribe implementation
void valkey_glide_subscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *channels, *callback;
    zend_long timeout_ms = 0; // Default timeout (0 = wait indefinitely)
    
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ARRAY(channels)
        Z_PARAM_ZVAL(callback)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(timeout_ms)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(zend_ce_exception, "Callback must be callable", 0);
        RETURN_FALSE;
    }

    // Register callback
    register_pubsub_callback((uintptr_t)connection, callback, ZEND_THIS);

    // Convert channels array to arguments
    HashTable *channels_ht = Z_ARRVAL_P(channels);
    uint32_t channel_count = zend_hash_num_elements(channels_ht);
    
    // For blocking subscribe, we need channels + timeout_ms
    uint32_t total_args = channel_count + 1;
    uintptr_t *args = emalloc(total_args * sizeof(uintptr_t));
    unsigned long *args_len = emalloc(total_args * sizeof(unsigned long));
    
    // Add channels
    uint32_t i = 0;
    zval *channel_zv;
    ZEND_HASH_FOREACH_VAL(channels_ht, channel_zv) {
        convert_to_string(channel_zv);
        args[i] = (uintptr_t)Z_STRVAL_P(channel_zv);
        args_len[i] = Z_STRLEN_P(channel_zv);
        i++;
    } ZEND_HASH_FOREACH_END();
    
    // Add timeout_ms as string (like Python does)
    char timeout_str[32];
    snprintf(timeout_str, sizeof(timeout_str), "%lld", (long long)timeout_ms);
    args[channel_count] = (uintptr_t)timeout_str;
    args_len[channel_count] = strlen(timeout_str);

    // Call FFI command
    struct CommandResult* result = command(
        connection, 0, REQUEST_TYPE_SUBSCRIBE,
        total_args, args, args_len, NULL, 0, 0
    );

    efree(args);
    efree(args_len);

    if (!result || result->command_error) {
        if (result) free_command_result(result);
        zend_throw_exception(zend_ce_exception, "Subscribe command failed", 0);
        RETVAL_FALSE;
        return;
    }
    free_command_result(result);
    
    // Get callback info for blocking loop
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)connection);
    zval *callback_zv = find_pubsub_callback(client_key);
    if (!callback_zv) {
        RETVAL_FALSE;
        return;
    }
    
    pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(callback_zv);
    
    // Blocking loop: poll queue and invoke callback
    while (info->is_active) {
        pubsub_message *msg = NULL;
        
        // Pop message from queue (thread-safe)
        mutex_lock(&info->queue_mutex);
        if (info->queue_head) {
            msg = info->queue_head;
            info->queue_head = msg->next;
            if (!info->queue_head) {
                info->queue_tail = NULL;
            }
        }
        mutex_unlock(&info->queue_mutex);
        
        if (msg) {
            // Invoke PHP callback
            zval php_channel, php_message, php_pattern;
            ZVAL_STRINGL(&php_channel, (char*)msg->channel, msg->channel_len);
            ZVAL_STRINGL(&php_message, (char*)msg->message, msg->message_len);
            
            if (msg->pattern && msg->pattern_len > 0) {
                ZVAL_STRINGL(&php_pattern, (char*)msg->pattern, msg->pattern_len);
            } else {
                ZVAL_NULL(&php_pattern);
            }

            // Increment refcount before passing to call_user_function
            Z_ADDREF(info->client_obj);
            
            zval args[4];
            args[0] = info->client_obj;
            args[1] = php_channel;
            args[2] = php_message;
            args[3] = php_pattern;

            zval retval;
            ZVAL_UNDEF(&retval);
            int arg_count = (msg->pattern && msg->pattern_len > 0) ? 4 : 3;
            
            if (call_user_function(NULL, NULL, &info->callback, &retval, arg_count, args) == SUCCESS) {
                zval_ptr_dtor(&retval);
            }
            
            // Decrement refcount after call
            Z_DELREF(info->client_obj);

            zval_ptr_dtor(&php_channel);
            zval_ptr_dtor(&php_message);
            if (msg->pattern && msg->pattern_len > 0) {
                zval_ptr_dtor(&php_pattern);
            }
            
            // Free message
            if (msg->channel) free(msg->channel);
            if (msg->message) free(msg->message);
            if (msg->pattern) free(msg->pattern);
            free(msg);
        } else {
            // No message, sleep briefly to avoid busy-wait
            usleep(1000); // 1ms
        }
    }
    
    // After loop exits, send unsubscribe to clean up server-side
    struct CommandResult* unsub_result = command(
        connection, 0, REQUEST_TYPE_UNSUBSCRIBE,
        0, NULL, NULL, NULL, 0, 0
    );
    if (unsub_result) {
        free_command_result(unsub_result);
    }
    
    // Give Rust thread time to process unsubscribe
    usleep(10000); // 10ms
    
    // Unregister callback to prevent Rust thread from accessing freed memory
    unregister_pubsub_callback((uintptr_t)connection);
    
    RETVAL_TRUE;
}

// PSubscribe implementation
void valkey_glide_psubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *patterns, *callback;
    zend_long timeout_ms = 0; // Default timeout (0 = wait indefinitely)
    
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ARRAY(patterns)
        Z_PARAM_ZVAL(callback)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(timeout_ms)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(zend_ce_exception, "Callback must be callable", 0);
        RETURN_FALSE;
    }

    // Register callback
    register_pubsub_callback((uintptr_t)connection, callback, ZEND_THIS);

    // Convert patterns array to arguments
    HashTable *patterns_ht = Z_ARRVAL_P(patterns);
    uint32_t pattern_count = zend_hash_num_elements(patterns_ht);
    
    // For blocking psubscribe, we need patterns + timeout_ms
    uint32_t total_args = pattern_count + 1;
    uintptr_t *args = emalloc(total_args * sizeof(uintptr_t));
    unsigned long *args_len = emalloc(total_args * sizeof(unsigned long));
    
    // Add patterns
    uint32_t i = 0;
    zval *pattern_zv;
    ZEND_HASH_FOREACH_VAL(patterns_ht, pattern_zv) {
        convert_to_string(pattern_zv);
        args[i] = (uintptr_t)Z_STRVAL_P(pattern_zv);
        args_len[i] = Z_STRLEN_P(pattern_zv);
        i++;
    } ZEND_HASH_FOREACH_END();
    
    // Add timeout_ms as string
    char timeout_str[32];
    snprintf(timeout_str, sizeof(timeout_str), "%lld", (long long)timeout_ms);
    args[pattern_count] = (uintptr_t)timeout_str;
    args_len[pattern_count] = strlen(timeout_str);

    // Call FFI command
    struct CommandResult* result = command(
        connection, 0, REQUEST_TYPE_PSUBSCRIBE,
        total_args, args, args_len, NULL, 0, 0
    );

    efree(args);
    efree(args_len);

    if (result) {
        if (result->response && !result->command_error) {
            RETVAL_TRUE;
        } else {
            zend_throw_exception(zend_ce_exception, "PSubscribe failed", 0);
            RETVAL_FALSE;
        }
        free_command_result(result);
    } else {
        zend_throw_exception(zend_ce_exception, "PSubscribe command failed", 0);
        RETVAL_FALSE;
    }
}

// Unsubscribe implementation
void valkey_glide_unsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *channels = NULL;
    
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(channels)
    ZEND_PARSE_PARAMETERS_END();

    // Set is_active to false to break the subscribe loop
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)connection);
    zval *callback_zv = find_pubsub_callback(client_key);
    if (callback_zv) {
        pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(callback_zv);
        if (info) {
            info->is_active = false;
        }
    }

    RETVAL_TRUE;
}

// PUnsubscribe implementation
void valkey_glide_punsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zval *patterns = NULL;
    
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(patterns)
    ZEND_PARSE_PARAMETERS_END();

    uint32_t pattern_count = 0;
    uintptr_t *args = NULL;
    unsigned long *args_len = NULL;
    
    if (patterns) {
        HashTable *patterns_ht = Z_ARRVAL_P(patterns);
        pattern_count = zend_hash_num_elements(patterns_ht);
        
        if (pattern_count > 0) {
            args = emalloc(pattern_count * sizeof(uintptr_t));
            args_len = emalloc(pattern_count * sizeof(unsigned long));
            
            uint32_t i = 0;
            zval *pattern_zv;
            ZEND_HASH_FOREACH_VAL(patterns_ht, pattern_zv) {
                convert_to_string(pattern_zv);
                args[i] = (uintptr_t)Z_STRVAL_P(pattern_zv);
                args_len[i] = Z_STRLEN_P(pattern_zv);
                i++;
            } ZEND_HASH_FOREACH_END();
        }
    }

    // Call FFI command
    struct CommandResult* result = command(
        connection, 0, REQUEST_TYPE_PUNSUBSCRIBE,
        pattern_count, args, args_len, NULL, 0, 0
    );

    if (args) efree(args);
    if (args_len) efree(args_len);

    if (result) {
        if (result->response && !result->command_error) {
            RETVAL_TRUE;
        } else {
            zend_throw_exception(zend_ce_exception, "PUnsubscribe failed", 0);
            RETVAL_FALSE;
        }
        free_command_result(result);
    } else {
        zend_throw_exception(zend_ce_exception, "PUnsubscribe command failed", 0);
        RETVAL_FALSE;
    }
}

// Publish implementation
void valkey_glide_publish_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zend_string *channel, *message;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(channel)
        Z_PARAM_STR(message)
    ZEND_PARSE_PARAMETERS_END();

    uintptr_t args[2];
    unsigned long args_len[2];
    
    args[0] = (uintptr_t)ZSTR_VAL(channel);
    args_len[0] = ZSTR_LEN(channel);
    args[1] = (uintptr_t)ZSTR_VAL(message);
    args_len[1] = ZSTR_LEN(message);

    // Call FFI command
    struct CommandResult* result = command(
        connection, 0, REQUEST_TYPE_PUBLISH,
        2, args, args_len, NULL, 0, 0
    );

    if (result) {
        if (result->response && !result->command_error) {
            RETVAL_LONG(1); // Return number of subscribers (simplified)
        } else {
            zend_throw_exception(zend_ce_exception, "Publish failed", 0);
            RETVAL_FALSE;
        }
        free_command_result(result);
    } else {
        zend_throw_exception(zend_ce_exception, "Publish command failed", 0);
        RETVAL_FALSE;
    }
}

// C callback handler for FFI - called from Rust
void valkey_glide_pubsub_callback(
    uintptr_t client_adapter_ptr,
    enum PushKind kind,
    const uint8_t *message,
    int64_t message_len,
    const uint8_t *channel,
    int64_t channel_len,
    const uint8_t *pattern,
    int64_t pattern_len
) {
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long)client_adapter_ptr);
    
    zval *callback_zv = find_pubsub_callback(client_key);
    if (callback_zv) {
        pubsub_callback_info *info = (pubsub_callback_info *)Z_PTR_P(callback_zv);
        if (info && info->is_active) {
            pubsub_callback_handler(client_adapter_ptr, (int)kind, message, message_len, 
                                  channel, channel_len, pattern, pattern_len);
        } else {
            remove_pubsub_callback(client_key);
        }
    }
}



// Shutdown function
void valkey_glide_pubsub_shutdown(void) {
    if (pubsub_callbacks_initialized) {
        zend_hash_destroy(&pubsub_callbacks);
        pubsub_callbacks_initialized = false;
    }
}
