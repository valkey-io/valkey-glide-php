/*
  +----------------------------------------------------------------------+
  | Valkey Glide Geo-Commands Common Utilities                           |
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

#include "valkey_glide_geo_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_z_common.h"

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* ====================================================================
 * OPTION PARSING HELPERS
 * ==================================================================== */


/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */


/**
 * Prepare member-based geo command arguments (key + members)
 */
int prepare_geo_members_args(geo_command_args_t* args,
                             uintptr_t**         args_out,
                             unsigned long**     args_len_out,
                             char***             allocated_strings,
                             int*                allocated_count) {
    if (!args || !args->key || !args->members || args->member_count <= 0 || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Prepare command arguments: key + members */
    unsigned long arg_count = 1 + args->member_count;

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
    for (int i = 0; i < args->member_count; i++) {
        zval*  member    = &args->members[i];
        char*  str_val   = NULL;
        size_t str_len   = 0;
        int    need_free = 0;

        str_val = zval_to_string_safe(member, &str_len, &need_free);

        if (!str_val) {
            /* Cleanup on error */
            free_allocated_strings(*allocated_strings, *allocated_count);
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

    return arg_count;
}

/**
 * Prepare GEODIST command arguments (key + source + destination + optional unit)
 */
int prepare_geo_dist_args(geo_command_args_t* args,
                          uintptr_t**         args_out,
                          unsigned long**     args_len_out) {
    /* Check if client, key, src, dst are valid */
    if (!args || !args->key || !args->src_member || !args->dst_member || !args_out ||
        !args_len_out) {
        return 0;
    }

    /* Prepare command arguments */
    unsigned long arg_count = args->unit ? 4 : 3;
    *args_out               = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    *args_len_out           = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

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

    (*args_out)[1]     = (uintptr_t) args->src_member;
    (*args_len_out)[1] = args->src_member_len;

    (*args_out)[2]     = (uintptr_t) args->dst_member;
    (*args_len_out)[2] = args->dst_member_len;

    /* Optional unit argument */
    if (args->unit) {
        (*args_out)[3]     = (uintptr_t) args->unit;
        (*args_len_out)[3] = args->unit_len;
    }

    return arg_count;
}

/**
 * Prepare GEOADD command arguments (key + [lon, lat, member] triplets)
 */
int prepare_geo_add_args(geo_command_args_t* args,
                         uintptr_t**         args_out,
                         unsigned long**     args_len_out,
                         char***             allocated_strings,
                         int*                allocated_count) {
    /* Check if client, key, and args are valid */
    if (!args || !args->key || !args->geo_args || args->geo_args_count < 3 ||
        args->geo_args_count % 3 != 0 || !args_out || !args_len_out || !allocated_strings ||
        !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Prepare command arguments */
    unsigned long arg_count =
        1 + args->geo_args_count; /* key + (longitude, latitude, member) triplets */
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

    /* Add arguments: lon, lat, member, lon, lat, member, ... */
    for (int i = 0; i < args->geo_args_count; i++) {
        zval*  value     = &args->geo_args[i];
        char*  str_val   = NULL;
        size_t str_len   = 0;
        int    need_free = 0;

        str_val = zval_to_string_safe(value, &str_len, &need_free);

        if (!str_val) {
            /* Cleanup on error */
            free_allocated_strings(*allocated_strings, *allocated_count);
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

    return arg_count;
}


/**
 * Prepare GEOSEARCH command arguments
 */
int prepare_geo_search_args(geo_command_args_t* args,
                            uintptr_t**         args_out,
                            unsigned long**     args_len_out,
                            char***             allocated_strings,
                            int*                allocated_count) {
    /* Check if client is valid */
    if (!args || !args->key || !args->from || !args->by_radius || !args->unit || !args_out ||
        !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Calculate the maximum arguments we might need */
    unsigned long max_args = 15; /* Conservative estimate */
    *args_out              = (uintptr_t*) emalloc(max_args * sizeof(uintptr_t));
    *args_len_out          = (unsigned long*) emalloc(max_args * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Start building command arguments */
    unsigned long arg_idx = 0;

    /* First argument: key */
    (*args_out)[arg_idx]       = (uintptr_t) args->key;
    (*args_len_out)[arg_idx++] = args->key_len;

    /* Handle FROM parameter - could be member name or coordinates */
    if (Z_TYPE_P(args->from) == IS_STRING) {
        /* FROMMEMBER <member> */
        (*args_out)[arg_idx]       = (uintptr_t) "FROMMEMBER";
        (*args_len_out)[arg_idx++] = strlen("FROMMEMBER");

        (*args_out)[arg_idx]       = (uintptr_t) Z_STRVAL_P(args->from);
        (*args_len_out)[arg_idx++] = Z_STRLEN_P(args->from);
    } else if (Z_TYPE_P(args->from) == IS_ARRAY) {
        /* FROMLONLAT <lon> <lat> */
        zval *lon, *lat;
        lon = zend_hash_index_find(Z_ARRVAL_P(args->from), 0);
        lat = zend_hash_index_find(Z_ARRVAL_P(args->from), 1);

        if (lon && lat) {
            (*args_out)[arg_idx]       = (uintptr_t) "FROMLONLAT";
            (*args_len_out)[arg_idx++] = strlen("FROMLONLAT");

            /* Convert longitude and latitude to strings */
            size_t lon_str_len, lat_str_len;
            char*  lon_str = double_to_string(zval_get_double(lon), &lon_str_len);
            if (!lon_str) {
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }
            (*args_out)[arg_idx]                       = (uintptr_t) lon_str;
            (*args_len_out)[arg_idx++]                 = lon_str_len;
            (*allocated_strings)[(*allocated_count)++] = lon_str;

            char* lat_str = double_to_string(zval_get_double(lat), &lat_str_len);
            if (!lat_str) {
                free_allocated_strings(*allocated_strings, *allocated_count);
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }
            (*args_out)[arg_idx]                       = (uintptr_t) lat_str;
            (*args_len_out)[arg_idx++]                 = lat_str_len;
            (*allocated_strings)[(*allocated_count)++] = lat_str;
        }
    }

    /* Handle BY parameter */
    if (args->by_radius != NULL) {
        /* BYRADIUS <radius> <unit> */
        (*args_out)[arg_idx]       = (uintptr_t) "BYRADIUS";
        (*args_len_out)[arg_idx++] = strlen("BYRADIUS");

        /* Convert radius to string */
        size_t radius_str_len;
        char*  radius_str = double_to_string(*args->by_radius, &radius_str_len);
        if (!radius_str) {
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]                       = (uintptr_t) radius_str;
        (*args_len_out)[arg_idx++]                 = radius_str_len;
        (*allocated_strings)[(*allocated_count)++] = radius_str;

        (*args_out)[arg_idx]       = (uintptr_t) args->unit;
        (*args_len_out)[arg_idx++] = args->unit_len;
    }

    /* Add WITH* options if enabled */
    if (args->radius_opts.with_opts.withcoord) {
        (*args_out)[arg_idx]       = (uintptr_t) "WITHCOORD";
        (*args_len_out)[arg_idx++] = strlen("WITHCOORD");
    }

    if (args->radius_opts.with_opts.withdist) {
        (*args_out)[arg_idx]       = (uintptr_t) "WITHDIST";
        (*args_len_out)[arg_idx++] = strlen("WITHDIST");
    }

    if (args->radius_opts.with_opts.withhash) {
        (*args_out)[arg_idx]       = (uintptr_t) "WITHHASH";
        (*args_len_out)[arg_idx++] = strlen("WITHHASH");
    }

    /* Add COUNT option if set */
    if (args->radius_opts.count > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx++] = strlen("COUNT");

        /* Convert count to string */
        size_t count_str_len;
        char*  count_str = long_to_string(args->radius_opts.count, &count_str_len);
        if (!count_str) {
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]                       = (uintptr_t) count_str;
        (*args_len_out)[arg_idx++]                 = count_str_len;
        (*allocated_strings)[(*allocated_count)++] = count_str;

        /* Add ANY if specified */
        if (args->radius_opts.any) {
            (*args_out)[arg_idx]       = (uintptr_t) "ANY";
            (*args_len_out)[arg_idx++] = strlen("ANY");
        }
    }

    /* Add sorting option if specified */
    if (args->radius_opts.sort && args->radius_opts.sort_len > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) args->radius_opts.sort;
        (*args_len_out)[arg_idx++] = args->radius_opts.sort_len;
    }

    return arg_idx;
}

/**
 * Prepare GEOSEARCHSTORE command arguments
 */
int prepare_geo_search_store_args(geo_command_args_t* args,
                                  uintptr_t**         args_out,
                                  unsigned long**     args_len_out,
                                  char***             allocated_strings,
                                  int*                allocated_count) {
    /* Check if client is valid */
    if (!args || !args->dest || !args->src || !args->from || !args->by_radius || !args->unit ||
        !args_out || !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Calculate the maximum arguments we might need */
    unsigned long max_args = 16; /* Conservative estimate */
    *args_out              = (uintptr_t*) emalloc(max_args * sizeof(uintptr_t));
    *args_len_out          = (unsigned long*) emalloc(max_args * sizeof(unsigned long));

    if (!(*args_out) || !(*args_len_out)) {
        if (*args_out)
            efree(*args_out);
        if (*args_len_out)
            efree(*args_len_out);
        return 0;
    }

    /* Start building command arguments */
    unsigned long arg_idx = 0;

    /* First two arguments: destination and source keys */
    (*args_out)[arg_idx]       = (uintptr_t) args->dest;
    (*args_len_out)[arg_idx++] = args->dest_len;

    (*args_out)[arg_idx]       = (uintptr_t) args->src;
    (*args_len_out)[arg_idx++] = args->src_len;

    /* Handle FROM parameter - could be member name or coordinates */
    if (Z_TYPE_P(args->from) == IS_STRING) {
        /* FROMMEMBER <member> */
        (*args_out)[arg_idx]       = (uintptr_t) "FROMMEMBER";
        (*args_len_out)[arg_idx++] = strlen("FROMMEMBER");

        (*args_out)[arg_idx]       = (uintptr_t) Z_STRVAL_P(args->from);
        (*args_len_out)[arg_idx++] = Z_STRLEN_P(args->from);
    } else if (Z_TYPE_P(args->from) == IS_ARRAY) {
        /* FROMLONLAT <lon> <lat> */
        zval *lon, *lat;
        lon = zend_hash_index_find(Z_ARRVAL_P(args->from), 0);
        lat = zend_hash_index_find(Z_ARRVAL_P(args->from), 1);

        if (lon && lat) {
            (*args_out)[arg_idx]       = (uintptr_t) "FROMLONLAT";
            (*args_len_out)[arg_idx++] = strlen("FROMLONLAT");

            /* Convert longitude and latitude to strings */
            size_t lon_str_len, lat_str_len;
            char*  lon_str = double_to_string(zval_get_double(lon), &lon_str_len);
            if (!lon_str) {
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }
            (*args_out)[arg_idx]                       = (uintptr_t) lon_str;
            (*args_len_out)[arg_idx++]                 = lon_str_len;
            (*allocated_strings)[(*allocated_count)++] = lon_str;

            char* lat_str = double_to_string(zval_get_double(lat), &lat_str_len);
            if (!lat_str) {
                free_allocated_strings(*allocated_strings, *allocated_count);
                efree(*args_out);
                efree(*args_len_out);
                return 0;
            }
            (*args_out)[arg_idx]                       = (uintptr_t) lat_str;
            (*args_len_out)[arg_idx++]                 = lat_str_len;
            (*allocated_strings)[(*allocated_count)++] = lat_str;
        }
    }

    /* Handle BY parameter */
    if (args->by_radius != NULL) {
        /* BYRADIUS <radius> <unit> */
        (*args_out)[arg_idx]       = (uintptr_t) "BYRADIUS";
        (*args_len_out)[arg_idx++] = strlen("BYRADIUS");

        /* Convert radius to string */
        size_t radius_str_len;
        char*  radius_str = double_to_string(*args->by_radius, &radius_str_len);
        if (!radius_str) {
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }
        (*args_out)[arg_idx]                       = (uintptr_t) radius_str;
        (*args_len_out)[arg_idx++]                 = radius_str_len;
        (*allocated_strings)[(*allocated_count)++] = radius_str;

        (*args_out)[arg_idx]       = (uintptr_t) args->unit;
        (*args_len_out)[arg_idx++] = args->unit_len;
    }

    /* Add COUNT option if set */
    if (args->radius_opts.count > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx++] = strlen("COUNT");

        /* Convert count to string */
        size_t count_str_len;
        char*  count_str = long_to_string(args->radius_opts.count, &count_str_len);
        if (!count_str) {
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]                       = (uintptr_t) count_str;
        (*args_len_out)[arg_idx++]                 = count_str_len;
        (*allocated_strings)[(*allocated_count)++] = count_str;

        /* Add ANY if specified */
        if (args->radius_opts.any) {
            (*args_out)[arg_idx]       = (uintptr_t) "ANY";
            (*args_len_out)[arg_idx++] = strlen("ANY");
        }
    }

    /* Add sorting option if specified */
    if (args->radius_opts.sort && args->radius_opts.sort_len > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) args->radius_opts.sort;
        (*args_len_out)[arg_idx++] = args->radius_opts.sort_len;
    }

    /* Add STOREDIST if specified */
    if (args->radius_opts.store_dist) {
        (*args_out)[arg_idx]       = (uintptr_t) "STOREDIST";
        (*args_len_out)[arg_idx++] = strlen("STOREDIST");
    }

    return arg_idx;
}

/* ====================================================================
 * RESULT PROCESSING FUNCTIONS
 * ==================================================================== */

int process_geo_int_result_async(CommandResponse* response, void* output, zval* return_value) {
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
    ZVAL_LONG(return_value, 0);
    return 0;
}

int process_geo_double_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        ZVAL_NULL(return_value);
        return 0;
    }

    if (response->response_type == Null) {
        ZVAL_NULL(return_value);
        return 1;
    } else if (response->response_type == String) {
        ZVAL_DOUBLE(return_value, atof(response->string_value));
        return 1;
    } else if (response->response_type == Float) {
        ZVAL_DOUBLE(return_value, response->float_value);
        return 1;
    }
    return 0;
}


int process_geo_hash_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        array_init(return_value);
        return 0;
    }

    /* Process array of geohash strings */
    if (response->response_type == Array) {
        array_init(return_value);

        for (size_t i = 0; i < response->array_value_len; i++) {
            struct CommandResponse* element = &response->array_value[i];
            if (element->response_type == String) {
                add_next_index_stringl(
                    return_value, element->string_value, element->string_value_len);
            } else if (element->response_type == Null) {
                add_next_index_null(return_value);
            }
        }
        return 1;
    }

    /* If not an array, initialize empty array and return */
    array_init(return_value);
    return 0;
}


/**
 * Batch-compatible async result processor for GEOPOS responses
 */
int process_geo_pos_result_async(CommandResponse* response, void* output, zval* return_value) {
    if (!response || !return_value) {
        array_init(return_value);
        return 0;
    }

    /* Process array of coordinates */
    if (response->response_type == Array) {
        array_init(return_value);

        for (size_t i = 0; i < response->array_value_len; i++) {
            struct CommandResponse* element = &response->array_value[i];

            if (element->response_type == Array && element->array_value_len == 2) {
                /* Create a position array with [longitude, latitude] */
                zval position_array;
                array_init(&position_array);

                /* Add longitude */
                if (element->array_value[0].response_type == String) {
                    add_next_index_double(&position_array,
                                          atof(element->array_value[0].string_value));
                } else if (element->array_value[0].response_type == Float) {
                    add_next_index_double(&position_array, element->array_value[0].float_value);
                }

                /* Add latitude */
                if (element->array_value[1].response_type == String) {
                    add_next_index_double(&position_array,
                                          atof(element->array_value[1].string_value));
                } else if (element->array_value[1].response_type == Float) {
                    add_next_index_double(&position_array, element->array_value[1].float_value);
                }

                add_next_index_zval(return_value, &position_array);
            } else if (element->response_type == Null) {
                add_next_index_null(return_value);
            }
        }
        return 1;
    }

    /* If not an array, initialize empty array and return */
    array_init(return_value);
    return 0;
}

/**
 * Batch-compatible async result processor for GEOSEARCH responses
 */
int process_geo_search_result_async(CommandResponse* response, void* output, zval* return_value) {
    struct {
        int withcoord;
        int withdist;
        int withhash;
    }* search_data = (void*) output;

    if (!response || !return_value || !search_data) {
        efree(search_data);
        array_init(return_value);
        return 0;
    }

    int withcoord = search_data->withcoord;
    int withdist  = search_data->withdist;
    int withhash  = search_data->withhash;

    /* If no WITH* options, just return the array of names */
    if (!withcoord && !withdist && !withhash) {
        /* Simple case - just return the array */
        efree(search_data);
        return command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    }

    /* Process the result and build an associative array */
    if (response->response_type == Array) {
        array_init(return_value);

        for (size_t i = 0; i < response->array_value_len; i++) {
            struct CommandResponse* element = &response->array_value[i];

            /* Process elements with member name and WITH* data */
            if (element->response_type == Array && element->array_value_len > 0) {
                /* First element is always the member name */
                if (element->array_value[0].response_type == String) {
                    char*  member_name = element->array_value[0].string_value;
                    size_t member_len  = element->array_value[0].string_value_len;

                    /* Create an array for this member's data */
                    zval member_data;
                    array_init(&member_data);
                    CommandResponse* inner_element = &element->array_value[1];

                    int idx = 0;
                    /* Distance if requested */
                    if (withdist && idx < inner_element->array_value_len) {
                        if (inner_element->array_value[idx].response_type == String) {
                            add_next_index_double(
                                &member_data, atof(inner_element->array_value[idx].string_value));
                        } else if (inner_element->array_value[idx].response_type == Float) {
                            add_next_index_double(&member_data,
                                                  inner_element->array_value[idx].float_value);
                        }
                        idx++;
                    }

                    /* Hash if requested */
                    if (withhash && idx < inner_element->array_value_len) {
                        if (inner_element->array_value[idx].response_type == Int) {
                            add_next_index_long(&member_data,
                                                inner_element->array_value[idx].int_value);
                        }
                        idx++;
                    }

                    /* Coordinates if requested */
                    if (withcoord && idx < inner_element->array_value_len) {
                        if (inner_element->array_value[idx].response_type == Array &&
                            inner_element->array_value[idx].array_value_len == 2) {
                            /* Create a coordinates array */
                            zval coordinates;
                            array_init(&coordinates);

                            /* Add longitude */
                            if (inner_element->array_value[idx].array_value[0].response_type ==
                                String) {
                                add_next_index_double(&coordinates,
                                                      atof(inner_element->array_value[idx]
                                                               .array_value[0]
                                                               .string_value));
                            } else if (inner_element->array_value[idx]
                                           .array_value[0]
                                           .response_type == Float) {
                                add_next_index_double(
                                    &coordinates,
                                    inner_element->array_value[idx].array_value[0].float_value);
                            }

                            /* Add latitude */
                            if (inner_element->array_value[idx].array_value[1].response_type ==
                                String) {
                                add_next_index_double(&coordinates,
                                                      atof(inner_element->array_value[idx]
                                                               .array_value[1]
                                                               .string_value));
                            } else if (inner_element->array_value[idx]
                                           .array_value[1]
                                           .response_type == Float) {
                                add_next_index_double(
                                    &coordinates,
                                    inner_element->array_value[idx].array_value[1].float_value);
                            }

                            add_next_index_zval(&member_data, &coordinates);
                        }
                    }

                    /* Add the member data to the result array with member name as key */
                    add_assoc_zval_ex(return_value, member_name, member_len, &member_data);
                }
            }
        }
        efree(search_data);
        return 1;
    }

    /* If not an array, initialize empty array and return */
    array_init(return_value);
    efree(search_data);
    return 0;
}


/* ====================================================================
 * GENERIC EXECUTION FRAMEWORK
 * ==================================================================== */

/**
 * Generic GEO-command execution framework with batch support
 */
int execute_geo_generic_command(valkey_glide_object*   valkey_glide,
                                enum RequestType       cmd_type,
                                geo_command_args_t*    args,
                                void*                  result_ptr,
                                geo_result_processor_t process_result,
                                zval*                  return_value) {
    /* Check if client is valid */
    if (!valkey_glide || !args) {
        return 0;
    }

    uintptr_t*     arg_values        = NULL;
    unsigned long* arg_lens          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            success           = 0;

    /* Determine argument preparation method based on command type */
    switch (cmd_type) {
        case GeoAdd:
            allocated_strings = (char**) emalloc(args->geo_args_count * sizeof(char*));
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_geo_add_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case GeoDist:
            arg_count = prepare_geo_dist_args(args, &arg_values, &arg_lens);
            break;

        case GeoHash:
        case GeoPos:
            allocated_strings = (char**) emalloc(args->member_count * sizeof(char*));
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_geo_members_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;


        case GeoSearch:
            allocated_strings = (char**) emalloc(10 * sizeof(char*));
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_geo_search_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        case GeoSearchStore:
            allocated_strings = (char**) emalloc(10 * sizeof(char*));
            if (!allocated_strings) {
                return 0;
            }
            arg_count = prepare_geo_search_store_args(
                args, &arg_values, &arg_lens, &allocated_strings, &allocated_count);
            break;

        default:
            /* Unsupported command type */
            return 0;
    }

    /* Check if argument preparation was successful */
    if (arg_count <= 0) {
        if (allocated_strings)
            efree(allocated_strings);
        if (arg_values)
            efree(arg_values);
        if (arg_lens)
            efree(arg_lens);
        return 0;
    }

    /* Check if we're in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode: buffer the command and return success */
        int status = buffer_command_for_batch(valkey_glide,
                                              cmd_type,
                                              arg_values,
                                              arg_lens,
                                              arg_count,
                                              result_ptr,
                                              (z_result_processor_t) process_result);

        /* Free allocated strings */
        for (int i = 0; i < allocated_count; i++) {
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

        return status;
    }

    /* Execute the command synchronously */
    CommandResult* result = execute_command(valkey_glide->glide_client,
                                            cmd_type,   /* command type */
                                            arg_count,  /* number of arguments */
                                            arg_values, /* arguments */
                                            arg_lens    /* argument lengths */
    );

    /* Free allocated strings */
    for (int i = 0; i < allocated_count; i++) {
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
    success = process_result(result->response, result_ptr, return_value);

    /* Free the result */
    free_command_result(result);

    return success;
}
