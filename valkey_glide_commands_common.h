/*
  +----------------------------------------------------------------------+
  | Valkey Glide Commands Common Framework                               |
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

#ifndef VALKEY_GLIDE_COMMANDS_COMMON_H
#define VALKEY_GLIDE_COMMANDS_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "include/glide/connection_request.pb-c.h"
#include "include/glide_bindings.h"

/* Forward declarations for types defined in glide_bindings.h */
typedef struct CommandResponse    CommandResponse;
typedef struct CommandResult      CommandResult;
typedef struct CommandError       CommandError;
typedef struct ConnectionResponse ConnectionResponse;

enum TLSMode {
    /**
     * No TLS encryption is used for the connection.
     */
    NoTLS = 0,

    /**
     * TLS encryption is used for the connection with certificate verification.
     */
    SecureTLS = 1,

    /**
     * TLS encryption is used for the connection without certificate verification.
     */
    InsecureTLS = 2,
};

/* ClientConfig removed - using valkey_glide_client_configuration_t instead */
/* Forward declaration for ClientAdapter */
typedef struct ClientAdapter ClientAdapter;

/* Function to close a Valkey Glide client */
void close_glide_client(const void* glide_client);
void free_command_response(CommandResponse* command_response_ptr);
void free_command_result(CommandResult* command_result_ptr);

/* Helper functions for Valkey Glide integration */
const ConnectionResponse* create_glide_client(valkey_glide_base_client_configuration_t* config);

const ConnectionResponse* create_glide_cluster_client(
    valkey_glide_cluster_client_configuration_t* config);

/* Return the protobuf message representing the connection request. Caller must free the result with
 * efree() */
uint8_t* create_connection_request(size_t*                                   len,
                                   valkey_glide_base_client_configuration_t* config,
                                   valkey_glide_periodic_checks_status_t     periodic_checks,
                                   bool                                      is_cluster,
                                   bool refresh_topology_from_initial_nodes);

/* Bit operations - UNIFIED SIGNATURES */
int execute_bitcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bitop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bitpos_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* String operations */
int execute_set_command_internal(valkey_glide_object* valkey_glide,
                                 const char*          key,
                                 size_t               key_len,
                                 const char*          val,
                                 size_t               val_len,
                                 long                 expire,
                                 zval*                opts,
                                 char**               old_val,
                                 size_t*              old_val_len,
                                 zval*                return_value);
int execute_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_setex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_psetex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_setnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_get_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Key operations */
int execute_randomkey_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Server operations */
int execute_echo_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ping_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_reset_command(const void* glide_client);
int execute_info_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Additional operations */
int execute_getbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_setbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_del_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_del_array(const void* glide_client,
                      HashTable*  keys_hash,
                      long*       output_value,
                      zval*       return_value);
int execute_unlink_array(const void* glide_client,
                         HashTable*  keys_hash,
                         long*       output_value,
                         zval*       return_value);
int execute_strlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_setrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_getset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_lcs_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Time to live operations */
int execute_ttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Sorted set operations */

/* Hash operations */

int execute_brpoplpush_command(const void* glide_client,
                               const char* src,
                               size_t      src_len,
                               const char* dst,
                               size_t      dst_len,
                               zend_long   timeout,
                               char**      result,
                               size_t*     result_len);

/* Object operations */
int execute_object_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Unified command functions */
int execute_watch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_unwatch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_flushdb_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_flushall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_time_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_scan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_cluster_scan_command(const void* glide_client,
                                 char**      cursor,
                                 const char* pattern,
                                 size_t      pattern_len,
                                 long        count,
                                 int         has_count,
                                 const char* type,
                                 size_t      type_len,
                                 int         has_type,
                                 zval*       return_value);
int execute_sscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_copy_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_hscan_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pfadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pfcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pfmerge_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_client_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_rawcommand_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_dbsize_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_select_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_move_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_echo_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bitop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_getbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_setbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bitcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bitpos_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_touch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_wait_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_config_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_multi_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pipeline_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_discard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_exec_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_fcall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_fcall_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_dump_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_restore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_expire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_expireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pexpire_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pexpireat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_persist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_expiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pexpiretime_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_mset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_msetnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_type_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_append_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_getrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sort_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sort_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

int execute_mget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_rename_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_renamenx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_getdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_getex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_incr_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_incrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_incrbyfloat_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_decr_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_decrby_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_mget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_exists_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_touch_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_unlink_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);


/* ====================================================================
 * METHOD IMPLEMENTATION MACROS
 * ==================================================================== */

#define ECHO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, echo) {                                              \
        if (execute_echo_command(getThis(),                                     \
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

#define BITOP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bitop) {                                              \
        if (execute_bitop_command(getThis(),                                     \
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

#define GETBIT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, getBit) {                                              \
        if (execute_getbit_command(getThis(),                                     \
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

#define SETBIT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, setBit) {                                              \
        if (execute_setbit_command(getThis(),                                     \
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

#define BITCOUNT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bitcount) {                                              \
        if (execute_bitcount_command(getThis(),                                     \
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

#define BITPOS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, bitpos) {                                              \
        if (execute_bitpos_command(getThis(),                                     \
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

/* DEL command needs special handling since it has different signature */
#define DEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, del) {                                              \
        if (execute_del_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

/* Additional unified macros for new converted commands */
#define SELECT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, select) {                                              \
        if (execute_select_command(getThis(),                                     \
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

#define GET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, get) {                                              \
        if (execute_get_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

#define RANDOMKEY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, randomKey) {                                              \
        if (execute_randomkey_command(getThis(),                                     \
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

#define STRLEN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, strlen) {                                              \
        if (execute_strlen_command(getThis(),                                     \
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

#define TTL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ttl) {                                              \
        if (execute_ttl_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

#define PTTL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pttl) {                                              \
        if (execute_pttl_command(getThis(),                                     \
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

#define PING_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ping) {                                              \
        if (execute_ping_command(getThis(),                                     \
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

#define INFO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, info) {                                              \
        if (execute_info_command(getThis(),                                     \
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

/* Additional SET family macros */
#define SETEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, setex) {                                              \
        if (execute_setex_command(getThis(),                                     \
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

#define PSETEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, psetex) {                                              \
        if (execute_psetex_command(getThis(),                                     \
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

#define SETNX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, setnx) {                                              \
        if (execute_setnx_command(getThis(),                                     \
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

#define SETRANGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, setRange) {                                              \
        if (execute_setrange_command(getThis(),                                     \
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

#define GETSET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, getset) {                                              \
        if (execute_getset_command(getThis(),                                     \
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

#define SET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, set) {                                              \
        if (execute_set_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

#define LCS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, lcs) {                                              \
        if (execute_lcs_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

#define WATCH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, watch) {                                              \
        if (execute_watch_command(getThis(),                                     \
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

#define UNWATCH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, unwatch) {                                              \
        if (execute_unwatch_command(getThis(),                                     \
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

#define FLUSHDB_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, flushDB) {                                              \
        if (execute_flushdb_command(getThis(),                                     \
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

#define FLUSHALL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, flushAll) {                                              \
        if (execute_flushall_command(getThis(),                                     \
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

#define TIME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, time) {                                              \
        if (execute_time_command(getThis(),                                     \
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


#define SCAN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, scan) {                                              \
        if (execute_scan_command(getThis(),                                     \
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

#define SSCAN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sscan) {                                              \
        if (execute_sscan_command(getThis(),                                     \
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

#define COPY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, copy) {                                              \
        if (execute_copy_command(getThis(),                                     \
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

#define HSCAN_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, hscan) {                                              \
        if (execute_hscan_command(getThis(),                                     \
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

#define PFADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pfadd) {                                              \
        if (execute_pfadd_command(getThis(),                                     \
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

#define PFCOUNT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pfcount) {                                              \
        if (execute_pfcount_command(getThis(),                                     \
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

#define PFMERGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pfmerge) {                                              \
        if (execute_pfmerge_command(getThis(),                                     \
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

#define CLIENT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, client) {                                              \
        if (execute_client_command(getThis(),                                     \
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

#define RAWCOMMAND_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, rawcommand) {                                              \
        if (execute_rawcommand_command(getThis(),                                     \
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

#define DBSIZE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, dbSize) {                                              \
        if (execute_dbsize_command(getThis(),                                     \
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

#define SELECT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, select) {                                              \
        if (execute_select_command(getThis(),                                     \
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

#define MOVE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, move) {                                              \
        if (execute_move_command(getThis(),                                     \
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

#define DECRBY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, decrBy) {                                              \
        if (execute_decrby_command(getThis(),                                     \
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

#define RENAME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, rename) {                                              \
        if (execute_rename_command(getThis(),                                     \
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

#define RENAMENX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, renameNx) {                                              \
        if (execute_renamenx_command(getThis(),                                     \
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

#define GETDEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, getDel) {                                              \
        if (execute_getdel_command(getThis(),                                     \
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

#define GETEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, getEx) {                                              \
        if (execute_getex_command(getThis(),                                     \
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

#define INCR_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, incr) {                                              \
        if (execute_incr_command(getThis(),                                     \
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

#define INCRBY_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, incrBy) {                                              \
        if (execute_incrby_command(getThis(),                                     \
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

#define INCRBYFLOAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, incrByFloat) {                                              \
        if (execute_incrbyfloat_command(getThis(),                                     \
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

#define DECR_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, decr) {                                              \
        if (execute_decr_command(getThis(),                                     \
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

#define MGET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, mget) {                                              \
        if (execute_mget_command(getThis(),                                     \
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

#define EXISTS_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, exists) {                                              \
        if (execute_exists_command(getThis(),                                     \
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

#define TOUCH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, touch) {                                              \
        if (execute_touch_command(getThis(),                                     \
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

#define UNLINK_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, unlink) {                                              \
        if (execute_unlink_command(getThis(),                                     \
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

#define WAIT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, wait) {                                              \
        if (execute_wait_command(getThis(),                                     \
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

#define CONFIG_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, config) {                                              \
        if (execute_config_command(getThis(),                                     \
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

#define FUNCTION_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, function) {                                              \
        if (execute_function_command(getThis(),                                     \
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

#define MULTI_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, multi) {                                              \
        if (execute_multi_command(getThis(),                                     \
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

#define PIPELINE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pipeline) {                                              \
        if (execute_pipeline_command(getThis(),                                     \
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

#define DISCARD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, discard) {                                              \
        if (execute_discard_command(getThis(),                                     \
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

#define EXEC_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, exec) {                                              \
        if (execute_exec_command(getThis(),                                     \
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

#define FCALL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, fcall) {                                              \
        if (execute_fcall_command(getThis(),                                     \
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

#define FCALL_RO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, fcall_ro) {                                              \
        if (execute_fcall_ro_command(getThis(),                                     \
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

#define DUMP_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, dump) {                                              \
        if (execute_dump_command(getThis(),                                     \
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

#define RESTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, restore) {                                              \
        if (execute_restore_command(getThis(),                                     \
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

#define EXPIRE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, expire) {                                              \
        if (execute_expire_command(getThis(),                                     \
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

#define EXPIREAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, expireAt) {                                              \
        if (execute_expireat_command(getThis(),                                     \
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

#define PEXPIRE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pexpire) {                                              \
        if (execute_pexpire_command(getThis(),                                     \
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

#define PEXPIREAT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pexpireAt) {                                              \
        if (execute_pexpireat_command(getThis(),                                     \
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

#define PERSIST_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, persist) {                                              \
        if (execute_persist_command(getThis(),                                     \
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

#define EXPIRETIME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, expiretime) {                                              \
        if (execute_expiretime_command(getThis(),                                     \
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

#define PEXPIRETIME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, pexpiretime) {                                              \
        if (execute_pexpiretime_command(getThis(),                                     \
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

#define MSET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, mset) {                                              \
        if (execute_mset_command(getThis(),                                     \
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

#define MSETNX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, msetnx) {                                              \
        if (execute_msetnx_command(getThis(),                                     \
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

#define TYPE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, type) {                                              \
        if (execute_type_command(getThis(),                                     \
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

#define APPEND_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, append) {                                              \
        if (execute_append_command(getThis(),                                     \
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

#define GETRANGE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, getRange) {                                              \
        if (execute_getrange_command(getThis(),                                     \
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

#define SORT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sort) {                                              \
        if (execute_sort_command(getThis(),                                     \
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

#define SORT_RO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sort_ro) {                                              \
        if (execute_sort_ro_command(getThis(),                                     \
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


#define OBJECT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, object) {                                              \
        if (execute_object_command(getThis(),                                     \
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


#endif /* VALKEY_GLIDE_COMMANDS_COMMON_H */
