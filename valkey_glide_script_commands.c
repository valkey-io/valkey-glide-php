/* Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/* Helper function to process array arguments */
static void process_array_to_args(zval*          array,
                                  uintptr_t*     cmd_args,
                                  unsigned long* args_len,
                                  unsigned long* arg_index) {
    if (array && Z_TYPE_P(array) == IS_ARRAY) {
        zval* val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), val) {
            if (Z_TYPE_P(val) != IS_STRING) {
                zval temp;
                ZVAL_COPY(&temp, val);
                convert_to_string(&temp);
                cmd_args[*arg_index] = (uintptr_t) Z_STRVAL(temp);
                args_len[*arg_index] = Z_STRLEN(temp);
                zval_dtor(&temp);
            } else {
                cmd_args[*arg_index] = (uintptr_t) Z_STRVAL_P(val);
                args_len[*arg_index] = Z_STRLEN_P(val);
            }
            (*arg_index)++;
        }
        ZEND_HASH_FOREACH_END();
    }
}

/* Execute EVAL command */
int execute_eval_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                script = NULL;
    size_t               script_len;
    zval*                args_array = NULL;
    zend_long            num_keys   = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|al", &object, ce, &script, &script_len, &args_array, &num_keys) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), ZEND_LONG_FMT, num_keys);

    unsigned long  args_count = args_array && Z_TYPE_P(args_array) == IS_ARRAY
                                    ? zend_hash_num_elements(Z_ARRVAL_P(args_array))
                                    : 0;
    unsigned long  arg_count  = 2 + args_count;
    uintptr_t*     cmd_args   = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len   = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) script;
    args_len[0] = script_len;
    cmd_args[1] = (uintptr_t) numkeys_str;
    args_len[1] = strlen(numkeys_str);

    unsigned long arg_index = 2;
    process_array_to_args(args_array, cmd_args, args_len, &arg_index);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, Eval, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}
/* Execute EVAL_RO command */
int execute_eval_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                script = NULL;
    size_t               script_len;
    zval*                args_array = NULL;
    zend_long            num_keys   = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|al", &object, ce, &script, &script_len, &args_array, &num_keys) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), ZEND_LONG_FMT, num_keys);

    unsigned long  args_count = args_array && Z_TYPE_P(args_array) == IS_ARRAY
                                    ? zend_hash_num_elements(Z_ARRVAL_P(args_array))
                                    : 0;
    unsigned long  arg_count  = 2 + args_count;
    uintptr_t*     cmd_args   = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len   = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) script;
    args_len[0] = script_len;
    cmd_args[1] = (uintptr_t) numkeys_str;
    args_len[1] = strlen(numkeys_str);

    unsigned long arg_index = 2;
    process_array_to_args(args_array, cmd_args, args_len, &arg_index);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, EvalReadOnly, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute EVALSHA command */
int execute_evalsha_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                sha1 = NULL;
    size_t               sha1_len;
    zval*                args_array = NULL;
    zend_long            num_keys   = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|al", &object, ce, &sha1, &sha1_len, &args_array, &num_keys) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), ZEND_LONG_FMT, num_keys);

    unsigned long  args_count = args_array && Z_TYPE_P(args_array) == IS_ARRAY
                                    ? zend_hash_num_elements(Z_ARRVAL_P(args_array))
                                    : 0;
    unsigned long  arg_count  = 2 + args_count;
    uintptr_t*     cmd_args   = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len   = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) sha1;
    args_len[0] = sha1_len;
    cmd_args[1] = (uintptr_t) numkeys_str;
    args_len[1] = strlen(numkeys_str);

    unsigned long arg_index = 2;
    process_array_to_args(args_array, cmd_args, args_len, &arg_index);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, EvalSha, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute EVALSHA_RO command */
int execute_evalsha_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                sha1 = NULL;
    size_t               sha1_len;
    zval*                args_array = NULL;
    zend_long            num_keys   = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|al", &object, ce, &sha1, &sha1_len, &args_array, &num_keys) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), ZEND_LONG_FMT, num_keys);

    unsigned long  args_count = args_array && Z_TYPE_P(args_array) == IS_ARRAY
                                    ? zend_hash_num_elements(Z_ARRVAL_P(args_array))
                                    : 0;
    unsigned long  arg_count  = 2 + args_count;
    uintptr_t*     cmd_args   = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len   = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) sha1;
    args_len[0] = sha1_len;
    cmd_args[1] = (uintptr_t) numkeys_str;
    args_len[1] = strlen(numkeys_str);

    unsigned long arg_index = 2;
    process_array_to_args(args_array, cmd_args, args_len, &arg_index);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, EvalShaReadOnly, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute SCRIPT EXISTS command */
int execute_script_exists_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                sha1s_array = NULL;

    if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &sha1s_array) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    unsigned long  args_count = zend_hash_num_elements(Z_ARRVAL_P(sha1s_array));
    uintptr_t*     cmd_args   = (uintptr_t*) emalloc(args_count * sizeof(uintptr_t));
    unsigned long* args_len   = (unsigned long*) emalloc(args_count * sizeof(unsigned long));

    unsigned long arg_index = 0;
    process_array_to_args(sha1s_array, cmd_args, args_len, &arg_index);

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptExists, args_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute SCRIPT FLUSH command */
int execute_script_flush_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                mode     = NULL;
    size_t               mode_len = 0;

    if (zend_parse_method_parameters(argc, object, "O|s", &object, ce, &mode, &mode_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    unsigned long  arg_count = mode ? 1 : 0;
    uintptr_t*     cmd_args  = arg_count ? (uintptr_t*) emalloc(sizeof(uintptr_t)) : NULL;
    unsigned long* args_len  = arg_count ? (unsigned long*) emalloc(sizeof(unsigned long)) : NULL;

    if (mode) {
        cmd_args[0] = (uintptr_t) mode;
        args_len[0] = mode_len;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptFlush, arg_count, cmd_args, args_len);
    if (cmd_args)
        efree(cmd_args);
    if (args_len)
        efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute SCRIPT KILL command */
int execute_script_kill_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptKill, 0, NULL, NULL);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute SCRIPT SHOW command */
int execute_script_show_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                sha1 = NULL;
    size_t               sha1_len;

    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &sha1, &sha1_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    uintptr_t     cmd_args[1] = {(uintptr_t) sha1};
    unsigned long args_len[1] = {sha1_len};

    CommandResult* result =
        execute_command(valkey_glide->glide_client, ScriptShow, 1, cmd_args, args_len);

    if (result && !result->command_error && result->response) {
        int status = command_response_to_zval(result->response, return_value, 0, false);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}
