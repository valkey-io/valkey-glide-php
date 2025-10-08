/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include <ext/hash/php_hash.h>
#include <ext/spl/spl_exceptions.h>
#include <ext/standard/info.h>

#include "command_response.h" /* Include command_response.h for string conversion functions */
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

#if PHP_VERSION_ID < 80400
#include <ext/standard/php_random.h>
#else
#include <ext/random/php_random.h>
#endif

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* Execute a RENAME command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_rename_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               src = NULL, *dst = NULL;
    size_t               src_len, dst_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &src, &src_len, &dst, &dst_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the RENAME command using the Glide client */

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Rename;
    args.key                 = src;
    args.key_len             = src_len;

    /* Add destination key as argument */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = dst;
    args.args[0].data.string_arg.len   = dst_len;
    args.arg_count                     = 1;

    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value);

    /* Return TRUE if successful, FALSE otherwise */
    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    } else {
        return 0;
    }
}

/* Execute a RENAMENX command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_renamenx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               src = NULL, *dst = NULL;
    size_t               src_len, dst_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &src, &src_len, &dst, &dst_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the RENAMENX command using the Glide client */

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = RenameNX;
    args.key                 = src;
    args.key_len             = src_len;

    /* Add destination key as argument */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = dst;
    args.args[0].data.string_arg.len   = dst_len;
    args.arg_count                     = 1;

    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* Return TRUE if successful, FALSE otherwise */
    if (result == 1) {
        return 1;
    } else {
        return 0;
    }
}
/* Execute a GETDEL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_getdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the GETDEL command using the Glide client */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = GetDel;
    args.key                 = key;
    args.key_len             = key_len;


    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_string_result, return_value);

    /* Process the result */
    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            /* Note: output will be freed later in process_core_string_result */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    } else if (result == 0) {
        /* Key didn't exist */
        return 1;
    } else {
        /* Error */
        return 0;
    }
}

/* Execute a GETEX command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_getex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                opts = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|a", &object, ce, &key, &key_len, &opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the GETEX command using the Glide client */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = GetEx;
    args.key                 = key;
    args.key_len             = key_len;
    args.raw_options         = opts;

    /* Parse options using existing core framework option parsing */
    if (opts) {
        parse_core_options(opts, &args.options);
    }


    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_string_result, return_value);

    /* Process the result */
    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            /* Note: output will be freed later in process_core_string_result */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    } else if (result == 0) {
        /* Key didn't exist */
        return 1;
    } else {
        /* Error */
        return 0;
    }
}

/* Execute an INCR command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_incr_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            value = 1;

    /* Parse parameters */
    if (argc == 1) {
        /* Only key parameter provided */
        if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) ==
            FAILURE) {
            return 0;
        }
    } else {
        /* Both key and value parameters provided */
        if (zend_parse_method_parameters(
                argc, object, "Osl", &object, ce, &key, &key_len, &value) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute command based on argument count */
    if (argc == 1) {
        /* Execute an INCR command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Incr;
        args.key                 = key;
        args.key_len             = key_len;

        /* Standard INCR command if only key is provided */
        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }
        }
    } else {
        /* Use INCRBY if both key and value are provided */

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = IncrBy;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add increment argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = value;
        args.arg_count                   = 1;


        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }
        }
    }


    return 1;
}

/* Execute an INCRBY command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_incrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osl", &object, ce, &key, &key_len, &value) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }
    /* Execute an INCRBY command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = IncrBy;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add increment argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = value;
    args.arg_count                   = 1;


    /* Execute the INCRBY command using the Glide client */
    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
    }

    return 1;
}

/* Execute an INCRBYFLOAT command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_incrbyfloat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    double               value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osd", &object, ce, &key, &key_len, &value) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the INCRBYFLOAT command using the Glide client */

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = IncrByFloat;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add increment argument */
    args.args[0].type                  = CORE_ARG_TYPE_DOUBLE;
    args.args[0].data.double_arg.value = value;
    args.arg_count                     = 1;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_double_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    } else {
        return 0;
    }
}

/* Execute a DECR command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_decr_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            value = 1;

    /* Parse parameters */
    if (argc == 1) {
        /* Only key parameter provided - standard DECR */
        if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) ==
            FAILURE) {
            return 0;
        }
    } else {
        /* Both key and value parameters provided - like DECRBY */
        if (zend_parse_method_parameters(
                argc, object, "Osl", &object, ce, &key, &key_len, &value) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute command based on argument count */
    if (argc == 1) {
        /* Standard DECR command if only key is provided */
        /* Execute a DECR command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */

        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Decr;
        args.key                 = key;
        args.key_len             = key_len;

        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }
            return 1;
        }
    } else {
        /* Use DECRBY if both key and value are provided */
        /* Execute a DECRBY command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = DecrBy;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add decrement argument */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = value;
        args.arg_count                   = 1;
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

/* Execute a DECRBY command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_decrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            value;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osl", &object, ce, &key, &key_len, &value) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the DECRBY command using the Glide client */

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = DecrBy;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add decrement argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = value;
    args.arg_count                   = 1;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    } else {
        return 0;
    }
}

/* Execute an MGET command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_mget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_array;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oa/", &object, ce, &z_array) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the MGET command using the Glide client */
    array_init(return_value);

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = MGet;

    /* Set up array argument for keys */
    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = z_array;
    args.args[0].data.array_arg.count = zend_hash_num_elements(Z_ARRVAL_P(z_array));
    args.arg_count                    = 1;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_array_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        /* Command succeeded, return_value is already set */
        return 1;
    } else {
        /* Command failed */
        zval_dtor(return_value);
        return 0;
    }
}

/* Execute an EXISTS command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_exists_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &argc) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if we received an array as a single argument */
    if (argc == 1 && Z_TYPE_P(z_args) == IS_ARRAY) {
        /* Single array argument - pass directly to EXISTS command */
        int actual_key_count = zend_hash_num_elements(Z_ARRVAL_P(z_args));
        if (execute_multi_key_command(
                valkey_glide, Exists, z_args, actual_key_count, object, return_value)) {
            return 1;
        } else {
            return 0;
        }
    } else {
        /* Single string key or multiple arguments - create temporary array */
        zval temp_array;
        array_init(&temp_array);

        /* Add all arguments to the temporary array */
        for (int i = 0; i < argc; i++) {
            /* Create safe copy and convert to string if needed */
            zval copy;
            ZVAL_COPY(&copy, &z_args[i]);
            convert_to_string(&copy);
            add_next_index_zval(&temp_array, &copy);
        }

        /* Execute the EXISTS command with the temporary array */
        int actual_key_count = zend_hash_num_elements(Z_ARRVAL(temp_array));

        if (execute_multi_key_command(
                valkey_glide, Exists, &temp_array, actual_key_count, object, return_value)) {
            zval_dtor(&temp_array);
            return 1;
        } else {
            zval_dtor(&temp_array);
            return 0;
        }
    }
}

/* Execute a TOUCH command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_touch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &argc) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if we received an array as a single argument */
    if (argc == 1 && Z_TYPE_P(z_args) == IS_ARRAY) {
        /* Single array argument - pass directly to TOUCH command */
        /* Execute a TOUCH command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
        int actual_key_count = zend_hash_num_elements(Z_ARRVAL_P(z_args));
        if (execute_multi_key_command(
                valkey_glide, Touch, z_args, actual_key_count, object, return_value)) {
            return 1;
        } else {
            return 0;
        }
    } else {
        /* Single string key or multiple arguments - create temporary array */
        zval temp_array;
        array_init(&temp_array);

        /* Add all arguments to the temporary array */
        for (int i = 0; i < argc; i++) {
            /* Create safe copy and convert to string if needed */
            zval copy;
            ZVAL_COPY(&copy, &z_args[i]);
            convert_to_string(&copy);
            add_next_index_zval(&temp_array, &copy);
        }

        /* Execute the TOUCH command with the temporary array */
        int actual_key_count = zend_hash_num_elements(Z_ARRVAL(temp_array));

        if (execute_multi_key_command(
                valkey_glide, Touch, &temp_array, actual_key_count, object, return_value)) {
            zval_dtor(&temp_array);
            return 1;
        } else {
            zval_dtor(&temp_array);
            return 0;
        }
    }
}

/* Execute an UNLINK command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_unlink_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &argc) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if we have a single array argument */
    if (argc == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        /* Use array elements as keys */
        if (execute_unlink_array(
                valkey_glide->glide_client, Z_ARRVAL(z_args[0]), &result_value, return_value)) {
            return 1;
        }
    } else {
        if (execute_multi_key_command(valkey_glide, Unlink, z_args, argc, object, return_value)) {
            return 1;
        }
    }

    return 0;
}
