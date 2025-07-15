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

extern zend_class_entry* ce;
extern zend_class_entry* get_valkey_glide_exception_ce();
/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

/**
 * Generic hash command execution framework
 */
int execute_h_generic_command(const void*          glide_client,
                              enum RequestType     cmd_type,
                              h_command_args_t*    args,
                              void*                result_ptr,
                              h_result_processor_t process_result) {
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            status            = 0;

    /* Validate basic arguments */
    VALIDATE_HASH_ARGS(glide_client, args->key);

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

    /* Execute the command */
    CommandResult* result = execute_command(glide_client, cmd_type, arg_count, cmd_args, args_len);

    /* Process result */
    if (result) {
        if (!result->command_error && result->response && process_result) {
            status = process_result(result, result_ptr);
        }
        free_command_result(result);
    }

cleanup:
    /* Clean up allocated resources */
    cleanup_h_command_args(allocated_strings, allocated_count, cmd_args, args_len);
    return status;
}

/**
 * Simplified execution for commands using standard response handlers
 */
int execute_h_simple_command(const void*       glide_client,
                             enum RequestType  cmd_type,
                             h_command_args_t* args,
                             void*             result_ptr,
                             int               response_type) {
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            status            = 0;

    /* Validate basic arguments */
    VALIDATE_HASH_ARGS(glide_client, args->key);

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

    /* Execute the command */
    CommandResult* result = execute_command(glide_client, cmd_type, arg_count, cmd_args, args_len);

    /* Process result using standard handlers */
    if (result) {
        switch (response_type) {
            case H_RESPONSE_INT:
                status = handle_int_response(result, (long*)result_ptr);
                break;
            case H_RESPONSE_STRING:
                status = handle_string_response(
                    result, ((char***)result_ptr)[0], ((size_t**)result_ptr)[1]);
                break;
            case H_RESPONSE_BOOL:
                status = handle_bool_response(result);
                if (status >= 0 && result_ptr) {
                    *((int*)result_ptr) = status;
                    status              = status >= 0 ? 1 : 0;
                }
                break;
            case H_RESPONSE_ARRAY:
                status = handle_array_response(result, (zval*)result_ptr);
                break;
            case H_RESPONSE_MAP:
                status = handle_map_response(result, (zval*)result_ptr);
                break;
            case H_RESPONSE_OK:
                status = handle_ok_response(result);
                break;
            default:
                free_command_result(result);
                status = 0;
                break;
        }
        /* Note: handle_* functions free the result internally */
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
    *args_out          = (uintptr_t*)emalloc(sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key as the only argument */
    (*args_out)[0]     = (uintptr_t)args->key;
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
    *args_out          = (uintptr_t*)emalloc(2 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(2 * sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key and field arguments */
    (*args_out)[0]     = (uintptr_t)args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t)args->field;
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
    *args_out          = (uintptr_t*)emalloc(3 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(3 * sizeof(unsigned long));
    *allocated_strings = NULL;
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key, field, and value arguments */
    (*args_out)[0]     = (uintptr_t)args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t)args->field;
    (*args_len_out)[1] = args->field_len;
    (*args_out)[2]     = (uintptr_t)args->value;
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
    *args_out          = (uintptr_t*)emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = (char**)emalloc(args->field_count * sizeof(char*));
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out || !*allocated_strings) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        if (*allocated_strings)
            efree(*allocated_strings);
        return 0;
    }

    /* Set key as first argument */
    (*args_out)[0]     = (uintptr_t)args->key;
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
        *args_out               = (uintptr_t*)emalloc(arg_count * sizeof(uintptr_t));
        *args_len_out           = (unsigned long*)emalloc(arg_count * sizeof(unsigned long));
        *allocated_strings      = (char**)emalloc((pairs_count * 2) * sizeof(char*));
        *allocated_count        = 0;

        if (!*args_out || !*args_len_out || !*allocated_strings) {
            if (*args_out)
                efree(*args_out);
            if (*args_len_out)
                efree(*args_len_out);
            if (*allocated_strings)
                efree(*allocated_strings);
            return 0;
        }

        /* First argument: key */
        (*args_out)[0]     = (uintptr_t)args->key;
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
        *args_out               = (uintptr_t*)emalloc(arg_count * sizeof(uintptr_t));
        *args_len_out           = (unsigned long*)emalloc(arg_count * sizeof(unsigned long));
        *allocated_strings      = (char**)emalloc(args->fv_count * sizeof(char*));
        *allocated_count        = 0;

        if (!*args_out || !*args_len_out || !*allocated_strings) {
            if (*args_out)
                efree(*args_out);
            if (*args_len_out)
                efree(*args_len_out);
            if (*allocated_strings)
                efree(*allocated_strings);
            return 0;
        }

        /* First argument: key */
        (*args_out)[0]     = (uintptr_t)args->key;
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

    *args_out          = (uintptr_t*)emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = (char**)emalloc((pairs_count * 2) * sizeof(char*));
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out || !*allocated_strings) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        if (*allocated_strings)
            efree(*allocated_strings);
        return 0;
    }

    /* First argument: key */
    (*args_out)[0]     = (uintptr_t)args->key;
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
    *args_out          = (uintptr_t*)emalloc(3 * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(3 * sizeof(unsigned long));
    *allocated_strings = (char**)emalloc(sizeof(char*));
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out || !*allocated_strings) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        if (*allocated_strings)
            efree(*allocated_strings);
        return 0;
    }

    /* Set key and field arguments */
    (*args_out)[0]     = (uintptr_t)args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t)args->field;
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

    if (!incr_str) {
        efree(*args_out);
        efree(*args_len_out);
        efree(*allocated_strings);
        return 0;
    }

    (*allocated_strings)[0] = incr_str;
    *allocated_count        = 1;

    (*args_out)[2]     = (uintptr_t)incr_str;
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

    *args_out          = (uintptr_t*)emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out      = (unsigned long*)emalloc(arg_count * sizeof(unsigned long));
    *allocated_strings = need_count_str ? (char**)emalloc(sizeof(char*)) : NULL;
    *allocated_count   = 0;

    if (!*args_out || !*args_len_out || (need_count_str && !*allocated_strings)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        if (*allocated_strings)
            efree(*allocated_strings);
        return 0;
    }

    /* First argument: key */
    int arg_idx              = 0;
    (*args_out)[arg_idx]     = (uintptr_t)args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add count if needed */
    if (need_count_str) {
        char* count_str = long_to_string(args->count, &(*args_len_out)[arg_idx]);
        if (!count_str) {
            efree(*args_out);
            efree(*args_len_out);
            if (*allocated_strings)
                efree(*allocated_strings);
            return 0;
        }

        (*allocated_strings)[0] = count_str;
        *allocated_count        = 1;

        (*args_out)[arg_idx] = (uintptr_t)count_str;
        arg_idx++;
    }

    /* Add WITHVALUES if specified */
    if (args->withvalues) {
        const char* withvalues_str = "WITHVALUES";
        (*args_out)[arg_idx]       = (uintptr_t)withvalues_str;
        (*args_len_out)[arg_idx]   = strlen(withvalues_str);
        arg_idx++;
    }

    return arg_count;
}

/* ====================================================================
 * RESULT PROCESSING FUNCTIONS
 * ==================================================================== */

/**
 * Process results for HMGET (associative field mapping)
 */
int process_h_mget_result(CommandResult* result, void* output) {
    h_command_args_t* args         = (h_command_args_t*)((void**)output)[0];
    zval*             return_value = (zval*)((void**)output)[1];

    /* Check if the command was successful */
    if (!result) {
        return 0;
    }

    /* Check if there was an error */
    if (result->command_error) {
        return 0;
    }

    /* Process the result - map back to original field names */
    int ret_val = 0;
    if (result->response && result->response->response_type == Array) {
        for (int i = 0; i < args->field_count && i < result->response->array_value_len; i++) {
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
            struct CommandResponse* element = &result->response->array_value[i];

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

    return ret_val;
}

/**
 * Process results for HRANDFIELD
 */
int process_h_randfield_result(CommandResult* result, void* output) {
    h_command_args_t* args         = (h_command_args_t*)((void**)output)[0];
    zval*             return_value = (zval*)((void**)output)[1];

    /* Check if the command was successful */
    if (!result) {
        return 0;
    }

    /* Check if there was an error */
    if (result->command_error) {
        return 0;
    }

    /* Process the result */
    int ret_val = 0;
    if (result->response) {
        /* Single field case */
        if (args->count == 1 && !args->withvalues) {
            if (result->response->response_type == String) {
                add_next_index_stringl(return_value,
                                       result->response->string_value,
                                       result->response->string_value_len);
                ret_val = 1;
            } else if (result->response->response_type == Null) {
                add_next_index_null(return_value);
                ret_val = 0;
            }
        }
        /* Multiple fields without values */
        else if (args->count != 1 && !args->withvalues &&
                 result->response->response_type == Array) {
            size_t i;
            for (i = 0; i < result->response->array_value_len; i++) {
                struct CommandResponse* element = &result->response->array_value[i];
                if (element->response_type == String) {
                    add_next_index_stringl(
                        return_value, element->string_value, element->string_value_len);
                } else if (element->response_type == Null) {
                    add_next_index_null(return_value);
                }
            }
            ret_val = 1;
        }
        /* Fields with values (associative) */
        else if (args->withvalues) {
            // ret_val = command_response_to_zval(result->response, return_value,
            // COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
            size_t i;
            for (i = 0; i < result->response->array_value_len; i++) {
                ret_val                         = 1;
                struct CommandResponse* element = &result->response->array_value[i];

                // Each element should be an array with a field and value
                if (element->response_type == Array && element->array_value_len == 2) {
                    struct CommandResponse* field = &element->array_value[0];
                    struct CommandResponse* value = &element->array_value[1];

                    if (field->response_type == String) {
                        if (value->response_type == String) {
                            add_assoc_stringl_ex(return_value,
                                                 field->string_value,
                                                 field->string_value_len,
                                                 value->string_value,
                                                 value->string_value_len);
                        } else if (value->response_type == Null) {
                            add_assoc_null_ex(
                                return_value, field->string_value, field->string_value_len);
                        } else if (value->response_type == Int) {
                            add_assoc_long_ex(return_value,
                                              field->string_value,
                                              field->string_value_len,
                                              value->int_value);
                        } else if (value->response_type == Float) {
                            add_assoc_double_ex(return_value,
                                                field->string_value,
                                                field->string_value_len,
                                                value->float_value);
                        } else if (value->response_type == Bool) {
                            add_assoc_bool_ex(return_value,
                                              field->string_value,
                                              field->string_value_len,
                                              value->bool_value);
                        }
                    }
                }
            }
        }
    }

    return ret_val;
}

/**
 * Process results for HINCRBYFLOAT
 */
int process_h_incrbyfloat_result(CommandResult* result, void* output) {
    double* output_value = (double*)output;

    /* Use command_response_to_zval to get the string result */
    zval temp_result;
    int  ret_val = command_response_to_zval(
        result->response, &temp_result, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    // php_var_dump(&temp_result, 2);
    if (ret_val) {
        if (Z_TYPE(temp_result) == IS_STRING) {
            /* Convert string to double */
            *output_value = strtod(Z_STRVAL(temp_result), NULL);
        } else if (Z_TYPE(temp_result) == IS_DOUBLE) {
            *output_value = Z_DVAL(temp_result);
        } else if (Z_TYPE(temp_result) == IS_LONG) {
            /* Convert long to double */
            *output_value = (double)Z_LVAL(temp_result);
        } else {
            zval_dtor(&temp_result);
            return 0;
        }

        zval_dtor(&temp_result);
        return 1;
    }

    zval_dtor(&temp_result);
    return 0;
}

/**
 * Process results for HGETALL (convert flat array to associative)
 */
int process_h_getall_result(CommandResult* result, void* output) {
    zval* return_value = (zval*)output;

    /* Check if the command was successful */
    if (!result || result->command_error) {
        return 0;
    }

    /* Convert response to associative array */
    return command_response_to_zval(
        result->response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
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
        if (!str_val) {
            return 0;
        }

        args[current_arg]     = (uintptr_t)str_val;
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
            args[arg_idx]     = (uintptr_t)ZSTR_VAL(hash_key);
            args_len[arg_idx] = ZSTR_LEN(hash_key);
        } else {
            /* Numeric index - convert to string */
            char* field_str = long_to_string(num_idx, &args_len[arg_idx]);
            if (!field_str) {
                return 0;
            }
            args[arg_idx]                       = (uintptr_t)field_str;
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

        args[arg_idx]     = (uintptr_t)str_val;
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
 * Execute HGET command using the framework
 */
int execute_h_get_command(const void* glide_client,
                          const char* key,
                          size_t      key_len,
                          char*       field,
                          size_t      field_len,
                          char**      result,
                          size_t*     result_len) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;

    void* result_ptr[2] = {(void*)result, (void*)result_len};
    return execute_h_simple_command(glide_client, HGet, &args, result_ptr, H_RESPONSE_STRING);
}

/**
 * Execute HLEN command using the framework
 */
int execute_h_len_command(const void* glide_client,
                          const char* key,
                          size_t      key_len,
                          long*       output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;

    return execute_h_simple_command(glide_client, HLen, &args, output_value, H_RESPONSE_INT);
}

/**
 * Execute HEXISTS command using the framework
 */
int execute_h_exists_command(const void* glide_client,
                             const char* key,
                             size_t      key_len,
                             char*       field,
                             size_t      field_len,
                             int*        output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;

    return execute_h_simple_command(glide_client, HExists, &args, output_value, H_RESPONSE_BOOL);
}

/**
 * Execute HDEL command using the framework
 */
int execute_h_del_command(const void* glide_client,
                          const char* key,
                          size_t      key_len,
                          zval*       fields,
                          int         fields_count,
                          long*       output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.fields           = fields;
    args.field_count      = fields_count;

    return execute_h_simple_command(glide_client, HDel, &args, output_value, H_RESPONSE_INT);
}

/**
 * Execute HSET command using the framework
 */
int execute_h_set_command(const void* glide_client,
                          const char* key,
                          size_t      key_len,
                          zval*       z_args,
                          int         argc,
                          long*       output_value,
                          int         is_array_arg) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field_values     = z_args;
    args.fv_count         = argc;
    args.is_array_arg     = is_array_arg;

    return execute_h_simple_command(glide_client, HSet, &args, output_value, H_RESPONSE_INT);
}

/**
 * Execute HSETNX command using the framework
 */
int execute_h_setnx_command(const void* glide_client,
                            const char* key,
                            size_t      key_len,
                            char*       field,
                            size_t      field_len,
                            char*       value,
                            size_t      value_len,
                            int*        output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.value            = value;
    args.value_len        = value_len;

    return execute_h_simple_command(glide_client, HSetNX, &args, output_value, H_RESPONSE_BOOL);
}

/**
 * Execute HMSET command using the framework
 */
int execute_h_mset_command(
    const void* glide_client, const char* key, size_t key_len, zval* keyvals, int keyvals_count) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field_values     = keyvals;
    args.fv_count         = keyvals_count;

    return execute_h_simple_command(glide_client, HMSet, &args, NULL, H_RESPONSE_OK);
}

/**
 * Execute HINCRBY command using the framework
 */
int execute_h_incrby_command(const void* glide_client,
                             const char* key,
                             size_t      key_len,
                             char*       field,
                             size_t      field_len,
                             long        increment,
                             long*       output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.increment        = increment;

    return execute_h_simple_command(glide_client, HIncrBy, &args, output_value, H_RESPONSE_INT);
}

/**
 * Execute HINCRBYFLOAT command using the framework
 */
int execute_h_incrbyfloat_command(const void* glide_client,
                                  const char* key,
                                  size_t      key_len,
                                  char*       field,
                                  size_t      field_len,
                                  double      increment,
                                  double*     output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;
    args.float_incr       = increment;

    return execute_h_generic_command(
        glide_client, HIncrByFloat, &args, output_value, process_h_incrbyfloat_result);
}

/**
 * Execute HMGET command using the framework
 */
int execute_h_mget_command(const void* glide_client,
                           const char* key,
                           size_t      key_len,
                           zval*       fields,
                           int         fields_count,
                           zval*       return_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.fields           = fields;
    args.field_count      = fields_count;

    void* output[2] = {&args, return_value};
    return execute_h_generic_command(glide_client, HMGet, &args, output, process_h_mget_result);
}

/**
 * Execute HKEYS command using the framework
 */
int execute_h_keys_command(const void* glide_client,
                           const char* key,
                           size_t      key_len,
                           zval*       return_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;

    return execute_h_simple_command(glide_client, HKeys, &args, return_value, H_RESPONSE_ARRAY);
}

/**
 * Execute HVALS command using the framework
 */
int execute_h_vals_command(const void* glide_client,
                           const char* key,
                           size_t      key_len,
                           zval*       return_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;

    return execute_h_simple_command(glide_client, HVals, &args, return_value, H_RESPONSE_ARRAY);
}

/**
 * Execute HGETALL command using the framework
 */
int execute_h_getall_command(const void* glide_client,
                             const char* key,
                             size_t      key_len,
                             zval*       return_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;

    return execute_h_generic_command(
        glide_client, HGetAll, &args, return_value, process_h_getall_result);
}

/**
 * Execute HSTRLEN command using the framework
 */
int execute_h_strlen_command(const void* glide_client,
                             const char* key,
                             size_t      key_len,
                             char*       field,
                             size_t      field_len,
                             long*       output_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.field            = field;
    args.field_len        = field_len;

    return execute_h_simple_command(glide_client, HStrlen, &args, output_value, H_RESPONSE_INT);
}

/**
 * Execute HRANDFIELD command using the framework
 */
int execute_h_randfield_command(const void* glide_client,
                                const char* key,
                                size_t      key_len,
                                long        count,
                                int         withvalues,
                                zval*       return_value) {
    h_command_args_t args = {0};
    args.glide_client     = glide_client;
    args.key              = key;
    args.key_len          = key_len;
    args.count            = count;
    args.withvalues       = withvalues;

    void* output[2] = {&args, return_value};
    return execute_h_generic_command(
        glide_client, HRandField, &args, output, process_h_randfield_result);
}

/* ====================================================================
 * UNIFIED HASH COMMAND EXECUTORS FOR MACROS
 * ==================================================================== */

/**
 * Execute HGET command with unified signature
 */
int execute_hget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *field = NULL, *response = NULL;
    size_t               key_len, field_len, response_len     = 0;

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

    /* Execute the HGET command */
    int result = execute_h_get_command(
        valkey_glide->glide_client, key, key_len, field, field_len, &response, &response_len);

    /* Process the result */
    if (result == 1 && response != NULL) {
        ZVAL_STRINGL(return_value, response, response_len);
        efree(response);
        return 1;
    } else if (result == 0) {
        ZVAL_FALSE(return_value);
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

    /* Execute the HLEN command */
    if (execute_h_len_command(valkey_glide->glide_client, key, key_len, &result_value)) {
        ZVAL_LONG(return_value, result_value);
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

    /* Execute the HEXISTS command */
    if (execute_h_exists_command(
            valkey_glide->glide_client, key, key_len, field, field_len, &result)) {
        ZVAL_BOOL(return_value, result == 1);
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
    long                 result_value;

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

    /* Execute the HDEL command */
    if (execute_h_del_command(
            valkey_glide->glide_client, key, key_len, fields, fields_count, &result_value)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    /* Execute the HSET command */
    if (execute_h_set_command(valkey_glide->glide_client,
                              key,
                              key_len,
                              z_args,
                              arg_count,
                              &result_value,
                              is_array_arg)) {
        ZVAL_LONG(return_value, result_value);
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

    /* Execute the HSETNX command */
    if (execute_h_setnx_command(
            valkey_glide->glide_client, key, key_len, field, field_len, val, val_len, &result)) {
        ZVAL_BOOL(return_value, result == 1);
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
                valkey_glide->glide_client, key, key_len, arr_keyvals, keyvals_count)) {
            ZVAL_TRUE(return_value);
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
    long                 result_value;

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

    /* Execute the HINCRBY command */
    if (execute_h_incrby_command(
            valkey_glide->glide_client, key, key_len, field, field_len, increment, &result_value)) {
        ZVAL_LONG(return_value, result_value);
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
    double               increment, result;

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
            valkey_glide->glide_client, key, key_len, field, field_len, increment, &result)) {
        ZVAL_DOUBLE(return_value, result);
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

    /* Initialize return array */
    array_init(return_value);

    /* Execute the HMGET command */
    int result = execute_h_mget_command(
        valkey_glide->glide_client, key, key_len, field_array, i, return_value);

    /* Free field array */
    for (int j = 0; j < i; j++) {
        zval_ptr_dtor(&field_array[j]);
    }
    efree(field_array);

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

    /* Initialize return array */
    array_init(return_value);

    /* Execute the HKEYS command */
    return execute_h_keys_command(valkey_glide->glide_client, key, key_len, return_value);
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

    /* Initialize return array */
    array_init(return_value);

    /* Execute the HVALS command */
    return execute_h_vals_command(valkey_glide->glide_client, key, key_len, return_value);
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

    /* Initialize return array */
    array_init(return_value);

    /* Execute the HGETALL command */
    return execute_h_getall_command(valkey_glide->glide_client, key, key_len, return_value);
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
    if (execute_h_strlen_command(
            valkey_glide->glide_client, key, key_len, field, field_len, &result_value)) {
        ZVAL_LONG(return_value, result_value);
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

    /* Initialize return array */
    array_init(return_value);

    /* Execute the HRANDFIELD command */
    if (execute_h_randfield_command(
            valkey_glide->glide_client, key, key_len, count, withvalues, return_value)) {
        /* If count is 1 and not withvalues, return single value */
        if (count == 1 && !withvalues && zend_hash_num_elements(Z_ARRVAL_P(return_value)) == 1) {
            zval *z_ele, z_copy;
            zend_hash_internal_pointer_reset(Z_ARRVAL_P(return_value));
            z_ele = zend_hash_get_current_data(Z_ARRVAL_P(return_value));
            if (z_ele) {
                ZVAL_COPY(&z_copy, z_ele);
                zval_dtor(return_value);
                ZVAL_COPY_VALUE(return_value, &z_copy);
            }
        }
        return 1;
    }

    return 0;
}
