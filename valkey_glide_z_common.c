/*
  +----------------------------------------------------------------------+
  | Valkey Glide Z-Commands Common Utilities                             |
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

#include "valkey_glide_z_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "valkey_glide_commands_common.h"

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* ====================================================================
 * OPTIONS PARSING HELPERS
 * ==================================================================== */

/**
 * Parse range command options (withscores, byscore, bylex, rev, limit)
 * Returns 1 on success, 0 on failure
 */
int parse_range_options(zval* options, range_options_t* opts) {
    if (!opts) {
        return 0;
    }

    /* Initialize options structure */
    memset(opts, 0, sizeof(range_options_t));

    if (!options) {
        return 1; /* No options is valid */
    }

    if (Z_TYPE_P(options) == IS_TRUE) {
        /* Direct boolean TRUE means WITHSCORES */
        opts->withscores = 1;
        return 1;
    }

    if (Z_TYPE_P(options) != IS_ARRAY) {
        return 1; /* No options is valid */
    }

    HashTable* ht = Z_ARRVAL_P(options);
    zval*      entry;

    /* Check for withscores option */
    if ((entry = zend_hash_str_find(ht, "withscores", sizeof("withscores") - 1)) != NULL ||
        (entry = zend_hash_str_find(ht, "WITHSCORES", sizeof("WITHSCORES") - 1)) != NULL) {
        if (Z_TYPE_P(entry) == IS_TRUE) {
            opts->withscores = 1;
        }
    }

    /* Check for byscore option */
    if ((entry = zend_hash_str_find(ht, "byscore", sizeof("byscore") - 1)) != NULL ||
        (entry = zend_hash_str_find(ht, "BYSCORE", sizeof("BYSCORE") - 1)) != NULL) {
        opts->byscore = 1;
    } else {
        /* Check if 'byscore' exists as a value in the array */
        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (Z_TYPE_P(entry) == IS_STRING &&
                strncasecmp(Z_STRVAL_P(entry), "byscore", Z_STRLEN_P(entry)) == 0) {
                opts->byscore = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Check for bylex option */
    if ((entry = zend_hash_str_find(ht, "bylex", sizeof("bylex") - 1)) != NULL ||
        (entry = zend_hash_str_find(ht, "BYLEX", sizeof("BYLEX") - 1)) != NULL) {
        opts->bylex = 1;
    } else {
        /* Check if 'bylex' exists as a value in the array */
        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (Z_TYPE_P(entry) == IS_STRING &&
                strncasecmp(Z_STRVAL_P(entry), "bylex", Z_STRLEN_P(entry)) == 0) {
                opts->bylex = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Check for rev option */
    if ((entry = zend_hash_str_find(ht, "rev", sizeof("rev") - 1)) != NULL ||
        (entry = zend_hash_str_find(ht, "REV", sizeof("REV") - 1)) != NULL) {
        opts->rev = 1;
    } else {
        /* Check if 'rev' exists as a value in the array */
        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (Z_TYPE_P(entry) == IS_STRING &&
                strncasecmp(Z_STRVAL_P(entry), "rev", Z_STRLEN_P(entry)) == 0) {
                opts->rev = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Check for limit option */
    if ((entry = zend_hash_str_find(ht, "limit", sizeof("limit") - 1)) != NULL ||
        (entry = zend_hash_str_find(ht, "LIMIT", sizeof("LIMIT") - 1)) != NULL) {
        if (Z_TYPE_P(entry) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(entry)) >= 2) {
            zval *     z_offset, *z_count;
            HashTable* limit_ht = Z_ARRVAL_P(entry);

            /* Get offset (first element) */
            z_offset = zend_hash_index_find(limit_ht, 0);
            if (z_offset && Z_TYPE_P(z_offset) == IS_LONG) {
                opts->limit_offset = Z_LVAL_P(z_offset);

                /* Get count (second element) */
                z_count = zend_hash_index_find(limit_ht, 1);
                if (z_count && Z_TYPE_P(z_count) == IS_LONG) {
                    opts->limit_count = Z_LVAL_P(z_count);
                    opts->has_limit   = 1;
                }
            }
        }
    }

    return 1;
}

/**
 * Parse ZADD command options (XX, NX, LT, GT, CH, INCR)
 * Returns 1 on success, 0 on failure
 */
int parse_zadd_options(zval* options, zadd_options_t* opts) {
    if (!opts) {
        return 0;
    }

    /* Initialize options structure */
    memset(opts, 0, sizeof(zadd_options_t));

    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    HashTable*   ht = Z_ARRVAL_P(options);
    zval*        entry;
    zend_string* key;
    zend_ulong   num_key;

    /* Process each option */
    ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, key, entry) {
        if (Z_TYPE_P(entry) == IS_STRING) {
            char* opt_str = Z_STRVAL_P(entry);

            if (strcasecmp(opt_str, "XX") == 0) {
                opts->xx = 1;
            } else if (strcasecmp(opt_str, "NX") == 0) {
                opts->nx = 1;
            } else if (strcasecmp(opt_str, "LT") == 0) {
                opts->lt = 1;
            } else if (strcasecmp(opt_str, "GT") == 0) {
                opts->gt = 1;
            } else if (strcasecmp(opt_str, "CH") == 0) {
                opts->ch = 1;
            } else if (strcasecmp(opt_str, "INCR") == 0) {
                opts->incr = 1;
            }
        } else if (key != NULL) {
            /* Handle associative array options */
            char* opt_key = ZSTR_VAL(key);

            if (strcasecmp(opt_key, "XX") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->xx = 1;
            } else if (strcasecmp(opt_key, "NX") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->nx = 1;
            } else if (strcasecmp(opt_key, "LT") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->lt = 1;
            } else if (strcasecmp(opt_key, "GT") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->gt = 1;
            } else if (strcasecmp(opt_key, "CH") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->ch = 1;
            } else if (strcasecmp(opt_key, "INCR") == 0 && Z_TYPE_P(entry) == IS_TRUE) {
                opts->incr = 1;
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    return 1;
}

/**
 * Parse store command options (weights, aggregate) for ZUNIONSTORE-style commands
 * Returns 1 on success, 0 on failure
 */
int parse_store_options(zval* weights, zval* options, store_options_t* opts) {
    if (!opts) {
        return 0;
    }

    /* Initialize options structure */
    memset(opts, 0, sizeof(store_options_t));

    /* Parse weights */
    if (weights && Z_TYPE_P(weights) == IS_ARRAY) {
        opts->weights     = weights;
        opts->has_weights = 1;
    }

    /* Parse aggregate option */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        HashTable* ht        = Z_ARRVAL_P(options);
        zval*      aggregate = zend_hash_str_find(ht, "AGGREGATE", sizeof("AGGREGATE") - 1);
        if (!aggregate) {
            aggregate = zend_hash_str_find(ht, "aggregate", sizeof("aggregate") - 1);
        }

        if (aggregate && Z_TYPE_P(aggregate) == IS_STRING) {
            const char* agg_str = Z_STRVAL_P(aggregate);
            if (strcasecmp(agg_str, "SUM") == 0 || strcasecmp(agg_str, "MIN") == 0 ||
                strcasecmp(agg_str, "MAX") == 0) {
                opts->aggregate     = aggregate;
                opts->has_aggregate = 1;
            }
        }

        /* Check for withscores option */
        zval* withscores = zend_hash_str_find(ht, "withscores", sizeof("withscores") - 1);
        if (!withscores) {
            withscores = zend_hash_str_find(ht, "WITHSCORES", sizeof("WITHSCORES") - 1);
        }

        if (withscores && (Z_TYPE_P(withscores) == IS_TRUE ||
                           (Z_TYPE_P(withscores) == IS_LONG && Z_LVAL_P(withscores) == 1))) {
            opts->withscores = 1;
        }
    }

    return 1;
}

/* ====================================================================
 * CONVERSION & UTILITY HELPERS
 * ==================================================================== */

/**
 * Create LIMIT arguments (offset, count)
 * Returns number of arguments added (0 or 3)
 */
int create_limit_args(range_options_t* opts,
                      uintptr_t*       args,
                      unsigned long*   args_len,
                      int              start_idx,
                      char**           allocated_strings,
                      int*             allocated_count) {
    if (!opts->has_limit) {
        return 0;
    }

    /* Add LIMIT keyword */
    args[start_idx]     = (uintptr_t) "LIMIT";
    args_len[start_idx] = 5;

    /* Add offset parameter */
    size_t len;
    char*  offset_str = long_to_string(opts->limit_offset, &len);
    if (!offset_str) {
        return 0;
    }
    args[start_idx + 1]                     = (uintptr_t) offset_str;
    args_len[start_idx + 1]                 = len;
    allocated_strings[(*allocated_count)++] = offset_str;

    /* Add count parameter */
    char* count_str = long_to_string(opts->limit_count, &len);
    if (!count_str) {
        return 0;
    }
    args[start_idx + 2]                     = (uintptr_t) count_str;
    args_len[start_idx + 2]                 = len;
    allocated_strings[(*allocated_count)++] = count_str;

    return 3; /* LIMIT + offset + count */
}

/* ====================================================================
 * RESPONSE PROCESSING HELPERS
 * ==================================================================== */


/**
 * Flatten withscores array from [[member, score]] to [member => score]
 * Returns 1 on success, 0 on failure
 */
int flatten_withscores_array(zval* return_value) {
    if (!return_value || Z_TYPE_P(return_value) != IS_ARRAY) {
        return 0;
    }

    HashTable* ht = Z_ARRVAL_P(return_value);
    zval       tmp_arr;
    array_init(&tmp_arr);

    zval* entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (Z_TYPE_P(entry) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(entry)) == 2) {
            zval* z_member = zend_hash_index_find(Z_ARRVAL_P(entry), 0);
            zval* z_score  = zend_hash_index_find(Z_ARRVAL_P(entry), 1);

            if (z_member && z_score) {
                /* Convert member to string if needed for use as key */
                if (Z_TYPE_P(z_member) != IS_STRING) {
                    convert_to_string_ex(z_member);
                }

                /* Add to associative array: member => score */
                Z_TRY_ADDREF_P(z_score);

                add_assoc_zval(&tmp_arr, Z_STRVAL_P(z_member), z_score);
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    /* Replace the original array with our flattened array */
    zval_ptr_dtor(return_value);
    ZVAL_COPY_VALUE(return_value, &tmp_arr);

    return 1;
}


/* ====================================================================
 * COMMON EXECUTION FRAMEWORK IMPLEMENTATION
 * ==================================================================== */

/**
 * Generic Z-command execution framework with integrated batch support
 */
int execute_z_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              z_command_args_t*    args,
                              void*                result_ptr,
                              z_result_processor_t process_result,
                              zval*                return_value) {
    /* Check if valkey_glide object is valid */
    if (!valkey_glide) {
        return 0;
    }

    /* Prepare arguments ONCE - single switch statement eliminates duplication */
    uintptr_t*     arg_values        = NULL;
    unsigned long* arg_lens          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;

    /* Single argument preparation logic for both batch and normal modes */
    switch (cmd_type) {
        case ZCard:
            arg_count = prepare_z_key_args(args, &arg_values, &arg_lens);
            break;

        case ZScore:
        case ZRank:
        case ZRevRank:
            arg_count = prepare_z_member_args(args, &arg_values, &arg_lens);
            break;

        case ZCount:
        case ZLexCount:
        case ZRemRangeByScore:
        case ZRemRangeByLex:
            arg_count = prepare_z_range_args(args, &arg_values, &arg_lens);
            break;

        case ZRem:
        case ZMScore:
            allocated_strings = (char**) emalloc(args->member_count * sizeof(char*));
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_members_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZRange:
        case ZRevRange:
        case ZRangeByScore:
        case ZRangeByLex:
        case ZRevRangeByScore:
        case ZRevRangeByLex:
            allocated_strings =
                (char**) emalloc(10 * sizeof(char*)); /* Enough for typical options */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_complex_range_args(
                args, &arg_values, cmd_type, &arg_lens, &allocated_strings, &allocated_count);
            cmd_type = ZRange;
            break;

        case ZIncrBy:
            arg_count         = 3; /* key + increment + member */
            arg_values        = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
            arg_lens          = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
            allocated_strings = (char**) emalloc(1 * sizeof(char*));

            if (!arg_values || !arg_lens || !allocated_strings) {
                if (arg_values)
                    efree(arg_values);
                if (arg_lens)
                    efree(arg_lens);
                if (allocated_strings)
                    efree(allocated_strings);
                return 0;
            }

            /* Set arguments */
            arg_values[0] = (uintptr_t) args->key;
            arg_lens[0]   = args->key_len;

            /* Add increment parameter */
            char increment_str[64];
            int  increment_str_len =
                snprintf(increment_str, sizeof(increment_str), "%.6g", args->increment);
            char* increment_str_copy = estrndup(increment_str, increment_str_len);
            if (!increment_str_copy) {
                efree(arg_values);
                efree(arg_lens);
                efree(allocated_strings);
                return 0;
            }

            arg_values[1]        = (uintptr_t) increment_str_copy;
            arg_lens[1]          = increment_str_len;
            allocated_strings[0] = increment_str_copy;
            allocated_count      = 1;

            /* Add member parameter */
            arg_values[2] = (uintptr_t) args->member;
            arg_lens[2]   = args->member_len;
            break;

        case ZRemRangeByRank:
            arg_count         = 3; /* key + start + stop */
            arg_values        = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
            arg_lens          = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
            allocated_strings = (char**) emalloc(2 * sizeof(char*));

            if (!arg_values || !arg_lens || !allocated_strings) {
                if (arg_values)
                    efree(arg_values);
                if (arg_lens)
                    efree(arg_lens);
                if (allocated_strings)
                    efree(allocated_strings);
                return 0;
            }

            /* Set arguments */
            arg_values[0] = (uintptr_t) args->key;
            arg_lens[0]   = args->key_len;

            /* Add start and end parameters */
            char start_str[32], end_str[32];
            int  start_str_len = snprintf(start_str, sizeof(start_str), "%ld", args->start);
            int  end_str_len   = snprintf(end_str, sizeof(end_str), "%ld", args->end);

            char* start_str_copy = estrndup(start_str, start_str_len);
            char* end_str_copy   = estrndup(end_str, end_str_len);
            if (!start_str_copy || !end_str_copy) {
                if (start_str_copy)
                    efree(start_str_copy);
                efree(arg_values);
                efree(arg_lens);
                efree(allocated_strings);
                return 0;
            }

            arg_values[1]        = (uintptr_t) start_str_copy;
            arg_lens[1]          = start_str_len;
            allocated_strings[0] = start_str_copy;

            arg_values[2]        = (uintptr_t) end_str_copy;
            arg_lens[2]          = end_str_len;
            allocated_strings[1] = end_str_copy;
            allocated_count      = 2;
            break;

        case ZDiffStore:
        case ZInterStore:
        case ZUnionStore:
            allocated_strings =
                (char**) emalloc(20 * sizeof(char*)); /* Enough for store commands */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_store_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZInterCard:
            allocated_strings = (char**) emalloc(5 * sizeof(char*)); /* Enough for ZINTERCARD */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_intercard_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZUnion:
            allocated_strings = (char**) emalloc(20 * sizeof(char*)); /* Enough for ZUNION */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_union_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZPopMax:
        case ZPopMin:
            allocated_strings = (char**) emalloc(2 * sizeof(char*)); /* Enough for ZPOP commands */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_pop_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZRangeStore:
            allocated_strings = (char**) emalloc(10 * sizeof(char*)); /* Enough for ZRANGESTORE */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_rangestore_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZAdd:
            allocated_strings = (char**) emalloc(20 * sizeof(char*)); /* Enough for ZADD */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_zadd_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZDiff:
            allocated_strings = (char**) emalloc(5 * sizeof(char*)); /* Enough for ZDIFF */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_zdiff_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZInter:
            allocated_strings = (char**) emalloc(20 * sizeof(char*)); /* Enough for ZINTER */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_union_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case ZRandMember:
            allocated_strings = (char**) emalloc(2 * sizeof(char*)); /* Enough for ZRANDMEMBER */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_randmember_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case BZPopMax:
        case BZPopMin:
            allocated_strings = (char**) emalloc(2 * sizeof(char*)); /* Enough for BZPOP commands */
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_z_bzpop_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        default:
            /* Unsupported command type */
            return 0;
    }
    /* Check if argument preparation was successful */
    if (arg_count <= 0) {
        if (arg_values)
            efree(arg_values);
        if (arg_lens)
            efree(arg_lens);
        if (allocated_strings)
            efree(allocated_strings);
        return 0;
    }

    if (valkey_glide->is_in_batch_mode) {
        int result = buffer_command_for_batch(valkey_glide,
                                              cmd_type,
                                              arg_values,
                                              arg_lens,
                                              arg_count,

                                              result_ptr,
                                              process_result);

        if (arg_values)
            efree(arg_values);
        if (arg_lens)
            efree(arg_lens);

        return result;
    }
    /* Execute the command */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, arg_count, arg_values, arg_lens);

    /* Free allocated strings */
    int i;
    for (i = 0; i < allocated_count; i++) {
        if (allocated_strings[i]) {
            efree(allocated_strings[i]);
        }
    }
    if (allocated_strings)
        efree(allocated_strings);
    if (arg_values)
        efree(arg_values);
    if (arg_lens)
        efree(arg_lens);

    /* Check if the command was successful */
    if (!result) {
        return 0;
    }

    /* Check if there was an error */
    if (result->command_error) {
        free_command_result(result);
        return 0;
    }

    /* Process the result */
    int success = process_result(result->response, result_ptr, return_value);

    /* Free the result */
    free_command_result(result);

    return success;
}

/* ====================================================================
 * ARGUMENT PREPARATION UTILITIES IMPLEMENTATION
 * ==================================================================== */

/**
 * Prepare basic Z-command arguments (just key)
 */
int prepare_z_key_args(z_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out) {
    if (!args || !args->key || !args_out || !args_len_out) {
        return 0;
    }

    unsigned long arg_count = 1; /* just key */

    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    return arg_count;
}

int prepare_z_pop_args(z_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count) {
    if (!args || !args->key || !args_out || !args_len_out || !allocated_strings ||
        !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    unsigned long arg_count = 1;
    if (args->start > 1) {
        arg_count++;
    }

    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    if (args->start > 1) {
        char count_str[32];
        snprintf(count_str, sizeof(count_str), "%ld", args->start);
        char* count_str_copy = estrdup(count_str);
        if (!count_str_copy) {
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[1]                             = (uintptr_t) count_str_copy;
        (*args_len_out)[1]                         = strlen(count_str);
        (*allocated_strings)[(*allocated_count)++] = count_str_copy;
    }

    return arg_count;
}

/**
 * Prepare member-based Z-command arguments (key + member)
 */
int prepare_z_member_args(z_command_args_t* args,
                          uintptr_t**       args_out,
                          unsigned long**   args_len_out) {
    if (!args || !args->key || !args->member || !args_out || !args_len_out) {
        return 0;
    }

    unsigned long arg_count = 2; /* key + member */

    if (args->withscores) {
        arg_count++; /* Add WITHSCORE parameter */
    }

    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    (*args_out)[1]     = (uintptr_t) args->member;
    (*args_len_out)[1] = args->member_len;

    /* Add WITHSCORE if required */
    if (args->withscores) {
        const char* withscore_str = "WITHSCORE";
        (*args_out)[2]            = (uintptr_t) withscore_str;
        (*args_len_out)[2]        = 9; /* length of "WITHSCORE" */
    }

    return arg_count;
}

/**
 * Prepare range-based Z-command arguments (key + min + max)
 */
int prepare_z_range_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out) {
    if (!args || !args->key || !args->min || !args->max || !args_out || !args_len_out) {
        return 0;
    }

    unsigned long arg_count = 3; /* key + min + max */

    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    (*args_out)[1]     = (uintptr_t) args->min;
    (*args_len_out)[1] = args->min_len;

    (*args_out)[2]     = (uintptr_t) args->max;
    (*args_len_out)[2] = args->max_len;

    return arg_count;
}

/**
 * Prepare multi-member Z-command arguments (key + multiple members)
 */
int prepare_z_members_args(z_command_args_t* args,
                           uintptr_t**       args_out,
                           unsigned long**   args_len_out,
                           char***           allocated_strings,
                           int*              allocated_count) {
    if (!args || !args->key || !args->members || args->member_count <= 0 || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Prepare command arguments */
    unsigned long arg_count = 1 + args->member_count; /* key + members */

    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Add members as arguments */
    int i;
    for (i = 0; i < args->member_count; i++) {
        zval* z_member = &args->members[i];

        if (Z_TYPE_P(z_member) == IS_STRING) {
            (*args_out)[i + 1]     = (uintptr_t) Z_STRVAL_P(z_member);
            (*args_len_out)[i + 1] = Z_STRLEN_P(z_member);
        } else {
            /* Convert non-string values to string */
            char*  str_val   = NULL;
            size_t str_len   = 0;
            int    need_free = 0;

            str_val = zval_to_string_safe(z_member, &str_len, &need_free);

            if (!str_val) {
                int j;
                for (j = 0; j < *allocated_count; j++) {
                    efree((*allocated_strings)[j]);
                }
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }

            (*args_out)[i + 1]     = (uintptr_t) str_val;
            (*args_len_out)[i + 1] = str_len;

            if (need_free) {
                (*allocated_strings)[(*allocated_count)++] = str_val;
            }
        }
    }

    return arg_count;
}

/**
 * Convert a zval to a string argument
 */
static int convert_zval_to_string_arg(zval*          z_value,
                                      uintptr_t*     arg_ptr,
                                      unsigned long* arg_len_ptr,
                                      char***        allocated_strings,
                                      int*           allocated_count) {
    if (Z_TYPE_P(z_value) == IS_STRING) {
        *arg_ptr     = (uintptr_t) Z_STRVAL_P(z_value);
        *arg_len_ptr = Z_STRLEN_P(z_value);
        return 1;
    } else {
        /* Convert non-string values to string */
        char*  str_val   = NULL;
        size_t str_len   = 0;
        int    need_free = 0;

        str_val = zval_to_string_safe(z_value, &str_len, &need_free);

        if (!str_val) {
            return 0;
        }

        *arg_ptr     = (uintptr_t) str_val;
        *arg_len_ptr = str_len;

        if (need_free) {
            (*allocated_strings)[(*allocated_count)++] = str_val;
        }

        return 1;
    }
}

/**
 * Prepare complex range Z-command arguments with options
 */
int prepare_z_complex_range_args(z_command_args_t* args,
                                 uintptr_t**       args_out,
                                 enum RequestType  cmd_type,
                                 unsigned long**   args_len_out,
                                 char***           allocated_strings,
                                 int*              allocated_count) {
    if (!args || !args->key || !args->z_start || !args->z_end || !args_out || !args_len_out ||
        !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse range options */
    range_options_t range_opts = {0};
    if (!parse_range_options(args->options, &range_opts)) {
        return 0;
    }
    switch (cmd_type) {
        case ZRevRange:
            range_opts.rev = 1; /* Reverse order */
            break;
        case ZRangeByScore:
            range_opts.byscore = 1; /* By score */
            break;
        case ZRangeByLex:
            range_opts.bylex = 1; /* By lexicographical order */
            break;
        case ZRevRangeByScore:
            range_opts.byscore = 1; /* By score */
            range_opts.rev     = 1; /* Reverse order */
            break;
        case ZRevRangeByLex:
            range_opts.bylex = 1; /* By lexicographical order */
            range_opts.rev   = 1; /* Reverse order */
            break;
        default:
            break;
    }

    /* Calculate argument count based on options */
    unsigned long arg_count = 3; /* key + start + end */
    if (range_opts.withscores)
        arg_count++; /* Add WITHSCORES parameter */
    if (range_opts.byscore)
        arg_count++; /* Add BYSCORE parameter */
    if (range_opts.bylex)
        arg_count++; /* Add BYLEX parameter */
    if (range_opts.rev)
        arg_count++; /* Add REV parameter */
    if (range_opts.has_limit)
        arg_count += 3; /* Add LIMIT + offset + count parameters */

    /* Allocate memory for arguments */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Convert start and end to strings if needed */
    if (!convert_zval_to_string_arg(args->z_start,
                                    &((*args_out)[1]),
                                    &((*args_len_out)[1]),
                                    allocated_strings,
                                    allocated_count)) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }

    if (!convert_zval_to_string_arg(args->z_end,
                                    &((*args_out)[2]),
                                    &((*args_len_out)[2]),
                                    allocated_strings,
                                    allocated_count)) {
        int i;
        for (i = 0; i < *allocated_count; i++) {
            efree((*allocated_strings)[i]);
        }
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }

    /* Add optional parameters in the correct order */
    int arg_idx = 3; /* Start after key, start, end */

    /* Add BYSCORE parameter if required */
    if (range_opts.byscore) {
        const char* byscore_str  = "BYSCORE";
        (*args_out)[arg_idx]     = (uintptr_t) byscore_str;
        (*args_len_out)[arg_idx] = 7; /* length of "BYSCORE" */
        arg_idx++;
    }

    /* Add BYLEX parameter if required */
    if (range_opts.bylex) {
        const char* bylex_str    = "BYLEX";
        (*args_out)[arg_idx]     = (uintptr_t) bylex_str;
        (*args_len_out)[arg_idx] = 5; /* length of "BYLEX" */
        arg_idx++;
    }

    /* Add REV parameter if required */
    if (range_opts.rev) {
        const char* rev_str      = "REV";
        (*args_out)[arg_idx]     = (uintptr_t) rev_str;
        (*args_len_out)[arg_idx] = 3; /* length of "REV" */
        arg_idx++;
    }

    /* Add LIMIT parameter if required */
    if (range_opts.has_limit) {
        /* Add LIMIT + offset + count using common helper */
        arg_idx += create_limit_args(
            &range_opts, *args_out, *args_len_out, arg_idx, *allocated_strings, allocated_count);
    }

    /* Add WITHSCORES if required - add it last as per ValkeyGlide command syntax */
    if (range_opts.withscores) {
        const char* withscores_str = "WITHSCORES";
        (*args_out)[arg_idx]       = (uintptr_t) withscores_str;
        (*args_len_out)[arg_idx]   = 10; /* length of "WITHSCORES" */
        arg_idx++;
    }

    return arg_idx; /* Return actual number of arguments used */
}

/**
 * Prepare store command arguments (destination + numkeys + keys + weights + aggregate)
 */
int prepare_z_store_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count) {
    if (!args || !args->key || !args->members || args->member_count <= 0 || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse store options */
    store_options_t store_opts = {0};
    parse_store_options(args->weights, args->options, &store_opts);

    /* Calculate total arguments (destination + numkeys + keys + optional WEIGHTS + optional
     * AGGREGATE) */
    unsigned long arg_count     = 2 + args->member_count; /* destination + numkeys + keys */
    int           weights_count = 0;

    if (store_opts.has_weights) {
        weights_count = zend_hash_num_elements(Z_ARRVAL_P(store_opts.weights));
        arg_count += 1 + weights_count; /* WEIGHTS + values */
    }

    if (store_opts.has_aggregate) {
        arg_count += 2; /* AGGREGATE + value */
    }

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set destination */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Add numkeys as the second argument */
    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%d", args->member_count);
    char* numkeys_str_copy = estrdup(numkeys_str);
    if (!numkeys_str_copy) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[1]                             = (uintptr_t) numkeys_str_copy;
    (*args_len_out)[1]                         = strlen(numkeys_str);
    (*allocated_strings)[(*allocated_count)++] = numkeys_str_copy;

    /* Add keys starting from index 2 */
    HashTable* keys_hash = Z_ARRVAL_P(args->members); /* members field is reused for keys */
    zval*      key;
    int        idx = 2;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        if (Z_TYPE_P(key) != IS_STRING) {
            convert_to_string(key);
        }
        (*args_out)[idx]     = (uintptr_t) Z_STRVAL_P(key);
        (*args_len_out)[idx] = Z_STRLEN_P(key);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    unsigned int offset = 2 + args->member_count;

    /* Add WEIGHTS if present */
    if (store_opts.has_weights) {
        (*args_out)[offset]     = (uintptr_t) "WEIGHTS";
        (*args_len_out)[offset] = 7;
        offset++;

        /* Add weights values using framework helper */
        HashTable* weights_hash = Z_ARRVAL_P(store_opts.weights);
        zval*      weight;
        ZEND_HASH_FOREACH_VAL(weights_hash, weight) {
            char*  weight_str = NULL;
            size_t weight_len = 0;
            int    need_free  = 0;

            weight_str = zval_to_string_safe(weight, &weight_len, &need_free);

            if (!weight_str) {
                /* Cleanup on error */
                int j;
                for (j = 0; j < *allocated_count; j++) {
                    efree((*allocated_strings)[j]);
                }
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }

            (*args_out)[offset]     = (uintptr_t) weight_str;
            (*args_len_out)[offset] = weight_len;

            if (need_free) {
                (*allocated_strings)[(*allocated_count)++] = weight_str;
            }

            offset++;
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Add AGGREGATE if present */
    if (store_opts.has_aggregate) {
        (*args_out)[offset]     = (uintptr_t) "AGGREGATE";
        (*args_len_out)[offset] = 9;
        offset++;

        const char* agg_str      = Z_STRVAL_P(store_opts.aggregate);
        char*       agg_str_copy = estrdup(agg_str);
        if (!agg_str_copy) {
            /* Cleanup on error */
            int j;
            for (j = 0; j < *allocated_count; j++) {
                efree((*allocated_strings)[j]);
            }
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[offset]                        = (uintptr_t) agg_str_copy;
        (*args_len_out)[offset]                    = Z_STRLEN_P(store_opts.aggregate);
        (*allocated_strings)[(*allocated_count)++] = agg_str_copy;
    }

    return arg_count;
}

/**
 * Prepare ZINTERCARD command arguments (numkeys + keys + optional LIMIT)
 */
int prepare_z_intercard_args(z_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count) {
    if (!args || !args->members || args->member_count <= 0 || !args_out || !args_len_out ||
        !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Calculate total arguments (numkeys + keys + LIMIT if present) */
    unsigned long arg_count = 1 + args->member_count; /* +1 for numkeys */
    int           has_limit = 0;
    long          limit     = 0;

    if (args->options && Z_TYPE_P(args->options) == IS_ARRAY) {
        HashTable* ht        = Z_ARRVAL_P(args->options);
        zval*      limit_val = zend_hash_str_find(ht, "LIMIT", sizeof("LIMIT") - 1);
        if (limit_val && Z_TYPE_P(limit_val) == IS_LONG) {
            has_limit = 1;
            limit     = Z_LVAL_P(limit_val);
            arg_count += 2; /* LIMIT + value */
        }
    }

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Add numkeys as the first argument */
    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%d", args->member_count);
    char* numkeys_str_copy = estrdup(numkeys_str);
    if (!numkeys_str_copy) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[0]                             = (uintptr_t) numkeys_str_copy;
    (*args_len_out)[0]                         = strlen(numkeys_str);
    (*allocated_strings)[(*allocated_count)++] = numkeys_str_copy;

    /* Add keys starting from index 1 */
    HashTable* keys_hash = Z_ARRVAL_P(args->members); /* members field is reused for keys */
    zval*      key;
    int        idx = 1;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        if (Z_TYPE_P(key) != IS_STRING) {
            convert_to_string(key);
        }
        (*args_out)[idx]     = (uintptr_t) Z_STRVAL_P(key);
        (*args_len_out)[idx] = Z_STRLEN_P(key);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add LIMIT option if present */
    if (has_limit) {
        unsigned int offset = 1 + args->member_count; /* +1 for numkeys */

        /* Add LIMIT keyword */
        (*args_out)[offset]     = (uintptr_t) "LIMIT";
        (*args_len_out)[offset] = 5;
        offset++;

        /* Add limit value */
        char limit_str[32];
        snprintf(limit_str, sizeof(limit_str), "%ld", limit);
        char* limit_str_copy = estrdup(limit_str);
        if (!limit_str_copy) {
            /* Cleanup on error */
            int j;
            for (j = 0; j < *allocated_count; j++) {
                efree((*allocated_strings)[j]);
            }
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[offset]                        = (uintptr_t) limit_str_copy;
        (*args_len_out)[offset]                    = strlen(limit_str);
        (*allocated_strings)[(*allocated_count)++] = limit_str_copy;
    }

    return arg_count;
}

/**
 * Prepare ZUNION command arguments (numkeys + keys + WEIGHTS + AGGREGATE + WITHSCORES if present)
 */
int prepare_z_union_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count) {
    if (!args || !args->members || args->member_count <= 0 || !args_out || !args_len_out ||
        !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse union options */
    store_options_t union_opts = {0};
    parse_store_options(args->weights, args->options, &union_opts);

    /* Calculate total arguments (numkeys + keys + WEIGHTS + AGGREGATE + WITHSCORES if
     * present) */
    unsigned long arg_count     = 1 + args->member_count; /* +1 for numkeys */
    int           weights_count = 0;

    if (union_opts.has_weights) {
        weights_count = zend_hash_num_elements(Z_ARRVAL_P(union_opts.weights));
        arg_count += 1 + weights_count; /* WEIGHTS + values */
    }

    if (union_opts.has_aggregate) {
        arg_count += 2; /* AGGREGATE + value */
    }

    if (union_opts.withscores) {
        arg_count += 1; /* WITHSCORES */
    }

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Add numkeys as the first argument */
    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%d", args->member_count);
    char* numkeys_str_copy = estrdup(numkeys_str);
    if (!numkeys_str_copy) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[0]                             = (uintptr_t) numkeys_str_copy;
    (*args_len_out)[0]                         = strlen(numkeys_str);
    (*allocated_strings)[(*allocated_count)++] = numkeys_str_copy;

    /* Add keys starting from index 1 */
    HashTable*   keys_hash = Z_ARRVAL_P(args->members); /* members field is reused for keys */
    zval*        key;
    unsigned int offset = 1;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        if (Z_TYPE_P(key) != IS_STRING) {
            convert_to_string(key);
        }
        (*args_out)[offset]     = (uintptr_t) Z_STRVAL_P(key);
        (*args_len_out)[offset] = Z_STRLEN_P(key);
        offset++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add WEIGHTS if present */
    if (union_opts.has_weights) {
        /* Add WEIGHTS keyword */
        (*args_out)[offset]     = (uintptr_t) "WEIGHTS";
        (*args_len_out)[offset] = 7;
        offset++;

        /* Add weights values using framework helper */
        HashTable* weights_hash = Z_ARRVAL_P(union_opts.weights);
        zval*      weight;
        ZEND_HASH_FOREACH_VAL(weights_hash, weight) {
            char*  weight_str = NULL;
            size_t weight_len = 0;
            int    need_free  = 0;

            weight_str = zval_to_string_safe(weight, &weight_len, &need_free);

            if (!weight_str) {
                /* Cleanup on error */
                int j;
                for (j = 0; j < *allocated_count; j++) {
                    efree((*allocated_strings)[j]);
                }
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }

            (*args_out)[offset]     = (uintptr_t) weight_str;
            (*args_len_out)[offset] = weight_len;

            if (need_free) {
                (*allocated_strings)[(*allocated_count)++] = weight_str;
            }

            offset++;
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Add AGGREGATE if present */
    if (union_opts.has_aggregate) {
        /* Add AGGREGATE keyword */
        (*args_out)[offset]     = (uintptr_t) "AGGREGATE";
        (*args_len_out)[offset] = 9;
        offset++;

        /* Add aggregate value */
        const char* agg_str      = Z_STRVAL_P(union_opts.aggregate);
        char*       agg_str_copy = estrdup(agg_str);
        if (!agg_str_copy) {
            /* Cleanup on error */
            int j;
            for (j = 0; j < *allocated_count; j++) {
                efree((*allocated_strings)[j]);
            }
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[offset]                        = (uintptr_t) agg_str_copy;
        (*args_len_out)[offset]                    = Z_STRLEN_P(union_opts.aggregate);
        (*allocated_strings)[(*allocated_count)++] = agg_str_copy;
        offset++;
    }

    /* Add WITHSCORES if present */
    if (union_opts.withscores) {
        /* Add WITHSCORES keyword */
        (*args_out)[offset]     = (uintptr_t) "WITHSCORES";
        (*args_len_out)[offset] = 10;
        offset++;
    }

    return arg_count;
}

/**
 * Prepare ZRANGESTORE command arguments (dst + src + start + end + range options)
 */
int prepare_z_rangestore_args(z_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out,
                              char***           allocated_strings,
                              int*              allocated_count) {
    if (!args || !args->key || !args->member || !args->z_start || !args->z_end || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse range options */
    range_options_t range_opts = {0};
    parse_range_options(args->options, &range_opts);

    /* Calculate total arguments: dst + src + start + end + range options */
    unsigned long arg_count = 4; /* dst + src + start + end */
    if (range_opts.byscore)
        arg_count++;
    if (range_opts.bylex)
        arg_count++;
    if (range_opts.rev)
        arg_count++;
    if (range_opts.has_limit)
        arg_count += 3; /* LIMIT + offset + count */

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set dst and src (args->key is dst, args->member is src) */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->member;
    (*args_len_out)[1] = args->member_len;

    /* Add start and end using framework helper */
    int    need_free = 0;
    size_t len       = 0;
    char*  str       = zval_to_string_safe(args->z_start, &len, &need_free);
    if (!str) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[2]     = (uintptr_t) str;
    (*args_len_out)[2] = len;
    if (need_free)
        (*allocated_strings)[(*allocated_count)++] = str;

    str = zval_to_string_safe(args->z_end, &len, &need_free);
    if (!str) {
        free_allocated_strings(*allocated_strings, *allocated_count);
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[3]     = (uintptr_t) str;
    (*args_len_out)[3] = len;
    if (need_free)
        (*allocated_strings)[(*allocated_count)++] = str;

    /* Add range options */
    unsigned int offset = 4;
    if (range_opts.bylex) {
        (*args_out)[offset]     = (uintptr_t) "BYLEX";
        (*args_len_out)[offset] = 5;
        offset++;
    } else if (range_opts.byscore) {
        (*args_out)[offset]     = (uintptr_t) "BYSCORE";
        (*args_len_out)[offset] = 7;
        offset++;
    }
    if (range_opts.rev) {
        (*args_out)[offset]     = (uintptr_t) "REV";
        (*args_len_out)[offset] = 3;
        offset++;
    }
    if (range_opts.has_limit) {
        offset += create_limit_args(
            &range_opts, *args_out, *args_len_out, offset, *allocated_strings, allocated_count);
    }

    return arg_count;
}

/**
 * Prepare ZADD command arguments (key + options + score-member pairs)
 */
int prepare_z_zadd_args(z_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count) {
    if (!args || !args->key || !args->members || args->member_count < 2 || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse ZADD options from the first element if it's an array */
    zadd_options_t zadd_opts       = {0};
    int            first_score_idx = 0;

    if (args->member_count > 0 && Z_TYPE(args->members[0]) == IS_ARRAY) {
        parse_zadd_options(&args->members[0], &zadd_opts);
        first_score_idx = 1;
    }

    /* Calculate score-member pairs */
    int remaining_args = args->member_count - first_score_idx;
    if (remaining_args < 2 || remaining_args % 2 != 0) {
        return 0; /* Must have pairs */
    }
    int score_member_pairs = remaining_args / 2;

    /* When INCR option is used, we can only have one score-member pair */
    if (zadd_opts.incr && score_member_pairs > 1) {
        return 0;
    }

    /* Calculate number of option arguments */
    int num_options =
        zadd_opts.xx + zadd_opts.nx + zadd_opts.lt + zadd_opts.gt + zadd_opts.ch + zadd_opts.incr;

    /* Total arguments: key + options + score-member pairs */
    unsigned long arg_count = 1 + num_options + (score_member_pairs * 2);

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    int arg_idx        = 1;

    /* Add options using existing framework pattern */
    if (zadd_opts.xx) {
        (*args_out)[arg_idx]       = (uintptr_t) "XX";
        (*args_len_out)[arg_idx++] = 2;
    }
    if (zadd_opts.nx) {
        (*args_out)[arg_idx]       = (uintptr_t) "NX";
        (*args_len_out)[arg_idx++] = 2;
    }
    if (zadd_opts.lt) {
        (*args_out)[arg_idx]       = (uintptr_t) "LT";
        (*args_len_out)[arg_idx++] = 2;
    }
    if (zadd_opts.gt) {
        (*args_out)[arg_idx]       = (uintptr_t) "GT";
        (*args_len_out)[arg_idx++] = 2;
    }
    if (zadd_opts.ch) {
        (*args_out)[arg_idx]       = (uintptr_t) "CH";
        (*args_len_out)[arg_idx++] = 2;
    }
    if (zadd_opts.incr) {
        (*args_out)[arg_idx]       = (uintptr_t) "INCR";
        (*args_len_out)[arg_idx++] = 4;
    }

    /* Add score-member pairs using existing framework helpers */
    for (int i = first_score_idx; i < args->member_count; i += 2) {
        /* Score - use existing zval_to_string_safe */
        zval*  score     = &args->members[i];
        char*  score_str = NULL;
        size_t score_len = 0;
        int    need_free = 0;

        if (Z_TYPE_P(score) == IS_DOUBLE) {
            score_str = double_to_string(Z_DVAL_P(score), &score_len);
            need_free = 1;
        } else if (Z_TYPE_P(score) == IS_LONG) {
            score_str = long_to_string(Z_LVAL_P(score), &score_len);
            need_free = 1;
        } else if (Z_TYPE_P(score) == IS_STRING) {
            score_str = Z_STRVAL_P(score);
            score_len = Z_STRLEN_P(score);
        } else {
            /* Cleanup and return error */
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]       = (uintptr_t) score_str;
        (*args_len_out)[arg_idx++] = score_len;
        if (need_free) {
            (*allocated_strings)[(*allocated_count)++] = score_str;
        }

        /* Member - validate it's a string */
        zval* member = &args->members[i + 1];
        if (Z_TYPE_P(member) != IS_STRING) {
            /* Cleanup and return error */
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]       = (uintptr_t) Z_STRVAL_P(member);
        (*args_len_out)[arg_idx++] = Z_STRLEN_P(member);
    }

    return arg_count;
}

/**
 * Prepare ZDIFF command arguments (numkeys + keys + optional WITHSCORES)
 */
int prepare_z_zdiff_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count) {
    if (!args || !args->members || args->member_count <= 0 || !args_out || !args_len_out ||
        !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Parse ZDIFF options (only WITHSCORES supported) */
    store_options_t zdiff_opts = {0};
    parse_store_options(NULL, args->options, &zdiff_opts);

    /* Calculate total arguments (numkeys + keys + WITHSCORES if present) */
    unsigned long arg_count = 1 + args->member_count; /* +1 for numkeys */
    if (zdiff_opts.withscores) {
        arg_count += 1; /* WITHSCORES */
    }

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Add numkeys as the first argument */
    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%d", args->member_count);
    char* numkeys_str_copy = estrdup(numkeys_str);
    if (!numkeys_str_copy) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[0]                             = (uintptr_t) numkeys_str_copy;
    (*args_len_out)[0]                         = strlen(numkeys_str);
    (*allocated_strings)[(*allocated_count)++] = numkeys_str_copy;

    /* Add keys starting from index 1 */
    HashTable*   keys_hash = Z_ARRVAL_P(args->members); /* members field is reused for keys */
    zval*        key;
    unsigned int offset = 1;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        if (Z_TYPE_P(key) != IS_STRING) {
            convert_to_string(key);
        }
        (*args_out)[offset]     = (uintptr_t) Z_STRVAL_P(key);
        (*args_len_out)[offset] = Z_STRLEN_P(key);
        offset++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add WITHSCORES if present */
    if (zdiff_opts.withscores) {
        /* Add WITHSCORES keyword */
        (*args_out)[offset]     = (uintptr_t) "WITHSCORES";
        (*args_len_out)[offset] = 10;
        offset++;
    }

    return arg_count;
}

/**
 * Prepare ZRANDMEMBER command arguments (key + optional count + optional WITHSCORES)
 */
int prepare_z_randmember_args(z_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out,
                              char***           allocated_strings,
                              int*              allocated_count) {
    if (!args || !args->key || !args_out || !args_len_out || !allocated_strings ||
        !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Calculate argument count: key + optional count + optional WITHSCORES */
    unsigned long arg_count = 1; /* key */
    if (args->start != 1)        /* reuse start field for count */
    {
        arg_count++; /* count */
    }
    if (args->withscores) {
        arg_count++; /* WITHSCORES */
    }

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    int arg_idx        = 1;

    /* Add count if not default (1) */
    if (args->start != 1) {
        char count_str[32];
        snprintf(count_str, sizeof(count_str), "%ld", args->start);
        char* count_str_copy = estrdup(count_str);
        if (!count_str_copy) {
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]                       = (uintptr_t) count_str_copy;
        (*args_len_out)[arg_idx]                   = strlen(count_str);
        (*allocated_strings)[(*allocated_count)++] = count_str_copy;
        arg_idx++;
    }

    /* Add WITHSCORES if present */
    if (args->withscores) {
        (*args_out)[arg_idx]     = (uintptr_t) "WITHSCORES";
        (*args_len_out)[arg_idx] = 10; /* length of "WITHSCORES" */
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare BZPOP command arguments (timeout + keys)
 * Handles both array format (single zval containing array) and variadic format (array of zvals)
 */
int prepare_z_bzpop_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count) {
    if (!args || !args->members || args->member_count <= 0 || !args_out || !args_len_out ||
        !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Detect format: array format vs variadic format */
    int        actual_key_count = 0;
    zval*      keys_array       = NULL;
    int        is_array_format  = 0;
    HashTable* keys_ht          = NULL;

    /* Check if we have array format (single zval containing an array) */
    if (Z_TYPE_P(&args->members[0]) == IS_ARRAY) {
        /* Array format: args->members[0] contains the array of keys */
        is_array_format  = 1;
        keys_array       = &args->members[0];
        keys_ht          = Z_ARRVAL_P(keys_array);
        actual_key_count = zend_hash_num_elements(keys_ht);
    } else {
        /* Variadic format: args->members is array of individual key zvals */
        is_array_format  = 0;
        actual_key_count = args->member_count;
    }

    if (actual_key_count <= 0) {
        return 0;
    }

    /* Calculate argument count: keys + timeout */
    unsigned long arg_count = actual_key_count + 1; /* keys + timeout */

    /* Allocate final args arrays */
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Add keys as arguments based on format */
    if (is_array_format) {
        /* Array format: iterate through the array */
        int   idx = 0;
        zval* key_entry;
        ZEND_HASH_FOREACH_VAL(keys_ht, key_entry) {
            if (Z_TYPE_P(key_entry) != IS_STRING) {
                /* Convert to string if needed */
                convert_to_string(key_entry);
            }
            (*args_out)[idx]     = (uintptr_t) Z_STRVAL_P(key_entry);
            (*args_len_out)[idx] = Z_STRLEN_P(key_entry);
            idx++;
        }
        ZEND_HASH_FOREACH_END();
    } else {
        /* Variadic format: iterate through individual zvals */
        int i;
        for (i = 0; i < actual_key_count; i++) {
            zval* key = &args->members[i];
            if (Z_TYPE_P(key) != IS_STRING) {
                /* Convert to string if needed */
                convert_to_string(key);
            }
            (*args_out)[i]     = (uintptr_t) Z_STRVAL_P(key);
            (*args_len_out)[i] = Z_STRLEN_P(key);
        }
    }

    /* Add timeout as the last argument (reuse increment field for timeout) */
    size_t timeout_len;
    char*  timeout_str = double_to_string(args->increment, &timeout_len);
    if (!timeout_str) {
        efree(*args_out);
        efree(*args_len_out);
        return 0;
    }
    (*args_out)[actual_key_count]              = (uintptr_t) timeout_str;
    (*args_len_out)[actual_key_count]          = timeout_len;
    (*allocated_strings)[(*allocated_count)++] = timeout_str;

    return arg_count;
}

/* ====================================================================
 * RESULT PROCESSING FUNCTIONS
 * ===================================================================== */

/**
 * Process integer result (for commands returning count)
 */
int process_z_int_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        return 0;
    }

    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 0;
}

/**
 * Process double result (for commands returning scores)
 */
int process_z_double_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        return 0;
    }

    if (response->response_type == String) {
        /* Parse string as double */
        char*  endptr;
        double output_value = strtod(response->string_value, &endptr);
        if (*endptr == '\0' || endptr == response->string_value + response->string_value_len) {
            ZVAL_DOUBLE(return_value, output_value);
            return 1;
        }
        return 0;
    }

    if (response->response_type == Float) {
        double output_value = response->float_value;
        ZVAL_DOUBLE(return_value, output_value);
        return 1;
    }

    return 0;
}

/**
 * Process rank result with optional score
 */
int process_z_rank_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        return -1;
    }

    if (response->response_type == Null) {
        ZVAL_NULL(return_value); /* Member doesn't exist */
        return 0;                /* Member doesn't exist */
    }

    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    }


    return -1;
}

int process_z_array_zrand_result(CommandResponse* response, void* output, zval* return_value) {
    struct {
        int withscores;
    }* array_data = output;
    if (!response || !array_data || !return_value) {
        return 0;
    }

    /* Process the result */

    int success =
        command_response_to_zval(response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);

    if (Z_TYPE_P(return_value) == IS_STRING) {
        // Save the string temporarily
        zval tmp;

        ZVAL_COPY(&tmp, return_value);

        // Convert return_value to an array
        array_init(return_value);

        // Add the original string as the first element (index 0)
        add_next_index_zval(return_value, &tmp);
    }

    if (array_data->withscores && success && Z_TYPE_P(return_value) == IS_ARRAY) {
        /* Use common helper to flatten withscores array */
        flatten_withscores_array(return_value);
    }
    efree(output);
    return success;
}

/**
 * Process array result (for commands returning arrays)
 */
int process_z_array_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }
    /* Initialize return array */
    array_init(return_value);
    /* Process the result */
    int success = command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, true);

    return success;
}

/**
 * Process integer result and set as ZVAL_LONG (for commands like ZINTERCARD)
 */
int process_z_long_to_zval_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Process BZPOP result (for BZPOPMAX/BZPOPMIN commands)
 */
int process_z_bzpop_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }
    if (response->response_type == Null) {
        /* Timeout occurred, return false */
        ZVAL_FALSE(return_value);
        return 1;
    } else if (response->response_type == Array) {
        /* For BZPOP commands, need to manually ensure the score is a string */
        if (response->array_value_len == 3 && response->array_value[2].response_type != String) {
            /* Convert the response array to PHP array */
            int success = command_response_to_zval(
                response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);

            /* Get the score element (should be index 2) */
            zval*      score = NULL;
            zval*      arr   = return_value;
            HashTable* ht    = Z_ARRVAL_P(arr);

            /* Convert numeric score to string */
            if (ht && zend_hash_index_exists(ht, 2)) {
                score = zend_hash_index_find(ht, 2);
                if (score && (Z_TYPE_P(score) == IS_LONG || Z_TYPE_P(score) == IS_DOUBLE)) {
                    convert_to_string(score);
                }
            }
            return success;
        } else {
            /* Regular array conversion */
            return command_response_to_zval(
                response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        }
    }

    return 0;
}
