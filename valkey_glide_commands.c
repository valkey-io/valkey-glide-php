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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/* Execute an MSET command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_mset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_arr;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_arr) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = MSet;

        /* Set up array argument for key-value pairs */
        args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
        args.args[0].data.array_arg.array = z_arr;
        args.args[0].data.array_arg.count = zend_hash_num_elements(Z_ARRVAL_P(z_arr));
        args.arg_count                    = 1;

        if (execute_core_command(&args, NULL, process_core_bool_result)) {
            ZVAL_TRUE(return_value);
            return 1;
        }
    }

    return 0;
}

/* Execute an MSETNX command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_msetnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_arr;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_arr) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = MSetNX;

        /* Set up array argument for key-value pairs */
        args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
        args.args[0].data.array_arg.array = z_arr;
        args.args[0].data.array_arg.count = zend_hash_num_elements(Z_ARRVAL_P(z_arr));
        args.arg_count                    = 1;

        if (execute_core_command(&args, NULL, process_core_bool_result)) {
            ZVAL_TRUE(return_value);
            return 1;
        }
    }

    return 0;
}

/* Execute a FLUSHDB command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_flushdb_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            async      = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = FlushDB;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - first parameter is route, optional second is async */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];

        /* Get optional async parameter */
        if (args_count > 1) {
            if (Z_TYPE(args[1]) == IS_TRUE) {
                async = 1;
            }
        }
    } else {
        /* Non-cluster case - parse optional async parameter only */
        if (zend_parse_method_parameters(argc, object, "O|b", &object, ce, &async) == FAILURE) {
            return 0;
        }
    }

    /* Add ASYNC option if requested */
    if (async) {
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = "ASYNC";
        core_args.args[0].data.string_arg.len   = 5;
        core_args.arg_count                     = 1;
    }

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Execute a FLUSHALL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_flushall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            async      = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = FlushAll;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - first parameter is route, optional second is async */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];

        /* Get optional async parameter */
        if (args_count > 1) {
            if (Z_TYPE(args[1]) == IS_TRUE) {
                async = 1;
            }
        }
    } else {
        /* Non-cluster case - parse optional async parameter only */
        if (zend_parse_method_parameters(argc, object, "O|b", &object, ce, &async) == FAILURE) {
            return 0;
        }
    }

    /* Add ASYNC option if requested */
    if (async) {
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = "ASYNC";
        core_args.args[0].data.string_arg.len   = 5;
        core_args.arg_count                     = 1;
    }

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Execute a TIME command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_time_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Time;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - route parameter is required */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need the route parameter */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];
    } else {
        /* Non-cluster case - parse no parameters */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, return_value, process_core_array_result)) {
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Execute a WATCH command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_watch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args;
    int                  arg_count;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &z_args, &arg_count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Watch;

    /* Set up array argument for keys */
    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = z_args;
    args.args[0].data.array_arg.count = arg_count;
    args.arg_count                    = 1;

    if (execute_core_command(&args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Execute an UNWATCH command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_unwatch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = UnWatch;

    if (execute_core_command(&args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Implementation of the OBJECT command with the original signature */
int execute_object_command_impl(const void* glide_client,
                                const char* subcommand,
                                size_t      subcommand_len,
                                const char* key,
                                size_t      key_len,
                                zval*       return_value) {
    CommandResult* result  = NULL;
    int            ret_val = -1; /* Default to error */

    /* Create command array: ["OBJECT", subcommand, key] */
    uintptr_t     args[1];
    unsigned long args_len[1];

    args[0]     = (uintptr_t) key;
    args_len[0] = key_len;

    /* Select appropriate request type based on subcommand */
    enum RequestType req_type = CustomCommand; /* Default to CustomCommand */

    if (strncasecmp(subcommand, "REFCOUNT", subcommand_len) == 0) {
        req_type = ObjectRefCount;
    } else if (strncasecmp(subcommand, "IDLETIME", subcommand_len) == 0) {
        req_type = ObjectIdleTime;
    } else if (strncasecmp(subcommand, "FREQ", subcommand_len) == 0) {
        req_type = ObjectFreq;
    } else if (strncasecmp(subcommand, "ENCODING", subcommand_len) == 0) {
        req_type = ObjectEncoding;
    }
    /* For HELP and other subcommands, use CustomCommand (default) */

    /* Execute the command */
    result = execute_command(glide_client, req_type, 1, args, args_len);
    if (result == NULL) {
        return -1;
    }

    /* Handle different result types based on the subcommand */
    if (strncasecmp(subcommand, "REFCOUNT", subcommand_len) == 0 ||
        strncasecmp(subcommand, "IDLETIME", subcommand_len) == 0 ||
        strncasecmp(subcommand, "FREQ", subcommand_len) == 0) {
        /* These subcommands return integers */
        if (result->response && result->response->response_type == Int) {
            /* Success, set return value */
            ZVAL_LONG(return_value, (long) result->response->int_value);
            ret_val = 1;
        } else if (result->response && result->response->response_type == Null) {
            /* Key doesn't exist */
            ZVAL_FALSE(return_value);
            ret_val = 0;
        }
    } else if (strncasecmp(subcommand, "ENCODING", subcommand_len) == 0) {
        /* ENCODING returns a string */
        if (result->response && result->response->response_type == String) {
            /* Success, set return value */
            ZVAL_STRINGL(
                return_value, result->response->string_value, result->response->string_value_len);
            ret_val = 1;
        } else if (result->response && result->response->response_type == Null) {
            /* Key doesn't exist */
            ZVAL_FALSE(return_value);
            ret_val = 0;
        }
    } else if (strncasecmp(subcommand, "HELP", subcommand_len) == 0) {
        /* HELP returns an array of strings */
        if (result->response && result->response->response_type == Array) {
            if (command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false) == 1) {
                ret_val = 1;
            } else {
                ret_val = -1;
            }
        } else {
            ret_val = -1;
        }
    } else {
        /* Unsupported subcommand */
        ret_val = -1;
    }

    /* Clean up */
    free_command_result(result);

    return ret_val;
}

/* New execute_object_command function with standardized signature that follows the pattern */
int execute_object_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *subcommand = NULL;
    size_t               key_len, subcommand_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &subcommand, &subcommand_len, &key, &key_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Execute the OBJECT command using the Glide client via the implementation function */
        if (execute_object_command_impl(valkey_glide->glide_client,
                                        subcommand,
                                        subcommand_len,
                                        key,
                                        key_len,
                                        return_value) >= 0) {
            return 1;
        }
    }

    return 0;
}

/* Unified COPY command implementation */
int execute_copy_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               src = NULL, *dst = NULL;
    size_t               src_len, dst_len;
    zend_bool            replace = 0;
    zval*                z_opts  = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss|a", &object, ce, &src, &src_len, &dst, &dst_len, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check for the REPLACE option if options array was passed */
    if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(z_opts);
        zval*      replace_val;
        replace_val = zend_hash_str_find(ht, "replace", sizeof("replace") - 1);
        if (replace_val && Z_TYPE_P(replace_val) == IS_TRUE) {
            replace = 1;
        }
    }

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Copy;
    args.key                 = src; /* Source key */
    args.key_len             = src_len;

    /* Destination key */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = dst;
    args.args[0].data.string_arg.len   = dst_len;

    int arg_count = 1;

    /* Optional REPLACE flag */
    if (replace) {
        args.args[1].type                  = CORE_ARG_TYPE_STRING;
        args.args[1].data.string_arg.value = "REPLACE";
        args.args[1].data.string_arg.len   = 7;
        arg_count                          = 2;
    }

    args.arg_count = arg_count;

    /* Execute the COPY command using the Glide client */
    if (execute_core_command(&args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
}

/* Unified PFADD command implementation */
int execute_pfadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_elements;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osa", &object, ce, &key, &key_len, &z_elements) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the PFADD command using the Glide client */
    int result_value   = 0;
    int elements_count = zend_hash_num_elements(Z_ARRVAL_P(z_elements));

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = PfAdd;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add elements array argument */
    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = z_elements;
    args.args[0].data.array_arg.count = elements_count;
    args.arg_count                    = 1;

    long result;
    if (execute_core_command(&args, &result, process_core_bool_result)) {
        ZVAL_LONG(return_value, 1);
        return 1;
    } else {
        ZVAL_LONG(return_value, 0);
        return 0;
    }

    return 0;
}

/* Unified PFCOUNT command implementation */
int execute_pfcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args       = NULL;
    int                  arg_count    = 0;
    long                 result_value = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &arg_count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the PFCOUNT command using the Glide client */

    if (execute_multi_key_command(
            valkey_glide->glide_client, PfCount, z_args, arg_count, &result_value)) {
        ZVAL_LONG(return_value, result_value);
        return 1;
    }

    return 0;
}

/* Unified PFMERGE command implementation */
int execute_pfmerge_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                dst = NULL;
    size_t               dst_len;
    zval*                z_keys;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osa", &object, ce, &dst, &dst_len, &z_keys) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the PFMERGE command using the Glide client */
    int keys_count = zend_hash_num_elements(Z_ARRVAL_P(z_keys));

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = PfMerge;
    args.key                 = dst; /* Destination key */
    args.key_len             = dst_len;

    /* Add source keys array */
    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = z_keys;
    args.args[0].data.array_arg.count = keys_count;
    args.arg_count                    = 1;

    if (execute_core_command(&args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    }

    ZVAL_FALSE(return_value);
    return 0;
}

/* Execute a SELECT command using the Valkey Glide client */
int execute_select_command_internal(const void* glide_client, long dbindex) {
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Select;

    /* Add database index argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = dbindex;
    args.arg_count                   = 1;

    return execute_core_command(&args, NULL, process_core_bool_result);
}

/* Execute a SELECT command - UNIFIED IMPLEMENTATION */
int execute_select_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 dbindex;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Ol", &object, ce, &dbindex) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the SELECT command using the Glide client */
    if (execute_select_command_internal(valkey_glide->glide_client, dbindex)) {
        ZVAL_TRUE(return_value);
        return 1;
    }

    return 0;
}

/* Execute a MOVE command using the Valkey Glide client */
int execute_move_command_internal(
    const void* glide_client, const char* key, size_t key_len, long db, int* output_value) {
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Move;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add db argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = db;
    args.arg_count                   = 1;

    return execute_core_command(&args, output_value, process_core_bool_result);
}

/* Execute a MOVE command - UNIFIED IMPLEMENTATION */
int execute_move_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 dbindex;
    int                  result_value = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osl", &object, ce, &key, &key_len, &dbindex) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the MOVE command using the Glide client */
    if (execute_move_command_internal(
            valkey_glide->glide_client, key, key_len, dbindex, &result_value)) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 0;
}
