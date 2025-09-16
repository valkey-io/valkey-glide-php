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
#include "valkey_glide_list_common.h"
#include "valkey_glide_s_common.h"
#include "valkey_glide_z_common.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend_exceptions.h>

#include <ext/hash/php_hash.h>
#include <ext/spl/spl_exceptions.h>
#include <ext/standard/info.h>

#include "command_response.h" /* Include command_response.h for string conversion functions */
#include "valkey_glide_commands_common.h"
#include "valkey_glide_z_common.h"

#if PHP_VERSION_ID < 80400
#include <ext/standard/php_random.h>
#else
#include <ext/random/php_random.h>
#endif

#ifdef PHP_SESSION
#include <ext/session/php_session.h>
#endif

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

int execute_zrandmember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*     key = NULL;
    size_t    key_len;
    zend_long count      = 1;
    zend_bool withscores = 0;

    zval* z_opts = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os|zb", &object, ce, &key, &key_len, &z_opts, &withscores) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Process the options if provided */
    if (argc >= 2) {
        /* If the second parameter is an array, it contains options */
        if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
            /* Reset default values as we'll get them from the array */
            count      = 1;
            withscores = 0;

            /* Look for 'count' option */
            zval* z_count;
            if ((z_count = zend_hash_str_find(Z_ARRVAL_P(z_opts), "count", sizeof("count") - 1)) !=
                    NULL ||
                (z_count = zend_hash_str_find(Z_ARRVAL_P(z_opts), "COUNT", sizeof("COUNT") - 1)) !=
                    NULL) {
                if (z_count && Z_TYPE_P(z_count) == IS_LONG) {
                    count = Z_LVAL_P(z_count);
                }
            }

            /* Look for 'withscores' option */
            zval* z_withscores;
            if ((z_withscores = zend_hash_str_find(
                     Z_ARRVAL_P(z_opts), "withscores", sizeof("withscores") - 1)) != NULL ||
                (z_withscores = zend_hash_str_find(
                     Z_ARRVAL_P(z_opts), "WITHSCORES", sizeof("WITHSCORES") - 1)) != NULL) {
                if (z_withscores && Z_TYPE_P(z_withscores) == IS_TRUE) {
                    withscores = 1;
                }
            }
        }
        /* If the second parameter is a long, it's a count (backward compatibility) */
        else if (Z_TYPE_P(z_opts) == IS_LONG) {
            count = Z_LVAL_P(z_opts);
            /* If there's a third argument, it's withscores (backward compatibility) */
            if (argc >= 3) {
                /* withscores was already parsed above via zend_parse_method_parameters */
            }
        }
        /* If the second parameter is boolean, it's withscores without a count */
        else if (Z_TYPE_P(z_opts) == IS_TRUE) {
            withscores = 1;
        }
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.start            = count; /* reuse start field for count */
    args.withscores       = withscores;

    typedef struct {
        int withscores;
    } array_data_t;

    array_data_t* array_data = emalloc(sizeof(array_data_t));
    array_data->withscores   = withscores;

    int res = execute_z_generic_command(
        valkey_glide, ZRandMember, &args, array_data, process_z_array_zrand_result, return_value);


    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    if (res == 0) {
        /* On empty set, return NULL */
        efree(array_data);
        ZVAL_NULL(return_value);
        return 0;
    }
    return res;
}

int execute_zscore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char * key = NULL, *member = NULL;
    size_t key_len, member_len;


    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &member, &member_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.member           = member;
    args.member_len       = member_len;


    int result = execute_z_generic_command(
        valkey_glide, ZScore, &args, NULL, process_z_double_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    if (result == 1) {
        return 1;
    } else if (result == 0) {
        return 0; /* Member not found */
    } else {
        return -1; /* Error */
    }
}

int execute_zmscore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  key = NULL;
    size_t key_len;
    int    member_count = 0;
    zval*  z_args       = NULL;

    /* Method signature can be either of the following:
     * - zMscore(string key, string member [, string ...])
     * - zMscore(string key, array members)
     */

    /* First, check if we have the second signature with an array */
    if (argc == 2) {
        zval* z_members;

        /* Try to parse as (key, array) */
        if (zend_parse_method_parameters(
                argc, object, "Osa", &object, ce, &key, &key_len, &z_members) == SUCCESS) {
            /* Get ValkeyGlide object */
            valkey_glide_object* valkey_glide =
                VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

            HashTable* ht_members = Z_ARRVAL_P(z_members);
            member_count          = zend_hash_num_elements(ht_members);

            if (member_count == 0) {
                return 0;
            }

            /* Create an array of members from the associative array */
            zval* members = emalloc(sizeof(zval) * member_count);
            zval* data;
            int   idx = 0;

            ZEND_HASH_FOREACH_VAL(ht_members, data) {
                ZVAL_COPY_VALUE(&members[idx++], data);
            }
            ZEND_HASH_FOREACH_END();


            /* Use framework for command execution */
            z_command_args_t args = {0};
            args.key              = key;
            args.key_len          = key_len;
            args.members          = members;
            args.member_count     = member_count;


            int result = execute_z_generic_command(
                valkey_glide, ZMScore, &args, NULL, process_z_array_result, return_value);

            /* Clean up */
            efree(members);
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return result;
        }
    }

    /* If we got here, either the array format failed or we have variadic args */
    /* Parse as (key, member, member, ...) format */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &member_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.members          = z_args; /* z_args already contains our variadic arguments */
    args.member_count     = member_count;


    int result = execute_z_generic_command(
        valkey_glide, ZMScore, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

int execute_zrank_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *member = NULL;
    size_t      key_len, member_len;
    const void* glide_client = NULL;
    long        rank;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &member, &member_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.member           = member;
    args.member_len       = member_len;
    args.withscores       = 0; /* ZRANK doesn't use withscores in this context */


    int result = execute_z_generic_command(
        valkey_glide, ZRank, &args, NULL, process_z_rank_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* For result == -1 (error), return_value remains uninitialized, which is handled by the macro
     */


    return result;
}

int execute_zrevrank_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *member = NULL;
    size_t      key_len, member_len;
    const void* glide_client = NULL;
    long        rank;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &member, &member_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.member           = member;
    args.member_len       = member_len;
    args.withscores       = 0; /* ZREVRANK doesn't use withscores in this context */


    int result = execute_z_generic_command(
        valkey_glide, ZRevRank, &args, NULL, process_z_rank_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* For result == -1 (error), return_value remains uninitialized, which is handled by the macro
     */


    return result;
}

int execute_zincrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *member = NULL;
    size_t      key_len, member_len;
    double      increment;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osds", &object, ce, &key, &key_len, &increment, &member, &member_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.increment        = increment;
    args.member           = member;
    args.member_len       = member_len;


    int result = execute_z_generic_command(
        valkey_glide, ZIncrBy, &args, NULL, process_z_double_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

int execute_zcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    char *      min, *max;
    size_t      min_len, max_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osss", &object, ce, &key, &key_len, &min, &min_len, &max, &max_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.min              = min;
    args.min_len          = min_len;
    args.max              = max;
    args.max_len          = max_len;


    int result = execute_z_generic_command(
        valkey_glide, ZCount, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

int execute_zlexcount_command_internal(valkey_glide_object* valkey_glide,
                                       const char*          key,
                                       size_t               key_len,
                                       const char*          min,
                                       size_t               min_len,
                                       const char*          max,
                                       size_t               max_len,
                                       zval*                return_value) {
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.min              = min;
    args.min_len          = min_len;
    args.max              = max;
    args.max_len          = max_len;

    return execute_z_generic_command(
        valkey_glide, ZLexCount, &args, NULL, process_z_int_result, return_value);
}

int execute_zrem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    int         variadic_argc = 0;
    zval*       z_args        = NULL;
    const void* glide_client  = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &variadic_argc) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.members          = z_args;
    args.member_count     = variadic_argc;


    int result = execute_z_generic_command(
        valkey_glide, ZRem, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

int execute_zremrangebylex_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    char *      min, *max;
    size_t      min_len, max_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osss", &object, ce, &key, &key_len, &min, &min_len, &max, &max_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.min              = min;
    args.min_len          = min_len;
    args.max              = max;
    args.max_len          = max_len;


    int result = execute_z_generic_command(
        valkey_glide, ZRemRangeByLex, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

int execute_zremrangebyrank_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zend_long   start, end;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osll", &object, ce, &key, &key_len, &start, &end) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.start            = start;
    args.end              = end;


    int result = execute_z_generic_command(
        valkey_glide, ZRemRangeByRank, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

int execute_zremrangebyscore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    char *      min, *max;
    size_t      min_len, max_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osss", &object, ce, &key, &key_len, &min, &min_len, &max, &max_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.min              = min;
    args.min_len          = min_len;
    args.max              = max;
    args.max_len          = max_len;


    int result = execute_z_generic_command(
        valkey_glide, ZRemRangeByScore, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

int execute_zrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  key = NULL;
    size_t key_len;
    zval * z_start, *z_end, *options = NULL;

    /* Parse parameters - allow either boolean or array for the optional 4th parameter */
    if (zend_parse_method_parameters(
            argc, object, "Oszz|z", &object, ce, &key, &key_len, &z_start, &z_end, &options) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.z_start          = z_start;
    args.z_end            = z_end;
    args.options          = options;

    /* Parse options to determine if withscores is set */
    range_options_t range_opts = {0};
    parse_range_options(options, &range_opts);


    int result = execute_z_generic_command(
        valkey_glide, ZRange, &args, NULL, process_z_array_result, return_value);

    /* If the command failed, clean up the return array */
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

int execute_zcard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;


    int result = execute_z_generic_command(
        valkey_glide, ZCard, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }
    return result;
}

/* Helper function for ZDIFFSTORE, ZINTERSTORE and ZUNIONSTORE commands */
int execute_zstore_command(valkey_glide_object* valkey_glide,
                           enum RequestType     cmd_type,
                           const char*          dst,
                           size_t               dst_len,
                           zval*                keys,
                           int                  keys_count,
                           zval*                weights,
                           zval*                options,
                           zval*                return_value) {
    z_command_args_t args = {0};
    args.key              = dst; /* Store commands use destination as key */
    args.key_len          = dst_len;
    args.members          = keys; /* Reuse members field for keys array */
    args.member_count     = keys_count;
    args.weights          = weights;
    args.options          = options;

    return execute_z_generic_command(
        valkey_glide, cmd_type, &args, NULL, process_z_int_result, return_value);
}

/* Execute a ZDIFFSTORE command using the Valkey Glide client */
int execute_zdiffstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_weights = NULL, *z_options = NULL;
    HashTable*  keys_hash;
    char*       dst;
    size_t      dst_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osa|aa", &object, ce, &dst, &dst_len, &z_keys, &z_weights, &z_options) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check key count */
    keys_hash = Z_ARRVAL_P(z_keys);
    if (zend_hash_num_elements(keys_hash) == 0) {
        return 0;
    }

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */

    int result = execute_zstore_command(valkey_glide,
                                        ZDiffStore,
                                        dst,
                                        dst_len,
                                        z_keys,
                                        zend_hash_num_elements(keys_hash),
                                        z_weights,
                                        z_options,

                                        return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute a ZINTERSTORE command using the Valkey Glide client */
int execute_zinterstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_weights = NULL, *z_options = NULL;
    zval        z_aggregate_option;
    int         free_options = 0;
    HashTable*  keys_hash;
    char*       dst;
    size_t      dst_len;
    const void* glide_client = NULL;

    /* Parse parameters - we accept both array and string for the options parameter */
    if (zend_parse_method_parameters(
            argc, object, "Osa|zz", &object, ce, &dst, &dst_len, &z_keys, &z_weights, &z_options) ==
        FAILURE) {
        return 0;
    }

    /* If weights is not an array, set it to NULL */
    if (z_weights != NULL && Z_TYPE_P(z_weights) != IS_ARRAY) {
        z_weights = NULL;
    }

    /* If z_options is a string, convert it to an array with ['AGGREGATE' => string] */
    if (z_options && Z_TYPE_P(z_options) == IS_STRING) {
        array_init(&z_aggregate_option);
        add_assoc_stringl(
            &z_aggregate_option, "AGGREGATE", Z_STRVAL_P(z_options), Z_STRLEN_P(z_options));
        z_options    = &z_aggregate_option;
        free_options = 1;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check key count */
    keys_hash = Z_ARRVAL_P(z_keys);
    if (zend_hash_num_elements(keys_hash) == 0) {
        if (free_options) {
            zval_dtor(&z_aggregate_option);
        }
        return 0;
    }

    /* Check if we have a valid glide client */
    if (!glide_client) {
        if (free_options) {
            zval_dtor(&z_aggregate_option);
        }
        return 0;
    }

    /* Use framework for command execution */

    int result = execute_zstore_command(valkey_glide,
                                        ZInterStore,
                                        dst,
                                        dst_len,
                                        z_keys,
                                        zend_hash_num_elements(keys_hash),
                                        z_weights,
                                        z_options,
                                        return_value);
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* Free the temporary options array if we created one */
    if (free_options) {
        zval_dtor(&z_aggregate_option);
    }


    return result;
}

/* Execute a ZUNIONSTORE command using the Valkey Glide client */
int execute_zunionstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_weights = NULL, *z_options = NULL;
    zval        z_aggregate_option;
    int         free_options = 0;
    HashTable*  keys_hash;
    char*       dst;
    size_t      dst_len;
    const void* glide_client = NULL;

    /* Parse parameters - we accept both array and string for the options parameter */
    if (zend_parse_method_parameters(
            argc, object, "Osa|zz", &object, ce, &dst, &dst_len, &z_keys, &z_weights, &z_options) ==
        FAILURE) {
        return 0;
    }

    /* If weights is not an array, set it to NULL */
    if (z_weights != NULL && Z_TYPE_P(z_weights) != IS_ARRAY) {
        z_weights = NULL;
    }

    /* If z_options is a string, convert it to an array with ['AGGREGATE' => string] */
    if (z_options && Z_TYPE_P(z_options) == IS_STRING) {
        array_init(&z_aggregate_option);
        add_assoc_stringl(
            &z_aggregate_option, "AGGREGATE", Z_STRVAL_P(z_options), Z_STRLEN_P(z_options));
        z_options    = &z_aggregate_option;
        free_options = 1;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check key count */
    keys_hash = Z_ARRVAL_P(z_keys);
    if (zend_hash_num_elements(keys_hash) == 0) {
        if (free_options) {
            zval_dtor(&z_aggregate_option);
        }
        return 0;
    }

    /* Check if we have a valid glide client */
    if (!glide_client) {
        if (free_options) {
            zval_dtor(&z_aggregate_option);
        }
        return 0;
    }

    /* Use framework for command execution */

    int result = execute_zstore_command(valkey_glide,
                                        ZUnionStore,
                                        dst,
                                        dst_len,
                                        z_keys,
                                        zend_hash_num_elements(keys_hash),
                                        z_weights,
                                        z_options,
                                        return_value);
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* Free the temporary options array if we created one */
    if (free_options) {
        zval_dtor(&z_aggregate_option);
    }


    return result;
}

/* Execute a ZRANGEBYSCORE command using the Valkey Glide client */
int execute_zrangebyscore_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval *      z_min = NULL, *z_max = NULL, *z_opts = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oszz|z", &object, ce, &key, &key_len, &z_min, &z_max, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.z_start          = z_min;
    args.z_end            = z_max;
    args.options          = z_opts;

    /* Parse options to determine if withscores is set */
    range_options_t range_opts = {0};
    parse_range_options(z_opts, &range_opts);


    int result = execute_z_generic_command(
        valkey_glide, ZRangeByScore, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute a ZREVRANGEBYSCORE command using the Valkey Glide client */
int execute_zrevrangebyscore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval *      z_max, *z_min, *options = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oszz|a", &object, ce, &key, &key_len, &z_max, &z_min, &options) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.z_start          = z_max; /* For ZREVRANGEBYSCORE, start is max */
    args.z_end            = z_min; /* For ZREVRANGEBYSCORE, end is min */
    args.options          = options;

    /* Parse options to determine if withscores is set */
    range_options_t range_opts = {0};
    parse_range_options(options, &range_opts);


    int result = execute_z_generic_command(
        valkey_glide, ZRevRangeByScore, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute a ZRANGEBYLEX command using the Valkey Glide client */
int execute_zrangebylex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval *      z_min, *z_max, *options = NULL;
    const void* glide_client = NULL;
    zend_long   offset = -1, count = -1;

    /* Parse parameters - allow either options array or offset/count */
    if (argc == 4) {
        if (zend_parse_method_parameters(
                argc, object, "Oszz|z", &object, ce, &key, &key_len, &z_min, &z_max, &options) ==
            FAILURE) {
            return 0;
        }
    } else if (argc == 5) {
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Oszzll",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &z_min,
                                         &z_max,
                                         &offset,
                                         &count) == FAILURE) {
            return 0;
        }
    } else {
        if (zend_parse_method_parameters(
                argc, object, "Oszz", &object, ce, &key, &key_len, &z_min, &z_max) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }


    /* If offset and count are provided as separate parameters, create options array */
    zval new_options;
    if (offset >= 0 && count >= 0) {
        array_init(&new_options);

        /* Create LIMIT subarray */
        zval limit_array;
        array_init(&limit_array);
        add_index_long(&limit_array, 0, offset);
        add_index_long(&limit_array, 1, count);

        /* Add LIMIT subarray to options */
        add_assoc_zval(&new_options, "LIMIT", &limit_array);
        options = &new_options;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.z_start          = z_min;
    args.z_end            = z_max;
    args.options          = options;

    /* Parse options to determine if withscores is set */
    range_options_t range_opts = {0};
    parse_range_options(options, &range_opts);
    range_opts.bylex = 1; /* ZRANGEBYLEX always has BYLEX */


    int result = execute_z_generic_command(
        valkey_glide, ZRangeByLex, &args, NULL, process_z_array_result, return_value);

    /* Free the temporary options array if we created one */
    if (offset >= 0 && count >= 0) {
        zval_dtor(&new_options);
    }

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute a ZINTERCARD command using the Valkey Glide client */
int execute_zintercard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_options = NULL;
    zval        z_temp_options;
    const void* glide_client = NULL;
    HashTable*  keys_hash;
    long        limit        = 0;
    int         free_options = 0;

    /* Parse parameters - accept either array,array or array,long */
    if (zend_parse_method_parameters(argc, object, "Oa|z", &object, ce, &z_keys, &z_options) ==
        FAILURE) {
        return 0;
    }

    /* If second parameter is an integer (limit), convert it to an options array */
    if (z_options && Z_TYPE_P(z_options) == IS_LONG) {
        limit = Z_LVAL_P(z_options);
        if (limit < 0) {
            return 0;
        }

        array_init(&z_temp_options);
        add_assoc_long(&z_temp_options, "LIMIT", limit);
        z_options    = &z_temp_options;
        free_options = 1; /* Flag to free the temporary array */
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check key count */
    keys_hash = Z_ARRVAL_P(z_keys);
    if (zend_hash_num_elements(keys_hash) == 0) {
        if (free_options) {
            zval_dtor(&z_temp_options);
        }
        return 0;
    }

    /* Check if we have a valid glide client */
    if (!glide_client) {
        if (free_options) {
            zval_dtor(&z_temp_options);
        }
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.members          = z_keys; /* Reuse members field for keys */
    args.member_count     = zend_hash_num_elements(keys_hash);
    args.options          = z_options;

    int result = execute_z_generic_command(
        valkey_glide, ZInterCard, &args, NULL, process_z_long_to_zval_result, return_value);

    /* If we created a temporary options array, free it */
    if (free_options) {
        zval_dtor(&z_temp_options);
    }


    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute a ZUNION command using the Valkey Glide client */
int execute_zunion_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_weights = NULL, *z_options = NULL;
    const void* glide_client = NULL;
    HashTable*  keys_hash;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oa|za", &object, ce, &z_keys, &z_weights, &z_options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check key count */
    keys_hash = Z_ARRVAL_P(z_keys);
    if (zend_hash_num_elements(keys_hash) == 0) {
        return 0;
    }

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.members          = z_keys; /* Reuse members field for keys */
    args.member_count     = zend_hash_num_elements(keys_hash);
    args.weights          = z_weights;
    args.options          = z_options;


    int result = execute_z_generic_command(
        valkey_glide, ZUnion, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute a ZPOPMAX command using the Valkey Glide client */
int execute_zpopmax_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    long        count        = 1;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|l", &object, ce, &key, &key_len, &count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.start            = count; /* Reuse start field for count */


    int result = execute_z_generic_command(
        valkey_glide, ZPopMax, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute a ZPOPMIN command using the Valkey Glide client */
int execute_zpopmin_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    long        count        = 1;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|l", &object, ce, &key, &key_len, &count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }


    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.start            = count; /* Reuse start field for count */


    int result = execute_z_generic_command(
        valkey_glide, ZPopMin, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute a ZADD command using the Valkey Glide client - internal implementation with batch support
 */
int execute_zadd_command_internal(valkey_glide_object* valkey_glide,
                                  const char*          key,
                                  size_t               key_len,
                                  zval*                z_args,
                                  int                  argc,
                                  int                  flags,
                                  long*                output_value,
                                  double*              output_value_double,
                                  zval*                return_value) {
    z_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.members          = z_args;
    args.member_count     = argc;

    /* Determine if INCR option is present by parsing first element if it's an array */
    int has_incr = 0;
    if (argc > 0 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        zadd_options_t zadd_opts = {0};
        parse_zadd_options(&z_args[0], &zadd_opts);
        has_incr = zadd_opts.incr;
    }

    /* Single call - let execute_z_generic_command handle batch vs normal internally */
    int result = execute_z_generic_command(
        valkey_glide, ZAdd, &args, NULL, process_z_long_to_zval_result, return_value);


    return result;
}

/* Execute a ZRANGESTORE command using the Valkey Glide client */
int execute_zrangestore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      src = NULL, *dst = NULL;
    size_t      src_len, dst_len;
    zval *      z_start, *z_end, *options = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osszz|a",
                                     &object,
                                     ce,
                                     &dst,
                                     &dst_len,
                                     &src,
                                     &src_len,
                                     &z_start,
                                     &z_end,
                                     &options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.key              = dst; /* dst is the destination key */
    args.key_len          = dst_len;
    args.member           = src; /* src is the source key (reuse member field) */
    args.member_len       = src_len;
    args.z_start          = z_start;
    args.z_end            = z_end;
    args.options          = options;

    int result = execute_z_generic_command(
        valkey_glide, ZRangeStore, &args, NULL, process_z_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute a ZDIFF command using the Valkey Glide client - standardized version */
/* Execute a ZDIFF command using the Valkey Glide client - internal implementation */
int execute_zdiff_command_internal(valkey_glide_object* valkey_glide,
                                   zval*                keys,
                                   zval*                options,
                                   zval*                return_value) {
    z_command_args_t args = {0};
    args.members          = keys; /* Reuse members field for keys */
    args.member_count     = zend_hash_num_elements(Z_ARRVAL_P(keys));
    args.options          = options;


    int result = execute_z_generic_command(
        valkey_glide, ZDiff, &args, NULL, process_z_array_result, return_value);

    return result;
}

int execute_zdiff_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_opts = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oa|a", &object, ce, &z_keys, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Execute the ZDIFF command using the internal function */
    if (execute_zdiff_command_internal(valkey_glide, z_keys, z_opts, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    /* If the command failed, clean up and return FALSE */
    zval_dtor(return_value);
    return 0;
}

/* Helper function to prepare arguments for MPOP commands */
int prepare_mpop_arguments(const void*     glide_client,
                           int             is_blocking,
                           double          timeout,
                           zval*           keys,
                           const char*     from,
                           size_t          from_len,
                           long            count,
                           unsigned long*  arg_count_ptr,
                           uintptr_t**     args_ptr,
                           unsigned long** args_len_ptr,
                           char**          numkeys_str_ptr,
                           char**          timeout_str_ptr,
                           char**          count_str_ptr) {
    /* Get the number of keys */
    int keys_count = 0;
    if (Z_TYPE_P(keys) == IS_ARRAY) {
        keys_count = zend_hash_num_elements(Z_ARRVAL_P(keys));
    } else {
        return 0; /* Keys must be an array */
    }

    /* Check if we have at least one key */
    if (keys_count <= 0) {
        return 0;
    }

    /* Calculate the number of arguments */
    unsigned long arg_count = keys_count + 2; /* numkeys + keys + direction */
    if (is_blocking) {
        arg_count++; /* Add timeout for blocking commands */
    }

    /* Allocate memory for arguments */
    uintptr_t*     args     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!args || !args_len) {
        if (args)
            efree(args);
        if (args_len)
            efree(args_len);
        return 0;
    }

    /* Current argument index */
    int arg_idx = 0;

    /* Add timeout for blocking commands */
    if (is_blocking) {
        /* Convert timeout to string */
        size_t timeout_len;
        char*  timeout_str = alloc_list_double_string(timeout, &timeout_len);
        if (!timeout_str) {
            efree(args);
            efree(args_len);
            return 0;
        }
        args[arg_idx]     = (uintptr_t) timeout_str;
        args_len[arg_idx] = timeout_len;
        *timeout_str_ptr  = timeout_str;
        arg_idx++;
    }

    /* Add numkeys first (this should be the first argument after timeout for blocking commands) */
    size_t numkeys_len;
    char*  numkeys_str = alloc_list_number_string(keys_count, &numkeys_len);
    if (!numkeys_str) {
        efree(args);
        efree(args_len);
        if (is_blocking) {
            efree(*timeout_str_ptr);
            *timeout_str_ptr = NULL;
        }
        return 0;
    }
    /* Debug output to see the value being passed */

    args[arg_idx]     = (uintptr_t) numkeys_str;
    args_len[arg_idx] = numkeys_len;
    *numkeys_str_ptr  = numkeys_str;
    arg_idx++;

    /* Add keys */
    HashTable* ht = Z_ARRVAL_P(keys);
    zval*      z_key;
    ZEND_HASH_FOREACH_VAL(ht, z_key) {
        if (Z_TYPE_P(z_key) != IS_STRING) {
            efree(args);
            efree(args_len);
            efree(numkeys_str);
            *numkeys_str_ptr = NULL;
            if (is_blocking) {
                efree(*timeout_str_ptr);
                *timeout_str_ptr = NULL;
            }
            return 0;
        }
        args[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_key);
        args_len[arg_idx] = Z_STRLEN_P(z_key);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add direction (LEFT or RIGHT) directly */
    args[arg_idx]     = (uintptr_t) from;
    args_len[arg_idx] = from_len;
    arg_idx++;

    /* Add COUNT if count > 1 */
    if (count > 1) {
        /* Increase arg_count for COUNT and its value */
        arg_count += 2;

        /* Reallocate args and args_len arrays */
        uintptr_t*     new_args = (uintptr_t*) erealloc(args, arg_count * sizeof(uintptr_t));
        unsigned long* new_args_len =
            (unsigned long*) erealloc(args_len, arg_count * sizeof(unsigned long));

        if (!new_args || !new_args_len) {
            efree(args);
            efree(args_len);
            efree(numkeys_str);
            *numkeys_str_ptr = NULL;
            if (is_blocking) {
                efree(*timeout_str_ptr);
                *timeout_str_ptr = NULL;
            }
            return 0;
        }

        args     = new_args;
        args_len = new_args_len;

        /* Add COUNT keyword */
        args[arg_idx]     = (uintptr_t) "COUNT";
        args_len[arg_idx] = 5;
        arg_idx++;

        /* Add count value */
        size_t count_len;
        char*  count_str = alloc_list_number_string(count, &count_len);
        if (!count_str) {
            efree(args);
            efree(args_len);
            efree(numkeys_str);
            *numkeys_str_ptr = NULL;
            if (is_blocking) {
                efree(*timeout_str_ptr);
                *timeout_str_ptr = NULL;
            }
            return 0;
        }
        args[arg_idx]     = (uintptr_t) count_str;
        args_len[arg_idx] = count_len;
        *count_str_ptr    = count_str;
        arg_idx++;
    }

    /* Set output parameters */
    *arg_count_ptr = arg_count;
    *args_ptr      = args;
    *args_len_ptr  = args_len;

    return keys_count;
}
int process_zmpop_result(CommandResponse* response,
                         void*            output,
                         zval*            return_value) { /* Process the result */
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
}

/* Execute a ZMPOP or BZMPOP command (for sorted set operations) using the Valkey Glide client
 */
int execute_zmpop_command_internal(valkey_glide_object* valkey_glide,
                                   zval*                object,
                                   const char*          cmd,
                                   double               timeout,
                                   zval*                keys,
                                   const char*          from,
                                   size_t               from_len,
                                   long                 count,
                                   zval*                return_value) {
    /* Check if client, keys, and from are valid */
    if (!valkey_glide || !keys || !from) {
        return 0;
    }

    /* Determine if this is a blocking command */
    int is_blocking = (strncmp(cmd, "B", 1) == 0);

    /* Prepare for argument construction */
    unsigned long  arg_count   = 0;
    uintptr_t*     args        = NULL;
    unsigned long* args_len    = NULL;
    char*          numkeys_str = NULL;
    char*          timeout_str = NULL;
    char*          count_str   = NULL;

    /* Prepare the arguments */
    int keys_count = prepare_mpop_arguments(valkey_glide->glide_client,
                                            is_blocking,
                                            timeout,
                                            keys,
                                            from,
                                            from_len,
                                            count,
                                            &arg_count,
                                            &args,
                                            &args_len,
                                            &numkeys_str,
                                            &timeout_str,
                                            &count_str);

    if (keys_count < 0) {
        return 0;
    }

    /* Determine the command type */
    enum RequestType cmd_type   = is_blocking ? BZMPop : ZMPop;
    CommandResult*   cmd_result = NULL;
    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* Create batch-compatible processor wrapper */
        int res = buffer_command_for_batch(
            valkey_glide, cmd_type, args, args_len, arg_count, NULL, process_zmpop_result);
    } else {
        /* Execute the command */
        cmd_result = command(valkey_glide->glide_client,
                             0,         /* channel */
                             cmd_type,  /* command type */
                             arg_count, /* number of arguments */
                             args,      /* arguments */
                             args_len,  /* argument lengths */
                             NULL,      /* route bytes */
                             0,         /* route bytes length */
                             0          /* span_ptr */
        );
    }

    /* Free the argument strings */
    if (numkeys_str)
        efree(numkeys_str);
    if (timeout_str)
        efree(timeout_str);
    if (count_str)
        efree(count_str);
    efree(args);
    efree(args_len);
    int ret_val = 0;
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        ret_val = 1;
    } else {
        /* Check if the command was successful */
        if (!cmd_result) {
            return 0;
        }

        /* Check if there was an error */
        if (cmd_result->command_error) {
            free_command_result(cmd_result);
            return 0;
        }

        ret_val = process_zmpop_result(cmd_result->response, NULL, return_value);

        /* Free the result */
        free_command_result(cmd_result);
    }

    return ret_val;
}

/* Execute a ZINTER command using the Valkey Glide client */
int execute_zinter_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *      z_keys, *z_weights = NULL, *z_opts = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oa|za", &object, ce, &z_keys, &z_weights, &z_opts) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.members          = z_keys; /* Reuse members field for keys */
    args.member_count     = zend_hash_num_elements(Z_ARRVAL_P(z_keys));
    args.weights          = z_weights;
    args.options          = z_opts;


    int result = execute_z_generic_command(
        valkey_glide, ZInter, &args, NULL, process_z_array_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }


    return result;
}

/* Execute BZMPOP command using the Valkey Glide client */
int execute_bzmpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval*       z_keys;
    double      timeout;
    zend_long   count = 1;
    char*       from  = NULL;
    size_t      from_len;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Odas|l", &object, ce, &timeout, &z_keys, &from, &from_len, &count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Initialize result zval */
    ZVAL_NULL(return_value);

    /* Execute the command - we pass "BZMPOP" as the command name for messaging */
    return execute_zmpop_command_internal(
        valkey_glide, object, "BZMPOP", timeout, z_keys, from, from_len, count, return_value);
}

/* Execute ZMPOP command using the Valkey Glide client */
int execute_zmpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval*       z_keys;
    char*       from = NULL;
    size_t      from_len;
    zend_long   count        = 1;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oas|l", &object, ce, &z_keys, &from, &from_len, &count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Execute the command */
    return execute_zmpop_command_internal(
        valkey_glide, object, "ZMPOP", 0.0, z_keys, from, from_len, count, return_value);
}

/* Execute a ZADD command - simplified to eliminate code duplication */
int execute_zadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  key = NULL;
    size_t key_len;
    zval*  z_args;
    int    variadic_argc       = 0;
    int    flags               = 0; /* No flags by default */
    long   result_value        = 0;
    double result_value_double = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &variadic_argc) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Check if we have a valid valkey_glide object */
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }


    int result = execute_zadd_command_internal(valkey_glide,
                                               key,
                                               key_len,
                                               z_args,
                                               variadic_argc,
                                               flags,
                                               &result_value,
                                               &result_value_double,
                                               return_value);

    if (result == 0) {
        return 0; /* Command failed */
    }

    /* Handle return value based on batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    } else if (result == 1) {
        /* Standard result as long */
        return 1;
    } else if (result == 2) {
        /* INCR result as double */
        return 1;
    }

    return 0; /* Should not happen */
}

/* Execute ZLEXCOUNT command with the standardized parameter format */
int execute_zlexcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*  key = NULL;
    size_t key_len;
    char * min = NULL, *max = NULL;
    size_t min_len, max_len;


    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osss", &object, ce, &key, &key_len, &min, &min_len, &max, &max_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Execute the ZLEXCOUNT command using the internal function */
    if (execute_zlexcount_command_internal(
            valkey_glide, key, key_len, min, min_len, max, max_len, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Execute BZPOPMAX command using the generic framework */
int execute_bzpopmax_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *    z_keys = NULL, *z_timeout = NULL;
    zval*     z_args = NULL;
    zval      z_processed_keys;
    int       keys_count   = 0;
    double    timeout      = 0.0;
    zend_bool is_array_arg = 0;

    /* Check if we have exactly 2 arguments (could be array + timeout) */
    if (argc == 2) {
        /* Parse as array + timeout */
        if (zend_parse_method_parameters(2, object, "Ozz", &object, ce, &z_keys, &z_timeout) ==
            FAILURE) {
            return 0;
        }

        /* Check if first parameter is an array */
        if (Z_TYPE_P(z_keys) == IS_ARRAY) {
            /* Get timeout value */
            if (Z_TYPE_P(z_timeout) == IS_LONG) {
                timeout = (double) Z_LVAL_P(z_timeout);
            } else if (Z_TYPE_P(z_timeout) == IS_DOUBLE) {
                timeout = Z_DVAL_P(z_timeout);
            } else {
                php_error_docref(NULL, E_WARNING, "Timeout must be a numeric value");
                return 0;
            }

            /* Create a new array for processed keys */
            array_init(&z_processed_keys);

            /* Copy all keys to the new array */
            zval* key_entry;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), key_entry) {
                if (Z_TYPE_P(key_entry) != IS_STRING) {
                    /* Convert to string if possible */
                    zval tmp;
                    ZVAL_COPY(&tmp, key_entry);
                    convert_to_string(&tmp);
                    add_next_index_zval(&z_processed_keys, &tmp);
                } else {
                    /* Add as-is if already string */
                    zval tmp;
                    ZVAL_COPY(&tmp, key_entry);
                    add_next_index_zval(&z_processed_keys, &tmp);
                }
            }
            ZEND_HASH_FOREACH_END();

            is_array_arg = 1;
            keys_count   = zend_hash_num_elements(Z_ARRVAL(z_processed_keys));
            z_keys       = &z_processed_keys; /* Use processed keys */
        }
    }
    if (is_array_arg == 0) {
        /* Use variadic format */
        if (zend_parse_method_parameters(
                argc, object, "O+d", &object, ce, &z_args, &keys_count, &timeout) == FAILURE) {
            return 0;
        }

        /* Need at least one key */
        if (keys_count < 1) {
            return 0;
        }

        z_keys = z_args; /* Use variadic args directly */
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Check for valid glide client */
    if (!valkey_glide || !valkey_glide->glide_client) {
        if (is_array_arg) {
            zval_ptr_dtor(&z_processed_keys);
        }
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.members          = z_keys;     /* Keys array */
    args.member_count     = keys_count; /* Number of keys */
    args.increment        = timeout;    /* Reuse increment field for timeout */

    int result = execute_z_generic_command(
        valkey_glide, BZPopMax, &args, NULL, process_z_bzpop_result, return_value);

    /* Clean up if we created a processed keys array */
    if (is_array_arg) {
        zval_ptr_dtor(&z_processed_keys);
    }

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute BZPOPMIN command using the generic framework */
int execute_bzpopmin_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    zval *    z_keys = NULL, *z_timeout = NULL;
    zval*     z_args = NULL;
    zval      z_processed_keys;
    int       keys_count   = 0;
    double    timeout      = 0.0;
    zend_bool is_array_arg = 0;

    /* Check if we have exactly 2 arguments (could be array + timeout) */
    if (argc == 2) {
        /* Parse as array + timeout */
        if (zend_parse_method_parameters(2, object, "Ozz", &object, ce, &z_keys, &z_timeout) ==
            FAILURE) {
            return 0;
        }

        /* Check if first parameter is an array */
        if (Z_TYPE_P(z_keys) == IS_ARRAY) {
            /* Get timeout value */
            if (Z_TYPE_P(z_timeout) == IS_LONG) {
                timeout = (double) Z_LVAL_P(z_timeout);
            } else if (Z_TYPE_P(z_timeout) == IS_DOUBLE) {
                timeout = Z_DVAL_P(z_timeout);
            } else {
                php_error_docref(NULL, E_WARNING, "Timeout must be a numeric value");
                return 0;
            }

            /* Create a new array for processed keys */
            array_init(&z_processed_keys);

            /* Copy all keys to the new array */
            zval* key_entry;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), key_entry) {
                if (Z_TYPE_P(key_entry) != IS_STRING) {
                    /* Convert to string if possible */
                    zval tmp;
                    ZVAL_COPY(&tmp, key_entry);
                    convert_to_string(&tmp);
                    add_next_index_zval(&z_processed_keys, &tmp);
                } else {
                    /* Add as-is if already string */
                    zval tmp;
                    ZVAL_COPY(&tmp, key_entry);
                    add_next_index_zval(&z_processed_keys, &tmp);
                }
            }
            ZEND_HASH_FOREACH_END();

            is_array_arg = 1;
            keys_count   = zend_hash_num_elements(Z_ARRVAL(z_processed_keys));
            z_keys       = &z_processed_keys; /* Use processed keys */
        }
    }
    if (is_array_arg == 0) {
        /* Use variadic format */
        if (zend_parse_method_parameters(
                argc, object, "O+d", &object, ce, &z_args, &keys_count, &timeout) == FAILURE) {
            return 0;
        }

        /* Need at least one key */
        if (keys_count < 1) {
            return 0;
        }

        z_keys = z_args; /* Use variadic args directly */
    }


    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Check for valid glide client */
    if (!valkey_glide || !valkey_glide->glide_client) {
        if (is_array_arg) {
            zval_ptr_dtor(&z_processed_keys);
        }
        return 0;
    }

    /* Use framework for command execution */
    z_command_args_t args = {0};
    args.members          = z_keys;     /* Keys array */
    args.member_count     = keys_count; /* Number of keys */
    args.increment        = timeout;    /* Reuse increment field for timeout */

    int result = execute_z_generic_command(
        valkey_glide, BZPopMin, &args, NULL, process_z_bzpop_result, return_value);

    /* Clean up if we created a processed keys array */
    if (is_array_arg) {
        zval_ptr_dtor(&z_processed_keys);
    }

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    return result;
}

/* Execute a ZSCAN command using the Valkey Glide client */
int execute_zscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_scan_command_generic(object, argc, return_value, ce, ZScan);
}
