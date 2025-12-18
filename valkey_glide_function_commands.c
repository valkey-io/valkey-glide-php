/* Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "command_response.h"
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/* Execute FUNCTION LOAD command */
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

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionLoad,
                                 cmd_args,
                                 args_len,
                                 arg_count,
                                 NULL,
                                 process_string_response);
        efree(cmd_args);
        efree(args_len);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(valkey_glide->glide_client,
                             COMMAND_REQUEST__REQUEST_TYPE__FunctionLoad,
                             arg_count,
                             cmd_args,
                             args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION LIST command */
int execute_function_list_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                lib_name_pattern     = NULL;
    size_t               lib_name_pattern_len = 0;
    zend_bool            with_code            = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "O|sb",
                                     &object,
                                     ce,
                                     &lib_name_pattern,
                                     &lib_name_pattern_len,
                                     &with_code) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    unsigned long arg_count = 0;
    if (lib_name_pattern)
        arg_count += 2;
    if (with_code)
        arg_count += 1;

    uintptr_t* cmd_args = arg_count ? (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t)) : NULL;
    unsigned long* args_len =
        arg_count ? (unsigned long*) emalloc(arg_count * sizeof(unsigned long)) : NULL;

    unsigned long idx = 0;
    if (lib_name_pattern) {
        cmd_args[idx]   = (uintptr_t) "LIBRARYNAME";
        args_len[idx++] = 11;
        cmd_args[idx]   = (uintptr_t) lib_name_pattern;
        args_len[idx++] = lib_name_pattern_len;
    }
    if (with_code) {
        cmd_args[idx]   = (uintptr_t) "WITHCODE";
        args_len[idx++] = 8;
    }

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionList,
                                 cmd_args,
                                 args_len,
                                 arg_count,
                                 NULL,
                                 process_generic_response);
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(valkey_glide->glide_client,
                             COMMAND_REQUEST__REQUEST_TYPE__FunctionList,
                             arg_count,
                             cmd_args,
                             args_len);
    if (cmd_args)
        efree(cmd_args);
    if (args_len)
        efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = process_generic_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION FLUSH command */
int execute_function_flush_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
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

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionFlush,
                                 cmd_args,
                                 args_len,
                                 arg_count,
                                 NULL,
                                 process_string_response);
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(valkey_glide->glide_client,
                             COMMAND_REQUEST__REQUEST_TYPE__FunctionFlush,
                             arg_count,
                             cmd_args,
                             args_len);
    if (cmd_args)
        efree(cmd_args);
    if (args_len)
        efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION DELETE command */
int execute_function_delete_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                lib_name = NULL;
    size_t               lib_name_len;

    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &lib_name, &lib_name_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    uintptr_t     cmd_args[1] = {(uintptr_t) lib_name};
    unsigned long args_len[1] = {lib_name_len};

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionDelete,
                                 cmd_args,
                                 args_len,
                                 1,
                                 NULL,
                                 process_string_response);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(valkey_glide->glide_client,
                             COMMAND_REQUEST__REQUEST_TYPE__FunctionDelete,
                             1,
                             cmd_args,
                             args_len);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION DUMP command */
int execute_function_dump_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionDump,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 process_string_response);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(
        valkey_glide->glide_client, COMMAND_REQUEST__REQUEST_TYPE__FunctionDump, 0, NULL, NULL);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION RESTORE command */
int execute_function_restore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                payload = NULL;
    size_t               payload_len;
    char*                policy     = NULL;
    size_t               policy_len = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|s", &object, ce, &payload, &payload_len, &policy, &policy_len) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    unsigned long  arg_count = policy ? 2 : 1;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) payload;
    args_len[0] = payload_len;

    if (policy) {
        cmd_args[1] = (uintptr_t) policy;
        args_len[1] = policy_len;
    }

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionRestore,
                                 cmd_args,
                                 args_len,
                                 arg_count,
                                 NULL,
                                 process_string_response);
        efree(cmd_args);
        efree(args_len);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(valkey_glide->glide_client,
                             COMMAND_REQUEST__REQUEST_TYPE__FunctionRestore,
                             arg_count,
                             cmd_args,
                             args_len);
    efree(cmd_args);
    efree(args_len);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION KILL command */
int execute_function_kill_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionKill,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 process_string_response);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(
        valkey_glide->glide_client, COMMAND_REQUEST__REQUEST_TYPE__FunctionKill, 0, NULL, NULL);

    if (result && !result->command_error && result->response) {
        int status = process_string_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}

/* Execute FUNCTION STATS command */
int execute_function_stats_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide->glide_client) {
        return 0;
    }

    CommandResult* result = NULL;
    if (valkey_glide->is_in_batch_mode) {
        buffer_command_for_batch(valkey_glide,
                                 COMMAND_REQUEST__REQUEST_TYPE__FunctionStats,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 process_generic_response);
        ZVAL_COPY(return_value, object);
        return 1;
    }

    result = execute_command(
        valkey_glide->glide_client, COMMAND_REQUEST__REQUEST_TYPE__FunctionStats, 0, NULL, NULL);

    if (result && !result->command_error && result->response) {
        int status = process_generic_response(result->response, NULL, return_value);
        free_command_result(result);
        return status;
    }
    if (result)
        free_command_result(result);
    return 0;
}
