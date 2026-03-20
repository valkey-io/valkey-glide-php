/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include "valkey_glide_ft_common.h"

/* ====================================================================
 * FT.CREATE
 * Usage: $client->ftCreate(string $index, array $schema, ?array $options = null)
 *
 * $schema is an array of associative arrays, each describing a field:
 *   [['name' => 'title', 'type' => 'TEXT', 'sortable' => true], ...]
 *
 * $options is an associative array:
 *   ['ON' => 'HASH', 'PREFIX' => ['docs:'], ...]
 * ==================================================================== */
int execute_ft_create_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    zval*                schema_arr     = NULL;
    zval*                options_arr    = NULL;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osa|a",
                                     &object,
                                     ce,
                                     &index_name,
                                     &index_name_len,
                                     &schema_arr,
                                     &options_arr) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    HashTable* opts_ht =
        (options_arr && Z_TYPE_P(options_arr) == IS_ARRAY) ? Z_ARRVAL_P(options_arr) : NULL;

    if (!build_ft_create_args(index_name,
                              index_name_len,
                              Z_ARRVAL_P(schema_arr),
                              opts_ht,
                              &args,
                              &lens,
                              &count,
                              &allocated,
                              &alloc_n)) {
        return 1; /* Exception already thrown by builder */
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtCreate,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    return status;
}

/* ====================================================================
 * FT.DROPINDEX
 * Usage: $client->ftDropIndex(string $index)
 * ==================================================================== */
int execute_ft_dropindex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os", &object, ce, &index_name, &index_name_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char* args[] = {index_name};
    size_t      lens[] = {index_name_len};

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtDropIndex,
                                       args,
                                       lens,
                                       1,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT._LIST
 * Usage: $client->ftList()
 * ==================================================================== */
int execute_ft_list_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtList,
                                       NULL,
                                       NULL,
                                       0,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT.SEARCH
 * Usage: $client->ftSearch(string $index, string $query, ?array $options = null)
 *
 * $options is an associative array:
 *   ['NOCONTENT' => true, 'LIMIT' => [0, 10], 'PARAMS' => ['k' => 'v'], ...]
 * ==================================================================== */
int execute_ft_search_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    char*                query          = NULL;
    size_t               query_len      = 0;
    zval*                options_arr    = NULL;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss|a",
                                     &object,
                                     ce,
                                     &index_name,
                                     &index_name_len,
                                     &query,
                                     &query_len,
                                     &options_arr) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    HashTable* opts_ht =
        (options_arr && Z_TYPE_P(options_arr) == IS_ARRAY) ? Z_ARRVAL_P(options_arr) : NULL;

    if (!build_ft_search_args(index_name,
                              index_name_len,
                              query,
                              query_len,
                              opts_ht,
                              &args,
                              &lens,
                              &count,
                              &allocated,
                              &alloc_n)) {
        return 1;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtSearch,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    return status;
}

/* ====================================================================
 * FT.AGGREGATE
 * Usage: $client->ftAggregate(string $index, string $query, ?array $options = null)
 * ==================================================================== */
int execute_ft_aggregate_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    char*                query          = NULL;
    size_t               query_len      = 0;
    zval*                options_arr    = NULL;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss|a",
                                     &object,
                                     ce,
                                     &index_name,
                                     &index_name_len,
                                     &query,
                                     &query_len,
                                     &options_arr) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    HashTable* opts_ht =
        (options_arr && Z_TYPE_P(options_arr) == IS_ARRAY) ? Z_ARRVAL_P(options_arr) : NULL;

    if (!build_ft_aggregate_args(index_name,
                                 index_name_len,
                                 query,
                                 query_len,
                                 opts_ht,
                                 &args,
                                 &lens,
                                 &count,
                                 &allocated,
                                 &alloc_n)) {
        return 1;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtAggregate,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    return status;
}

/* ====================================================================
 * FT.INFO
 * Usage: $client->ftInfo(string $index, ?array $options = null)
 *
 * $options is an associative array:
 *   ['scope' => 'LOCAL']
 * ==================================================================== */
int execute_ft_info_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    zval*                options_arr    = NULL;

    if (zend_parse_method_parameters(
            argc, object, "Os|a", &object, ce, &index_name, &index_name_len, &options_arr) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    HashTable* opts_ht =
        (options_arr && Z_TYPE_P(options_arr) == IS_ARRAY) ? Z_ARRVAL_P(options_arr) : NULL;

    if (!build_ft_info_args(
            index_name, index_name_len, opts_ht, &args, &lens, &count, &allocated, &alloc_n)) {
        return 1;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtInfo,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP);

    free_ft_collected(args, lens, allocated, alloc_n);
    return status;
}

/* ====================================================================
 * FT.ALIASADD
 * Usage: $client->ftAliasAdd(string $alias, string $index)
 * ==================================================================== */
int execute_ft_aliasadd_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                alias_name     = NULL;
    size_t               alias_name_len = 0;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss",
                                     &object,
                                     ce,
                                     &alias_name,
                                     &alias_name_len,
                                     &index_name,
                                     &index_name_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char* args[] = {alias_name, index_name};
    size_t      lens[] = {alias_name_len, index_name_len};

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtAliasAdd,
                                       args,
                                       lens,
                                       2,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT.ALIASDEL
 * Usage: $client->ftAliasDel(string $alias)
 * ==================================================================== */
int execute_ft_aliasdel_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                alias_name     = NULL;
    size_t               alias_name_len = 0;

    if (zend_parse_method_parameters(
            argc, object, "Os", &object, ce, &alias_name, &alias_name_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char* args[] = {alias_name};
    size_t      lens[] = {alias_name_len};

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtAliasDel,
                                       args,
                                       lens,
                                       1,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT.ALIASUPDATE
 * Usage: $client->ftAliasUpdate(string $alias, string $index)
 * ==================================================================== */
int execute_ft_aliasupdate_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                alias_name     = NULL;
    size_t               alias_name_len = 0;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Oss",
                                     &object,
                                     ce,
                                     &alias_name,
                                     &alias_name_len,
                                     &index_name,
                                     &index_name_len) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    const char* args[] = {alias_name, index_name};
    size_t      lens[] = {alias_name_len, index_name_len};

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtAliasUpdate,
                                       args,
                                       lens,
                                       2,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT._ALIASLIST
 * Usage: $client->ftAliasList()
 * ==================================================================== */
int execute_ft_aliaslist_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       FtAliasList,
                                       NULL,
                                       NULL,
                                       0,
                                       return_value,
                                       COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP);
}
