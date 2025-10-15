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

#include "valkey_glide_pubsub.h"

#include <stdatomic.h>

#include "command_response.h"
#include "common.h"

// Forward declarations
static void release_callback(safe_callback_t* safe_cb);

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
void invalidate_callback(valkey_glide_object* obj) {
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
    if (pattern && pattern_len > 0) {
        ZVAL_STRINGL(&args[1], (char*) pattern, pattern_len);
        ZVAL_STRINGL(&args[2], (char*) channel, channel_len);
        ZVAL_STRINGL(&args[3], (char*) message, message_len);
    } else {
        ZVAL_STRINGL(&args[1], (char*) channel, channel_len);
        ZVAL_STRINGL(&args[2], (char*) message, message_len);
        ZVAL_NULL(&args[3]);
    }

    // Call PHP callback safely
    zval retval;
    if (call_user_function(NULL, NULL, safe_cb->callback, &retval, pattern ? 4 : 3, args) ==
        SUCCESS) {
        zval_ptr_dtor(&retval);
    }

    // Cleanup
    if (pattern && pattern_len > 0) {
        zval_ptr_dtor(&args[1]);  // pattern
        zval_ptr_dtor(&args[2]);  // channel
        zval_ptr_dtor(&args[3]);  // message
    } else {
        zval_ptr_dtor(&args[1]);  // channel
        zval_ptr_dtor(&args[2]);  // message
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
                                   enum RequestType     command_type,
                                   zval*                channels_or_patterns,
                                   const char*          command_name) {
    if (!channels_or_patterns) {
        // Handle unsubscribe all case
        CommandResult* cmd_result =
            command(valkey_glide->glide_client, 0, command_type, 0, NULL, NULL, NULL, 0, 0);

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

    CommandResult* cmd_result = command(valkey_glide->glide_client,
                                        0,
                                        command_type,
                                        num_items,
                                        (const uintptr_t*) args,
                                        (const unsigned long*) args_len,
                                        NULL,
                                        0,
                                        0);

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
    zval *channels = NULL, *callback = NULL;

    if (zend_parse_method_parameters(argc, object, "Oaz", &object, ce, &channels, &callback) ==
        FAILURE) {
        return 0;
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        php_printf("Error: Callback must be callable\n");
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
        ZVAL_NULL(return_value);  // PHPRedis returns null on successful subscription
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 1;
}

int execute_psubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *patterns = NULL, *callback = NULL;

    if (zend_parse_method_parameters(argc, object, "Oaz", &object, ce, &patterns, &callback) ==
        FAILURE) {
        return 0;
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        php_printf("Error: Callback must be callable\n");
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
        ZVAL_NULL(return_value);  // PHPRedis returns null on successful subscription
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 1;
}

int execute_unsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval* channels = NULL;

    if (zend_parse_method_parameters(argc, object, "O|z!", &object, ce, &channels) == FAILURE) {
        return 0;
    }

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

    if (!channels ||
        (Z_TYPE_P(channels) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(channels)) == 0)) {
        invalidate_callback(valkey_glide);
    }

    ZVAL_TRUE(return_value);  // PHPRedis returns true for successful unsubscribe
    return 1;
}

int execute_punsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval* patterns = NULL;

    if (zend_parse_method_parameters(argc, object, "O|z!", &object, ce, &patterns) == FAILURE) {
        return 0;
    }

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

    if (!patterns ||
        (Z_TYPE_P(patterns) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(patterns)) == 0)) {
        invalidate_callback(valkey_glide);
    }

    ZVAL_TRUE(return_value);  // PHPRedis returns true for successful unsubscribe
    return 1;
}

int execute_publish_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  channel = NULL;
    char*  message = NULL;
    size_t channel_len, message_len;

    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &channel, &channel_len, &message, &message_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        ZVAL_LONG(return_value, 0);
        return 1;
    }

    const uintptr_t     args[]     = {(uintptr_t) channel, (uintptr_t) message};
    const unsigned long args_len[] = {channel_len, message_len};

    CommandResult* cmd_result =
        command(valkey_glide->glide_client, 0, Publish, 2, args, args_len, NULL, 0, 0);

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

int execute_pubsub_introspection_command(zval*             object,
                                         int               argc,
                                         zval*             return_value,
                                         zend_class_entry* ce) {
    char*  subcommand = NULL;
    size_t subcommand_len;
    zval*  argument = NULL;

    if (zend_parse_method_parameters(
            argc, object, "Os|z", &object, ce, &subcommand, &subcommand_len, &argument) ==
        FAILURE) {
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 1;
    }

    // Determine RequestType and validate arguments based on subcommand
    enum RequestType command_type;
    bool             requires_argument = false;
    bool             allows_multiple   = false;

    if (strcasecmp(subcommand, "CHANNELS") == 0) {
        command_type = PubSubChannels;
        // Optional pattern argument
    } else if (strcasecmp(subcommand, "NUMSUB") == 0) {
        command_type    = PubSubNumSub;
        allows_multiple = true;
        // Optional channels argument(s)
    } else if (strcasecmp(subcommand, "NUMPAT") == 0) {
        command_type = PubSubNumPat;
        // No arguments allowed
        if (argument && Z_TYPE_P(argument) != IS_NULL) {
            ZVAL_FALSE(return_value);
            return 1;
        }
    } else if (strcasecmp(subcommand, "SHARDCHANNELS") == 0) {
        command_type = PubSubShardChannels;
        // Optional pattern argument
    } else if (strcasecmp(subcommand, "SHARDNUMSUB") == 0) {
        command_type    = PubSubShardNumSub;
        allows_multiple = true;
        // Optional channels argument(s)
    } else {
        ZVAL_FALSE(return_value);
        return 1;
    }

    // Build command arguments - optimize for 0-1 args case
    const char*  single_arg;
    size_t       single_arg_len;
    const char** args        = NULL;
    size_t*      args_len    = NULL;
    int          num_args    = 0;
    bool         use_dynamic = false;

    if (argument && Z_TYPE_P(argument) != IS_NULL) {
        if (Z_TYPE_P(argument) == IS_STRING) {
            // Single argument - no allocation needed
            single_arg     = Z_STRVAL_P(argument);
            single_arg_len = Z_STRLEN_P(argument);
            num_args       = 1;
        } else if (Z_TYPE_P(argument) == IS_ARRAY && allows_multiple) {
            int array_count = zend_hash_num_elements(Z_ARRVAL_P(argument));
            if (array_count == 0) {
                // Empty array - no arguments
                num_args = 0;
            } else if (array_count == 1) {
                // Single element - no allocation needed
                zval* first_entry = zend_hash_index_find(Z_ARRVAL_P(argument), 0);
                if (first_entry && Z_TYPE_P(first_entry) == IS_STRING) {
                    single_arg     = Z_STRVAL_P(first_entry);
                    single_arg_len = Z_STRLEN_P(first_entry);
                    num_args       = 1;
                }
            } else {
                // Multiple elements - use dynamic allocation
                use_dynamic = true;
                args        = emalloc(array_count * sizeof(char*));
                args_len    = emalloc(array_count * sizeof(size_t));

                zval* entry;
                ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(argument), entry) {
                    if (Z_TYPE_P(entry) == IS_STRING) {
                        args[num_args]     = Z_STRVAL_P(entry);
                        args_len[num_args] = Z_STRLEN_P(entry);
                        num_args++;
                    }
                }
                ZEND_HASH_FOREACH_END();
            }
        } else if (!allows_multiple) {
            // Invalid argument type for commands that don't allow arrays
            ZVAL_FALSE(return_value);
            return 1;
        }
    }

    // Prepare arguments for command call
    const uintptr_t*     cmd_args;
    const unsigned long* cmd_args_len;

    if (num_args == 0) {
        cmd_args     = NULL;
        cmd_args_len = NULL;
    } else if (num_args == 1 && !use_dynamic) {
        cmd_args     = (const uintptr_t*) &single_arg;
        cmd_args_len = (const unsigned long*) &single_arg_len;
    } else {
        cmd_args     = (const uintptr_t*) args;
        cmd_args_len = (const unsigned long*) args_len;
    }

    CommandResult* cmd_result = command(
        valkey_glide->glide_client, 0, command_type, num_args, cmd_args, cmd_args_len, NULL, 0, 0);

    // Clean up dynamic allocation only if used
    if (use_dynamic) {
        if (args) {
            efree(args);
        }
        if (args_len) {
            efree(args_len);
        }
    }

    if (!cmd_result) {
        ZVAL_FALSE(return_value);
        return 1;
    }

    // Parse result based on subcommand
    if (command_type == PubSubNumPat) {
        // Return integer count for NUMPAT
        if (cmd_result->command_error) {
            free_command_result(cmd_result);
            ZVAL_LONG(return_value, 0);
        } else if (cmd_result->response && cmd_result->response->response_type == Int) {
            ZVAL_LONG(return_value, cmd_result->response->int_value);
            free_command_result(cmd_result);
        } else {
            free_command_result(cmd_result);
            ZVAL_LONG(return_value, 0);
        }
    } else {
        // Return array for CHANNELS, NUMSUB, SHARDCHANNELS, SHARDNUMSUB
        if (cmd_result->command_error) {
            array_init(return_value);
            free_command_result(cmd_result);
        } else if (cmd_result->response) {
            // Use command_response_to_zval to parse array response
            int result = command_response_to_zval(cmd_result->response, return_value, 0, false);
            if (result <= 0) {
                // If parsing failed, return empty array
                array_init(return_value);
            }
            free_command_result(cmd_result);
        } else {
            array_init(return_value);
            free_command_result(cmd_result);
        }
    }

    return 1;
}

int execute_ssubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *channels = NULL, *callback = NULL;

    if (zend_parse_method_parameters(argc, object, "Oaz", &object, ce, &channels, &callback) ==
        FAILURE) {
        return 0;
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        php_printf("Error: Callback must be callable\n");
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* SSUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: SSUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    replace_callback(valkey_glide, callback);

    if (execute_pubsub_command(valkey_glide, SSubscribe, channels, "ssubscribe")) {
        ZVAL_NULL(return_value);  // Return null on successful subscription
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 1;
}

int execute_sunsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval* channels = NULL;

    if (zend_parse_method_parameters(argc, object, "O|z!", &object, ce, &channels) == FAILURE) {
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        return 0;
    }

    /* SUNSUBSCRIBE cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        php_printf("Error: SUNSUBSCRIBE command cannot be used in batch mode\n");
        return 0;
    }

    if (!execute_pubsub_command(valkey_glide, SUnsubscribe, channels, "sunsubscribe")) {
        return 0;
    }

    if (!channels ||
        (Z_TYPE_P(channels) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(channels)) == 0)) {
        invalidate_callback(valkey_glide);
    }

    ZVAL_TRUE(return_value);  // Return true for successful unsubscribe
    return 1;
}

int execute_spublish_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  channel = NULL;
    char*  message = NULL;
    size_t channel_len, message_len;

    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &channel, &channel_len, &message, &message_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        php_printf("Error: ValkeyGlide client not initialized\n");
        ZVAL_LONG(return_value, 0);
        return 1;
    }

    const uintptr_t     args[]     = {(uintptr_t) channel, (uintptr_t) message};
    const unsigned long args_len[] = {channel_len, message_len};

    CommandResult* cmd_result =
        command(valkey_glide->glide_client, 0, SPublish, 2, args, args_len, NULL, 0, 0);

    if (!cmd_result) {
        php_printf("Error: Failed to execute spublish command\n");
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
