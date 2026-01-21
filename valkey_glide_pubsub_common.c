/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_pubsub_common.h"

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

// Global pubsub callback storage
static HashTable pubsub_callbacks;
static bool      pubsub_callbacks_initialized = false;

// Initialize pubsub callbacks
void init_pubsub_callbacks(void) {
    if (!pubsub_callbacks_initialized) {
        zend_hash_init(&pubsub_callbacks, 16, NULL, cleanup_callback_info, 0);
        pubsub_callbacks_initialized = true;
    }
}

// Find pubsub callback by client key
zval* find_pubsub_callback(const char* client_key) {
    if (!pubsub_callbacks_initialized) {
        return NULL;
    }
    return zend_hash_str_find(&pubsub_callbacks, client_key, strlen(client_key));
}

// Remove pubsub callback by client key
void remove_pubsub_callback(const char* client_key) {
    if (pubsub_callbacks_initialized) {
        zend_hash_str_del(&pubsub_callbacks, client_key, strlen(client_key));
    }
}

// Cleanup callback info
void cleanup_callback_info(zval* zv) {
    pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(zv);
    if (info) {
        // Free all messages in queue
        pubsub_message* msg = info->queue_head;
        while (msg) {
            pubsub_message* next = msg->next;
            if (msg->channel)
                efree(msg->channel);
            if (msg->message)
                efree(msg->message);
            if (msg->pattern)
                efree(msg->pattern);
            efree(msg);
            msg = next;
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

// Cleanup callback info for pointer-based storage
void cleanup_callback_info_ptr(void* ptr) {
    pubsub_callback_info* info = (pubsub_callback_info*) ptr;
    if (info) {
        zval_dtor(&info->callback);
        zval_dtor(&info->client_obj);
        efree(info);
    }
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

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = zend_hash_str_find(&pubsub_callbacks, client_key, key_len);
    if (!callback_zv) {
        return;
    }

    pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
    if (!info || !info->is_active) {
        return;
    }

    // Only handle message types
    if (kind != PUBSUB_KIND_MESSAGE && kind != PUBSUB_KIND_PMESSAGE &&
        kind != PUBSUB_KIND_SMESSAGE) {
        return;
    }

    // Allocate and populate message node
    pubsub_message* msg = (pubsub_message*) emalloc(sizeof(pubsub_message));
    if (!msg)
        return;

    msg->kind = kind;
    msg->next = NULL;

    // Copy channel
    msg->channel = (uint8_t*) emalloc(channel_len);
    if (msg->channel) {
        memcpy(msg->channel, channel, channel_len);
        msg->channel_len = channel_len;
    } else {
        efree(msg);
        return;
    }

    // Copy message
    msg->message = (uint8_t*) emalloc(message_len);
    if (msg->message) {
        memcpy(msg->message, message, message_len);
        msg->message_len = message_len;
    } else {
        efree(msg->channel);
        efree(msg);
        return;
    }

    // Copy pattern if present
    if (pattern && pattern_len > 0) {
        msg->pattern = (uint8_t*) emalloc(pattern_len);
        if (msg->pattern) {
            memcpy(msg->pattern, pattern, pattern_len);
            msg->pattern_len = pattern_len;
        } else {
            efree(msg->message);
            efree(msg->channel);
            efree(msg);
            return;
        }
    } else {
        msg->pattern     = NULL;
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
    cond_signal(&info->queue_cond);
    mutex_unlock(&info->queue_mutex);
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
    info->queue_head = NULL;
    info->queue_tail = NULL;
    mutex_init(&info->queue_mutex);
    cond_init(&info->queue_cond);

    // Initialize subscribed channels HashTable
    info->subscribed_channels = emalloc(sizeof(HashTable));
    zend_hash_init(info->subscribed_channels, 8, NULL, ZVAL_PTR_DTOR, 0);

    info->in_subscribe_mode = false;

    // Store the pointer in a zval using ZVAL_PTR
    zval callback_zv;
    ZVAL_PTR(&callback_zv, info);
    zend_hash_str_update(&pubsub_callbacks, client_key, key_len, &callback_zv);
}

// Unregister callback
void php_unregister_pubsub_callback(uintptr_t client_ptr) {
    if (!pubsub_callbacks_initialized)
        return;

    char client_key[32];
    int  key_len = snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = zend_hash_str_find(&pubsub_callbacks, client_key, key_len);
    if (callback_zv) {
        pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
        if (info) {
            info->is_active = false;
            cond_signal(&info->queue_cond);
        }
        // Delete from hashtable - this will call cleanup_callback_info
        zend_hash_str_del(&pubsub_callbacks, client_key, key_len);
    }
}

// Check if client is in subscribe mode
bool is_client_in_subscribe_mode(uintptr_t client_ptr) {
    if (!pubsub_callbacks_initialized)
        return false;

    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_ptr);

    zval* callback_zv = find_pubsub_callback(client_key);
    if (!callback_zv)
        return false;

    pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
    return info ? info->in_subscribe_mode : false;
}

// Common subscribe blocking loop
static void subscribe_blocking_loop(uintptr_t connection, enum RequestType unsub_type) {
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) connection);
    zval* callback_zv = find_pubsub_callback(client_key);
    if (!callback_zv)
        return;

    pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
    info->in_subscribe_mode    = true;

    while (info->is_active && zend_hash_num_elements(info->subscribed_channels) > 0) {
        pubsub_message* msg = NULL;

        mutex_lock(&info->queue_mutex);
        while (!info->queue_head && info->is_active &&
               zend_hash_num_elements(info->subscribed_channels) > 0) {
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

            if (msg->channel)
                efree(msg->channel);
            if (msg->message)
                efree(msg->message);
            if (msg->pattern)
                efree(msg->pattern);
            efree(msg);
        }
    }

    struct CommandResult* unsub_result =
        command((const void*) connection, 0, unsub_type, 0, NULL, NULL, NULL, 0, 0);
    if (unsub_result) {
        free_command_result(unsub_result);
    }

    info->in_subscribe_mode = false;
    php_unregister_pubsub_callback(connection);
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
    zval* callback_zv = find_pubsub_callback(client_key);
    if (!callback_zv) {
        VALKEY_LOG_ERROR(command_name, "Failed to find pubsub callback after command execution");
        ZVAL_FALSE(return_value);
        return 0;
    }

    pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);

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
        zval* callback_zv = find_pubsub_callback(client_key);
        if (callback_zv) {
            pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
            if (info) {
                // Remove channels from subscribed set
                ZEND_HASH_FOREACH_VAL(items_ht, item_zv) {
                    convert_to_string(item_zv);
                    zend_hash_str_del(
                        info->subscribed_channels, Z_STRVAL_P(item_zv), Z_STRLEN_P(item_zv));
                }
                ZEND_HASH_FOREACH_END();

                if (zend_hash_num_elements(info->subscribed_channels) == 0) {
                    info->is_active = false;
                    cond_signal(&info->queue_cond);
                }
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
        zval* callback_zv = find_pubsub_callback(client_key);
        if (callback_zv) {
            pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
            if (info) {
                zend_hash_clean(info->subscribed_channels);
                info->is_active = false;
                cond_signal(&info->queue_cond);
            }
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
    char client_key[32];
    snprintf(client_key, sizeof(client_key), "%lu", (unsigned long) client_adapter_ptr);

    zval* callback_zv = find_pubsub_callback(client_key);
    if (callback_zv) {
        pubsub_callback_info* info = (pubsub_callback_info*) Z_PTR_P(callback_zv);
        if (info && info->is_active) {
            pubsub_callback_handler(client_adapter_ptr,
                                    (int) kind,
                                    message,
                                    message_len,
                                    channel,
                                    channel_len,
                                    pattern,
                                    pattern_len);
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
