/*
  +----------------------------------------------------------------------+
  | Valkey Glide JSON Commands                                           |
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_z_common.h"

extern zend_class_entry* get_valkey_glide_exception_ce();

/**
 * Batch result processor for JSON.SET: Ok -> true, Null -> null.
 */
static int process_json_set_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_FALSE(return_value);
        return 0;
    }
    if (response->response_type == Ok) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    if (response->response_type == Null) {
        ZVAL_NULL(return_value);
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 0;
}

/**
 * Batch result processor for JSON.GET: String -> string, Null -> null.
 */
static int process_json_get_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_NULL(return_value);
        return 0;
    }
    if (response->response_type == String && response->string_value) {
        RETVAL_STRINGL(response->string_value, response->string_value_len);
        return 1;
    }
    if (response->response_type == Null) {
        ZVAL_NULL(return_value);
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 0;
}

/**
 * Execute JSON.SET command.
 *
 * JSON.SET key path value [NX | XX]
 */
int execute_json_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *path = NULL, *value = NULL, *condition = NULL;
    size_t               key_len, path_len, value_len, condition_len         = 0;

    /* Parse parameters: key, path, value, optional condition */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osss|s!",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &path,
                                     &path_len,
                                     &value,
                                     &value_len,
                                     &condition,
                                     &condition_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Build command arguments: key path value [NX|XX] */
    int            has_condition  = (condition != NULL && condition_len > 0);
    unsigned long  full_arg_count = has_condition ? 4 : 3;
    uintptr_t*     full_args      = (uintptr_t*) emalloc(full_arg_count * sizeof(uintptr_t));
    unsigned long* full_args_len = (unsigned long*) emalloc(full_arg_count * sizeof(unsigned long));

    full_args[0]     = (uintptr_t) key;
    full_args_len[0] = key_len;
    full_args[1]     = (uintptr_t) path;
    full_args_len[1] = path_len;
    full_args[2]     = (uintptr_t) value;
    full_args_len[2] = value_len;

    if (has_condition) {
        full_args[3]     = (uintptr_t) condition;
        full_args_len[3] = condition_len;
    }

    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonSet,
                                           full_args,
                                           full_args_len,
                                           full_arg_count,
                                           NULL,
                                           process_json_set_result);
        efree(full_args);
        efree(full_args_len);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    /* Execute the command via FFI */
    CommandResult* result = execute_command(
        valkey_glide->glide_client, JsonSet, full_arg_count, full_args, full_args_len);

    efree(full_args);
    efree(full_args_len);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }

        if (result->response) {
            /* JSON.SET returns "OK" on success, null if condition not met */
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (result->response->response_type == Ok || result->response->string_value) {
                ZVAL_TRUE(return_value);
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }

    return 0;
}

/**
 * Execute JSON.GET command.
 *
 * JSON.GET key [INDENT indent] [NEWLINE newline] [SPACE space] [path [path ...]]
 */
int execute_json_get_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                paths_param   = NULL;
    zval*                options_param = NULL;

    /* Parse parameters: key, optional paths (string or array), optional options (array or object)
     */
    if (zend_parse_method_parameters(
            argc, object, "Os|zz!", &object, ce, &key, &key_len, &paths_param, &options_param) ==
        FAILURE) {
        return 0;
    }

    /* If options is an object with toArray(), call it to get the array */
    zval options_arr;
    ZVAL_UNDEF(&options_arr);
    if (options_param && Z_TYPE_P(options_param) == IS_OBJECT) {
        zval func_name;
        ZVAL_STRING(&func_name, "toArray");
        if (call_user_function(NULL, options_param, &func_name, &options_arr, 0, NULL) == SUCCESS &&
            Z_TYPE(options_arr) == IS_ARRAY) {
            options_param = &options_arr;
        }
        zval_ptr_dtor(&func_name);
    }

#define CLEANUP_OPTIONS_ARR() zval_ptr_dtor(&options_arr)

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        CLEANUP_OPTIONS_ARR();
        return 0;
    }

    /* Count formatting options (INDENT/NEWLINE/SPACE = 2 args each) */
    int         opt_count   = 0;
    const char* indent_val  = NULL;
    size_t      indent_len  = 0;
    const char* newline_val = NULL;
    size_t      newline_len = 0;
    const char* space_val   = NULL;
    size_t      space_len   = 0;

    if (options_param && Z_TYPE_P(options_param) == IS_ARRAY) {
        zval* zv;
        if ((zv = zend_hash_str_find(Z_ARRVAL_P(options_param), "indent", 6)) != NULL &&
            Z_TYPE_P(zv) == IS_STRING) {
            indent_val = Z_STRVAL_P(zv);
            indent_len = Z_STRLEN_P(zv);
            opt_count += 2;
        }
        if ((zv = zend_hash_str_find(Z_ARRVAL_P(options_param), "newline", 7)) != NULL &&
            Z_TYPE_P(zv) == IS_STRING) {
            newline_val = Z_STRVAL_P(zv);
            newline_len = Z_STRLEN_P(zv);
            opt_count += 2;
        }
        if ((zv = zend_hash_str_find(Z_ARRVAL_P(options_param), "space", 5)) != NULL &&
            Z_TYPE_P(zv) == IS_STRING) {
            space_val = Z_STRVAL_P(zv);
            space_len = Z_STRLEN_P(zv);
            opt_count += 2;
        }
    }

    /* Determine paths */
    const char* single_path     = "$";
    size_t      single_path_len = 1;
    int         path_count      = 1;
    zend_bool   use_array_paths = 0;

    if (paths_param == NULL) {
        /* default '$' */
    } else if (Z_TYPE_P(paths_param) == IS_STRING) {
        single_path     = Z_STRVAL_P(paths_param);
        single_path_len = Z_STRLEN_P(paths_param);
    } else if (Z_TYPE_P(paths_param) == IS_ARRAY) {
        path_count      = zend_hash_num_elements(Z_ARRVAL_P(paths_param));
        use_array_paths = 1;
    } else {
        php_error_docref(NULL, E_WARNING, "jsonGet expects paths as string or array");
        CLEANUP_OPTIONS_ARR();
        return 0;
    }

    /* Build args: key [INDENT v] [NEWLINE v] [SPACE v] path [path ...] */
    unsigned long  arg_count    = 1 + opt_count + path_count;
    uintptr_t*     cmd_args     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* cmd_args_len = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
    int            idx          = 0;

    cmd_args[idx]     = (uintptr_t) key;
    cmd_args_len[idx] = key_len;
    idx++;

    if (indent_val) {
        cmd_args[idx]     = (uintptr_t) "INDENT";
        cmd_args_len[idx] = 6;
        idx++;
        cmd_args[idx]     = (uintptr_t) indent_val;
        cmd_args_len[idx] = indent_len;
        idx++;
    }
    if (newline_val) {
        cmd_args[idx]     = (uintptr_t) "NEWLINE";
        cmd_args_len[idx] = 7;
        idx++;
        cmd_args[idx]     = (uintptr_t) newline_val;
        cmd_args_len[idx] = newline_len;
        idx++;
    }
    if (space_val) {
        cmd_args[idx]     = (uintptr_t) "SPACE";
        cmd_args_len[idx] = 5;
        idx++;
        cmd_args[idx]     = (uintptr_t) space_val;
        cmd_args_len[idx] = space_len;
        idx++;
    }

    if (use_array_paths) {
        zval* val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(paths_param), val) {
            if (Z_TYPE_P(val) != IS_STRING) {
                convert_to_string(val);
            }
            cmd_args[idx]     = (uintptr_t) Z_STRVAL_P(val);
            cmd_args_len[idx] = Z_STRLEN_P(val);
            idx++;
        }
        ZEND_HASH_FOREACH_END();
    } else {
        cmd_args[idx]     = (uintptr_t) single_path;
        cmd_args_len[idx] = single_path_len;
        idx++;
    }

    /* Extract option values before cleanup since they point into options_arr */

    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonGet,
                                           cmd_args,
                                           cmd_args_len,
                                           arg_count,
                                           NULL,
                                           process_json_get_result);
        efree(cmd_args);
        efree(cmd_args_len);
        CLEANUP_OPTIONS_ARR();
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    /* Execute the command via FFI */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonGet, arg_count, cmd_args, cmd_args_len);

    efree(cmd_args);
    efree(cmd_args_len);
    CLEANUP_OPTIONS_ARR();
#undef CLEANUP_OPTIONS_ARR

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }

        if (result->response) {
            /* JSON.GET returns the JSON string or null if key doesn't exist */
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (result->response->string_value) {
                RETVAL_STRINGL(result->response->string_value, result->response->string_value_len);
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }

    return 0;
}
