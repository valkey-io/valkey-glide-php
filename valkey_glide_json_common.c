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

    /* Parse parameters: key, optional paths (string or array), optional options array */
    if (zend_parse_method_parameters(
            argc, object, "Os|za!", &object, ce, &key, &key_len, &paths_param, &options_param) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
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

/* ====================================================================
 * GENERIC JSON HELPERS
 * ==================================================================== */

/**
 * Batch result processor for JSON commands returning mixed types.
 * Uses command_response_to_zval for generic conversion.
 */
static int process_json_mixed_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Generic executor for JSON commands with pattern: key [path]
 * Returns mixed (int, array, string, null) via command_response_to_zval.
 */
static int execute_json_key_path_command(
    zval* object, int argc, zval* return_value, zend_class_entry* ce, enum RequestType cmd_type) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    size_t               key_len, path_len = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|s", &object, ce, &key, &key_len, &path, &path_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    unsigned long arg_count   = (path && path_len > 0) ? 2 : 1;
    uintptr_t     cmd_args[2] = {(uintptr_t) key, (uintptr_t) path};
    unsigned long cmd_lens[2] = {key_len, path_len};

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(
            valkey_glide, cmd_type, cmd_args, cmd_lens, arg_count, NULL, process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, arg_count, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.DEL / JSON.FORGET / JSON.CLEAR
 * ==================================================================== */

int execute_json_del_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonDel);
}

int execute_json_forget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonForget);
}

int execute_json_clear_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonClear);
}

/* ====================================================================
 * JSON.TYPE / JSON.TOGGLE / JSON.ARRLEN / JSON.STRLEN
 * JSON.OBJLEN / JSON.OBJKEYS / JSON.RESP
 * ==================================================================== */

int execute_json_type_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonType);
}

int execute_json_toggle_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonToggle);
}

int execute_json_arrlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonArrLen);
}

int execute_json_strlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonStrLen);
}

int execute_json_objlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonObjLen);
}

int execute_json_objkeys_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonObjKeys);
}

int execute_json_resp_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_json_key_path_command(object, argc, return_value, ce, JsonResp);
}

/* ====================================================================
 * JSON.MGET — keys... path
 * ==================================================================== */

int execute_json_mget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                keys_param = NULL;
    char*                path       = NULL;
    size_t               path_len;

    if (zend_parse_method_parameters(
            argc, object, "Oas", &object, ce, &keys_param, &path, &path_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    HashTable*    ht        = Z_ARRVAL_P(keys_param);
    int           key_count = zend_hash_num_elements(ht);
    unsigned long arg_count = key_count + 1; /* keys... + path */

    uintptr_t*     cmd_args = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* cmd_lens = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    zval* val;
    int   idx = 0;
    ZEND_HASH_FOREACH_VAL(ht, val) {
        if (Z_TYPE_P(val) != IS_STRING) {
            convert_to_string(val);
        }
        cmd_args[idx] = (uintptr_t) Z_STRVAL_P(val);
        cmd_lens[idx] = Z_STRLEN_P(val);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    cmd_args[idx] = (uintptr_t) path;
    cmd_lens[idx] = path_len;

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(
            valkey_glide, JsonMGet, cmd_args, cmd_lens, arg_count, NULL, process_json_mixed_result);
        efree(cmd_args);
        efree(cmd_lens);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonMGet, arg_count, cmd_args, cmd_lens);
    efree(cmd_args);
    efree(cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.NUMINCRBY / JSON.NUMMULTBY — key path number → string
 * ==================================================================== */

static int execute_json_num_command(
    zval* object, int argc, zval* return_value, zend_class_entry* ce, enum RequestType cmd_type) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    double               number;
    size_t               key_len, path_len;

    if (zend_parse_method_parameters(
            argc, object, "Ossd", &object, ce, &key, &key_len, &path, &path_len, &number) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    char   num_buf[64];
    size_t num_len = snprintf(num_buf, sizeof(num_buf), "%.17g", number);
    /* Strip trailing zeros after decimal point for cleaner output */
    if (strchr(num_buf, '.')) {
        char* p = num_buf + num_len - 1;
        while (*p == '0') {
            p--;
        }
        if (*p == '.') {
            p--;
        }
        num_len          = (size_t) (p - num_buf + 1);
        num_buf[num_len] = '\0';
    }

    uintptr_t     cmd_args[3] = {(uintptr_t) key, (uintptr_t) path, (uintptr_t) num_buf};
    unsigned long cmd_lens[3] = {key_len, path_len, num_len};

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(
            valkey_glide, cmd_type, cmd_args, cmd_lens, 3, NULL, process_json_get_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, 3, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
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

int execute_json_numincrby_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    return execute_json_num_command(object, argc, return_value, ce, JsonNumIncrBy);
}

int execute_json_nummultby_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    return execute_json_num_command(object, argc, return_value, ce, JsonNumMultBy);
}

/* ====================================================================
 * JSON.STRAPPEND — key [path] value
 * ==================================================================== */

int execute_json_strappend_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key   = NULL;
    char*                path  = NULL;
    char*                value = NULL;
    size_t               key_len, path_len = 0, value_len;

    /* Two forms: (key, value) or (key, path, value) */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss|s",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &path,
                                     &path_len,
                                     &value,
                                     &value_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    uintptr_t     cmd_args[3];
    unsigned long cmd_lens[3];
    unsigned long arg_count;

    if (value == NULL) {
        /* 2 args: key, value (no path) */
        cmd_args[0] = (uintptr_t) key;
        cmd_lens[0] = key_len;
        cmd_args[1] = (uintptr_t) path; /* path is actually the value */
        cmd_lens[1] = path_len;
        arg_count   = 2;
    } else {
        /* 3 args: key, path, value */
        cmd_args[0] = (uintptr_t) key;
        cmd_lens[0] = key_len;
        cmd_args[1] = (uintptr_t) path;
        cmd_lens[1] = path_len;
        cmd_args[2] = (uintptr_t) value;
        cmd_lens[2] = value_len;
        arg_count   = 3;
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonStrAppend,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonStrAppend, arg_count, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.DEBUG MEMORY / JSON.DEBUG FIELDS — subcmd key [path]
 * ==================================================================== */

static int execute_json_debug_command(zval*             object,
                                      int               argc,
                                      zval*             return_value,
                                      zend_class_entry* ce,
                                      const char*       subcmd,
                                      size_t            subcmd_len) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    size_t               key_len, path_len = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os|s", &object, ce, &key, &key_len, &path, &path_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    unsigned long arg_count = (path && path_len > 0) ? 3 : 2;
    uintptr_t     cmd_args[3];
    unsigned long cmd_lens[3];

    cmd_args[0] = (uintptr_t) subcmd;
    cmd_lens[0] = subcmd_len;
    cmd_args[1] = (uintptr_t) key;
    cmd_lens[1] = key_len;
    if (path && path_len > 0) {
        cmd_args[2] = (uintptr_t) path;
        cmd_lens[2] = path_len;
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonDebug,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonDebug, arg_count, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

int execute_json_debug_memory_command(zval*             object,
                                      int               argc,
                                      zval*             return_value,
                                      zend_class_entry* ce) {
    return execute_json_debug_command(object, argc, return_value, ce, "MEMORY", 6);
}

int execute_json_debug_fields_command(zval*             object,
                                      int               argc,
                                      zval*             return_value,
                                      zend_class_entry* ce) {
    return execute_json_debug_command(object, argc, return_value, ce, "FIELDS", 6);
}

/* ====================================================================
 * JSON.ARRAPPEND — key path value [value ...]
 * ==================================================================== */

int execute_json_arrappend_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    size_t               key_len, path_len;
    zval*                values     = NULL;
    int                  values_cnt = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss+",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &path,
                                     &path_len,
                                     &values,
                                     &values_cnt) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    unsigned long  arg_count = 2 + values_cnt;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* cmd_lens  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) key;
    cmd_lens[0] = key_len;
    cmd_args[1] = (uintptr_t) path;
    cmd_lens[1] = path_len;
    for (int i = 0; i < values_cnt; i++) {
        if (Z_TYPE(values[i]) != IS_STRING) {
            convert_to_string(&values[i]);
        }
        cmd_args[2 + i] = (uintptr_t) Z_STRVAL(values[i]);
        cmd_lens[2 + i] = Z_STRLEN(values[i]);
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonArrAppend,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        efree(cmd_args);
        efree(cmd_lens);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonArrAppend, arg_count, cmd_args, cmd_lens);
    efree(cmd_args);
    efree(cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.ARRINSERT — key path index value [value ...]
 * ==================================================================== */

int execute_json_arrinsert_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    zend_long            index;
    size_t               key_len, path_len;
    zval*                values     = NULL;
    int                  values_cnt = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Ossl+",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &path,
                                     &path_len,
                                     &index,
                                     &values,
                                     &values_cnt) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    char   idx_buf[24];
    size_t idx_len = snprintf(idx_buf, sizeof(idx_buf), "%ld", (long) index);

    unsigned long  arg_count = 3 + values_cnt;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* cmd_lens  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    cmd_args[0] = (uintptr_t) key;
    cmd_lens[0] = key_len;
    cmd_args[1] = (uintptr_t) path;
    cmd_lens[1] = path_len;
    cmd_args[2] = (uintptr_t) idx_buf;
    cmd_lens[2] = idx_len;
    for (int i = 0; i < values_cnt; i++) {
        if (Z_TYPE(values[i]) != IS_STRING) {
            convert_to_string(&values[i]);
        }
        cmd_args[3 + i] = (uintptr_t) Z_STRVAL(values[i]);
        cmd_lens[3 + i] = Z_STRLEN(values[i]);
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonArrInsert,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        efree(cmd_args);
        efree(cmd_lens);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonArrInsert, arg_count, cmd_args, cmd_lens);
    efree(cmd_args);
    efree(cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.ARRINDEX — key path scalar [start [end]]
 * ==================================================================== */

int execute_json_arrindex_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key    = NULL;
    char*                path   = NULL;
    char*                scalar = NULL;
    size_t               key_len, path_len, scalar_len;
    zend_long            start = 0;
    zend_long            end   = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osss|ll",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &path,
                                     &path_len,
                                     &scalar,
                                     &scalar_len,
                                     &start,
                                     &end) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Determine how many optional args were passed */
    /* argc counts user-visible args (key, path, scalar, [start], [end]) */

    char   start_buf[24], end_buf[24];
    size_t start_len = 0, end_len = 0;

    uintptr_t     cmd_args[5];
    unsigned long cmd_lens[5];
    unsigned long arg_count = 3;

    cmd_args[0] = (uintptr_t) key;
    cmd_lens[0] = key_len;
    cmd_args[1] = (uintptr_t) path;
    cmd_lens[1] = path_len;
    cmd_args[2] = (uintptr_t) scalar;
    cmd_lens[2] = scalar_len;

    /* argc is ZEND_NUM_ARGS() passed from the macro */
    if (argc > 3) {
        start_len   = snprintf(start_buf, sizeof(start_buf), "%ld", (long) start);
        cmd_args[3] = (uintptr_t) start_buf;
        cmd_lens[3] = start_len;
        arg_count   = 4;
    }
    if (argc > 4) {
        end_len     = snprintf(end_buf, sizeof(end_buf), "%ld", (long) end);
        cmd_args[4] = (uintptr_t) end_buf;
        cmd_lens[4] = end_len;
        arg_count   = 5;
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonArrIndex,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonArrIndex, arg_count, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.ARRPOP — key [path [index]]
 * ==================================================================== */

int execute_json_arrpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    size_t               key_len, path_len = 0;
    zend_long            index = -1;

    if (zend_parse_method_parameters(
            argc, object, "Os|sl", &object, ce, &key, &key_len, &path, &path_len, &index) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    char   idx_buf[24];
    size_t idx_len = 0;

    uintptr_t     cmd_args[3];
    unsigned long cmd_lens[3];
    unsigned long arg_count = 1;

    cmd_args[0] = (uintptr_t) key;
    cmd_lens[0] = key_len;

    if (path && path_len > 0) {
        cmd_args[1] = (uintptr_t) path;
        cmd_lens[1] = path_len;
        arg_count   = 2;

        if (argc > 2) {
            idx_len     = snprintf(idx_buf, sizeof(idx_buf), "%ld", (long) index);
            cmd_args[2] = (uintptr_t) idx_buf;
            cmd_lens[2] = idx_len;
            arg_count   = 3;
        }
    }

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(valkey_glide,
                                           JsonArrPop,
                                           cmd_args,
                                           cmd_lens,
                                           arg_count,
                                           NULL,
                                           process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonArrPop, arg_count, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}

/* ====================================================================
 * JSON.ARRTRIM — key path start end
 * ==================================================================== */

int execute_json_arrtrim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key  = NULL;
    char*                path = NULL;
    size_t               key_len, path_len;
    zend_long            start, end;

    if (zend_parse_method_parameters(
            argc, object, "Ossll", &object, ce, &key, &key_len, &path, &path_len, &start, &end) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    char   start_buf[24], end_buf[24];
    size_t start_len = snprintf(start_buf, sizeof(start_buf), "%ld", (long) start);
    size_t end_len   = snprintf(end_buf, sizeof(end_buf), "%ld", (long) end);

    uintptr_t cmd_args[4] = {
        (uintptr_t) key, (uintptr_t) path, (uintptr_t) start_buf, (uintptr_t) end_buf};
    unsigned long cmd_lens[4] = {key_len, path_len, start_len, end_len};

    if (valkey_glide->is_in_batch_mode) {
        int res = buffer_command_for_batch(
            valkey_glide, JsonArrTrim, cmd_args, cmd_lens, 4, NULL, process_json_mixed_result);
        if (res) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    CommandResult* result =
        execute_command(valkey_glide->glide_client, JsonArrTrim, 4, cmd_args, cmd_lens);

    if (result) {
        if (result->command_error) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
            free_command_result(result);
            return 0;
        }
        if (result->response) {
            if (result->response->response_type == Null) {
                ZVAL_NULL(return_value);
                free_command_result(result);
                return 1;
            }
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
                free_command_result(result);
                return 1;
            }
        }
        free_command_result(result);
    }
    return 0;
}
