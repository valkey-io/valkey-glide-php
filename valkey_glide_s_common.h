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

#ifndef VALKEY_GLIDE_S_COMMON_H
#define VALKEY_GLIDE_S_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * ENUMS AND CONSTANTS
 * ==================================================================== */

/**
 * S command categories based on argument patterns
 */
typedef enum {
    S_CMD_KEY_MEMBERS,     /* SADD, SREM, SMISMEMBER - key + array of members */
    S_CMD_KEY_ONLY,        /* SCARD, SMEMBERS - key only */
    S_CMD_KEY_MEMBER,      /* SISMEMBER - key + single member */
    S_CMD_KEY_COUNT,       /* SPOP, SRANDMEMBER - key + optional count */
    S_CMD_MULTI_KEY,       /* SINTER, SUNION, SDIFF - array of keys */
    S_CMD_MULTI_KEY_LIMIT, /* SINTERCARD - keys + limit option */
    S_CMD_DST_MULTI_KEY,   /* SINTERSTORE, SUNIONSTORE, SDIFFSTORE - destination + source keys */
    S_CMD_TWO_KEY_MEMBER,  /* SMOVE - source + destination + member */
    S_CMD_SCAN             /* SCAN, SSCAN - cursor + pattern + count */
} s_command_category_t;

/**
 * S command response types
 */
typedef enum {
    S_RESPONSE_INT,   /* Integer response */
    S_RESPONSE_BOOL,  /* Boolean response (0/1) */
    S_RESPONSE_SET,   /* Set/Array response */
    S_RESPONSE_MIXED, /* Mixed response (string or array) */
    S_RESPONSE_SCAN   /* Scan response (cursor + array) */
} s_response_type_t;

/* ====================================================================
 * STRUCTURES
 * ==================================================================== */

/**
 * Generic command arguments structure for S commands
 */
typedef struct _s_command_args_t {
    /* Common fields */
    const void* glide_client; /* GlideClient instance */
    const char* key;          /* Primary key argument */
    size_t      key_len;      /* Primary key length */

    /* Multi-key commands */
    zval* keys;       /* Array of keys */
    int   keys_count; /* Number of keys */

    /* Member operations */
    zval*      members;       /* Array of members */
    int        members_count; /* Number of members */
    HashTable* members_ht;    /* HashTable for member operations */

    /* Single member operations */
    const char* member;     /* Single member */
    size_t      member_len; /* Single member length */

    /* Two-key operations */
    const char* dst_key;     /* Destination key (SMOVE, SINTERSTORE, etc.) */
    size_t      dst_key_len; /* Destination key length */
    const char* src_key;     /* Source key (SMOVE) */
    size_t      src_key_len; /* Source key length */

    /* Numeric parameters */
    long count;     /* COUNT parameter */
    long limit;     /* LIMIT parameter */
    int  has_count; /* Whether count is specified */
    int  has_limit; /* Whether limit is specified */

    /* Scan-specific parameters */
    char** cursor;    /* Cursor pointer for scan operations */
    zval*  scan_iter; /* Iterator for scan operations */

    const char* pattern;     /* MATCH pattern */
    size_t      pattern_len; /* Pattern length */
    const char* type;        /* TYPE filter (SCAN only) */
    size_t      type_len;    /* Type filter length */
    int         has_type;    /* Whether type filter is specified */

    /* Output parameters */
    long*   output_long;       /* For integer outputs */
    int*    output_int;        /* For boolean outputs */
    char**  output_string;     /* For string outputs */
    size_t* output_string_len; /* For string output length */
} s_command_args_t;

/**
 * Command definition structure
 */
typedef struct _s_command_def_t {
    enum RequestType     cmd_type;      /* ValkeyGlide command type */
    s_command_category_t category;      /* Command category */
    s_response_type_t    response_type; /* Expected response type */
} s_command_def_t;

/* ====================================================================
 * FUNCTION DECLARATIONS
 * ==================================================================== */

/* Core execution framework */
int execute_s_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              s_command_category_t category,
                              s_response_type_t    response_type,
                              s_command_args_t*    args,
                              zval*                return_value);

/* Argument preparation functions */
int prepare_s_key_members_args(s_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out);
int prepare_s_key_only_args(s_command_args_t* args,
                            uintptr_t**       args_out,
                            unsigned long**   args_len_out);
int prepare_s_key_member_args(s_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out);
int prepare_s_key_count_args(s_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out);
int prepare_s_multi_key_args(s_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out);
int prepare_s_multi_key_limit_args(s_command_args_t* args,
                                   uintptr_t**       args_out,
                                   unsigned long**   args_len_out);
int prepare_s_dst_multi_key_args(s_command_args_t* args,
                                 uintptr_t**       args_out,
                                 unsigned long**   args_len_out);
int prepare_s_two_key_member_args(s_command_args_t* args,
                                  uintptr_t**       args_out,
                                  unsigned long**   args_len_out);
int prepare_s_scan_args(s_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);


/* Utility functions */
int  allocate_s_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out);
void cleanup_s_command_args(uintptr_t* args, unsigned long* args_len);
int  convert_zval_to_string_args(
     zval* input, int count, uintptr_t** args_out, unsigned long** args_len_out, int offset);

char* alloc_long_string(long value, size_t* len_out);

/* Specific command implementations */
int execute_sadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_scard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_srem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_smove_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_spop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_srandmember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sismember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_smembers_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_smismember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sinter_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sintercard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sinterstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sunion_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sunionstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sdiff_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sdiffstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Scan command implementations */
int execute_scan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);


int execute_gen_scan_command_internal(valkey_glide_object* valkey_glide,
                                      enum RequestType     cmd_type,
                                      const char*          key,
                                      size_t               key_len,
                                      char**               cursor,
                                      const char*          pattern,
                                      size_t               pattern_len,
                                      long                 count,
                                      zval*                scan_iter,
                                      zval*                return_value);

/* Generic scan command wrapper for HSCAN, ZSCAN, SSCAN */
int execute_scan_command_generic(
    zval* object, int argc, zval* return_value, zend_class_entry* ce, enum RequestType cmd_type);

/* ====================================================================
 * CONVENIENCE MACROS
 * ==================================================================== */

/**
 * Initialize s_command_args_t structure with default values
 */
#define INIT_S_COMMAND_ARGS(args)                     \
    do {                                              \
        memset(&(args), 0, sizeof(s_command_args_t)); \
    } while (0)

/**
 * Set basic key arguments
 */
#define SET_S_KEY_ARGS(args, client, k, k_len) \
    do {                                       \
        (args).glide_client = (client);        \
        (args).key          = (k);             \
        (args).key_len      = (k_len);         \
    } while (0)

/**
 * Set member arguments
 */
#define SET_S_MEMBER_ARGS(args, m, m_count) \
    do {                                    \
        (args).members       = (m);         \
        (args).members_count = (m_count);   \
    } while (0)


/* ====================================================================
 * S COMMAND MACROS
 * ==================================================================== */

#define SADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sAdd) {                                              \
        if (execute_sadd_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            return;                                                             \
        }                                                                       \
        zval_dtor(return_value);                                                \
        RETURN_FALSE;                                                           \
    }


#define SCARD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, scard) {                                              \
        if (execute_scard_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        zval_dtor(return_value);                                                 \
        RETURN_FALSE;                                                            \
    }

#define SREM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, srem) {                                              \
        if (execute_srem_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            return;                                                             \
        }                                                                       \
        zval_dtor(return_value);                                                \
        RETURN_FALSE;                                                           \
    }

#define SMOVE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sMove) {                                              \
        if (execute_smove_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        zval_dtor(return_value);                                                 \
        RETURN_FALSE;                                                            \
    }

#define SPOP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sPop) {                                              \
        if (execute_spop_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            return;                                                             \
        }                                                                       \
        zval_dtor(return_value);                                                \
        RETURN_FALSE;                                                           \
    }

#define SRANDMEMBER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sRandMember) {                                              \
        if (execute_srandmember_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define SISMEMBER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sismember) {                                              \
        if (execute_sismember_command(getThis(),                                     \
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

#define SMEMBERS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sMembers) {                                              \
        if (execute_smembers_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#define SMISMEMBER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sMisMember) {                                              \
        if (execute_smismember_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        zval_dtor(return_value);                                                      \
        RETURN_FALSE;                                                                 \
    }

#define SINTER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sInter) {                                              \
        if (execute_sinter_command(getThis(),                                     \
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

#define SINTERCARD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sintercard) {                                              \
        if (execute_sintercard_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        zval_dtor(return_value);                                                      \
        RETURN_FALSE;                                                                 \
    }

#define SINTERSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sInterStore) {                                              \
        if (execute_sinterstore_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define SUNION_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sUnion) {                                              \
        if (execute_sunion_command(getThis(),                                     \
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

#define SDIFF_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sDiff) {                                              \
        if (execute_sdiff_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        zval_dtor(return_value);                                                 \
        RETURN_FALSE;                                                            \
    }

#define SDIFFSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sDiffStore) {                                              \
        if (execute_sdiffstore_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        zval_dtor(return_value);                                                      \
        RETURN_FALSE;                                                                 \
    }

#define SUNIONSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sUnionStore) {                                              \
        if (execute_sunionstore_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#endif /* VALKEY_GLIDE_S_COMMON_H */
