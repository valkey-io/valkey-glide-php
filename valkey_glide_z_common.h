/*
  +----------------------------------------------------------------------+
  | Valkey Glide Z-Commands Common Utilities Header                      |
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

#ifndef VALKEY_GLIDE_Z_COMMON_H
#define VALKEY_GLIDE_Z_COMMON_H

#include "common.h"
#include "include/glide_bindings.h"

/* ====================================================================
 * STRUCTURE DEFINITIONS
 * ==================================================================== */

/**
 * Range command options structure
 * Used for ZRANGE, ZREVRANGE, ZRANGEBYSCORE, etc.
 */
typedef struct {
    int  withscores;   /* WITHSCORES option */
    int  byscore;      /* BYSCORE option */
    int  bylex;        /* BYLEX option */
    int  rev;          /* REV option */
    int  has_limit;    /* Whether LIMIT is specified */
    long limit_offset; /* LIMIT offset value */
    long limit_count;  /* LIMIT count value */
} range_options_t;

/**
 * ZADD command options structure
 */
typedef struct {
    int xx;   /* XX option - only update existing elements */
    int nx;   /* NX option - only add new elements */
    int lt;   /* LT option - only update if new score is less than current */
    int gt;   /* GT option - only update if new score is greater than current */
    int ch;   /* CH option - return number of changed elements */
    int incr; /* INCR option - increment the score */
} zadd_options_t;

/**
 * Store command options structure
 * Used for ZUNIONSTORE, ZINTERSTORE, etc.
 */
typedef struct {
    zval* weights;       /* Weights array */
    int   has_weights;   /* Whether weights are specified */
    zval* aggregate;     /* Aggregate option (SUM, MIN, MAX) */
    int   has_aggregate; /* Whether aggregate is specified */
    int   withscores;    /* WITHSCORES option */
} store_options_t;

/**
 * Generic Z-command arguments structure
 */
typedef struct {
    /* Key arguments */
    const char* key;
    size_t      key_len;

    /* Member arguments */
    const char* member;
    size_t      member_len;

    /* Multiple members */
    zval* members;
    int   member_count;

    /* Range arguments */
    const char* min;
    size_t      min_len;
    const char* max;
    size_t      max_len;

    /* Numeric range arguments */
    long start;
    long end;

    /* Score/increment arguments */
    double score;
    double increment;

    /* Options */
    zval* z_start;
    zval* z_end;
    zval* options;
    zval* weights;

    /* Command-specific flags */
    int withscores;

    /* Result destinations */
    long*   long_result;
    double* double_result;
    zval*   zval_result;
} z_command_args_t;


/* ====================================================================
 * BATCH STATE MANAGEMENT
 * ==================================================================== */

/**
 * Z-command batch state for result processing
 */
typedef struct {
    z_result_processor_t processor;         /* Result processing function */
    void*                output_ptr;        /* Output pointer for results */
    enum RequestType     cmd_type;          /* Command type for reference */
    char**               allocated_strings; /* Strings to free after batch */
    int                  allocated_count;   /* Number of allocated strings */
    int                  withscores;        /* For array result processing */
} z_batch_state_t;

/* ====================================================================
 * COMMON EXECUTION FRAMEWORK
 * ==================================================================== */

/**
 * Generic Z-command execution framework with integrated batch support
 */
int execute_z_generic_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              z_command_args_t*    args,
                              void*                result_ptr,
                              z_result_processor_t process_result,
                              zval*                return_value);

/**
 * Process Z-command batch results
 */
int process_z_batch_results(valkey_glide_object* valkey_glide,
                            CommandResult*       batch_result,
                            zval*                return_value);

/**
 * Process integer result (for commands returning count)
 */
int process_z_int_result(CommandResponse* response, void* output, zval* return_value);

/**
 * Process double result (for commands returning scores)
 */
int process_z_double_result(CommandResponse* response, void* output, zval* return_value);

/**
 * Process array result (for commands returning arrays)
 */
int process_z_array_result(CommandResponse* response, void* output, zval* return_value);

int process_z_array_zrand_result(CommandResponse* response, void* output, zval* return_value);

int process_z_long_to_zval_result(CommandResponse* response, void* output, zval* return_value);

/**
 * Process rank result with optional score
 */
int process_z_rank_result(CommandResponse* response, void* output, zval* return_value);

/**
 * Process BZPOP result (for BZPOPMAX/BZPOPMIN commands)
 */
int process_z_bzpop_result(CommandResponse* response, void* output, zval* return_value);

/* ====================================================================
 * ARGUMENT PREPARATION UTILITIES
 * ==================================================================== */

/**
 * Prepare basic Z-command arguments (just key)
 */
int prepare_z_key_args(z_command_args_t* args, uintptr_t** args_out, unsigned long** args_len_out);

/**
 * Prepare member-based Z-command arguments (key + member)
 */
int prepare_z_member_args(z_command_args_t* args,
                          uintptr_t**       args_out,
                          unsigned long**   args_len_out);

/**
 * Prepare range-based Z-command arguments (key + min + max)
 */
int prepare_z_range_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out);

/**
 * Prepare multi-member Z-command arguments (key + multiple members)
 */
int prepare_z_members_args(z_command_args_t* args,
                           uintptr_t**       args_out,
                           unsigned long**   args_len_out,
                           char***           allocated_strings,
                           int*              allocated_count);

/**
 * Prepare complex range Z-command arguments with options
 */
int prepare_z_complex_range_args(z_command_args_t* args,
                                 uintptr_t**       args_out,
                                 enum RequestType  cmd_type,
                                 unsigned long**   args_len_out,
                                 char***           allocated_strings,
                                 int*              allocated_count);

/**
 * Prepare store command arguments (destination + numkeys + keys + weights + aggregate)
 */
int prepare_z_store_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

/**
 * Prepare ZINTERCARD command arguments (numkeys + keys + optional LIMIT)
 */
int prepare_z_intercard_args(z_command_args_t* args,
                             uintptr_t**       args_out,
                             unsigned long**   args_len_out,
                             char***           allocated_strings,
                             int*              allocated_count);

/**
 * Prepare ZUNION command arguments (numkeys + keys + WEIGHTS + AGGREGATE + WITHSCORES)
 */
int prepare_z_union_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

/**
 * Prepare ZPOP command arguments (key + optional count)
 */
int prepare_z_pop_args(z_command_args_t* args,
                       uintptr_t**       args_out,
                       unsigned long**   args_len_out,
                       char***           allocated_strings,
                       int*              allocated_count);

/**
 * Prepare ZRANGESTORE command arguments (dst + src + start + end + range options)
 */
int prepare_z_rangestore_args(z_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out,
                              char***           allocated_strings,
                              int*              allocated_count);

/**
 * Prepare ZADD command arguments (key + options + score-member pairs)
 */
int prepare_z_zadd_args(z_command_args_t* args,
                        uintptr_t**       args_out,
                        unsigned long**   args_len_out,
                        char***           allocated_strings,
                        int*              allocated_count);

/**
 * Prepare ZDIFF command arguments (numkeys + keys + optional WITHSCORES)
 */
int prepare_z_zdiff_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

/**
 * Prepare ZRANDMEMBER command arguments (key + optional count + optional WITHSCORES)
 */
int prepare_z_randmember_args(z_command_args_t* args,
                              uintptr_t**       args_out,
                              unsigned long**   args_len_out,
                              char***           allocated_strings,
                              int*              allocated_count);

/**
 * Prepare BZPOP command arguments (keys + timeout)
 */
int prepare_z_bzpop_args(z_command_args_t* args,
                         uintptr_t**       args_out,
                         unsigned long**   args_len_out,
                         char***           allocated_strings,
                         int*              allocated_count);

/* ====================================================================
 * OPTIONS PARSING HELPERS
 * ==================================================================== */

/**
 * Parse range command options (withscores, byscore, bylex, rev, limit)
 * Returns 1 on success, 0 on failure
 */
int parse_range_options(zval* options, range_options_t* opts);

/**
 * Parse ZADD command options (XX, NX, LT, GT, CH, INCR)
 * Returns 1 on success, 0 on failure
 */
int parse_zadd_options(zval* options, zadd_options_t* opts);

/**
 * Parse store command options (weights, aggregate) for ZUNIONSTORE-style commands
 * Returns 1 on success, 0 on failure
 */
int parse_store_options(zval* weights, zval* options, store_options_t* opts);

/* ====================================================================
 * CONVERSION & UTILITY HELPERS
 * ==================================================================== */

/**
 * Create LIMIT arguments (offset, count)
 * Returns number of arguments added (0 or 3)
 */
int create_limit_args(range_options_t* opts,
                      uintptr_t*       args,
                      unsigned long*   args_len,
                      int              start_idx,
                      char**           allocated_strings,
                      int*             allocated_count);

/* ====================================================================
 * RESPONSE PROCESSING HELPERS
 * ==================================================================== */

/**
 * Flatten withscores array from [[member, score]] to [member => score]
 * Returns 1 on success, 0 on failure
 */
int flatten_withscores_array(zval* return_value);

int prepare_mpop_arguments(const void*     glide_client,
                           int             is_blocking,
                           double          timeout,
                           zval*           keys,
                           const char*     from,
                           size_t          from_len,
                           long            count,
                           unsigned long*  arg_count_ptr,
                           uintptr_t**     args_ptr,
                           unsigned long** args_len_ptr,
                           char**          numkeys_str_ptr,
                           char**          timeout_str_ptr,
                           char**          count_str_ptr);
/* ====================================================================
 * Z COMMAND IMPLEMENTATION FUNCTIONS (THIN WRAPPERS)
 * ==================================================================== */

/* Traditional function signatures (original) */
int execute_zrandmember_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zscore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zmscore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zrank_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zrevrank_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zincrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zlexcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zrem_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zremrangebylex_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_zremrangebyrank_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce);
int execute_zrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zcard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
/* ZADD command with options */
int execute_zadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* ZRANGE command family */
int execute_zrangestore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_zdiffstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zinterstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zmpop_command_internal(valkey_glide_object* valkey_glide,
                                   zval*                object,
                                   const char*          cmd,
                                   double               timeout,
                                   zval*                keys,
                                   const char*          from,
                                   size_t               from_len,
                                   long                 count,
                                   zval*                return_value);
int execute_zintercard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zunion_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zunionstore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zpopmax_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zpopmin_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_zrangebyscore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zrevrangebyscore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce);
int execute_zrangebylex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_zdiff_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zinter_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zremrangebyscore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce);
int execute_bzmpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_zmpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bzpopmax_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bzpopmin_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int buffer_command_for_batch(valkey_glide_object* valkey_glide,
                             enum RequestType     cmd_type,
                             const uintptr_t*     args,
                             const unsigned long* args_len,
                             uintptr_t            arg_count,
                             void*                result_ptr,
                             z_result_processor_t process_result);
/**
 * Initialize array return value and check for allocation success
 */
#define INIT_RETURN_ARRAY(return_value) \
    do {                                \
        array_init(return_value);       \
    } while (0)

/**
 * Clean up and return FALSE on failure
 */
#define CLEANUP_AND_RETURN_FALSE(return_value)                    \
    do {                                                          \
        if (return_value && Z_TYPE_P(return_value) == IS_ARRAY) { \
            zval_dtor(return_value);                              \
        }                                                         \
        RETURN_FALSE;                                             \
    } while (0)

/* Ultra-simple macro for ZRANDMEMBER method implementation */
#define ZRANDMEMBER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRandMember) {                                              \
        if (execute_zrandmember_command(getThis(),                                     \
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

/* Ultra-simple macro for ZRANGE method implementation */
#define ZRANGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRange) {                                              \
        if (execute_zrange_command(getThis(),                                     \
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

/* Ultra-simple macro for ZRANGESTORE method implementation */
#define ZRANGESTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zrangestore) {                                              \
        if (execute_zrangestore_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        RETURN_FALSE;                                                                  \
    }


/* Ultra-simple macro for ZRANGEBYSCORE method implementation */
#define ZRANGEBYSCORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRangeByScore) {                                              \
        if (execute_zrangebyscore_command(getThis(),                                     \
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

/* Ultra-simple macro for ZREVRANGEBYSCORE method implementation */
#define ZREVRANGEBYSCORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRevRangeByScore) {                                              \
        if (execute_zrevrangebyscore_command(getThis(),                                     \
                                             ZEND_NUM_ARGS(),                               \
                                             return_value,                                  \
                                             strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                 ? get_valkey_glide_cluster_ce()            \
                                                 : get_valkey_glide_ce())) {                \
            return;                                                                         \
        }                                                                                   \
        zval_dtor(return_value);                                                            \
        RETURN_FALSE;                                                                       \
    }

/* Ultra-simple macro for ZRANGEBYLEX method implementation */
#define ZRANGEBYLEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRangeByLex) {                                              \
        if (execute_zrangebylex_command(getThis(),                                     \
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


/* Ultra-simple macro for ZREMRANGEBYLEX method implementation */
#define ZREMRANGEBYLEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRemRangeByLex) {                                              \
        if (execute_zremrangebylex_command(getThis(),                                     \
                                           ZEND_NUM_ARGS(),                               \
                                           return_value,                                  \
                                           strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                               ? get_valkey_glide_cluster_ce()            \
                                               : get_valkey_glide_ce())) {                \
            return;                                                                       \
        }                                                                                 \
        RETURN_FALSE;                                                                     \
    }

/* Ultra-simple macro for ZREM method implementation */
#define ZREM_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRem) {                                              \
        if (execute_zrem_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            return;                                                             \
        }                                                                       \
        RETURN_FALSE;                                                           \
    }

/* Ultra-simple macro for ZREMRANGEBYSCORE method implementation */
#define ZREMRANGEBYSCORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRemRangeByScore) {                                              \
        if (execute_zremrangebyscore_command(getThis(),                                     \
                                             ZEND_NUM_ARGS(),                               \
                                             return_value,                                  \
                                             strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                 ? get_valkey_glide_cluster_ce()            \
                                                 : get_valkey_glide_ce())) {                \
            return;                                                                         \
        }                                                                                   \
        RETURN_FALSE;                                                                       \
    }

/* Ultra-simple macro for ZREVRANK method implementation */
#define ZREVRANK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRevRank) {                                              \
        if (execute_zrevrank_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        RETURN_FALSE;                                                               \
    }

/* Ultra-simple macro for ZREMRANGEBYRANK method implementation */
#define ZREMRANGEBYRANK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRemRangeByRank) {                                              \
        if (execute_zremrangebyrank_command(getThis(),                                     \
                                            ZEND_NUM_ARGS(),                               \
                                            return_value,                                  \
                                            strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                ? get_valkey_glide_cluster_ce()            \
                                                : get_valkey_glide_ce())) {                \
            return;                                                                        \
        }                                                                                  \
        RETURN_FALSE;                                                                      \
    }

/* Ultra-simple macro for ZCOUNT method implementation */
#define ZCOUNT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zCount) {                                              \
        if (execute_zcount_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            return;                                                               \
        }                                                                         \
        RETURN_FALSE;                                                             \
    }

/* Ultra-simple macro for ZCARD method implementation */
#define ZCARD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zCard) {                                              \
        if (execute_zcard_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        RETURN_FALSE;                                                            \
    }

/* Ultra-simple macro for ZSCORE method implementation */
#define ZSCORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zScore) {                                              \
        if (execute_zscore_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            return;                                                               \
        }                                                                         \
        RETURN_FALSE;                                                             \
    }

/* Ultra-simple macro for ZMSCORE method implementation */
#define ZMSCORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zMscore) {                                              \
        if (execute_zmscore_command(getThis(),                                     \
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

/* Ultra-simple macro for ZRANK method implementation */
#define ZRANK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zRank) {                                              \
        if (execute_zrank_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        RETURN_FALSE;                                                            \
    }

/* Ultra-simple macro for ZINCRBY method implementation */
#define ZINCRBY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zIncrBy) {                                              \
        if (execute_zincrby_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            return;                                                                \
        }                                                                          \
        RETURN_FALSE;                                                              \
    }

/* Ultra-simple macro for ZINTER method implementation */
#define ZINTER_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zinter) {                                              \
        if (execute_zinter_command(getThis(),                                     \
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

/* Ultra-simple macro for ZINTERCARD method implementation */
#define ZINTERCARD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zintercard) {                                              \
        if (execute_zintercard_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        RETURN_FALSE;                                                                 \
    }

/* Ultra-simple macro for ZUNION method implementation */
#define ZUNION_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zunion) {                                              \
        if (execute_zunion_command(getThis(),                                     \
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

/* Ultra-simple macro for ZDIFFSTORE method implementation */
#define ZDIFFSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zdiffstore) {                                              \
        if (execute_zdiffstore_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        RETURN_FALSE;                                                                 \
    }

/* Ultra-simple macro for ZINTERSTORE method implementation */
#define ZINTERSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zinterstore) {                                              \
        if (execute_zinterstore_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        RETURN_FALSE;                                                                  \
    }

/* Ultra-simple macro for ZUNIONSTORE method implementation */
#define ZUNIONSTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zunionstore) {                                              \
        if (execute_zunionstore_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        RETURN_FALSE;                                                                  \
    }

/* Ultra-simple macro for ZPOPMAX method implementation */
#define ZPOPMAX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zPopMax) {                                              \
        if (execute_zpopmax_command(getThis(),                                     \
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

/* Ultra-simple macro for ZPOPMIN method implementation */
#define ZPOPMIN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zPopMin) {                                              \
        if (execute_zpopmin_command(getThis(),                                     \
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

/* Ultra-simple macro for ZSCAN method implementation */
#define ZSCAN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zscan) {                                              \
        if (execute_zscan_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        RETURN_FALSE;                                                            \
    }

/* Ultra-simple macro for BZMPOP method implementation */
#define BZMPOP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bzmpop) {                                              \
        if (execute_bzmpop_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            return;                                                               \
        }                                                                         \
        RETURN_FALSE;                                                             \
    }

/* Ultra-simple macro for ZMPOP method implementation */
#define ZMPOP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zmpop) {                                              \
        if (execute_zmpop_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            return;                                                              \
        }                                                                        \
        RETURN_FALSE;                                                            \
    }

/* Ultra-simple macro for ZADD method implementation */
#define ZADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zAdd) {                                              \
        if (execute_zadd_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            return;                                                             \
        }                                                                       \
        RETURN_FALSE;                                                           \
    }

/* Ultra-simple macro for ZLEXCOUNT method implementation */
#define ZLEXCOUNT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zLexCount) {                                              \
        if (execute_zlexcount_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        RETURN_FALSE;                                                                \
    }

/* Ultra-simple macro for ZDIFF method implementation */
#define ZDIFF_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, zdiff) {                                              \
        if (execute_zdiff_command(getThis(),                                     \
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

/* Ultra-simple macro for BZPOPMAX method implementation */
#define BZPOPMAX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bzPopMax) {                                              \
        if (execute_bzpopmax_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        RETURN_FALSE;                                                               \
    }

/* Ultra-simple macro for BZPOPMIN method implementation */
#define BZPOPMIN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bzPopMin) {                                              \
        if (execute_bzpopmin_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        RETURN_FALSE;                                                               \
    }

#endif /* VALKEY_GLIDE_Z_COMMON_H */
