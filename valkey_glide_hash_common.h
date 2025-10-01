/*
  +----------------------------------------------------------------------+
  | Valkey Glide Hash Commands Common Utilities                          |
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

#ifndef VALKEY_GLIDE_HASH_COMMON_H
#define VALKEY_GLIDE_HASH_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * STRUCTURES AND TYPES
 * ==================================================================== */

/**
 * Generic hash command arguments structure
 */
typedef struct _h_command_args_t {
    /* Core fields */
    const void* glide_client; /* GlideClient instance */
    const char* key;          /* Hash key */
    size_t      key_len;      /* Hash key length */

    /* Single field operations (HGET, HEXISTS, HSTRLEN, HSETNX) */
    char*  field;     /* Field name */
    size_t field_len; /* Field name length */
    char*  value;     /* Field value */
    size_t value_len; /* Field value length */

    /* Multi-field operations (HDEL, HMGET, HKEYS, HVALS) */
    zval* fields;      /* Array of field names */
    int   field_count; /* Number of fields */

    /* Field-value operations (HSET, HMSET) */
    zval* field_values; /* Associative array or alternating field/value array */
    int   fv_count;     /* Number of field-value pairs */
    int   is_array_arg; /* Whether using associative array format */

    /* Increment operations (HINCRBY, HINCRBYFLOAT) */
    long   increment;  /* Integer increment value */
    double float_incr; /* Float increment value */

    /* HRANDFIELD specific */
    long count;      /* Number of fields to return */
    int  withvalues; /* Whether to return values with fields */

    /* Hash Field Expiration specific */
    int         expiry;      /* Expiration time (seconds/milliseconds/timestamp) */
    const char* expiry_type; /* Expiry type: EX, PX, EXAT, PXAT, KEEPTTL, PERSIST */
    const char* condition;   /* Condition: NX, XX, GT, LT */
} h_command_args_t;


/**
 * Argument preparation function type
 */
typedef int (*h_arg_preparer_t)(h_command_args_t* args,
                                uintptr_t**       args_out,
                                unsigned long**   args_len_out,
                                char***           allocated_strings,
                                int*              allocated_count);

/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

/**
 * Generic hash command execution framework
 */
int execute_h_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              h_command_args_t*    args,
                              void*                result_ptr,
                              z_result_processor_t process_result,
                              zval*                return_value);

/**
 * Simplified execution for commands using standard response handlers
 */
int execute_h_simple_command(valkey_glide_object* valkey_glide,
                             enum RequestType     cmd_type,
                             h_command_args_t*    args,
                             void*                result_ptr,
                             int                  response_type,
                             zval*                return_value);

/* ====================================================================
 * ARGUMENT PREPARATION FUNCTIONS
 * ==================================================================== */

/**
 * Prepare arguments for single-key commands (HLEN)
 */
int prepare_h_key_only_args(h_command_args_t* args,
                            uintptr_t**       args_out,
                            unsigned long**   args_len_out,
                            char***           allocated_strings,
                            int*              allocated_count);

/**
 * Prepare arguments for single-field commands (HGET, HEXISTS, HSTRLEN)
 */
int prepare_h_single_field_args(h_command_args_t* args,
                                uintptr_t**       args_out,
                                unsigned long**   args_len_out,
                                char***           allocated_strings,
                                int*              allocated_count);

/**
 * Prepare arguments for field-value commands (HSETNX)
 */
int prepare_h_field_value_args(h_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out,
                               char***           allocated_strings,
                               int*              allocated_count);

/**
 * Prepare arguments for multi-field commands (HDEL, HMGET)
 */
int prepare_h_multi_field_args(h_command_args_t* args,
                               uintptr_t**       args_out,
                               unsigned long**   args_len_out,
                               char***           allocated_strings,
                               int*              allocated_count);

/**
 * Prepare arguments for HSET command (handles both formats)
 */
int prepare_h_set_args(h_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

/**
 * Prepare arguments for HMSET command
 */
int prepare_h_mset_args(h_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

/**
 * Prepare arguments for increment commands (HINCRBY, HINCRBYFLOAT)
 */
int prepare_h_incr_args(h_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

/**
 * Prepare arguments for HRANDFIELD command
 */
int prepare_h_randfield_args(h_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count);

/**
 * Prepare arguments for Hash Field Expiration commands
 */
int prepare_h_hfe_args(h_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

int prepare_h_expire_args(h_command_args_t* args,
                          uintptr_t**       args_out,
                          unsigned long**   args_len_out,
                          char***           allocated_strings,
                          int*              allocated_count);

int prepare_h_field_only_args(h_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out,
                              char***           allocated_strings,
                              int*              allocated_count);

int prepare_h_getex_args(h_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

/**
 * Safely allocate and format an integer as a string
 * Uses exact buffer size to prevent overruns
 */
char* safe_format_int(int value);

/**
 * Safely populate field arguments from zval array
 * Handles string conversion and memory management correctly
 * NOTE: Always creates copies because cleanup code expects to efree() all allocated_strings
 */
int populate_field_args(zval*          field_values,
                        int            fv_count,
                        int            start_idx,
                        uintptr_t*     args_out,
                        unsigned long* args_len_out,
                        char**         allocated_strings,
                        int*           allocated_count);

/* ====================================================================
 * RESULT PROCESSING FUNCTIONS
 * ==================================================================== */

/**
 * Process results for HRANDFIELD
 */
int process_h_randfield_result(CommandResponse* respone, void* output, zval* return_value);

/**
 * Process results for HINCRBYFLOAT
 */
int process_h_incrbyfloat_result(CommandResponse* respone, void* output, zval* return_value);

/* ====================================================================
 * UTILITY FUNCTIONS
 * ==================================================================== */

/**
 * Convert zval array to command arguments with proper string conversion
 */
int convert_zval_array_to_args(zval*          z_array,
                               int            start_index,
                               uintptr_t*     args,
                               unsigned long* args_len,
                               char**         allocated_strings,
                               int*           allocated_count,
                               int            max_allocations);

/**
 * Process field-value pairs from associative array
 */
int process_field_value_pairs(zval*          field_values,
                              uintptr_t*     args,
                              unsigned long* args_len,
                              int            start_index,
                              char**         allocated_strings,
                              int*           allocated_count);

/**
 * Safe cleanup for allocated argument strings
 */
void cleanup_h_command_args(char**         allocated_strings,
                            int            allocated_count,
                            uintptr_t*     args,
                            unsigned long* args_len);

/* ====================================================================
 * RESPONSE TYPE CONSTANTS
 * ==================================================================== */

#define H_RESPONSE_INT 1
#define H_RESPONSE_STRING 2
#define H_RESPONSE_BOOL 3
#define H_RESPONSE_ARRAY 4
#define H_RESPONSE_MAP 5
#define H_RESPONSE_OK 6
#define H_RESPONSE_CUSTOM 7

/* ====================================================================
 * HASH COMMAND EXECUTION FUNCTIONS
 * ==================================================================== */

/* Unified hash command executors matching macro signature pattern */
int execute_hget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexists_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hmset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hincrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hincrbyfloat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hmget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hkeys_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hvals_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hgetall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hstrlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hrandfield_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);


int execute_h_mset_command(valkey_glide_object* valkey_glide,
                           const char*          key,
                           size_t               key_len,
                           zval*                keyvals,
                           int                  keyvals_count,
                           zval*                return_value);


int execute_h_incrbyfloat_command(valkey_glide_object* valkey_glide,
                                  const char*          key,
                                  size_t               key_len,
                                  char*                field,
                                  size_t               field_len,
                                  double               increment,
                                  zval*                return_value);

int execute_h_mget_command(valkey_glide_object* valkey_glide,
                           const char*          key,
                           size_t               key_len,
                           zval*                fields,
                           int                  fields_count,
                           zval*                return_value);


int execute_h_randfield_command(valkey_glide_object* valkey_glide,
                                const char*          key,
                                size_t               key_len,
                                long                 count,
                                int                  withvalues,
                                zval*                return_value);

int process_h_ok_result_async(CommandResponse* response, void* output, zval* return_value);
/* ====================================================================
 * HASH COMMAND MACROS
 * ==================================================================== */

/**
 * Hash command method implementation macros
 */
#define HGET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hGet) {                                              \
        if (execute_hget_command(getThis(),                                     \
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

#define HLEN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hLen) {                                              \
        if (execute_hlen_command(getThis(),                                     \
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

#define HEXISTS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExists) {                                              \
        if (execute_hexists_command(getThis(),                                     \
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

#define HDEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hDel) {                                              \
        if (execute_hdel_command(getThis(),                                     \
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

#define HSET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hSet) {                                              \
        if (execute_hset_command(getThis(),                                     \
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

#define HSETNX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hSetNx) {                                              \
        if (execute_hsetnx_command(getThis(),                                     \
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

#define HMSET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hMset) {                                              \
        if (execute_hmset_command(getThis(),                                     \
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

#define HINCRBY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hIncrBy) {                                              \
        if (execute_hincrby_command(getThis(),                                     \
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

#define HINCRBYFLOAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hIncrByFloat) {                                              \
        if (execute_hincrbyfloat_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce())) {                \
            return;                                                                     \
        }                                                                               \
        zval_dtor(return_value);                                                        \
        RETURN_FALSE;                                                                   \
    }

#define HMGET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hMget) {                                              \
        if (execute_hmget_command(getThis(),                                     \
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

#define HKEYS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hKeys) {                                              \
        if (execute_hkeys_command(getThis(),                                     \
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

#define HVALS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hVals) {                                              \
        if (execute_hvals_command(getThis(),                                     \
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

#define HGETALL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hGetAll) {                                              \
        if (execute_hgetall_command(getThis(),                                     \
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

#define HSTRLEN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hStrLen) {                                              \
        if (execute_hstrlen_command(getThis(),                                     \
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

#define HRANDFIELD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hRandField) {                                              \
        if (execute_hrandfield_command(getThis(),                                     \
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

/* ====================================================================
 * CONVENIENCE MACROS
 * ==================================================================== */

/**
 * Macro to define PHP method implementations for hash commands
 */
#define HASH_METHOD_IMPL(class_name, method_name, execute_func, ...) \
    PHP_METHOD(class_name, method_name) {                            \
        if (execute_func(__VA_ARGS__)) {                             \
            return;                                                  \
        }                                                            \
        zval_dtor(return_value);                                     \
        RETURN_FALSE;                                                \
    }

/**
 * Helper macro for basic validation
 */
#define VALIDATE_HASH_ARGS(glide_client, key) \
    do {                                      \
        if (!(glide_client) || !(key)) {      \
            return 0;                         \
        }                                     \
    } while (0)

// Unified macro for hash field expiration methods
#define HSETEX_VARIANT_METHOD_IMPL(class_name, method_name, execute_func) \
    PHP_METHOD(class_name, method_name) {                                 \
        if (execute_func(getThis(),                                       \
                         ZEND_NUM_ARGS(),                                 \
                         return_value,                                    \
                         strcmp(#class_name, "ValkeyGlideCluster") == 0   \
                             ? get_valkey_glide_cluster_ce()              \
                             : get_valkey_glide_ce()))                    \
            return;                                                       \
        zval_dtor(return_value);                                          \
        RETURN_FALSE;                                                     \
    }

// Define all hSetEx method implementations
#define HSETEX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetEx, execute_hsetex_command)

#define HPSETEX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetEx, execute_hpsetex_command)

#define HSETEXAT_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetExAt, execute_hsetexat_command)

#define HPSETEXAT_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetExAt, execute_hpsetexat_command)

#define HSETEXNX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetExNx, execute_hsetexnx_command)

#define HPSETEXNX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetExNx, execute_hpsetexnx_command)

#define HSETEXATNX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetExAtNx, execute_hsetexatnx_command)

#define HPSETEXATNX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetExAtNx, execute_hpsetexatnx_command)

#define HSETEXXX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetExXx, execute_hsetexxx_command)

#define HPSETEXXX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetExXx, execute_hpsetexxx_command)

#define HSETEXATXX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hSetExAtXx, execute_hsetexatxx_command)

#define HPSETEXATXX_METHOD_IMPL(class_name) \
    HSETEX_VARIANT_METHOD_IMPL(class_name, hPSetExAtXx, execute_hpsetexatxx_command)


#define HEXPIRENX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireNx) {                                              \
        if (execute_hexpirenx_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce()))                  \
            return;                                                                  \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define HEXPIREXX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireXx) {                                              \
        if (execute_hexpirexx_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce()))                  \
            return;                                                                  \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define HPEXPIRENX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireNx) {                                              \
        if (execute_hpexpirenx_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce()))                  \
            return;                                                                   \
        zval_dtor(return_value);                                                      \
        RETURN_FALSE;                                                                 \
    }

#define HPEXPIREXX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireXx) {                                              \
        if (execute_hpexpirexx_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce()))                  \
            return;                                                                   \
        zval_dtor(return_value);                                                      \
        RETURN_FALSE;                                                                 \
    }

#define HEXPIREATNX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireAtNx) {                                              \
        if (execute_hexpireatnx_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce()))                  \
            return;                                                                    \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define HEXPIREATXX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireAtXx) {                                              \
        if (execute_hexpireatxx_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce()))                  \
            return;                                                                    \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define HPEXPIREATNX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireAtNx) {                                              \
        if (execute_hpexpireatnx_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce()))                  \
            return;                                                                     \
        zval_dtor(return_value);                                                        \
        RETURN_FALSE;                                                                   \
    }

#define HPEXPIREATXX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireAtXx) {                                              \
        if (execute_hpexpireatxx_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce()))                  \
            return;                                                                     \
        zval_dtor(return_value);                                                        \
        RETURN_FALSE;                                                                   \
    }

#define HEXPIRE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpire) {                                              \
        if (execute_hexpire_command(getThis(),                                     \
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

#define HTTL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hTtl) {                                              \
        if (execute_httl_command(getThis(),                                     \
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

#define HPERSIST_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPersist) {                                              \
        if (execute_hpersist_command(getThis(),                                     \
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

#define HPEXPIRE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpire) {                                              \
        if (execute_hpexpire_command(getThis(),                                     \
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

#define HEXPIREAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireAt) {                                              \
        if (execute_hexpireat_command(getThis(),                                     \
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

#define HPEXPIREAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireAt) {                                              \
        if (execute_hpexpireat_command(getThis(),                                     \
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

#define HPTTL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPTtl) {                                              \
        if (execute_hpttl_command(getThis(),                                     \
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

#define HEXPIRETIME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hExpireTime) {                                              \
        if (execute_hexpiretime_command(getThis(),                                     \
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

#define HPEXPIRETIME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hPExpireTime) {                                              \
        if (execute_hpexpiretime_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce())) {                \
            return;                                                                     \
        }                                                                               \
        zval_dtor(return_value);                                                        \
        RETURN_FALSE;                                                                   \
    }

#define HGETEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hGetEx) {                                              \
        if (execute_hgetex_command(getThis(),                                     \
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

// Hash Field Expiration function declarations
int execute_hsetex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetexat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetexat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetexnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetexnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetexatnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetexatnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetexxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetexxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hsetexatxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpsetexatxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

// Hash Field Expiration NX/XX variants
int execute_hexpirenx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpirexx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpirenx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpirexx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpireatnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpireatxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpireatnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpireatxx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_httl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hexpiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpexpiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hpersist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hgetex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

#endif /* VALKEY_GLIDE_HASH_COMMON_H */
