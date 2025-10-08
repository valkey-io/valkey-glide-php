/*
  +----------------------------------------------------------------------+
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

#include "valkey_glide_core_common.h"

/* Execute an EXPIRE command using the Valkey Glide client */
int execute_expire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *mode = NULL;
    size_t               key_len, mode_len = 0;
    zend_long            seconds;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osl|s", &object, ce, &key, &key_len, &seconds, &mode, &mode_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* If mode is specified but not one of the valid options, return FALSE */
        if (mode_len > 0) {
            if (strncasecmp(mode, "NX", mode_len) != 0 && strncasecmp(mode, "XX", mode_len) != 0 &&
                strncasecmp(mode, "GT", mode_len) != 0 && strncasecmp(mode, "LT", mode_len) != 0) {
                return 0;
            }
        }

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Expire;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add time argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = seconds;
        args.arg_count                   = 1;

        /* Add mode argument if provided */
        if (mode && mode_len > 0) {
            args.args[1].type                  = CORE_ARG_TYPE_STRING;
            args.args[1].data.string_arg.value = mode;
            args.args[1].data.string_arg.len   = mode_len;
            args.arg_count                     = 2;
        }

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }
            return 1;
        }
    }

    return 0;
}

/* Execute an EXPIREAT command using the Valkey Glide client */
int execute_expireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *mode = NULL;
    size_t               key_len, mode_len = 0;
    zend_long            timestamp;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osl|s", &object, ce, &key, &key_len, &timestamp, &mode, &mode_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* If mode is specified but not one of the valid options, return FALSE */
        if (mode_len > 0) {
            if (strncasecmp(mode, "NX", mode_len) != 0 && strncasecmp(mode, "XX", mode_len) != 0 &&
                strncasecmp(mode, "GT", mode_len) != 0 && strncasecmp(mode, "LT", mode_len) != 0) {
                return 0;
            }
        }

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = ExpireAt;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add timestamp argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = timestamp;
        args.arg_count                   = 1;

        /* Add mode argument if provided */
        if (mode && mode_len > 0) {
            args.args[1].type                  = CORE_ARG_TYPE_STRING;
            args.args[1].data.string_arg.value = mode;
            args.args[1].data.string_arg.len   = mode_len;
            args.arg_count                     = 2;
        }

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}

/* Execute a PEXPIRE command using the Valkey Glide client */
int execute_pexpire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *mode = NULL;
    size_t               key_len, mode_len = 0;
    zend_long            milliseconds;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osl|s", &object, ce, &key, &key_len, &milliseconds, &mode, &mode_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* If mode is specified but not one of the valid options, return FALSE */
        if (mode_len > 0) {
            if (strncasecmp(mode, "NX", mode_len) != 0 && strncasecmp(mode, "XX", mode_len) != 0 &&
                strncasecmp(mode, "GT", mode_len) != 0 && strncasecmp(mode, "LT", mode_len) != 0) {
                return 0;
            }
        }

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = PExpire;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add milliseconds argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = milliseconds;
        args.arg_count                   = 1;

        /* Add mode argument if provided */
        if (mode && mode_len > 0) {
            args.args[1].type                  = CORE_ARG_TYPE_STRING;
            args.args[1].data.string_arg.value = mode;
            args.args[1].data.string_arg.len   = mode_len;
            args.arg_count                     = 2;
        }

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}

/* Execute a PEXPIREAT command using the Valkey Glide client */
int execute_pexpireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *mode = NULL;
    size_t               key_len, mode_len = 0;
    zend_long            timestamp_ms;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osl|s", &object, ce, &key, &key_len, &timestamp_ms, &mode, &mode_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* If mode is specified but not one of the valid options, return FALSE */
        if (mode_len > 0) {
            if (strncasecmp(mode, "NX", mode_len) != 0 && strncasecmp(mode, "XX", mode_len) != 0 &&
                strncasecmp(mode, "GT", mode_len) != 0 && strncasecmp(mode, "LT", mode_len) != 0) {
                return 0;
            }
        }

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = PExpireAt;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add timestamp in milliseconds argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = timestamp_ms;
        args.arg_count                   = 1;

        /* Add mode argument if provided */
        if (mode && mode_len > 0) {
            args.args[1].type                  = CORE_ARG_TYPE_STRING;
            args.args[1].data.string_arg.value = mode;
            args.args[1].data.string_arg.len   = mode_len;
            args.arg_count                     = 2;
        }

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}

/* Execute a PERSIST command using the Valkey Glide client */
int execute_persist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* PERSIST doesn't take any additional arguments besides the key */
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Persist;
        args.key                 = key;
        args.key_len             = key_len;
        args.arg_count           = 0; /* No additional arguments for PERSIST */

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}

/* Execute an EXPIRETIME command using the Valkey Glide client */
int execute_expiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* EXPIRETIME returns an integer (timestamp) and takes no additional arguments */
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = ExpireTime;
        args.key                 = key;
        args.key_len             = key_len;
        args.arg_count           = 0; /* No additional arguments for EXPIRETIME */


        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}

/* Execute a PEXPIRETIME command using the Valkey Glide client */
int execute_pexpiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* PEXPIRETIME returns an integer (timestamp in milliseconds) and takes no additional
         * arguments */
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = PExpireTime;
        args.key                 = key;
        args.key_len             = key_len;
        args.arg_count           = 0; /* No additional arguments for PEXPIRETIME */

        long output_value;
        if (execute_core_command(
                valkey_glide, &args, &output_value, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    return 0;
}
