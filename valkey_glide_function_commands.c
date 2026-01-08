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
    if (!valkey_glide->glide_client) {
        return 0;
    }

    unsigned long  arg_count = replace ? 2 : 1;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) library_code;
    args_len[0] = library_code_len;

    if (replace) {
        cmd_args[1] = (uintptr_t) "REPLACE";
        args_len[1] = 7;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionLoad, arg_count, cmd_args, args_len);
    efree(cmd_args);
    efree(args_len);

    return handle_function_command_result_or_return_false(result, "FunctionLoad", return_value);
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

    CommandResult* result =
        execute_command(valkey_glide->glide_client, FunctionList, 0, NULL, NULL);

    return handle_function_command_result_or_return_false(result, "FunctionList", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionFlush", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionDelete", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionDump", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionRestore", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionKill", return_value);
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
    return handle_function_command_result_or_return_false(result, "FunctionStats", return_value);
}
