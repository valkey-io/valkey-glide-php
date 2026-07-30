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
#include <zend.h>
#include <zend_exceptions.h>

#include "command_response.h"
#include "common.h"
#include "include/glide/connection_request.pb-c.h"
#include "include/glide_bindings.h"

// Function declarations
char* store_script_and_get_hash(const char* script);

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

/* Constants */
#define SHA1_HASH_LENGTH 40

/* Helper function to check if a string is a valid SHA1 hash */
static inline bool is_sha1_hash(const char* str, size_t len) {
    return len == SHA1_HASH_LENGTH && strspn(str, "0123456789abcdefABCDEF") == SHA1_HASH_LENGTH;
}

/* Helper to throw the correct exception for a command error */
static inline void throw_command_error(CommandError* error) {
    zend_class_entry* exception_ce = get_valkey_glide_exception_ce();
    if (error->command_error_type == VALKEY_GLIDE_ERROR_TYPE_CIRCUIT_BREAKER_OPEN) {
        exception_ce = get_valkey_glide_circuit_breaker_exception_ce();
    }
    zend_throw_exception(exception_ce, error->command_error_message, 0);
}

/* Helper function to handle command result with consistent error handling */
static inline void handle_command_result_or_throw(CommandResult* result,
                                                  const char*    command_name,
                                                  zval*          return_value) {
    if (!result) {
        char* error_msg;
        spprintf(&error_msg, 0, "%s: Failed to execute command", command_name);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
        efree(error_msg);
        return;
    }
    if (result->command_error) {
        throw_command_error(result->command_error);
        free_command_result(result);
        return;
    }
    if (!result->response) {
        char* error_msg = emalloc(strlen(command_name) + 25);
        sprintf(error_msg, "%s: No response received", command_name);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
        efree(error_msg);
        free_command_result(result);
        return;
    }
    command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
}

// Helper that returns false on errors instead of throwing exceptions
static inline int handle_function_command_result_or_return_false(valkey_glide_object* valkey_glide,
                                                                 CommandResult*       result,
                                                                 zval* return_value) {
    if (!result || result->command_error || !result->response) {
        valkey_glide_record_command_error(valkey_glide, result);
        ZVAL_FALSE(return_value);
        if (result) {
            free_command_result(result);
        }
        return 0;
    }
    int status = command_response_to_zval(result->response, return_value, 0, false);
    free_command_result(result);
    return status;
}

/* ClientConfig removed - using valkey_glide_client_configuration_t instead */
/* Forward declaration for ClientAdapter */
typedef struct ClientAdapter ClientAdapter;

/* Function to close a Valkey Glide client */
void close_glide_client(const void* glide_client);
void free_command_response(CommandResponse* command_response_ptr);
void free_command_result(CommandResult* command_result_ptr);

/* Helper functions for Valkey Glide integration */
const ConnectionResponse* create_glide_client(valkey_glide_base_client_configuration_t* config,
                                              AddressResolverCallback* out_resolver_cb);

const ConnectionResponse* create_glide_cluster_client(
    valkey_glide_cluster_client_configuration_t* config, AddressResolverCallback* out_resolver_cb);

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
int execute_bgsave_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_bgrewriteaof_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_migrate_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_client_pause_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_client_unpause_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_reset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_save_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
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
int execute_multi_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pipeline_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_discard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_exec_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_fcall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_fcall_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* Forward declarations for script command functions */
void execute_script_flush_command(zval* object, zval* return_value, bool is_cluster);
void execute_script_exists_command(zval* object, zval* sha1s, zval* return_value, bool is_cluster);
void execute_script_show_command(
    zval* object, char* sha1, size_t sha1_len, zval* return_value, bool is_cluster);
void  execute_script_kill_command(zval* object, zval* return_value, bool is_cluster);
char* store_script_and_get_hash(const char* script);
void  execute_eval_command(zval* object, int argc, zval* return_value, bool is_cluster);
void  execute_evalsha_command(zval* object, int argc, zval* return_value, bool is_cluster);
void  execute_eval_ro_command(zval* object, int argc, zval* return_value, bool is_cluster);
void  execute_evalsha_ro_command(zval* object, int argc, zval* return_value, bool is_cluster);
int   execute_function_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_load_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_load_internal(valkey_glide_object* valkey_glide,
                                   char*                library_code,
                                   size_t               library_code_len,
                                   zend_bool            replace,
                                   zval*                return_value);
int execute_function_list_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_flush_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_function_delete_command(zval*             object,
                                    int               argc,
                                    zval*             return_value,
                                    zend_class_entry* ce);
int execute_function_dump_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_restore_command(zval*             object,
                                     int               argc,
                                     zval*             return_value,
                                     zend_class_entry* ce);
int execute_function_kill_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_function_stats_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
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

/* Generic helper for commands that follow the standard pattern:
 * call execute function, return on success, RETURN_FALSE on failure. */
#define STANDARD_METHOD_IMPL(class_name, method_name, execute_fn)     \
    PHP_METHOD(class_name, method_name) {                             \
        if (execute_fn(getThis(),                                     \
                       ZEND_NUM_ARGS(),                               \
                       return_value,                                  \
                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                           ? get_valkey_glide_cluster_ce()            \
                           : get_valkey_glide_ce())) {                \
            return;                                                   \
        }                                                             \
        zval_dtor(return_value);                                      \
        RETURN_FALSE;                                                 \
    }

#define ECHO_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, echo, execute_echo_command)

#define BITOP_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, bitop, execute_bitop_command)

#define GETBIT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, getBit, execute_getbit_command)

#define SETBIT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, setBit, execute_setbit_command)

#define BITCOUNT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, bitcount, execute_bitcount_command)

#define BITPOS_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, bitpos, execute_bitpos_command)

/* DEL command needs special handling since it has different signature */
#define DEL_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, del, execute_del_command)

/* Additional unified macros for new converted commands */
#define SELECT_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, select) {                                              \
        if (execute_select_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                    \
            return;                                                               \
        }                                                                         \
        zval_dtor(return_value);                                                  \
        RETURN_FALSE;                                                             \
    }

#define GET_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, get, execute_get_command)

#define RANDOMKEY_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, randomKey, execute_randomkey_command)

#define STRLEN_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, strlen, execute_strlen_command)

#define TTL_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, ttl, execute_ttl_command)

#define PTTL_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, pttl, execute_pttl_command)

#define PING_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, ping, execute_ping_command)

#define INFO_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, info, execute_info_command)

/* Additional SET family macros */
#define SETEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, setex) {                                              \
        if (execute_setex_command(getThis(),                                     \
                                  ZEND_NUM_ARGS(),                               \
                                  return_value,                                  \
                                  strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                      ? get_valkey_glide_cluster_ce()            \
                                      : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                   \
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
            APPLY_REPLY_LITERAL(return_value);                                    \
            return;                                                               \
        }                                                                         \
        zval_dtor(return_value);                                                  \
        RETURN_FALSE;                                                             \
    }

#define SETNX_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, setnx, execute_setnx_command)

#define SETRANGE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, setRange, execute_setrange_command)

#define GETSET_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, getset, execute_getset_command)

#define SET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, set) {                                              \
        if (execute_set_command(getThis(),                                     \
                                ZEND_NUM_ARGS(),                               \
                                return_value,                                  \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                    ? get_valkey_glide_cluster_ce()            \
                                    : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                 \
            return;                                                            \
        }                                                                      \
        zval_dtor(return_value);                                               \
        RETURN_FALSE;                                                          \
    }

#define LCS_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, lcs, execute_lcs_command)

#define WATCH_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, watch, execute_watch_command)

#define UNWATCH_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, unwatch, execute_unwatch_command)

#define FLUSHDB_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, flushDB) {                                              \
        if (execute_flushdb_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                     \
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
            APPLY_REPLY_LITERAL(return_value);                                      \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#define MIGRATE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, migrate, execute_migrate_command)

#define BGSAVE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, bgSave, execute_bgsave_command)

#define BGREWRITEAOF_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, bgRewriteAof, execute_bgrewriteaof_command)

#define SAVE_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, save, execute_save_command)

#define RESET_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, reset, execute_reset_command)

#define CLIENT_PAUSE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, clientPause, execute_client_pause_command)

#define CLIENT_UNPAUSE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, clientUnpause, execute_client_unpause_command)

#define TIME_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, time, execute_time_command)

#define SCAN_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, scan, execute_scan_command)

#define SSCAN_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, sscan, execute_sscan_command)

#define COPY_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, copy, execute_copy_command)

#define HSCAN_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, hscan, execute_hscan_command)

#define PFADD_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, pfadd, execute_pfadd_command)

#define PFCOUNT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pfcount, execute_pfcount_command)

#define PFMERGE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pfmerge, execute_pfmerge_command)

#define CLIENT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, client, execute_client_command)

#define RAWCOMMAND_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, rawcommand, execute_rawcommand_command)

#define DBSIZE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, dbSize, execute_dbsize_command)

#define MOVE_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, move, execute_move_command)

#define DECRBY_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, decrBy, execute_decrby_command)

#define RENAME_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, rename) {                                              \
        if (execute_rename_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                    \
            return;                                                               \
        }                                                                         \
        zval_dtor(return_value);                                                  \
        RETURN_FALSE;                                                             \
    }

#define RENAMENX_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, renameNx, execute_renamenx_command)

#define GETDEL_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, getDel, execute_getdel_command)

#define GETEX_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, getEx, execute_getex_command)

#define INCR_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, incr, execute_incr_command)

#define INCRBY_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, incrBy, execute_incrby_command)

#define INCRBYFLOAT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, incrByFloat, execute_incrbyfloat_command)

#define DECR_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, decr, execute_decr_command)

#define MGET_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, mget, execute_mget_command)

#define EXISTS_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, exists, execute_exists_command)

#define TOUCH_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, touch, execute_touch_command)

#define UNLINK_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, unlink, execute_unlink_command)

#define WAIT_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, wait, execute_wait_command)

#define CONFIG_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, config) {                                              \
        if (execute_config_command(getThis(),                                     \
                                   ZEND_NUM_ARGS(),                               \
                                   return_value,                                  \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                       ? get_valkey_glide_cluster_ce()            \
                                       : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                    \
            return;                                                               \
        }                                                                         \
        zval_dtor(return_value);                                                  \
        RETURN_FALSE;                                                             \
    }

#define FUNCTION_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, function, execute_function_command)

#define SCRIPT_METHOD_IMPL(class_name)                                        \
    PHP_METHOD(class_name, script) {                                          \
        execute_script_command(getThis(),                                     \
                               ZEND_NUM_ARGS(),                               \
                               return_value,                                  \
                               strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                   ? get_valkey_glide_cluster_ce()            \
                                   : get_valkey_glide_ce());                  \
    }

#define EVAL_METHOD_IMPL(class_name)                                          \
    PHP_METHOD(class_name, eval) {                                            \
        execute_eval_command(getThis(),                                       \
                             ZEND_NUM_ARGS(),                                 \
                             return_value,                                    \
                             strcmp(#class_name, "ValkeyGlideCluster") == 0); \
    }

#define EVALSHA_METHOD_IMPL(class_name)                                          \
    PHP_METHOD(class_name, evalsha) {                                            \
        execute_evalsha_command(getThis(),                                       \
                                ZEND_NUM_ARGS(),                                 \
                                return_value,                                    \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0); \
    }

#define EVAL_RO_METHOD_IMPL(class_name)                                          \
    PHP_METHOD(class_name, eval_ro) {                                            \
        execute_eval_ro_command(getThis(),                                       \
                                ZEND_NUM_ARGS(),                                 \
                                return_value,                                    \
                                strcmp(#class_name, "ValkeyGlideCluster") == 0); \
    }

#define EVALSHA_RO_METHOD_IMPL(class_name)                                          \
    PHP_METHOD(class_name, evalsha_ro) {                                            \
        execute_evalsha_ro_command(getThis(),                                       \
                                   ZEND_NUM_ARGS(),                                 \
                                   return_value,                                    \
                                   strcmp(#class_name, "ValkeyGlideCluster") == 0); \
    }

#define MULTI_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, multi, execute_multi_command)

#define PIPELINE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pipeline, execute_pipeline_command)

#define DISCARD_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, discard, execute_discard_command)

#define EXEC_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, exec, execute_exec_command)

#define FCALL_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, fcall, execute_fcall_command)

#define FCALL_RO_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, fcall_ro, execute_fcall_ro_command)

#define SCRIPT_EXISTS_METHOD_IMPL(class_name)                                 \
    PHP_METHOD(class_name, scriptExists) {                                    \
        zval* sha1s;                                                          \
        ZEND_PARSE_PARAMETERS_START(1, 1)                                     \
        Z_PARAM_ARRAY(sha1s)                                                  \
        ZEND_PARSE_PARAMETERS_END();                                          \
        execute_script_exists_command(getThis(), sha1s, return_value, false); \
    }

#define SCRIPT_SHOW_METHOD_IMPL(class_name)                                          \
    PHP_METHOD(class_name, scriptShow) {                                             \
        char*  sha1;                                                                 \
        size_t sha1_len;                                                             \
        ZEND_PARSE_PARAMETERS_START(1, 1)                                            \
        Z_PARAM_STRING(sha1, sha1_len)                                               \
        ZEND_PARSE_PARAMETERS_END();                                                 \
        execute_script_show_command(getThis(), sha1, sha1_len, return_value, false); \
    }

#define SCRIPT_KILL_METHOD_IMPL(class_name)                          \
    PHP_METHOD(class_name, scriptKill) {                             \
        execute_script_kill_command(getThis(), return_value, false); \
    }

#define SCRIPT_FLUSH_METHOD_IMPL(class_name)                          \
    PHP_METHOD(class_name, scriptFlush) {                             \
        execute_script_flush_command(getThis(), return_value, false); \
    }

#define DUMP_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, dump, execute_dump_command)

#define RESTORE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, restore) {                                              \
        if (execute_restore_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                     \
            return;                                                                \
        }                                                                          \
        zval_dtor(return_value);                                                   \
        RETURN_FALSE;                                                              \
    }

#define EXPIRE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, expire, execute_expire_command)

#define EXPIREAT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, expireAt, execute_expireat_command)

#define PEXPIRE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pexpire, execute_pexpire_command)

#define PEXPIREAT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pexpireAt, execute_pexpireat_command)

#define PERSIST_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, persist, execute_persist_command)

#define EXPIRETIME_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, expiretime, execute_expiretime_command)

#define PEXPIRETIME_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, pexpiretime, execute_pexpiretime_command)

#define MSET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, mset) {                                              \
        if (execute_mset_command(getThis(),                                     \
                                 ZEND_NUM_ARGS(),                               \
                                 return_value,                                  \
                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                     ? get_valkey_glide_cluster_ce()            \
                                     : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                  \
            return;                                                             \
        }                                                                       \
        zval_dtor(return_value);                                                \
        RETURN_FALSE;                                                           \
    }

#define MSETNX_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, msetnx, execute_msetnx_command)

#define TYPE_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, type, execute_type_command)

#define APPEND_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, append, execute_append_command)

#define GETRANGE_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, getRange, execute_getrange_command)

#define SORT_METHOD_IMPL(class_name) STANDARD_METHOD_IMPL(class_name, sort, execute_sort_command)

#define SORT_RO_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, sort_ro, execute_sort_ro_command)


#define OBJECT_METHOD_IMPL(class_name) \
    STANDARD_METHOD_IMPL(class_name, object, execute_object_command)

/* Function command declarations */
int  execute_function_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
void execute_script_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
/* Helper macro to convert true to "OK" string if OPT_REPLY_LITERAL is enabled
 * This macro fetches the valkey_glide object internally and applies the transformation.
 * Usage: APPLY_REPLY_LITERAL(return_value);
 */
#define APPLY_REPLY_LITERAL(rv)                                                             \
    do {                                                                                    \
        valkey_glide_object* _valkey_glide =                                                \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());               \
        if (_valkey_glide && _valkey_glide->opt_reply_literal && Z_TYPE_P(rv) == IS_TRUE) { \
            zval_dtor(rv);                                                                  \
            ZVAL_STRING(rv, "OK");                                                          \
        }                                                                                   \
    } while (0)

/* Option methods - matching PHPRedis setOption/getOption API */
#define SETOPTION_METHOD_IMPL(class_name)                                     \
    PHP_METHOD(class_name, setOption) {                                       \
        zend_long option;                                                     \
        zval*     value;                                                      \
                                                                              \
        ZEND_PARSE_PARAMETERS_START(2, 2)                                     \
        Z_PARAM_LONG(option)                                                  \
        Z_PARAM_ZVAL(value)                                                   \
        ZEND_PARSE_PARAMETERS_END();                                          \
                                                                              \
        valkey_glide_object* valkey_glide =                                   \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        if (!valkey_glide) {                                                  \
            RETURN_FALSE;                                                     \
        }                                                                     \
                                                                              \
        switch (option) {                                                     \
            case VALKEY_GLIDE_OPT_REPLY_LITERAL:                              \
                valkey_glide->opt_reply_literal = zval_is_true(value);        \
                RETURN_TRUE;                                                  \
            default:                                                          \
                RETURN_FALSE;                                                 \
        }                                                                     \
    }

#define GETOPTION_METHOD_IMPL(class_name)                                     \
    PHP_METHOD(class_name, getOption) {                                       \
        zend_long option;                                                     \
                                                                              \
        ZEND_PARSE_PARAMETERS_START(1, 1)                                     \
        Z_PARAM_LONG(option)                                                  \
        ZEND_PARSE_PARAMETERS_END();                                          \
                                                                              \
        valkey_glide_object* valkey_glide =                                   \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        if (!valkey_glide) {                                                  \
            RETURN_FALSE;                                                     \
        }                                                                     \
                                                                              \
        switch (option) {                                                     \
            case VALKEY_GLIDE_OPT_REPLY_LITERAL:                              \
                RETURN_BOOL(valkey_glide->opt_reply_literal);                 \
            default:                                                          \
                RETURN_FALSE;                                                 \
        }                                                                     \
    }

/* Error introspection methods - matching PHPRedis getLastError/clearLastError API */
#define GETLASTERROR_METHOD_IMPL(class_name)                                  \
    PHP_METHOD(class_name, getLastError) {                                    \
        ZEND_PARSE_PARAMETERS_NONE();                                         \
                                                                              \
        valkey_glide_object* valkey_glide =                                   \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        if (!valkey_glide || !valkey_glide->last_error) {                     \
            RETURN_NULL();                                                    \
        }                                                                     \
                                                                              \
        RETURN_STR_COPY(valkey_glide->last_error);                            \
    }

#define CLEARLASTERROR_METHOD_IMPL(class_name)                                \
    PHP_METHOD(class_name, clearLastError) {                                  \
        ZEND_PARSE_PARAMETERS_NONE();                                         \
                                                                              \
        valkey_glide_object* valkey_glide =                                   \
            VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        valkey_glide_clear_last_error(valkey_glide);                          \
                                                                              \
        RETURN_TRUE;                                                          \
    }

/* FFI Compression functions - Statistics struct already defined in glide_bindings.h */
unsigned long get_min_compressed_size(void);

/* ====================================================================
 * STATISTICS METHOD IMPLEMENTATION MACRO
 * ==================================================================== */

#define GET_STATISTICS_METHOD_IMPL(class_name)                                                    \
    PHP_METHOD(class_name, getStatistics) {                                                       \
        ZEND_PARSE_PARAMETERS_NONE();                                                             \
                                                                                                  \
        Statistics stats = get_statistics();                                                      \
                                                                                                  \
        array_init(return_value);                                                                 \
        add_assoc_long(return_value, "total_connections", stats.total_connections);               \
        add_assoc_long(return_value, "total_clients", stats.total_clients);                       \
        add_assoc_long(return_value, "total_values_compressed", stats.total_values_compressed);   \
        add_assoc_long(                                                                           \
            return_value, "total_values_decompressed", stats.total_values_decompressed);          \
        add_assoc_long(return_value, "total_original_bytes", stats.total_original_bytes);         \
        add_assoc_long(return_value, "total_bytes_compressed", stats.total_bytes_compressed);     \
        add_assoc_long(return_value, "total_bytes_decompressed", stats.total_bytes_decompressed); \
        add_assoc_long(                                                                           \
            return_value, "compression_skipped_count", stats.compression_skipped_count);          \
    }

#endif /* VALKEY_GLIDE_COMMANDS_COMMON_H */
