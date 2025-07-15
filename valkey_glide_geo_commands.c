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
#include "valkey_glide_geo_common.h"
extern zend_class_entry* ce;

/* Execute a GEOADD command using the Valkey Glide client */
int execute_geoadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval*       z_args;
    int         variadic_argc = 0;
    const void* glide_client  = NULL;
    long        result_value  = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &variadic_argc) == FAILURE) {
        return 0;
    }

    /* Check that we have the right number of arguments */
    if (variadic_argc < 3 || variadic_argc % 3 != 0) {
        php_error_docref(
            NULL, E_WARNING, "geoadd requires at least one longitude/latitude/member triplet");
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

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.geo_args           = z_args;
    args.geo_args_count     = variadic_argc;

    /* Execute the generic command with appropriate result processor */
    int result = execute_geo_generic_command(
        glide_client, GeoAdd, &args, &result_value, process_geo_int_result);

    if (result) {
        ZVAL_LONG(return_value, result_value);
    }

    return result;
}

/* Execute a GEODIST command using the Valkey Glide client */
int execute_geodist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *src = NULL, *dst = NULL, *unit = NULL;
    size_t      key_len, src_len, dst_len, unit_len         = 0;
    const void* glide_client = NULL;
    double      result_value;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osss|s",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &src,
                                     &src_len,
                                     &dst,
                                     &dst_len,
                                     &unit,
                                     &unit_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client || !key || !src || !dst) {
        return 0;
    }

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.src_member         = src;
    args.src_member_len     = src_len;
    args.dst_member         = dst;
    args.dst_member_len     = dst_len;
    args.unit               = unit;
    args.unit_len           = unit_len;

    /* Execute the generic command with appropriate result processor */
    int ret = execute_geo_generic_command(
        glide_client, GeoDist, &args, &result_value, process_geo_double_result);

    if (ret == 1) {
        /* Command succeeded, return the value */
        ZVAL_DOUBLE(return_value, result_value);
    } else if (ret == 0) {
        /* Key or member doesn't exist */
        ZVAL_NULL(return_value);
    }

    return ret;
}

/* Execute a GEOHASH command using the Valkey Glide client */
int execute_geohash_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval*       z_args;
    int         variadic_argc = 0;
    const void* glide_client  = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &variadic_argc) == FAILURE) {
        return 0;
    }

    /* Check that we have at least one member */
    if (variadic_argc < 1) {
        php_error_docref(NULL, E_WARNING, "geohash requires at least one member");
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

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.members            = z_args;
    args.member_count       = variadic_argc;

    /* Initialize return value as array */
    array_init(return_value);

    /* Execute the generic command with appropriate result processor */
    return execute_geo_generic_command(
        glide_client, GeoHash, &args, return_value, process_geo_hash_result);
}

/* Execute a GEOPOS command using the Valkey Glide client */
int execute_geopos_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char*       key = NULL;
    size_t      key_len;
    zval*       z_args;
    int         variadic_argc = 0;
    const void* glide_client  = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os*", &object, ce, &key, &key_len, &z_args, &variadic_argc) == FAILURE) {
        return 0;
    }

    /* Check that we have at least one member */
    if (variadic_argc < 1) {
        php_error_docref(NULL, E_WARNING, "geopos requires at least one member");
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

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.members            = z_args;
    args.member_count       = variadic_argc;

    /* Initialize return value as array */
    array_init(return_value);

    /* Execute the generic command with appropriate result processor */
    return execute_geo_generic_command(
        glide_client, GeoPos, &args, return_value, process_geo_pos_result);
}

/* Execute a GEORADIUS command using the Valkey Glide client */
int execute_georadius_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *unit = NULL;
    size_t      key_len, unit_len;
    double      lng, lat, radius;
    zval*       z_opts       = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osddds|a",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &lng,
                                     &lat,
                                     &radius,
                                     &unit,
                                     &unit_len,
                                     &z_opts) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client || !key || !unit) {
        return 0;
    }

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.longitude          = lng;
    args.latitude           = lat;
    args.radius             = radius;
    args.unit               = unit;
    args.unit_len           = unit_len;
    args.options            = z_opts;

    /* Parse the WITH* options if provided */
    if (z_opts != NULL && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(z_opts);
        zval *     z_withcoord, *z_withdist, *z_withhash, *z_count, *z_sort;

        /* Check for WITHCOORD option */
        if ((z_withcoord = zend_hash_str_find(ht, "WITHCOORD", sizeof("WITHCOORD") - 1)) != NULL) {
            args.radius_opts.with_opts.withcoord = zval_is_true(z_withcoord);
        }

        /* Check for WITHDIST option */
        if ((z_withdist = zend_hash_str_find(ht, "WITHDIST", sizeof("WITHDIST") - 1)) != NULL) {
            args.radius_opts.with_opts.withdist = zval_is_true(z_withdist);
        }

        /* Check for WITHHASH option */
        if ((z_withhash = zend_hash_str_find(ht, "WITHHASH", sizeof("WITHHASH") - 1)) != NULL) {
            args.radius_opts.with_opts.withhash = zval_is_true(z_withhash);
        }

        /* Check for COUNT option */
        if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
            if (Z_TYPE_P(z_count) == IS_LONG) {
                args.radius_opts.count = Z_LVAL_P(z_count);
            }
        }

        /* Check for sorting option (ASC/DESC) */
        if ((z_sort = zend_hash_str_find(ht, "SORT", sizeof("SORT") - 1)) != NULL) {
            if (Z_TYPE_P(z_sort) == IS_STRING) {
                args.radius_opts.sort     = Z_STRVAL_P(z_sort);
                args.radius_opts.sort_len = Z_STRLEN_P(z_sort);
            }
        }
    }

    /* Create a data structure to pass the WITH* options to the result processor */
    struct {
        zval* return_value;
        int   withcoord;
        int   withdist;
        int   withhash;
    } radius_data = {return_value,
                     args.radius_opts.with_opts.withcoord,
                     args.radius_opts.with_opts.withdist,
                     args.radius_opts.with_opts.withhash};

    /* Initialize return value as array */
    array_init(return_value);

    /* Execute the generic command with appropriate result processor */
    return execute_geo_generic_command(
        glide_client, GeoRadius, &args, &radius_data, process_geo_radius_result);
}

/* Execute a GEORADIUS_RO command using the Valkey Glide client */
int execute_georadius_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *unit = NULL;
    size_t      key_len, unit_len;
    double      lng, lat, radius;
    zval*       z_opts       = NULL;
    const void* glide_client = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osddds|a",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &lng,
                                     &lat,
                                     &radius,
                                     &unit,
                                     &unit_len,
                                     &z_opts) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client || !key || !unit) {
        return 0;
    }

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.longitude          = lng;
    args.latitude           = lat;
    args.radius             = radius;
    args.unit               = unit;
    args.unit_len           = unit_len;
    args.options            = z_opts;

    /* Parse the WITH* options if provided */
    if (z_opts != NULL && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(z_opts);
        zval *     z_withcoord, *z_withdist, *z_withhash, *z_count, *z_sort;

        /* Check for WITHCOORD option */
        if ((z_withcoord = zend_hash_str_find(ht, "WITHCOORD", sizeof("WITHCOORD") - 1)) != NULL) {
            args.radius_opts.with_opts.withcoord = zval_is_true(z_withcoord);
        }

        /* Check for WITHDIST option */
        if ((z_withdist = zend_hash_str_find(ht, "WITHDIST", sizeof("WITHDIST") - 1)) != NULL) {
            args.radius_opts.with_opts.withdist = zval_is_true(z_withdist);
        }

        /* Check for WITHHASH option */
        if ((z_withhash = zend_hash_str_find(ht, "WITHHASH", sizeof("WITHHASH") - 1)) != NULL) {
            args.radius_opts.with_opts.withhash = zval_is_true(z_withhash);
        }

        /* Check for COUNT option */
        if ((z_count = zend_hash_str_find(ht, "COUNT", sizeof("COUNT") - 1)) != NULL) {
            if (Z_TYPE_P(z_count) == IS_LONG) {
                args.radius_opts.count = Z_LVAL_P(z_count);
            }
        }

        /* Check for sorting option (ASC/DESC) */
        if ((z_sort = zend_hash_str_find(ht, "SORT", sizeof("SORT") - 1)) != NULL) {
            if (Z_TYPE_P(z_sort) == IS_STRING) {
                args.radius_opts.sort     = Z_STRVAL_P(z_sort);
                args.radius_opts.sort_len = Z_STRLEN_P(z_sort);
            }
        }
    }

    /* Create a data structure to pass the WITH* options to the result processor */
    struct {
        zval* return_value;
        int   withcoord;
        int   withdist;
        int   withhash;
    } radius_data = {return_value,
                     args.radius_opts.with_opts.withcoord,
                     args.radius_opts.with_opts.withdist,
                     args.radius_opts.with_opts.withhash};

    /* Initialize return value as array */
    array_init(return_value);

    /* Execute the generic command with appropriate result processor */
    return execute_geo_generic_command(
        glide_client,
        GeoRadius, /* Same command type - GeoRadius since no RO variant in enum */
        &args,
        &radius_data,
        process_geo_radius_result);
}

/* GEOSEARCH implementation */
int execute_geosearch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    char *      key = NULL, *unit = NULL;
    size_t      key_len, unit_len;
    zval*       from = NULL;
    double      radius;
    zval*       options      = NULL;
    const void* glide_client = NULL;

    /* Parse parameters for simple case: geosearch(key, member, radius, unit [, options]) */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oszds|a",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &from,
                                     &radius,
                                     &unit,
                                     &unit_len,
                                     &options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client || !key || !from) {
        return 0;
    }

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.key                = key;
    args.key_len            = key_len;
    args.from               = from;
    args.by_radius          = &radius;
    args.unit               = unit;
    args.unit_len           = unit_len;
    args.options            = options;

    /* Parse the WITH* options if provided */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(options);
        zval*      opt;

        ZEND_HASH_FOREACH_VAL(ht, opt) {
            if (Z_TYPE_P(opt) == IS_STRING) {
                if (strcasecmp(Z_STRVAL_P(opt), "withcoord") == 0) {
                    args.radius_opts.with_opts.withcoord = 1;
                } else if (strcasecmp(Z_STRVAL_P(opt), "withdist") == 0) {
                    args.radius_opts.with_opts.withdist = 1;
                } else if (strcasecmp(Z_STRVAL_P(opt), "withhash") == 0) {
                    args.radius_opts.with_opts.withhash = 1;
                }
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    /* Create a data structure to pass the WITH* options to the result processor */
    struct {
        zval* return_value;
        int   withcoord;
        int   withdist;
        int   withhash;
    } search_data = {return_value,
                     args.radius_opts.with_opts.withcoord,
                     args.radius_opts.with_opts.withdist,
                     args.radius_opts.with_opts.withhash};

    /* Initialize return value as array */
    array_init(return_value);

    /* Execute the generic command with appropriate result processor */
    return execute_geo_generic_command(
        glide_client, GeoSearch, &args, &search_data, process_geo_search_result);
}

/* GEOSEARCHSTORE implementation */
int execute_geosearchstore_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    char *      dest = NULL, *src = NULL, *unit = NULL;
    size_t      dest_len, src_len, unit_len;
    zval*       from;
    double      radius;
    zval*       options      = NULL;
    const void* glide_client = NULL;
    long        result_value = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osszds|a",
                                     &object,
                                     ce,
                                     &dest,
                                     &dest_len,
                                     &src,
                                     &src_len,
                                     &from,
                                     &radius,
                                     &unit,
                                     &unit_len,
                                     &options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    glide_client = valkey_glide->glide_client;

    /* Check if we have a valid glide client */
    if (!glide_client || !dest || !src || !from) {
        return 0;
    }

    /* Initialize geo command arguments structure */
    geo_command_args_t args = {0};
    args.dest               = dest;
    args.dest_len           = dest_len;
    args.src                = src;
    args.src_len            = src_len;
    args.from               = from;
    args.by_radius          = &radius;
    args.unit               = unit;
    args.unit_len           = unit_len;
    args.options            = options;

    /* Parse options if provided */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        HashTable* ht_options = Z_ARRVAL_P(options);
        zval*      opt_val;

        /* COUNT option */
        if ((opt_val = zend_hash_str_find(ht_options, "count", sizeof("count") - 1)) != NULL) {
            args.radius_opts.count = zval_get_long(opt_val);
        }

        /* SORT option (ASC/DESC) */
        if ((opt_val = zend_hash_str_find(ht_options, "sort", sizeof("sort") - 1)) != NULL) {
            if (Z_TYPE_P(opt_val) == IS_STRING) {
                args.radius_opts.sort     = Z_STRVAL_P(opt_val);
                args.radius_opts.sort_len = Z_STRLEN_P(opt_val);
            }
        }

        /* STOREDIST option */
        if ((opt_val = zend_hash_str_find(ht_options, "storedist", sizeof("storedist") - 1)) !=
            NULL) {
            args.radius_opts.store_dist = zval_is_true(opt_val);
        }
    }

    /* Execute the generic command with appropriate result processor */
    int result = execute_geo_generic_command(
        glide_client, GeoSearchStore, &args, &result_value, process_geo_int_result);

    if (result) {
        ZVAL_LONG(return_value, result_value);
    }

    return result;
}
