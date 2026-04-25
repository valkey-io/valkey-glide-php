/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_FT_COMMON_H
#define VALKEY_GLIDE_FT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

/**
 * Execute an FT.* command with a dynamic string argument list.
 *
 * Builds the FFI argument arrays from a C string array, calls execute_command(),
 * and converts the response to a PHP zval.
 *
 * Return value convention (consistent across all FT command helpers):
 *   1  — success; return_value has been populated.
 *   0  — failure; either a PHP exception has been thrown (check EG(exception))
 *         or the FFI call returned no result. The caller must NOT set return_value
 *         itself — the PHP_METHOD wrapper handles RETURN_FALSE on 0.
 *
 * @param glide_client  The FFI client pointer.
 * @param cmd_type      The RequestType enum value (e.g. FtCreate, FtSearch).
 * @param strings       Array of C strings (arguments).
 * @param lengths       Array of string lengths.
 * @param count         Number of arguments.
 * @param return_value  PHP return value zval.
 * @param assoc_flag    Associative array flag for response conversion.
 * @return 1 on success, 0 on failure (exception may be pending).
 */
int execute_ft_command_internal(const void*      glide_client,
                                enum RequestType cmd_type,
                                const char**     strings,
                                size_t*          lengths,
                                int              count,
                                zval*            return_value,
                                int              assoc_flag);

/* ====================================================================
 * ARGUMENT COLLECTION UTILITIES
 * ==================================================================== */

/**
 * Free resources allocated by FT argument builders (e.g. build_ft_create_args).
 * Builders populate parallel arrays of C string pointers, lengths, and a list
 * of emalloc'd strings that own their backing memory.
 */
void free_ft_collected(const char** strings,
                       size_t*      lengths,
                       char**       allocated,
                       int          allocated_count);

/* ====================================================================
 * STRUCTURED ARGUMENT BUILDERS
 *
 * Convert associative PHP arrays into flat string token lists.
 * Defined in valkey_glide_ft_common.c.
 * ==================================================================== */

int build_ft_create_args(const char*   index_name,
                         size_t        index_name_len,
                         HashTable*    schema_ht,
                         HashTable*    options_ht,
                         const char*** out_strings,
                         size_t**      out_lengths,
                         int*          out_count,
                         char***       out_allocated,
                         int*          out_alloc_count);

int build_ft_search_args(const char*   index_name,
                         size_t        index_name_len,
                         const char*   query,
                         size_t        query_len,
                         HashTable*    options_ht,
                         const char*** out_strings,
                         size_t**      out_lengths,
                         int*          out_count,
                         char***       out_allocated,
                         int*          out_alloc_count);

int build_ft_aggregate_args(const char*   index_name,
                            size_t        index_name_len,
                            const char*   query,
                            size_t        query_len,
                            HashTable*    options_ht,
                            const char*** out_strings,
                            size_t**      out_lengths,
                            int*          out_count,
                            char***       out_allocated,
                            int*          out_alloc_count);

int build_ft_info_args(const char*   index_name,
                       size_t        index_name_len,
                       HashTable*    options_ht,
                       const char*** out_strings,
                       size_t**      out_lengths,
                       int*          out_count,
                       char***       out_allocated,
                       int*          out_alloc_count);

/* ====================================================================
 * COMMAND EXECUTION FUNCTIONS
 * ==================================================================== */

int execute_ft_create_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_dropindex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_list_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_search_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_aggregate_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_info_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_aliasadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_aliasdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ft_aliasupdate_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_ft_aliaslist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* ====================================================================
 * CONVENIENCE MACROS
 * ==================================================================== */

/**
 * Validate FT client and key
 */
#define VALIDATE_FT_CLIENT(client) \
    if (!(client)) {               \
        return 0;                  \
    }

/**
 * Cleanup collected FT strings if non-NULL
 */
#define CLEANUP_FT_COLLECTED(strings, lengths, allocated, alloc_count)   \
    do {                                                                 \
        if (strings)                                                     \
            free_ft_collected(strings, lengths, allocated, alloc_count); \
    } while (0)

/* ====================================================================
 * FT COMMAND METHOD IMPLEMENTATION MACROS
 * ==================================================================== */

#define FT_CREATE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftCreate) {                                               \
        if (execute_ft_create_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                       \
            return;                                                                  \
        }                                                                            \
        zval_dtor(return_value);                                                     \
        RETURN_FALSE;                                                                \
    }

#define FT_DROPINDEX_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftDropIndex) {                                               \
        if (execute_ft_dropindex_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                          \
            return;                                                                     \
        }                                                                               \
        zval_dtor(return_value);                                                        \
        RETURN_FALSE;                                                                   \
    }

#define FT_LIST_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftList) {                                               \
        if (execute_ft_list_command(getThis(),                                     \
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

#define FT_SEARCH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftSearch) {                                               \
        if (execute_ft_search_command(getThis(),                                     \
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

#define FT_AGGREGATE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftAggregate) {                                               \
        if (execute_ft_aggregate_command(getThis(),                                     \
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

#define FT_INFO_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftInfo) {                                               \
        if (execute_ft_info_command(getThis(),                                     \
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

#define FT_ALIASADD_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftAliasAdd) {                                               \
        if (execute_ft_aliasadd_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                         \
            return;                                                                    \
        }                                                                              \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define FT_ALIASDEL_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftAliasDel) {                                               \
        if (execute_ft_aliasdel_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                         \
            return;                                                                    \
        }                                                                              \
        zval_dtor(return_value);                                                       \
        RETURN_FALSE;                                                                  \
    }

#define FT_ALIASUPDATE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftAliasUpdate) {                                               \
        if (execute_ft_aliasupdate_command(getThis(),                                     \
                                           ZEND_NUM_ARGS(),                               \
                                           return_value,                                  \
                                           strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                               ? get_valkey_glide_cluster_ce()            \
                                               : get_valkey_glide_ce())) {                \
            APPLY_REPLY_LITERAL(return_value);                                            \
            return;                                                                       \
        }                                                                                 \
        zval_dtor(return_value);                                                          \
        RETURN_FALSE;                                                                     \
    }

#define FT_ALIASLIST_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ftAliasList) {                                               \
        if (execute_ft_aliaslist_command(getThis(),                                     \
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

#endif /* VALKEY_GLIDE_FT_COMMON_H */
