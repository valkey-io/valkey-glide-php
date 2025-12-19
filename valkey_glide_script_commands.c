#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_script_common.h"

// Helper function to process array arguments
char** process_array_to_args(zval* array, int* count) {
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
                                   zval*                return_value) {
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

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);

    // Cleanup
    if (key_ptrs)
        efree(key_ptrs);
    if (key_lens)
        efree(key_lens);
    if (arg_ptrs)
        efree(arg_ptrs);
    if (arg_lens)
        efree(arg_lens);
}

// Helper to store script and get hash (like Go's storeScript)
static char* store_script_and_get_hash(const char* script) {
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

// Script management commands using RequestType (like Go's executeCommand)


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

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

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
    execute_invoke_script_command(valkey_glide, script_hash, keys, args, return_value);

    efree(script_hash);
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

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

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
        valkey_glide, script_hash, &keys_array, &args_array, return_value);

    efree(script_hash);
    zval_dtor(&keys_array);
    zval_dtor(&args_array);
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

    execute_invoke_script_command(valkey_glide, sha1, &keys_array, &args_array, return_value);

    zval_dtor(&keys_array);
    zval_dtor(&args_array);
}

PHP_METHOD(ValkeyGlide, scriptExists) {
    zval* sha1s;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_ARRAY(sha1s)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    int            count;
    uintptr_t*     args;
    unsigned long* args_len;
    process_array_to_uintptr_args(sha1s, &count, &args, &args_len);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptExists, count, args, args_len);

    // Free allocated memory
    if (args) {
        efree(args);
    }
    if (args_len) {
        efree(args_len);
    }

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Cluster implementations
PHP_METHOD(ValkeyGlideCluster, eval) {
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

    int            count = 0;
    uintptr_t*     cmd_args;
    unsigned long* cmd_args_len;

    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        count = zend_hash_num_elements(Z_ARRVAL_P(args));
    }

    cmd_args     = emalloc((count + 2) * sizeof(uintptr_t));
    cmd_args_len = emalloc((count + 2) * sizeof(unsigned long));

    cmd_args[0]     = (uintptr_t) script;
    cmd_args_len[0] = script_len;
    cmd_args[1]     = (uintptr_t) &num_keys;
    cmd_args_len[1] = sizeof(zend_long);

    if (args && count > 0) {
        zval* entry;
        int   i = 2;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            convert_to_string(entry);
            cmd_args[i]     = (uintptr_t) Z_STRVAL_P(entry);
            cmd_args_len[i] = Z_STRLEN_P(entry);
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, Eval, count + 2, cmd_args, cmd_args_len);

    efree(cmd_args);
    efree(cmd_args_len);
    efree(script_hash);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

PHP_METHOD(ValkeyGlideCluster, evalsha) {
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

    int            count = 0;
    uintptr_t*     cmd_args;
    unsigned long* cmd_args_len;

    if (args && Z_TYPE_P(args) == IS_ARRAY) {
        count = zend_hash_num_elements(Z_ARRVAL_P(args));
    }

    cmd_args     = emalloc((count + 2) * sizeof(uintptr_t));
    cmd_args_len = emalloc((count + 2) * sizeof(unsigned long));

    cmd_args[0]     = (uintptr_t) sha1;
    cmd_args_len[0] = sha1_len;
    cmd_args[1]     = (uintptr_t) &num_keys;
    cmd_args_len[1] = sizeof(zend_long);

    if (args && count > 0) {
        zval* entry;
        int   i = 2;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), entry) {
            convert_to_string(entry);
            cmd_args[i]     = (uintptr_t) Z_STRVAL_P(entry);
            cmd_args_len[i] = Z_STRLEN_P(entry);
            i++;
        }
        ZEND_HASH_FOREACH_END();
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, EvalSha, count + 2, cmd_args, cmd_args_len);

    efree(cmd_args);
    efree(cmd_args_len);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

PHP_METHOD(ValkeyGlide, scriptFlush) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptFlush, 0, NULL, NULL);
    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

PHP_METHOD(ValkeyGlide, scriptKill) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptKill, 0, NULL, NULL);
    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

PHP_METHOD(ValkeyGlide, scriptShow) {
    char*  sha1;
    size_t sha1_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_STRING(sha1, sha1_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    uintptr_t      args[]     = {(uintptr_t) sha1};
    unsigned long  args_len[] = {sha1_len};
    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptShow, 1, args, args_len);
    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Function declarations for helper functions
char** process_array_to_args(zval* array, int* count);
