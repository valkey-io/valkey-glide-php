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
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* Execute a TYPE command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_type_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key        = NULL;
    size_t               key_len    = 0;
    long                 type_value = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Type;
        args.key                 = key;
        args.key_len             = key_len;

        if (execute_core_command(
                valkey_glide, &args, &type_value, process_core_type_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            ZVAL_LONG(return_value, type_value);
            return 1;
        }
    }

    return 0;
}

/* Execute an APPEND command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_append_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *value = NULL;
    size_t               key_len = 0, value_len = 0;
    long                 result_value = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &value, &value_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Append;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add value argument */
        args.args[0].type                  = CORE_ARG_TYPE_STRING;
        args.args[0].data.string_arg.value = value;
        args.args[0].data.string_arg.len   = value_len;
        args.arg_count                     = 1;

        if (execute_core_command(
                valkey_glide, &args, &result_value, process_core_int_result_batch, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            ZVAL_LONG(return_value, result_value);
            return 1;
        }
    }

    return 0;
}

/* Execute a GETRANGE command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_getrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *result = NULL;
    size_t               key_len = 0, result_len = 0;
    long                 start = 0, end = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osll", &object, ce, &key, &key_len, &start, &end) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = GetRange;
        args.key                 = key;
        args.key_len             = key_len;

        /* Add start and end arguments */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = start;
        args.args[1].type                = CORE_ARG_TYPE_LONG;
        args.args[1].data.long_arg.value = end;
        args.arg_count                   = 2;

        /* Use string result processor */
        struct {
            char**  result;
            size_t* result_len;
        } output = {&result, &result_len};

        int ret = execute_core_command(
            valkey_glide, &args, &output, process_core_string_result_batch, return_value);

        if (ret > 0) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            /* Command succeeded with data */
            RETVAL_STRINGL(result, result_len);
            efree(result);
            return 1;
        } else if (ret == 0) {
            /* Key didn't exist, return empty string */
            ZVAL_EMPTY_STRING(return_value);
            return 1;
        }
    }

    return 0;
}

/* Helper function to build SORT command arguments */
static void build_sort_args(const char*     key,
                            size_t          key_len,
                            zval*           sort_pattern,
                            zend_bool*      alpha_out,
                            zend_bool*      desc_out,
                            uintptr_t**     args_ptr,
                            unsigned long** args_len_ptr,
                            unsigned long*  arg_count_ptr) {
    zend_bool alpha = 0, desc = 0, explicit_asc = 0;

    /* Parse sort options from the pattern array first */
    if (sort_pattern && Z_TYPE_P(sort_pattern) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(sort_pattern);
        zval*      z_ele;

        /* Check for SORT option (case-insensitive) */
        if ((z_ele = zend_hash_str_find(ht, "sort", sizeof("sort") - 1)) != NULL ||
            (z_ele = zend_hash_str_find(ht, "SORT", sizeof("SORT") - 1)) != NULL) {
            if (Z_TYPE_P(z_ele) == IS_STRING) {
                const char* sort_val = Z_STRVAL_P(z_ele);
                if (strcasecmp(sort_val, "desc") == 0 || strcasecmp(sort_val, "DESC") == 0) {
                    desc = 1;
                } else if (strcasecmp(sort_val, "asc") == 0 || strcasecmp(sort_val, "ASC") == 0) {
                    explicit_asc = 1; /* Explicitly ASC */
                    desc         = 0;
                }
            }
        }

        /* Check for ALPHA option (case-insensitive) */
        if ((z_ele = zend_hash_str_find(ht, "alpha", sizeof("alpha") - 1)) != NULL ||
            (z_ele = zend_hash_str_find(ht, "ALPHA", sizeof("ALPHA") - 1)) != NULL) {
            if (Z_TYPE_P(z_ele) == IS_TRUE ||
                (Z_TYPE_P(z_ele) == IS_LONG && Z_LVAL_P(z_ele) != 0) ||
                (Z_TYPE_P(z_ele) == IS_STRING && strcasecmp(Z_STRVAL_P(z_ele), "true") == 0)) {
                alpha = 1;
            }
        }
    }

    /* Set output flags */
    if (alpha_out)
        *alpha_out = alpha;
    if (desc_out)
        *desc_out = desc;

    /* Calculate the maximum number of arguments */
    unsigned long max_args = 1; /* key */
    if (sort_pattern && Z_TYPE_P(sort_pattern) == IS_ARRAY) {
        /* Patterns array can have: BY, LIMIT, GET, STORE */
        HashTable* ht = Z_ARRVAL_P(sort_pattern);
        max_args +=
            2 * zend_hash_num_elements(ht) + 2; /* Extra space for possible LIMIT offset count */
    }
    if (alpha)
        max_args++; /* ALPHA */
    if (desc)
        max_args++; /* DESC */

    /* Allocate arrays for arguments */
    uintptr_t*     args     = (uintptr_t*) emalloc(max_args * sizeof(uintptr_t));
    unsigned long* args_len = (unsigned long*) emalloc(max_args * sizeof(unsigned long));

    if (!args || !args_len) {
        if (args)
            efree(args);
        if (args_len)
            efree(args_len);
        *args_ptr      = NULL;
        *args_len_ptr  = NULL;
        *arg_count_ptr = 0;
        return;
    }

    /* Current argument index */
    unsigned long arg_idx = 0;

    /* First argument: key */
    args[arg_idx]     = (uintptr_t) key;
    args_len[arg_idx] = key_len;
    arg_idx++;

    /* Add sort patterns if provided */
    if (sort_pattern && Z_TYPE_P(sort_pattern) == IS_ARRAY) {
        HashTable*   ht = Z_ARRVAL_P(sort_pattern);
        zval*        z_ele;
        zend_string* z_key;
        zend_ulong   num_key;

        /* Check for BY pattern (case-insensitive) */
        if ((z_ele = zend_hash_str_find(ht, "by", sizeof("by") - 1)) != NULL ||
            (z_ele = zend_hash_str_find(ht, "BY", sizeof("BY") - 1)) != NULL) {
            if (Z_TYPE_P(z_ele) == IS_STRING) {
                /* Add BY keyword */
                args[arg_idx]     = (uintptr_t) "BY";
                args_len[arg_idx] = 2;
                arg_idx++;

                /* Add BY pattern */
                args[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_ele);
                args_len[arg_idx] = Z_STRLEN_P(z_ele);
                arg_idx++;
            }
        }

        /* Check for LIMIT array format: 'LIMIT' => [offset, count] */
        zval* z_limit = NULL;
        if ((z_limit = zend_hash_str_find(ht, "limit", sizeof("limit") - 1)) != NULL ||
            (z_limit = zend_hash_str_find(ht, "LIMIT", sizeof("LIMIT") - 1)) != NULL) {
            if (Z_TYPE_P(z_limit) == IS_ARRAY) {
                HashTable* limit_ht = Z_ARRVAL_P(z_limit);
                if (zend_hash_num_elements(limit_ht) >= 2) {
                    zval* z_offset = zend_hash_index_find(limit_ht, 0);
                    zval* z_count  = zend_hash_index_find(limit_ht, 1);

                    if (z_offset && z_count) {
                        /* Add LIMIT keyword */
                        args[arg_idx]     = (uintptr_t) "LIMIT";
                        args_len[arg_idx] = 5;
                        arg_idx++;

                        /* Add offset */
                        char*  offset_str;
                        size_t offset_len;
                        long   offset_val = zval_get_long(z_offset);
                        offset_str        = long_to_string(offset_val, &offset_len);
                        if (offset_str) {
                            args[arg_idx]     = (uintptr_t) offset_str;
                            args_len[arg_idx] = offset_len;
                            arg_idx++;

                            /* Add count */
                            char*  count_str;
                            size_t count_len;
                            long   count_val = zval_get_long(z_count);
                            count_str        = long_to_string(count_val, &count_len);
                            if (count_str) {
                                args[arg_idx]     = (uintptr_t) count_str;
                                args_len[arg_idx] = count_len;
                                arg_idx++;
                            } else {
                                efree(offset_str);
                            }
                        }
                    }
                }
            }
        }
        /* Fallback to old format for backward compatibility */
        else {
            zval *z_offset, *z_count;
            if ((z_offset = zend_hash_str_find(ht, "limit_offset", sizeof("limit_offset") - 1)) !=
                    NULL &&
                (z_count = zend_hash_str_find(ht, "limit_count", sizeof("limit_count") - 1)) !=
                    NULL) {
                /* Add LIMIT keyword */
                args[arg_idx]     = (uintptr_t) "LIMIT";
                args_len[arg_idx] = 5;
                arg_idx++;

                /* Add offset */
                char*  offset_str;
                size_t offset_len;
                long   offset_val = zval_get_long(z_offset);
                offset_str        = long_to_string(offset_val, &offset_len);
                if (offset_str) {
                    args[arg_idx]     = (uintptr_t) offset_str;
                    args_len[arg_idx] = offset_len;
                    arg_idx++;

                    /* Add count */
                    char*  count_str;
                    size_t count_len;
                    long   count_val = zval_get_long(z_count);
                    count_str        = long_to_string(count_val, &count_len);
                    if (count_str) {
                        args[arg_idx]     = (uintptr_t) count_str;
                        args_len[arg_idx] = count_len;
                        arg_idx++;
                    } else {
                        efree(offset_str);
                    }
                }
            }
        }

        /* Check for GET patterns (case-insensitive) */
        if ((z_ele = zend_hash_str_find(ht, "get", sizeof("get") - 1)) != NULL ||
            (z_ele = zend_hash_str_find(ht, "GET", sizeof("GET") - 1)) != NULL) {
            if (Z_TYPE_P(z_ele) == IS_ARRAY) {
                /* Handle array of GET patterns: 'get' => ['pattern1', 'pattern2'] */
                HashTable* get_ht = Z_ARRVAL_P(z_ele);
                zval*      z_pattern;
                ZEND_HASH_FOREACH_VAL(get_ht, z_pattern) {
                    if (Z_TYPE_P(z_pattern) == IS_STRING) {
                        /* Add GET keyword */
                        args[arg_idx]     = (uintptr_t) "GET";
                        args_len[arg_idx] = 3;
                        arg_idx++;

                        /* Add GET pattern */
                        args[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_pattern);
                        args_len[arg_idx] = Z_STRLEN_P(z_pattern);
                        arg_idx++;
                    }
                }
                ZEND_HASH_FOREACH_END();
            } else if (Z_TYPE_P(z_ele) == IS_STRING) {
                /* Handle single GET pattern: 'get' => 'pattern' */
                args[arg_idx]     = (uintptr_t) "GET";
                args_len[arg_idx] = 3;
                arg_idx++;

                args[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_ele);
                args_len[arg_idx] = Z_STRLEN_P(z_ele);
                arg_idx++;
            }
        }

        /* Check for STORE destination (case-insensitive) */
        if ((z_ele = zend_hash_str_find(ht, "store", sizeof("store") - 1)) != NULL ||
            (z_ele = zend_hash_str_find(ht, "STORE", sizeof("STORE") - 1)) != NULL) {
            if (Z_TYPE_P(z_ele) == IS_STRING) {
                /* Add STORE keyword */
                args[arg_idx]     = (uintptr_t) "STORE";
                args_len[arg_idx] = 5;
                arg_idx++;

                /* Add STORE destination key */
                args[arg_idx]     = (uintptr_t) Z_STRVAL_P(z_ele);
                args_len[arg_idx] = Z_STRLEN_P(z_ele);
                arg_idx++;
            }
        }
    }

    /* Add sorting options */
    if (alpha) {
        args[arg_idx]     = (uintptr_t) "ALPHA";
        args_len[arg_idx] = 5;
        arg_idx++;
    }

    if (desc) {
        args[arg_idx]     = (uintptr_t) "DESC";
        args_len[arg_idx] = 4;
        arg_idx++;
    } else if (explicit_asc) {
        args[arg_idx]     = (uintptr_t) "ASC";
        args_len[arg_idx] = 3;
        arg_idx++;
    }

    /* Set output parameters */
    *args_ptr      = args;
    *args_len_ptr  = args_len;
    *arg_count_ptr = arg_idx;
}

/* Free memory allocated for SORT command arguments */
static void free_sort_args(uintptr_t* args, unsigned long* args_len, unsigned long arg_count) {
    if (args && args_len) {
        /* Free any dynamically allocated argument strings (offset and count) */
        for (unsigned long i = 0; i < arg_count; i++) {
            /* Skip key, BY, GET, LIMIT, STORE, ALPHA, DESC, ASC and any string that was
               directly extracted from a zval (not allocated) */
            if (strcmp((const char*) args[i], "BY") != 0 &&
                strcmp((const char*) args[i], "GET") != 0 &&
                strcmp((const char*) args[i], "LIMIT") != 0 &&
                strcmp((const char*) args[i], "STORE") != 0 &&
                strcmp((const char*) args[i], "ALPHA") != 0 &&
                strcmp((const char*) args[i], "DESC") != 0 &&
                strcmp((const char*) args[i], "ASC") != 0 && i > 0 && /* Skip key */
                ((i > 1 && strcmp((const char*) args[i - 1], "LIMIT") ==
                               0) || /* Only free offset and count */
                 (i > 2 && strcmp((const char*) args[i - 2], "LIMIT") == 0))) {
                efree((void*) args[i]);
            }
        }

        efree(args);
        efree(args_len);
    }
}

/* Execute a SORT command using the Valkey Glide client */
int execute_sort_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key     = NULL;
    size_t               key_len = 0;
    zval*                z_opts  = NULL;
    zend_bool            alpha = 0, desc = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|a", &object, ce, &key, &key_len, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Build command arguments */
        uintptr_t*     args      = NULL;
        unsigned long* args_len  = NULL;
        unsigned long  arg_count = 0;
        build_sort_args(key, key_len, z_opts, &alpha, &desc, &args, &args_len, &arg_count);

        if (!args || !args_len || arg_count == 0) {
            if (args)
                efree(args);
            if (args_len)
                efree(args_len);
            return 0;
        }

        /* Execute the command */
        CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                    Sort,      /* command type */
                                                    arg_count, /* number of arguments */
                                                    args,      /* arguments */
                                                    args_len   /* argument lengths */
        );

        /* Free the argument arrays */
        free_sort_args(args, args_len, arg_count);

        /* Check if we have a valid result */
        if (!cmd_result || !cmd_result->response) {
            if (cmd_result)
                free_command_result(cmd_result);
            return 0;
        }

        /* Process the result */
        int ret_val = 0;

        /* Check for STORE option */
        if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
            zval* z_store = zend_hash_str_find(Z_ARRVAL_P(z_opts), "store", sizeof("store") - 1);
            if (z_store && Z_TYPE_P(z_store) == IS_STRING) {
                /* With STORE option, we get the number of stored elements */
                long result_value = 0;
                if (handle_int_response(cmd_result, &result_value)) {
                    ZVAL_LONG(return_value, result_value);
                    free_command_result(cmd_result);
                    return 1;
                }
                free_command_result(cmd_result);
                return 0;
            }
        }

        /* Without STORE option, we get an array of sorted values */
        if (cmd_result->response->response_type == Array) {
            array_init(return_value);
            ret_val = command_response_to_zval(
                cmd_result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        } else if (cmd_result->response->response_type == Null) {
            /* Empty array */
            array_init(return_value);
            ret_val = 1;
        }

        free_command_result(cmd_result);
        return ret_val;
    }

    return 0;
}


/* Execute a SORT_RO command using the Valkey Glide client */
int execute_sort_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key     = NULL;
    size_t               key_len = 0;
    zval*                z_opts  = NULL;
    zend_bool            alpha = 0, desc = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os|a", &object, ce, &key, &key_len, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Build command arguments */
        uintptr_t*     args      = NULL;
        unsigned long* args_len  = NULL;
        unsigned long  arg_count = 0;
        build_sort_args(key, key_len, z_opts, &alpha, &desc, &args, &args_len, &arg_count);

        if (!args || !args_len || arg_count == 0) {
            if (args)
                efree(args);
            if (args_len)
                efree(args_len);
            return 0;
        }

        /* Execute the command */
        CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                    SortReadOnly, /* command type */
                                                    arg_count,    /* number of arguments */
                                                    args,         /* arguments */
                                                    args_len      /* argument lengths */
        );

        /* Free the argument arrays */
        free_sort_args(args, args_len, arg_count);

        /* Check if we have a valid result */
        if (!cmd_result || !cmd_result->response) {
            if (cmd_result)
                free_command_result(cmd_result);
            return 0;
        }

        /* Process the result */
        int ret_val = 0;

        /* SORT_RO doesn't support STORE option, so we always get an array of values */
        if (cmd_result->response->response_type == Array) {
            array_init(return_value);
            ret_val = command_response_to_zval(
                cmd_result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        } else if (cmd_result->response->response_type == Null) {
            /* Empty array */
            array_init(return_value);
            ret_val = 1;
        }

        free_command_result(cmd_result);
        return ret_val;
    }

    return 0;
}
