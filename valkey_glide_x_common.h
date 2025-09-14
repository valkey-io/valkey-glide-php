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

#ifndef VALKEY_GLIDE_X_COMMON_H
#define VALKEY_GLIDE_X_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * STRUCTURES
 * ==================================================================== */

/**
 * Options for X* commands with COUNT option
 */
typedef struct _x_count_options_t {
    long count;     /* COUNT option value */
    int  has_count; /* Whether COUNT option is set */
} x_count_options_t;

/**
 * Options for XREAD/XREADGROUP commands
 */
typedef struct _x_read_options_t {
    long block;     /* BLOCK option value (milliseconds) */
    int  has_block; /* Whether BLOCK option is set */
    long count;     /* COUNT option value */
    int  has_count; /* Whether COUNT option is set */
    int  noack;     /* NOACK flag (for XREADGROUP) */
} x_read_options_t;

/**
 * Options for XPENDING command
 */
typedef struct _x_pending_options_t {
    const char* start;        /* Start ID */
    size_t      start_len;    /* Start ID length */
    const char* end;          /* End ID */
    size_t      end_len;      /* End ID length */
    long        count;        /* COUNT option value */
    int         has_count;    /* Whether COUNT option is set */
    const char* consumer;     /* Consumer name */
    size_t      consumer_len; /* Consumer name length */
} x_pending_options_t;

/**
 * Options for XTRIM command
 */
typedef struct _x_trim_options_t {
    int  approximate; /* Approximate flag (~) */
    long limit;       /* LIMIT option value */
    int  has_limit;   /* Whether LIMIT option is set */
} x_trim_options_t;

/**
 * Options for XADD command
 */
typedef struct _x_add_options_t {
    long maxlen;         /* MAXLEN option value */
    int  has_maxlen;     /* Whether MAXLEN option is set */
    int  approximate;    /* Approximate flag (~) */
    int  nomkstream;     /* NOMKSTREAM flag */
    int  minid_strategy; /* Whether to use MINID instead of MAXLEN */
} x_add_options_t;

/**
 * Options for XCLAIM/XAUTOCLAIM commands
 */
typedef struct _x_claim_options_t {
    long idle;           /* IDLE option value */
    int  has_idle;       /* Whether IDLE option is set */
    long time;           /* TIME option value */
    int  has_time;       /* Whether TIME option is set */
    long retrycount;     /* RETRYCOUNT option value */
    int  has_retrycount; /* Whether RETRYCOUNT option is set */
    int  force;          /* FORCE flag */
    int  justid;         /* JUSTID flag */
    int  has_count;      /* Whether COUNT option is set */
    long count;          /* COUNT option value */
} x_claim_options_t;

typedef struct {
    int justid;
} x_claim_result_context_t;

/**
 * Generic command arguments structure for X commands
 */
typedef struct _x_command_args_t {
    /* Common fields */
    const void* glide_client; /* GlideClient instance */
    const char* key;          /* Key argument */
    size_t      key_len;      /* Key argument length */

    /* For XLEN command - no additional fields needed */

    /* For XDEL command */
    zval* ids;      /* Array of IDs */
    int   id_count; /* Number of IDs */

    /* For XACK command */
    const char* group;     /* Group name */
    size_t      group_len; /* Group name length */
    /* ids and id_count are reused from XDEL */

    /* For XADD command */
    const char*     id;           /* ID to add */
    size_t          id_len;       /* ID length */
    zval*           field_values; /* Field-value pairs to add */
    int             fv_count;     /* Number of field-value pairs */
    x_add_options_t add_opts;     /* XADD options */

    /* For XTRIM command */
    const char*      strategy;      /* Strategy (MAXLEN, MINID) */
    size_t           strategy_len;  /* Strategy length */
    const char*      threshold;     /* Threshold value */
    size_t           threshold_len; /* Threshold length */
    x_trim_options_t trim_opts;     /* XTRIM options */

    /* For XRANGE/XREVRANGE commands */
    const char*       start;      /* Start ID */
    size_t            start_len;  /* Start ID length */
    const char*       end;        /* End ID */
    size_t            end_len;    /* End ID length */
    x_count_options_t range_opts; /* XRANGE options */

    /* For XPENDING command */
    /* key, group, and group_len reused from above */
    x_pending_options_t pending_opts; /* XPENDING options */

    /* For XREAD command */
    zval* streams; /* Array of stream keys */
    /* ids reused from above (contains stream IDs) */
    x_read_options_t read_opts; /* XREAD options */

    /* For XREADGROUP command */
    /* group and group_len reused from above */
    const char* consumer;     /* Consumer name */
    size_t      consumer_len; /* Consumer name length */
    /* streams and ids reused from above */
    /* read_opts reused from above */

    /* For XAUTOCLAIM command */
    /* key, group, group_len, consumer, consumer_len reused from above */
    long min_idle_time; /* Minimum idle time */
    /* start and start_len reused from above */
    x_claim_options_t claim_opts; /* XCLAIM options */

    /* For XINFO command */
    const char* subcommand;     /* Subcommand (CONSUMERS, GROUPS, STREAM) */
    size_t      subcommand_len; /* Subcommand length */
    zval*       args;           /* Additional arguments */
    int         args_count;     /* Number of additional arguments */

    /* For XGROUP command */
    /* subcommand, subcommand_len, args, args_count reused from above */

    /* General options */
    zval* options; /* Raw options array from PHP */
} x_command_args_t;

/* Function pointer types */
typedef int (*x_result_processor_t)(CommandResponse* response, void* output, zval* return_value);
typedef int (*x_arg_preparation_func_t)(x_command_args_t* args,
                                        uintptr_t**       args_out,
                                        unsigned long**   args_len_out,
                                        char***           allocated_strings,
                                        int*              allocated_count);
typedef int (*x_simple_arg_preparation_func_t)(x_command_args_t* args,
                                               uintptr_t**       args_out,
                                               unsigned long**   args_len_out);

/**
 * Command definition structure to encapsulate command properties
 */
typedef struct _x_command_def_t {
    enum RequestType         cmd_type;     /* ValkeyGlide command type */
    x_arg_preparation_func_t prepare_args; /* Function to prepare arguments */
} x_command_def_t;

/* Utility functions */
int  allocate_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out);
void free_command_args(uintptr_t* args, unsigned long* args_len);

/* Generic command execution framework */
int execute_x_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              x_command_args_t*    args,
                              void*                result_ptr,
                              x_result_processor_t process_result,
                              zval*                return_value);

/* Argument preparation */
int prepare_x_len_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);

int prepare_x_del_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);

int prepare_x_ack_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);

int prepare_x_add_args(x_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

int prepare_x_trim_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

int prepare_x_range_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

int prepare_x_claim_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out);

int prepare_x_autoclaim_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out);

int prepare_x_group_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out);

int prepare_x_pending_args(x_command_args_t* args,
                           uintptr_t**       args_out,
                           unsigned long**   args_len_out);

int prepare_x_read_args(x_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);

int prepare_x_readgroup_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out);

int prepare_x_info_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

int parse_x_add_options(zval* options, x_add_options_t* opts);
int parse_x_claim_options(zval* options, x_claim_options_t* opts);
int parse_x_count_options(zval* options, x_count_options_t* opts);
int parse_x_read_options(zval* options, x_read_options_t* opts);
int parse_x_pending_options(zval* options, x_pending_options_t* opts);
int parse_x_trim_options(zval* options, x_trim_options_t* opts);
int parse_x_group_options(zval* options, x_command_args_t* args);
int parse_x_info_options(zval* options, x_command_args_t* args);

/* Result processing */
int process_x_int_result(CommandResponse* response, void* output, zval* return_value);
int process_x_double_result(CommandResponse* response, void* output, zval* return_value);
int process_x_array_result(CommandResponse* response, void* output, zval* return_value);
int process_x_stream_result(CommandResponse* response, void* output, zval* return_value);
int process_x_add_result(CommandResponse* response, void* output, zval* return_value);
int process_x_group_result(CommandResponse* response, void* output, zval* return_value);
int process_x_pending_result(CommandResponse* response, void* output, zval* return_value);
int process_x_readgroup_result(CommandResponse* response, void* output, zval* return_value);
int process_x_claim_result(CommandResponse* response, void* output, zval* return_value);
int process_x_autoclaim_result(CommandResponse* response, void* output, zval* return_value);
int process_x_info_result(CommandResponse* response, void* output, zval* return_value);


/* Command implementation functions */
int execute_xlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xack_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_xadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xtrim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_xrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_xrevrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_xpending_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xread_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xreadgroup_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xautoclaim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xclaim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xgroup_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_xinfo_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
/* ====================================================================
 * X COMMAND MACROS
 * ==================================================================== */

#define XREADGROUP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xreadgroup) {                                              \
        if (execute_xreadgroup_command(getThis(),                                     \
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
#define XACK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xack) {                                              \
        if (execute_xack_command(getThis(),                                     \
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
#define XADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xadd) {                                              \
        if (execute_xadd_command(getThis(),                                     \
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

#define XAUTOCLAIM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xautoclaim) {                                              \
        if (execute_xautoclaim_command(getThis(),                                     \
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

#define XDEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xdel) {                                              \
        if (execute_xdel_command(getThis(),                                     \
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

#define XACK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xack) {                                              \
        if (execute_xack_command(getThis(),                                     \
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

#define XAUTOCLAIM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xautoclaim) {                                              \
        if (execute_xautoclaim_command(getThis(),                                     \
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

#define XCLAIM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xclaim) {                                              \
        if (execute_xclaim_command(getThis(),                                     \
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

#define XDEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xdel) {                                              \
        if (execute_xdel_command(getThis(),                                     \
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

#define XGROUP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xgroup) {                                              \
        if (execute_xgroup_command(getThis(),                                     \
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

#define XINFO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xinfo) {                                              \
        if (execute_xinfo_command(getThis(),                                     \
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

#define XLEN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xlen) {                                              \
        if (execute_xlen_command(getThis(),                                     \
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

#define XPENDING_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xpending) {                                              \
        if (execute_xpending_command(getThis(),                                     \
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

#define XRANGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xrange) {                                              \
        if (execute_xrange_command(getThis(),                                     \
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

#define XREAD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xread) {                                              \
        if (execute_xread_command(getThis(),                                     \
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

#define XREVRANGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xrevrange) {                                              \
        if (execute_xrevrange_command(getThis(),                                     \
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

#define XTRIM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, xtrim) {                                              \
        if (execute_xtrim_command(getThis(),                                     \
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

#endif /* VALKEY_GLIDE_X_COMMON_H */
