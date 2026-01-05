#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_script_common.h"
#include "zend_exceptions.h"

// Helper function to process array arguments with lengths for FFI
static void process_array_to_uintptr_args(zval*           array,
                                          int*            count,
                                          uintptr_t**     args,
                                          unsigned long** args_len) {
    if (Z_TYPE_P(array) != IS_ARRAY) {
        *count    = 0;
        *args     = NULL;
        *args_len = NULL;
        return;
    }

    *count = zend_hash_num_elements(Z_ARRVAL_P(array));
    if (*count == 0) {
        *args     = NULL;
        *args_len = NULL;
        return;
    }

    *args     = emalloc(sizeof(uintptr_t) * (*count));
    *args_len = emalloc(sizeof(unsigned long) * (*count));
    zval* entry;
    int   i = 0;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
        convert_to_string(entry);
        (*args)[i]     = (uintptr_t) Z_STRVAL_P(entry);
        (*args_len)[i] = Z_STRLEN_P(entry);
        i++;
    }
    ZEND_HASH_FOREACH_END();
}

// Helper to convert string array to FFI format
static void prepare_ffi_args(zval*           array,
                             uintptr_t**     ptrs,
                             unsigned long** lens,
                             unsigned long*  count) {
    if (Z_TYPE_P(array) != IS_ARRAY) {
        *count = 0;
        *ptrs  = NULL;
        *lens  = NULL;
        return;
    }

    *count = zend_hash_num_elements(Z_ARRVAL_P(array));
    if (*count == 0) {
        *ptrs = NULL;
        *lens = NULL;
        return;
    }

    *ptrs = emalloc(sizeof(uintptr_t) * (*count));
    *lens = emalloc(sizeof(unsigned long) * (*count));

    zval* entry;
    int   i = 0;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
        convert_to_string(entry);
        (*ptrs)[i] = (uintptr_t) Z_STRVAL_P(entry);
        (*lens)[i] = Z_STRLEN_P(entry);
        i++;
    }
    ZEND_HASH_FOREACH_END();
}

// Script execution using invoke_script FFI (following Go pattern)
void execute_invoke_script_command(valkey_glide_object* valkey_glide,
                                   const char*          script_hash,
                                   zval*                keys,
                                   zval*                args,
                                   zval*                return_value,
                                   bool                 use_false_if_null) {
    // Prepare FFI arguments
    uintptr_t *    key_ptrs, *arg_ptrs;
    unsigned long *key_lens, *arg_lens;
    unsigned long  key_count, arg_count;

    prepare_ffi_args(keys, &key_ptrs, &key_lens, &key_count);
    prepare_ffi_args(args, &arg_ptrs, &arg_lens, &arg_count);

    // Call invoke_script (like Go's executeScriptWithRoute)
    struct CommandResult* result = invoke_script(valkey_glide->glide_client,
                                                 0,  // request_id
                                                 script_hash,
                                                 key_count,
                                                 key_ptrs,
                                                 key_lens,
                                                 arg_count,
                                                 arg_ptrs,
                                                 arg_lens,
                                                 NULL,  // route_bytes
                                                 0      // route_bytes_len
    );

    // Cleanup FFI arguments
    if (key_ptrs)
        efree(key_ptrs);
    if (key_lens)
        efree(key_lens);
    if (arg_ptrs)
        efree(arg_ptrs);
    if (arg_lens)
        efree(arg_lens);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    if (!result->response) {
        free_command_result(result);
        RETURN_FALSE;
    }

    command_response_to_zval(result->response, return_value, 0, use_false_if_null);
    free_command_result(result);
}

// Helper to store script and get hash (like Go's storeScript)
char* store_script_and_get_hash(const char* script) {
    struct ScriptHashBuffer* hash_buffer = store_script((const uint8_t*) script, strlen(script));
    if (!hash_buffer || !hash_buffer->ptr) {
        if (hash_buffer) {
            free_script_hash_buffer(hash_buffer);
        }
        return NULL;
    }

    char* hash = emalloc(hash_buffer->len + 1);
    memcpy(hash, hash_buffer->ptr, hash_buffer->len);
    hash[hash_buffer->len] = '\0';
    free_script_hash_buffer(hash_buffer);
    return hash;
}

// Helper function for script flush command
void execute_script_flush_command(zval* object, zval* return_value, bool is_cluster) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptFlush, 0, NULL, NULL);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    free_command_result(result);

    if (is_cluster) {
        // Cluster scriptFlush returns "OK" string for PHPRedis compatibility
        RETURN_STRING("OK");
    } else {
        // PHPRedis scriptFlush returns boolean true
        RETURN_TRUE;
    }
}

// Script management commands using RequestType (like Go's executeCommand)


// PHP method implementations following Go pattern
PHP_METHOD(ValkeyGlide, invokeScript) {
    char*  script_or_hash;
    size_t script_or_hash_len;
    zval * keys = NULL, *args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(script_or_hash, script_or_hash_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(keys)
    Z_PARAM_ARRAY(args)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    // Prepare empty arrays if not provided
    zval empty_keys, empty_args;
    if (!keys) {
        array_init(&empty_keys);
        keys = &empty_keys;
    }
    if (!args) {
        array_init(&empty_args);
        args = &empty_args;
    }

    // Check if input looks like a SHA1 hash (40 hex characters)
    if (script_or_hash_len == 40 && strspn(script_or_hash, "0123456789abcdefABCDEF") == 40) {
        // Use as hash directly (like evalsha)
        execute_invoke_script_command(
            valkey_glide, script_or_hash, keys, args, return_value, false);
    } else {
        // Store script and get hash (like eval)
        char* script_hash = store_script_and_get_hash(script_or_hash);
        if (!script_hash) {
            RETURN_FALSE;
        }
        execute_invoke_script_command(valkey_glide, script_hash, keys, args, return_value, false);
        efree(script_hash);
    }
}

// PHPRedis compatibility methods
PHP_METHOD(ValkeyGlide, eval) {
    // TODO: EVAL command is not supported by glide-core. Remove this comment when supported.
    zend_throw_exception(zend_ce_exception, "EVAL command is not supported by glide-core", 0);
    RETURN_FALSE;

    char*     script;
    size_t    script_len;
    zval*     args     = NULL;
    zend_long num_keys = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(script, script_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(args)
    Z_PARAM_LONG(num_keys)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    // Build command arguments like rawcommand
    int total_args = 2;  // EVAL + script
    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        total_args += zend_hash_num_elements(Z_ARRVAL_P(args));
    }
    total_args++;  // for num_keys

    char** command_args = emalloc(sizeof(char*) * total_args);
    command_args[0]     = "EVAL";
    command_args[1]     = script;

    // Convert num_keys to string
    char num_keys_str[32];
    snprintf(num_keys_str, sizeof(num_keys_str), "%lld", (long long) num_keys);
    command_args[2] = num_keys_str;

    int arg_index = 3;
    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        zval* entry;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            convert_to_string(entry);
            command_args[arg_index++] = Z_STRVAL_P(entry);
        }
        ZEND_HASH_FOREACH_END();
    }

    // Convert char** to uintptr_t* for execute_command
    uintptr_t*     cmd_args = emalloc(sizeof(uintptr_t) * total_args);
    unsigned long* args_len = emalloc(sizeof(unsigned long) * total_args);
    for (int i = 0; i < total_args; i++) {
        cmd_args[i] = (uintptr_t) command_args[i];
        args_len[i] = strlen(command_args[i]);
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, Eval, total_args, cmd_args, args_len);
    efree(command_args);
    efree(cmd_args);
    efree(args_len);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    if (!result->response) {
        free_command_result(result);
        RETURN_FALSE;
    }

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

PHP_METHOD(ValkeyGlide, evalsha) {
    // TODO: EVALSHA command is not supported by glide-core. Remove this comment when supported.
    zend_throw_exception(zend_ce_exception, "EVALSHA command is not supported by glide-core", 0);
    RETURN_FALSE;

    char*     sha1;
    size_t    sha1_len;
    zval*     args     = NULL;
    zend_long num_keys = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(sha1, sha1_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(args)
    Z_PARAM_LONG(num_keys)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    // Build command arguments like rawcommand
    int total_args = 2;  // EVALSHA + sha1
    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        total_args += zend_hash_num_elements(Z_ARRVAL_P(args));
    }
    total_args++;  // for num_keys

    char** command_args = emalloc(sizeof(char*) * total_args);
    command_args[0]     = "EVALSHA";
    command_args[1]     = sha1;

    // Convert num_keys to string
    char num_keys_str[32];
    snprintf(num_keys_str, sizeof(num_keys_str), "%lld", (long long) num_keys);
    command_args[2] = num_keys_str;

    int arg_index = 3;
    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        zval* entry;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            convert_to_string(entry);
            command_args[arg_index++] = Z_STRVAL_P(entry);
        }
        ZEND_HASH_FOREACH_END();
    }

    // Convert char** to uintptr_t* for execute_command
    uintptr_t*     cmd_args = emalloc(sizeof(uintptr_t) * total_args);
    unsigned long* args_len = emalloc(sizeof(unsigned long) * total_args);
    for (int i = 0; i < total_args; i++) {
        cmd_args[i] = (uintptr_t) command_args[i];
        args_len[i] = strlen(command_args[i]);
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, EvalSha, total_args, cmd_args, args_len);
    efree(command_args);
    efree(cmd_args);
    efree(args_len);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    if (!result->response) {
        free_command_result(result);
        RETURN_FALSE;
    }

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Cluster implementations
PHP_METHOD(ValkeyGlideCluster, eval) {
    // TODO: EVAL command is not supported by glide-core. Remove this comment when supported.
    zend_throw_exception(
        get_valkey_glide_cluster_exception_ce(), "EVAL command is not supported by glide-core", 0);
    RETURN_FALSE;

    char*     script;
    size_t    script_len;
    zval*     args     = NULL;
    zend_long num_keys = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(script, script_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(args)
    Z_PARAM_LONG(num_keys)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    char* script_hash = store_script_and_get_hash(script);
    if (!script_hash) {
        RETURN_FALSE;
    }

    // Split args into keys and args based on num_keys (PHPRedis style)
    zval keys_array, args_array;
    array_init(&keys_array);
    array_init(&args_array);

    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        zval*     entry;
        zend_long i = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            if (i < num_keys) {
                add_next_index_zval(&keys_array, entry);
                Z_TRY_ADDREF_P(entry);
            } else {
                add_next_index_zval(&args_array, entry);
                Z_TRY_ADDREF_P(entry);
            }
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    execute_invoke_script_command(
        valkey_glide, script_hash, &keys_array, &args_array, return_value, false);

    efree(script_hash);
    zval_dtor(&keys_array);
    zval_dtor(&args_array);
}

PHP_METHOD(ValkeyGlideCluster, evalsha) {
    // TODO: EVALSHA command is not supported by glide-core. Remove this comment when supported.
    zend_throw_exception(get_valkey_glide_cluster_exception_ce(),
                         "EVALSHA command is not supported by glide-core",
                         0);
    RETURN_FALSE;

    char*     sha1;
    size_t    sha1_len;
    zval*     args     = NULL;
    zend_long num_keys = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(sha1, sha1_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(args)
    Z_PARAM_LONG(num_keys)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    // Split args into keys and args based on num_keys (PHPRedis style)
    zval keys_array, args_array;
    array_init(&keys_array);
    array_init(&args_array);

    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        zval*     entry;
        zend_long i = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            if (i < num_keys) {
                add_next_index_zval(&keys_array, entry);
                Z_TRY_ADDREF_P(entry);
            } else {
                add_next_index_zval(&args_array, entry);
                Z_TRY_ADDREF_P(entry);
            }
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    // Use provided SHA1 hash directly (no need to hash again)
    execute_invoke_script_command(valkey_glide, sha1, &keys_array, &args_array, return_value, true);

    zval_dtor(&keys_array);
    zval_dtor(&args_array);
}

PHP_METHOD(ValkeyGlideCluster, invokeScript) {
    char*  script_or_hash;
    size_t script_or_hash_len;
    zval * keys = NULL, *args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(script_or_hash, script_or_hash_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(keys)
    Z_PARAM_ARRAY(args)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    // Prepare empty arrays if not provided
    zval empty_keys, empty_args;
    if (!keys) {
        array_init(&empty_keys);
        keys = &empty_keys;
    }
    if (!args) {
        array_init(&empty_args);
        args = &empty_args;
    }

    // Check if input looks like a SHA1 hash (40 hex characters)
    if (script_or_hash_len == 40 && strspn(script_or_hash, "0123456789abcdefABCDEF") == 40) {
        // Use as hash directly (like evalsha)
        execute_invoke_script_command(valkey_glide, script_or_hash, keys, args, return_value, true);
    } else {
        // Store script and get hash (like eval)
        char* script_hash = store_script_and_get_hash(script_or_hash);
        if (!script_hash) {
            RETURN_FALSE;
        }
        execute_invoke_script_command(valkey_glide, script_hash, keys, args, return_value, true);
        efree(script_hash);
    }

    // Clean up empty arrays if we created them
    if (!keys) {
        zval_dtor(&empty_keys);
    }
    if (!args) {
        zval_dtor(&empty_args);
    }
}
