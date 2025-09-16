/*
  +----------------------------------------------------------------------+
  | Valkey Glide X-Commands Implementation                               |
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
#include "valkey_glide_commands_common.h"
#include "valkey_glide_x_common.h"

/* ====================================================================
 * COMMAND IMPLEMENTATION FUNCTIONS
 * ==================================================================== */

/**
 * Execute an XLEN command
 */
int execute_xlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key     = NULL;
    size_t               key_len = 0;


    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;

        /* Use the generic command execution framework */
        int result = execute_x_generic_command(
            valkey_glide, XLen, &args, NULL, process_x_int_result, return_value);

        if (result && valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            /* Note: out will be freed later in process_core_string_result */
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XDEL command
 */
int execute_xdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key     = NULL;
    size_t               key_len = 0;
    zval*                z_ids;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osa", &object, ce, &key, &key_len, &z_ids) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.ids              = z_ids;
        args.id_count         = zend_hash_num_elements(Z_ARRVAL_P(z_ids));

        /* Use the generic command execution framework */
        int result = execute_x_generic_command(
            valkey_glide, XDel, &args, NULL, process_x_int_result, return_value);

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XACK command
 */
int execute_xack_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *group = NULL;
    size_t               key_len = 0, group_len = 0;
    zval*                z_ids;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Ossa", &object, ce, &key, &key_len, &group, &group_len, &z_ids) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.group            = group;
        args.group_len        = group_len;
        args.ids              = z_ids;
        args.id_count         = zend_hash_num_elements(Z_ARRVAL_P(z_ids));


        /* Use the generic command execution framework */
        int result = execute_x_generic_command(
            valkey_glide, XAck, &args, NULL, process_x_int_result, return_value);

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XADD command
 */
int execute_xadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *id = NULL;
    size_t               key_len = 0, id_len        = 0;
    zval *               z_field_values, *z_options = NULL;
    zend_long            maxlen          = 0;
    zend_bool            approximate     = 0;
    int                  options_created = 0;

    /* First, try parsing as (key, id, fields, maxlen, approximate) */
    if (argc >= 4 && argc <= 5) {
        zend_bool parse_success = 0;

        if (argc == 4) {
            if (zend_parse_method_parameters(argc,
                                             object,
                                             "Ossal",
                                             &object,
                                             ce,
                                             &key,
                                             &key_len,
                                             &id,
                                             &id_len,
                                             &z_field_values,
                                             &maxlen) == SUCCESS) {
                parse_success = 1;
            }
        } else if (argc == 5) {
            if (zend_parse_method_parameters(argc,
                                             object,
                                             "Ossalb",
                                             &object,
                                             ce,
                                             &key,
                                             &key_len,
                                             &id,
                                             &id_len,
                                             &z_field_values,
                                             &maxlen,
                                             &approximate) == SUCCESS) {
                parse_success = 1;
            }
        }

        if (parse_success) {
            /* Create options array with MAXLEN */
            z_options = emalloc(sizeof(zval));
            array_init(z_options);

            /* Add MAXLEN option */
            add_assoc_long(z_options, "MAXLEN", maxlen);

            /* Add APPROXIMATE option if true */
            if (approximate) {
                add_assoc_bool(z_options, "APPROXIMATE", 1);
            }

            /* Flag that we created this and will need to free it later */
            options_created = 1;
        }
    }

    /* If above parsing failed or was not attempted, try the standard way */
    if (!z_options) {
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossa|a",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &id,
                                         &id_len,
                                         &z_field_values,
                                         &z_options) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.id               = id;
        args.id_len           = id_len;
        args.field_values     = z_field_values;
        args.fv_count         = zend_hash_num_elements(Z_ARRVAL_P(z_field_values));
        args.options          = z_options;

        /* Parse options */
        parse_x_add_options(z_options, &args.add_opts);

        /* Execute the command */
        int result = execute_x_generic_command(
            valkey_glide, XAdd, &args, NULL, process_x_add_result, return_value);

        /* Clean up if we created options array */
        if (options_created) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XTRIM command
 */
int execute_xtrim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *threshold = NULL;
    size_t               key_len = 0, threshold_len = 0;
    zend_bool            approx = 0, minid = 0;
    zend_long            limit     = -1;
    zval*                z_options = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss|bbl",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &threshold,
                                     &threshold_len,
                                     &approx,
                                     &minid,
                                     &limit) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Determine strategy based on minid flag */
        const char* strategy     = minid ? "MINID" : "MAXLEN";
        size_t      strategy_len = minid ? sizeof("MINID") - 1 : sizeof("MAXLEN") - 1;

        /* Create options array if we have approx or limit */
        if (approx || limit >= 0) {
            z_options = emalloc(sizeof(zval));
            array_init(z_options);

            if (approx) {
                add_assoc_bool(z_options, "APPROXIMATE", 1);
            }

            if (limit >= 0) {
                add_assoc_long(z_options, "LIMIT", limit);
            }
        }

        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.strategy         = strategy;
        args.strategy_len     = strategy_len;
        args.threshold        = threshold;
        args.threshold_len    = threshold_len;
        args.options          = z_options;

        /* Parse options */
        parse_x_trim_options(z_options, &args.trim_opts);

        /* Execute the command */
        int result = execute_x_generic_command(
            valkey_glide, XTrim, &args, NULL, process_x_int_result, return_value);

        /* Clean up if we created options array */
        if (z_options) {
            zval_dtor(z_options);
            efree(z_options);
        }
        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XRANGE command
 */
int execute_xrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *start = NULL, *end = NULL;
    size_t               key_len = 0, start_len = 0, end_len = 0;
    zval*                z_options       = NULL;
    long                 count           = 0;
    int                  options_created = 0;

    /* Parse parameters - try different combinations based on argument count */
    if (argc == 4) {
        /* xrange(key, start, end, count) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osssl",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &start,
                                         &start_len,
                                         &end,
                                         &end_len,
                                         &count) == FAILURE) {
            return 0;
        }

        /* Create options array with COUNT */
        z_options = emalloc(sizeof(zval));
        array_init(z_options);
        add_assoc_long(z_options, "COUNT", count);
        options_created = 1;
    } else if (argc == 5) {
        /* xrange(key, start, end, count, options) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossla",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &start,
                                         &start_len,
                                         &end,
                                         &end_len,
                                         &count,
                                         &z_options) == FAILURE) {
            return 0;
        }

        /* Add COUNT to existing options array or create new one */
        if (z_options && Z_TYPE_P(z_options) == IS_ARRAY) {
            add_assoc_long(z_options, "COUNT", count);
        } else {
            z_options = emalloc(sizeof(zval));
            array_init(z_options);
            add_assoc_long(z_options, "COUNT", count);
            options_created = 1;
        }
    } else {
        /* xrange(key, start, end [, options]) - original format for backward compatibility */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osss|a",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &start,
                                         &start_len,
                                         &end,
                                         &end_len,
                                         &z_options) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.start            = start;
        args.start_len        = start_len;
        args.end              = end;
        args.end_len          = end_len;
        args.options          = z_options;

        /* Parse options */
        parse_x_count_options(z_options, &args.range_opts);

        /* Use the generic command execution framework */
        int result = execute_x_generic_command(
            valkey_glide, XRange, &args, NULL, process_x_stream_result, return_value);

        /* Clean up if we created options array */
        if (options_created) {
            zval_dtor(z_options);
            efree(z_options);
        }
        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/**
 * Execute an XREVRANGE command
 */
int execute_xrevrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *start = NULL, *end = NULL;
    size_t               key_len = 0, start_len = 0, end_len = 0;
    zval*                z_options       = NULL;
    long                 count           = 0;
    int                  options_created = 0;

    /* Parse parameters - try different combinations based on argument count */
    if (argc == 4) {
        /* xrevrange(key, end, start, count) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osssl",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &end,
                                         &end_len,
                                         &start,
                                         &start_len,
                                         &count) == FAILURE) {
            return 0;
        }

        /* Create options array with COUNT */
        z_options = emalloc(sizeof(zval));
        array_init(z_options);
        add_assoc_long(z_options, "COUNT", count);
        options_created = 1;
    } else if (argc == 5) {
        /* xrevrange(key, end, start, count, options) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossla",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &end,
                                         &end_len,
                                         &start,
                                         &start_len,
                                         &count,
                                         &z_options) == FAILURE) {
            return 0;
        }

        /* Add COUNT to existing options array or create new one */
        if (z_options && Z_TYPE_P(z_options) == IS_ARRAY) {
            add_assoc_long(z_options, "COUNT", count);
        } else {
            z_options = emalloc(sizeof(zval));
            array_init(z_options);
            add_assoc_long(z_options, "COUNT", count);
            options_created = 1;
        }
    } else {
        /* xrevrange(key, end, start [, options]) - original format for backward compatibility */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osss|a",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &end,
                                         &end_len,
                                         &start,
                                         &start_len,
                                         &z_options) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        /* Note: For XREVRANGE, the function parameters are already swapped (end comes before start)
         * But in the command arguments, we need to preserve the order expected by
         * prepare_x_range_args So we assign them in the correct mapping for command construction */
        args.start     = end; /* end is actually the 'start' argument for XREVRANGE */
        args.start_len = end_len;
        args.end       = start; /* start is actually the 'end' argument for XREVRANGE */
        args.end_len   = start_len;
        args.options   = z_options;

        /* Parse options */
        parse_x_count_options(z_options, &args.range_opts);

        /* Use the generic command execution framework */
        int result = execute_x_generic_command(
            valkey_glide, XRevRange, &args, NULL, process_x_stream_result, return_value);

        /* Clean up if we created options array */
        if (options_created) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}
/* Execute an XPENDING command using the Valkey Glide client */
int execute_xpending_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_options = NULL;
    char *               key = NULL, *group = NULL;
    char *               start = NULL, *end = NULL, *consumer = NULL;
    size_t               key_len = 0, group_len = 0;
    size_t               start_len = 0, end_len = 0, consumer_len = 0;
    zend_long            count           = 0;
    zend_bool            options_created = 0;

    /* Handle different parameter formats based on argument count */
    if (argc == 3 || argc == 2) {
        /* Format: xpending(key, group, options_array) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Oss|a",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &group,
                                         &group_len,
                                         &z_options) == FAILURE) {
            return 0;
        }
    } else if (argc == 5) {
        /* Format: xpending(key, group, start, end, count) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osssl",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &group,
                                         &group_len,
                                         &start,
                                         &start_len,
                                         &end,
                                         &end_len,
                                         &count) == FAILURE) {
            return 0;
        }
    } else if (argc == 6) {
        /* Format: xpending(key, group, start, end, count, consumer) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossssls",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &group,
                                         &group_len,
                                         &start,
                                         &start_len,
                                         &end,
                                         &end_len,
                                         &count,
                                         &consumer,
                                         &consumer_len) == FAILURE) {
            return 0;
        }
    } else {
        /* Invalid number of arguments */
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* If we got the extended format (start, end, count), convert to options array */
        if (z_options == NULL && start != NULL) {
            options_created = 1;
            z_options       = emalloc(sizeof(zval));
            array_init(z_options);

            /* Add START, END to options array */
            add_assoc_stringl(z_options, "START", start, start_len);
            add_assoc_stringl(z_options, "END", end, end_len);
            add_assoc_long(z_options, "COUNT", count);

            /* Add CONSUMER to options array if provided */
            if (consumer) {
                add_assoc_stringl(z_options, "CONSUMER", consumer, consumer_len);
            }
        }

        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.group            = group;
        args.group_len        = group_len;
        args.options          = z_options;

        /* Parse options for XPENDING command */
        parse_x_pending_options(z_options, &args.pending_opts);

        /* Execute the command */
        int result = execute_x_generic_command(
            valkey_glide, XPending, &args, NULL, process_x_pending_result, return_value);

        /* Clean up if we created options array */
        if (options_created && z_options) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/* Execute an XREAD command using the Valkey Glide client */
int execute_xread_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval *               z_streams_and_ids, *z_options = NULL;
    long                 count = -1, block = -1;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oa|ll", &object, ce, &z_streams_and_ids, &count, &block) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Convert associative array to separate streams and ids arrays */
        zval z_streams, z_ids;
        array_init(&z_streams);
        array_init(&z_ids);

        zend_string* stream_key;
        zval*        stream_id;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_streams_and_ids), stream_key, stream_id) {
            if (stream_key) {
                add_next_index_str(&z_streams, zend_string_copy(stream_key));
                if (Z_TYPE_P(stream_id) != IS_STRING) {
                    convert_to_string(stream_id);
                }
                add_next_index_str(&z_ids, zend_string_copy(Z_STR_P(stream_id)));
            }
        }
        ZEND_HASH_FOREACH_END();

        /* Create options array if count or block were specified */
        if (count >= 0 || block >= 0) {
            z_options = emalloc(sizeof(zval));
            array_init(z_options);

            if (count >= 0) {
                add_assoc_long(z_options, "COUNT", count);
            }
            if (block >= 0) {
                add_assoc_long(z_options, "BLOCK", block);
            }
        }

        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.streams          = &z_streams;
        args.ids              = &z_ids;
        args.options          = z_options;

        /* Parse options for XREAD command */
        parse_x_read_options(z_options, &args.read_opts);

        /* Execute the command */
        int result = execute_x_generic_command(
            valkey_glide, XRead, &args, NULL, process_x_stream_result, return_value);

        /* Clean up */
        zval_dtor(&z_streams);
        zval_dtor(&z_ids);
        if (z_options) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/* Execute an XREADGROUP command using the Valkey Glide client */
int execute_xreadgroup_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               group = NULL, *consumer = NULL;
    size_t               group_len = 0, consumer_len   = 0;
    zval *               z_streams_and_ids, *z_options = NULL;
    long                 count           = -1;
    int                  options_created = 0;

    /* Parse parameters - handle multiple calling patterns */
    if (argc == 4) {
        /* Try parsing as (group, consumer, streams, count) first */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossal",
                                         &object,
                                         ce,
                                         &group,
                                         &group_len,
                                         &consumer,
                                         &consumer_len,
                                         &z_streams_and_ids,
                                         &count) == SUCCESS) {
            /* Create options array with COUNT */
            z_options = emalloc(sizeof(zval));
            array_init(z_options);
            add_assoc_long(z_options, "COUNT", count);
            options_created = 1;
        } else {
            /* Try parsing as (group, consumer, streams, options) */
            if (zend_parse_method_parameters(argc,
                                             object,
                                             "Ossa",
                                             &object,
                                             ce,
                                             &group,
                                             &group_len,
                                             &consumer,
                                             &consumer_len,
                                             &z_streams_and_ids,
                                             &z_options) == FAILURE) {
                return 0;
            }
        }
    } else if (argc == 5) {
        long block = -1;

        /* First try parsing as (group, consumer, streams, count, block) */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossall",
                                         &object,
                                         ce,
                                         &group,
                                         &group_len,
                                         &consumer,
                                         &consumer_len,
                                         &z_streams_and_ids,
                                         &count,
                                         &block) == SUCCESS) {
            /* Create options array with both COUNT and BLOCK */
            z_options = emalloc(sizeof(zval));
            array_init(z_options);
            add_assoc_long(z_options, "COUNT", count);
            add_assoc_long(z_options, "BLOCK", block);
            options_created = 1;
        } else {
            /* Fallback to parsing as (group, consumer, streams, count, options) */
            if (zend_parse_method_parameters(argc,
                                             object,
                                             "Ossala",
                                             &object,
                                             ce,
                                             &group,
                                             &group_len,
                                             &consumer,
                                             &consumer_len,
                                             &z_streams_and_ids,
                                             &count,
                                             &z_options) == FAILURE) {
                return 0;
            }

            /* Add COUNT to existing options array or create new one */
            if (z_options && Z_TYPE_P(z_options) == IS_ARRAY) {
                add_assoc_long(z_options, "COUNT", count);
            } else {
                z_options = emalloc(sizeof(zval));
                array_init(z_options);
                add_assoc_long(z_options, "COUNT", count);
                options_created = 1;
            }
        }
    } else {
        /* Parse as (group, consumer, streams [, options]) - original format for backward
         * compatibility */
        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Ossa|a",
                                         &object,
                                         ce,
                                         &group,
                                         &group_len,
                                         &consumer,
                                         &consumer_len,
                                         &z_streams_and_ids,
                                         &z_options) == FAILURE) {
            return 0;
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* For the combined format, we need to separate streams and IDs */
        zval z_streams, z_ids;
        array_init(&z_streams);
        array_init(&z_ids);

        /* Extract streams and IDs from the combined array */
        zend_string* stream_key;
        zval*        stream_id;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_streams_and_ids), stream_key, stream_id) {
            if (stream_key) {
                add_next_index_str(&z_streams, zend_string_copy(stream_key));
                if (Z_TYPE_P(stream_id) != IS_STRING) {
                    convert_to_string(stream_id);
                }
                add_next_index_str(&z_ids, zend_string_copy(Z_STR_P(stream_id)));
            }
        }
        ZEND_HASH_FOREACH_END();

        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.group            = group;
        args.group_len        = group_len;
        args.consumer         = consumer;
        args.consumer_len     = consumer_len;
        args.streams          = &z_streams;
        args.ids              = &z_ids;
        args.options          = z_options;

        /* Parse options for XREADGROUP command */
        parse_x_read_options(z_options, &args.read_opts);

        /* Execute the command with the generic framework */
        int result = execute_x_generic_command(
            valkey_glide, XReadGroup, &args, NULL, process_x_readgroup_result, return_value);

        /* Clean up temporary arrays */
        zval_dtor(&z_streams);
        zval_dtor(&z_ids);

        /* Clean up if we created options array */
        if (options_created) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }

        return result;
    }

    return 0;
}

/* Execute an XCLAIM command using the Valkey Glide client */
int execute_xclaim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *group = NULL, *consumer = NULL;
    size_t               key_len = 0, group_len = 0, consumer_len = 0;
    long                 min_idle_time = 0;
    zval *               z_ids = NULL, *z_options = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osssla|a",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &group,
                                     &group_len,
                                     &consumer,
                                     &consumer_len,
                                     &min_idle_time,
                                     &z_ids,
                                     &z_options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.group            = group;
        args.group_len        = group_len;
        args.consumer         = consumer;
        args.consumer_len     = consumer_len;
        args.min_idle_time    = min_idle_time;
        args.ids              = z_ids; /* Array of IDs to claim */
        args.id_count         = zend_hash_num_elements(Z_ARRVAL_P(z_ids));
        args.options          = z_options;

        /* Parse options for XCLAIM command */
        parse_x_claim_options(z_options, &args.claim_opts);

        x_claim_result_context_t* result_context = emalloc(sizeof(x_claim_result_context_t));
        memset(result_context, 0, sizeof(x_claim_result_context_t));
        result_context->justid = args.claim_opts.justid;

        /* Use the generic command execution framework */
        int res = execute_x_generic_command(
            valkey_glide, XClaim, &args, result_context, process_x_claim_result, return_value);
        if (res && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }
        return res;
    }

    return 0;
}

/* Execute an XAUTOCLAIM command using the Valkey Glide client */
int execute_xautoclaim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *group = NULL, *consumer = NULL, *start = NULL;
    size_t               key_len = 0, group_len = 0, consumer_len = 0, start_len = 0;
    long                 min_idle_time   = 0;
    long                 count           = -1;
    zend_bool            justid          = 0;
    zval*                z_options       = NULL;
    int                  options_created = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osssls|lb",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &group,
                                     &group_len,
                                     &consumer,
                                     &consumer_len,
                                     &min_idle_time,
                                     &start,
                                     &start_len,
                                     &count,
                                     &justid) == FAILURE) {
        return 0;
    }

    /* Create options array if count or justid are specified */
    if (count != -1 || justid) {
        z_options = emalloc(sizeof(zval));
        array_init(z_options);
        options_created = 1;

        if (count != -1) {
            add_assoc_long(z_options, "COUNT", count);
        }

        if (justid) {
            add_assoc_bool(z_options, "JUSTID", 1);
        }
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args = {0};
        args.glide_client     = valkey_glide->glide_client;
        args.key              = key;
        args.key_len          = key_len;
        args.group            = group;
        args.group_len        = group_len;
        args.consumer         = consumer;
        args.consumer_len     = consumer_len;
        args.min_idle_time    = min_idle_time;
        args.start            = start;
        args.start_len        = start_len;
        args.options          = z_options;

        /* Parse options for XAUTOCLAIM command */
        parse_x_claim_options(z_options, &args.claim_opts);

        /* Check if COUNT is specified (not in standard claim_opts) */
        if (z_options && Z_TYPE_P(z_options) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(z_options);
            zval*      z_count;

            /* Check for COUNT option */
            if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
                if (Z_TYPE_P(z_count) == IS_LONG) {
                    args.claim_opts.count     = Z_LVAL_P(z_count);
                    args.claim_opts.has_count = 1;
                }
            }
        }

        /* Use the generic command execution framework */
        int res = execute_x_generic_command(
            valkey_glide, XAutoClaim, &args, NULL, process_x_autoclaim_result, return_value);

        /* Clean up if we created options array */
        if (options_created) {
            zval_dtor(z_options);
            efree(z_options);
        }

        if (res && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }
        return res;
    }

    return 0;
}

/**
 * Execute an XINFO command
 */
int execute_xinfo_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                op     = NULL;
    size_t               op_len = 0;
    zval*                z_args;
    int                  args_count;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &op, &op_len, &z_args, &args_count) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Initialize the arguments structure */
        x_command_args_t args_struct = {0};
        args_struct.glide_client     = valkey_glide->glide_client;
        args_struct.subcommand       = op;
        args_struct.subcommand_len   = op_len;
        args_struct.args             = z_args;
        args_struct.args_count       = args_count;

        /* Parse the XINFO command and extract arguments */
        enum RequestType command_type;
        if (strcasecmp(op, "CONSUMERS") == 0) {
            command_type = XInfoConsumers;
        } else if (strcasecmp(op, "GROUPS") == 0) {
            command_type = XInfoGroups;
        } else if (strcasecmp(op, "STREAM") == 0) {
            command_type = XInfoStream;
        } else {
            /* Unsupported subcommand */
            return 0;
        }

        /* Execute the command */
        int res = execute_x_generic_command(
            valkey_glide, command_type, &args_struct, NULL, process_x_info_result, return_value);
        if (res && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }
        return res;
    }

    return 0;
}

/**
 * Execute an XGROUP command
 */
int execute_xgroup_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                op         = NULL;
    size_t               op_len     = 0;
    zval*                z_args     = NULL;
    int                  args_count = 0;
    int                  result     = 0;

    /* Parse using flexible parameter parsing to match PHP signature */
    char *    key = NULL, *group = NULL, *id_or_consumer = NULL;
    size_t    key_len = 0, group_len = 0, id_or_consumer_len = 0;
    zend_bool mkstream     = 0;
    zend_long entries_read = -2;

    /* Parse method parameters with defaults matching PHP signature */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Os|s!s!s!bl",
                                     &object,
                                     ce,
                                     &op,
                                     &op_len,
                                     &key,
                                     &key_len,
                                     &group,
                                     &group_len,
                                     &id_or_consumer,
                                     &id_or_consumer_len,
                                     &mkstream,
                                     &entries_read) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Determine the command type based on the operation */
        enum RequestType command_type;

        /* Initialize the arguments structure */
        x_command_args_t args_struct = {0};
        args_struct.glide_client     = valkey_glide->glide_client;
        args_struct.subcommand       = op;
        args_struct.subcommand_len   = op_len;

        /* We need to handle parameters differently based on operation */
        if (strcasecmp(op, "CREATE") == 0) {
            /* Validate required parameters for CREATE */
            if (!key || !group || !id_or_consumer) {
                return 0;
            }

            /* Allocate memory for arguments */
            int max_args = 3 + (mkstream ? 1 : 0) + (entries_read != -2 ? 2 : 0);
            z_args       = emalloc(max_args * sizeof(zval));

            if (!z_args) {
                return 0;
            }

            /* Add key, group, id */
            ZVAL_STRINGL(&z_args[args_count], key, key_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], group, group_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], id_or_consumer, id_or_consumer_len);
            args_count++;

            /* Add MKSTREAM if specified */
            if (mkstream) {
                ZVAL_STRING(&z_args[args_count], "MKSTREAM");
                args_count++;
            }

            /* Add ENTRIESREAD and value if specified */
            if (entries_read != -2) {
                ZVAL_STRING(&z_args[args_count], "ENTRIESREAD");
                args_count++;

                ZVAL_LONG(&z_args[args_count], entries_read);
                args_count++;
            }

            command_type = XGroupCreate;
        } else if (strcasecmp(op, "SETID") == 0) {
            /* Validate required parameters for SETID */
            if (!key || !group || !id_or_consumer) {
                return 0;
            }

            /* Allocate memory for arguments */
            int max_args = 3 + (entries_read != -2 ? 2 : 0);
            z_args       = emalloc(max_args * sizeof(zval));

            if (!z_args) {
                return 0;
            }

            /* Add key, group, id */
            ZVAL_STRINGL(&z_args[args_count], key, key_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], group, group_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], id_or_consumer, id_or_consumer_len);
            args_count++;

            /* Add ENTRIESREAD and value if specified */
            if (entries_read != -2) {
                ZVAL_STRING(&z_args[args_count], "ENTRIESREAD");
                args_count++;

                ZVAL_LONG(&z_args[args_count], entries_read);
                args_count++;
            }

            command_type = XGroupSetId;
        } else if (strcasecmp(op, "DESTROY") == 0) {
            /* Validate required parameters for DESTROY */
            if (!key || !group) {
                return 0;
            }

            /* Allocate memory for arguments */
            z_args = emalloc(2 * sizeof(zval));

            if (!z_args) {
                return 0;
            }

            /* Add key, group */
            ZVAL_STRINGL(&z_args[args_count], key, key_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], group, group_len);
            args_count++;

            command_type = XGroupDestroy;
        } else if (strcasecmp(op, "CREATECONSUMER") == 0) {
            /* Validate required parameters for CREATECONSUMER */
            if (!key || !group || !id_or_consumer) {
                return 0;
            }
            if (argc > 4) {
                return 0;  // Invalid number of arguments for CREATECONSUMER
            }

            /* Allocate memory for arguments */
            z_args = emalloc(3 * sizeof(zval));

            if (!z_args) {
                return 0;
            }

            /* Add key, group, consumer */
            ZVAL_STRINGL(&z_args[args_count], key, key_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], group, group_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], id_or_consumer, id_or_consumer_len);
            args_count++;

            command_type = XGroupCreateConsumer;
        } else if (strcasecmp(op, "DELCONSUMER") == 0) {
            /* Validate required parameters for DELCONSUMER */
            if (!key || !group || !id_or_consumer) {
                return 0;
            }

            /* Allocate memory for arguments */
            z_args = emalloc(3 * sizeof(zval));

            if (!z_args) {
                return 0;
            }

            /* Add key, group, consumer */
            ZVAL_STRINGL(&z_args[args_count], key, key_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], group, group_len);
            args_count++;

            ZVAL_STRINGL(&z_args[args_count], id_or_consumer, id_or_consumer_len);
            args_count++;

            command_type = XGroupDelConsumer;
        } else {
            /* Unsupported subcommand */
            return 0;
        }

        /* Set the args in the args_struct */
        args_struct.args       = z_args;
        args_struct.args_count = args_count;

        /* Execute the command */
        result = execute_x_generic_command(
            valkey_glide, command_type, &args_struct, NULL, process_x_group_result, return_value);

        /* Clean up */
        for (int i = 0; i < args_count; i++) {
            zval_dtor(&z_args[i]);
        }
        if (z_args) {
            efree(z_args);
        }
        if (result && valkey_glide->is_in_batch_mode) {
            ZVAL_COPY(return_value, object);
        }


        return result;
    }

    return 0;
}
