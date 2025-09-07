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

#ifndef VALKEY_GLIDE_GEO_COMMON_H
#define VALKEY_GLIDE_GEO_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * STRUCTURES
 * ==================================================================== */

/**
 * Options for GEO* commands with WITH* options
 */
typedef struct _geo_with_options_t {
    int withcoord; /* Include coordinates in the result */
    int withdist;  /* Include distance in the result */
    int withhash;  /* Include geohash in the result */
} geo_with_options_t;

/**
 * Options for GEORADIUS and GEOSEARCH commands
 */
typedef struct _geo_radius_options_t {
    geo_with_options_t with_opts; /* WITH* options */

    long        count;      /* COUNT option */
    int         any;        /* ANY flag for COUNT option */
    const char* sort;       /* Sort order (ASC/DESC) */
    size_t      sort_len;   /* Sort order string length */
    int         store_dist; /* STOREDIST option (for GEOSEARCHSTORE) */
} geo_radius_options_t;

/**
 * Common arguments structure for GEO commands
 */
typedef struct _geo_command_args_t {
    /* Common fields */
    const void* glide_client; /* GlideClient instance */
    const char* key;          /* Key argument */
    size_t      key_len;      /* Key argument length */

    /* For member-based commands (GEOHASH, GEOPOS) */
    zval* members;      /* Array of members or NULL */
    int   member_count; /* Number of members */

    /* Single-member fields (GEODIST) */
    const char* src_member;     /* Source member (for GEODIST) */
    size_t      src_member_len; /* Source member length */
    const char* dst_member;     /* Destination member (for GEODIST) */
    size_t      dst_member_len; /* Destination member length */

    /* For GEOADD command */
    zval* geo_args;       /* Array of [lon, lat, member] triplets */
    int   geo_args_count; /* Number of arguments in geo_args */

    /* For GEORADIUS/GEOSEARCH commands */
    double      longitude; /* Longitude for center point */
    double      latitude;  /* Latitude for center point */
    double      radius;    /* Radius for search */
    const char* unit;      /* Unit for radius (m, km, ft, mi) */
    size_t      unit_len;  /* Unit string length */

    /* For GEOSEARCH/GEOSEARCHSTORE */
    zval*   from;      /* FROMMEMBER or FROMLONLAT */
    double* by_radius; /* BYRADIUS value */

    /* For GEOSEARCHSTORE */
    const char* dest;     /* Destination key */
    size_t      dest_len; /* Destination key length */
    const char* src;      /* Source key */
    size_t      src_len;  /* Source key length */

    /* Common options */
    zval*                options;     /* Options array or NULL */
    geo_radius_options_t radius_opts; /* Parsed radius options */
} geo_command_args_t;

/* Function pointer type for result processors */
typedef int (*geo_result_processor_t)(CommandResponse* response, void* output, zval* return_value);

/* ====================================================================
 * FUNCTION PROTOTYPES
 * ==================================================================== */

int prepare_geo_members_args(geo_command_args_t* args,
                             uintptr_t**         args_out,
                             unsigned long**     args_len_out,
                             char***             allocated_strings,
                             int*                allocated_count);

int prepare_geo_dist_args(geo_command_args_t* args,
                          uintptr_t**         args_out,
                          unsigned long**     args_len_out);

int prepare_geo_add_args(geo_command_args_t* args,
                         uintptr_t**         args_out,
                         unsigned long**     args_len_out,
                         char***             allocated_strings,
                         int*                allocated_count);

int prepare_geo_search_args(geo_command_args_t* args,
                            uintptr_t**         args_out,
                            unsigned long**     args_len_out,
                            char***             allocated_strings,
                            int*                allocated_count);

int prepare_geo_search_store_args(geo_command_args_t* args,
                                  uintptr_t**         args_out,
                                  unsigned long**     args_len_out,
                                  char***             allocated_strings,
                                  int*                allocated_count);


/* Batch-compatible async result processors */
int process_geo_int_result_async(CommandResponse* response, void* output, zval* return_value);
int process_geo_double_result_async(CommandResponse* response, void* output, zval* return_value);
int process_geo_hash_result_async(CommandResponse* response, void* output, zval* return_value);
int process_geo_pos_result_async(CommandResponse* response, void* output, zval* return_value);
int process_geo_search_result_async(CommandResponse* response, void* output, zval* return_value);

int execute_geoadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_geohash_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_geodist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_geopos_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_geosearch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_geosearchstore_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);

/* Execution framework */
int execute_geo_generic_command(valkey_glide_object*   valkey_glide,
                                enum RequestType       cmd_type,
                                geo_command_args_t*    args,
                                void*                  result_ptr,
                                geo_result_processor_t process_result,
                                zval*                  return_value);

/* ====================================================================
 * GEO COMMAND MACROS
 * ==================================================================== */

/* Ultra-simple macro for GEOADD method implementation */
#define GEOADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geoadd) {                                              \
        if (execute_geoadd_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            return;                                                               \
        }                                                                         \
        RETURN_FALSE;                                                             \
    }

/* Ultra-simple macro for GEODIST method implementation */
#define GEODIST_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geodist) {                                              \
        if (execute_geodist_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            return;                                                                \
        }                                                                          \
        RETURN_FALSE;                                                              \
    }

/* Ultra-simple macro for GEOHASH method implementation */
#define GEOHASH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geohash) {                                              \
        if (execute_geohash_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            return;                                                                \
        }                                                                          \
        zval_dtor(return_value);                                                   \
        RETURN_FALSE;                                                              \
    }

/* Ultra-simple macro for GEOPOS method implementation */
#define GEOPOS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geopos) {                                              \
        if (execute_geopos_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            return;                                                               \
        }                                                                         \
        zval_dtor(return_value);                                                  \
        RETURN_FALSE;                                                             \
    }

/* Ultra-simple macro for GEORADIUSBYMEMBER method implementation */
#define GEORADIUSBYMEMBER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, georadiusbymember) {                                              \
        if (execute_georadiusbymember_command(getThis(),                                     \
                                              ZEND_NUM_ARGS(),                               \
                                              return_value,                                  \
                                              strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                  ? get_valkey_glide_cluster_ce()            \
                                                  : get_valkey_glide_ce())) {                \
            return;                                                                          \
        }                                                                                    \
        zval_dtor(return_value);                                                             \
        RETURN_FALSE;                                                                        \
    }

/* Ultra-simple macro for GEORADIUSBYMEMBER_RO method implementation */
#define GEORADIUSBYMEMBER_RO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, georadiusbymember_ro) {                                              \
        if (execute_georadiusbymember_ro_command(getThis(),                                     \
                                                 ZEND_NUM_ARGS(),                               \
                                                 return_value,                                  \
                                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                     ? get_valkey_glide_cluster_ce()            \
                                                     : get_valkey_glide_ce())) {                \
            return;                                                                             \
        }                                                                                       \
        zval_dtor(return_value);                                                                \
        RETURN_FALSE;                                                                           \
    }

/* Ultra-simple macro for GEOSEARCH method implementation */
#define GEOSEARCH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geosearch) {                                              \
        if (execute_geosearch_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

/* Ultra-simple macro for GEOSEARCHSTORE method implementation */
#define GEOSEARCHSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, geosearchstore) {                                              \
        if (execute_geosearchstore_command(getThis(),                                     \
                                           ZEND_NUM_ARGS(),                               \
                                           return_value,                                  \
                                           strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                               ? get_valkey_glide_cluster_ce()            \
                                               : get_valkey_glide_ce())) {                \
            return;                                                                       \
        }                                                                                 \
        RETURN_FALSE;                                                                     \
    }

#endif /* VALKEY_GLIDE_GEO_COMMON_H */
