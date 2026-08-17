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

/* Execute a FLUSHDB command using the Valkey Glide client */
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

/* Execute a FLUSHALL command using the Valkey Glide client */
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

/* Execute a BGSAVE command using the Valkey Glide client */
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

/* Execute a BGREWRITEAOF command using the Valkey Glide client */
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

/* Execute a MIGRATE command using the Valkey Glide client */
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

    /* Parse parameters: host, port, key (string|array), dstdb, timeout, copy, replace,
     * credentials
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
            /* Simple password: AUTH password (need 2 slots) */
            if (arg_idx + 1 < 12) {
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = "AUTH";
                core_args.args[arg_idx].data.string_arg.len   = 4;
                arg_idx++;
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_credentials);
                core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_credentials);
                arg_idx++;
            }
        } else if (Z_TYPE_P(z_credentials) == IS_ARRAY) {
            /* Array: [password] for AUTH, or [username, password] for AUTH2 */
            HashTable* ht       = Z_ARRVAL_P(z_credentials);
            int        num_elem = zend_hash_num_elements(ht);
            zval*      z_elem0  = zend_hash_index_find(ht, 0);
            zval*      z_elem1  = zend_hash_index_find(ht, 1);

            if (num_elem == 1 && z_elem0 && Z_TYPE_P(z_elem0) == IS_STRING) {
                /* Single element: AUTH password */
                if (arg_idx + 2 <= 12) {
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = "AUTH";
                    core_args.args[arg_idx].data.string_arg.len   = 4;
                    arg_idx++;
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_elem0);
                    core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_elem0);
                    arg_idx++;
                }
            } else if (num_elem >= 2 && z_elem0 && Z_TYPE_P(z_elem0) == IS_STRING && z_elem1 &&
                       Z_TYPE_P(z_elem1) == IS_STRING) {
                /* Two elements: AUTH2 username password */
                if (arg_idx + 3 <= 12) {
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = "AUTH2";
                    core_args.args[arg_idx].data.string_arg.len   = 5;
                    arg_idx++;
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_elem0);
                    core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_elem0);
                    arg_idx++;
                    core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                    core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_elem1);
                    core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_elem1);
                    arg_idx++;
                }
            }
        }
    }

    /* For multi-key: allocate a single combined array with all args */
    if (Z_TYPE_P(z_key) == IS_ARRAY) {
        int num_keys    = zend_hash_num_elements(Z_ARRVAL_P(z_key));
        int total_count = arg_idx + 1 + num_keys; /* existing args + KEYS keyword + keys */

        core_arg_t* all = (core_arg_t*) ecalloc(total_count, sizeof(core_arg_t));

        /* Copy existing fixed args into the combined array */
        memcpy(all, core_args.args, arg_idx * sizeof(core_arg_t));

        /* Append KEYS keyword */
        all[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        all[arg_idx].data.string_arg.value = "KEYS";
        all[arg_idx].data.string_arg.len   = 4;
        arg_idx++;

        /* Append individual keys, converting non-string values to strings */
        zval* z_val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_key), z_val) {
            if (Z_TYPE_P(z_val) != IS_STRING) {
                convert_to_string(z_val);
            }
            all[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            all[arg_idx].data.string_arg.value = Z_STRVAL_P(z_val);
            all[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_val);
            arg_idx++;
        }
        ZEND_HASH_FOREACH_END();

        core_args.all_args = all;
    }

    core_args.arg_count = arg_idx;

    /* Select processor based on OPT_REPLY_LITERAL:
     * - With OPT_REPLY_LITERAL: return raw string ("OK" or "NOKEY")
     * - Without OPT_REPLY_LITERAL: return bool true on success */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    int result =
        execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);

    /* Free dynamic args if allocated */
    if (core_args.all_args) {
        efree(core_args.all_args);
    }

    return result;
}

/* Execute a SAVE command using the Valkey Glide client */
int execute_save_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
    core_args.cmd_type            = Save;
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
     * This matches PHPRedis behavior where save() returns bool. */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

static int process_memory_stats_result(CommandResponse* response,
                                       void*            output,
                                       zval*            return_value) {
    if (!response || !return_value) {
        return 0;
    }
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, true);
}

static int process_cluster_array_result(CommandResponse* response,
                                        void*            output,
                                        zval*            return_value) {
    if (!response || !return_value) {
        return 0;
    }
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, true);
}

int execute_latency_history_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                event      = NULL;
    size_t               event_len  = 0;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = LatencyHistory;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(
                argc, object, "Os*", &object, ce, &event, &event_len, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "Expected at most 1 additional argument (route)",
                                 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &event, &event_len) ==
            FAILURE) {
            return 0;
        }
    }

    core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
    core_args.args[0].data.string_arg.value = event;
    core_args.args[0].data.string_arg.len   = event_len;
    core_args.arg_count                     = 1;

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_cluster_array_result, return_value, object);
}

int execute_latency_latest_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = LatencyLatest;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), "Expected at most 1 argument (route)", 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_cluster_array_result, return_value, object);
}

static int latency_reset_build_and_execute(valkey_glide_object* valkey_glide,
                                           core_command_args_t* core_args,
                                           zval*                events,
                                           int                  event_count,
                                           zval*                return_value,
                                           zval*                object) {
    int arg_idx = 0;

    if (event_count <= CORE_ARGS_FIXED_MAX) {
        for (int i = 0; i < event_count; i++) {
            core_args->args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            core_args->args[arg_idx].data.string_arg.value = Z_STRVAL(events[i]);
            core_args->args[arg_idx].data.string_arg.len   = Z_STRLEN(events[i]);
            arg_idx++;
        }
    } else {
        core_arg_t* all = (core_arg_t*) ecalloc(event_count, sizeof(core_arg_t));
        for (int i = 0; i < event_count; i++) {
            all[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            all[arg_idx].data.string_arg.value = Z_STRVAL(events[i]);
            all[arg_idx].data.string_arg.len   = Z_STRLEN(events[i]);
            arg_idx++;
        }
        core_args->all_args = all;
    }
    core_args->arg_count = arg_idx;

    int result = execute_and_handle_batch(
        valkey_glide, core_args, process_core_int_result, return_value, object);

    if (core_args->all_args) {
        efree(core_args->all_args);
    }

    return result;
}

int execute_latency_reset_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
        FAILURE) {
        return 0;
    }

    /* Validate all variadic arguments are strings */
    for (int i = 0; i < args_count; i++) {
        if (Z_TYPE(args[i]) != IS_STRING) {
            zend_type_error(
                "ValkeyGlide::latencyReset(): Argument #%d must be of type string, %s given",
                i + 1,
                zend_zval_type_name(&args[i]));
            return 0;
        }
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = LatencyReset;
    core_args.is_cluster          = is_cluster;

    return latency_reset_build_and_execute(
        valkey_glide, &core_args, args, args_count, return_value, object);
}

int execute_latency_reset_with_route_command(zval*             object,
                                             int               argc,
                                             zval*             return_value,
                                             zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
        FAILURE) {
        return 0;
    }

    if (args_count < 1) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "latencyResetWithRoute() requires a route argument",
                             0);
        return 0;
    }

    /* Validate remaining args (events) are strings */
    for (int i = 1; i < args_count; i++) {
        if (Z_TYPE(args[i]) != IS_STRING) {
            zend_type_error(
                "ValkeyGlideCluster::latencyResetWithRoute(): Argument #%d must be of type "
                "string, %s given",
                i + 1,
                zend_zval_type_name(&args[i]));
            return 0;
        }
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = LatencyReset;
    core_args.is_cluster          = 1;
    core_args.has_route           = 1;
    core_args.route_param         = &args[0];

    return latency_reset_build_and_execute(
        valkey_glide, &core_args, &args[1], args_count - 1, return_value, object);
}

int execute_memory_doctor_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = MemoryDoctor;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "Expected at most 1 argument (route), got extra arguments",
                                 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_core_status_string_result, return_value, object);
}

int execute_memory_malloc_stats_command(zval*             object,
                                        int               argc,
                                        zval*             return_value,
                                        zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = MemoryMallocStats;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "Expected at most 1 argument (route), got extra arguments",
                                 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_core_status_string_result, return_value, object);
}

int execute_memory_purge_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = MemoryPurge;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "Expected at most 1 argument (route), got extra arguments",
                                 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

int execute_memory_stats_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = MemoryStats;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "Expected at most 1 argument (route), got extra arguments",
                                 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_memory_stats_result, return_value, object);
}

/* Execute a RESET command using the Valkey Glide client */
int execute_reset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* No parameters - validate object only */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Reset;
    core_args.is_cluster          = (ce == get_valkey_glide_cluster_ce());

    /* Select processor based on OPT_REPLY_LITERAL:
     * - With OPT_REPLY_LITERAL: return raw string "RESET"
     * - Without OPT_REPLY_LITERAL: return bool true
     * This matches PHPRedis behavior where reset() returns bool. */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a CLIENT PAUSE command using the Valkey Glide client */
int execute_client_pause_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 timeout;
    char*                mode     = NULL;
    size_t               mode_len = 0;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Parse parameters: timeout (required), mode (optional) */
    if (zend_parse_method_parameters(
            argc, object, "Ol|s!", &object, ce, &timeout, &mode, &mode_len) == FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = ClientPause;
    core_args.is_cluster          = (ce == get_valkey_glide_cluster_ce());

    /* Add timeout argument */
    core_args.args[0].type                = CORE_ARG_TYPE_LONG;
    core_args.args[0].data.long_arg.value = timeout;
    core_args.arg_count                   = 1;

    /* Add optional mode argument (ALL or WRITE) */
    if (mode && mode_len > 0) {
        core_args.args[1].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[1].data.string_arg.value = mode;
        core_args.args[1].data.string_arg.len   = mode_len;
        core_args.arg_count                     = 2;
    }

    /* Select processor based on OPT_REPLY_LITERAL */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a CLIENT UNPAUSE command using the Valkey Glide client */
int execute_client_unpause_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* No parameters - validate object only */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = ClientUnpause;
    core_args.is_cluster          = (ce == get_valkey_glide_cluster_ce());

    /* Select processor based on OPT_REPLY_LITERAL */
    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a failover command using the Valkey Glide client */
int execute_failover_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                to_arr  = NULL;
    zend_bool            abort   = 0;
    zend_long            timeout = 0;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Parse parameters: failover(?array $to = null, bool $abort = false, int $timeout = 0) */
    if (zend_parse_method_parameters(
            argc, object, "O|a!bl", &object, ce, &to_arr, &abort, &timeout) == FAILURE) {
        return 0;
    }

    /* Validate conflicting parameters: abort cannot be combined with $to or $timeout */
    if (timeout < 0) {
        VALKEY_LOG_ERROR("command_validation", "FAILOVER timeout must not be negative");
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "FAILOVER timeout must not be negative", 0);
        return 0;
    }
    if (abort && (to_arr || timeout > 0)) {
        VALKEY_LOG_ERROR("command_validation",
                         "FAILOVER ABORT cannot be combined with 'to' or 'timeout' parameters");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "FAILOVER ABORT cannot be combined with 'to' or 'timeout' parameters",
                             0);
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = FailOver;
    core_args.is_cluster          = 0;

    int arg_idx = 0;

    if (abort) {
        /* FAILOVER ABORT */
        core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[arg_idx].data.string_arg.value = "ABORT";
        core_args.args[arg_idx].data.string_arg.len   = 5;
        arg_idx++;
    } else {
        if (to_arr) {
            /* Extract host and port from array */
            zval* z_host = zend_hash_str_find(Z_ARRVAL_P(to_arr), "host", 4);
            zval* z_port = zend_hash_str_find(Z_ARRVAL_P(to_arr), "port", 4);

            if (!z_host || Z_TYPE_P(z_host) != IS_STRING || Z_STRLEN_P(z_host) == 0 || !z_port ||
                Z_TYPE_P(z_port) != IS_LONG) {
                VALKEY_LOG_ERROR(
                    "command_validation",
                    "'to' array must contain a non-empty string 'host' and an int 'port'");
                zend_throw_exception(
                    get_valkey_glide_exception_ce(),
                    "'to' array must contain a non-empty string 'host' and an int 'port'",
                    0);
                return 0;
            }

            core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            core_args.args[arg_idx].data.string_arg.value = "TO";
            core_args.args[arg_idx].data.string_arg.len   = 2;
            arg_idx++;

            core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            core_args.args[arg_idx].data.string_arg.value = Z_STRVAL_P(z_host);
            core_args.args[arg_idx].data.string_arg.len   = Z_STRLEN_P(z_host);
            arg_idx++;

            core_args.args[arg_idx].type                = CORE_ARG_TYPE_LONG;
            core_args.args[arg_idx].data.long_arg.value = zval_get_long(z_port);
            arg_idx++;

            /* Check for FORCE in array */
            zval* z_force = zend_hash_str_find(Z_ARRVAL_P(to_arr), "force", 5);
            if (z_force && zend_is_true(z_force)) {
                core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
                core_args.args[arg_idx].data.string_arg.value = "FORCE";
                core_args.args[arg_idx].data.string_arg.len   = 5;
                arg_idx++;
            }
        }

        if (timeout > 0) {
            core_args.args[arg_idx].type                  = CORE_ARG_TYPE_STRING;
            core_args.args[arg_idx].data.string_arg.value = "TIMEOUT";
            core_args.args[arg_idx].data.string_arg.len   = 7;
            arg_idx++;

            core_args.args[arg_idx].type                = CORE_ARG_TYPE_LONG;
            core_args.args[arg_idx].data.long_arg.value = timeout;
            arg_idx++;
        }
    }

    core_args.arg_count = arg_idx;

    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

int execute_replicaof_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                host     = NULL;
    size_t               host_len = 0;
    zend_long            port     = 6379;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Parse parameters: replicaof(?string $host = null, int $port = 6379) */
    if (zend_parse_method_parameters(argc, object, "O|s!l", &object, ce, &host, &host_len, &port) ==
        FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = ReplicaOf;
    core_args.is_cluster          = 0;

    if (host == NULL) {
        /* REPLICAOF NO ONE */
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = "NO";
        core_args.args[0].data.string_arg.len   = 2;
        core_args.args[1].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[1].data.string_arg.value = "ONE";
        core_args.args[1].data.string_arg.len   = 3;
        core_args.arg_count                     = 2;
    } else if (host_len == 0) {
        /* Reject empty string host */
        VALKEY_LOG_ERROR("command_validation", "REPLICAOF host must not be an empty string");
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "REPLICAOF host must not be an empty string", 0);
        return 0;
    } else {
        /* REPLICAOF host port */
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = host;
        core_args.args[0].data.string_arg.len   = host_len;
        core_args.args[1].type                  = CORE_ARG_TYPE_LONG;
        core_args.args[1].data.long_arg.value   = port;
        core_args.arg_count                     = 2;
    }

    z_result_processor_t processor = valkey_glide->opt_reply_literal
                                         ? process_core_status_string_result
                                         : process_core_status_bool_result;

    return execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);
}

/* Execute a TIME command using the Valkey Glide client */
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

/* Execute a LOLWUT command using the Valkey Glide client. */
int execute_lolwut_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                version    = NULL;
    zval*                parameters = NULL;
    zval*                route      = NULL;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Cluster batches cannot preserve a per-command route. */
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        return 0;
    }

    if (is_cluster) {
        if (zend_parse_method_parameters(
                argc, object, "O|z!z!z!", &object, ce, &version, &parameters, &route) == FAILURE) {
            return 0;
        }
    } else if (zend_parse_method_parameters(
                   argc, object, "O|z!z!", &object, ce, &version, &parameters) == FAILURE) {
        return 0;
    }

    if (version && Z_TYPE_P(version) != IS_NULL && Z_TYPE_P(version) != IS_LONG) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "LOLWUT version must be an integer", 0);
        return 0;
    }
    if (parameters && Z_TYPE_P(parameters) != IS_NULL && Z_TYPE_P(parameters) != IS_ARRAY) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "LOLWUT parameters must be an array of integers", 0);
        return 0;
    }

    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Lolwut;
    core_args.is_cluster          = is_cluster;

    if (is_cluster && route && Z_TYPE_P(route) != IS_NULL) {
        core_args.has_route   = 1;
        core_args.route_param = route;
    }

    uint32_t parameter_count = parameters && Z_TYPE_P(parameters) == IS_ARRAY
                                   ? zend_hash_num_elements(Z_ARRVAL_P(parameters))
                                   : 0;
    uint32_t arg_count       = parameter_count + (version && Z_TYPE_P(version) == IS_LONG ? 2 : 0);

    if (arg_count > 0) {
        core_args.all_args = ecalloc(arg_count, sizeof(core_arg_t));
        if (!core_args.all_args) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), "Unable to allocate LOLWUT command arguments", 0);
            return 0;
        }
    }

    uint32_t arg_index = 0;
    if (version && Z_TYPE_P(version) == IS_LONG) {
        core_args.all_args[arg_index].type                  = CORE_ARG_TYPE_STRING;
        core_args.all_args[arg_index].data.string_arg.value = "VERSION";
        core_args.all_args[arg_index].data.string_arg.len   = sizeof("VERSION") - 1;
        arg_index++;

        core_args.all_args[arg_index].type                = CORE_ARG_TYPE_LONG;
        core_args.all_args[arg_index].data.long_arg.value = Z_LVAL_P(version);
        arg_index++;
    }

    if (parameters && Z_TYPE_P(parameters) == IS_ARRAY) {
        zval* parameter;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(parameters), parameter) {
            if (Z_TYPE_P(parameter) != IS_LONG) {
                efree(core_args.all_args);
                zend_throw_exception(get_valkey_glide_exception_ce(),
                                     "LOLWUT parameters must contain only integers",
                                     0);
                return 0;
            }
            core_args.all_args[arg_index].type                = CORE_ARG_TYPE_LONG;
            core_args.all_args[arg_index].data.long_arg.value = Z_LVAL_P(parameter);
            arg_index++;
        }
        ZEND_HASH_FOREACH_END();
    }
    core_args.arg_count = arg_index;

    z_result_processor_t processor =
        is_cluster ? process_core_status_string_result : process_core_string_result;
    int result =
        execute_and_handle_batch(valkey_glide, &core_args, processor, return_value, object);

    if (core_args.all_args) {
        efree(core_args.all_args);
    }

    return result;
}
/* Execute a WATCH command using the Valkey Glide client */
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

/* Execute an UNWATCH command using the Valkey Glide client */
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

/* Execute a SELECT command */
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

/* Execute a MOVE command */
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

/* Execute a CLIENT TRACKINGINFO command using the Valkey Glide client */
int execute_client_tracking_info_command(zval*             object,
                                         int               argc,
                                         zval*             return_value,
                                         zend_class_entry* ce) {
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
    core_args.cmd_type            = ClientTrackingInfo;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }
        if (args_count > 1) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), "Expected at most 1 argument (route)", 0);
            return 0;
        }
        if (args_count == 1 && Z_TYPE(args[0]) != IS_NULL) {
            core_args.has_route   = 1;
            core_args.route_param = &args[0];
        }
    } else {
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    return execute_and_handle_batch(
        valkey_glide, &core_args, process_cluster_array_result, return_value, object);
}
