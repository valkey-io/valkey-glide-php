/*
  +----------------------------------------------------------------------+
  | ValkeyGlide Glide List Commands Common Framework                           |
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
#include "valkey_glide_list_common.h"

#include "common.h"
#include "valkey_glide_z_common.h"
extern zend_class_entry* ce;
extern zend_class_entry* get_valkey_glide_exception_ce();

/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/**
 * Allocate command arguments arrays
 */
int allocate_list_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out) {
    *args_out     = (uintptr_t*) emalloc(count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(count * sizeof(unsigned long));

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    return 1;
}

/**
 * Free command arguments arrays
 */
void free_list_command_args(uintptr_t* args, unsigned long* args_len) {
    if (args)
        efree(args);
    if (args_len)
        efree(args_len);
}

int process_list_ok_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    if (response->response_type == Ok) {
        ZVAL_TRUE(return_value);
        return 1;
    }

    ZVAL_FALSE(return_value);
    return 0;
}

int process_list_int_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_LONG(return_value, 0);
        return 0;
    }

    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    } else if (response->response_type == Null) {
        ZVAL_NULL(return_value);
        return 1;
    }
    return 0;
}

/**
 * Batch-compatible wrapper for string responses
 */
int process_list_string_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response)
        return 0;

    if (response->response_type == String) {
        ZVAL_STRINGL(return_value, response->string_value, response->string_value_len);
        return 1;
    } else if (response->response_type == Null) {
        ZVAL_FALSE(return_value);
        return 1;
    }
    return 0;
}

/**
 * Batch-compatible wrapper for array responses
 */
int process_list_array_result_async(CommandResponse* response, void* output, zval* return_value) {
    array_init(return_value);

    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Batch-compatible wrapper for boolean responses
 */
int process_list_bool_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response)
        return 0;

    if (response->response_type == Bool) {
        ZVAL_BOOL(return_value, response->bool_value);
        return 1;
    } else if (response->response_type == Ok) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    return 0;
}

/**
 * Batch-compatible wrapper for pop result responses (handles both single values and arrays)
 */
int process_list_pop_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        return 0;
    }

    if (response->response_type == String) {
        /* Single value returned */
        ZVAL_STRINGL(return_value, response->string_value, response->string_value_len);
        return 1;
    } else if (response->response_type == Array) {
        /* Multiple values returned (when count > 1) */
        return command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    } else if (response->response_type == Null) {
        /* No elements in the list */
        ZVAL_FALSE(return_value);
        return 1;
    }

    return 0;
}

/**
 * Batch-compatible wrapper for blocking result responses
 */
int process_list_blocking_result_async(CommandResponse* response,
                                       void*            output,
                                       zval*            return_value) {
    if (!response) {
        return 0;
    }

    if (response->response_type == Array) {
        /* Got a result, convert to PHP array [key, value] */
        return command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    } else if (response->response_type == Null) {
        /* Timeout reached, no elements available */
        ZVAL_NULL(return_value);
        return 1;
    }

    return 0;
}

/**
 * Batch-compatible wrapper for MPOP result responses
 */
int process_list_mpop_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_FALSE(return_value);
        return 1; /* Return success but with FALSE value when no results */
    }

    /* Handle NULL response (empty list or list doesn't exist) */
    if (response->response_type == Null) {
        ZVAL_FALSE(return_value);
        return 1; /* Return success but with FALSE value */
    }

    /* MPOP returns associative array format for the values */
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/* ====================================================================
 * GENERIC COMMAND EXECUTION FRAMEWORK
 * ==================================================================== */

/**
 * Generic command execution framework with batch support
 */
int execute_list_generic_command(valkey_glide_object* valkey_glide,
                                 enum RequestType     cmd_type,
                                 list_command_args_t* args,
                                 void*                result_ptr,
                                 z_result_processor_t process_result,
                                 zval*                return_value) {
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            status            = 0;

    /* Validate basic arguments */
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Prepare arguments based on command type */
    switch (cmd_type) {
        case LLen:
            arg_count = prepare_list_key_only_args(args, &cmd_args, &args_len);
            break;
        case LPush:
        case RPush:
        case LPushX:
        case RPushX:
            arg_count = prepare_list_key_values_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LPop:
        case RPop:
            arg_count = prepare_list_key_count_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case BLPop:
        case BRPop:
            arg_count = prepare_list_blocking_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LRange:
            arg_count = prepare_list_range_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LPos:
            arg_count = prepare_list_position_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LInsert:
            arg_count = prepare_list_insert_args(args, &cmd_args, &args_len);
            break;
        case LIndex:
        case LSet:
            arg_count = prepare_list_index_set_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LRem:
            arg_count = prepare_list_rem_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LTrim:
            arg_count = prepare_list_trim_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LMove:
        case BLMove:
        case RPopLPush:
        case BRPopLPush:
            arg_count = prepare_list_move_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case LMPop:
        case BLMPop:
            arg_count = prepare_list_mpop_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        default:
            return 0;
    }

    if (arg_count <= 0) {
        goto cleanup;
    }

    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* For batch mode, we need to use batch-compatible result processors */


        status = buffer_command_for_batch(
            valkey_glide, cmd_type, cmd_args, args_len, arg_count, result_ptr, process_result);

        goto cleanup;
    }

    /* Execute the command */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, arg_count, cmd_args, args_len);

    /* Process result */
    if (result) {
        if (!result->command_error && result->response && process_result) {
            status = process_result(result->response, result_ptr, return_value);
        }
        free_command_result(result);
    }

cleanup:
    /* Free allocated strings */
    FREE_LIST_ALLOCATED_STRINGS(allocated_strings, allocated_count);

    /* Free command arguments */
    free_list_command_args(cmd_args, args_len);

    return status;
}

/**
 * Allocate a string representation of a long integer
 */
char* alloc_list_number_string(long value, size_t* len_out) {
    char   temp[32];
    size_t len    = snprintf(temp, sizeof(temp), "%ld", value);
    char*  result = emalloc(len + 1);
    if (result) {
        memcpy(result, temp, len);
        result[len] = '\0';
        if (len_out)
            *len_out = len;
    }
    return result;
}

/**
 * Allocate a string representation of a double
 */
char* alloc_list_double_string(double value, size_t* len_out) {
    char   temp[64];
    size_t len    = snprintf(temp, sizeof(temp), "%.6f", value);
    char*  result = emalloc(len + 1);
    if (result) {
        memcpy(result, temp, len);
        result[len] = '\0';
        if (len_out)
            *len_out = len;
    }
    return result;
}

/* ====================================================================
 * OPTION PARSING FUNCTIONS
 * ==================================================================== */

/**
 * Parse position command options (for LPOS)
 */
int parse_list_position_options(zval* options, list_position_options_t* opts) {
    /* Initialize options to default values */
    opts->position     = NULL;
    opts->position_len = 0;
    opts->pivot        = NULL;
    opts->pivot_len    = 0;
    opts->rank         = 0;
    opts->count        = 0;
    opts->maxlen       = 0;
    opts->has_rank     = 0;
    opts->has_count    = 0;
    opts->has_maxlen   = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_rank, *z_count, *z_maxlen;

    /* Check for RANK option */
    if ((z_rank = zend_hash_str_find(ht, "rank", sizeof("rank") - 1)) != NULL) {
        if (Z_TYPE_P(z_rank) == IS_LONG) {
            opts->rank     = Z_LVAL_P(z_rank);
            opts->has_rank = 1;
        }
    }

    /* Check for COUNT option */
    if ((z_count = zend_hash_str_find(ht, "count", sizeof("count") - 1)) != NULL) {
        if (Z_TYPE_P(z_count) == IS_LONG) {
            opts->count     = Z_LVAL_P(z_count);
            opts->has_count = 1;
        }
    }

    /* Check for MAXLEN option */
    if ((z_maxlen = zend_hash_str_find(ht, "maxlen", sizeof("maxlen") - 1)) != NULL) {
        if (Z_TYPE_P(z_maxlen) == IS_LONG) {
            opts->maxlen     = Z_LVAL_P(z_maxlen);
            opts->has_maxlen = 1;
        }
    }

    return 1;
}


/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */

/**
 * Prepare arguments for key-only commands (LLEN)
 */
int prepare_list_key_only_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!allocate_list_command_args(1, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    return 1;
}

/**
 * Prepare arguments for key+values commands (LPUSH, RPUSH, etc.)
 */
int prepare_list_key_values_args(list_command_args_t* args,
                                 uintptr_t**          args_out,
                                 unsigned long**      args_len_out,
                                 char***              allocated_strings,
                                 int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);
    VALIDATE_LIST_VALUES(args->values, args->value_count);

    /* Handle both simple arrays and nested arrays (like RPUSH) */
    unsigned long total_args = 1; /* Start with 1 for the key */
    int           i;

    /* Count all items, including those in nested arrays */
    for (i = 0; i < args->value_count; i++) {
        zval* value = &args->values[i];
        if (Z_TYPE_P(value) == IS_STRING || Z_TYPE_P(value) == IS_LONG ||
            Z_TYPE_P(value) == IS_DOUBLE) {
            total_args++;
        } else if (Z_TYPE_P(value) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(value);
            total_args += zend_hash_num_elements(ht);
        } else {
            return 0;
        }
    }

    if (!allocate_list_command_args(total_args, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(total_args * sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Process all values and add to args array */
    unsigned long arg_idx = 1;

    for (i = 0; i < args->value_count; i++) {
        zval* value = &args->values[i];

        if (Z_TYPE_P(value) == IS_STRING) {
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(value);
            (*args_len_out)[arg_idx] = Z_STRLEN_P(value);
            arg_idx++;
        } else if (Z_TYPE_P(value) == IS_LONG) {
            /* Convert long to string */
            size_t str_len;
            char*  str_val = alloc_list_number_string(Z_LVAL_P(value), &str_len);
            if (!str_val) {
                FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                free_list_command_args(*args_out, *args_len_out);
                return 0;
            }

            (*allocated_strings)[*allocated_count] = str_val;
            (*allocated_count)++;

            (*args_out)[arg_idx]     = (uintptr_t) str_val;
            (*args_len_out)[arg_idx] = str_len;
            arg_idx++;
        } else if (Z_TYPE_P(value) == IS_DOUBLE) {
            /* Convert double to string */
            size_t str_len;
            char*  str_val = alloc_list_double_string(Z_DVAL_P(value), &str_len);
            if (!str_val) {
                FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                free_list_command_args(*args_out, *args_len_out);
                return 0;
            }

            (*allocated_strings)[*allocated_count] = str_val;
            (*allocated_count)++;

            (*args_out)[arg_idx]     = (uintptr_t) str_val;
            (*args_len_out)[arg_idx] = str_len;
            arg_idx++;
        } else if (Z_TYPE_P(value) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(value);
            zval*      z_item;

            ZEND_HASH_FOREACH_VAL(ht, z_item) {
                if (Z_TYPE_P(z_item) == IS_STRING) {
                    (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_item);
                    (*args_len_out)[arg_idx] = Z_STRLEN_P(z_item);
                    arg_idx++;
                } else if (Z_TYPE_P(z_item) == IS_LONG) {
                    /* Convert long to string */
                    size_t str_len;
                    char*  str_val = alloc_list_number_string(Z_LVAL_P(z_item), &str_len);
                    if (!str_val) {
                        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                        free_list_command_args(*args_out, *args_len_out);
                        return 0;
                    }

                    (*allocated_strings)[*allocated_count] = str_val;
                    (*allocated_count)++;

                    (*args_out)[arg_idx]     = (uintptr_t) str_val;
                    (*args_len_out)[arg_idx] = str_len;
                    arg_idx++;
                } else if (Z_TYPE_P(z_item) == IS_DOUBLE) {
                    /* Convert double to string */
                    size_t str_len;
                    char*  str_val = alloc_list_double_string(Z_DVAL_P(z_item), &str_len);
                    if (!str_val) {
                        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                        free_list_command_args(*args_out, *args_len_out);
                        return 0;
                    }

                    (*allocated_strings)[*allocated_count] = str_val;
                    (*allocated_count)++;

                    (*args_out)[arg_idx]     = (uintptr_t) str_val;
                    (*args_len_out)[arg_idx] = str_len;
                    arg_idx++;
                } else {
                    FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                    free_list_command_args(*args_out, *args_len_out);
                    return 0;
                }
            }
            ZEND_HASH_FOREACH_END();
        }
    }

    return total_args;
}


/**
 * Prepare arguments for key+count commands (LPOP, RPOP)
 */
int prepare_list_key_count_args(list_command_args_t* args,
                                uintptr_t**          args_out,
                                unsigned long**      args_len_out,
                                char***              allocated_strings,
                                int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    unsigned long arg_count = 1;
    if (args->count > 0) {
        arg_count = 2;
    }

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = NULL;
    *allocated_count   = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Add count if provided */
    if (args->count > 0) {
        size_t count_len;
        char*  count_str = alloc_list_number_string(args->count, &count_len);
        if (!count_str) {
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        /* Track allocated string */
        *allocated_strings = (char**) emalloc(sizeof(char*));
        if (!*allocated_strings) {
            efree(count_str);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[0] = count_str;
        *allocated_count        = 1;

        (*args_out)[1]     = (uintptr_t) count_str;
        (*args_len_out)[1] = count_len;
    }

    return arg_count;
}

/**
 * Prepare arguments for blocking commands (BLPOP, BRPOP)
 */
int prepare_list_blocking_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out,
                               char***              allocated_strings,
                               int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);

    int keys_count = 0;

    /* Check if keys is an array or single key */
    if (args->keys && Z_TYPE_P(args->keys) == IS_ARRAY) {
        keys_count = zend_hash_num_elements(Z_ARRVAL_P(args->keys));
    } else if (args->keys && Z_TYPE_P(args->keys) == IS_STRING) {
        keys_count = 1;
    } else if (args->key) {
        keys_count = 1;
    }

    if (keys_count <= 0) {
        return 0;
    }

    /* Calculate the number of arguments: keys + timeout */
    unsigned long arg_count = keys_count + 1;

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    int arg_idx = 0;

    /* Add keys */
    if (args->keys && Z_TYPE_P(args->keys) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(args->keys);
        zval*      z_key;
        ZEND_HASH_FOREACH_VAL(ht, z_key) {
            if (Z_TYPE_P(z_key) != IS_STRING) {
                FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
                free_list_command_args(*args_out, *args_len_out);
                return 0;
            }
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_key);
            (*args_len_out)[arg_idx] = Z_STRLEN_P(z_key);
            arg_idx++;
        }
        ZEND_HASH_FOREACH_END();
    } else if (args->keys && Z_TYPE_P(args->keys) == IS_STRING) {
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(args->keys);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(args->keys);
        arg_idx++;
    } else if (args->key) {
        (*args_out)[arg_idx]     = (uintptr_t) args->key;
        (*args_len_out)[arg_idx] = args->key_len;
        arg_idx++;
    }

    /* Add timeout */
    size_t timeout_len;
    char*  timeout_str = alloc_list_double_string(args->blocking_opts.timeout, &timeout_len);
    if (!timeout_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = timeout_str;
    (*allocated_count)++;

    (*args_out)[arg_idx]     = (uintptr_t) timeout_str;
    (*args_len_out)[arg_idx] = timeout_len;

    return arg_count;
}

/**
 * Execute list move command (LMOVE, BLMOVE)
 */
int execute_list_move_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               src = NULL, *dst = NULL, *wherefrom = NULL, *whereto = NULL;
    size_t               src_len, dst_len, wherefrom_len, whereto_len;
    double               timeout      = -1.0;
    char*                output_value = NULL;
    size_t               output_len;

    /* Parse parameters based on command type */
    if (cmd_type == BLMove) {
        /* BLMOVE: src, dst, wherefrom, whereto, timeout */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossssd",
                                         &object,
                                         ce,
                                         &src,
                                         &src_len,
                                         &dst,
                                         &dst_len,
                                         &wherefrom,
                                         &wherefrom_len,
                                         &whereto,
                                         &whereto_len,
                                         &timeout) == FAILURE) {
            return 0;
        }
    } else {
        /* LMOVE: src, dst, wherefrom, whereto */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossss",
                                         &object,
                                         ce,
                                         &src,
                                         &src_len,
                                         &dst,
                                         &dst_len,
                                         &wherefrom,
                                         &wherefrom_len,
                                         &whereto,
                                         &whereto_len) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, src, src_len);
        args.move_opts.dest_key             = dst;
        args.move_opts.dest_key_len         = dst_len;
        args.move_opts.source_direction     = wherefrom;
        args.move_opts.source_direction_len = wherefrom_len;
        args.move_opts.dest_direction       = whereto;
        args.move_opts.dest_direction_len   = whereto_len;
        args.move_opts.timeout              = timeout;
        args.move_opts.has_timeout          = (timeout >= 0.0);

        /* Create array to pass both output_value and output_len */

        int status = execute_list_generic_command(
            valkey_glide, cmd_type, &args, NULL, process_list_string_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list MPOP command (LMPOP, BLMPOP)
 */
int execute_list_mpop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                keys = NULL;
    char*                from = NULL;
    size_t               from_len;
    zend_long            count   = 1;
    double               timeout = 0.0;

    /* Parse parameters based on command type */
    if (cmd_type == BLMPop) {
        /* BLMPOP: timeout, keys, from, count */
        if (zend_parse_method_parameters(
                argc, object, "Odas|l", &object, ce, &timeout, &keys, &from, &from_len, &count) ==
            FAILURE) {
            return 0;
        }
    } else {
        /* LMPOP: keys, from, count */
        if (zend_parse_method_parameters(
                argc, object, "Oas|l", &object, ce, &keys, &from, &from_len, &count) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client            = valkey_glide->glide_client;
        args.keys                    = keys;
        args.mpop_opts.direction     = from;
        args.mpop_opts.direction_len = from_len;
        args.mpop_opts.count         = count;
        args.mpop_opts.timeout       = timeout;
        args.mpop_opts.has_count     = (count > 0);
        args.mpop_opts.has_timeout   = (cmd_type == BLMPop);

        int status = execute_list_generic_command(
            valkey_glide, cmd_type, &args, NULL, process_list_mpop_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Prepare arguments for range commands (LRANGE)
 */
int prepare_list_range_args(list_command_args_t* args,
                            uintptr_t**          args_out,
                            unsigned long**      args_len_out,
                            char***              allocated_strings,
                            int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!allocate_list_command_args(3, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(2 * sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Second argument: start */
    size_t start_len;
    char*  start_str = alloc_list_number_string(args->start, &start_len);
    if (!start_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = start_str;
    (*allocated_count)++;

    (*args_out)[1]     = (uintptr_t) start_str;
    (*args_len_out)[1] = start_len;

    /* Third argument: end */
    size_t end_len;
    char*  end_str = alloc_list_number_string(args->end, &end_len);
    if (!end_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = end_str;
    (*allocated_count)++;

    (*args_out)[2]     = (uintptr_t) end_str;
    (*args_len_out)[2] = end_len;

    return 3;
}

/**
 * Prepare arguments for position commands (LPOS)
 */
int prepare_list_position_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out,
                               char***              allocated_strings,
                               int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!args->element || args->element_len <= 0) {
        return 0;
    }

    /* Count optional arguments */
    int opt_count = 0;
    if (args->position_opts.has_rank)
        opt_count += 2;
    if (args->position_opts.has_count)
        opt_count += 2;
    if (args->position_opts.has_maxlen)
        opt_count += 2;

    unsigned long arg_count = 2 + opt_count; /* key + element + options */

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(3 * sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* Key and element are first two arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->element;
    (*args_len_out)[1] = args->element_len;

    unsigned int arg_idx = 2;

    /* Add optional arguments */
    if (args->position_opts.has_rank) {
        (*args_out)[arg_idx]     = (uintptr_t) "RANK";
        (*args_len_out)[arg_idx] = 4;
        arg_idx++;

        size_t rank_len;
        char*  rank_str = alloc_list_number_string(args->position_opts.rank, &rank_len);
        if (!rank_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = rank_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) rank_str;
        (*args_len_out)[arg_idx] = rank_len;
        arg_idx++;
    }

    if (args->position_opts.has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = 5;
        arg_idx++;

        size_t count_len;
        char*  count_str = alloc_list_number_string(args->position_opts.count, &count_len);
        if (!count_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = count_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) count_str;
        (*args_len_out)[arg_idx] = count_len;
        arg_idx++;
    }

    if (args->position_opts.has_maxlen) {
        (*args_out)[arg_idx]     = (uintptr_t) "MAXLEN";
        (*args_len_out)[arg_idx] = 6;
        arg_idx++;

        size_t maxlen_len;
        char*  maxlen_str = alloc_list_number_string(args->position_opts.maxlen, &maxlen_len);
        if (!maxlen_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = maxlen_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) maxlen_str;
        (*args_len_out)[arg_idx] = maxlen_len;
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare arguments for insert commands (LINSERT)
 */
int prepare_list_insert_args(list_command_args_t* args,
                             uintptr_t**          args_out,
                             unsigned long**      args_len_out) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!args->position_opts.position || args->position_opts.position_len <= 0 ||
        !args->position_opts.pivot || args->position_opts.pivot_len <= 0 || !args->value ||
        args->value_len <= 0) {
        return 0;
    }

    if (!allocate_list_command_args(4, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->position_opts.position;
    (*args_len_out)[1] = args->position_opts.position_len;
    (*args_out)[2]     = (uintptr_t) args->position_opts.pivot;
    (*args_len_out)[2] = args->position_opts.pivot_len;
    (*args_out)[3]     = (uintptr_t) args->value;
    (*args_len_out)[3] = args->value_len;

    return 4;
}

/**
 * Prepare arguments for index/set commands (LINDEX, LSET)
 */
int prepare_list_index_set_args(list_command_args_t* args,
                                uintptr_t**          args_out,
                                unsigned long**      args_len_out,
                                char***              allocated_strings,
                                int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    /* For LSET, we need value as well */
    unsigned long arg_count = (args->value) ? 3 : 2;

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Second argument: index */
    size_t index_len;
    char*  index_str = alloc_list_number_string(args->index, &index_len);
    if (!index_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = index_str;
    (*allocated_count)++;

    (*args_out)[1]     = (uintptr_t) index_str;
    (*args_len_out)[1] = index_len;

    /* Third argument: value (for LSET) */
    if (args->value) {
        (*args_out)[2]     = (uintptr_t) args->value;
        (*args_len_out)[2] = args->value_len;
    }

    return arg_count;
}

/**
 * Prepare arguments for remove commands (LREM)
 */
int prepare_list_rem_args(list_command_args_t* args,
                          uintptr_t**          args_out,
                          unsigned long**      args_len_out,
                          char***              allocated_strings,
                          int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!args->value || args->value_len <= 0) {
        return 0;
    }

    if (!allocate_list_command_args(3, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Second argument: count */
    size_t count_len;
    char*  count_str = alloc_list_number_string(args->count, &count_len);
    if (!count_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = count_str;
    (*allocated_count)++;

    (*args_out)[1]     = (uintptr_t) count_str;
    (*args_len_out)[1] = count_len;

    /* Third argument: value */
    (*args_out)[2]     = (uintptr_t) args->value;
    (*args_len_out)[2] = args->value_len;

    return 3;
}

/**
 * Prepare arguments for trim commands (LTRIM)
 */
int prepare_list_trim_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!allocate_list_command_args(3, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(2 * sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Second argument: start */
    size_t start_len;
    char*  start_str = alloc_list_number_string(args->start, &start_len);
    if (!start_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = start_str;
    (*allocated_count)++;

    (*args_out)[1]     = (uintptr_t) start_str;
    (*args_len_out)[1] = start_len;

    /* Third argument: end */
    size_t end_len;
    char*  end_str = alloc_list_number_string(args->end, &end_len);
    if (!end_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = end_str;
    (*allocated_count)++;

    (*args_out)[2]     = (uintptr_t) end_str;
    (*args_len_out)[2] = end_len;

    return 3;
}

/**
 * Prepare arguments for move commands (LMOVE, BLMOVE, RPOPLPUSH, BRPOPLPUSH)
 */
int prepare_list_move_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);
    VALIDATE_LIST_KEY(args->key, args->key_len);

    if (!args->move_opts.dest_key || args->move_opts.dest_key_len <= 0) {
        return 0;
    }

    /* For blocking commands, we need timeout. For RPOPLPUSH-style, we only need src and dest keys
     */
    unsigned long arg_count = 2; /* src_key, dest_key */

    /* Check if we have direction arguments (LMOVE/BLMOVE style) */
    if (args->move_opts.source_direction && args->move_opts.dest_direction) {
        arg_count = 4; /* src_key, dest_key, src_dir, dest_dir */
    }

    /* Add timeout for blocking commands */
    if (args->move_opts.has_timeout) {
        arg_count++;
    }

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(sizeof(char*));
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    unsigned int arg_idx = 0;

    /* First argument: source key */
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Second argument: destination key */
    (*args_out)[arg_idx]     = (uintptr_t) args->move_opts.dest_key;
    (*args_len_out)[arg_idx] = args->move_opts.dest_key_len;
    arg_idx++;

    /* Add direction arguments if present (LMOVE/BLMOVE style) */
    if (args->move_opts.source_direction && args->move_opts.dest_direction) {
        (*args_out)[arg_idx]     = (uintptr_t) args->move_opts.source_direction;
        (*args_len_out)[arg_idx] = args->move_opts.source_direction_len;
        arg_idx++;

        (*args_out)[arg_idx]     = (uintptr_t) args->move_opts.dest_direction;
        (*args_len_out)[arg_idx] = args->move_opts.dest_direction_len;
        arg_idx++;
    }

    /* Add timeout for blocking commands */
    if (args->move_opts.has_timeout) {
        size_t timeout_len;
        char*  timeout_str = alloc_list_double_string(args->move_opts.timeout, &timeout_len);
        if (!timeout_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = timeout_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) timeout_str;
        (*args_len_out)[arg_idx] = timeout_len;
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare arguments for MPOP commands (LMPOP, BLMPOP)
 */
int prepare_list_mpop_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count) {
    VALIDATE_LIST_CLIENT(args->glide_client);

    if (!args->keys || !args->mpop_opts.direction || args->mpop_opts.direction_len <= 0) {
        return 0;
    }

    /* Get the number of keys */
    int keys_count = 0;
    if (Z_TYPE_P(args->keys) == IS_ARRAY) {
        keys_count = zend_hash_num_elements(Z_ARRVAL_P(args->keys));
    }

    if (keys_count <= 0) {
        return 0;
    }

    /* Calculate arguments: [timeout] + numkeys + keys + direction + [COUNT + count_value] */
    unsigned long arg_count = 1 + keys_count + 1; /* numkeys + keys + direction */
    if (args->mpop_opts.has_timeout)
        arg_count++; /* timeout for blocking version */
    if (args->mpop_opts.has_count)
        arg_count += 2; /* COUNT + value */

    if (!allocate_list_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = (char**) emalloc(3 * sizeof(char*)); /* timeout, numkeys, count */
    if (!*allocated_strings) {
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }
    *allocated_count = 0;

    unsigned int arg_idx = 0;

    /* Add timeout for blocking commands (first argument) */
    if (args->mpop_opts.has_timeout) {
        size_t timeout_len;
        char*  timeout_str = alloc_list_double_string(args->mpop_opts.timeout, &timeout_len);
        if (!timeout_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = timeout_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) timeout_str;
        (*args_len_out)[arg_idx] = timeout_len;
        arg_idx++;
    }

    /* Add numkeys */
    size_t numkeys_len;
    char*  numkeys_str = alloc_list_number_string(keys_count, &numkeys_len);
    if (!numkeys_str) {
        FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
        free_list_command_args(*args_out, *args_len_out);
        return 0;
    }

    (*allocated_strings)[*allocated_count] = numkeys_str;
    (*allocated_count)++;

    (*args_out)[arg_idx]     = (uintptr_t) numkeys_str;
    (*args_len_out)[arg_idx] = numkeys_len;
    arg_idx++;

    /* Add keys */
    HashTable* ht = Z_ARRVAL_P(args->keys);
    zval*      z_key;
    ZEND_HASH_FOREACH_VAL(ht, z_key) {
        if (Z_TYPE_P(z_key) != IS_STRING) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_key);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_key);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add direction */
    (*args_out)[arg_idx]     = (uintptr_t) args->mpop_opts.direction;
    (*args_len_out)[arg_idx] = args->mpop_opts.direction_len;
    arg_idx++;

    /* Add COUNT if specified */
    if (args->mpop_opts.has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = 5;
        arg_idx++;

        size_t count_len;
        char*  count_str = alloc_list_number_string(args->mpop_opts.count, &count_len);
        if (!count_str) {
            FREE_LIST_ALLOCATED_STRINGS(*allocated_strings, *allocated_count);
            free_list_command_args(*args_out, *args_len_out);
            return 0;
        }

        (*allocated_strings)[*allocated_count] = count_str;
        (*allocated_count)++;

        (*args_out)[arg_idx]     = (uintptr_t) count_str;
        (*args_len_out)[arg_idx] = count_len;
        arg_idx++;
    }

    return arg_count;
}

/* ====================================================================
 * HIGH-LEVEL COMMAND EXECUTION FUNCTIONS
 * ==================================================================== */

/**
 * Execute list push command (LPUSH, RPUSH, LPUSHX, RPUSHX)
 */
int execute_list_push_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_args;
    int                  arg_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os+", &object, ce, &key, &key_len, &z_args, &arg_count) == FAILURE) {
        return 0;
    }

    /* Make sure we have at least one value to push */
    if (arg_count < 1) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Set up command args */
    list_command_args_t args;
    INIT_LIST_COMMAND_ARGS(args);

    args.glide_client = valkey_glide->glide_client;
    SET_LIST_KEY(args, key, key_len);
    args.values      = z_args;
    args.value_count = arg_count;


    /* In batch mode, buffer the command and return $this for method chaining */
    int status = execute_list_generic_command(
        valkey_glide, cmd_type, &args, NULL, process_list_int_result_async, return_value);

    if (status) {
        if (valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }
        return 1;
    }
    return 0;
}

/**
 * Execute list pop command (LPOP, RPOP)
 */
int execute_list_pop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            count     = 0;
    int                  has_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|l", &object, ce, &key, &key_len, &count) ==
        FAILURE) {
        return 0;
    }

    /* Check if count parameter was provided */
    has_count = (argc > 1);

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        SET_LIST_COUNT(args, has_count ? count : 0);

        int status = execute_list_generic_command(
            valkey_glide, cmd_type, &args, NULL, process_list_pop_result_async, return_value);

        /* Return value is already set by execute_list_generic_command if successful */
        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list blocking pop command (BLPOP, BRPOP)
 */
int execute_list_blocking_pop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                keys;
    double               timeout = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Ozd", &object, ce, &keys, &timeout) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client              = valkey_glide->glide_client;
        args.keys                      = keys;
        args.blocking_opts.timeout     = timeout;
        args.blocking_opts.has_timeout = 1;

        int status = execute_list_generic_command(
            valkey_glide, cmd_type, &args, NULL, process_list_blocking_result_async, return_value);

        /* Return value is already set by execute_list_generic_command if successful */
        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list length command (LLEN)
 */
int execute_list_len_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 output_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);

        int status = execute_list_generic_command(
            valkey_glide, LLen, &args, NULL, process_list_int_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/* Execute an LRANGE command using the Valkey Glide client - New pattern */
int execute_list_range_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            start, end;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osll", &object, ce, &key, &key_len, &start, &end) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        SET_LIST_RANGE(args, start, end);

        int status = execute_list_generic_command(
            valkey_glide, LRange, &args, NULL, process_list_array_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list index command (LINDEX)
 */
int execute_list_index_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            index;
    char*                output_value = NULL;
    size_t               output_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osl", &object, ce, &key, &key_len, &index) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        args.index = index;


        int status = execute_list_generic_command(
            valkey_glide, LIndex, &args, NULL, process_list_string_result_async, return_value);
        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list set command (LSET)
 */
int execute_list_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    zend_long            index;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osls", &object, ce, &key, &key_len, &index, &val, &val_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        args.index     = index;
        args.value     = val;
        args.value_len = val_len;


        int status = execute_list_generic_command(
            valkey_glide, LSet, &args, NULL, process_list_ok_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
    }

    return 0;
}

/**
 * Execute list insert command (LINSERT)
 */
int execute_list_insert_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *pos = NULL, *pivot = NULL, *val = NULL;
    size_t               key_len, pos_len, pivot_len, val_len;
    long                 output_value;
    char*                upper_pos = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Ossss",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &pos,
                                     &pos_len,
                                     &pivot,
                                     &pivot_len,
                                     &val,
                                     &val_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Make position uppercase for comparison */
        if (pos_len > 0) {
            upper_pos = emalloc(pos_len + 1);
            int i;
            for (i = 0; i < pos_len; i++) {
                upper_pos[i] = toupper(pos[i]);
            }
            upper_pos[pos_len] = '\0';
        }

        /* Check if position is BEFORE or AFTER */
        if (upper_pos == NULL ||
            (strcmp(upper_pos, "BEFORE") != 0 && strcmp(upper_pos, "AFTER") != 0)) {
            if (upper_pos)
                efree(upper_pos);
            return 0;
        }

        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        args.position_opts.position     = upper_pos;
        args.position_opts.position_len = strlen(upper_pos);
        args.position_opts.pivot        = pivot;
        args.position_opts.pivot_len    = pivot_len;
        args.value                      = val;
        args.value_len                  = val_len;

        int status = execute_list_generic_command(
            valkey_glide, LInsert, &args, NULL, process_list_int_result_async, return_value);

        /* Clean up */
        if (upper_pos)
            efree(upper_pos);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
    }

    return 0;
}

/**
 * Execute list position command (LPOS)
 */
int execute_list_position_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval *               z_value, *z_opts = NULL;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osz|a", &object, ce, &key, &key_len, &z_value, &z_opts) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Convert value to string if needed */
        switch (Z_TYPE_P(z_value)) {
            case IS_STRING:
                val     = Z_STRVAL_P(z_value);
                val_len = Z_STRLEN_P(z_value);
                break;
            default:
                /* For now, only handle string values */
                return 0;
        }

        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        args.element     = val;
        args.element_len = val_len;

        /* Parse position options */
        if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
            parse_list_position_options(z_opts, &args.position_opts);
        }

        /* Use the correct processor depending on whether COUNT option is used */
        z_result_processor_t processor = args.position_opts.has_count
                                             ? process_list_array_result_async
                                             : process_list_int_result_async;

        int status =
            execute_list_generic_command(valkey_glide, LPos, &args, NULL, processor, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list remove command (LREM)
 */
int execute_list_rem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *value = NULL;
    size_t               key_len, value_len;
    zend_long            count = 0;
    long                 output_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss|l", &object, ce, &key, &key_len, &value, &value_len, &count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        SET_LIST_COUNT(args, count);
        args.value     = value;
        args.value_len = value_len;

        int status = execute_list_generic_command(
            valkey_glide, LRem, &args, NULL, process_list_int_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}

/**
 * Execute list trim command (LTRIM)
 */
int execute_list_trim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            start, end;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osll", &object, ce, &key, &key_len, &start, &end) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        list_command_args_t args;
        INIT_LIST_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        SET_LIST_KEY(args, key, key_len);
        SET_LIST_RANGE(args, start, end);


        int status = execute_list_generic_command(
            valkey_glide, LTrim, &args, NULL, process_list_ok_result_async, return_value);

        if (status) {
            if (valkey_glide->is_in_batch_mode) {
                ZVAL_COPY(return_value, object);
            }
            return 1;
        }
        return 0;
    }

    return 0;
}
