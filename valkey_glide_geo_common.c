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
        char*  radius_str          = double_to_string(*args->by_radius, &radius_str_len);
        (*args_out)[arg_idx]       = (uintptr_t) radius_str;
        (*args_len_out)[arg_idx++] = radius_str_len;
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

/* ====================================================================
 * UNIFIED GEOSEARCH/GEOSEARCHSTORE IMPLEMENTATION
 * ==================================================================== */

/**
 * Parse parameters for GEOSEARCH/GEOSEARCHSTORE commands with flexible API support
 */
int parse_geosearch_parameters(int                  argc,
                               zval*                object,
                               zend_class_entry*    ce,
                               geo_search_params_t* params,
                               int                  is_store_variant) {
    zval* position = NULL;
    zval* shape    = NULL;
    zval* options  = NULL;

    /* Initialize parameters structure */
    memset(params, 0, sizeof(geo_search_params_t));

    if (is_store_variant) {
        /* GEOSEARCHSTORE: (object, dest, src, position, shape, unit [, options]) */
        char * dest = NULL, *src = NULL, *unit = NULL;
        size_t dest_len, src_len, unit_len;

        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Osszzs|a",
                                         &object,
                                         ce,
                                         &dest,
                                         &dest_len,
                                         &src,
                                         &src_len,
                                         &position,
                                         &shape,
                                         &unit,
                                         &unit_len,
                                         &options) == FAILURE) {
            return 0;
        }

        params->key         = dest;
        params->key_len     = dest_len;
        params->src_key     = src;
        params->src_key_len = src_len;
        params->unit        = unit;
        params->unit_len    = unit_len;
    } else {
        /* GEOSEARCH: (object, key, position, shape, unit [, options]) */
        char * key = NULL, *unit = NULL;
        size_t key_len, unit_len;

        if (zend_parse_method_parameters(argc,
                                         object,
                                         "Oszzs|a",
                                         &object,
                                         ce,
                                         &key,
                                         &key_len,
                                         &position,
                                         &shape,
                                         &unit,
                                         &unit_len,
                                         &options) == FAILURE) {
            return 0;
        }

        params->key      = key;
        params->key_len  = key_len;
        params->unit     = unit;
        params->unit_len = unit_len;
    }

    /* Parse position parameter */
    if (Z_TYPE_P(position) == IS_STRING) {
        /* FROMMEMBER */
        params->is_from_member = 1;
        params->member         = Z_STRVAL_P(position);
        params->member_len     = Z_STRLEN_P(position);
    } else if (Z_TYPE_P(position) == IS_ARRAY) {
        /* FROMLONLAT */
        HashTable* pos_ht = Z_ARRVAL_P(position);
        if (zend_hash_num_elements(pos_ht) != 2) {
            php_error_docref(
                NULL,
                E_WARNING,
                "Position array must contain exactly 2 elements [longitude, latitude]");
            return 0;
        }

        zval *lon_val, *lat_val;
        lon_val = zend_hash_index_find(pos_ht, 0);
        lat_val = zend_hash_index_find(pos_ht, 1);

        if (!lon_val || !lat_val) {
            php_error_docref(
                NULL, E_WARNING, "Position array must contain longitude and latitude values");
            return 0;
        }

        params->is_from_member = 0;
        params->longitude      = zval_get_double(lon_val);
        params->latitude       = zval_get_double(lat_val);
    } else {
        php_error_docref(
            NULL,
            E_WARNING,
            "Position must be either a string (member) or array [longitude, latitude]");
        return 0;
    }

    /* Parse shape parameter */
    if (Z_TYPE_P(shape) == IS_LONG || Z_TYPE_P(shape) == IS_DOUBLE) {
        /* BYRADIUS */
        params->is_by_radius = 1;
        params->radius       = zval_get_double(shape);
    } else if (Z_TYPE_P(shape) == IS_ARRAY) {
        /* BYBOX */
        HashTable* shape_ht = Z_ARRVAL_P(shape);
        if (zend_hash_num_elements(shape_ht) != 2) {
            php_error_docref(
                NULL, E_WARNING, "Shape array must contain exactly 2 elements [width, height]");
            return 0;
        }

        zval *width_val, *height_val;
        width_val  = zend_hash_index_find(shape_ht, 0);
        height_val = zend_hash_index_find(shape_ht, 1);

        if (!width_val || !height_val) {
            php_error_docref(NULL, E_WARNING, "Shape array must contain width and height values");
            return 0;
        }

        params->is_by_radius = 0;
        params->width        = zval_get_double(width_val);
        params->height       = zval_get_double(height_val);
    } else {
        php_error_docref(
            NULL, E_WARNING, "Shape must be either a number (radius) or array [width, height]");
        return 0;
    }

    /* Parse options if provided */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(options);
        zval*      opt;

        /* Parse array-based options (WITHCOORD, WITHDIST, WITHHASH) */
        ZEND_HASH_FOREACH_VAL(ht, opt) {
            if (Z_TYPE_P(opt) == IS_STRING) {
                if (strcasecmp(Z_STRVAL_P(opt), "withcoord") == 0) {
                    params->options.with_opts.withcoord = 1;
                } else if (strcasecmp(Z_STRVAL_P(opt), "withdist") == 0) {
                    params->options.with_opts.withdist = 1;
                } else if (strcasecmp(Z_STRVAL_P(opt), "withhash") == 0) {
                    params->options.with_opts.withhash = 1;
                } else if (strcasecmp(Z_STRVAL_P(opt), "asc") == 0) {
                    params->options.sort     = "ASC";
                    params->options.sort_len = 3;
                } else if (strcasecmp(Z_STRVAL_P(opt), "desc") == 0) {
                    params->options.sort     = "DESC";
                    params->options.sort_len = 4;
                }
            }
        }
        ZEND_HASH_FOREACH_END();

        /* Parse key-based options */
        zval* opt_val;

        /* COUNT option */
        if ((opt_val = zend_hash_str_find(ht, "count", sizeof("count") - 1)) != NULL) {
            if (Z_TYPE_P(opt_val) == IS_ARRAY) {
                /* COUNT with optional ANY: [count, "ANY"] */
                HashTable* count_ht  = Z_ARRVAL_P(opt_val);
                zval*      count_val = zend_hash_index_find(count_ht, 0);
                zval*      any_val   = zend_hash_index_find(count_ht, 1);

                if (count_val) {
                    params->options.count = zval_get_long(count_val);
                }
                if (any_val && Z_TYPE_P(any_val) == IS_STRING &&
                    strcasecmp(Z_STRVAL_P(any_val), "any") == 0) {
                    params->options.any = 1;
                }
            } else {
                /* Simple COUNT */
                params->options.count = zval_get_long(opt_val);
            }
        }

        /* SORT option (alternative to array-based) */
        if ((opt_val = zend_hash_str_find(ht, "sort", sizeof("sort") - 1)) != NULL) {
            if (Z_TYPE_P(opt_val) == IS_STRING) {
                params->options.sort     = Z_STRVAL_P(opt_val);
                params->options.sort_len = Z_STRLEN_P(opt_val);
            }
        }

        /* STOREDIST option (GEOSEARCHSTORE only) */
        if (is_store_variant) {
            if ((opt_val = zend_hash_str_find(ht, "storedist", sizeof("storedist") - 1)) != NULL) {
                params->options.store_dist = zval_is_true(opt_val);
            }
        }
    }

    return 1;
}

/**
 * Prepare arguments for unified GEOSEARCH/GEOSEARCHSTORE commands
 */
int prepare_geo_search_unified_args(geo_search_params_t* params,
                                    uintptr_t**          args_out,
                                    unsigned long**      args_len_out,
                                    char***              allocated_strings,
                                    int*                 allocated_count,
                                    int                  is_store_variant) {
    if (!params || !args_out || !args_len_out || !allocated_strings || !allocated_count) {
        return 0;
    }

    *allocated_count = 0;

    /* Calculate maximum arguments needed */
    unsigned long max_args = VALKEY_GLIDE_MAX_OPTIONS;
    *args_out              = (uintptr_t*) emalloc(max_args * sizeof(uintptr_t));
    *args_len_out          = (unsigned long*) emalloc(max_args * sizeof(unsigned long));

    unsigned long arg_idx = 0;

    /* Add key(s) */
    if (is_store_variant) {
        /* GEOSEARCHSTORE: destination, source */
        (*args_out)[arg_idx]       = (uintptr_t) params->key;
        (*args_len_out)[arg_idx++] = params->key_len;
        (*args_out)[arg_idx]       = (uintptr_t) params->src_key;
        (*args_len_out)[arg_idx++] = params->src_key_len;
    } else {
        /* GEOSEARCH: key */
        (*args_out)[arg_idx]       = (uintptr_t) params->key;
        (*args_len_out)[arg_idx++] = params->key_len;
    }

    /* Add FROM parameter */
    if (params->is_from_member) {
        /* FROMMEMBER */
        (*args_out)[arg_idx]       = (uintptr_t) "FROMMEMBER";
        (*args_len_out)[arg_idx++] = strlen("FROMMEMBER");
        (*args_out)[arg_idx]       = (uintptr_t) params->member;
        (*args_len_out)[arg_idx++] = params->member_len;
    } else {
        /* FROMLONLAT */
        (*args_out)[arg_idx]       = (uintptr_t) "FROMLONLAT";
        (*args_len_out)[arg_idx++] = strlen("FROMLONLAT");

        /* Convert coordinates to strings */
        size_t lon_str_len, lat_str_len;
        char*  lon_str = double_to_string(params->longitude, &lon_str_len);
        char*  lat_str = double_to_string(params->latitude, &lat_str_len);

        if (!lon_str || !lat_str) {
            if (lon_str)
                efree(lon_str);
            if (lat_str)
                efree(lat_str);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]                       = (uintptr_t) lon_str;
        (*args_len_out)[arg_idx++]                 = lon_str_len;
        (*allocated_strings)[(*allocated_count)++] = lon_str;

        (*args_out)[arg_idx]                       = (uintptr_t) lat_str;
        (*args_len_out)[arg_idx++]                 = lat_str_len;
        (*allocated_strings)[(*allocated_count)++] = lat_str;
    }

    /* Add BY parameter */
    if (params->is_by_radius) {
        /* BYRADIUS */
        (*args_out)[arg_idx]       = (uintptr_t) "BYRADIUS";
        (*args_len_out)[arg_idx++] = strlen("BYRADIUS");

        size_t radius_str_len;
        char*  radius_str = double_to_string(params->radius, &radius_str_len);
        if (!radius_str) {
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]                       = (uintptr_t) radius_str;
        (*args_len_out)[arg_idx++]                 = radius_str_len;
        (*allocated_strings)[(*allocated_count)++] = radius_str;
    } else {
        /* BYBOX */
        (*args_out)[arg_idx]       = (uintptr_t) "BYBOX";
        (*args_len_out)[arg_idx++] = strlen("BYBOX");

        size_t width_str_len, height_str_len;
        char*  width_str  = double_to_string(params->width, &width_str_len);
        char*  height_str = double_to_string(params->height, &height_str_len);

        if (!width_str || !height_str) {
            if (width_str)
                efree(width_str);
            if (height_str)
                efree(height_str);
            free_allocated_strings(*allocated_strings, *allocated_count);
            efree(*args_out);
            efree(*args_len_out);
            return 0;
        }

        (*args_out)[arg_idx]                       = (uintptr_t) width_str;
        (*args_len_out)[arg_idx++]                 = width_str_len;
        (*allocated_strings)[(*allocated_count)++] = width_str;

        (*args_out)[arg_idx]                       = (uintptr_t) height_str;
        (*args_len_out)[arg_idx++]                 = height_str_len;
        (*allocated_strings)[(*allocated_count)++] = height_str;
    }

    /* Add unit */
    (*args_out)[arg_idx]       = (uintptr_t) params->unit;
    (*args_len_out)[arg_idx++] = params->unit_len;

    /* Add sorting option */
    if (params->options.sort && params->options.sort_len > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) params->options.sort;
        (*args_len_out)[arg_idx++] = params->options.sort_len;
    }

    /* Add COUNT option */
    if (params->options.count > 0) {
        (*args_out)[arg_idx]       = (uintptr_t) "COUNT";
        (*args_len_out)[arg_idx++] = strlen("COUNT");

        size_t count_str_len;
        char*  count_str = long_to_string(params->options.count, &count_str_len);
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
        if (params->options.any) {
            (*args_out)[arg_idx]       = (uintptr_t) "ANY";
            (*args_len_out)[arg_idx++] = strlen("ANY");
        }
    }

    /* Add WITH* options (GEOSEARCH only) */
    if (!is_store_variant) {
        if (params->options.with_opts.withcoord) {
            (*args_out)[arg_idx]       = (uintptr_t) "WITHCOORD";
            (*args_len_out)[arg_idx++] = strlen("WITHCOORD");
        }
        if (params->options.with_opts.withdist) {
            (*args_out)[arg_idx]       = (uintptr_t) "WITHDIST";
            (*args_len_out)[arg_idx++] = strlen("WITHDIST");
        }
        if (params->options.with_opts.withhash) {
            (*args_out)[arg_idx]       = (uintptr_t) "WITHHASH";
            (*args_len_out)[arg_idx++] = strlen("WITHHASH");
        }
    }

    /* Add STOREDIST option (GEOSEARCHSTORE only) */
    if (is_store_variant && params->options.store_dist) {
        (*args_out)[arg_idx]       = (uintptr_t) "STOREDIST";
        (*args_len_out)[arg_idx++] = strlen("STOREDIST");
    }

    return arg_idx;
}

/**
 * Unified execution function for GEOSEARCH/GEOSEARCHSTORE
 */
int execute_geosearch_unified(
    zval* object, int argc, zval* return_value, zend_class_entry* ce, int is_store_variant) {
    geo_search_params_t params;
    const void*         glide_client = NULL;

    /* Parse parameters */
    if (!parse_geosearch_parameters(argc, object, ce, &params, is_store_variant)) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client) {
        return 0;
    }

    /* Prepare command arguments */
    uintptr_t*     arg_values        = NULL;
    unsigned long* arg_lens          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;

    allocated_strings = (char**) emalloc(15 * sizeof(char*));
    if (!allocated_strings) {
        return 0;
    }

    arg_count = prepare_geo_search_unified_args(
        &params, &arg_values, &arg_lens, &allocated_strings, &allocated_count, is_store_variant);

    if (arg_count <= 0) {
        if (allocated_strings)
            efree(allocated_strings);
        return 0;
    }

    /* Handle batch mode */
    if (valkey_glide->is_in_batch_mode) {
        enum RequestType cmd_type = is_store_variant ? GeoSearchStore : GeoSearch;

        void*                  result_ptr = NULL;
        geo_result_processor_t processor =
            is_store_variant ? process_geo_int_result_async : process_geo_search_result_async;

        if (!is_store_variant) {
            /* Create search data for GEOSEARCH result processing */
            typedef struct {
                int withcoord;
                int withdist;
                int withhash;
            } search_data_t;

            search_data_t* search_data = emalloc(sizeof(search_data_t));
            search_data->withcoord     = params.options.with_opts.withcoord;
            search_data->withdist      = params.options.with_opts.withdist;
            search_data->withhash      = params.options.with_opts.withhash;
            result_ptr                 = search_data;
        }

        int status = buffer_command_for_batch(valkey_glide,
                                              cmd_type,
                                              arg_values,
                                              arg_lens,
                                              arg_count,
                                              result_ptr,
                                              (z_result_processor_t) processor);

        /* Cleanup */
        for (int i = 0; i < allocated_count; i++) {
            if (allocated_strings[i]) {
                efree(allocated_strings[i]);
            }
        }
        efree(allocated_strings);
        efree(arg_values);
        efree(arg_lens);

        if (status) {
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    /* Execute synchronously */
    enum RequestType cmd_type = is_store_variant ? GeoSearchStore : GeoSearch;
    CommandResult*   result =
        execute_command(glide_client, cmd_type, arg_count, arg_values, arg_lens);

    /* Cleanup allocated strings */
    for (int i = 0; i < allocated_count; i++) {
        if (allocated_strings[i]) {
            efree(allocated_strings[i]);
        }
    }
    efree(allocated_strings);
    efree(arg_values);
    efree(arg_lens);

    if (!result || result->command_error) {
        if (result)
            free_command_result(result);
        return 0;
    }

    /* Process result */
    int success = 0;
    if (is_store_variant) {
        success = process_geo_int_result_async(result->response, NULL, return_value);
    } else {
        /* Create search data for result processing */
        typedef struct {
            int withcoord;
            int withdist;
            int withhash;
        } search_data_t;

        search_data_t* search_data = emalloc(sizeof(search_data_t));
        search_data->withcoord     = params.options.with_opts.withcoord;
        search_data->withdist      = params.options.with_opts.withdist;
        search_data->withhash      = params.options.with_opts.withhash;

        success = process_geo_search_result_async(result->response, search_data, return_value);
    }

    free_command_result(result);
    return success;
}
