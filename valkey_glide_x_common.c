/*
  +----------------------------------------------------------------------+
  | Valkey Glide X-Commands Common Utilities                             |
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

#include "valkey_glide_x_common.h"

#include "valkey_glide_z_common.h"

/* ====================================================================
 * OPTION PARSING FUNCTIONS
 * ==================================================================== */

/**
 * Parse COUNT option common to several X commands.
 */
int parse_x_count_options(zval* options, x_count_options_t* opts) {
    /* Initialize options to default values */
    opts->count     = 0;
    opts->has_count = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Look for COUNT option */
    HashTable* ht = Z_ARRVAL_P(options);
    zval*      z_count;

    /* Check for COUNT option */
    if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
        if (Z_TYPE_P(z_count) == IS_LONG) {
            opts->count     = Z_LVAL_P(z_count);
            opts->has_count = 1;
        }
    }

    return 1;
}

/**
 * Parse XREAD/XREADGROUP command options.
 */
int parse_x_read_options(zval* options, x_read_options_t* opts) {
    /* Initialize options to default values */
    opts->block     = 0;
    opts->has_block = 0;
    opts->count     = 0;
    opts->has_count = 0;
    opts->noack     = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Parse options */
    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_block, *z_count, *z_noack;

    /* Check for BLOCK option */
    if ((z_block = zend_hash_str_find(ht, "BLOCK", sizeof("BLOCK") - 1)) != NULL) {
        if (Z_TYPE_P(z_block) == IS_LONG) {
            opts->block     = Z_LVAL_P(z_block);
            opts->has_block = 1;
        }
    }

    /* Check for COUNT option */
    if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
        if (Z_TYPE_P(z_count) == IS_LONG) {
            opts->count     = Z_LVAL_P(z_count);
            opts->has_count = 1;
        }
    }

    /* Check for NOACK option */
    if ((z_noack = zend_hash_str_find(ht, "NOACK", sizeof("NOACK") - 1)) != NULL) {
        opts->noack = zval_is_true(z_noack);
    } else {
        /* Legacy: Check if NOACK is specified as an array value */
        zval* z_val;
        ZEND_HASH_FOREACH_VAL(ht, z_val) {
            if (Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) == 5 &&
                strcasecmp(Z_STRVAL_P(z_val), "NOACK") == 0) {
                opts->noack = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    return 1;
}

/**
 * Parse XPENDING command options.
 */
int parse_x_pending_options(zval* options, x_pending_options_t* opts) {
    /* Initialize options to default values */
    opts->start        = NULL;
    opts->start_len    = 0;
    opts->end          = NULL;
    opts->end_len      = 0;
    opts->count        = 0;
    opts->has_count    = 0;
    opts->consumer     = NULL;
    opts->consumer_len = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Parse options */
    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_start, *z_end, *z_count, *z_consumer;

    /* Check for START option */
    if ((z_start = zend_hash_str_find(ht, "START", sizeof("START") - 1)) != NULL) {
        if (Z_TYPE_P(z_start) == IS_STRING) {
            opts->start     = Z_STRVAL_P(z_start);
            opts->start_len = Z_STRLEN_P(z_start);
        }
    }

    /* Check for END option */
    if ((z_end = zend_hash_str_find(ht, "END", sizeof("END") - 1)) != NULL) {
        if (Z_TYPE_P(z_end) == IS_STRING) {
            opts->end     = Z_STRVAL_P(z_end);
            opts->end_len = Z_STRLEN_P(z_end);
        }
    }

    /* Check for COUNT option */
    if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
        if (Z_TYPE_P(z_count) == IS_LONG) {
            opts->count     = Z_LVAL_P(z_count);
            opts->has_count = 1;
        }
    }

    /* Check for CONSUMER option */
    if ((z_consumer = zend_hash_str_find(ht, "CONSUMER", sizeof("CONSUMER") - 1)) != NULL) {
        if (Z_TYPE_P(z_consumer) == IS_STRING) {
            opts->consumer     = Z_STRVAL_P(z_consumer);
            opts->consumer_len = Z_STRLEN_P(z_consumer);
        }
    }

    return 1;
}

/**
 * Parse XTRIM command options.
 */
int parse_x_trim_options(zval* options, x_trim_options_t* opts) {
    /* Initialize options to default values */
    opts->approximate = 0;
    opts->limit       = 0;
    opts->has_limit   = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Parse options */
    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_approx, *z_limit;

    /* Check for APPROXIMATE option */
    if ((z_approx = zend_hash_str_find(ht, "APPROXIMATE", sizeof("APPROXIMATE") - 1)) != NULL) {
        opts->approximate = zval_is_true(z_approx);
    }

    /* Check for LIMIT option */
    if ((z_limit = zend_hash_str_find(ht, "LIMIT", sizeof("LIMIT") - 1)) != NULL) {
        if (Z_TYPE_P(z_limit) == IS_LONG) {
            opts->limit     = Z_LVAL_P(z_limit);
            opts->has_limit = 1;
        }
    }

    return 1;
}

/**
 * Parse XADD command options.
 */
int parse_x_add_options(zval* options, x_add_options_t* opts) {
    /* Initialize options to default values */
    opts->maxlen         = 0;
    opts->has_maxlen     = 0;
    opts->approximate    = 0;
    opts->nomkstream     = 0;
    opts->minid_strategy = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Parse options */
    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_maxlen, *z_approx, *z_nomkstream, *z_minid_strategy;

    /* Check for MAXLEN option */
    if ((z_maxlen = zend_hash_str_find(ht, "MAXLEN", sizeof("MAXLEN") - 1)) != NULL) {
        if (Z_TYPE_P(z_maxlen) == IS_LONG) {
            opts->maxlen     = Z_LVAL_P(z_maxlen);
            opts->has_maxlen = 1;
        }
    }

    /* Check for APPROXIMATE option */
    if ((z_approx = zend_hash_str_find(ht, "APPROXIMATE", sizeof("APPROXIMATE") - 1)) != NULL) {
        opts->approximate = zval_is_true(z_approx);
    }

    /* Check for NOMKSTREAM option */
    if ((z_nomkstream = zend_hash_str_find(ht, "NOMKSTREAM", sizeof("NOMKSTREAM") - 1)) != NULL) {
        opts->nomkstream = zval_is_true(z_nomkstream);
    }

    /* Check for MINID strategy option */
    if ((z_minid_strategy = zend_hash_str_find(ht, "MINID", sizeof("MINID") - 1)) != NULL) {
        opts->minid_strategy = zval_is_true(z_minid_strategy);
    }

    return 1;
}

/**
 * Parse XCLAIM/XAUTOCLAIM command options.
 */
int parse_x_claim_options(zval* options, x_claim_options_t* opts) {
    /* Initialize options to default values */
    opts->idle           = 0;
    opts->has_idle       = 0;
    opts->time           = 0;
    opts->has_time       = 0;
    opts->retrycount     = 0;
    opts->has_retrycount = 0;
    opts->force          = 0;
    opts->justid         = 0;

    /* If no options, return success */
    if (!options || Z_TYPE_P(options) != IS_ARRAY) {
        return 1;
    }

    /* Parse options */
    HashTable* ht = Z_ARRVAL_P(options);
    zval *     z_idle, *z_time, *z_retry, *z_force, *z_justid;

    /* Check for IDLE option */
    if ((z_idle = zend_hash_str_find(ht, "IDLE", sizeof("IDLE") - 1)) != NULL) {
        if (Z_TYPE_P(z_idle) == IS_LONG) {
            opts->idle     = Z_LVAL_P(z_idle);
            opts->has_idle = 1;
        }
    }

    /* Check for TIME option */
    if ((z_time = zend_hash_str_find(ht, "TIME", sizeof("TIME") - 1)) != NULL) {
        if (Z_TYPE_P(z_time) == IS_LONG) {
            opts->time     = Z_LVAL_P(z_time);
            opts->has_time = 1;
        }
    }

    /* Check for RETRYCOUNT option */
    if ((z_retry = zend_hash_str_find(ht, "RETRYCOUNT", sizeof("RETRYCOUNT") - 1)) != NULL) {
        if (Z_TYPE_P(z_retry) == IS_LONG) {
            opts->retrycount     = Z_LVAL_P(z_retry);
            opts->has_retrycount = 1;
        }
    }

    /* Check for FORCE option - first check associative key */
    if ((z_force = zend_hash_str_find(ht, "FORCE", sizeof("FORCE") - 1)) != NULL) {
        opts->force = zval_is_true(z_force);
    } else {
        /* If not found as associative key, check array values */
        zval* z_val;
        ZEND_HASH_FOREACH_VAL(ht, z_val) {
            if (Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) == 5 &&
                strcasecmp(Z_STRVAL_P(z_val), "FORCE") == 0) {
                opts->force = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Check for JUSTID option - first check associative key */
    if ((z_justid = zend_hash_str_find(ht, "JUSTID", sizeof("JUSTID") - 1)) != NULL) {
        opts->justid = zval_is_true(z_justid);
    } else {
        /* If not found as associative key, check array values */
        zval* z_val;
        ZEND_HASH_FOREACH_VAL(ht, z_val) {
            if (Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) == 6 &&
                strcasecmp(Z_STRVAL_P(z_val), "JUSTID") == 0) {
                opts->justid = 1;
                break;
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    return 1;
}

/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/**
 * Allocate command arguments arrays
 */
int allocate_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out) {
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
void free_command_args(uintptr_t* args, unsigned long* args_len) {
    if (args)
        efree(args);
    if (args_len)
        efree(args_len);
}


/**
 * Generic command execution framework with integrated batch support
 */
int execute_x_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              x_command_args_t*    args,
                              void*                result_ptr,
                              x_result_processor_t process_result,
                              zval*                return_value) {
    /* Check if valkey_glide object is valid */
    if (!valkey_glide) {
        return 0;
    }

    /* Prepare arguments ONCE - single switch statement eliminates duplication */
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;

    /* Single argument preparation logic for both batch and normal modes */
    switch (cmd_type) {
        case XGroupCreate:
        case XGroupCreateConsumer:
        case XGroupDelConsumer:
        case XGroupDestroy:
        case XGroupSetId:
            arg_count = prepare_x_group_args(args, &cmd_args, &args_len);
            break;
        case XLen:
            arg_count = prepare_x_len_args(args, &cmd_args, &args_len);
            break;
        case XDel:
            arg_count = prepare_x_del_args(args, &cmd_args, &args_len);
            break;
        case XAck:
            arg_count = prepare_x_ack_args(args, &cmd_args, &args_len);
            break;
        case XAdd:
            arg_count = prepare_x_add_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case XTrim:
            arg_count = prepare_x_trim_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case XRange:
        case XRevRange:
            arg_count = prepare_x_range_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        case XPending:
            arg_count = prepare_x_pending_args(args, &cmd_args, &args_len);
            break;
        case XRead:
            arg_count = prepare_x_read_args(args, &cmd_args, &args_len);
            break;
        case XReadGroup:
            arg_count = prepare_x_readgroup_args(args, &cmd_args, &args_len);
            break;
        case XAutoClaim:
            arg_count = prepare_x_autoclaim_args(args, &cmd_args, &args_len);
            break;
        case XClaim:
            arg_count = prepare_x_claim_args(args, &cmd_args, &args_len);
            break;
        case XInfoGroups:
        case XInfoConsumers:
        case XInfoStream:
            /* XINFO commands need special handling for allocated strings */
            arg_count = prepare_x_info_args(
                args, &cmd_args, &args_len, &allocated_strings, &allocated_count);
            break;
        default:
            printf("Unknown command type: %d\n", cmd_type);
            return 0;
    }

    /* Check if argument preparation was successful */
    if (arg_count <= 0) {
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        if (allocated_strings)
            efree(allocated_strings);
        return 0;
    }

    if (valkey_glide->is_in_batch_mode) {
        int result = buffer_command_for_batch(valkey_glide,
                                              cmd_type,
                                              cmd_args,
                                              args_len,
                                              arg_count,

                                              result_ptr,
                                              process_result);

        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);

        return result;
    }

    /* Execute the command */
    CommandResult* result =
        execute_command(valkey_glide->glide_client, cmd_type, arg_count, cmd_args, args_len);

    /* Free allocated strings */
    int i;
    for (i = 0; i < allocated_count; i++) {
        if (allocated_strings[i]) {
            efree(allocated_strings[i]);
        }
    }
    if (allocated_strings)
        efree(allocated_strings);
    if (cmd_args)
        efree(cmd_args);
    if (args_len)
        efree(args_len);

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
 * RESULT PROCESSING FUNCTIONS
 * ==================================================================== */

/**
 * Process an integer result from a command
 */
int process_x_int_result(CommandResponse* response, void* output, zval* return_value) {
    /* For ValkeyGlide stream commands, integer response is the count */
    if (response->response_type == Int) {
        ZVAL_LONG(return_value, response->int_value);
        return 1;
    }

    return 0;
}


/**
 * Process a stream result from a command
 */
int process_x_stream_result(CommandResponse* response, void* output, zval* return_value) {
    /* Use the command_response_to_stream_zval function */
    return command_response_to_stream_zval(response, return_value);
}

/**
 * Process an XADD result from a command
 */
int process_x_add_result(CommandResponse* response, void* output, zval* return_value) {
    /* XADD returns the ID string, convert to proper output */
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Process an XGROUP result from a command
 */
int process_x_group_result(CommandResponse* response, void* output, zval* return_value) {
    /* XGROUP response depends on subcommand */
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
}

/**
 * Process an XPENDING result from a command
 */
int process_x_pending_result(CommandResponse* response, void* output, zval* return_value) {
    int status = 0;

    /* XPENDING returns pending entries info */
    status =
        command_response_to_zval(response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);

    /* Special handling for empty XPENDING response */
    if (status && Z_TYPE_P(return_value) == IS_ARRAY) {
        HashTable* ht           = Z_ARRVAL_P(return_value);
        int        num_elements = zend_hash_num_elements(ht);

        if (num_elements > 0) {
            /* Iterate through all elements */
            zval*      element;
            zend_ulong idx;

            ZEND_HASH_FOREACH_NUM_KEY_VAL(ht, idx, element) {
                /* If element is NULL, convert it to bool(false) */
                if (Z_TYPE_P(element) == IS_NULL) {
                    ZVAL_BOOL(element, 0);
                }
            }
            ZEND_HASH_FOREACH_END();
        }
    }

    return status;
}

/**
 * Process an XREADGROUP result from a command
 */
int process_x_readgroup_result(CommandResponse* response, void* output, zval* return_value) {
    int status = 0;

    /* Initialize array for results */
    array_init(return_value);

    if (response->response_type == Map && response->array_value_len > 0) {
        /* Process each stream in the map */
        for (int i = 0; i < response->array_value_len; i++) {
            CommandResponse* element = &response->array_value[i];

            if (element->map_key && element->map_key->response_type == String &&
                element->map_value) {
                /* Extract stream name */
                zval stream_name;
                command_response_to_zval(
                    element->map_key, &stream_name, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);

                /* Process stream entries */
                zval stream_entries;
                command_response_to_stream_zval(element->map_value, &stream_entries);

                /* Add stream entries to output as an associative array */
                add_assoc_zval(return_value, Z_STRVAL(stream_name), &stream_entries);
                zval_dtor(&stream_name);  // Clean up stream name after adding
                status = 1;
            }
        }
    }

    return status;
}

/**
 * Process an XCLAIM result from a command
 */
int process_x_claim_result(CommandResponse* response, void* output, zval* return_value) {
    x_claim_result_context_t* ctx    = (x_claim_result_context_t*) output;
    int                       status = 0;
    int                       justid = ctx->justid;
    if (justid) {
        /* If JUSTID was specified, return an array of IDs */
        status = command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    } else {
        /* Otherwise, return the full entries */
        status = command_response_to_stream_zval(response, return_value);
    }
    efree(ctx); /* Free the context */

    return status;
}

/**
 * Process an XAUTOCLAIM result from a command
 */
int process_x_autoclaim_result(CommandResponse* response, void* output, zval* return_value) {
    /* XAUTOCLAIM returns a multi-part response */
    if (response->response_type != Array || response->array_value_len < 1) {
        return 0;
    }

    /* Initialize the return array */
    array_init(return_value);

    /* Extract the cursor (first element) */
    CommandResponse* cursor_response = &response->array_value[0];
    if (cursor_response->response_type == String) {
        zval cursor_zval;
        ZVAL_STRINGL(
            &cursor_zval, cursor_response->string_value, cursor_response->string_value_len);
        add_next_index_zval(return_value, &cursor_zval);
    } else {
        /* If not a string, add null */
        add_next_index_null(return_value);
    }

    /* Extract the messages (second element) */
    if (response->array_value_len >= 2) {
        CommandResponse* messages_response = &response->array_value[1];
        zval             messages_zval;

        /* Process the messages using stream format */
        command_response_to_stream_zval(messages_response, &messages_zval);
        add_next_index_zval(return_value, &messages_zval);
    } else {
        /* If no messages, add empty array */
        zval messages_zval;
        array_init(&messages_zval);
        add_next_index_zval(return_value, &messages_zval);
    }

    /* Extract deleted IDs if present (third element) */
    if (response->array_value_len >= 3) {
        CommandResponse* deleted_response = &response->array_value[2];
        zval             deleted_zval;
        command_response_to_zval(
            deleted_response, &deleted_zval, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        add_next_index_zval(return_value, &deleted_zval);
    } else {
        /* If no deleted IDs, add empty array */
        zval deleted_zval;
        array_init(&deleted_zval);
        add_next_index_zval(return_value, &deleted_zval);
    }

    return 1;
}

/**
 * Process an XINFO result from a command
 */
int process_x_info_result(CommandResponse* response, void* output, zval* return_value) {
    /* XINFO returns information about the stream or consumers in associative array format */
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
}

/**
 * Prepare arguments for XINFO command.
 */
int prepare_x_info_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->subcommand || args->subcommand_len <= 0) {
        return 0;
    }

    /* Initialize allocated strings tracking */
    *allocated_strings = NULL;
    *allocated_count   = 0;

    /* Calculate arg count based on subcommand */
    unsigned long    arg_count    = 0;
    enum RequestType command_type = 0;

    /* Determine which XINFO command to use based on subcommand */
    if (strcasecmp(args->subcommand, "CONSUMERS") == 0) {
        command_type = XInfoConsumers;

        /* We need key + group */
        if (!args->args || args->args_count < 2) {
            return 0;
        }

        arg_count = 2; /* key and group */
    } else if (strcasecmp(args->subcommand, "GROUPS") == 0) {
        command_type = XInfoGroups;

        /* We need at least key */
        if (!args->args || args->args_count < 1) {
            return 0;
        }

        arg_count = 1; /* just key */
    } else if (strcasecmp(args->subcommand, "STREAM") == 0) {
        command_type = XInfoStream;

        /* We need at least key */
        if (!args->args || args->args_count < 1) {
            return 0;
        }

        /* Count options */
        unsigned long extra_args = 0;
        zend_bool     has_full   = 0;
        zend_bool     has_count  = 0;

        if (args->args_count >= 2) {
            /* Check if FULL option is present */
            if (Z_TYPE(args->args[1]) == IS_STRING &&
                strcasecmp(Z_STRVAL(args->args[1]), "FULL") == 0) {
                has_full = 1;
                extra_args += 1; /* FULL */

                /* Check for COUNT option */
                if (args->args_count >= 3 && Z_TYPE(args->args[2]) != IS_NULL) {
                    if (Z_TYPE(args->args[2]) == IS_LONG) {
                        long count_value = Z_LVAL(args->args[2]);
                        if (count_value != -1) {
                            has_count = 1;
                            extra_args += 2; /* COUNT + value */
                        }
                    }
                }
            }
        }

        arg_count = 1 + extra_args; /* key + options */

        /* Allocate memory to track dynamic strings */
        if (has_count) {
            *allocated_strings = (char**) ecalloc(1, sizeof(char*));
            if (!*allocated_strings) {
                return 0;
            }
        }
    } else {
        /* Unknown subcommand */
        return 0;
    }

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        if (*allocated_strings) {
            efree(*allocated_strings);
            *allocated_strings = NULL;
        }
        return 0;
    }

    /* Set arguments based on subcommand */
    unsigned int arg_idx = 0;

    if (strcasecmp(args->subcommand, "CONSUMERS") == 0) {
        /* First argument: key */
        if (Z_TYPE(args->args[0]) == IS_STRING) {
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        } else {
            convert_to_string(&args->args[0]);
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        }

        /* Second argument: group */
        if (Z_TYPE(args->args[1]) == IS_STRING) {
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[1]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[1]);
            arg_idx++;
        } else {
            convert_to_string(&args->args[1]);
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[1]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[1]);
            arg_idx++;
        }
    } else if (strcasecmp(args->subcommand, "GROUPS") == 0) {
        /* Only argument: key */
        if (Z_TYPE(args->args[0]) == IS_STRING) {
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        } else {
            convert_to_string(&args->args[0]);
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        }
    } else if (strcasecmp(args->subcommand, "STREAM") == 0) {
        /* First argument: key */
        if (Z_TYPE(args->args[0]) == IS_STRING) {
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        } else {
            convert_to_string(&args->args[0]);
            (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL(args->args[0]);
            (*args_len_out)[arg_idx] = Z_STRLEN(args->args[0]);
            arg_idx++;
        }

        /* Check for FULL option */
        zend_bool has_full = 0;
        if (args->args_count >= 2 && Z_TYPE(args->args[1]) == IS_STRING &&
            strcasecmp(Z_STRVAL(args->args[1]), "FULL") == 0) {
            has_full                 = 1;
            (*args_out)[arg_idx]     = (uintptr_t) "FULL";
            (*args_len_out)[arg_idx] = sizeof("FULL") - 1;
            arg_idx++;

            /* Check for COUNT option */
            if (has_full && args->args_count >= 3 && Z_TYPE(args->args[2]) != IS_NULL) {
                zend_bool has_count   = 0;
                long      count_value = 0;

                if (Z_TYPE(args->args[2]) == IS_LONG) {
                    count_value = Z_LVAL(args->args[2]);
                    if (count_value != -1) {
                        has_count = 1;
                    }
                } else if (Z_TYPE(args->args[2]) == IS_STRING) {
                    if (Z_STRLEN(args->args[2]) != 2 ||
                        strcmp(Z_STRVAL(args->args[2]), "-1") != 0) {
                        has_count   = 1;
                        count_value = atol(Z_STRVAL(args->args[2]));
                    }
                }

                if (has_count) {
                    (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
                    (*args_len_out)[arg_idx] = sizeof("COUNT") - 1;
                    arg_idx++;

                    /* Convert count to string */
                    char   count_str[32];
                    size_t count_str_len =
                        snprintf(count_str, sizeof(count_str), "%ld", count_value);
                    char* count_copy = estrndup(count_str, count_str_len);
                    if (count_copy) {
                        (*allocated_strings)[*allocated_count] = count_copy;
                        (*allocated_count)++;

                        (*args_out)[arg_idx]     = (uintptr_t) count_copy;
                        (*args_len_out)[arg_idx] = count_str_len;
                        arg_idx++;
                    }
                }
            }
        }
    }

    return arg_count;
}

/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */

/**
 * Prepare arguments for XLEN command.
 */
int prepare_x_len_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out) {
    /* Check if client and key are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0) {
        return 0;
    }

    /* Allocate memory for arguments */
    *args_out     = (uintptr_t*) emalloc(sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(sizeof(unsigned long));

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key as the only argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    return 1;
}

/**
 * Prepare arguments for XACK command.
 */
int prepare_x_ack_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->group ||
        args->group_len <= 0 || !args->ids || args->id_count <= 0) {
        return 0;
    }

    /* Prepare command arguments: key + group + IDs */
    unsigned long arg_count = 2 + args->id_count;
    *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key as first argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Set group as second argument */
    (*args_out)[1]     = (uintptr_t) args->group;
    (*args_len_out)[1] = args->group_len;

    /* Add all stream IDs */
    zval* z_id;
    int   i = 2;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args->ids), z_id) {
        if (Z_TYPE_P(z_id) != IS_STRING) {
            convert_to_string(z_id);
        }
        (*args_out)[i]     = (uintptr_t) Z_STRVAL_P(z_id);
        (*args_len_out)[i] = Z_STRLEN_P(z_id);
        i++;
    }
    ZEND_HASH_FOREACH_END();

    return arg_count;
}

/**
 * Prepare arguments for XDEL command.
 */
int prepare_x_del_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->ids ||
        args->id_count <= 0) {
        return 0;
    }

    /* Prepare command arguments: key + IDs */
    unsigned long arg_count = 1 + args->id_count;
    *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key as first argument */
    (*args_out)[0]     = (uintptr_t) args->key;
    (*args_len_out)[0] = args->key_len;

    /* Add all stream IDs */
    zval* z_id;
    int   i = 1;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args->ids), z_id) {
        if (Z_TYPE_P(z_id) != IS_STRING) {
            convert_to_string(z_id);
        }
        (*args_out)[i]     = (uintptr_t) Z_STRVAL_P(z_id);
        (*args_len_out)[i] = Z_STRLEN_P(z_id);
        i++;
    }
    ZEND_HASH_FOREACH_END();

    return arg_count;
}

/**
 * Prepare arguments for XRANGE/XREVRANGE commands.
 */
int prepare_x_range_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->start ||
        args->start_len <= 0 || !args->end || args->end_len <= 0) {
        return 0;
    }

    /* Calculate total args: key + start + end + (COUNT + count_value) */
    unsigned long arg_count = 1 + 1 + 1 + (args->range_opts.has_count ? 2 : 0);
    *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));


    /* Check if memory allocation was successful */
    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set arguments */
    unsigned int arg_idx = 0;

    /* Key */
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Start ID */
    (*args_out)[arg_idx]     = (uintptr_t) args->start;
    (*args_len_out)[arg_idx] = args->start_len;
    arg_idx++;

    /* End ID */
    (*args_out)[arg_idx]     = (uintptr_t) args->end;
    (*args_len_out)[arg_idx] = args->end_len;
    arg_idx++;

    /* Add COUNT if specified */
    if (args->range_opts.has_count) {
        /* Add COUNT keyword */
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = sizeof("COUNT") - 1;
        arg_idx++;


        /* Convert count to string */
        char          count_str[32];
        unsigned long count_str_len =
            snprintf(count_str, sizeof(count_str), "%ld", args->range_opts.count);

        /* Allocate memory for the string */
        char* count_str_copy                 = emalloc(count_str_len + 1);
        *allocated_strings                   = (char**) ecalloc(1, sizeof(char*));
        *allocated_count                     = 0;
        *allocated_strings[*allocated_count] = count_str_copy;
        if (count_str_copy) {
            memcpy(count_str_copy, count_str, count_str_len);
            count_str_copy[count_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) count_str_copy;
            (*args_len_out)[arg_idx] = count_str_len;
            arg_idx++;
        }
    }

    return arg_count;
}

/**
 * Prepare arguments for XADD command.
 */
int prepare_x_add_args(x_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->id || args->id_len <= 0 ||
        !args->field_values || args->fv_count <= 0) {
        return 0;
    }

    /* Count options */
    unsigned long extra_args = 0;
    if (args->add_opts.nomkstream) {
        extra_args += 1; /* NOMKSTREAM */
    }
    if (args->add_opts.has_maxlen) {
        extra_args += 2; /* MAXLEN/MINID + value */
        if (args->add_opts.approximate) {
            extra_args += 1; /* ~ (tilde) */
        }
    }

    /* Calculate total args: key + options + ID + field/value pairs (each entry is a pair) */
    unsigned long arg_count = 1 + extra_args + 1 + (args->fv_count * 2);
    *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    /* Allocate array to track temporary string allocations */
    *allocated_strings = (char**) ecalloc(args->fv_count + 5, sizeof(char*));
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
    unsigned int arg_idx     = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add NOMKSTREAM if specified */
    if (args->add_opts.nomkstream) {
        (*args_out)[arg_idx]     = (uintptr_t) "NOMKSTREAM";
        (*args_len_out)[arg_idx] = sizeof("NOMKSTREAM") - 1;
        arg_idx++;
    }

    /* Add MAXLEN/MINID if specified */
    if (args->add_opts.has_maxlen) {
        char          maxlen_str[64];
        unsigned long maxlen_str_len = 0;
        maxlen_str_len = snprintf(maxlen_str, sizeof(maxlen_str), "%ld", args->add_opts.maxlen);

        if (args->add_opts.minid_strategy) {
            (*args_out)[arg_idx]     = (uintptr_t) "MINID";
            (*args_len_out)[arg_idx] = sizeof("MINID") - 1;
        } else {
            (*args_out)[arg_idx]     = (uintptr_t) "MAXLEN";
            (*args_len_out)[arg_idx] = sizeof("MAXLEN") - 1;
        }
        arg_idx++;

        /* Add ~ for approximate trimming */
        if (args->add_opts.approximate) {
            (*args_out)[arg_idx]     = (uintptr_t) "~";
            (*args_len_out)[arg_idx] = 1;
            arg_idx++;
        }

        /* Add the threshold value - need to allocate a copy for persistence */
        char* maxlen_str_copy = estrndup(maxlen_str, maxlen_str_len);
        if (maxlen_str_copy) {
            (*allocated_strings)[*allocated_count] = maxlen_str_copy;
            (*allocated_count)++;

            (*args_out)[arg_idx]     = (uintptr_t) maxlen_str_copy;
            (*args_len_out)[arg_idx] = maxlen_str_len;
            arg_idx++;
        }
    }

    /* Add stream ID */
    (*args_out)[arg_idx]     = (uintptr_t) args->id;
    (*args_len_out)[arg_idx] = args->id_len;
    arg_idx++;

    /* Add field-value pairs */
    HashTable*   ht = Z_ARRVAL_P(args->field_values);
    zend_string* field_str;
    zval*        z_value;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, field_str, z_value) {
        /* Add field name */
        if (field_str) {
            (*args_out)[arg_idx]     = (uintptr_t) ZSTR_VAL(field_str);
            (*args_len_out)[arg_idx] = ZSTR_LEN(field_str);
            arg_idx++;

            /* Add field value, convert to string if needed */
            if (Z_TYPE_P(z_value) != IS_STRING) {
                zval temp;
                ZVAL_COPY(&temp, z_value);
                convert_to_string(&temp);

                /* Create persistent copy of the string */
                char* str_copy = estrndup(Z_STRVAL(temp), Z_STRLEN(temp));
                if (str_copy) {
                    (*allocated_strings)[*allocated_count] = str_copy;
                    (*allocated_count)++;

                    (*args_out)[arg_idx]     = (uintptr_t) str_copy;
                    (*args_len_out)[arg_idx] = Z_STRLEN(temp);
                    arg_idx++;
                }

                zval_dtor(&temp);
            } else {
                (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_value);
                (*args_len_out)[arg_idx] = Z_STRLEN_P(z_value);
                arg_idx++;
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    return arg_idx; /* Return the actual number of arguments used */
}

/**
 * Prepare arguments for XGROUP command.
 */
int prepare_x_group_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->subcommand || args->subcommand_len <= 0 || !args->args) {
        return 0;
    }

    /* Allocate memory for arguments */
    if (!allocate_command_args(args->args_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set subcommand as first argument */
    unsigned int arg_idx = 0;
    ////   (*args_out)[arg_idx] = (uintptr_t)args->subcommand;
    //    (*args_len_out)[arg_idx] = args->subcommand_len;
    //  arg_idx++;

    /* Add all additional arguments */
    for (int i = 0; i < args->args_count; i++) {
        zval* arg = &args->args[i];

        /* Convert to string if not already a string */
        if (Z_TYPE_P(arg) != IS_STRING) {
            convert_to_string(arg);
        }

        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(arg);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(arg);
        arg_idx++;
    }

    return args->args_count;
}

/**
 * Prepare arguments for XPENDING command.
 */
int prepare_x_pending_args(x_command_args_t* args,
                           uintptr_t**       args_out,
                           unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->group ||
        args->group_len <= 0) {
        return 0;
    }

    /* Count extra args based on options */
    unsigned long extra_args = 0;
    if (args->pending_opts.start)
        extra_args++; /* START */
    if (args->pending_opts.end)
        extra_args++; /* END */
    if (args->pending_opts.has_count)
        extra_args++; /* COUNT */
    if (args->pending_opts.consumer)
        extra_args++; /* CONSUMER */

    /* Calculate total args: key + group + optional args */
    unsigned long arg_count = 2 + extra_args;

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set key and group as first two arguments */
    unsigned int arg_idx     = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->group;
    (*args_len_out)[arg_idx] = args->group_len;
    arg_idx++;

    /* Add additional options */
    if (args->pending_opts.start) {
        (*args_out)[arg_idx]     = (uintptr_t) args->pending_opts.start;
        (*args_len_out)[arg_idx] = args->pending_opts.start_len;
        arg_idx++;
    }

    if (args->pending_opts.end) {
        (*args_out)[arg_idx]     = (uintptr_t) args->pending_opts.end;
        (*args_len_out)[arg_idx] = args->pending_opts.end_len;
        arg_idx++;
    }

    if (args->pending_opts.has_count) {
        /* Convert count to string */
        char          count_str[32];
        unsigned long count_str_len =
            snprintf(count_str, sizeof(count_str), "%ld", args->pending_opts.count);

        /* Allocate memory for the count string */
        char* count_str_copy = emalloc(count_str_len + 1);
        if (count_str_copy) {
            memcpy(count_str_copy, count_str, count_str_len);
            count_str_copy[count_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) count_str_copy;
            (*args_len_out)[arg_idx] = count_str_len;
            arg_idx++;

            /* This needs to be freed later */
            (*args_out)[arg_count - 1] = (uintptr_t) count_str_copy;
        }
    }

    if (args->pending_opts.consumer) {
        (*args_out)[arg_idx]     = (uintptr_t) args->pending_opts.consumer;
        (*args_len_out)[arg_idx] = args->pending_opts.consumer_len;
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare arguments for XREADGROUP command.
 */
int prepare_x_readgroup_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->group || args->group_len <= 0 || !args->consumer ||
        args->consumer_len <= 0 || !args->streams || !args->ids) {
        return 0;
    }

    /* Get the number of streams and IDs */
    HashTable* streams_ht    = Z_ARRVAL_P(args->streams);
    HashTable* ids_ht        = Z_ARRVAL_P(args->ids);
    int        streams_count = zend_hash_num_elements(streams_ht);
    int        ids_count     = zend_hash_num_elements(ids_ht);

    /* Check counts match */
    if (streams_count <= 0 || streams_count != ids_count) {
        return 0;
    }

    /* Count options */
    unsigned long extra_args = 0;
    if (args->read_opts.has_count)
        extra_args += 2; /* COUNT + value */
    if (args->read_opts.has_block)
        extra_args += 2; /* BLOCK + value */
    if (args->read_opts.noack)
        extra_args += 1; /* NOACK */

    /* Calculate total args: GROUP + group + consumer + options + STREAMS + streams + ids */
    unsigned long arg_count = 3 + extra_args + 1 + streams_count + ids_count;

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set arguments */
    unsigned int arg_idx = 0;

    /* Add GROUP, group, consumer */
    (*args_out)[arg_idx]     = (uintptr_t) "GROUP";
    (*args_len_out)[arg_idx] = sizeof("GROUP") - 1;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->group;
    (*args_len_out)[arg_idx] = args->group_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->consumer;
    (*args_len_out)[arg_idx] = args->consumer_len;
    arg_idx++;

    /* Add COUNT if specified */
    if (args->read_opts.has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = sizeof("COUNT") - 1;
        arg_idx++;

        /* Convert count to string */
        char          count_str[32];
        unsigned long count_str_len =
            snprintf(count_str, sizeof(count_str), "%ld", args->read_opts.count);

        /* Allocate memory for the count string */
        char* count_str_copy = emalloc(count_str_len + 1);
        if (count_str_copy) {
            memcpy(count_str_copy, count_str, count_str_len);
            count_str_copy[count_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) count_str_copy;
            (*args_len_out)[arg_idx] = count_str_len;
            arg_idx++;
        }
    }

    /* Add BLOCK if specified */
    if (args->read_opts.has_block) {
        (*args_out)[arg_idx]     = (uintptr_t) "BLOCK";
        (*args_len_out)[arg_idx] = sizeof("BLOCK") - 1;
        arg_idx++;

        /* Convert block to string */
        char          block_str[32];
        unsigned long block_str_len =
            snprintf(block_str, sizeof(block_str), "%ld", args->read_opts.block);

        /* Allocate memory for the block string */
        char* block_str_copy = emalloc(block_str_len + 1);
        if (block_str_copy) {
            memcpy(block_str_copy, block_str, block_str_len);
            block_str_copy[block_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) block_str_copy;
            (*args_len_out)[arg_idx] = block_str_len;
            arg_idx++;
        }
    }

    /* Add NOACK if specified */
    if (args->read_opts.noack) {
        (*args_out)[arg_idx]     = (uintptr_t) "NOACK";
        (*args_len_out)[arg_idx] = sizeof("NOACK") - 1;
        arg_idx++;
    }

    /* Add STREAMS keyword */
    (*args_out)[arg_idx]     = (uintptr_t) "STREAMS";
    (*args_len_out)[arg_idx] = sizeof("STREAMS") - 1;
    arg_idx++;

    /* Add all stream keys */
    zval* z_stream;
    ZEND_HASH_FOREACH_VAL(streams_ht, z_stream) {
        if (Z_TYPE_P(z_stream) != IS_STRING) {
            convert_to_string(z_stream);
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_stream);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_stream);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add all stream IDs */
    zval* z_id;
    ZEND_HASH_FOREACH_VAL(ids_ht, z_id) {
        if (Z_TYPE_P(z_id) != IS_STRING) {
            convert_to_string(z_id);
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_id);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_id);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    return arg_count;
}

/**
 * Prepare arguments for XREAD command.
 */
int prepare_x_read_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->streams || !args->ids) {
        return 0;
    }

    /* Get the number of streams and IDs */
    HashTable* streams_ht    = Z_ARRVAL_P(args->streams);
    HashTable* ids_ht        = Z_ARRVAL_P(args->ids);
    int        streams_count = zend_hash_num_elements(streams_ht);
    int        ids_count     = zend_hash_num_elements(ids_ht);

    /* Check counts match */
    if (streams_count <= 0 || streams_count != ids_count) {
        return 0;
    }

    /* Count options */
    unsigned long extra_args = 0;
    if (args->read_opts.has_count)
        extra_args += 2; /* COUNT + value */
    if (args->read_opts.has_block)
        extra_args += 2; /* BLOCK + value */
    if (args->read_opts.noack)
        extra_args += 1; /* NOACK */

    /* Calculate total args: options + STREAMS + streams + ids */
    unsigned long arg_count = extra_args + 1 + streams_count + ids_count;

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set arguments */
    unsigned int arg_idx = 0;

    /* Add COUNT if specified */
    if (args->read_opts.has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = sizeof("COUNT") - 1;
        arg_idx++;

        /* Convert count to string */
        char          count_str[32];
        unsigned long count_str_len =
            snprintf(count_str, sizeof(count_str), "%ld", args->read_opts.count);

        /* Allocate memory for the count string */
        char* count_str_copy = emalloc(count_str_len + 1);
        if (count_str_copy) {
            memcpy(count_str_copy, count_str, count_str_len);
            count_str_copy[count_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) count_str_copy;
            (*args_len_out)[arg_idx] = count_str_len;
            arg_idx++;
        }
    }

    /* Add BLOCK if specified */
    if (args->read_opts.has_block) {
        (*args_out)[arg_idx]     = (uintptr_t) "BLOCK";
        (*args_len_out)[arg_idx] = sizeof("BLOCK") - 1;
        arg_idx++;

        /* Convert block to string */
        char          block_str[32];
        unsigned long block_str_len =
            snprintf(block_str, sizeof(block_str), "%ld", args->read_opts.block);

        /* Allocate memory for the block string */
        char* block_str_copy = emalloc(block_str_len + 1);
        if (block_str_copy) {
            memcpy(block_str_copy, block_str, block_str_len);
            block_str_copy[block_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) block_str_copy;
            (*args_len_out)[arg_idx] = block_str_len;
            arg_idx++;
        }
    }

    /* Add NOACK if specified */
    if (args->read_opts.noack) {
        (*args_out)[arg_idx]     = (uintptr_t) "NOACK";
        (*args_len_out)[arg_idx] = sizeof("NOACK") - 1;
        arg_idx++;
    }

    /* Add STREAMS keyword */
    (*args_out)[arg_idx]     = (uintptr_t) "STREAMS";
    (*args_len_out)[arg_idx] = sizeof("STREAMS") - 1;
    arg_idx++;

    /* Add all stream keys */
    zval* z_stream;
    ZEND_HASH_FOREACH_VAL(streams_ht, z_stream) {
        if (Z_TYPE_P(z_stream) != IS_STRING) {
            convert_to_string(z_stream);
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_stream);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_stream);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add all stream IDs */
    zval* z_id;
    ZEND_HASH_FOREACH_VAL(ids_ht, z_id) {
        if (Z_TYPE_P(z_id) != IS_STRING) {
            convert_to_string(z_id);
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_id);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_id);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    return arg_count;
}

/**
 * Prepare arguments for XCLAIM command.
 */
int prepare_x_claim_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->group ||
        args->group_len <= 0 || !args->consumer || args->consumer_len <= 0 || !args->ids ||
        args->id_count <= 0) {
        return 0;
    }

    /* Count options */
    unsigned long extra_args = 0;
    if (args->claim_opts.has_idle)
        extra_args += 2; /* IDLE + value */
    if (args->claim_opts.has_time)
        extra_args += 2; /* TIME + value */
    if (args->claim_opts.has_retrycount)
        extra_args += 2; /* RETRYCOUNT + value */
    if (args->claim_opts.force)
        extra_args += 1; /* FORCE */
    if (args->claim_opts.justid)
        extra_args += 1; /* JUSTID */

    /* Calculate total args: key + group + consumer + min_idle_time + options + ids */
    unsigned long arg_count = 4 + extra_args + args->id_count;

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set key, group, consumer, min_idle_time */
    unsigned int arg_idx     = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->group;
    (*args_len_out)[arg_idx] = args->group_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->consumer;
    (*args_len_out)[arg_idx] = args->consumer_len;
    arg_idx++;

    /* Convert min_idle_time to string */
    char          min_idle_str[32];
    unsigned long min_idle_str_len =
        snprintf(min_idle_str, sizeof(min_idle_str), "%ld", args->min_idle_time);

    /* Allocate memory for the min_idle_time string */
    char* min_idle_str_copy = emalloc(min_idle_str_len + 1);
    if (min_idle_str_copy) {
        memcpy(min_idle_str_copy, min_idle_str, min_idle_str_len);
        min_idle_str_copy[min_idle_str_len] = '\0';

        (*args_out)[arg_idx]     = (uintptr_t) min_idle_str_copy;
        (*args_len_out)[arg_idx] = min_idle_str_len;
        arg_idx++;

        /* This string needs to be freed later */
        // TODO: Track this string for cleanup
    } else {
        /* Failed to allocate memory for min_idle_time */
        free_command_args(*args_out, *args_len_out);
        return 0;
    }

    /* Add all message IDs */
    zval* z_id;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args->ids), z_id) {
        if (Z_TYPE_P(z_id) != IS_STRING) {
            convert_to_string(z_id);
        }
        (*args_out)[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_id);
        (*args_len_out)[arg_idx] = Z_STRLEN_P(z_id);
        arg_idx++;
    }
    ZEND_HASH_FOREACH_END();

    /* Add options */
    if (args->claim_opts.has_idle) {
        (*args_out)[arg_idx]     = (uintptr_t) "IDLE";
        (*args_len_out)[arg_idx] = sizeof("IDLE") - 1;
        arg_idx++;

        /* Convert idle to string */
        char          idle_str[32];
        unsigned long idle_str_len =
            snprintf(idle_str, sizeof(idle_str), "%ld", args->claim_opts.idle);

        /* Allocate memory for the idle string */
        char* idle_str_copy = emalloc(idle_str_len + 1);
        if (idle_str_copy) {
            memcpy(idle_str_copy, idle_str, idle_str_len);
            idle_str_copy[idle_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) idle_str_copy;
            (*args_len_out)[arg_idx] = idle_str_len;
            arg_idx++;

            /* This string needs to be freed later */
            // TODO: Track this string for cleanup
        }
    }

    if (args->claim_opts.has_time) {
        (*args_out)[arg_idx]     = (uintptr_t) "TIME";
        (*args_len_out)[arg_idx] = sizeof("TIME") - 1;
        arg_idx++;

        /* Convert time to string */
        char          time_str[32];
        unsigned long time_str_len =
            snprintf(time_str, sizeof(time_str), "%ld", args->claim_opts.time);

        /* Allocate memory for the time string */
        char* time_str_copy = emalloc(time_str_len + 1);
        if (time_str_copy) {
            memcpy(time_str_copy, time_str, time_str_len);
            time_str_copy[time_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) time_str_copy;
            (*args_len_out)[arg_idx] = time_str_len;
            arg_idx++;

            /* This string needs to be freed later */
            // TODO: Track this string for cleanup
        }
    }

    if (args->claim_opts.has_retrycount) {
        (*args_out)[arg_idx]     = (uintptr_t) "RETRYCOUNT";
        (*args_len_out)[arg_idx] = sizeof("RETRYCOUNT") - 1;
        arg_idx++;

        /* Convert retrycount to string */
        char          retry_str[32];
        unsigned long retry_str_len =
            snprintf(retry_str, sizeof(retry_str), "%ld", args->claim_opts.retrycount);

        /* Allocate memory for the retry string */
        char* retry_str_copy = emalloc(retry_str_len + 1);
        if (retry_str_copy) {
            memcpy(retry_str_copy, retry_str, retry_str_len);
            retry_str_copy[retry_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) retry_str_copy;
            (*args_len_out)[arg_idx] = retry_str_len;
            arg_idx++;

            /* This string needs to be freed later */
            // TODO: Track this string for cleanup
        }
    }

    if (args->claim_opts.force) {
        (*args_out)[arg_idx]     = (uintptr_t) "FORCE";
        (*args_len_out)[arg_idx] = sizeof("FORCE") - 1;
        arg_idx++;
    }

    if (args->claim_opts.justid) {
        (*args_out)[arg_idx]     = (uintptr_t) "JUSTID";
        (*args_len_out)[arg_idx] = sizeof("JUSTID") - 1;
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare arguments for XAUTOCLAIM command.
 */
int prepare_x_autoclaim_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->group ||
        args->group_len <= 0 || !args->consumer || args->consumer_len <= 0 || !args->start ||
        args->start_len <= 0) {
        return 0;
    }

    /* Count options */
    unsigned long extra_args = 0;
    if (args->claim_opts.has_count)
        extra_args += 2; /* COUNT + value */
    if (args->claim_opts.justid)
        extra_args += 1; /* JUSTID */

    /* Calculate total args: key + group + consumer + min_idle_time + start + options */
    unsigned long arg_count = 5 + extra_args;

    /* Allocate memory for arguments */
    if (!allocate_command_args(arg_count, args_out, args_len_out)) {
        return 0;
    }

    /* Set key, group, consumer */
    unsigned int arg_idx     = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->group;
    (*args_len_out)[arg_idx] = args->group_len;
    arg_idx++;

    (*args_out)[arg_idx]     = (uintptr_t) args->consumer;
    (*args_len_out)[arg_idx] = args->consumer_len;
    arg_idx++;

    /* Convert min_idle_time to string */
    char          min_idle_str[32];
    unsigned long min_idle_str_len =
        snprintf(min_idle_str, sizeof(min_idle_str), "%ld", args->min_idle_time);

    /* Allocate memory for the min_idle_time string */
    char* min_idle_str_copy = emalloc(min_idle_str_len + 1);
    if (min_idle_str_copy) {
        memcpy(min_idle_str_copy, min_idle_str, min_idle_str_len);
        min_idle_str_copy[min_idle_str_len] = '\0';

        (*args_out)[arg_idx]     = (uintptr_t) min_idle_str_copy;
        (*args_len_out)[arg_idx] = min_idle_str_len;
        arg_idx++;

        /* This string needs to be freed later */
        // TODO: Track this string for cleanup
    } else {
        /* Failed to allocate memory for min_idle_time */
        free_command_args(*args_out, *args_len_out);
        return 0;
    }

    /* Add start ID */
    (*args_out)[arg_idx]     = (uintptr_t) args->start;
    (*args_len_out)[arg_idx] = args->start_len;
    arg_idx++;

    /* Add COUNT if specified */
    if (args->claim_opts.has_count) {
        (*args_out)[arg_idx]     = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx] = sizeof("COUNT") - 1;
        arg_idx++;

        /* Convert count to string */
        char          count_str[32];
        unsigned long count_str_len =
            snprintf(count_str, sizeof(count_str), "%ld", args->claim_opts.count);

        /* Allocate memory for the count string */
        char* count_str_copy = emalloc(count_str_len + 1);
        if (count_str_copy) {
            memcpy(count_str_copy, count_str, count_str_len);
            count_str_copy[count_str_len] = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) count_str_copy;
            (*args_len_out)[arg_idx] = count_str_len;
            arg_idx++;

            /* This string needs to be freed later */
            // TODO: Track this string for cleanup
        }
    }

    /* Add JUSTID if specified */
    if (args->claim_opts.justid) {
        (*args_out)[arg_idx]     = (uintptr_t) "JUSTID";
        (*args_len_out)[arg_idx] = sizeof("JUSTID") - 1;
        arg_idx++;
    }

    return arg_count;
}

/**
 * Prepare arguments for XTRIM command.
 */
int prepare_x_trim_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count) {
    /* Check if client and arguments are valid */
    if (!args->glide_client || !args->key || args->key_len <= 0 || !args->strategy ||
        args->strategy_len <= 0 || !args->threshold || args->threshold_len <= 0) {
        return 0;
    }

    /* Calculate total args: key + strategy + [~] + threshold + [LIMIT + value] */
    unsigned long arg_count =
        1 + 1 + (args->trim_opts.approximate ? 1 : 0) + 1 + (args->trim_opts.has_limit ? 2 : 0);
    *args_out     = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!*args_out || !*args_len_out) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Set key as first argument */
    unsigned int arg_idx     = 0;
    (*args_out)[arg_idx]     = (uintptr_t) args->key;
    (*args_len_out)[arg_idx] = args->key_len;
    arg_idx++;

    /* Add strategy */
    (*args_out)[arg_idx]     = (uintptr_t) args->strategy;
    (*args_len_out)[arg_idx] = args->strategy_len;
    arg_idx++;

    /* Add ~ for approximate trimming */
    if (args->trim_opts.approximate) {
        (*args_out)[arg_idx]     = (uintptr_t) "~";
        (*args_len_out)[arg_idx] = 1;
        arg_idx++;
    }

    /* Add threshold value */
    (*args_out)[arg_idx]     = (uintptr_t) args->threshold;
    (*args_len_out)[arg_idx] = args->threshold_len;
    arg_idx++;

    /* Add LIMIT if specified */
    if (args->trim_opts.has_limit) {
        (*args_out)[arg_idx]     = (uintptr_t) "LIMIT";
        (*args_len_out)[arg_idx] = sizeof("LIMIT") - 1;
        arg_idx++;

        /* Convert limit to string */
        char          limit_str[32];
        unsigned long limit_str_len =
            snprintf(limit_str, sizeof(limit_str), "%ld", args->trim_opts.limit);

        /* Need to allocate memory for the string */
        char* limit_str_copy = emalloc(limit_str_len + 1);
        if (limit_str_copy) {
            memcpy(limit_str_copy, limit_str, limit_str_len);
            *allocated_strings                   = (char**) ecalloc(1, sizeof(char*));
            *allocated_count                     = 0;
            *allocated_strings[*allocated_count] = limit_str_copy;
            limit_str_copy[limit_str_len]        = '\0';

            (*args_out)[arg_idx]     = (uintptr_t) limit_str_copy;
            (*args_len_out)[arg_idx] = limit_str_len;
            arg_idx++;
        }
    }

    return arg_count;
}
