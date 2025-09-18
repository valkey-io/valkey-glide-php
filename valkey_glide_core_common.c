/*
  +----------------------------------------------------------------------+
  | Valkey Glide Core Common Framework                                   |
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

#include "valkey_glide_core_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "valkey_glide_z_common.h"
/* ====================================================================
 * CORE FRAMEWORK IMPLEMENTATION
 * ==================================================================== */

/**
 * Main command execution framework
 * This is the central function that handles all ValkeyGlide/Valkey commands
 */
int execute_core_command(valkey_glide_object* valkey_glide,
                         core_command_args_t* args,
                         void*                result_ptr,
                         z_result_processor_t processor,
                         zval*                return_value) {
    if (!valkey_glide || !args || !args->glide_client || !processor) {
        efree(result_ptr);
        return 0;
    }

    uintptr_t*     cmd_args          = NULL;
    unsigned long* cmd_args_len      = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            res               = 0;
    CommandResult* result            = NULL;

    debug_print_core_args(args);

    /* Prepare command arguments based on command type */
    arg_count =
        prepare_core_args(args, &cmd_args, &cmd_args_len, &allocated_strings, &allocated_count);

    if (arg_count < 0) {
        efree(result_ptr);
        return 0;
    }

    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* Create batch-compatible processor wrapper */


        res = buffer_command_for_batch(valkey_glide,
                                       args->cmd_type,
                                       cmd_args,
                                       cmd_args_len,
                                       arg_count,

                                       result_ptr,
                                       processor);

        free_core_args(cmd_args, cmd_args_len, allocated_strings, allocated_count);
        if (res == 0) {
            efree(result_ptr);
        }
        return res;
    }

    /* Execute the command - use routing if cluster mode and route provided */
    if (args->has_route && args->route_param) {
        /* Cluster mode with routing */
        result = execute_command_with_route(args->glide_client,
                                            args->cmd_type,
                                            arg_count,
                                            cmd_args,
                                            cmd_args_len,
                                            args->route_param);
    } else {
        /* Non-cluster mode or no routing */
        result =
            execute_command(args->glide_client, args->cmd_type, arg_count, cmd_args, cmd_args_len);
    }

    debug_print_command_result(result);

    /* Process result using appropriate handler */
    if (result) {
        if (!result->command_error && result->response) {
            /* Non-routed commands use standard processor */
            res = processor(result->response, result_ptr, return_value);
        }

        /* Free the result - handle_string_response doesn't free it */

        free_command_result(result);
    }

    /* Cleanup */
    free_core_args(cmd_args, cmd_args_len, allocated_strings, allocated_count);

    return res;
}

/**
 * Prepare command arguments based on command type and structure
 */
int prepare_core_args(core_command_args_t* args,
                      uintptr_t**          cmd_args,
                      unsigned long**      cmd_args_len,
                      char***              allocated_strings,
                      int*                 allocated_count) {
    if (!args) {
        return 0;
    }

    /* Determine preparation strategy based on command type */
    switch (args->cmd_type) {
        /* Zero argument operations */
        case RandomKey:
        case Discard:
        case Exec:
        case Time:
        case Role:
        case DBSize:
            return prepare_zero_args(args, cmd_args, cmd_args_len);

        /* Single key operations */
        case GetDel:
        case Get:
        case Strlen:
        case Type:
        case TTL:
        case PTTL:
        case ExpireTime:
        case PExpireTime:
        case Persist:
        case Dump:
            return prepare_key_only_args(args, cmd_args, cmd_args_len);

        /* Pattern-based operations */
        case Keys:
            return prepare_message_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Zero-argument operations */
        case UnWatch:
            return prepare_zero_args(args, cmd_args, cmd_args_len);

        /* Key-value operations */
        case Set:
        case SetEx:
        case PSetEx:
        case SetNX:

        case GetSet:

        case GetEx:
        case Append:
        case Incr:
        case Decr:
        case IncrBy:
        case DecrBy:
        case Rename:
        case RenameNX:
        case IncrByFloat:
        case Move:
        case Copy:
            return prepare_key_value_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* DEL and UNLINK: Support both single-key and multi-key operations */
        case Del:
        case Unlink:
            /* Check if single key or multi-key operation */
            if (args->key && args->key_len > 0 && args->arg_count == 0) {
                /* Single key: DEL key */

                return prepare_key_only_args(args, cmd_args, cmd_args_len);
            } else if (args->arg_count > 0 && args->args[0].type == CORE_ARG_TYPE_ARRAY) {
                /* Multi-key: DEL key1 key2 key3 */

                return prepare_multi_key_args(args, cmd_args, cmd_args_len);
            }
            return 0;

        /* Multi-key operations */
        case Exists:
        case Touch:
        case MGet:
        case Watch:
        case PfCount:
            /* Check if single key or multi-key operation */
            if (args->key && args->key_len > 0 && args->arg_count == 0) {
                /* Single key: PFCOUNT key */
                return prepare_key_only_args(args, cmd_args, cmd_args_len);
            } else if (args->arg_count > 0 && args->args[0].type == CORE_ARG_TYPE_ARRAY) {
                /* Multi-key: PFCOUNT key1 key2 key3 */
                return prepare_multi_key_args(args, cmd_args, cmd_args_len);
            }
            return 0;

        /* HyperLogLog operations */
        case PfAdd:
        case PfMerge:
            return prepare_key_value_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Bit operations */
        case BitCount:
        case BitPos:
        case GetBit:
        case SetBit:
        case BitOp:
            return prepare_bit_operation_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Expire operations */
        case Expire:
        case ExpireAt:
        case PExpire:
        case PExpireAt:
            return prepare_expire_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Range operations */
        case GetRange:
        case SetRange:
            return prepare_range_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Message operations (no key, just arguments) */
        case Ping:
        case Echo:
        case Wait:
        case FlushDB:
        case FlushAll:
        case Select:
        case SwapDb:
            return prepare_message_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        /* Key-value pair operations */
        case MSet:
        case MSetNX:
            return prepare_key_value_pairs_args(
                args, cmd_args, cmd_args_len, allocated_strings, allocated_count);

        default:
            return 0;
    }
}

/**
 * Free all allocated command arguments and strings
 */
void free_core_args(uintptr_t*     cmd_args,
                    unsigned long* cmd_args_len,
                    char**         allocated_strings,
                    int            allocated_count) {
    if (cmd_args) {
        efree(cmd_args);
    }
    if (cmd_args_len) {
        efree(cmd_args_len);
    }
    if (allocated_strings) {
        free_tracked_strings(allocated_strings, allocated_count);
        efree(allocated_strings);
    }
}

/* ====================================================================
 * ARGUMENT PREPARATION HELPERS
 * ==================================================================== */

/**
 * Prepare arguments for zero-argument operations (RANDOMKEY, etc.)
 */
int prepare_zero_args(core_command_args_t* args,
                      uintptr_t**          cmd_args,
                      unsigned long**      cmd_args_len) {
    /* No arguments needed - just return 0 to indicate success but no args */
    *cmd_args     = NULL;
    *cmd_args_len = NULL;
    return 0; /* Zero arguments */
}

/**
 * Prepare arguments for single key operations
 */
int prepare_key_only_args(core_command_args_t* args,
                          uintptr_t**          cmd_args,
                          unsigned long**      cmd_args_len) {
    if (!args->key || args->key_len == 0) {
        return 0;
    }

    if (!allocate_core_arg_arrays(1, cmd_args, cmd_args_len)) {
        return 0;
    }

    (*cmd_args)[0]     = (uintptr_t) args->key;
    (*cmd_args_len)[0] = args->key_len;

    return 1;
}

/**
 * Prepare arguments for key-value operations
 */
int prepare_key_value_args(core_command_args_t* args,
                           uintptr_t**          cmd_args,
                           unsigned long**      cmd_args_len,
                           char***              allocated_strings,
                           int*                 allocated_count) {
    if (!args->key || args->key_len == 0) {
        return 0;
    }
    /* Calculate total argument count */
    int total_args = 1; /* key */

    /* Add primary arguments */
    for (int i = 0; i < args->arg_count; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_STRING:
            case CORE_ARG_TYPE_LONG:
            case CORE_ARG_TYPE_DOUBLE:
                total_args++;
                break;
            case CORE_ARG_TYPE_MULTI_STRING:
                total_args += args->args[i].data.multi_string_arg.count;
                break;
            case CORE_ARG_TYPE_ARRAY:
                /* Count elements in the array */
                total_args += args->args[i].data.array_arg.count;
                break;
            default:
                break;
        }
    }

    /* Add option arguments */
    if (args->options.has_expire) {
        total_args += 2; /* EX/PX/EXAT/PXAT + value */
    }
    if (args->options.nx) {
        total_args++; /* NX */
    }
    if (args->options.xx) {
        total_args++; /* XX */
    }
    if (args->options.get_old_value) {
        total_args++; /* GET */
    }
    if (args->options.keep_ttl) {
        total_args++; /* KEEPTTL */
    }
    if (args->options.has_ifeq) {
        total_args += 2; /* IFEQ + value */
    }
    if (args->options.persist) {
        total_args++; /* PERSIST */
    }

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    /* Initialize string tracking */
    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int arg_idx = 0;

    /* Add key */
    (*cmd_args)[arg_idx]     = (uintptr_t) args->key;
    (*cmd_args_len)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add primary arguments */
    for (int i = 0; i < args->arg_count; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_STRING:
                (*cmd_args)[arg_idx]     = (uintptr_t) args->args[i].data.string_arg.value;
                (*cmd_args_len)[arg_idx] = args->args[i].data.string_arg.len;
                arg_idx++;
                break;

            case CORE_ARG_TYPE_LONG: {
                size_t len;
                char*  str = core_long_to_string(args->args[i].data.long_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }

            case CORE_ARG_TYPE_DOUBLE: {
                size_t len;
                char*  str = core_double_to_string(args->args[i].data.double_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }

            case CORE_ARG_TYPE_MULTI_STRING:
                for (int j = 0; j < args->args[i].data.multi_string_arg.count; j++) {
                    (*cmd_args)[arg_idx] =
                        (uintptr_t) args->args[i].data.multi_string_arg.values[j];
                    (*cmd_args_len)[arg_idx] = args->args[i].data.multi_string_arg.lengths[j];
                    arg_idx++;
                }
                break;

            case CORE_ARG_TYPE_ARRAY: {
                /* Expand array elements into individual arguments */
                zval* array = args->args[i].data.array_arg.array;
                if (Z_TYPE_P(array) == IS_ARRAY) {
                    HashTable* ht = Z_ARRVAL_P(array);
                    zval*      element;

                    ZEND_HASH_FOREACH_VAL(ht, element) {
                        if (Z_TYPE_P(element) == IS_STRING) {
                            (*cmd_args)[arg_idx]     = (uintptr_t) Z_STRVAL_P(element);
                            (*cmd_args_len)[arg_idx] = Z_STRLEN_P(element);
                            arg_idx++;
                        } else {
                            /* Convert non-string to string */
                            zend_string* str = zval_get_string(element);
                            if (str) {
                                size_t len      = ZSTR_LEN(str);
                                char*  str_copy = emalloc(len + 1);
                                if (str_copy) {
                                    memcpy(str_copy, ZSTR_VAL(str), len);
                                    str_copy[len]            = '\0';
                                    (*cmd_args)[arg_idx]     = (uintptr_t) str_copy;
                                    (*cmd_args_len)[arg_idx] = len;
                                    add_tracked_string(
                                        *allocated_strings, allocated_count, str_copy);
                                    arg_idx++;
                                }
                                zend_string_release(str);
                            }
                        }
                    }
                    ZEND_HASH_FOREACH_END();
                }
                break;
            }

            default:
                break;
        }
    }

    /* Add options */
    if (args->options.has_expire) {
        if (args->options.has_pxat) {
            (*cmd_args)[arg_idx]     = (uintptr_t) "PXAT";
            (*cmd_args_len)[arg_idx] = 4;
            arg_idx++;

            size_t len;
            char*  str = core_long_to_string(args->options.expire_at_milliseconds, &len);
            if (str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, str);
                arg_idx++;
            }
        } else if (args->options.has_exat) {
            (*cmd_args)[arg_idx]     = (uintptr_t) "EXAT";
            (*cmd_args_len)[arg_idx] = 4;
            arg_idx++;

            size_t len;
            char*  str = core_long_to_string(args->options.expire_at_seconds, &len);
            if (str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, str);
                arg_idx++;
            }
        } else if (args->options.has_pexpire) {
            (*cmd_args)[arg_idx]     = (uintptr_t) "PX";
            (*cmd_args_len)[arg_idx] = 2;
            arg_idx++;

            size_t len;
            char*  str = core_long_to_string(args->options.expire_milliseconds, &len);
            if (str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, str);
                arg_idx++;
            }
        } else {
            (*cmd_args)[arg_idx]     = (uintptr_t) "EX";
            (*cmd_args_len)[arg_idx] = 2;
            arg_idx++;

            size_t len;
            char*  str = core_long_to_string(args->options.expire_seconds, &len);
            if (str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, str);
                arg_idx++;
            }
        }
    }

    if (args->options.nx) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "NX";
        (*cmd_args_len)[arg_idx] = 2;
        arg_idx++;
    }

    if (args->options.xx) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "XX";
        (*cmd_args_len)[arg_idx] = 2;
        arg_idx++;
    }

    if (args->options.get_old_value) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "GET";
        (*cmd_args_len)[arg_idx] = 3;
        arg_idx++;
    }

    if (args->options.keep_ttl) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "KEEPTTL";
        (*cmd_args_len)[arg_idx] = 7;
        arg_idx++;
    }

    if (args->options.has_ifeq) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "IFEQ";
        (*cmd_args_len)[arg_idx] = 4;
        arg_idx++;

        (*cmd_args)[arg_idx]     = (uintptr_t) args->options.ifeq_value;
        (*cmd_args_len)[arg_idx] = args->options.ifeq_len;
        arg_idx++;
    }

    if (args->options.persist) {
        (*cmd_args)[arg_idx]     = (uintptr_t) "PERSIST";
        (*cmd_args_len)[arg_idx] = 7;
        arg_idx++;
    }

    return arg_idx;
}

/**
 * Prepare arguments for message operations (ECHO, etc.)
 */
int prepare_message_args(core_command_args_t* args,
                         uintptr_t**          cmd_args,
                         unsigned long**      cmd_args_len,
                         char***              allocated_strings,
                         int*                 allocated_count) {
    if (args->arg_count == 0) {
        return 0;
    }

    /* Calculate total argument count - just the arguments, no key */
    int total_args = 0;

    for (int i = 0; i < args->arg_count; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_STRING:
            case CORE_ARG_TYPE_LONG:
            case CORE_ARG_TYPE_DOUBLE:
                total_args++;
                break;
            case CORE_ARG_TYPE_MULTI_STRING:
                total_args += args->args[i].data.multi_string_arg.count;
                break;
            default:
                break;
        }
    }

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    /* Initialize string tracking */
    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int arg_idx = 0;

    /* Add all arguments */
    for (int i = 0; i < args->arg_count; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_STRING:
                (*cmd_args)[arg_idx]     = (uintptr_t) args->args[i].data.string_arg.value;
                (*cmd_args_len)[arg_idx] = args->args[i].data.string_arg.len;
                arg_idx++;
                break;

            case CORE_ARG_TYPE_LONG: {
                size_t len;
                char*  str = core_long_to_string(args->args[i].data.long_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }

            case CORE_ARG_TYPE_DOUBLE: {
                size_t len;
                char*  str = core_double_to_string(args->args[i].data.double_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }

            case CORE_ARG_TYPE_MULTI_STRING:
                for (int j = 0; j < args->args[i].data.multi_string_arg.count; j++) {
                    (*cmd_args)[arg_idx] =
                        (uintptr_t) args->args[i].data.multi_string_arg.values[j];
                    (*cmd_args_len)[arg_idx] = args->args[i].data.multi_string_arg.lengths[j];
                    arg_idx++;
                }
                break;

            default:
                break;
        }
    }

    return arg_idx;
}

/**
 * Prepare arguments for key-value pairs operations (MSET, MSETNX)
 */
int prepare_key_value_pairs_args(core_command_args_t* args,
                                 uintptr_t**          cmd_args,
                                 unsigned long**      cmd_args_len,
                                 char***              allocated_strings,
                                 int*                 allocated_count) {
    if (args->arg_count == 0 || args->args[0].type != CORE_ARG_TYPE_ARRAY) {
        return 0;
    }

    zval* arr = args->args[0].data.array_arg.array;
    if (Z_TYPE_P(arr) != IS_ARRAY) {
        return 0;
    }

    HashTable* ht        = Z_ARRVAL_P(arr);
    int        key_count = zend_hash_num_elements(ht);

    if (key_count == 0) {
        return 0;
    }

    /* Each key-value pair requires 2 arguments */
    int total_args = key_count * 2;

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    /* Initialize string tracking */
    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int          arg_idx = 0;
    zval*        data;
    zend_string* key;
    zend_ulong   num_key;

    ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, key, data) {
        /* Add key */
        if (!key) {
            /* Numeric key - convert to string */
            size_t key_len;
            char*  key_str = core_long_to_string((long) num_key, &key_len);
            if (key_str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) key_str;
                (*cmd_args_len)[arg_idx] = key_len;
                add_tracked_string(*allocated_strings, allocated_count, key_str);
                arg_idx++;
            }
        } else {
            /* String key */
            (*cmd_args)[arg_idx]     = (uintptr_t) ZSTR_VAL(key);
            (*cmd_args_len)[arg_idx] = ZSTR_LEN(key);
            arg_idx++;
        }

        /* Add value */
        if (Z_TYPE_P(data) == IS_STRING) {
            (*cmd_args)[arg_idx]     = (uintptr_t) Z_STRVAL_P(data);
            (*cmd_args_len)[arg_idx] = Z_STRLEN_P(data);
            arg_idx++;
        } else {
            /* Convert non-string value to string using safe method */
            zend_string* str = zval_get_string(data);
            if (str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) ZSTR_VAL(str);
                (*cmd_args_len)[arg_idx] = ZSTR_LEN(str);
                arg_idx++;
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    return arg_idx;
}

/**
 * Prepare arguments for multi-key operations
 */
int prepare_multi_key_args(core_command_args_t* args,
                           uintptr_t**          cmd_args,
                           unsigned long**      cmd_args_len) {
    if (args->arg_count == 0 || args->args[0].type != CORE_ARG_TYPE_ARRAY) {
        return 0;
    }

    zval* keys      = args->args[0].data.array_arg.array;
    int   key_count = args->args[0].data.array_arg.count;

    if (!allocate_core_arg_arrays(key_count, cmd_args, cmd_args_len)) {
        return 0;
    }

    HashTable* keys_hash = Z_ARRVAL_P(keys);
    zval*      key;
    int        idx = 0;

    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        if (Z_TYPE_P(key) != IS_STRING) {
            convert_to_string(key);
        }
        (*cmd_args)[idx]     = (uintptr_t) Z_STRVAL_P(key);
        (*cmd_args_len)[idx] = Z_STRLEN_P(key);
        idx++;
    }
    ZEND_HASH_FOREACH_END();

    return idx;
}

/**
 * Prepare arguments for bit operations
 */
int prepare_bit_operation_args(core_command_args_t* args,
                               uintptr_t**          cmd_args,
                               unsigned long**      cmd_args_len,
                               char***              allocated_strings,
                               int*                 allocated_count) {
    if (!args->key || args->key_len == 0) {
        return 0;
    }

    int total_args = 1; /* key */

    /* Calculate arguments based on command type */
    switch (args->cmd_type) {
        case BitCount:
            total_args += (args->options.has_range ? 2 : 0); /* start, end */
            total_args += (args->options.bybit ? 1 : 0);     /* BYBIT */
            break;
        case BitPos:
            total_args += 1;                                 /* bit value */
            total_args += (args->options.has_range ? 2 : 0); /* start, end */
            total_args += (args->options.bybit ? 1 : 0);     /* BYBIT */
            break;
        case GetBit:
            total_args += 1; /* offset */
            break;
        case SetBit:
            total_args += 2; /* offset, value */
            break;
        case BitOp:
            total_args += 1; /* operation */
            /* Handle array of source keys */
            if (args->arg_count > 1 && args->args[1].type == CORE_ARG_TYPE_ARRAY) {
                total_args += args->args[1].data.array_arg.count;
            }
            break;
        default:
            return 0;
    }

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int arg_idx = 0;

    /* Handle BitOp differently - operation comes first */
    if (args->cmd_type == BitOp) {
        /* Add operation */
        (*cmd_args)[arg_idx]     = (uintptr_t) args->args[0].data.string_arg.value;
        (*cmd_args_len)[arg_idx] = args->args[0].data.string_arg.len;
        arg_idx++;

        /* Add destination key */
        (*cmd_args)[arg_idx]     = (uintptr_t) args->key;
        (*cmd_args_len)[arg_idx] = args->key_len;
        arg_idx++;

        /* Add source keys from array */
        if (args->arg_count > 1 && args->args[1].type == CORE_ARG_TYPE_ARRAY) {
            zval* array = args->args[1].data.array_arg.array;
            if (Z_TYPE_P(array) == IS_ARRAY) {
                HashTable* ht = Z_ARRVAL_P(array);
                zval*      element;

                ZEND_HASH_FOREACH_VAL(ht, element) {
                    if (Z_TYPE_P(element) == IS_STRING) {
                        (*cmd_args)[arg_idx]     = (uintptr_t) Z_STRVAL_P(element);
                        (*cmd_args_len)[arg_idx] = Z_STRLEN_P(element);
                        arg_idx++;
                    } else {
                        /* Convert non-string to string */
                        zend_string* str = zval_get_string(element);
                        if (str) {
                            size_t len      = ZSTR_LEN(str);
                            char*  str_copy = emalloc(len + 1);
                            if (str_copy) {
                                memcpy(str_copy, ZSTR_VAL(str), len);
                                str_copy[len]            = '\0';
                                (*cmd_args)[arg_idx]     = (uintptr_t) str_copy;
                                (*cmd_args_len)[arg_idx] = len;
                                add_tracked_string(*allocated_strings, allocated_count, str_copy);
                                arg_idx++;
                            }
                            zend_string_release(str);
                        }
                    }
                }
                ZEND_HASH_FOREACH_END();
            }
        }
    } else {
        /* Add key first for other bit operations */
        (*cmd_args)[arg_idx]     = (uintptr_t) args->key;
        (*cmd_args_len)[arg_idx] = args->key_len;
        arg_idx++;

        /* Add arguments based on command type */
        for (int i = 0; i < args->arg_count; i++) {
            switch (args->args[i].type) {
                case CORE_ARG_TYPE_LONG: {
                    size_t len;
                    char*  str = core_long_to_string(args->args[i].data.long_arg.value, &len);
                    if (str) {
                        (*cmd_args)[arg_idx]     = (uintptr_t) str;
                        (*cmd_args_len)[arg_idx] = len;
                        add_tracked_string(*allocated_strings, allocated_count, str);
                        arg_idx++;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        /* Add range arguments if present */
        if (args->options.has_range) {
            size_t len;
            char*  start_str = core_long_to_string(args->options.start, &len);
            if (start_str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) start_str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, start_str);
                arg_idx++;
            }

            char* end_str = core_long_to_string(args->options.end, &len);
            if (end_str) {
                (*cmd_args)[arg_idx]     = (uintptr_t) end_str;
                (*cmd_args_len)[arg_idx] = len;
                add_tracked_string(*allocated_strings, allocated_count, end_str);
                arg_idx++;
            }
        }

        /* Add BYBIT flag if present */
        if (args->options.bybit) {
            (*cmd_args)[arg_idx]     = (uintptr_t) "BIT";
            (*cmd_args_len)[arg_idx] = 3;
            arg_idx++;
        }
    }

    return arg_idx;
}

/**
 * Prepare arguments for expire operations
 */
int prepare_expire_args(core_command_args_t* args,
                        uintptr_t**          cmd_args,
                        unsigned long**      cmd_args_len,
                        char***              allocated_strings,
                        int*                 allocated_count) {
    if (!args->key || args->key_len == 0 || args->arg_count == 0) {
        return 0;
    }

    /* Calculate total arguments: key + time + optional mode */
    int total_args = 1 + args->arg_count; /* key + all provided arguments */

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int arg_idx = 0;

    /* Add key */
    (*cmd_args)[arg_idx]     = (uintptr_t) args->key;
    (*cmd_args_len)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add all arguments (time value and optional mode) */
    for (int i = 0; i < args->arg_count; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_LONG: {
                size_t len;
                char*  str = core_long_to_string(args->args[i].data.long_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }
            case CORE_ARG_TYPE_STRING:
                (*cmd_args)[arg_idx]     = (uintptr_t) args->args[i].data.string_arg.value;
                (*cmd_args_len)[arg_idx] = args->args[i].data.string_arg.len;
                arg_idx++;
                break;
            default:
                break;
        }
    }

    return arg_idx;
}

/**
 * Prepare arguments for range operations
 */
int prepare_range_args(core_command_args_t* args,
                       uintptr_t**          cmd_args,
                       unsigned long**      cmd_args_len,
                       char***              allocated_strings,
                       int*                 allocated_count) {
    if (!args->key || args->key_len == 0) {
        return 0;
    }

    int total_args = 1; /* key */

    /* Add arguments based on command type */
    switch (args->cmd_type) {
        case GetRange:
            total_args += 2; /* start, end */
            break;
        case SetRange:
            total_args += 2; /* offset, value */
            break;
        default:
            return 0;
    }

    if (!allocate_core_arg_arrays(total_args, cmd_args, cmd_args_len)) {
        return 0;
    }

    *allocated_strings = create_string_tracker(total_args);
    *allocated_count   = 0;

    int arg_idx = 0;

    /* Add key */
    (*cmd_args)[arg_idx]     = (uintptr_t) args->key;
    (*cmd_args_len)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add range-specific arguments */
    for (int i = 0; i < args->arg_count && arg_idx < total_args; i++) {
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_LONG: {
                size_t len;
                char*  str = core_long_to_string(args->args[i].data.long_arg.value, &len);
                if (str) {
                    (*cmd_args)[arg_idx]     = (uintptr_t) str;
                    (*cmd_args_len)[arg_idx] = len;
                    add_tracked_string(*allocated_strings, allocated_count, str);
                    arg_idx++;
                }
                break;
            }
            case CORE_ARG_TYPE_STRING:
                (*cmd_args)[arg_idx]     = (uintptr_t) args->args[i].data.string_arg.value;
                (*cmd_args_len)[arg_idx] = args->args[i].data.string_arg.len;
                arg_idx++;
                break;
            default:
                break;
        }
    }

    return arg_idx;
}

/* ====================================================================
 * RESULT PROCESSORS
 * ==================================================================== */


/**
 * Process array result
 */
int process_core_array_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        return 0;
    }

    return command_response_to_zval(response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, true);
}

/**
 * Process double result
 */
int process_core_double_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_DOUBLE(return_value, 0.0);
        return 0;
    }

    if (response->response_type == Float) {
        ZVAL_DOUBLE(return_value, response->float_value);
        return 1;
    } else if (response->response_type == String) {
        char*  endptr;
        double res = strtod(response->string_value, &endptr);
        ZVAL_DOUBLE(return_value, res);
        if (endptr != response->string_value && *endptr == '\0') {
            return 1;
        }
    }
    ZVAL_DOUBLE(return_value, 0.0);
    return 0;
}


/**
 * Process TYPE command result (maps ValkeyGlide type strings to PHP constants)
 */
int process_core_type_result(CommandResponse* response, void* output, zval* return_value) {
    long type_code = -1;

    if (!response) {
        return -1;
    }

    if (response->response_type == String && response->string_value &&
        response->string_value_len > 0) {
        char*  type_str = response->string_value;
        size_t str_len  = response->string_value_len;

        /* Map ValkeyGlide type strings to PHP constants using safe bounded comparisons */
        if (str_len >= 6 && strncmp(type_str, "string", 6) == 0) {
            type_code = 1; /* VALKEY_GLIDE_STRING */
        } else if (str_len >= 4 && strncmp(type_str, "list", 4) == 0) {
            type_code = 3; /* VALKEY_GLIDE_LIST */
        } else if (str_len >= 3 && strncmp(type_str, "set", 3) == 0) {
            type_code = 2; /* VALKEY_GLIDE_SET */
        } else if (str_len >= 4 && strncmp(type_str, "zset", 4) == 0) {
            type_code = 4; /* VALKEY_GLIDE_ZSET */
        } else if (str_len >= 4 && strncmp(type_str, "hash", 4) == 0) {
            type_code = 5; /* VALKEY_GLIDE_HASH */
        } else if (str_len >= 6 && strncmp(type_str, "stream", 6) == 0) {
            type_code = 6; /* VALKEY_GLIDE_STREAM */
        } else if (str_len >= 4 && strncmp(type_str, "none", 4) == 0) {
            type_code = 0; /* VALKEY_GLIDE_NOT_FOUND */
        } else {
            /* Unknown type, default to NOT_FOUND */
            type_code = 0;
        }
        ZVAL_LONG(return_value, type_code);
        return 1; /* Success */
    } else if (response->response_type == Null) {
        /* Key doesn't exist */
        type_code = 0; /* VALKEY_GLIDE_NOT_FOUND */
        ZVAL_LONG(return_value, type_code);
        return 1;
    }
    ZVAL_LONG(return_value, -1);
    return -1; /* Error */
}

/* ====================================================================
 * MEMORY MANAGEMENT UTILITIES
 * ==================================================================== */

/**
 * Allocate command argument arrays
 */
int allocate_core_arg_arrays(int count, uintptr_t** args_out, unsigned long** args_len_out) {
    *args_out     = (uintptr_t*) emalloc(count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(count * sizeof(unsigned long));


    return 1;
}


/**
 * Create string tracker for memory management
 */
char** create_string_tracker(int max_strings) {
    return (char**) ecalloc(max_strings, sizeof(char*));
}

/**
 * Add string to tracker
 */
void add_tracked_string(char** tracker, int* count, char* str) {
    if (tracker && str) {
        tracker[*count] = str;
        (*count)++;
    }
}

/**
 * Free all tracked strings
 */
void free_tracked_strings(char** tracker, int count) {
    if (!tracker)
        return;

    for (int i = 0; i < count; i++) {
        if (tracker[i]) {
            efree(tracker[i]);
        }
    }
}

/**
 * Convert long to string
 */
char* core_long_to_string(long value, size_t* len) {
    char buffer[32];
    *len      = snprintf(buffer, sizeof(buffer), "%ld", value);
    char* str = (char*) emalloc(*len + 1);
    if (str) {
        memcpy(str, buffer, *len);
        str[*len] = '\0';
    }
    return str;
}

/**
 * Convert double to string
 */
char* core_double_to_string(double value, size_t* len) {
    char buffer[64];
    *len      = snprintf(buffer, sizeof(buffer), "%.6g", value);
    char* str = (char*) emalloc(*len + 1);
    if (str) {
        memcpy(str, buffer, *len);
        str[*len] = '\0';
    }
    return str;
}

/* ====================================================================
 * OPTION PARSING UTILITIES
 * ==================================================================== */

/**
 * Parse common command options
 */
int parse_core_options(zval* options, core_options_t* opts) {
    if (!opts) {
        return 0;
    }

    /* Initialize options */
    memset(opts, 0, sizeof(core_options_t));

    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    HashTable* ht = Z_ARRVAL_P(options);
    zval*      entry;

    /* Parse expiry options */
    if ((entry = zend_hash_str_find(ht, "EX", 2)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->expire_seconds = Z_LVAL_P(entry);
        opts->has_expire     = 1;
    }

    if ((entry = zend_hash_str_find(ht, "PX", 2)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->expire_milliseconds = Z_LVAL_P(entry);
        opts->has_expire          = 1;
        opts->has_pexpire         = 1;
    }

    /* Parse EXAT option - unix timestamp in seconds */
    if ((entry = zend_hash_str_find(ht, "EXAT", 4)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->expire_at_seconds = Z_LVAL_P(entry);
        opts->has_expire        = 1;
        opts->has_exat          = 1;
    }

    /* Parse PXAT option - unix timestamp in milliseconds */
    if ((entry = zend_hash_str_find(ht, "PXAT", 4)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->expire_at_milliseconds = Z_LVAL_P(entry);
        opts->has_expire             = 1;
        opts->has_pxat               = 1;
    }

    /* Parse PERSIST option - associative format ['PERSIST' => true] */
    if ((entry = zend_hash_str_find(ht, "PERSIST", 7)) != NULL) {
        opts->persist = zval_is_true(entry);
    }

    /* Parse indexed array format ['PERSIST'] */
    zend_string* option_key;
    zend_ulong   num_key;
    ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, option_key, entry) {
        if (option_key == NULL && Z_TYPE_P(entry) == IS_STRING) {
            /* Handle numeric keys - check if value is "PERSIST" */
            if (strcasecmp(Z_STRVAL_P(entry), "PERSIST") == 0) {
                opts->persist = 1;
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    /* Parse conditional options */
    if ((entry = zend_hash_str_find(ht, "NX", 2)) != NULL) {
        opts->nx = zval_is_true(entry);
    }

    if ((entry = zend_hash_str_find(ht, "XX", 2)) != NULL) {
        opts->xx = zval_is_true(entry);
    }

    if ((entry = zend_hash_str_find(ht, "CH", 2)) != NULL) {
        opts->ch = zval_is_true(entry);
    }

    if ((entry = zend_hash_str_find(ht, "GET", 3)) != NULL) {
        opts->get_old_value = zval_is_true(entry);
    }

    /* Parse range options */
    if ((entry = zend_hash_str_find(ht, "START", 5)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->start     = Z_LVAL_P(entry);
        opts->has_range = 1;
    }

    if ((entry = zend_hash_str_find(ht, "END", 3)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->end       = Z_LVAL_P(entry);
        opts->has_range = 1;
    }

    if ((entry = zend_hash_str_find(ht, "OFFSET", 6)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->offset = Z_LVAL_P(entry);
    }

    if ((entry = zend_hash_str_find(ht, "COUNT", 5)) != NULL && Z_TYPE_P(entry) == IS_LONG) {
        opts->count     = Z_LVAL_P(entry);
        opts->has_limit = 1;
    }

    /* Parse special flags */
    if ((entry = zend_hash_str_find(ht, "BYBIT", 5)) != NULL) {
        opts->bybit = zval_is_true(entry);
    }

    if ((entry = zend_hash_str_find(ht, "APPROXIMATE", 11)) != NULL) {
        opts->approximate = zval_is_true(entry);
    }

    return 1;
}

/**
 * Parse SET command specific options
 */
int parse_set_options(zval* options, core_options_t* opts) {
    if (!opts) {
        return 0;
    }

    /* Initialize options */
    memset(opts, 0, sizeof(core_options_t));

    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    HashTable*   options_ht = Z_ARRVAL_P(options);
    zval*        z_option;
    zend_string* option_key;
    zend_ulong   num_key;

    /* Iterate through all options */
    ZEND_HASH_FOREACH_KEY_VAL(options_ht, num_key, option_key, z_option) {
        if (option_key == NULL) {
            /* Handle numeric keys - these are option flags without values */
            if (Z_TYPE_P(z_option) == IS_STRING) {
                zend_string* opt_str = Z_STR_P(z_option);
                char*        opt     = ZSTR_VAL(opt_str);

                /* NX option */
                if (strcasecmp(opt, "NX") == 0) {
                    opts->nx = 1;
                }
                /* XX option */
                else if (strcasecmp(opt, "XX") == 0) {
                    opts->xx = 1;
                }
                /* GET option */
                else if (strcasecmp(opt, "GET") == 0) {
                    opts->get_old_value = 1;
                }
                /* KEEPTTL option */
                else if (strcasecmp(opt, "KEEPTTL") == 0) {
                    opts->keep_ttl = 1;
                }
            }
        } else {
            /* Handle string keys - these are options with values */
            char* opt = ZSTR_VAL(option_key);

            /* Check for time-based options */
            if (strcasecmp(opt, "EX") == 0) {
                /* EX option - seconds */
                if (Z_TYPE_P(z_option) == IS_LONG || Z_TYPE_P(z_option) == IS_DOUBLE) {
                    long expire_val = zval_get_long(z_option);
                    if (expire_val > 0) {
                        opts->expire_seconds = expire_val;
                        opts->has_expire     = 1;
                        /* Reset other time options */
                        opts->has_pexpire = opts->has_exat = opts->has_pxat = 0;
                    } else {
                        /* Invalid expire value */
                        return 0;
                    }
                } else {
                    /* Invalid value type for EX option - should be numeric */
                    return 0;
                }
            } else if (strcasecmp(opt, "PX") == 0) {
                /* PX option - milliseconds */
                if (Z_TYPE_P(z_option) == IS_LONG || Z_TYPE_P(z_option) == IS_DOUBLE) {
                    long expire_val = zval_get_long(z_option);
                    if (expire_val > 0) {
                        opts->expire_milliseconds = expire_val;
                        opts->has_expire          = 1;
                        opts->has_pexpire         = 1;
                        /* Reset other time options */
                        opts->has_exat = opts->has_pxat = 0;
                    } else {
                        /* Invalid expire value */
                        return 0;
                    }
                } else {
                    /* Invalid value type for PX option - should be numeric */
                    return 0;
                }
            } else if (strcasecmp(opt, "EXAT") == 0) {
                /* EXAT option - unix time in seconds */
                if (Z_TYPE_P(z_option) == IS_LONG || Z_TYPE_P(z_option) == IS_DOUBLE) {
                    long expire_val = zval_get_long(z_option);
                    if (expire_val > 0) {
                        opts->expire_at_seconds = expire_val;
                        opts->has_expire        = 1;
                        opts->has_exat          = 1;
                        /* Reset other time options */
                        opts->has_pexpire = opts->has_pxat = 0;
                    } else {
                        /* Invalid expire value */
                        return 0;
                    }
                } else {
                    /* Invalid value type for EXAT option - should be numeric */
                    return 0;
                }
            } else if (strcasecmp(opt, "PXAT") == 0) {
                /* PXAT option - unix time in milliseconds */
                if (Z_TYPE_P(z_option) == IS_LONG || Z_TYPE_P(z_option) == IS_DOUBLE) {
                    long expire_val = zval_get_long(z_option);
                    if (expire_val > 0) {
                        opts->expire_at_milliseconds = expire_val;
                        opts->has_expire             = 1;
                        opts->has_pxat               = 1;
                        /* Reset other time options */
                        opts->has_pexpire = opts->has_exat = 0;
                    } else {
                        /* Invalid expire value */
                        return 0;
                    }
                } else {
                    /* Invalid value type for PXAT option - should be numeric */
                    return 0;
                }
            }
            /* IFEQ option */
            else if (strcasecmp(opt, "IFEQ") == 0) {
                /* IFEQ option - comparison value */
                if (Z_TYPE_P(z_option) == IS_STRING) {
                    opts->ifeq_value = Z_STRVAL_P(z_option);
                    opts->ifeq_len   = Z_STRLEN_P(z_option);
                    opts->has_ifeq   = 1;
                } else {
                    /* Invalid value type for IFEQ option - should be string */
                    return 0;
                }
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    return 1;
}

int process_core_int_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_LONG(return_value, 0);
        return 0;
    }

    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    } else if (response->response_type == Bool) {
        ZVAL_LONG(return_value, (long) response->bool_value ? 1 : 0);
        return 1;
    } else if (response->response_type == Ok) {
        ZVAL_TRUE(return_value);
        return 1;
    }
    ZVAL_LONG(return_value, 0);
    return 0;
}

/**
 * Batch-compatible wrapper for string results
 */
int process_core_string_result(CommandResponse* response, void* output, zval* return_value) {
    char*  result = NULL;
    size_t result_len;


    if (!response) {
        ZVAL_NULL(return_value);
        return 0;
    }

    if (response->response_type == String) {
        if (response->string_value_len == 0) {
            result = emalloc(1);
            if (result) {
                (result)[0] = '\0';
            }
            result_len = 0;
        } else {
            result = emalloc(response->string_value_len + 1);
            if (result) {
                memcpy(result, response->string_value, response->string_value_len);
                (result)[response->string_value_len] = '\0';
            }
            result_len = response->string_value_len;
        }
        if (result) {
            ZVAL_STRINGL(return_value, result, result_len);
            efree(result);
        } else {
            ZVAL_NULL(return_value);
        }

        return 1;
    } else if (response->response_type == Null) {
        ZVAL_FALSE(return_value);
        return 1;
    }

    /* Free the heap-allocated output struct on unknown response type */
    ZVAL_NULL(return_value);
    return 0;
}

/**
 * Batch-compatible wrapper for boolean results
 */
int process_core_bool_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    int result_val = 0;
    if (response->response_type == Bool) {
        result_val = response->bool_value ? 1 : 0;
    } else if (response->response_type == Int) {
        result_val = response->int_value ? 1 : 0;
    } else if (response->response_type == Ok) {
        result_val = 1;
    }

    ZVAL_BOOL(return_value, result_val);

    return 1;
}


/* ====================================================================
 * SPECIALIZED COMMAND HELPERS
 * ==================================================================== */


/**
 * Generic multi-key command handler for DEL, UNLINK, and similar commands with batch support
 * Supports all 3 usage patterns: single key, array, and multiple arguments
 */
int execute_multi_key_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              zval*                keys,
                              int                  keys_count,
                              zval*                object,
                              zval*                return_value) {
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = cmd_type;

    /* Detect single key vs multi-key scenario */
    if (keys_count == 1 && Z_TYPE_P(keys) == IS_STRING) {
        /* Single key case - use single-key mode for efficiency */
        args.key     = Z_STRVAL_P(keys);
        args.key_len = Z_STRLEN_P(keys);

        args.arg_count = 0; /* Triggers single-key mode in core framework */
    } else if (keys_count > 0 && Z_TYPE_P(keys) == IS_ARRAY) {
        /* Multi-key array case */
        args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
        args.args[0].data.array_arg.array = keys;
        args.args[0].data.array_arg.count = keys_count;
        args.arg_count                    = 1; /* Triggers multi-key mode in core framework */
    } else if (keys_count > 1 && Z_TYPE_P(keys) == IS_STRING) {
        /* Multiple separate string arguments case: del('x', 'y', 'z') */
        /* Convert to temporary array for multi-key processing */
        zval temp_array;
        array_init(&temp_array);

        for (int i = 0; i < keys_count; i++) {
            add_next_index_zval(&temp_array, &keys[i]);
        }

        /* Use multi-key mode with temporary array */
        args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
        args.args[0].data.array_arg.array = &temp_array;
        args.args[0].data.array_arg.count = keys_count;
        args.arg_count                    = 1; /* Triggers multi-key mode in core framework */

        /* Execute using core framework with batch support */
        int result =
            execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value);

        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object); /* return_value should already contain $this */
        }

        return result;
    } else {
        /* Invalid input - neither single string, array, nor multiple strings */
        return 0;
    }

    /* Use batch-aware core framework */
    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value);

    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        /* execute_core_command already handles this case */
        ZVAL_COPY(return_value, object); /* return_value should already contain $this */
        return result;
    }


    return result;
}


/* ====================================================================
 * DEBUG FUNCTIONS (only in debug builds)
 * ==================================================================== */

#ifdef DEBUG_VALKEY_GLIDE_PHP
void debug_print_core_args(core_command_args_t* args) {
    if (!args) {
        printf("DEBUG: core_args is NULL\n");
        return;
    }

    printf("DEBUG: Core Command Args:\n");
    printf("  cmd_type: %d\n", args->cmd_type);
    printf("  key: %.*s (len: %zu)\n",
           (int) args->key_len,
           args->key ? args->key : "NULL",
           args->key_len);
    printf("  arg_count: %d\n", args->arg_count);

    for (int i = 0; i < args->arg_count; i++) {
        printf("  arg[%d]: type=%d\n", i, args->args[i].type);
        switch (args->args[i].type) {
            case CORE_ARG_TYPE_STRING:
                printf("    string: %.*s (len: %zu)\n",
                       (int) args->args[i].data.string_arg.len,
                       args->args[i].data.string_arg.value,
                       args->args[i].data.string_arg.len);
                break;
            case CORE_ARG_TYPE_LONG:
                printf("    long: %ld\n", args->args[i].data.long_arg.value);
                break;
            case CORE_ARG_TYPE_DOUBLE:
                printf("    double: %f\n", args->args[i].data.double_arg.value);
                break;
            default:
                printf("    (other type)\n");
                break;
        }
    }

    printf("  options: has_expire=%d, nx=%d, xx=%d\n",
           args->options.has_expire,
           args->options.nx,
           args->options.xx);
}

void debug_print_command_result(CommandResult* result) {
    if (!result) {
        printf("DEBUG: CommandResult is NULL\n");
        return;
    }

    printf("DEBUG: CommandResult:\n");
    printf("  command_error: %s\n", result->command_error ? "YES" : "NO");
    if (result->command_error) {
        printf("  error_message: %s\n",
               result->command_error->command_error_message
                   ? result->command_error->command_error_message
                   : "NULL");
    }

    if (result->response) {
        printf("  response_type: %d\n", result->response->response_type);
        switch (result->response->response_type) {
            case Int:
                printf("  int_value: %ld\n", result->response->int_value);
                break;
            case String:
                printf("  string_value: %.*s (len: %ld)\n",
                       (int) result->response->string_value_len,
                       result->response->string_value,
                       result->response->string_value_len);
                break;
            case Bool:
                printf("  bool_value: %s\n", result->response->bool_value ? "true" : "false");
                break;
            case Float:
                printf("  float_value: %f\n", result->response->float_value);
                break;
            default:
                printf("  (other response type)\n");
                break;
        }
    } else {
        printf("  response: NULL\n");
    }
}
#endif
