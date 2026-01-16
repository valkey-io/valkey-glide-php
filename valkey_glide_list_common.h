/*
  +----------------------------------------------------------------------+
  | ValkeyGlide Glide List Commands Common Framework                           |
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

#ifndef VALKEY_GLIDE_LIST_COMMON_H
#define VALKEY_GLIDE_LIST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * STRUCTURES
 * ==================================================================== */

/**
 * Options for blocking list commands (BLPOP, BRPOP, etc.)
 */
typedef struct _list_blocking_options_t {
    double timeout;     /* Timeout in seconds */
    int    has_timeout; /* Whether timeout is set */
} list_blocking_options_t;

/**
 * Options for range-based list commands (LRANGE, LTRIM)
 */
typedef struct _list_range_options_t {
    long start;     /* Start index */
    long end;       /* End index */
    int  has_range; /* Whether range is set */
} list_range_options_t;

/**
 * Options for list position commands (LINSERT, LPOS)
 */
typedef struct _list_position_options_t {
    const char* position;     /* Position string (BEFORE/AFTER/LEFT/RIGHT) */
    const char* pivot;        /* Pivot element for LINSERT */
    size_t      position_len; /* Position string length */
    size_t      pivot_len;    /* Pivot element length */
    long        rank;         /* Rank for LPOS */
    long        count;        /* Count for LPOS */
    long        maxlen;       /* Max length for LPOS */
    int         has_rank;     /* Whether rank is set */
    int         has_count;    /* Whether count is set */
    int         has_maxlen;   /* Whether maxlen is set */
} list_position_options_t;

/**
 * Options for list move commands (LMOVE, BLMOVE, RPOPLPUSH, BRPOPLPUSH)
 */
typedef struct _list_move_options_t {
    const char* source_direction;     /* Source direction (LEFT/RIGHT) */
    const char* dest_direction;       /* Destination direction (LEFT/RIGHT) */
    const char* dest_key;             /* Destination key */
    size_t      source_direction_len; /* Source direction length */
    size_t      dest_direction_len;   /* Destination direction length */
    size_t      dest_key_len;         /* Destination key length */
    double      timeout;              /* Timeout for blocking commands */
    int         has_timeout;          /* Whether timeout is set */
} list_move_options_t;

/**
 * Options for MPOP commands (LMPOP, BLMPOP)
 */
typedef struct _list_mpop_options_t {
    const char* direction;     /* Direction (LEFT/RIGHT) */
    size_t      direction_len; /* Direction length */
    double      timeout;       /* Timeout for blocking version */
    long        count;         /* Number of elements to pop */
    int         has_count;     /* Whether count is set */
    int         has_timeout;   /* Whether timeout is set */
} list_mpop_options_t;

/**
 * Generic command arguments structure for list commands
 */
typedef struct _list_command_args_t {
    const void*             glide_client;  /* GlideClient instance */
    const char*             key;           /* Primary key argument */
    zval*                   keys;          /* Array of keys */
    zval*                   values;        /* Array of values to push/insert */
    const char*             element;       /* Single element value */
    const char*             value;         /* Single value string */
    zval*                   options;       /* Raw options array from PHP */
    size_t                  key_len;       /* Primary key length */
    size_t                  element_len;   /* Single element length */
    size_t                  value_len;     /* Single value length */
    long                    count;         /* Count for pop operations */
    long                    index;         /* Index for LINDEX, LSET */
    long                    start;         /* Start index for range operations */
    long                    end;           /* End index for range operations */
    list_position_options_t position_opts; /* Position command options */
    list_move_options_t     move_opts;     /* Move command options */
    list_mpop_options_t     mpop_opts;     /* MPOP command options */
    list_range_options_t    range_opts;    /* Range command options */
    list_blocking_options_t blocking_opts; /* Blocking command options */
    int                     key_count;     /* Number of keys */
    int                     value_count;   /* Number of values */
} list_command_args_t;

/* Function pointer types */
typedef int (*z_result_processor_t)(CommandResponse* response, void* output, zval* return_value);
typedef int (*list_arg_preparation_func_t)(list_command_args_t* args,
                                           uintptr_t**          args_out,
                                           unsigned long**      args_len_out,
                                           char***              allocated_strings,
                                           int*                 allocated_count);

/* ====================================================================
 * FUNCTION DECLARATIONS
 * ==================================================================== */

/* Utility functions */
int   allocate_list_command_args(int count, uintptr_t** args_out, unsigned long** args_len_out);
void  free_list_command_args(uintptr_t* args, unsigned long* args_len);
char* alloc_list_number_string(long value, size_t* len_out);
char* alloc_list_double_string(double value, size_t* len_out);

/* Generic command execution framework */
int execute_list_generic_command(valkey_glide_object* valkey_glide,
                                 enum RequestType     cmd_type,
                                 list_command_args_t* args,
                                 void*                result_ptr,
                                 z_result_processor_t process_result,
                                 zval*                return_value);

/* Option parsing functions */
int parse_list_position_options(zval* options, list_position_options_t* opts);


/* Argument preparation functions */
int prepare_list_key_only_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out);

int prepare_list_key_values_args(list_command_args_t* args,
                                 uintptr_t**          args_out,
                                 unsigned long**      args_len_out,
                                 char***              allocated_strings,
                                 int*                 allocated_count);

int prepare_list_key_count_args(list_command_args_t* args,
                                uintptr_t**          args_out,
                                unsigned long**      args_len_out,
                                char***              allocated_strings,
                                int*                 allocated_count);

int prepare_list_blocking_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out,
                               char***              allocated_strings,
                               int*                 allocated_count);

int prepare_list_range_args(list_command_args_t* args,
                            uintptr_t**          args_out,
                            unsigned long**      args_len_out,
                            char***              allocated_strings,
                            int*                 allocated_count);

int prepare_list_position_args(list_command_args_t* args,
                               uintptr_t**          args_out,
                               unsigned long**      args_len_out,
                               char***              allocated_strings,
                               int*                 allocated_count);

int prepare_list_move_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count);

int prepare_list_mpop_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count);

int prepare_list_insert_args(list_command_args_t* args,
                             uintptr_t**          args_out,
                             unsigned long**      args_len_out);

int prepare_list_index_set_args(list_command_args_t* args,
                                uintptr_t**          args_out,
                                unsigned long**      args_len_out,
                                char***              allocated_strings,
                                int*                 allocated_count);

int prepare_list_rem_args(list_command_args_t* args,
                          uintptr_t**          args_out,
                          unsigned long**      args_len_out,
                          char***              allocated_strings,
                          int*                 allocated_count);

int prepare_list_trim_args(list_command_args_t* args,
                           uintptr_t**          args_out,
                           unsigned long**      args_len_out,
                           char***              allocated_strings,
                           int*                 allocated_count);

/* Result processing functions */
int process_list_int_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_string_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_array_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_pop_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_blocking_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_ok_result_async(CommandResponse* response, void* output, zval* return_value);
int process_list_mpop_result_async(CommandResponse* response, void* output, zval* return_value);

/* High-level command execution functions */
int execute_list_push_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce);
int execute_list_move_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce);
int execute_list_pop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce);

int execute_list_blocking_pop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce);

int execute_list_range_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_list_index_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_list_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_list_position_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_list_insert_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_list_rem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_list_len_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_list_trim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_list_mpop_command(
    zval* object, int argc, zval* return_value, enum RequestType cmd_type, zend_class_entry* ce);

/* ====================================================================
 * HELPER MACROS
 * ==================================================================== */

#define INIT_LIST_COMMAND_ARGS(args) memset(&(args), 0, sizeof(list_command_args_t))

#define SET_LIST_KEY(args, k, k_len) \
    do {                             \
        (args).key     = (k);        \
        (args).key_len = (k_len);    \
    } while (0)

#define SET_LIST_VALUES(args, vals, val_count) \
    do {                                       \
        (args).values      = (vals);           \
        (args).value_count = (val_count);      \
    } while (0)

#define SET_LIST_COUNT(args, c) \
    do {                        \
        (args).count = (c);     \
    } while (0)

#define SET_LIST_RANGE(args, s, e)         \
    do {                                   \
        (args).start                = (s); \
        (args).end                  = (e); \
        (args).range_opts.start     = (s); \
        (args).range_opts.end       = (e); \
        (args).range_opts.has_range = 1;   \
    } while (0)

/* Validation macros */
#define VALIDATE_LIST_CLIENT(client) \
    if (!(client)) {                 \
        return 0;                    \
    }

#define VALIDATE_LIST_KEY(key, key_len) \
    if (!(key) || (key_len) <= 0) {     \
        return 0;                       \
    }

#define VALIDATE_LIST_VALUES(values, count) \
    if (!(values) || (count) <= 0) {        \
        return 0;                           \
    }

/* Memory management macros */
#define FREE_LIST_ALLOCATED_STRINGS(strings, count) \
    do {                                            \
        if (strings) {                              \
            for (int _i = 0; _i < (count); _i++) {  \
                if ((strings)[_i]) {                \
                    efree((strings)[_i]);           \
                }                                   \
            }                                       \
            efree(strings);                         \
        }                                           \
    } while (0)

#define CLEANUP_LIST_COMMAND_ARGS(args, args_len, strings, str_count) \
    do {                                                              \
        free_list_command_args((args), (args_len));                   \
        FREE_LIST_ALLOCATED_STRINGS((strings), (str_count));          \
    } while (0)

/* ====================================================================
 * LIST COMMAND MACROS
 * ==================================================================== */

#define LPUSH_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lPush) {                                                  \
        if (execute_list_push_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      LPush,                                         \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define RPUSH_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, rPush) {                                                  \
        if (execute_list_push_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      RPush,                                         \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define LPUSHX_METHOD_IMPL(class_name)                                               \
    PHP_METHOD(class_name, lPushx) {                                                 \
        if (execute_list_push_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      LPushX,                                        \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define RPUSHX_METHOD_IMPL(class_name)                                               \
    PHP_METHOD(class_name, rPushx) {                                                 \
        if (execute_list_push_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      RPushX,                                        \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define LPOP_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lPop) {                                                  \
        if (execute_list_pop_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     LPop,                                          \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#define RPOP_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, rPop) {                                                  \
        if (execute_list_pop_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     RPop,                                          \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#define BLPOP_METHOD_IMPL(class_name)                                                        \
    PHP_METHOD(class_name, blPop) {                                                          \
        if (execute_list_blocking_pop_command(getThis(),                                     \
                                              ZEND_NUM_ARGS(),                               \
                                              return_value,                                  \
                                              BLPop,                                         \
                                              strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                  ? get_valkey_glide_cluster_ce()            \
                                                  : get_valkey_glide_ce())) {                \
            return;                                                                          \
        }                                                                                    \
        zval_dtor(return_value);                                                             \
        RETURN_FALSE;                                                                        \
    }

#define BRPOP_METHOD_IMPL(class_name)                                                        \
    PHP_METHOD(class_name, brPop) {                                                          \
        if (execute_list_blocking_pop_command(getThis(),                                     \
                                              ZEND_NUM_ARGS(),                               \
                                              return_value,                                  \
                                              BRPop,                                         \
                                              strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                  ? get_valkey_glide_cluster_ce()            \
                                                  : get_valkey_glide_ce())) {                \
            return;                                                                          \
        }                                                                                    \
        zval_dtor(return_value);                                                             \
        RETURN_FALSE;                                                                        \
    }

#define LSET_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lSet) {                                                  \
        if (execute_list_set_command(getThis(),                                     \
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

#define LINDEX_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lindex) {                                                  \
        if (execute_list_index_command(getThis(),                                     \
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

#define LTRIM_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, ltrim) {                                                  \
        if (execute_list_trim_command(getThis(),                                     \
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

#define LREM_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lrem) {                                                  \
        if (execute_list_rem_command(getThis(),                                     \
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

#define LMOVE_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lMove) {                                                  \
        if (execute_list_move_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      LMove,                                         \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define BLMOVE_METHOD_IMPL(class_name)                                               \
    PHP_METHOD(class_name, blmove) {                                                 \
        if (execute_list_move_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      BLMove,                                        \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define LLEN_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lLen) {                                                  \
        if (execute_list_len_command(getThis(),                                     \
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

#define LINSERT_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lInsert) {                                                  \
        if (execute_list_insert_command(getThis(),                                     \
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

#define LPOS_METHOD_IMPL(class_name)                                                     \
    PHP_METHOD(class_name, lPos) {                                                       \
        if (execute_list_position_command(getThis(),                                     \
                                          ZEND_NUM_ARGS(),                               \
                                          return_value,                                  \
                                          strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                              ? get_valkey_glide_cluster_ce()            \
                                              : get_valkey_glide_ce())) {                \
            return;                                                                      \
        }                                                                                \
        zval_dtor(return_value);                                                         \
        RETURN_FALSE;                                                                    \
    }

#define LMPOP_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lmpop) {                                                  \
        if (execute_list_mpop_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      LMPop,                                         \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define BLMPOP_METHOD_IMPL(class_name)                                               \
    PHP_METHOD(class_name, blmpop) {                                                 \
        if (execute_list_mpop_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      BLMPop,                                        \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define LRANGE_METHOD_IMPL(class_name)                                                \
    PHP_METHOD(class_name, lrange) {                                                  \
        if (execute_list_range_command(getThis(),                                     \
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

#endif /* VALKEY_GLIDE_LIST_COMMON_H */
