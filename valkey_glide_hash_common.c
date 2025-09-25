/*
  +----------------------------------------------------------------------+
  | Valkey Glide Hash Commands Common Utilities                          |
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
#include "valkey_glide_hash_common.h"

#include "common.h"
#include "ext/standard/php_var.h"
#include "valkey_glide_z_common.h"

extern zend_class_entry* ce;
extern zend_class_entry* get_valkey_glide_exception_ce();
/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

/**
 * Generic hash command execution framework with batch support
 */
int execute_h_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              h_command_args_t*    args,
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
    VALIDATE_HASH_ARGS(valkey_glide->glide_client, args->key);

    /* Prepare arguments based on command type */
    switch (cmd_type) {
        case HLen:
            arg_count = prepare_h_key_only_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HGet:
        case HExists:
        case HStrlen:
            arg_count = prepare_h_single_field_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HSetNX:
            arg_count = prepare_h_field_value_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HDel:
        case HMGet:
            arg_count = prepare_h_multi_field_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HSet:
            arg_count = prepare_h_set_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HMSet:
            arg_count = prepare_h_mset_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HIncrBy:
        case HIncrByFloat:
            arg_count = prepare_h_incr_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HRandField:
            arg_count = prepare_h_randfield_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HKeys:
        case HVals:
        case HGetAll:
            arg_count = prepare_h_key_only_args(
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
    /* Clean up allocated resources */
    cleanup_h_command_args(allocated_strings, allocated_count, cmd_args, args_len);
    return status;
}


z_result_processor_t get_processor_for_response_type(int response_type);

/**
 * Batch-aware version for unified command executors
 */
int execute_h_simple_command(valkey_glide_object* valkey_glide,
                             enum RequestType     cmd_type,
                             h_command_args_t*    args,
                             void*                result_ptr,
                             int                  response_type,
                             zval*                return_value) {
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            status            = 0;

    /* Validate basic arguments */
    VALIDATE_HASH_ARGS(valkey_glide->glide_client, args->key);

    /* Prepare arguments based on command type */
    switch (cmd_type) {
        case HLen:
        case HKeys:
        case HVals:
        case HGetAll:
            arg_count = prepare_h_key_only_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HGet:
        case HExists:
        case HStrlen:
            arg_count = prepare_h_single_field_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HSetNX:
            arg_count = prepare_h_field_value_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HDel:
            arg_count = prepare_h_multi_field_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HSet:
            arg_count = prepare_h_set_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HMSet:
            arg_count = prepare_h_mset_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case HIncrBy:
            arg_count = prepare_h_incr_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        default:
            goto cleanup;
    }

    if (arg_count <= 0) {
        goto cleanup;
    }

    /* Check for batch mode */
    z_result_processor_t processor = get_processor_for_response_type(response_type);
    if (!processor) {
        status = 0;
        goto cleanup;
    }
    if (valkey_glide->is_in_batch_mode) {
        status = buffer_command_for_batch(
            valkey_glide, cmd_type, cmd_args, args_len, arg_count, result_ptr, processor);
        goto cleanup;
    }

    /* Execute the command */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, arg_count, cmd_args, args_len);


    /* Process result using standard handlers */
    if (result) {
        if (!result->command_error && result->response && processor) {
            status = processor(result->response, result_ptr, return_value);
        }
        free_command_result(result);
    } else {
        status = 0;
    }

cleanup:
    /* Clean up allocated resources */
    cleanup_h_command_args(allocated_strings, allocated_count, cmd_args, args_len);
    return status;
}

/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */

/**
 * Prepare arguments for single-key commands (HLEN, HKEYS, HVALS, HGETALL)
 */
int prepare_h_key_only_args(h_command_args_t* args,
                            uintptr_t**       args_out,
                            unsigned long**   args_len_out,
                            char***           allocated_strings,
                            int*              allocated_count) {
    /* Allocate argument arrays */
    *args_out          = (uintptr_t*) emalloc(sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;

    /* Set key as the only argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    return 1;
}

/**
 * Prepare arguments for single-field commands (HGET, HEXISTS, HSTRLEN)
 */
int prepare_h_single_field_args(h_command_args_t* args,
                                uintptr_t**       args_out,
                                unsigned long**   args_len_out,
                                char***           allocated_strings,
                                int*              allocated_count) {
    if (!args->field) {
        return 0;
    }

    /* Allocate argument arrays */
    *args_out          = (uintptr_t*) emalloc(2 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(2 * sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;

    /* Set key and field arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->field;
    (*args_len_out)[1] = args->field_len;

    return 2;
}

/**
 * Prepare arguments for field-value commands (HSETNX)
 */
int prepare_h_field_value_args(h_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out,
                               char***           allocated_strings,
                               int*              allocated_count) {
    if (!args->field || !args->value) {
        return 0;
    }

    /* Allocate argument arrays */
    *args_out          = (uintptr_t*) emalloc(3 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(3 * sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;


    /* Set key, field, and value arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->field;
    (*args_len_out)[1] = args->field_len;
    (*args_out)[2]     = (uintptr_t) args->value;
    (*args_len_out)[2] = args->value_len;

    return 3;
}

/**
 * Prepare arguments for multi-field commands (HDEL, HMGET)
 */
int prepare_h_multi_field_args(h_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out,
                               char***           allocated_strings,
                               int*              allocated_count) {
    if (!args->fields || args->field_count <= 0) {
        return 0;
    }

    /* Calculate total argument count: key + fields */
    unsigned long arg_count = 1 + args->field_count;

    /* Allocate argument arrays */
    *args_out          = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = (char**) emalloc(args->field_count * sizeof(char*));
    *allocated_count   = 0;

    /* Set key as first argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Convert and add fields */
    return convert_zval_array_to_args(args->fields,
                                      1,
                                      *args_out,
                                      *args_len_out,
                                      *allocated_strings,
                                      allocated_count,
                                      args->field_count);
}

/**
 * Prepare arguments for HSET command (handles both formats)
 */
int prepare_h_set_args(h_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count) {
    if (!args->field_values) {
        return 0;
    }

    /* Handle associative array format */
    if (args->is_array_arg) {
        if (args->fv_count != 1 || Z_TYPE(args->field_values[0]) != IS_ARRAY) {
            return 0;
        }

        zval*      z_array     = &args->field_values[0];
        HashTable* ht          = Z_ARRVAL_P(z_array);
        int        pairs_count = zend_hash_num_elements(ht);

        if (pairs_count == 0) {
            return 0;
        }

        /* Prepare command arguments: key + field-value pairs */
        unsigned long arg_count = 1 + (pairs_count * 2);
        *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
        *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
        *allocated_strings      = (char**) emalloc((pairs_count * 2) * sizeof(char*));
        *allocated_count        = 0;

        /* First argument: key */
        (*args_out)[0]     = (uintptr_t) args->key;
        (*args_len_out)[0] = args->key_len;

        /* Process field-value pairs */
        return process_field_value_pairs(
            z_array, *args_out, *args_len_out, 1, *allocated_strings, allocated_count);
    } else {
        /* Original variadic usage */
        if (args->fv_count < 2 || args->fv_count % 2 != 0) {
            return 0;
        }

        /* Prepare command arguments: key + field/value pairs */
        unsigned long arg_count = 1 + args->fv_count;
        *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
        *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
        *allocated_strings      = (char**) emalloc(args->fv_count * sizeof(char*));
        *allocated_count        = 0;

        /* First argument: key */
        (*args_out)[0]     = (uintptr_t) args->key;
        (*args_len_out)[0] = args->key_len;

        /* Convert field/value pairs */
        return convert_zval_array_to_args(args->field_values,
                                          1,
                                          *args_out,
                                          *args_len_out,
                                          *allocated_strings,
                                          allocated_count,
                                          args->fv_count);
    }
}

/**
 * Prepare arguments for HMSET command
 */
int prepare_h_mset_args(h_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count) {
    if (!args->field_values || args->fv_count <= 0) {
        return 0;
    }

    /* HMSET expects an associative array */
    HashTable*    ht          = Z_ARRVAL_P(args->field_values);
    int           pairs_count = zend_hash_num_elements(ht);
    unsigned long arg_count   = 1 + (pairs_count * 2);

    *args_out          = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = (char**) emalloc((pairs_count * 2) * sizeof(char*));
    *allocated_count   = 0;

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Process field-value pairs */
    return process_field_value_pairs(
        args->field_values, *args_out, *args_len_out, 1, *allocated_strings, allocated_count);
}

/**
 * Prepare arguments for increment commands (HINCRBY, HINCRBYFLOAT)
 */
int prepare_h_incr_args(h_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count) {
    if (!args->field) {
        return 0;
    }

    /* Allocate argument arrays */
    *args_out          = (uintptr_t*) emalloc(3 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(3 * sizeof(unsigned long));
    *allocated_strings = (char**) emalloc(sizeof(char*));
    *allocated_count   = 0;

    /* Set key and field arguments */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->field;
    (*args_len_out)[1] = args->field_len;

    /* Convert increment value to string */
    char*  incr_str;
    size_t incr_len;

    if (args->float_incr != 0.0) {
        /* HINCRBYFLOAT */
        incr_str = double_to_string(args->float_incr, &incr_len);
    } else {
        /* HINCRBY */
        incr_str = long_to_string(args->increment, &incr_len);
    }

    (*allocated_strings)[0] = incr_str;
    *allocated_count        = 1;

    (*args_out)[2]     = (uintptr_t) incr_str;
    (*args_len_out)[2] = incr_len;

    return 3;
}

/**
 * Prepare arguments for HRANDFIELD command
 */
int prepare_h_randfield_args(h_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count) {
    /* Calculate argument count */
    unsigned long arg_count      = 1; /* key */
    int           need_count_str = 0;

    if (args->count != 1) {
        arg_count++; /* count */
        need_count_str = 1;
    }
    if (args->withvalues) {
        arg_count++; /* WITHVALUES */
        if (need_count_str == 0) {
            arg_count++; /* count (required with WITHVALUES) */
            need_count_str = 1;
        }
    }

    *args_out          = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = need_count_str ? (char**) emalloc(sizeof(char*)) : NULL;
    *allocated_count   = 0;

    /* First argument: key */
    int arg_idx              = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add count if needed */
    if (need_count_str) {
        char* count_str = long_to_string(args->count, &(*args_len_out)[arg_idx]);

        (*allocated_strings)[0] = count_str;
        *allocated_count        = 1;

        (*args_out)[arg_idx] = (uintptr_t) count_str;
        arg_idx++;
    }

    /* Add WITHVALUES if specified */
    if (args->withvalues) {
        const char* withvalues_str = "WITHVALUES";
        (*args_out)[arg_idx]       = (uintptr_t) withvalues_str;
        (*args_len_out)[arg_idx]   = strlen(withvalues_str);
        arg_idx++;
    }

    return arg_count;
}

/* ====================================================================
 * BATCH-COMPATIBLE RESULT PROCESSORS
 * ==================================================================== */
int process_h_int_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_LONG(return_value, 0);
        return 0;
    }

    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    }
    return 0;
}

/**
 * Batch-compatible wrapper for boolean responses
 */
int process_h_bool_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response)
        return 0;

    int ret_val = -1;
    if (response && response->response_type == Bool) {
        ret_val = response->bool_value ? 1 : 0;
        ZVAL_BOOL(return_value, ret_val);
    }

    return ret_val;
}

/**
 * Batch-compatible wrapper for string responses
 */
int process_h_string_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response)
        return 0;

    char*  result_str = NULL;
    size_t result_len = 0;

    if (response->response_type == String) {
        result_str = estrndup(response->string_value, response->string_value_len);
        result_len = response->string_value_len;
        ZVAL_STRINGL(return_value, result_str, result_len);
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
int process_h_array_result_async(CommandResponse* response, void* output, zval* return_value) {
    /* Initialize return array */
    array_init(return_value);
    return command_response_to_zval(
        response, (zval*) return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Batch-compatible wrapper for map responses
 */
int process_h_map_result_async(CommandResponse* response, void* output, zval* return_value) {
    array_init(return_value);
    return command_response_to_zval(
        response, (zval*) return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
}

/**
 * Batch-compatible wrapper for OK responses
 */
int process_h_ok_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response)
        return 0;

    if (response->response_type == Ok) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    ZVAL_FALSE(return_value);
    return 0;
}

/**
 * Get batch-compatible processor for response type
 */
z_result_processor_t get_processor_for_response_type(int response_type) {
    switch (response_type) {
        case H_RESPONSE_INT:
            return process_h_int_result_async;
        case H_RESPONSE_BOOL:
            return process_h_bool_result_async;
        case H_RESPONSE_STRING:
            return process_h_string_result_async;
        case H_RESPONSE_ARRAY:
            return process_h_array_result_async;
        case H_RESPONSE_MAP:
            return process_h_map_result_async;
        case H_RESPONSE_OK:
            return process_h_ok_result_async;
        default:
            return NULL;
    }
}

/* ====================================================================
 * RESULT PROCESSING FUNCTIONS
 * ==================================================================== */

/**
 * Process results for HMGET (associative field mapping) - legacy version
 */
int process_h_mget_result(CommandResponse* response, void* output, zval* return_value) {
    h_command_args_t* args = (h_command_args_t*) output;

    /* Check if the command was successful */
    if (!response) {
        return 0;
    }
    /* Initialize return array */
    array_init(return_value);

    /* Process the result - map back to original field names */
    int ret_val = 0;
    if (response && response->response_type == Array) {
        for (int i = 0; i < args->field_count && i < response->array_value_len; i++) {
            zval*  field = &args->fields[i];
            zval   field_value;
            char*  field_str    = NULL;
            size_t field_len    = 0;
            int    need_to_free = 0;

            /* Convert field to string for associative array key */
            field_str = zval_to_string_safe(field, &field_len, &need_to_free);

            if (!field_str) {
                continue;
            }

            /* Set value in result array */
            struct CommandResponse* element = &response->array_value[i];

            if (element->response_type == String) {
                ZVAL_STRINGL(&field_value, element->string_value, element->string_value_len);
            } else if (element->response_type == Null) {
                ZVAL_FALSE(&field_value);
            } else {
                ZVAL_NULL(&field_value);
            }

            add_assoc_zval_ex(return_value, field_str, field_len, &field_value);

            /* Free the field string if we allocated it */
            if (need_to_free) {
                efree(field_str);
            }
        }
        ret_val = 1;
    }

    /* Free field array */
    for (int j = 0; j < args->field_count; j++) {
        zval_ptr_dtor(&args->fields[j]);
    }
    efree(args->fields);
    efree(args);

    return ret_val;
}

/**
 * Process results for HRANDFIELD
 */
int process_h_randfield_result(CommandResponse* response, void* output, zval* return_value) {
    /* Check if the command was successful */
    if (!response) {
        efree(output);
        return 0;
    }
    bool* withvalues_ptr = output;

    int ret_val = command_response_to_zval(
        response,
        return_value,
        *withvalues_ptr ? COMMAND_RESPONSE_ARRAY_ASSOCIATIVE : COMMAND_RESPONSE_NOT_ASSOSIATIVE,
        false);

    if (ret_val && Z_TYPE_P(return_value) == IS_ARRAY &&
        zend_hash_num_elements(Z_ARRVAL_P(return_value)) == 0) {
        zval_dtor(return_value);  /* clean up the array */
        ZVAL_FALSE(return_value); /* set to boolean false */
    }
    efree(output);
    return ret_val;
}

/**
 * Process results for HINCRBYFLOAT
 */
int process_h_incrbyfloat_result(CommandResponse* response, void* output, zval* return_value) {
    /* Use command_response_to_zval to get the string result */
    zval   temp_result;
    double output_value;
    int    ret_val =
        command_response_to_zval(response, &temp_result, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    // php_var_dump(&temp_result, 2);
    if (ret_val) {
        if (Z_TYPE(temp_result) == IS_STRING) {
            /* Convert string to double */
            output_value = strtod(Z_STRVAL(temp_result), NULL);
        } else if (Z_TYPE(temp_result) == IS_DOUBLE) {
            output_value = Z_DVAL(temp_result);
        } else if (Z_TYPE(temp_result) == IS_LONG) {
            /* Convert long to double */
            output_value = (double) Z_LVAL(temp_result);
        } else {
            zval_dtor(&temp_result);
            return 0;
        }
        ZVAL_DOUBLE(return_value, output_value);
        zval_dtor(&temp_result);
        return 1;
    }

    zval_dtor(&temp_result);
    return 0;
}

/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/**
 * Convert zval array to command arguments with proper string conversion
 */
int convert_zval_array_to_args(zval*          z_array,
                               int            start_index,
                               uintptr_t*     args,
                               unsigned long* args_len,
                               char**         allocated_strings,
                               int*           allocated_count,
                               int            max_allocations) {
    int current_arg = start_index;

    for (int i = 0; i < max_allocations; i++) {
        zval*  value = &z_array[i];
        size_t str_len;
        int    need_free;

        char* str_val = zval_to_string_safe(value, &str_len, &need_free);

        args[current_arg]     = (uintptr_t) str_val;
        args_len[current_arg] = str_len;

        if (need_free) {
            allocated_strings[*allocated_count] = str_val;
            (*allocated_count)++;
        }

        current_arg++;
    }

    return current_arg;
}

/**
 * Process field-value pairs from associative array
 */
int process_field_value_pairs(zval*          field_values,
                              uintptr_t*     args,
                              unsigned long* args_len,
                              int            start_index,
                              char**         allocated_strings,
                              int*           allocated_count) {
    HashTable*   ht = Z_ARRVAL_P(field_values);
    zval*        data;
    zend_string* hash_key;
    zend_ulong   num_idx;
    int          arg_idx = start_index;

    ZEND_HASH_FOREACH_KEY_VAL(ht, num_idx, hash_key, data) {
        /* Add field */
        if (hash_key) {
            /* Associative array: key is the field */
            args[arg_idx]     = (uintptr_t) ZSTR_VAL(hash_key);
            args_len[arg_idx] = ZSTR_LEN(hash_key);
        } else {
            /* Numeric index - convert to string */
            char* field_str = long_to_string(num_idx, &args_len[arg_idx]);
            if (!field_str) {
                return 0;
            }
            args[arg_idx]                       = (uintptr_t) field_str;
            allocated_strings[*allocated_count] = field_str;
            (*allocated_count)++;
        }
        arg_idx++;

        /* Add value with enhanced type handling */
        size_t str_len;
        int    need_free;
        char*  str_val = NULL;

        /* Handle different zval types appropriately */
        switch (Z_TYPE_P(data)) {
            case IS_NULL:
                /* Convert NULL to empty string */
                str_val   = estrdup("");
                str_len   = 0;
                need_free = 1;
                break;

            case IS_FALSE:
                /* Convert false to "0" */
                str_val   = estrdup("0");
                str_len   = 1;
                need_free = 1;
                break;

            case IS_TRUE:
                /* Convert true to "1" */
                str_val   = estrdup("1");
                str_len   = 1;
                need_free = 1;
                break;

            case IS_ARRAY:
                /* Convert array to JSON representation */
                {
                    smart_str json_str = {0};

                    {
                        /* Fallback to "Array" if JSON encoding fails */
                        str_val   = estrdup("Array");
                        str_len   = 5;
                        need_free = 1;
                        if (json_str.s)
                            smart_str_free(&json_str);
                    }
                }
                break;

            case IS_OBJECT:
                /* Try to convert object to string */
                if (Z_OBJ_HT_P(data)->cast_object) {
                    zval tmp;
                    if (Z_OBJ_HT_P(data)->cast_object(Z_OBJ_P(data), &tmp, IS_STRING) == SUCCESS) {
                        str_val   = estrndup(Z_STRVAL(tmp), Z_STRLEN(tmp));
                        str_len   = Z_STRLEN(tmp);
                        need_free = 1;
                        zval_ptr_dtor(&tmp);
                    } else {
                        /* Fallback to object class name */
                        zend_string* class_name = Z_OBJCE_P(data)->name;
                        str_val   = estrndup(ZSTR_VAL(class_name), ZSTR_LEN(class_name));
                        str_len   = ZSTR_LEN(class_name);
                        need_free = 1;
                    }
                } else {
                    /* No cast_object handler, use class name */
                    zend_string* class_name = Z_OBJCE_P(data)->name;
                    str_val                 = estrndup(ZSTR_VAL(class_name), ZSTR_LEN(class_name));
                    str_len                 = ZSTR_LEN(class_name);
                    need_free               = 1;
                }
                break;

            case IS_RESOURCE:
                /* Convert resource to string representation */
                str_val   = estrdup("Resource");
                str_len   = 8;
                need_free = 1;
                break;

            default:
                /* Use standard conversion for strings, numbers, etc. */
                str_val = zval_to_string_safe(data, &str_len, &need_free);
                break;
        }

        if (!str_val) {
            /* Final fallback - should not happen with above handling */
            str_val   = estrdup("unknown");
            str_len   = 7;
            need_free = 1;
        }

        args[arg_idx]     = (uintptr_t) str_val;
        args_len[arg_idx] = str_len;

        if (need_free) {
            allocated_strings[*allocated_count] = str_val;
            (*allocated_count)++;
        }

        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    return arg_idx;
}

/**
 * Safe cleanup for allocated argument strings
 */
void cleanup_h_command_args(char**         allocated_strings,
                            int            allocated_count,
                            uintptr_t*     args,
                            unsigned long* args_len) {
    /* Free allocated strings */
    if (allocated_strings) {
        for (int i = 0; i < allocated_count; i++) {
            if (allocated_strings[i]) {
                efree(allocated_strings[i]);
            }
        }
        efree(allocated_strings);
    }

    /* Free argument arrays */
    if (args) {
        efree(args);
    }
    if (args_len) {
        efree(args_len);
    }
}

/* ====================================================================
 * HASH COMMAND EXECUTION FUNCTIONS
 * ==================================================================== */


/**
 * Execute HMSET command using the framework
 */
int execute_h_mset_command(valkey_glide_object* valkey_glide,
                           const char*          key,
                           size_t               key_len,
                           zval*                keyvals,
                           int                  keyvals_count,
                           zval*                return_value) {
    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field_values     = keyvals;
    args.fv_count         = keyvals_count;

    return execute_h_simple_command(valkey_glide, HMSet, &args, NULL, H_RESPONSE_OK, return_value);
}


/**
 * Execute HINCRBYFLOAT command using the framework
 */
int execute_h_incrbyfloat_command(valkey_glide_object* valkey_glide,
                                  const char*          key,
                                  size_t               key_len,
                                  char*                field,
                                  size_t               field_len,
                                  double               increment,
                                  zval*                return_value) {
    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.float_incr       = increment;

    return execute_h_generic_command(
        valkey_glide, HIncrByFloat, &args, NULL, process_h_incrbyfloat_result, return_value);
}

/**
 * Execute HMGET command using the framework
 */
int execute_h_mget_command(valkey_glide_object* valkey_glide,
                           const char*          key,
                           size_t               key_len,
                           zval*                fields,
                           int                  fields_count,
                           zval*                return_value) {
    h_command_args_t* args = ecalloc(1, sizeof(h_command_args_t));
    args->glide_client     = valkey_glide->glide_client;
    args->key              = key;
    args->key_len          = key_len;
    args->fields           = fields;
    args->field_count      = fields_count;

    return execute_h_generic_command(
        valkey_glide, HMGet, args, args, process_h_mget_result, return_value);
}


/**
 * Execute HRANDFIELD command using the framework
 */
int execute_h_randfield_command(valkey_glide_object* valkey_glide,
                                const char*          key,
                                size_t               key_len,
                                long                 count,
                                int                  withvalues,
                                zval*                return_value) {
    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.count            = count;
    args.withvalues       = withvalues;

    bool* withvalues_ptr = emalloc(sizeof(bool));
    *withvalues_ptr      = withvalues ? true : false;

    return execute_h_generic_command(
        valkey_glide, HRandField, &args, withvalues_ptr, process_h_randfield_result, return_value);
}

/* ====================================================================
 * UNIFIED HASH COMMAND EXECUTORS FOR MACROS
 * ==================================================================== */

/**
 * Execute HGET command with unified signature
 */
int execute_hget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL;
    size_t               key_len, field_len;
    char*                response     = NULL;
    size_t               response_len = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &field, &field_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;


    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HGet, &args, NULL, H_RESPONSE_STRING, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* Process the result */
        return 1;
    }

    return 0;
}

/**
 * Execute HLEN command with unified signature
 */
int execute_hlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;

    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HLen, &args, &result_value, H_RESPONSE_INT, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_int_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HEXISTS command with unified signature
 */
int execute_hexists_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL;
    size_t               key_len, field_len;
    int                  result;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &field, &field_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;

    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HExists, &args, &result, H_RESPONSE_BOOL, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_bool_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HDEL command with unified signature
 */
int execute_hdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                fields       = NULL;
    int                  fields_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &fields, &fields_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.fields           = fields;
    args.field_count      = fields_count;

    /* Execute with batch support */
    if (execute_h_simple_command(valkey_glide, HDel, &args, NULL, H_RESPONSE_INT, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_int_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HSET command with unified signature
 */
int execute_hset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_args = NULL;
    int                  arg_count;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &arg_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if we have a single array argument */
    int is_array_arg = 0;
    if (arg_count == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        is_array_arg = 1;
    }

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;
    args.field_values     = z_args;
    args.fv_count         = arg_count;
    args.is_array_arg     = is_array_arg;

    /* Execute with batch support */
    if (execute_h_simple_command(valkey_glide, HSet, &args, NULL, H_RESPONSE_INT, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/**
 * Execute HSETNX command with unified signature
 */
int execute_hsetnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL, *val = NULL;
    size_t               key_len, field_len, val_len;
    int                  result;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osss",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &field,
                                     &field_len,
                                     &val,
                                     &val_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }
    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.value            = val;
    args.value_len        = val_len;


    /* Execute the HSETNX command */
    if (execute_h_simple_command(
            valkey_glide, HSetNX, &args, NULL, H_RESPONSE_BOOL, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}

/**
 * Execute HMSET command with unified signature
 */
int execute_hmset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                arr_keyvals;
    HashTable*           keyvals_hash;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osa", &object, ce, &key, &key_len, &arr_keyvals) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the HMSET command */
    keyvals_hash      = Z_ARRVAL_P(arr_keyvals);
    int keyvals_count = zend_hash_num_elements(keyvals_hash) * 2;

    if (keyvals_count > 0) {
        if (execute_h_mset_command(
                valkey_glide, key, key_len, arr_keyvals, keyvals_count, return_value)) {
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

/**
 * Execute HINCRBY command with unified signature
 */
int execute_hincrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL;
    size_t               key_len, field_len;
    zend_long            increment;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Ossl", &object, ce, &key, &key_len, &field, &field_len, &increment) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.increment        = increment;


    /* Execute the HINCRBY command */
    if (execute_h_simple_command(
            valkey_glide, HIncrBy, &args, NULL, H_RESPONSE_INT, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}

/**
 * Execute HINCRBYFLOAT command with unified signature
 */
int execute_hincrbyfloat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL;
    size_t               key_len, field_len;
    double               increment;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Ossd", &object, ce, &key, &key_len, &field, &field_len, &increment) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the HINCRBYFLOAT command */
    if (execute_h_incrbyfloat_command(
            valkey_glide, key, key_len, field, field_len, increment, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}

/**
 * Execute HMGET command with unified signature
 */
int execute_hmget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                fields = NULL;
    HashTable*           fields_hash;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osa", &object, ce, &key, &key_len, &fields) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Process fields array similar to original implementation */
    fields_hash      = Z_ARRVAL_P(fields);
    int fields_count = zend_hash_num_elements(fields_hash);
    if (fields_count == 0) {
        return 0;
    }

    /* Count valid fields and create field array */
    int          valid_fields_count = 0;
    zval *       data, *real_data;
    zend_string* hash_key;
    zend_ulong   num_idx;

    ZEND_HASH_FOREACH_KEY_VAL(fields_hash, num_idx, hash_key, data) {
        real_data = Z_ISREF_P(data) ? Z_REFVAL_P(data) : data;
        if (hash_key || (Z_TYPE_P(real_data) == IS_STRING && Z_STRLEN_P(real_data) > 0) ||
            Z_TYPE_P(real_data) == IS_LONG || Z_TYPE_P(real_data) == IS_DOUBLE ||
            Z_TYPE_P(real_data) == IS_TRUE) {
            valid_fields_count++;
        }
    }
    ZEND_HASH_FOREACH_END();

    if (valid_fields_count == 0) {
        return 0;
    }

    /* Create field array */
    zval* field_array = ecalloc(fields_count, sizeof(zval));
    if (!field_array) {
        return 0;
    }

    /* Fill field array */
    int i = 0;
    ZEND_HASH_FOREACH_KEY_VAL(fields_hash, num_idx, hash_key, data) {
        zval* real_data = Z_ISREF_P(data) ? Z_REFVAL_P(data) : data;
        if (hash_key) {
            ZVAL_STR_COPY(&field_array[i], hash_key);
            i++;
        } else if (Z_TYPE_P(real_data) == IS_STRING && Z_STRLEN_P(real_data) > 0) {
            ZVAL_COPY(&field_array[i], real_data);
            i++;
        } else if (Z_TYPE_P(real_data) == IS_LONG || Z_TYPE_P(real_data) == IS_DOUBLE ||
                   Z_TYPE_P(real_data) == IS_TRUE) {
            ZVAL_COPY(&field_array[i], real_data);
            i++;
        }
    }
    ZEND_HASH_FOREACH_END();


    /* Execute the HMGET command */
    int result = execute_h_mget_command(valkey_glide, key, key_len, field_array, i, return_value);

    /* Handle batch mode */
    if (result && valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
    }

    return result;
}

/**
 * Execute HKEYS command with unified signature
 */
int execute_hkeys_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;


    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HKeys, &args, NULL, H_RESPONSE_ARRAY, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_array_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HVALS command with unified signature
 */
int execute_hvals_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;

    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HVals, &args, NULL, H_RESPONSE_ARRAY, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_array_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HGETALL command with unified signature
 */
int execute_hgetall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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

    /* Set up command args */
    h_command_args_t args = {0};
    args.key              = key;
    args.key_len          = key_len;


    /* Execute with batch support */
    if (execute_h_simple_command(
            valkey_glide, HGetAll, &args, NULL, H_RESPONSE_MAP, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        /* In non-batch mode, result is already set by process_h_map_result_async */
        return 1;
    }

    return 0;
}

/**
 * Execute HSTRLEN command with unified signature
 */
int execute_hstrlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL;
    size_t               key_len, field_len;
    long                 result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &field, &field_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute the HSTRLEN command */

    h_command_args_t args = {0};
    args.glide_client     = valkey_glide->glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;


    if (execute_h_simple_command(
            valkey_glide, HStrlen, &args, NULL, H_RESPONSE_INT, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}

/**
 * Execute HRANDFIELD command with unified signature
 */
int execute_hrandfield_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_opts     = NULL;
    zend_long            count      = 1;
    zend_bool            withvalues = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|a", &object, ce, &key, &key_len, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Process options if provided */
    if (z_opts) {
        HashTable* htopts  = Z_ARRVAL_P(z_opts);
        zval*      z_count = zend_hash_str_find(htopts, "count", sizeof("count") - 1);
        zval* z_withvalues = zend_hash_str_find(htopts, "withvalues", sizeof("withvalues") - 1);

        if (z_count) {
            count = zval_get_long(z_count);
        }

        if (z_withvalues) {
            withvalues = zval_is_true(z_withvalues);
        }
    }


    /* Execute the HRANDFIELD command */
    if (execute_h_randfield_command(valkey_glide, key, key_len, count, withvalues, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}
