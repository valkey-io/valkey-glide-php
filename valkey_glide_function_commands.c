/* Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/**
 * Helper function to handle CommandResult for boolean function commands
 * Returns 0 on error, otherwise returns the status from process_core_bool_result
 */
static int handle_function_bool_result(CommandResult* result, zval* return_value) {
    if (!result || result->command_error || !result->response) {
        ZVAL_FALSE(return_value);
        if (result) {
            free_command_result(result);
        }
        return 0;
    }

    int status = process_core_bool_result(result->response, NULL, return_value);
    free_command_result(result);
    return status;
}

/**
 * Internal helper for FUNCTION LOAD - shared by functionLoad() and function('LOAD')
 */
int execute_function_load_internal(valkey_glide_object* valkey_glide,
                                   char*                library_code,
                                   size_t               library_code_len,
                                   zend_bool            replace,
                                   zval*                return_value) {
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    unsigned long  arg_count = replace ? 2 : 1;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) library_code;
    args_len[0] = library_code_len;

    if (replace) {
        cmd_args[1] = (uintptr_t) "REPLACE";
        args_len[1] = strlen("REPLACE");
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionLoad, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    return handle_function_command_result_or_return_false(result, return_value);
}

int execute_function_load_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                library_code = NULL;
    size_t               library_code_len;
    zend_bool            replace = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|b", &object, ce, &library_code, &library_code_len, &replace) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    return execute_function_load_internal(
        valkey_glide, library_code, library_code_len, replace, return_value);
}

int execute_function_list_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    char*  library_name = NULL;
    size_t library_name_len;

    /* Parse parameters: optional library_name */
    if (zend_parse_method_parameters(
            argc, object, "O|s", &object, ce, &library_name, &library_name_len) == FAILURE) {
        return 0;
    }

    CommandResult* result;
    if (argc > 0) {
        uintptr_t     cmd_args[1] = {(uintptr_t) library_name};
        unsigned long args_len[1] = {library_name_len};
        result = execute_command(valkey_glide->glide_client, FunctionList, 1, cmd_args, args_len);
    } else {
        result = execute_command(valkey_glide->glide_client, FunctionList, 0, NULL, NULL);
    }

    return handle_function_command_result_or_return_false(result, return_value);
}

int execute_function_flush_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionFlush, 0, NULL, NULL);

    return handle_function_bool_result(result, return_value);
}

int execute_function_delete_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                lib_name = NULL;
    size_t               lib_name_len;

    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &lib_name, &lib_name_len) ==
        FAILURE) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    uintptr_t     cmd_args[1] = {(uintptr_t) lib_name};
    unsigned long args_len[1] = {lib_name_len};

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionDelete, 1, cmd_args, args_len);

    return handle_function_bool_result(result, return_value);
}

int execute_function_dump_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionDump, 0, NULL, NULL);
    return handle_function_command_result_or_return_false(result, return_value);
}

int execute_function_restore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                payload = NULL;
    size_t               payload_len;

    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &payload, &payload_len) ==
        FAILURE) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    uintptr_t     cmd_args[1] = {(uintptr_t) payload};
    unsigned long args_len[1] = {payload_len};

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionRestore, 1, cmd_args, args_len);

    return handle_function_bool_result(result, return_value);
}

int execute_function_kill_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionKill, 0, NULL, NULL);

    return handle_function_bool_result(result, return_value);
}

int execute_function_stats_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionStats, 0, NULL, NULL);
    return handle_function_command_result_or_return_false(result, return_value);
}
