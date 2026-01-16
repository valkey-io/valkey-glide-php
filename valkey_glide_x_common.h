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
    long count;     /* COUNT option value */
    int  has_block; /* Whether BLOCK option is set */
    int  has_count; /* Whether COUNT option is set */
    int  noack;     /* NOACK flag (for XREADGROUP) */
} x_read_options_t;

/**
 * Options for XPENDING command
 */
typedef struct _x_pending_options_t {
    const char* start;        /* Start ID */
    const char* end;          /* End ID */
    const char* consumer;     /* Consumer name */
    size_t      start_len;    /* Start ID length */
    size_t      end_len;      /* End ID length */
    size_t      consumer_len; /* Consumer name length */
    long        count;        /* COUNT option value */
    int         has_count;    /* Whether COUNT option is set */
} x_pending_options_t;

/**
 * Options for XTRIM command
 */
typedef struct _x_trim_options_t {
    long limit;       /* LIMIT option value */
    int  approximate; /* Approximate flag (~) */
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
    long time;           /* TIME option value */
    long retrycount;     /* RETRYCOUNT option value */
    long count;          /* COUNT option value */
    int  has_idle;       /* Whether IDLE option is set */
    int  has_time;       /* Whether TIME option is set */
    int  has_retrycount; /* Whether RETRYCOUNT option is set */
    int  force;          /* FORCE flag */
    int  justid;         /* JUSTID flag */
    int  has_count;      /* Whether COUNT option is set */
} x_claim_options_t;

typedef struct {
    int justid;
} x_claim_result_context_t;

/**
 * Generic command arguments structure for X commands
 */
typedef struct _x_command_args_t {
    const void*         glide_client;   /* GlideClient instance */
    const char*         key;            /* Key argument */
    zval*               ids;            /* Array of IDs */
    const char*         group;          /* Group name */
    const char*         id;             /* ID to add */
    zval*               field_values;   /* Field-value pairs to add */
    const char*         strategy;       /* Strategy (MAXLEN, MINID) */
    const char*         threshold;      /* Threshold value */
    const char*         start;          /* Start ID */
    const char*         end;            /* End ID */
    zval*               streams;        /* Array of stream keys */
    const char*         consumer;       /* Consumer name */
    const char*         subcommand;     /* Subcommand (CONSUMERS, GROUPS, STREAM) */
    zval*               args;           /* Additional arguments */
    zval*               options;        /* Raw options array from PHP */
    size_t              key_len;        /* Key argument length */
    size_t              group_len;      /* Group name length */
    size_t              id_len;         /* ID length */
    size_t              strategy_len;   /* Strategy length */
    size_t              threshold_len;  /* Threshold length */
    size_t              start_len;      /* Start ID length */
    size_t              end_len;        /* End ID length */
    size_t              consumer_len;   /* Consumer name length */
    size_t              subcommand_len; /* Subcommand length */
    long                min_idle_time;  /* Minimum idle time */
    x_pending_options_t pending_opts;   /* XPENDING options */
    x_read_options_t    read_opts;      /* XREAD options */
    x_claim_options_t   claim_opts;     /* XCLAIM options */
    x_add_options_t     add_opts;       /* XADD options */
    x_trim_options_t    trim_opts;      /* XTRIM options */
    x_count_options_t   range_opts;     /* XRANGE options */
    int                 id_count;       /* Number of IDs */
    int                 fv_count;       /* Number of field-value pairs */
    int                 args_count;     /* Number of additional arguments */
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
    x_arg_preparation_func_t prepare_args; /* Function to prepare arguments */
    enum RequestType         cmd_type;     /* ValkeyGlide command type */
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
int prepare_x_len_args(x_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

int prepare_x_del_args(x_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

int prepare_x_ack_args(x_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

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
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

int prepare_x_autoclaim_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count);

int prepare_x_group_args(x_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

int prepare_x_pending_args(x_command_args_t* args,
                           uintptr_t**       args_out,
                           unsigned long**   args_len_out,
                           char***           allocated_strings,
                           int*              allocated_count);

int prepare_x_read_args(x_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

int prepare_x_readgroup_args(x_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count);

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
