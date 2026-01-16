#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

// Helper macros for validating CommandResult in script commands
#define VALIDATE_SCRIPT_RESULT_OR_RETURN_FALSE(result, return_value)       \
    do {                                                                   \
        if (!(result) || (result)->command_error || !(result)->response) { \
            if (result) {                                                  \
                free_command_result(result);                               \
            }                                                              \
            RETURN_FALSE;                                                  \
        }                                                                  \
    } while (0)

#define VALIDATE_SCRIPT_RESULT_OR_RETURN_NULL(result, return_value)        \
    do {                                                                   \
        if (!(result) || (result)->command_error || !(result)->response) { \
            if (result) {                                                  \
                free_command_result(result);                               \
            }                                                              \
            RETURN_NULL();                                                 \
        }                                                                  \
    } while (0)

#define VALIDATE_SCRIPT_RESULT_NO_RESPONSE_OR_RETURN_FALSE(result, return_value) \
    do {                                                                         \
        if (!(result) || (result)->command_error) {                              \
            if (result) {                                                        \
                free_command_result(result);                                     \
            }                                                                    \
            RETURN_FALSE;                                                        \
        }                                                                        \
    } while (0)

/**
 * Helper function to handle CommandResult for boolean script commands (void return)
 */
static void handle_script_bool_result(CommandResult* result, zval* return_value) {
    if (!result || result->command_error || !result->response) {
        ZVAL_FALSE(return_value);
        if (result) {
            free_command_result(result);
        }
        return;
    }

    process_core_bool_result(result->response, NULL, return_value);
    free_command_result(result);
}
#include "zend_exceptions.h"

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

// Script execution using invoke_script FFI
// Helper to store script and get hash
char* store_script_and_get_hash(const char* script) {
    struct ScriptHashBuffer* hash_buffer = store_script((const uint8_t*) script, strlen(script));
    if (!hash_buffer || !hash_buffer->ptr) {
        if (hash_buffer) {
            free_script_hash_buffer(hash_buffer);
        }
        return NULL;
    }

    char* hash = estrndup((const char*) hash_buffer->ptr, hash_buffer->len);
    free_script_hash_buffer(hash_buffer);
    return hash;
}

// Helper function for script flush command
void execute_script_flush_command(zval* object, zval* return_value, bool is_cluster) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return;
    }
    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptFlush, 0, NULL, NULL);
    handle_script_bool_result(result, return_value);
}


// Helper to build and execute eval-style commands
static void execute_eval_style_command(const char* cmd_name,
                                       size_t      cmd_len,
                                       char*       script_or_sha,
                                       size_t      script_len,
                                       zval*       keys_array,
                                       zval*       args_array,
                                       zend_long   num_keys,
                                       bool        num_keys_set,
                                       zval*       object,
                                       zval*       return_value) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide->glide_client) {
        RETURN_FALSE;
    }

    int keys_count = keys_array ? zend_hash_num_elements(Z_ARRVAL_P(keys_array)) : 0;
    int args_count = args_array ? zend_hash_num_elements(Z_ARRVAL_P(args_array)) : 0;

    if (!num_keys_set) {
        num_keys = keys_count;
    }

    int            cmd_count = 3 + keys_count + args_count;  // cmd + script + numkeys + keys + args
    uintptr_t*     cmd_args  = emalloc(sizeof(uintptr_t) * cmd_count);
    unsigned long* cmd_args_len = emalloc(sizeof(unsigned long) * cmd_count);

    int idx             = 0;
    cmd_args[idx]       = (uintptr_t) cmd_name;
    cmd_args_len[idx++] = cmd_len;

    cmd_args[idx]       = (uintptr_t) script_or_sha;
    cmd_args_len[idx++] = script_len;

    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%lld", (long long) num_keys);
    cmd_args[idx]       = (uintptr_t) numkeys_str;
    cmd_args_len[idx++] = strlen(numkeys_str);

    if (keys_array) {
        zval* entry;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(keys_array), entry) {
            convert_to_string(entry);
            cmd_args[idx]       = (uintptr_t) Z_STRVAL_P(entry);
            cmd_args_len[idx++] = Z_STRLEN_P(entry);
        }
        ZEND_HASH_FOREACH_END();
    }

    if (args_array) {
        zval* entry;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args_array), entry) {
            convert_to_string(entry);
            cmd_args[idx]       = (uintptr_t) Z_STRVAL_P(entry);
            cmd_args_len[idx++] = Z_STRLEN_P(entry);
        }
        ZEND_HASH_FOREACH_END();
    }

    CommandResult* result = execute_command(
        valkey_glide->glide_client, CustomCommand, cmd_count, cmd_args, cmd_args_len);

    efree(cmd_args);
    efree(cmd_args_len);

    VALIDATE_SCRIPT_RESULT_NO_RESPONSE_OR_RETURN_FALSE(result, return_value);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Helper to split PHPRedis-style combined args array into keys and args
static void split_args_array(zval* args_array, zend_long num_keys, zval* keys_out, zval* args_out) {
    if (!args_array || num_keys == 0) {
        return;
    }

    int total_args = zend_hash_num_elements(Z_ARRVAL_P(args_array));

    // Initialize output arrays
    if (num_keys > 0) {
        array_init_size(keys_out, num_keys);
    }
    if (total_args > num_keys) {
        array_init_size(args_out, total_args - num_keys);
    }

    int   idx = 0;
    zval* entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args_array), entry) {
        if (idx < num_keys) {
            Z_TRY_ADDREF_P(entry);
            add_next_index_zval(keys_out, entry);
        } else {
            Z_TRY_ADDREF_P(entry);
            add_next_index_zval(args_out, entry);
        }
        idx++;
    }
    ZEND_HASH_FOREACH_END();
}

// Generic implementation for all eval-style commands
static void execute_eval_generic(const char* command_name,
                                 size_t      command_name_len,
                                 zval*       object,
                                 int         argc,
                                 zval*       return_value,
                                 bool        is_cluster) {
    char*     first_param;
    size_t    first_param_len;
    zval*     args_array = NULL;
    zend_long num_keys   = 0;

    if (argc < 1 || argc > 3) {
        RETURN_FALSE;
    }

    zval* params = emalloc(sizeof(zval) * argc);
    if (zend_get_parameters_array_ex(argc, params) == FAILURE) {
        efree(params);
        RETURN_FALSE;
    }

    if (Z_TYPE(params[0]) != IS_STRING) {
        efree(params);
        RETURN_FALSE;
    }
    first_param     = Z_STRVAL(params[0]);
    first_param_len = Z_STRLEN(params[0]);

    if (argc >= 2 && Z_TYPE(params[1]) == IS_ARRAY) {
        args_array = &params[1];
    }
    if (argc >= 3 && Z_TYPE(params[2]) == IS_LONG) {
        num_keys = Z_LVAL(params[2]);
    }

    // Split args_array into keys and args based on num_keys (PHPRedis compatibility)
    zval  keys_zval, args_zval;
    zval* keys_array = NULL;
    zval* argv_array = NULL;

    if (args_array && num_keys > 0) {
        ZVAL_UNDEF(&keys_zval);
        ZVAL_UNDEF(&args_zval);
        split_args_array(args_array, num_keys, &keys_zval, &args_zval);
        keys_array = &keys_zval;
        if (Z_TYPE(args_zval) == IS_ARRAY) {
            argv_array = &args_zval;
        }
    } else if (args_array) {
        argv_array = args_array;
    }

    execute_eval_style_command(command_name,
                               command_name_len,
                               first_param,
                               first_param_len,
                               keys_array,
                               argv_array,
                               num_keys,
                               true,
                               object,
                               return_value);

    if (keys_array && Z_TYPE_P(keys_array) == IS_ARRAY) {
        zval_ptr_dtor(keys_array);
    }
    if (argv_array && argv_array != args_array && Z_TYPE_P(argv_array) == IS_ARRAY) {
        zval_ptr_dtor(argv_array);
    }
    efree(params);
}

void execute_eval_command(zval* object, int argc, zval* return_value, bool is_cluster) {
    execute_eval_generic("EVAL", strlen("EVAL"), object, argc, return_value, is_cluster);
}

void execute_evalsha_command(zval* object, int argc, zval* return_value, bool is_cluster) {
    execute_eval_generic("EVALSHA", strlen("EVALSHA"), object, argc, return_value, is_cluster);
}

void execute_eval_ro_command(zval* object, int argc, zval* return_value, bool is_cluster) {
    execute_eval_generic("EVAL_RO", strlen("EVAL_RO"), object, argc, return_value, is_cluster);
}

void execute_evalsha_ro_command(zval* object, int argc, zval* return_value, bool is_cluster) {
    execute_eval_generic(
        "EVALSHA_RO", strlen("EVALSHA_RO"), object, argc, return_value, is_cluster);
}

void execute_script_exists_command(zval* object, zval* sha1s, zval* return_value, bool is_cluster) {
    if (Z_TYPE_P(sha1s) != IS_ARRAY) {
        RETURN_FALSE;
    }

    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide->glide_client) {
        RETURN_FALSE;
    }

    int            count        = zend_hash_num_elements(Z_ARRVAL_P(sha1s));
    uintptr_t*     cmd_args     = emalloc(sizeof(uintptr_t) * count);
    unsigned long* cmd_args_len = emalloc(sizeof(unsigned long) * count);

    int   idx = 0;
    zval* entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(sha1s), entry) {
        convert_to_string(entry);
        cmd_args[idx]     = (uintptr_t) Z_STRVAL_P(entry);
        cmd_args_len[idx] = Z_STRLEN_P(entry);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptExists, count, cmd_args, cmd_args_len);

    efree(cmd_args);
    efree(cmd_args_len);

    VALIDATE_SCRIPT_RESULT_OR_RETURN_FALSE(result, return_value);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Helper function for script show command
void execute_script_show_command(
    zval* object, char* sha1, size_t sha1_len, zval* return_value, bool is_cluster) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide->glide_client) {
        RETURN_NULL();
    }

    uintptr_t     cmd_args[1]     = {(uintptr_t) sha1};
    unsigned long cmd_args_len[1] = {sha1_len};

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptShow, 1, cmd_args, cmd_args_len);

    VALIDATE_SCRIPT_RESULT_OR_RETURN_NULL(result, return_value);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Helper function for script kill command
void execute_script_kill_command(zval* object, zval* return_value, bool is_cluster) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide->glide_client) {
        RETURN_FALSE;
    }

    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptKill, 0, NULL, NULL);

    VALIDATE_SCRIPT_RESULT_NO_RESPONSE_OR_RETURN_FALSE(result, return_value);

    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}
