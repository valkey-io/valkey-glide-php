/*
  +----------------------------------------------------------------------+
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

#include <stdatomic.h>

#include "common.h"

// Cross-platform thread-local storage
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#include <threads.h>
#define THREAD_LOCAL thread_local
#elif defined(_WIN32)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define THREAD_LOCAL __thread
#else
#error "Thread-local storage not supported on this compiler"
#endif

// Safe callback wrapper with reference counting
typedef struct {
    zval*        callback;
    _Atomic int  refcount;
    _Atomic bool valid;
} safe_callback_t;

// Separate thread-local hash tables for standalone and cluster clients
static THREAD_LOCAL HashTable* g_standalone_client_map = NULL;
static THREAD_LOCAL HashTable* g_cluster_client_map    = NULL;

// Helper to safely replace callback (new_callback can be NULL to just clear)
static void replace_callback(valkey_glide_object* obj, zval* new_callback) {
    if (!obj)
        return;

    safe_callback_t* old_cb = (safe_callback_t*) atomic_load(&obj->pubsub_callback);
    safe_callback_t* new_cb = NULL;

    // Create new callback wrapper if provided
    if (new_callback) {
        new_cb           = emalloc(sizeof(safe_callback_t));
        new_cb->callback = emalloc(sizeof(zval));
        ZVAL_COPY(new_cb->callback, new_callback);
        atomic_store(&new_cb->refcount, 1);
        atomic_store(&new_cb->valid, true);
    }

    // Invalidate old callback if it exists
    if (old_cb) {
        atomic_store(&old_cb->valid, false);
    }

    // Atomically replace callback pointer
    atomic_store(&obj->pubsub_callback, (void*) new_cb);

    // Release old callback after replacement
    if (old_cb) {
        release_callback(old_cb);
    }
}

// Helper to safely invalidate and clear callback
static void invalidate_callback(valkey_glide_object* obj) {
    replace_callback(obj, NULL);
}

// Helper to acquire callback safely
static safe_callback_t* acquire_callback(valkey_glide_object* obj) {
    if (!obj)
        return NULL;

    safe_callback_t* safe_cb = (safe_callback_t*) atomic_load(&obj->pubsub_callback);
    if (!safe_cb || !atomic_load(&safe_cb->valid))
        return NULL;

    atomic_fetch_add(&safe_cb->refcount, 1);

    // Double-check after acquiring reference
    if (!atomic_load(&safe_cb->valid)) {
        atomic_fetch_sub(&safe_cb->refcount, 1);
        return NULL;
    }

    return safe_cb;
}

// Helper to release callback safely
static void release_callback(safe_callback_t* safe_cb) {
    if (!safe_cb)
        return;

    if (atomic_fetch_sub(&safe_cb->refcount, 1) == 1) {
        // Last reference - safe to free
        if (safe_cb->callback) {
            zval_ptr_dtor(safe_cb->callback);
            efree(safe_cb->callback);
        }
        efree(safe_cb);
    }
}

// Helper function to execute pubsub callback with given hash table
static void execute_pubsub_callback(HashTable*     client_map,
                                    uintptr_t      client_ptr,
                                    int            kind,
                                    const uint8_t* message,
                                    int64_t        message_len,
                                    const uint8_t* channel,
                                    int64_t        channel_len,
                                    const uint8_t* pattern,
                                    int64_t        pattern_len) {
    if (!client_map)
        return;

    zend_ulong           key = (zend_ulong) client_ptr;
    valkey_glide_object* obj = zend_hash_index_find_ptr(client_map, key);
    if (!obj)
        return;

    // Safely acquire callback reference
    safe_callback_t* safe_cb = acquire_callback(obj);
    if (!safe_cb)
        return;

    // Convert C data to PHP strings
    zval args[4];
    ZVAL_OBJ(&args[0], &obj->std);
    ZVAL_STRINGL(&args[1], (char*) channel, channel_len);
    ZVAL_STRINGL(&args[2], (char*) message, message_len);
    if (pattern && pattern_len > 0) {
        ZVAL_STRINGL(&args[3], (char*) pattern, pattern_len);
    } else {
        ZVAL_NULL(&args[3]);
    }

    // Call PHP callback safely
    zval retval;
    if (call_user_function(NULL, NULL, safe_cb->callback, &retval, pattern ? 4 : 3, args) ==
        SUCCESS) {
        zval_ptr_dtor(&retval);
    }

    // Cleanup
    zval_ptr_dtor(&args[1]);
    zval_ptr_dtor(&args[2]);
    if (pattern && pattern_len > 0) {
        zval_ptr_dtor(&args[3]);
    }

    // Release callback reference
    release_callback(safe_cb);
}

// Standalone client callback function called by Rust FFI
void standalone_pubsub_callback(uintptr_t      client_ptr,
                                enum PushKind  kind,
                                const uint8_t* message,
                                int64_t        message_len,
                                const uint8_t* channel,
                                int64_t        channel_len,
                                const uint8_t* pattern,
                                int64_t        pattern_len) {
    execute_pubsub_callback(g_standalone_client_map,
                            client_ptr,
                            kind,
                            message,
                            message_len,
                            channel,
                            channel_len,
                            pattern,
                            pattern_len);
}

// Cluster client callback function called by Rust FFI
void cluster_pubsub_callback(uintptr_t      client_ptr,
                             enum PushKind  kind,
                             const uint8_t* message,
                             int64_t        message_len,
                             const uint8_t* channel,
                             int64_t        channel_len,
                             const uint8_t* pattern,
                             int64_t        pattern_len) {
    execute_pubsub_callback(g_cluster_client_map,
                            client_ptr,
                            kind,
                            message,
                            message_len,
                            channel,
                            channel_len,
                            pattern,
                            pattern_len);
}

// Register standalone client mapping
void register_standalone_client_mapping(const void* rust_client_ptr, valkey_glide_object* obj) {
    if (!g_standalone_client_map) {
        ALLOC_HASHTABLE(g_standalone_client_map);
        zend_hash_init(g_standalone_client_map, 0, NULL, NULL, 0);
    }

    zend_ulong key = (zend_ulong) rust_client_ptr;
    zend_hash_index_update_ptr(g_standalone_client_map, key, obj);
}

// Register cluster client mapping
void register_cluster_client_mapping(const void* rust_client_ptr, valkey_glide_object* obj) {
    if (!g_cluster_client_map) {
        ALLOC_HASHTABLE(g_cluster_client_map);
        zend_hash_init(g_cluster_client_map, 0, NULL, NULL, 0);
    }

    zend_ulong key = (zend_ulong) rust_client_ptr;
    zend_hash_index_update_ptr(g_cluster_client_map, key, obj);
}

// Unregister standalone client mapping
void unregister_standalone_client_mapping(const void* rust_client_ptr) {
    if (g_standalone_client_map) {
        zend_ulong key = (zend_ulong) rust_client_ptr;
        zend_hash_index_del(g_standalone_client_map, key);
    }
}

// Unregister cluster client mapping
void unregister_cluster_client_mapping(const void* rust_client_ptr) {
    if (g_cluster_client_map) {
        zend_ulong key = (zend_ulong) rust_client_ptr;
        zend_hash_index_del(g_cluster_client_map, key);
    }
}

// Backward-compatible unregister (tries both tables)
void unregister_client_mapping(const void* rust_client_ptr) {
    unregister_standalone_client_mapping(rust_client_ptr);
    unregister_cluster_client_mapping(rust_client_ptr);
}

// Optimized unregister using client type
void unregister_client_mapping_typed(const void* rust_client_ptr, bool is_cluster) {
    if (is_cluster) {
        unregister_cluster_client_mapping(rust_client_ptr);
    } else {
        unregister_standalone_client_mapping(rust_client_ptr);
    }
}

// Legacy function for backward compatibility
void register_client_mapping(const void* rust_client_ptr, valkey_glide_object* obj) {
    // Default to standalone for backward compatibility
    register_standalone_client_mapping(rust_client_ptr, obj);
}
// Helper function for pubsub command execution
static bool execute_pubsub_command(valkey_glide_object* valkey_glide,
                                   CommandType          command_type,
                                   zval*                channels_or_patterns,
                                   const char*          command_name) {
    if (!channels_or_patterns) {
        // Handle unsubscribe all case
        CommandResult* cmd_result =
            execute_command(valkey_glide->glide_client, command_type, 0, NULL, NULL);

        if (!cmd_result) {
            php_printf("Error: Failed to execute %s command\n", command_name);
            return false;
        }

        if (cmd_result->command_error) {
            php_printf("Error: %s\n", cmd_result->command_error->command_error_message);
            free_command_result(cmd_result);
            return false;
        }

        free_command_result(cmd_result);
        return true;
    }

    // Handle channels/patterns array
    HashTable* ht        = Z_ARRVAL_P(channels_or_patterns);
    uint32_t   num_items = zend_hash_num_elements(ht);

    if (num_items == 0) {
        return execute_pubsub_command(valkey_glide, command_type, NULL, command_name);
    }

    uintptr_t*     args     = emalloc(num_items * sizeof(uintptr_t));
    unsigned long* args_len = emalloc(num_items * sizeof(unsigned long));

    uint32_t i = 0;
    zval*    entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        convert_to_string(entry);
        args[i]     = (uintptr_t) Z_STRVAL_P(entry);
        args_len[i] = Z_STRLEN_P(entry);
        i++;
    }
    ZEND_HASH_FOREACH_END();

    CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                command_type,
                                                num_items,
                                                (const uintptr_t*) args,
                                                (const unsigned long*) args_len);

    efree(args);
    efree(args_len);

    if (!cmd_result) {
        php_printf("Error: Failed to execute %s command\n", command_name);
        return false;
    }

    if (cmd_result->command_error) {
        php_printf("Error: %s\n", cmd_result->command_error->command_error_message);
        free_command_result(cmd_result);
        return false;
    }

    free_command_result(cmd_result);
    return true;
}

// Execute command implementations
int execute_subscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *channels, *callback;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_ARRAY(channels)
    Z_PARAM_ZVAL(callback)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(zend_ce_exception, "Callback must be callable", 0);
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* SUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: SUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    replace_callback(valkey_glide, callback);

    if (execute_pubsub_command(valkey_glide, Subscribe, channels, "subscribe")) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    return 0;
}

int execute_psubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *patterns, *callback;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_ARRAY(patterns)
    Z_PARAM_ZVAL(callback)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception(zend_ce_exception, "Callback must be callable", 0);
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* PSUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: PSUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    replace_callback(valkey_glide, callback);

    if (execute_pubsub_command(valkey_glide, PSubscribe, patterns, "psubscribe")) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    return 0;
}

int execute_unsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval* channels = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(channels)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* UNSUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: UNSUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    if (!execute_pubsub_command(valkey_glide, Unsubscribe, channels, "unsubscribe")) {
        return 0;
    }

    if (!channels || zend_hash_num_elements(Z_ARRVAL_P(channels)) == 0) {
        invalidate_callback(valkey_glide);
    }

    array_init(return_value);
    return 1;
}

int execute_punsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval* patterns = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(patterns)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* PUNSUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: PUNSUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    if (!execute_pubsub_command(valkey_glide, PUnsubscribe, patterns, "punsubscribe")) {
        return 0;
    }

    if (!patterns || zend_hash_num_elements(Z_ARRVAL_P(patterns)) == 0) {
        invalidate_callback(valkey_glide);
    }

    array_init(return_value);
    return 1;
}

int execute_publish_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zend_string *channel, *message;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STR(channel)
    Z_PARAM_STR(message)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        ZVAL_LONG(return_value, 0);
        return 1;
    }

    const uintptr_t     args[]     = {(uintptr_t) ZSTR_VAL(channel), (uintptr_t) ZSTR_VAL(message)};
    const unsigned long args_len[] = {ZSTR_LEN(channel), ZSTR_LEN(message)};

    CommandResult* cmd_result =
        execute_command(valkey_glide->glide_client, Publish, 2, args, args_len);

    if (!cmd_result) {
        php_printf("Error: Failed to execute publish command\n");
        ZVAL_LONG(return_value, 0);
        return 1;
    }

    if (cmd_result->command_error) {
        php_printf("Error: %s\n", cmd_result->command_error->command_error_message);
        free_command_result(cmd_result);
        ZVAL_LONG(return_value, 0);
        return 1;
    }

    long subscriber_count = 0;
    if (cmd_result->response && cmd_result->response->response_type == Int) {
        subscriber_count = cmd_result->response->int_value;
    }

    free_command_result(cmd_result);
    ZVAL_LONG(return_value, subscriber_count);
    return 1;
}

// PubSub method implementation macros
#define SUBSCRIBE_METHOD_IMPL(class_name)                        \
    PHP_METHOD(class_name, subscribe) {                          \
        shared_subscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
    }

#define PSUBSCRIBE_METHOD_IMPL(class_name)                        \
    PHP_METHOD(class_name, psubscribe) {                          \
        shared_psubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
    }

#define UNSUBSCRIBE_METHOD_IMPL(class_name)                        \
    PHP_METHOD(class_name, unsubscribe) {                          \
        shared_unsubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
    }

#define PUNSUBSCRIBE_METHOD_IMPL(class_name)                        \
    PHP_METHOD(class_name, punsubscribe) {                          \
        shared_punsubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
    }

#define PUBLISH_METHOD_IMPL(class_name)                                                              \
    PHP_METHOD(class_name, publish) {                                                                \
        zend_string *channel, *message;                                                              \
                                                                                                     \
        ZEND_PARSE_PARAMETERS_START(2, 2)                                                            \
        Z_PARAM_STR(channel)                                                                         \
        Z_PARAM_STR(message)                                                                         \
        ZEND_PARSE_PARAMETERS_END();                                                                 \
                                                                                                     \
        valkey_glide_object* valkey_glide =                                                          \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());                        \
        if (!valkey_glide || !valkey_glide->glide_client) {                                          \
            php_printf("Error: ValkeyGlide client not initialized\n");                               \
            RETURN_LONG(0);                                                                          \
        }                                                                                            \
                                                                                                     \
        const uintptr_t     args[] = {(uintptr_t) ZSTR_VAL(channel), (uintptr_t) ZSTR_VAL(message)}; \
        const unsigned long args_len[] = {ZSTR_LEN(channel), ZSTR_LEN(message)};                     \
                                                                                                     \
        CommandResult* cmd_result =                                                                  \
            execute_command(valkey_glide->glide_client, Publish, 2, args, args_len);                 \
                                                                                                     \
        if (!cmd_result) {                                                                           \
            php_printf("Error: Failed to execute publish command\n");                                \
            RETURN_LONG(0);                                                                          \
        }                                                                                            \
                                                                                                     \
        if (cmd_result->command_error) {                                                             \
            php_printf("Error: %s\n", cmd_result->command_error->command_error_message);             \
            free_command_result(cmd_result);                                                         \
            RETURN_LONG(0);                                                                          \
        }                                                                                            \
                                                                                                     \
        long subscriber_count = 0;                                                                   \
        if (cmd_result->response && cmd_result->response->response_type == Int) {                    \
            subscriber_count = cmd_result->response->int_value;                                      \
        }                                                                                            \
                                                                                                     \
        free_command_result(cmd_result);                                                             \
        RETURN_LONG(subscriber_count);                                                               \
    }
