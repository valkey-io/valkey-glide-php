/*
  +----------------------------------------------------------------------+
  | ValkeyGlide Glide S-Commands Common Utilities                              |
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
#include "valkey_glide_s_common.h"

#include "cluster_scan_cursor.h"
#include "command_response.h"
#include "common.h"

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/**
 * Allocate command arguments arrays
 */
int allocate_s_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out) {
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
void cleanup_s_command_args(uintptr_t* args, unsigned long* args_len) {
    if (args)
        efree(args);
    if (args_len)
        efree(args_len);
}

/**
 * Convert array of zvals to string arguments
 */
int convert_zval_to_string_args(
    zval* input, int count, uintptr_t** args_out, unsigned long** args_len_out, int offset) {
    int  i;
    zval temp;

    for (i = 0; i < count; i++) {
        zval* element = &input[i];

        if (Z_TYPE_P(element) == IS_STRING) {
            (*args_out)[offset + i]     = (uintptr_t) Z_STRVAL_P(element);
            (*args_len_out)[offset + i] = Z_STRLEN_P(element);
        } else if (Z_TYPE_P(element) == IS_OBJECT) {
            /* Handle object conversion specially */
            zval tmp_zval;
            ZVAL_COPY(&tmp_zval, element);
            zend_std_cast_object_tostring(&tmp_zval, &tmp_zval, IS_STRING);

            /* Create a permanent copy of the string before freeing the zval */
            size_t str_len  = Z_STRLEN(tmp_zval);
            char*  str_copy = emalloc(str_len + 1);
            memcpy(str_copy, Z_STRVAL(tmp_zval), str_len);
            str_copy[str_len] = '\0';

            (*args_out)[offset + i]     = (uintptr_t) str_copy;
            (*args_len_out)[offset + i] = str_len;
            zval_dtor(&tmp_zval);
        } else {
            /* Convert other types to string */
            ZVAL_COPY(&temp, element);
            convert_to_string(&temp);

            /* Create a permanent copy of the string before freeing the zval */
            size_t str_len  = Z_STRLEN(temp);
            char*  str_copy = emalloc(str_len + 1);
            memcpy(str_copy, Z_STRVAL(temp), str_len);
            str_copy[str_len] = '\0';

            (*args_out)[offset + i]     = (uintptr_t) str_copy;
            (*args_len_out)[offset + i] = str_len;
            zval_dtor(&temp);
        }
    }

    return 1;
}

/**
 * Allocate a string representation of a long value
 */
char* alloc_long_string(long value, size_t* len_out) {
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

/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */

/**
 * Prepare arguments for key + members commands (SADD, SREM, SMISMEMBER)
 */
int prepare_s_key_members_args(s_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out) {
    if (!args->glide_client || !args->key || args->key_len == 0 || !args->members ||
        args->members_count <= 0) {
        return 0;
    }

    unsigned long arg_count = 1 + args->members_count; /* key + members */

    if (!allocate_s_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set key as first argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Convert and set member arguments */
    convert_zval_to_string_args(args->members, args->members_count, args_out, args_len_out, 1);

    return arg_count;
}

/**
 * Prepare arguments for key-only commands (SCARD, SMEMBERS)
 */
int prepare_s_key_only_args(s_command_args_t* args,
                            uintptr_t**       args_out,
                            unsigned long**   args_len_out) {
    if (!args->glide_client || !args->key || args->key_len == 0) {
        return 0;
    }

    if (!allocate_s_command_args(1, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    return 1;
}

/**
 * Prepare arguments for key + member commands (SISMEMBER)
 */
int prepare_s_key_member_args(s_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out) {
    if (!args->glide_client || !args->key || args->key_len == 0 || !args->member ||
        args->member_len == 0) {
        return 0;
    }

    if (!allocate_s_command_args(2, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;
    (*args_out)[1]     = (uintptr_t) args->member;
    (*args_len_out)[1] = args->member_len;

    return 2;
}

/**
 * Prepare arguments for key + count commands (SPOP, SRANDMEMBER)
 */
int prepare_s_key_count_args(s_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out) {
    if (!args->glide_client || !args->key || args->key_len == 0) {
        return 0;
    }

    unsigned long arg_count = args->has_count ? 2 : 1;

    if (!allocate_s_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    if (args->has_count) {
        char* count_str = alloc_long_string(args->count, NULL);
        if (!count_str) {
            cleanup_s_command_args(*args_out, *args_len_out);
            return 0;
        }
        (*args_out)[1]     = (uintptr_t) count_str;
        (*args_len_out)[1] = strlen(count_str);
    }

    return arg_count;
}

/**
 * Prepare arguments for multi-key commands (SINTER, SUNION, SDIFF)
 */
int prepare_s_multi_key_args(s_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out) {
    if (!args->glide_client || !args->keys || args->keys_count <= 0) {
        return 0;
    }

    if (!allocate_s_command_args(args->keys_count, args_out, args_len_out)) {
        return 0;
    }

    convert_zval_to_string_args(args->keys, args->keys_count, args_out, args_len_out, 0);

    return args->keys_count;
}

/**
 * Prepare arguments for multi-key + limit commands (SINTERCARD)
 */
int prepare_s_multi_key_limit_args(s_command_args_t* args,
                                   uintptr_t**       args_out,
                                   unsigned long**   args_len_out) {
    if (!args->glide_client || !args->keys || args->keys_count <= 0) {
        return 0;
    }

    unsigned long arg_count =
        1 + args->keys_count + (args->has_limit ? 2 : 0); /* numkeys + keys + [LIMIT value] */

    if (!allocate_s_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* First argument is the number of keys */
    char* numkeys_str = alloc_long_string(args->keys_count, NULL);
    if (!numkeys_str) {
        cleanup_s_command_args(*args_out, *args_len_out);
        return 0;
    }
    (*args_out)[0]     = (uintptr_t) numkeys_str;
    (*args_len_out)[0] = strlen(numkeys_str);

    /* Add keys */
    convert_zval_to_string_args(args->keys, args->keys_count, args_out, args_len_out, 1);

    /* Add LIMIT if specified */
    if (args->has_limit) {
        (*args_out)[1 + args->keys_count]     = (uintptr_t) "LIMIT";
        (*args_len_out)[1 + args->keys_count] = 5;

        char* limit_str = alloc_long_string(args->limit, NULL);
        if (!limit_str) {
            efree((void*) (*args_out)[0]); /* Free numkeys_str */
            cleanup_s_command_args(*args_out, *args_len_out);
            return 0;
        }
        (*args_out)[2 + args->keys_count]     = (uintptr_t) limit_str;
        (*args_len_out)[2 + args->keys_count] = strlen(limit_str);
    }

    return arg_count;
}

/**
 * Prepare arguments for destination + multi-key commands (SINTERSTORE, SUNIONSTORE, SDIFFSTORE)
 */
int prepare_s_dst_multi_key_args(s_command_args_t* args,
                                 uintptr_t**       args_out,
                                 unsigned long**   args_len_out) {
    if (!args->glide_client || !args->dst_key || args->dst_key_len == 0 || !args->keys ||
        args->keys_count <= 0) {
        return 0;
    }

    unsigned long arg_count = 1 + args->keys_count; /* destination + keys */

    if (!allocate_s_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set destination key */
    (*args_out)[0]     = (uintptr_t) args->dst_key;
    (*args_len_out)[0] = args->dst_key_len;

    /* Add source keys */
    convert_zval_to_string_args(args->keys, args->keys_count, args_out, args_len_out, 1);

    return arg_count;
}

/**
 * Prepare arguments for two-key + member commands (SMOVE)
 */
int prepare_s_two_key_member_args(s_command_args_t* args,
                                  uintptr_t**       args_out,
                                  unsigned long**   args_len_out) {
    if (!args->glide_client || !args->src_key || args->src_key_len == 0 || !args->dst_key ||
        args->dst_key_len == 0 || !args->member || args->member_len == 0) {
        return 0;
    }

    if (!allocate_s_command_args(3, args_out, args_len_out)) {
        return 0;
    }

    (*args_out)[0]     = (uintptr_t) args->src_key;
    (*args_len_out)[0] = args->src_key_len;
    (*args_out)[1]     = (uintptr_t) args->dst_key;
    (*args_len_out)[1] = args->dst_key_len;
    (*args_out)[2]     = (uintptr_t) args->member;
    (*args_len_out)[2] = args->member_len;

    return 3;
}

/**
 * Prepare arguments for scan commands (SCAN, SSCAN)
 */
int prepare_s_scan_args(s_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out) {
    if (!args->glide_client || !args->cursor) {
        return 0;
    }

    int has_pattern = (args->pattern && args->pattern_len > 0);
    int has_count   = args->has_count;
    int has_key     = (args->key && args->key_len > 0);                     /* For SSCAN */
    int has_type    = args->has_type && (args->type && args->type_len > 0); /* For SCAN only */

    unsigned long arg_count =
        (has_key ? 1 : 0) + 1 + (has_pattern ? 2 : 0) + (has_count ? 2 : 0) + (has_type ? 2 : 0);

    if (!allocate_s_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    int arg_idx = 0;

    /* Add key if this is SSCAN */
    if (has_key) {
        (*args_out)[arg_idx]     = (uintptr_t) args->key;
        (*args_len_out)[arg_idx] = args->key_len;
        arg_idx++;
    }

    /* Add cursor */

    size_t cursor_len = strlen(*args->cursor);

    (*args_out)[arg_idx]     = (uintptr_t) *args->cursor;
    (*args_len_out)[arg_idx] = cursor_len;
    arg_idx++;

    /* Add MATCH pattern if provided */
    if (has_pattern) {
        (*args_out)[arg_idx]     = (uintptr_t) "MATCH";
        (*args_len_out)[arg_idx] = 5;
        arg_idx++;
        (*args_out)[arg_idx]     = (uintptr_t) args->pattern;
        (*args_len_out)[arg_idx] = args->pattern_len;
        arg_idx++;
    }

    /* Add COUNT if provided */
    if (has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = 5;
        arg_idx++;

        char* count_str = alloc_long_string(args->count, NULL);
        if (!count_str) {
            efree((void*) (*args_out)[has_key ? 1 : 0]); /* Free cursor_str */
            cleanup_s_command_args(*args_out, *args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]     = (uintptr_t) count_str;
        (*args_len_out)[arg_idx] = strlen(count_str);
        arg_idx++;
    }

    /* Add TYPE if provided (SCAN only) */
    if (has_type) {
        (*args_out)[arg_idx]     = (uintptr_t) "TYPE";
        (*args_len_out)[arg_idx] = 4;
        arg_idx++;
        (*args_out)[arg_idx]     = (uintptr_t) args->type;
        (*args_len_out)[arg_idx] = args->type_len;
        arg_idx++;
    }

    return arg_count;
}


/* ====================================================================
 * RESPONSE PROCESSING FUNCTIONS
 * ==================================================================== */

/**
 * Process integer response
 */
int process_s_int_response(CommandResult* result, s_command_args_t* args, zval* return_value) {
    if (result && result->response && !result->command_error) {
        if (result->response->response_type == Int) {
            if (args->output_long) {
                *args->output_long = result->response->int_value;
                return 1;
            }
            if (return_value) {
                ZVAL_LONG(return_value, result->response->int_value);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Process boolean response
 */
int process_s_bool_response(CommandResult* result, s_command_args_t* args, zval* return_value) {
    if (result && result->response && !result->command_error) {
        if (result->response->response_type == Bool) {
            int bool_val = result->response->bool_value ? 1 : 0;
            if (args->output_int) {
                *args->output_int = bool_val;
                return 1;
            }
            if (return_value) {
                ZVAL_BOOL(return_value, bool_val);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Process set/array response
 */
int process_s_set_response(CommandResult* result, s_command_args_t* args, zval* return_value) {
    if (!result || !return_value) {
        return 0;
    }

    /* Check if there was an error */
    if (result->command_error) {
        return 0;
    }

    /* Process the result */
    if (result->response) {
        if (result->response->response_type == Null) {
            ZVAL_NULL(return_value);
            return 0;
        } else if (result->response->response_type == Sets) {
            return command_response_to_zval(
                result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        }
    }

    return 0;
}

/**
 * Process mixed response (string or array)
 */
int process_s_mixed_response(CommandResult* result, s_command_args_t* args, zval* return_value) {
    if (result && result->response && !result->command_error && return_value) {
        return command_response_to_zval(
            result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    }
    return 0;
}


/**
 * Process scan response (cursor + array) - Updated for string cursors
 * Refactored to use command_response_to_zval utility for better robustness
 */
int process_s_scan_response(CommandResult*    result,
                            enum RequestType  cmd_type,
                            s_command_args_t* args,
                            zval*             return_value) {
    if (!result || !result->response || result->command_error || !return_value || !args->cursor) {
        return 0;
    }

    /* Validate response structure: should be Array with at least 2 elements [cursor, elements] */
    if (result->response->response_type != Array || result->response->array_value_len < 2) {
        return 0;
    }

    /* Extract cursor from first element */
    CommandResponse* cursor_resp    = &result->response->array_value[0];
    const char*      new_cursor_str = NULL;
    if (cursor_resp->response_type == String) {
        new_cursor_str = cursor_resp->string_value;
    } else {
        /* Handle unexpected cursor type */
        fprintf(stderr,
                "[SCAN_DEBUG] process_s_scan_response: Unexpected cursor type %d\n",
                cursor_resp->response_type);
        return 0;
    }

    /* Extract elements from second element */
    CommandResponse* elements_resp = &result->response->array_value[1];
    if (elements_resp->response_type != Array) {
        return 0;
    }

    /* Handle scan completion: when server returns cursor="0", scan is complete */
    if (cursor_resp->string_value_len == 1 && cursor_resp->string_value[0] == '0') {
        /* Free old cursor and keep it as "0" to indicate completion */
        if (*args->cursor) {
            efree(*args->cursor);
        }
        *args->cursor = emalloc(2);
        strcpy(*args->cursor, "0");


        /* If there are elements in this final batch, return them using robust conversion */
        if (elements_resp->array_value_len > 0) {
            return command_response_to_zval(elements_resp,
                                            return_value,
                                            (cmd_type == HScan || cmd_type == ZScan)
                                                ? COMMAND_RESPONSE_SCAN_ASSOSIATIVE_ARRAY
                                                : COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                            false);
        } else {
            /* No elements in final batch - return FALSE to terminate loop */
            array_init(return_value);
            return 1;
        }
    }

    /* Normal case: cursor != "0", update cursor string and return elements array */
    if (*args->cursor) {
        efree(*args->cursor);
    }

    /* Use length-controlled string copying to prevent reading beyond string boundary */
    size_t cursor_len = cursor_resp->string_value_len;
    *args->cursor     = emalloc(cursor_len + 1);
    memcpy(*args->cursor, new_cursor_str, cursor_len);
    (*args->cursor)[cursor_len] = '\0';


    /* Use command_response_to_zval for robust element processing */
    return command_response_to_zval(elements_resp,
                                    return_value,
                                    (cmd_type == HScan || cmd_type == ZScan)
                                        ? COMMAND_RESPONSE_SCAN_ASSOSIATIVE_ARRAY
                                        : COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                    false);
}

/* ====================================================================
 * CORE EXECUTION FRAMEWORK
 * ==================================================================== */

/**
 * Generic command execution for S commands
 */
int execute_s_generic_command(const void*          glide_client,
                              enum RequestType     cmd_type,
                              s_command_category_t category,
                              s_response_type_t    response_type,
                              s_command_args_t*    args,
                              zval*                return_value) {
    uintptr_t*     cmd_args  = NULL;
    unsigned long* args_len  = NULL;
    int            arg_count = 0;
    int            status    = 0;
    CommandResult* result    = NULL;

    /* Validate basic parameters */
    if (!glide_client || !args) {
        return 0;
    }

    /* Prepare arguments based on category */
    switch (category) {
        case S_CMD_KEY_MEMBERS:
            arg_count = prepare_s_key_members_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_KEY_ONLY:
            arg_count = prepare_s_key_only_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_KEY_MEMBER:
            arg_count = prepare_s_key_member_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_KEY_COUNT:
            arg_count = prepare_s_key_count_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_MULTI_KEY:
            arg_count = prepare_s_multi_key_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_MULTI_KEY_LIMIT:
            arg_count = prepare_s_multi_key_limit_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_DST_MULTI_KEY:
            arg_count = prepare_s_dst_multi_key_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_TWO_KEY_MEMBER:
            arg_count = prepare_s_two_key_member_args(args, &cmd_args, &args_len);
            break;
        case S_CMD_SCAN:
            arg_count = prepare_s_scan_args(args, &cmd_args, &args_len);
            break;
        default:
            return 0;
    }

    if (arg_count <= 0 || !cmd_args || !args_len) {
        goto cleanup;
    }

    /* Execute the command */
    result = execute_command(glide_client, cmd_type, arg_count, cmd_args, args_len);

    /* Process response based on type */
    if (result) {
        switch (response_type) {
            case S_RESPONSE_INT:
                status = process_s_int_response(result, args, return_value);
                break;
            case S_RESPONSE_BOOL:
                status = process_s_bool_response(result, args, return_value);
                break;
            case S_RESPONSE_SET:
                status = process_s_set_response(result, args, return_value);
                break;
            case S_RESPONSE_MIXED:
                status = process_s_mixed_response(result, args, return_value);
                break;

            case S_RESPONSE_SCAN:
                status = process_s_scan_response(result, cmd_type, args, return_value);
                break;
            default:
                status = 0;
                break;
        }

        free_command_result(result);
    }

cleanup:
    /* Clean up allocated strings for specific categories */
    if (cmd_args && args_len) {
        if (category == S_CMD_KEY_COUNT && args->has_count && arg_count > 1) {
            efree((void*) cmd_args[1]); /* Free count string */
        } else if (category == S_CMD_MULTI_KEY_LIMIT) {
            efree((void*) cmd_args[0]); /* Free numkeys string */
            if (args->has_limit && arg_count > 2 + args->keys_count) {
                efree((void*) cmd_args[2 + args->keys_count]); /* Free limit string */
            }
        } else if (category == S_CMD_SCAN) {
            /* No need to free cursor string anymore - it's directly referenced */
            if (args->has_count) {
                /* Find and free count string */
                int has_key   = (args->key && args->key_len > 0);
                int count_idx = (has_key ? 1 : 0) + 1 + (args->pattern ? 2 : 0) + 1;
                if (count_idx < arg_count) {
                    efree((void*) cmd_args[count_idx]);
                }
            }
        }

        /* Clean up allocated strings from convert_zval_to_string_args for member-based commands */
        if (category == S_CMD_KEY_MEMBERS || category == S_CMD_MULTI_KEY ||
            category == S_CMD_DST_MULTI_KEY) {
            int start_idx = 0, count = 0;

            if (category == S_CMD_KEY_MEMBERS) {
                start_idx = 1; /* Skip key, clean up member strings */
                count     = args->members_count;
            } else if (category == S_CMD_MULTI_KEY) {
                start_idx = 0; /* Clean up all key strings */
                count     = args->keys_count;
            } else if (category == S_CMD_DST_MULTI_KEY) {
                start_idx = 1; /* Skip destination key, clean up source key strings */
                count     = args->keys_count;
            }

            /* Free converted strings - check if they were allocated by our conversion function */
            for (int i = 0; i < count && (start_idx + i) < arg_count; i++) {
                /* Only free if it's not pointing to original string data */
                if (args->members && category == S_CMD_KEY_MEMBERS) {
                    zval* element = &args->members[i];
                    if (Z_TYPE_P(element) != IS_STRING && cmd_args[start_idx + i] != 0) {
                        efree((void*) cmd_args[start_idx + i]);
                    }
                } else if (args->keys &&
                           (category == S_CMD_MULTI_KEY || category == S_CMD_DST_MULTI_KEY)) {
                    zval* element = &args->keys[i];
                    if (Z_TYPE_P(element) != IS_STRING && cmd_args[start_idx + i] != 0) {
                        efree((void*) cmd_args[start_idx + i]);
                    }
                }
            }
        }
    }

    cleanup_s_command_args(cmd_args, args_len);
    return status;
}

/* ====================================================================
 * WRAPPER FUNCTIONS FOR EXISTING COMMANDS
 * ==================================================================== */

/**
 * Execute SADD command using the new signature pattern
 */
int execute_sadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_args;
    int                  members_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os+", &object, ce, &key, &key_len, &z_args, &members_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client  = valkey_glide->glide_client;
        args.key           = key;
        args.key_len       = key_len;
        args.members       = z_args;
        args.members_count = members_count;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SAdd,
                                      S_CMD_KEY_MEMBERS,
                                      S_RESPONSE_INT,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}


/**
 * Execute SCARD command using the new signature pattern
 */
int execute_scard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.key          = key;
        args.key_len      = key_len;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SCard,
                                      S_CMD_KEY_ONLY,
                                      S_RESPONSE_INT,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SRANDMEMBER command using the new signature pattern
 */
int execute_srandmember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.key          = key;
        args.key_len      = key_len;
        args.count        = has_count ? count : 1;
        args.has_count    = has_count;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SRandMember,
                                      S_CMD_KEY_COUNT,
                                      S_RESPONSE_MIXED,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SISMEMBER command using the new signature pattern
 */
int execute_sismember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *member = NULL;
    size_t               key_len, member_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &member, &member_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.key          = key;
        args.key_len      = key_len;
        args.member       = member;
        args.member_len   = member_len;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SIsMember,
                                      S_CMD_KEY_MEMBER,
                                      S_RESPONSE_BOOL,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SMEMBERS command using the new signature pattern
 */
int execute_smembers_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.key          = key;
        args.key_len      = key_len;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SMembers,
                                      S_CMD_KEY_ONLY,
                                      S_RESPONSE_SET,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SREM command using the new signature pattern
 */
int execute_srem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_args;
    int                  members_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os+", &object, ce, &key, &key_len, &z_args, &members_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client  = valkey_glide->glide_client;
        args.key           = key;
        args.key_len       = key_len;
        args.members       = z_args;
        args.members_count = members_count;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SRem,
                                      S_CMD_KEY_MEMBERS,
                                      S_RESPONSE_INT,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SMOVE command using the new signature pattern
 */
int execute_smove_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               src = NULL, *dst = NULL, *member = NULL;
    size_t               src_len, dst_len, member_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osss",
                                     &object,
                                     ce,
                                     &src,
                                     &src_len,
                                     &dst,
                                     &dst_len,
                                     &member,
                                     &member_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.src_key      = src;
        args.src_key_len  = src_len;
        args.dst_key      = dst;
        args.dst_key_len  = dst_len;
        args.member       = member;
        args.member_len   = member_len;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SMove,
                                      S_CMD_TWO_KEY_MEMBER,
                                      S_RESPONSE_BOOL,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SPOP command using the new signature pattern
 */
int execute_spop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.key          = key;
        args.key_len      = key_len;
        args.count        = has_count ? count : 1;
        args.has_count    = has_count;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SPop,
                                      S_CMD_KEY_COUNT,
                                      S_RESPONSE_MIXED,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SMISMEMBER command using the new signature pattern
 */
int execute_smismember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zval*                z_args;
    int                  members_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os+", &object, ce, &key, &key_len, &z_args, &members_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client  = valkey_glide->glide_client;
        args.key           = key;
        args.key_len       = key_len;
        args.members       = z_args;
        args.members_count = members_count;

        if (execute_s_generic_command(valkey_glide->glide_client,
                                      SMIsMember,
                                      S_CMD_KEY_MEMBERS,
                                      S_RESPONSE_MIXED,
                                      &args,
                                      return_value)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute SINTER command using the new signature pattern
 */
int execute_sinter_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;

    /* Check if we have a single array argument or variadic string arguments */
    if (argc == 1) {
        /* Try to parse as a single array argument */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array of keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* If array is empty, return FALSE */
            if (keys_count == 0) {
                return 0;
            }

            /* Allocate memory for array of zvals */
            z_extracted_keys = ecalloc(keys_count, sizeof(zval));

            /* Copy array values to sequential array */
            zval* data;
            int   idx = 0;
            ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
            }
            ZEND_HASH_FOREACH_END();

            /* Set for later use */
            z_args = z_extracted_keys;
        }
    }

    /* If we didn't get an array, parse as variadic arguments */
    if (!z_args) {
        if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &keys_count) ==
            FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SInter,
                                               S_CMD_MULTI_KEY,
                                               S_RESPONSE_SET,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute SINTERCARD command using the new signature pattern
 */
int execute_sintercard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_keys;
    zend_long            limit     = 0;
    int                  has_limit = 0;
    HashTable*           ht_keys;
    zval*                z_args;
    int                  keys_count;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oa|l", &object, ce, &z_keys, &limit) ==
        FAILURE) {
        return 0;
    }

    /* Check if limit parameter was provided */
    has_limit = (argc > 1);
    if (has_limit && limit < 0) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* Get keys count and convert HashTable to zval array */
    ht_keys    = Z_ARRVAL_P(z_keys);
    keys_count = zend_hash_num_elements(ht_keys);

    /* If we have no keys, return false */
    if (keys_count == 0) {
        return 0;
    }

    /* Allocate memory for array of zvals */
    z_args = ecalloc(keys_count, sizeof(zval));

    /* Copy array values to sequential array */
    zval* data;
    int   idx = 0;
    ZEND_HASH_FOREACH_VAL(ht_keys, data) {
        ZVAL_COPY(&z_args[idx], data);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.keys         = z_args;
        args.keys_count   = keys_count;
        args.limit        = has_limit ? limit : 0;
        args.has_limit    = has_limit;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SInterCard,
                                               S_CMD_MULTI_KEY_LIMIT,
                                               S_RESPONSE_INT,
                                               &args,
                                               return_value);

        /* Clean up allocated array */
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_args[i]);
        }
        efree(z_args);

        return result;
    }

    /* Clean up allocated array */
    for (int i = 0; i < keys_count; i++) {
        zval_dtor(&z_args[i]);
    }
    efree(z_args);

    return 0;
}

/**
 * Execute SINTERSTORE command using the new signature pattern
 */
int execute_sinterstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                dst = NULL;
    size_t               dst_len;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;
    zval*                data;
    int                  idx             = 0;
    int                  has_destination = 0;

    /* Check if we have a single array argument */
    if (argc == 1) {
        /* Try to parse it as an array */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array which will contain both destination and source keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* We need at least one element (destination key) */
            if (keys_count == 0) {
                return 0;
            }

            /* Extract the first element as the destination key */
            HashPosition pointer;
            zend_hash_internal_pointer_reset_ex(ht_keys, &pointer);
            data = zend_hash_get_current_data_ex(ht_keys, &pointer);
            if (data == NULL || Z_TYPE_P(data) != IS_STRING) {
                return 0;
            }

            /* Set the destination key */
            dst             = Z_STRVAL_P(data);
            dst_len         = Z_STRLEN_P(data);
            has_destination = 1;

            /* If there's only the destination key, return false */
            if (keys_count == 1) {
                return 0;
            }

            /* Move past the destination key */
            zend_hash_move_forward_ex(ht_keys, &pointer);

            /* Allocate memory for array of source keys (excluding destination) */
            z_extracted_keys = ecalloc(keys_count - 1, sizeof(zval));

            /* Copy all remaining values (source keys) to sequential array */
            idx = 0;
            while ((data = zend_hash_get_current_data_ex(ht_keys, &pointer))) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
                zend_hash_move_forward_ex(ht_keys, &pointer);
            }

            /* Set for later use */
            z_args     = z_extracted_keys;
            keys_count = keys_count - 1;
        }
    }

    /* If we didn't get a single array, try other parameter formats */
    if (!has_destination) {
        /* First argument is always the destination key */
        if (zend_parse_method_parameters(1, object, "Os", &object, ce, &dst, &dst_len) == FAILURE) {
            return 0;
        }

        /* Parse remaining args as variadic or array */
        if (argc == 2) {
            /* Try to parse second parameter as an array */
            zval* second_arg;
            if (zend_parse_parameters(1, "z", &second_arg) == SUCCESS &&
                Z_TYPE_P(second_arg) == IS_ARRAY) {
                /* We have an array of source keys */
                ht_keys    = Z_ARRVAL_P(second_arg);
                keys_count = zend_hash_num_elements(ht_keys);

                /* If array is empty, return FALSE */
                if (keys_count == 0) {
                    return 0;
                }

                /* Allocate memory for array of zvals */
                z_extracted_keys = ecalloc(keys_count, sizeof(zval));

                /* Copy array values to sequential array */
                idx = 0;
                ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                    ZVAL_COPY(&z_extracted_keys[idx], data);
                    idx++;
                }
                ZEND_HASH_FOREACH_END();

                /* Set for later use */
                z_args = z_extracted_keys;
            }
        }

        /* If we didn't get an array as the second parameter, parse remaining args as variadic */
        if (!z_args) {
            /* Parse all parameters including destination key */
            if (zend_parse_method_parameters(
                    argc, object, "Os+", &object, ce, &dst, &dst_len, &z_args, &keys_count) ==
                FAILURE) {
                return 0;
            }
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.dst_key      = dst;
        args.dst_key_len  = dst_len;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SInterStore,
                                               S_CMD_DST_MULTI_KEY,
                                               S_RESPONSE_INT,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute SUNION command using the new signature pattern
 */
int execute_sunion_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;

    /* Check if we have a single array argument or variadic string arguments */
    if (argc == 1) {
        /* Try to parse as a single array argument */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array of keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* If array is empty, return FALSE */
            if (keys_count == 0) {
                return 0;
            }

            /* Allocate memory for array of zvals */
            z_extracted_keys = ecalloc(keys_count, sizeof(zval));

            /* Copy array values to sequential array */
            zval* data;
            int   idx = 0;
            ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
            }
            ZEND_HASH_FOREACH_END();

            /* Set for later use */
            z_args = z_extracted_keys;
        }
    }

    /* If we didn't get an array, parse as variadic arguments */
    if (!z_args) {
        if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &keys_count) ==
            FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SUnion,
                                               S_CMD_MULTI_KEY,
                                               S_RESPONSE_SET,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute SUNIONSTORE command using the new signature pattern
 */
int execute_sunionstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                dst = NULL;
    size_t               dst_len;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;
    zval*                data;
    int                  idx             = 0;
    int                  has_destination = 0;

    /* Check if we have a single array argument */
    if (argc == 1) {
        /* Try to parse it as an array */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array which will contain both destination and source keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* We need at least one element (destination key) */
            if (keys_count == 0) {
                return 0;
            }

            /* Extract the first element as the destination key */
            HashPosition pointer;
            zend_hash_internal_pointer_reset_ex(ht_keys, &pointer);
            data = zend_hash_get_current_data_ex(ht_keys, &pointer);
            if (data == NULL || Z_TYPE_P(data) != IS_STRING) {
                return 0;
            }

            /* Set the destination key */
            dst             = Z_STRVAL_P(data);
            dst_len         = Z_STRLEN_P(data);
            has_destination = 1;

            /* If there's only the destination key, return false */
            if (keys_count == 1) {
                return 0;
            }

            /* Move past the destination key */
            zend_hash_move_forward_ex(ht_keys, &pointer);

            /* Allocate memory for array of source keys (excluding destination) */
            z_extracted_keys = ecalloc(keys_count - 1, sizeof(zval));

            /* Copy all remaining values (source keys) to sequential array */
            idx = 0;
            while ((data = zend_hash_get_current_data_ex(ht_keys, &pointer))) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
                zend_hash_move_forward_ex(ht_keys, &pointer);
            }

            /* Set for later use */
            z_args     = z_extracted_keys;
            keys_count = keys_count - 1;
        }
    }

    /* If we didn't get a single array, try other parameter formats */
    if (!has_destination) {
        /* First argument is always the destination key */
        if (zend_parse_method_parameters(1, object, "Os", &object, ce, &dst, &dst_len) == FAILURE) {
            return 0;
        }

        /* Parse remaining args as variadic or array */
        if (argc == 2) {
            /* Try to parse second parameter as an array */
            zval* second_arg;
            if (zend_parse_parameters(1, "z", &second_arg) == SUCCESS &&
                Z_TYPE_P(second_arg) == IS_ARRAY) {
                /* We have an array of source keys */
                ht_keys    = Z_ARRVAL_P(second_arg);
                keys_count = zend_hash_num_elements(ht_keys);

                /* If array is empty, return FALSE */
                if (keys_count == 0) {
                    return 0;
                }

                /* Allocate memory for array of zvals */
                z_extracted_keys = ecalloc(keys_count, sizeof(zval));

                /* Copy array values to sequential array */
                idx = 0;
                ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                    ZVAL_COPY(&z_extracted_keys[idx], data);
                    idx++;
                }
                ZEND_HASH_FOREACH_END();

                /* Set for later use */
                z_args = z_extracted_keys;
            }
        }

        /* If we didn't get an array as the second parameter, parse remaining args as variadic */
        if (!z_args) {
            /* Parse all parameters including destination key */
            if (zend_parse_method_parameters(
                    argc, object, "Os+", &object, ce, &dst, &dst_len, &z_args, &keys_count) ==
                FAILURE) {
                return 0;
            }
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.dst_key      = dst;
        args.dst_key_len  = dst_len;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SUnionStore,
                                               S_CMD_DST_MULTI_KEY,
                                               S_RESPONSE_INT,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute SDIFF command using the new signature pattern
 */
int execute_sdiff_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;

    /* Check if we have a single array argument or variadic string arguments */
    if (argc == 1) {
        /* Try to parse as a single array argument */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array of keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* If array is empty, return FALSE */
            if (keys_count == 0) {
                return 0;
            }

            /* Allocate memory for array of zvals */
            z_extracted_keys = ecalloc(keys_count, sizeof(zval));

            /* Copy array values to sequential array */
            zval* data;
            int   idx = 0;
            ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
            }
            ZEND_HASH_FOREACH_END();

            /* Set for later use */
            z_args = z_extracted_keys;
        }
    }

    /* If we didn't get an array, parse as variadic arguments */
    if (!z_args) {
        if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &keys_count) ==
            FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SDiff,
                                               S_CMD_MULTI_KEY,
                                               S_RESPONSE_SET,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute SDIFFSTORE command using the new signature pattern
 */
int execute_sdiffstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                dst = NULL;
    size_t               dst_len;
    zval*                z_args           = NULL;
    int                  keys_count       = 0;
    zval*                z_keys_arr       = NULL;
    HashTable*           ht_keys          = NULL;
    zval*                z_extracted_keys = NULL;
    zval*                data;
    int                  idx             = 0;
    int                  has_destination = 0;

    /* Check if we have a single array argument */
    if (argc == 1) {
        /* Try to parse it as an array */
        if (zend_parse_method_parameters(argc, object, "Oa", &object, ce, &z_keys_arr) == SUCCESS) {
            /* We have an array which will contain both destination and source keys */
            ht_keys    = Z_ARRVAL_P(z_keys_arr);
            keys_count = zend_hash_num_elements(ht_keys);

            /* We need at least one element (destination key) */
            if (keys_count == 0) {
                return 0;
            }

            /* Extract the first element as the destination key */
            HashPosition pointer;
            zend_hash_internal_pointer_reset_ex(ht_keys, &pointer);
            data = zend_hash_get_current_data_ex(ht_keys, &pointer);
            if (data == NULL || Z_TYPE_P(data) != IS_STRING) {
                return 0;
            }

            /* Set the destination key */
            dst             = Z_STRVAL_P(data);
            dst_len         = Z_STRLEN_P(data);
            has_destination = 1;

            /* If there's only the destination key, return false */
            if (keys_count == 1) {
                return 0;
            }

            /* Move past the destination key */
            zend_hash_move_forward_ex(ht_keys, &pointer);

            /* Allocate memory for array of source keys (excluding destination) */
            z_extracted_keys = ecalloc(keys_count - 1, sizeof(zval));

            /* Copy all remaining values (source keys) to sequential array */
            idx = 0;
            while ((data = zend_hash_get_current_data_ex(ht_keys, &pointer))) {
                ZVAL_COPY(&z_extracted_keys[idx], data);
                idx++;
                zend_hash_move_forward_ex(ht_keys, &pointer);
            }

            /* Set for later use */
            z_args     = z_extracted_keys;
            keys_count = keys_count - 1;
        }
    }

    /* If we didn't get a single array, try other parameter formats */
    if (!has_destination) {
        /* First argument is always the destination key */
        if (zend_parse_method_parameters(1, object, "Os", &object, ce, &dst, &dst_len) == FAILURE) {
            return 0;
        }

        /* Parse remaining args as variadic or array */
        if (argc == 2) {
            /* Try to parse second parameter as an array */
            zval* second_arg;
            if (zend_parse_parameters(1, "z", &second_arg) == SUCCESS &&
                Z_TYPE_P(second_arg) == IS_ARRAY) {
                /* We have an array of source keys */
                ht_keys    = Z_ARRVAL_P(second_arg);
                keys_count = zend_hash_num_elements(ht_keys);

                /* If array is empty, return FALSE */
                if (keys_count == 0) {
                    return 0;
                }

                /* Allocate memory for array of zvals */
                z_extracted_keys = ecalloc(keys_count, sizeof(zval));

                /* Copy array values to sequential array */
                idx = 0;
                ZEND_HASH_FOREACH_VAL(ht_keys, data) {
                    ZVAL_COPY(&z_extracted_keys[idx], data);
                    idx++;
                }
                ZEND_HASH_FOREACH_END();

                /* Set for later use */
                z_args = z_extracted_keys;
            }
        }

        /* If we didn't get an array as the second parameter, parse remaining args as variadic */
        if (!z_args) {
            /* Parse all parameters including destination key */
            if (zend_parse_method_parameters(
                    argc, object, "Os+", &object, ce, &dst, &dst_len, &z_args, &keys_count) ==
                FAILURE) {
                return 0;
            }
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.dst_key      = dst;
        args.dst_key_len  = dst_len;
        args.keys         = z_args;
        args.keys_count   = keys_count;

        int result = execute_s_generic_command(valkey_glide->glide_client,
                                               SDiffStore,
                                               S_CMD_DST_MULTI_KEY,
                                               S_RESPONSE_INT,
                                               &args,
                                               return_value);

        /* Clean up if we allocated memory for the array keys */
        if (z_extracted_keys) {
            for (int i = 0; i < keys_count; i++) {
                zval_dtor(&z_extracted_keys[i]);
            }
            efree(z_extracted_keys);
        }

        return result;
    }

    /* Clean up if we allocated memory for the array keys but didn't execute the command */
    if (z_extracted_keys) {
        for (int i = 0; i < keys_count; i++) {
            zval_dtor(&z_extracted_keys[i]);
        }
        efree(z_extracted_keys);
    }

    return 0;
}

/**
 * Execute cluster scan command using the FFI request_cluster_scan function
 */
int execute_cluster_scan_command(const void* glide_client,
                                 char**      cursor,
                                 const char* pattern,
                                 size_t      pattern_len,
                                 long        count,
                                 int         has_count,
                                 const char* type,
                                 size_t      type_len,
                                 int         has_type,
                                 zval*       return_value) {
    if (!glide_client || !cursor || !return_value) {
        return 0;
    }

    /* Build arguments array */
    /* Count arguments: pattern (MATCH + value), count (COUNT + value), type (TYPE + value) */
    int arg_count = 0;
    if (pattern && pattern_len > 0)
        arg_count += 2; /* MATCH + pattern */
    if (has_count)
        arg_count += 2; /* COUNT + count_value */
    if (has_type && type && type_len > 0)
        arg_count += 2; /* TYPE + type_value */

    uintptr_t*     args      = NULL;
    unsigned long* args_len  = NULL;
    char*          count_str = NULL;

    if (arg_count > 0) {
        args     = emalloc(arg_count * sizeof(uintptr_t));
        args_len = emalloc(arg_count * sizeof(unsigned long));

        int idx = 0;

        /* Add MATCH pattern */
        if (pattern && pattern_len > 0) {
            args[idx]     = (uintptr_t) "MATCH";
            args_len[idx] = 5;
            idx++;
            args[idx]     = (uintptr_t) pattern;
            args_len[idx] = pattern_len;
            idx++;
        }

        /* Add COUNT */
        if (has_count) {
            count_str = alloc_long_string(count, NULL);
            if (!count_str) {
                efree(args);
                efree(args_len);
                return 0;
            }
            args[idx]     = (uintptr_t) "COUNT";
            args_len[idx] = 5;
            idx++;
            args[idx]     = (uintptr_t) count_str;
            args_len[idx] = strlen(count_str);
            idx++;
        }

        /* Add TYPE (for SCAN only) */
        if (has_type && type && type_len > 0) {
            args[idx]     = (uintptr_t) "TYPE";
            args_len[idx] = 4;
            idx++;
            args[idx]     = (uintptr_t) type;
            args_len[idx] = type_len;
            idx++;
        }
    }

    /* Call request_cluster_scan FFI function directly */
    CommandResult* result =
        request_cluster_scan(glide_client, 0, *cursor, arg_count, args, args_len);

    int success = 0;

    if (result) {
        /* Create temporary args structure for response processing */
        s_command_args_t scan_args;
        INIT_S_COMMAND_ARGS(scan_args);
        scan_args.cursor = cursor;

        /* Process scan response */
        success = process_s_scan_response(result, Scan, &scan_args, return_value);
        /* Convert legacy "finished" cursor to "0" for backward compatibility */
        if (*cursor && strcmp(*cursor, "finished") == 0) {
            efree(*cursor);
            *cursor = estrdup("0");
        }
        free_command_result(result);
    }

    /* Cleanup */
    if (count_str) {
        efree(count_str);
    }
    if (args) {
        efree(args);
    }
    if (args_len) {
        efree(args_len);
    }

    return success;
}


/**
 * Execute SCAN command with unified signature
 */
int execute_scan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_iter;
    char *               pattern = NULL, *type = NULL;
    size_t               pattern_len = 0, type_len = 0;
    int                  has_pattern = 0, has_type = 0;
    zend_long            count     = 0;
    int                  has_count = 0;

    /* Get ValkeyGlide object first */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if this is cluster mode */
    int is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Initialize optional parameters to safe defaults */
    pattern     = NULL;
    pattern_len = 0;
    count       = 10;
    type        = NULL;
    type_len    = 0;

    if (is_cluster) {
        /* For cluster mode, expect ClusterScanCursor object as first parameter */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "OO|sls",
                                         &object,
                                         ce,
                                         &z_iter,
                                         get_cluster_scan_cursor_ce(),
                                         &pattern,
                                         &pattern_len,
                                         &count,
                                         &type,
                                         &type_len) == FAILURE) {
            return 0;
        }

        /* Determine what was actually provided based on argc */
        has_pattern = (argc >= 2 && pattern != NULL && pattern_len > 0);
        has_count   = (argc >= 3);
        has_type    = (argc >= 4 && type != NULL && type_len > 0);

        /* Get cursor string from ClusterScanCursor object */
        zval cursor_method_name;
        ZVAL_STRING(&cursor_method_name, "getCursor");

        zval cursor_result;
        if (call_user_function(NULL, z_iter, &cursor_method_name, &cursor_result, 0, NULL) !=
            SUCCESS) {
            zval_dtor(&cursor_method_name);
            return 0;
        }
        zval_dtor(&cursor_method_name);

        if (Z_TYPE(cursor_result) != IS_STRING) {
            zval_dtor(&cursor_result);
            return 0;
        }

        /* Create a copy of cursor for passing to functions */
        char* cursor_ptr = estrdup(Z_STRVAL(cursor_result));
        zval_dtor(&cursor_result);

        /* Use empty pattern if not specified */
        const char* scan_pattern     = has_pattern ? pattern : "";
        size_t      scan_pattern_len = has_pattern ? pattern_len : 0;

        /* Use default count if not specified */
        long scan_count = has_count ? count : 10;

        /* Use cluster scan implementation */
        if (execute_cluster_scan_command(valkey_glide->glide_client,
                                         &cursor_ptr,
                                         scan_pattern,
                                         scan_pattern_len,
                                         scan_count,
                                         (scan_count > 0),
                                         has_type ? type : NULL,
                                         has_type ? type_len : 0,
                                         has_type,
                                         return_value)) {
            /* Update ClusterScanCursor object with new cursor value directly */
            cluster_scan_cursor_object* cursor_obj = CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(z_iter);


            cursor_obj->next_cursor_id = estrdup(cursor_ptr);

            efree(cursor_ptr);
            return 1;
        }

        efree(cursor_ptr);
        return 0;

    } else {
        /* For non-cluster mode, use string reference as before */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Oz|sls",
                                         &object,
                                         ce,
                                         &z_iter,
                                         &pattern,
                                         &pattern_len,
                                         &count,
                                         &type,
                                         &type_len) == FAILURE) {
            return 0;
        }

        /* Determine what was actually provided based on argc */
        has_pattern = (argc >= 2 && pattern != NULL && pattern_len > 0);
        has_count   = (argc >= 3);
        has_type    = (argc >= 4 && type != NULL && type_len > 0);

        /* Dereference if it's a reference */
        ZVAL_DEREF(z_iter);

        /* Make sure we have a valid cursor - accept NULL or string */
        if (Z_TYPE_P(z_iter) != IS_STRING && Z_TYPE_P(z_iter) != IS_NULL) {
            php_error_docref(NULL, E_WARNING, "Cursor must be string");
            return 0;
        }

        /* Get cursor string */
        char* cursor_value;
        if (Z_TYPE_P(z_iter) == IS_NULL) {
            /* NULL cursor means start from the beginning (0) */
            cursor_value = "0";
        } else if (Z_TYPE_P(z_iter) == IS_STRING) {
            cursor_value = Z_STRVAL_P(z_iter);
        } else {
            return 0;
        }

        /* Create a copy of cursor for passing to functions */
        char* cursor_ptr = estrdup(cursor_value);

        /* Use empty pattern if not specified */
        const char* scan_pattern     = has_pattern ? pattern : "";
        size_t      scan_pattern_len = has_pattern ? pattern_len : 0;

        /* Use default count if not specified */
        long scan_count = has_count ? count : 10;

        /* Use existing non-cluster implementation */
        s_command_args_t args;
        INIT_S_COMMAND_ARGS(args);

        args.glide_client = valkey_glide->glide_client;
        args.cursor       = &cursor_ptr;
        args.pattern      = scan_pattern;
        args.pattern_len  = scan_pattern_len;
        args.count        = scan_count;
        args.has_count    = (scan_count > 0);
        args.type         = has_type ? type : NULL;
        args.type_len     = has_type ? type_len : 0;
        args.has_type     = has_type;

        /* Execute the SCAN command using the S-command framework */
        if (execute_s_generic_command(valkey_glide->glide_client,
                                      Scan,
                                      S_CMD_SCAN,
                                      S_RESPONSE_SCAN,
                                      &args,
                                      return_value)) {
            /* Update iterator value */
            ZVAL_STRING(z_iter, cursor_ptr);
            efree(cursor_ptr);
            return 1;
        }

        /* Clean up on failure */
        efree(cursor_ptr);
        return 0;
    }
}

/**
 * Execute SSCAN command with unified signature
 */
int execute_sscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_scan_command_generic(object, argc, return_value, ce, SScan);
}


/**
 * Execute generic SCAN command using the generic framework - Updated for string cursors
 */
int execute_gen_scan_command_internal(const void*      glide_client,
                                      enum RequestType cmd_type,
                                      const char*      key,
                                      size_t           key_len,
                                      char**           cursor,
                                      const char*      pattern,
                                      size_t           pattern_len,
                                      long             count,
                                      zval*            return_value) {
    s_command_args_t args;
    INIT_S_COMMAND_ARGS(args);

    args.glide_client = glide_client;
    args.key          = key;
    args.key_len      = key_len;
    args.cursor       = cursor;
    args.pattern      = pattern;
    args.pattern_len  = pattern_len;
    args.count        = count;
    args.has_count    = (count > 0);

    int result = execute_s_generic_command(
        glide_client, cmd_type, S_CMD_SCAN, S_RESPONSE_SCAN, &args, return_value);

    return result;
}

/**
 * Generic scan command implementation for HSCAN, ZSCAN, SSCAN - Updated for string cursors
 */
int execute_scan_command_generic(
    zval* object, int argc, zval* return_value, zend_class_entry* ce, enum RequestType cmd_type) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *pattern = NULL;
    size_t               key_len, pattern_len = 0;
    zval*                z_iter;
    zend_long            count       = 0;
    int                  has_pattern = 0;
    int                  has_count   = 0;

    /* Parse arguments */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osz|sl",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &z_iter,
                                     &pattern,
                                     &pattern_len,
                                     &count) == FAILURE) {
        return 0;
    }

    /* Check if optional parameters are provided */
    has_pattern = (pattern != NULL && pattern_len > 0);
    has_count   = (argc > 3);

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Dereference if it's a reference */
    ZVAL_DEREF(z_iter);

    /* Make sure we have a valid cursor - accept NULL or string */
    if (Z_TYPE_P(z_iter) != IS_STRING && Z_TYPE_P(z_iter) != IS_NULL) {
        php_error_docref(NULL, E_WARNING, "Cursor must be string");
        return 0;
    }

    /* Get cursor string */
    char* cursor_value;
    if (Z_TYPE_P(z_iter) == IS_NULL) {
        /* NULL cursor means start from the beginning (0) */
        cursor_value = "0";
    } else if (Z_TYPE_P(z_iter) == IS_STRING) {
        cursor_value = Z_STRVAL_P(z_iter);
    } else {
        return 0;
    }

    /* Create a copy of cursor for passing to functions */
    char* cursor_ptr = estrdup(cursor_value);

    /* Use empty pattern if not specified */
    const char* scan_pattern     = has_pattern ? pattern : "";
    size_t      scan_pattern_len = has_pattern ? pattern_len : 0;

    /* Use default count if not specified */
    long scan_count = has_count ? count : 10;

    /* Execute the scan command using the generic internal function */
    if (execute_gen_scan_command_internal(valkey_glide->glide_client,
                                          cmd_type,
                                          key,
                                          key_len,
                                          &cursor_ptr,
                                          scan_pattern,
                                          scan_pattern_len,
                                          scan_count,
                                          return_value)) {
        /* Update iterator value */
        ZVAL_STRING(z_iter, cursor_ptr);
        efree(cursor_ptr);
        return 1;
    }

    /* Clean up on failure */
    efree(cursor_ptr);
    return 0;
}

/**
 * Execute HSCAN command with unified signature
 */
int execute_hscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    return execute_scan_command_generic(object, argc, return_value, ce, HScan);
}
