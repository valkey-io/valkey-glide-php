#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

// Helper function to process array arguments
static char** process_array_to_args(zval* array, int* count) {
    if (Z_TYPE_P(array) != IS_ARRAY) {
        *count = 0;
        return NULL;
    }

    *count = zend_hash_num_elements(Z_ARRVAL_P(array));
    if (*count == 0) {
        return NULL;
    }

    char** args = emalloc(sizeof(char*) * (*count));
    zval*  entry;
    int    i = 0;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
        convert_to_string(entry);
        args[i] = estrdup(Z_STRVAL_P(entry));
        i++;
    }
    ZEND_HASH_FOREACH_END();

    return args;
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
zval execute_invoke_script_command(valkey_glide_object* valkey_glide,
                                   const char*          script_hash,
                                   zval*                keys,
                                   zval*                args) {
    // Prepare FFI arguments
    uintptr_t *    key_ptrs, *arg_ptrs;
    unsigned long *key_lens, *arg_lens;
    unsigned long  key_count, arg_count;

    prepare_ffi_args(keys, &key_ptrs, &key_lens, &key_count);
    prepare_ffi_args(args, &arg_ptrs, &arg_lens, &arg_count);

    // Call invoke_script (like Go's executeScriptWithRoute)
    struct CommandResult* result = invoke_script(valkey_glide->client,
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

    zval php_result = command_response_to_zval(result);

    // Cleanup
    if (key_ptrs)
        efree(key_ptrs);
    if (key_lens)
        efree(key_lens);
    if (arg_ptrs)
        efree(arg_ptrs);
    if (arg_lens)
        efree(arg_lens);

    return php_result;
}

// Helper to store script and get hash (like Go's storeScript)
static char* store_script_and_get_hash(const char* script) {
    struct ScriptHashBuffer* hash_buffer = store_script((const uint8_t*) script, strlen(script));
    if (!hash_buffer || !hash_buffer->hash) {
        if (hash_buffer) {
            free_script_hash_buffer(hash_buffer);
        }
        return NULL;
    }

    char* hash = estrdup(hash_buffer->hash);
    free_script_hash_buffer(hash_buffer);
    return hash;
}

// Script management commands using RequestType (like Go's executeCommand)
zval execute_script_exists_command(valkey_glide_object* valkey_glide, zval* sha1s) {
    int    count;
    char** args = process_array_to_args(sha1s, &count);

    CommandResult result = execute_command(valkey_glide->client, ScriptExists, args, count);

    // Free allocated memory
    if (args) {
        for (int i = 0; i < count; i++) {
            efree(args[i]);
        }
        efree(args);
    }

    return command_response_to_zval(&result);
}

zval execute_script_flush_command(valkey_glide_object* valkey_glide) {
    CommandResult result = execute_command(valkey_glide->client, ScriptFlush, NULL, 0);
    return command_response_to_zval(&result);
}

zval execute_script_kill_command(valkey_glide_object* valkey_glide) {
    CommandResult result = execute_command(valkey_glide->client, ScriptKill, NULL, 0);
    return command_response_to_zval(&result);
}

zval execute_script_show_command(valkey_glide_object* valkey_glide, const char* sha1) {
    char*         args[] = {(char*) sha1};
    CommandResult result = execute_command(valkey_glide->client, ScriptShow, args, 1);
    return command_response_to_zval(&result);
}

// PHP method implementations following Go pattern
PHP_METHOD(ValkeyGlide, invokeScript) {
    char*  script;
    size_t script_len;
    zval * keys = NULL, *args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
    Z_PARAM_STRING(script, script_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(keys)
    Z_PARAM_ARRAY(args)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);

    // Store script and get hash (like Go's NewScript)
    char* script_hash = store_script_and_get_hash(script);
    if (!script_hash) {
        RETURN_FALSE;
    }

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

    // Execute script (like Go's InvokeScript)
    zval result = execute_invoke_script_command(valkey_glide, script_hash, keys, args);

    efree(script_hash);
    RETURN_ZVAL(&result, 1, 1);
}

// PHPRedis compatibility methods
PHP_METHOD(ValkeyGlide, eval) {
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

    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);

    // Store script and get hash
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
            } else {
                add_next_index_zval(&args_array, entry);
            }
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    zval result =
        execute_invoke_script_command(valkey_glide, script_hash, &keys_array, &args_array);

    efree(script_hash);
    zval_dtor(&keys_array);
    zval_dtor(&args_array);
    RETURN_ZVAL(&result, 1, 1);
}

PHP_METHOD(ValkeyGlide, evalsha) {
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

    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);

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
            } else {
                add_next_index_zval(&args_array, entry);
            }
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    zval result = execute_invoke_script_command(valkey_glide, sha1, &keys_array, &args_array);

    zval_dtor(&keys_array);
    zval_dtor(&args_array);
    RETURN_ZVAL(&result, 1, 1);
}

PHP_METHOD(ValkeyGlide, scriptExists) {
    zval* sha1s;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_ARRAY(sha1s)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);
    zval                 result       = execute_script_exists_command(valkey_glide, sha1s);
    RETURN_ZVAL(&result, 1, 1);
}

PHP_METHOD(ValkeyGlide, scriptFlush) {
    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);
    zval                 result       = execute_script_flush_command(valkey_glide);
    RETURN_ZVAL(&result, 1, 1);
}

PHP_METHOD(ValkeyGlide, scriptKill) {
    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);
    zval                 result       = execute_script_kill_command(valkey_glide);
    RETURN_ZVAL(&result, 1, 1);
}

PHP_METHOD(ValkeyGlide, scriptShow) {
    char*  sha1;
    size_t sha1_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_STRING(sha1, sha1_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide = get_valkey_glide_object(ZEND_THIS);
    zval                 result       = execute_script_show_command(valkey_glide, sha1);
    RETURN_ZVAL(&result, 1, 1);
}
