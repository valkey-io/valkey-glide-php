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
#include "logger.h"
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_z_common.h"
#include "zend_exceptions.h"

/**
 * Parse cluster route from method parameters.
 * Parses variadic args, validates that at least one arg (the route) is present,
 * and populates core_args routing fields.
 *
 * Returns 1 on success, 0 on failure.
 * On success, *args and *args_count are set for further argument processing.
 */
static int parse_cluster_route(int                  argc,
                               zval**               object,
                               zend_class_entry*    ce,
                               zval**               args,
                               int*                 args_count,
                               core_command_args_t* core_args) {
    if (zend_parse_method_parameters(argc, *object, "O*", object, ce, args, args_count) ==
        FAILURE) {
        return 0;
    }
    if (*args_count == 0) {
        return 0;
    }
    core_args->has_route   = 1;
    core_args->route_param = &(*args)[0];
    return 1;
}

/**
 * Execute a core command and handle batch mode.
 * If in batch mode, copies the object to return_value for method chaining.
 *
 * Returns 1 on success, 0 on failure.
 */
static int execute_and_handle_batch(valkey_glide_object* valkey_glide,
                                    core_command_args_t* core_args,
                                    z_result_processor_t processor,
                                    zval*                return_value,
                                    zval*                object) {
    if (!execute_core_command(valkey_glide, core_args, NULL, processor, return_value)) {
        return 0;
    }
    if (valkey_glide->is_in_batch_mode) {
        ZVAL_COPY(return_value, object);
    }
    return 1;
}

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
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* FLUSHDB cannot be used in batch mode */
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = FlushDB;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (!parse_cluster_route(argc, &object, ce, &args, &args_count, &core_args)) {
            return 0;
        }

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

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_core_bool_result, return_value, object);
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
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* FLUSHALL cannot be used in batch mode */
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = FlushAll;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (!parse_cluster_route(argc, &object, ce, &args, &args_count, &core_args)) {
            return 0;
        }

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

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_core_bool_result, return_value, object);
}

/* Execute a BGSAVE command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bgsave_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());
    char*                mode       = NULL;
    size_t               mode_len   = 0;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = BgSave;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (!parse_cluster_route(argc, &object, ce, &args, &args_count, &core_args)) {
            return 0;
        }

        /* Get optional mode parameter */
        if (args_count > 1 && Z_TYPE(args[1]) == IS_STRING) {
            mode     = Z_STRVAL(args[1]);
            mode_len = Z_STRLEN(args[1]);
        }
    } else {
        /* Non-cluster case - parse optional mode parameter */
        if (zend_parse_method_parameters(argc, object, "O|s!", &object, ce, &mode, &mode_len) ==
            FAILURE) {
            return 0;
        }
    }

    /* Add mode option if provided (SCHEDULE or CANCEL) */
    if (mode && mode_len > 0) {
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = mode;
        core_args.args[0].data.string_arg.len   = mode_len;
        core_args.arg_count                     = 1;
    }

    /* Select processor based on OPT_REPLY_LITERAL:
     * - With OPT_REPLY_LITERAL: return raw string (process_core_status_string_result)
     * - Without OPT_REPLY_LITERAL: return bool (process_core_status_bool_result)
     * This ensures correct types in both normal and batch/pipeline mode. */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a BGREWRITEAOF command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bgrewriteaof_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
    core_args.cmd_type            = BgRewriteAof;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (!parse_cluster_route(argc, &object, ce, &args, &args_count, &core_args)) {
            return 0;
        }
    } else {
        /* Non-cluster case - no parameters */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    /* Select processor based on OPT_REPLY_LITERAL:
     * - With OPT_REPLY_LITERAL: return raw string (process_core_status_string_result)
     * - Without OPT_REPLY_LITERAL: return bool (process_core_status_bool_result)
     * This matches PHPRedis behavior where bgrewriteaof() returns bool. */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a MIGRATE command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_migrate_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                host     = NULL;
    size_t               host_len = 0;
    long                 port;
    zval*                z_key = NULL;
    long                 dstdb;
    long                 timeout;
    zend_bool            copy          = 0;
    zend_bool            replace       = 0;
    zval*                z_credentials = NULL;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Parse parameters: host, port, key (string|array), dstdb, timeout, copy, replace, credentials
     */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oslzll|bbz!",
                                     &object,
                                     ce,
                                     &host,
                                     &host_len,
                                     &port,
                                     &z_key,
                                     &dstdb,
                                     &timeout,
                                     &copy,
                                     &replace,
                                     &z_credentials) == FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Migrate;
    core_args.is_cluster          = (ce == get_valkey_glide_cluster_ce());

    int arg_idx = 0;

    /* arg[0]: host */
    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
    core_args.args[arg_idx].data.string_arg.value = host;
    core_args.args[arg_idx].data.string_arg.len   = host_len;
    arg_idx++;

    /* arg[1]: port */
    core_args.args[arg_idx].type                = CORE_ARG_TYPE_LONG;
    core_args.args[arg_idx].data.long_arg.value = port;
    arg_idx++;

    /* arg[2]: key or "" for multi-key */
    if (Z_TYPE_P(z_key) == IS_STRING) {
        /* Single key */
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_key);
        core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_key);
    } else if (Z_TYPE_P(z_key) == IS_ARRAY) {
        /* Multi-key: validate non-empty */
        if (zend_hash_num_elements(Z_ARRVAL_P(z_key)) == 0) {
            return 0;
        }
        /* Pass empty string as key placeholder, keys will be appended via KEYS keyword */
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = "";
        core_args.args[arg_idx].data.string_arg.len   = 0;
    } else {
        return 0;
    }
    arg_idx++;

    /* arg[3]: destination db */
    core_args.args[arg_idx].type                = CORE_ARG_TYPE_LONG;
    core_args.args[arg_idx].data.long_arg.value = dstdb;
    arg_idx++;

    /* arg[4]: timeout */
    core_args.args[arg_idx].type                = CORE_ARG_TYPE_LONG;
    core_args.args[arg_idx].data.long_arg.value = timeout;
    arg_idx++;

    /* arg[5]: COPY (optional) */
    if (copy) {
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = "COPY";
        core_args.args[arg_idx].data.string_arg.len   = 4;
        arg_idx++;
    }

    /* arg[6]: REPLACE (optional) */
    if (replace) {
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = "REPLACE";
        core_args.args[arg_idx].data.string_arg.len   = 7;
        arg_idx++;
    }

    /* Handle credentials: AUTH password or AUTH2 username password */
    if (z_credentials && Z_TYPE_P(z_credentials) != IS_NULL) {
        if (Z_TYPE_P(z_credentials) == IS_STRING) {
            /* Simple password: AUTH password */
            if (arg_idx < 11) {
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = "AUTH";
                core_args.args[arg_idx].data.string_arg.len   = 4;
                arg_idx++;
            }
            if (arg_idx < 12) {
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_credentials);
                core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_credentials);
                arg_idx++;
            }
        } else if (Z_TYPE_P(z_credentials) == IS_ARRAY) {
            /* Array: [username, password] for AUTH2 */
            HashTable* ht           = Z_ARRVAL_P(z_credentials);
            zval*      z_val        = NULL;
            int        idx          = 0;
            char*      username     = NULL;
            size_t     username_len = 0;
            char*      password     = NULL;
            size_t     password_len = 0;

            ZEND_HASH_FOREACH_VAL(ht, z_val) {
                if (Z_TYPE_P(z_val) == IS_STRING) {
                    if (idx == 0) {
                        username     = Z_STRVAL_P(z_val);
                        username_len = Z_STRLEN_P(z_val);
                    } else if (idx == 1) {
                        password     = Z_STRVAL_P(z_val);
                        password_len = Z_STRLEN_P(z_val);
                    }
                }
                idx++;
            }
            ZEND_HASH_FOREACH_END();

            if (password && arg_idx + 2 < 12) {
                if (username) {
                    /* AUTH2 username password */
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = "AUTH2";
                    core_args.args[arg_idx].data.string_arg.len   = 5;
                    arg_idx++;
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = username;
                    core_args.args[arg_idx].data.string_arg.len   = username_len;
                    arg_idx++;
                    if (arg_idx < 12) {
                        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                        core_args.args[arg_idx].data.string_arg.value = password;
                        core_args.args[arg_idx].data.string_arg.len   = password_len;
                        arg_idx++;
                    }
                } else {
                    /* AUTH password (array with single element) */
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = "AUTH";
                    core_args.args[arg_idx].data.string_arg.len   = 4;
                    arg_idx++;
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = password;
                    core_args.args[arg_idx].data.string_arg.len   = password_len;
                    arg_idx++;
                }
            }
        }
    }

    /* For multi-key: append KEYS keyword and individual keys */
    if (Z_TYPE_P(z_key) == IS_ARRAY && arg_idx < 12) {
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = "KEYS";
        core_args.args[arg_idx].data.string_arg.len   = 4;
        arg_idx++;

        /* Add individual keys from the array */
        zval* z_val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_key), z_val) {
            if (arg_idx >= 12)
                break;
            if (Z_TYPE_P(z_val) == IS_STRING) {
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_val);
                core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_val);
                arg_idx++;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    core_args.arg_count = arg_idx;

    /* Select processor based on OPT_REPLY_LITERAL:
     * - With OPT_REPLY_LITERAL: return raw string ("OK" or "NOKEY")
     * - Without OPT_REPLY_LITERAL: return bool true on success */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
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
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* TIME cannot be used in batch mode */
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Time;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (!parse_cluster_route(argc, &object, ce, &args, &args_count, &core_args)) {
            return 0;
        }
    } else {
        /* Non-cluster case - parse no parameters */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_core_array_result, return_value, object);
}

/* Execute a WATCH command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_watch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args    = NULL;
    int                  arg_count = 0;

    /* Parse parameters - handle both array and variadic string parameters */
    if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &arg_count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Need at least one key to watch */
    if (arg_count == 0) {
        return 0;
    }

    /* Handle different parameter patterns:
     * 1. watch(['key1', 'key2']) - first arg is array
     * 2. watch('key1', 'key2', 'key3') - multiple string args
     */

    if (arg_count == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        /* Pattern 1: Single array argument */
        int keys_count = zend_hash_num_elements(Z_ARRVAL(z_args[0]));
        if (execute_multi_key_command(
                valkey_glide, Watch, &z_args[0], keys_count, object, return_value)) {
            return 1;
        }
    } else {
        /* Pattern 2: Multiple string arguments or single string */
        if (execute_multi_key_command(
                valkey_glide, Watch, z_args, arg_count, object, return_value)) {
            return 1;
        }
    }

    return 0;
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

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = UnWatch;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
        return 1;
    } else {
        return 0;
    }
}

/* Shared utility function to process OBJECT command results */
static int process_object_command_result(CommandResponse* response,
                                         const char*      subcommand,
                                         size_t           subcommand_len,
                                         zval*            return_value) {
    if (!response || !subcommand || !return_value) {
        return -1;
    }

    /* Handle different result types based on the subcommand */
    if (strncasecmp(subcommand, "REFCOUNT", subcommand_len) == 0 ||
        strncasecmp(subcommand, "IDLETIME", subcommand_len) == 0 ||
        strncasecmp(subcommand, "FREQ", subcommand_len) == 0) {
        /* These subcommands return integers */
        if (response->response_type == Int) {
            /* Success, set return value */
            ZVAL_LONG(return_value, (long) response->int_value);
            return 1;
        } else if (response->response_type == Null) {
            /* Key doesn't exist */
            ZVAL_FALSE(return_value);
            return 0;
        }
    } else if (strncasecmp(subcommand, "ENCODING", subcommand_len) == 0) {
        /* ENCODING returns a string */
        if (response->response_type == String) {
            /* Success, set return value */
            ZVAL_STRINGL(return_value, response->string_value, response->string_value_len);
            return 1;
        } else if (response->response_type == Null) {
            /* Key doesn't exist */
            ZVAL_FALSE(return_value);
            return 0;
        }
    } else {
        /* Unsupported subcommand */
        return -1;
    }

    return -1;
}

/* Result processor callback for OBJECT command */
static int process_object_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }

    /* Get subcommand from output parameter */
    char* subcommand = (char*) output;
    if (!subcommand) {
        return 0;
    }

    /* Use the shared utility function */
    int result =
        process_object_command_result(response, subcommand, strlen(subcommand), return_value);

    /* Free the subcommand memory that was allocated in batch mode */
    efree(subcommand);

    /* Convert return values: shared function returns -1/0/1, callback expects 0/1 */
    return (result >= 0) ? 1 : 0;
}

/* Implementation of the OBJECT command with batching support */
int execute_object_command_impl(valkey_glide_object* valkey_glide,
                                const char*          subcommand,
                                size_t               subcommand_len,
                                const char*          key,
                                size_t               key_len,
                                zval*                object,
                                zval*                return_value) {
    if (!valkey_glide || !valkey_glide->glide_client) {
        return -1;
    }

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


    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* Create a copy of subcommand for the callback */
        char* subcommand_copy = estrndup(subcommand, subcommand_len);

        /* Buffer command for batch execution */
        int result = buffer_command_for_batch(
            valkey_glide, req_type, args, args_len, 1, subcommand_copy, process_object_result);

        if (result) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        } else {
            efree(subcommand_copy);
            return -1;
        }
    }

    /* Execute the command */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, req_type, 1, args, args_len);
    if (result == NULL) {
        return -1;
    }

    /* Use the shared utility function to process the result */
    int ret_val = -1; /* Default to error */
    if (result->response) {
        ret_val = process_object_command_result(
            result->response, subcommand, subcommand_len, return_value);
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
        if (execute_object_command_impl(
                valkey_glide, subcommand, subcommand_len, key, key_len, object, return_value) >=
            0) {
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
    zend_bool            replace          = 0;
    zval*                z_opts           = NULL;
    char*                db_str_allocated = NULL; /* Track allocated memory */
    core_command_args_t  args             = {0};
    int                  arg_count        = 1;

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

    /* Initialize args structure */
    args.glide_client = valkey_glide->glide_client;
    args.cmd_type     = Copy;
    args.key          = src; /* Source key */
    args.key_len      = src_len;

    /* Destination key */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = dst;
    args.args[0].data.string_arg.len   = dst_len;

    /* Check for options if options array was passed */
    if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(z_opts);
        zval*      replace_val;
        zval*      db_val;

        /* Check for REPLACE option (case-insensitive) */
        zend_string* key;
        zval*        val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
            if (key && ZSTR_LEN(key) == 7 && strcasecmp(ZSTR_VAL(key), "REPLACE") == 0) {
                if (Z_TYPE_P(val) == IS_TRUE) {
                    replace = 1;
                }
            } else if (key && ZSTR_LEN(key) == 2 && strcasecmp(ZSTR_VAL(key), "DB") == 0) {
                if (Z_TYPE_P(val) == IS_LONG) {
                    zend_long db_id = Z_LVAL_P(val);
                    if (db_id < 0) {
                        VALKEY_LOG_ERROR("command_validation", "Database ID must be non-negative");
                        zend_throw_exception(
                            get_valkey_glide_exception_ce(), "Database ID must be non-negative", 0);
                        return 0;
                    }

                    /* Add DB argument */
                    args.args[arg_count].type                  = CORE_ARG_TYPE_STRING;
                    args.args[arg_count].data.string_arg.value = "DB";
                    args.args[arg_count].data.string_arg.len   = 2;
                    arg_count++;

                    /* Add database ID */
                    size_t db_str_len;
                    if (db_str_allocated) {
                        efree(db_str_allocated);
                    }
                    db_str_allocated          = safe_format_long_long(db_id, &db_str_len);
                    args.args[arg_count].type = CORE_ARG_TYPE_STRING;
                    args.args[arg_count].data.string_arg.value = db_str_allocated;
                    args.args[arg_count].data.string_arg.len   = db_str_len;
                    arg_count++;
                }
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Optional REPLACE flag */
    if (replace) {
        args.args[arg_count].type                  = CORE_ARG_TYPE_STRING;
        args.args[arg_count].data.string_arg.value = "REPLACE";
        args.args[arg_count].data.string_arg.len   = 7;
        arg_count++;
    }

    args.arg_count = arg_count;

    /* Execute the COPY command using the Glide client */
    int result = 0;
    if (execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            result = 1;
        } else {
            result = 1;
        }
    }

    /* Cleanup allocated memory */
    if (db_str_allocated) {
        efree(db_str_allocated);
    }

    return result;
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

    return execute_and_handle_batch(
        valkey_glide, &args, process_core_int_result, return_value, object);
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

    if (Z_TYPE_P(z_args) == IS_ARRAY) {
        arg_count = zend_hash_num_elements(Z_ARRVAL_P(z_args));
    }

    if (execute_multi_key_command(valkey_glide, PfCount, z_args, arg_count, object, return_value)) {
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

    return execute_and_handle_batch(
        valkey_glide, &args, process_core_bool_result, return_value, object);
}

/* Execute a SELECT command using the Valkey Glide client */
int execute_select_command_internal(valkey_glide_object* valkey_glide,
                                    long                 dbindex,
                                    zval*                return_value) {
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Select;

    /* Add database index argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = dbindex;
    args.arg_count                   = 1;

    return execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value);
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

    /* SELECT cannot be used in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        VALKEY_LOG_ERROR("batch_validation", "SELECT command cannot be used in batch mode");
        return 0;
    }

    /* Execute the SELECT command using the Glide client */
    if (execute_select_command_internal(valkey_glide, dbindex, return_value)) {
        return 1;
    }

    return 0;
}

/* Execute a MOVE command using the Valkey Glide client */
int execute_move_command_internal(valkey_glide_object* valkey_glide,
                                  const char*          key,
                                  size_t               key_len,
                                  long                 db,
                                  zval*                return_value) {
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Move;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add db argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = db;
    args.arg_count                   = 1;


    return execute_core_command(valkey_glide, &args, NULL, process_core_bool_result, return_value);
}

/* Execute a MOVE command - UNIFIED IMPLEMENTATION */
int execute_move_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 dbindex;


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
    if (execute_move_command_internal(valkey_glide, key, key_len, dbindex, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}
