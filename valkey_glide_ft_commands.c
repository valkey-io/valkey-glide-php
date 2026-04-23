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
 * Usage: $client->ftCreate(FtCreateBuilder $builder)
 *
 * The builder is a PHP object with a toArray() method that returns:
 *   ['index' => string, 'schema' => array, 'options' => array|null]
 * ==================================================================== */
int execute_ft_create_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                builder_zval = NULL;

    if (zend_parse_method_parameters(argc, object, "Oo", &object, ce, &builder_zval) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    /* Call toArray() on the builder object */
    zval method_name, builder_result;
    ZVAL_STRING(&method_name, "toArray");
    if (call_user_function(NULL, builder_zval, &method_name, &builder_result, 0, NULL) != SUCCESS ||
        Z_TYPE(builder_result) != IS_ARRAY) {
        zval_dtor(&method_name);
        zval_dtor(&builder_result);
        if (!EG(exception)) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(),
                "ftCreate: argument must be an FtCreateBuilder with a toArray() method",
                0);
        }
        return 0;
    }
    zval_dtor(&method_name);

    /* Extract index, schema, options from the result array */
    HashTable* result_ht = Z_ARRVAL(builder_result);

    zval* z_index = zend_hash_str_find(result_ht, "index", sizeof("index") - 1);
    if (!z_index || Z_TYPE_P(z_index) != IS_STRING) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftCreate: builder toArray() must return an 'index' string",
                             0);
        return 0;
    }

    zval* z_schema = zend_hash_str_find(result_ht, "schema", sizeof("schema") - 1);
    if (!z_schema || Z_TYPE_P(z_schema) != IS_ARRAY) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftCreate: builder toArray() must return a 'schema' array",
                             0);
        return 0;
    }

    zval*      z_options = zend_hash_str_find(result_ht, "options", sizeof("options") - 1);
    HashTable* opts_ht =
        (z_options && Z_TYPE_P(z_options) == IS_ARRAY) ? Z_ARRVAL_P(z_options) : NULL;

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    if (!build_ft_create_args(Z_STRVAL_P(z_index),
                              Z_STRLEN_P(z_index),
                              Z_ARRVAL_P(z_schema),
                              opts_ht,
                              &args,
                              &lens,
                              &count,
                              &allocated,
                              &alloc_n)) {
        zval_dtor(&builder_result);
        return 0;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtCreate,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    zval_dtor(&builder_result);
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
 * Usage: $client->ftSearch(FtSearchBuilder $builder)
 *
 * The builder is a PHP object with a toArray() method that returns:
 *   ['index' => string, 'query' => string, 'options' => array|null]
 * ==================================================================== */
int execute_ft_search_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                builder_zval = NULL;

    if (zend_parse_method_parameters(argc, object, "Oo", &object, ce, &builder_zval) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    zval method_name, builder_result;
    ZVAL_STRING(&method_name, "toArray");
    if (call_user_function(NULL, builder_zval, &method_name, &builder_result, 0, NULL) != SUCCESS ||
        Z_TYPE(builder_result) != IS_ARRAY) {
        zval_dtor(&method_name);
        zval_dtor(&builder_result);
        if (!EG(exception)) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(),
                "ftSearch: argument must be an FtSearchBuilder with a toArray() method",
                0);
        }
        return 0;
    }
    zval_dtor(&method_name);

    HashTable* result_ht = Z_ARRVAL(builder_result);

    zval* z_index = zend_hash_str_find(result_ht, "index", sizeof("index") - 1);
    if (!z_index || Z_TYPE_P(z_index) != IS_STRING) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftSearch: builder toArray() must return an 'index' string",
                             0);
        return 0;
    }

    zval* z_query = zend_hash_str_find(result_ht, "query", sizeof("query") - 1);
    if (!z_query || Z_TYPE_P(z_query) != IS_STRING) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftSearch: builder toArray() must return a 'query' string",
                             0);
        return 0;
    }

    zval*      z_options = zend_hash_str_find(result_ht, "options", sizeof("options") - 1);
    HashTable* opts_ht =
        (z_options && Z_TYPE_P(z_options) == IS_ARRAY) ? Z_ARRVAL_P(z_options) : NULL;

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    if (!build_ft_search_args(Z_STRVAL_P(z_index),
                              Z_STRLEN_P(z_index),
                              Z_STRVAL_P(z_query),
                              Z_STRLEN_P(z_query),
                              opts_ht,
                              &args,
                              &lens,
                              &count,
                              &allocated,
                              &alloc_n)) {
        zval_dtor(&builder_result);
        return 0;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtSearch,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    zval_dtor(&builder_result);
    return status;
}

/* ====================================================================
 * FT.AGGREGATE
 * Usage: $client->ftAggregate(FtAggregateBuilder $builder)
 *
 * The builder is a PHP object with a toArray() method that returns:
 *   ['index' => string, 'query' => string, 'options' => array|null]
 * ==================================================================== */
int execute_ft_aggregate_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                builder_zval = NULL;

    if (zend_parse_method_parameters(argc, object, "Oo", &object, ce, &builder_zval) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    zval method_name, builder_result;
    ZVAL_STRING(&method_name, "toArray");
    if (call_user_function(NULL, builder_zval, &method_name, &builder_result, 0, NULL) != SUCCESS ||
        Z_TYPE(builder_result) != IS_ARRAY) {
        zval_dtor(&method_name);
        zval_dtor(&builder_result);
        if (!EG(exception)) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(),
                "ftAggregate: argument must be an FtAggregateBuilder with a toArray() method",
                0);
        }
        return 0;
    }
    zval_dtor(&method_name);

    HashTable* result_ht = Z_ARRVAL(builder_result);

    zval* z_index = zend_hash_str_find(result_ht, "index", sizeof("index") - 1);
    if (!z_index || Z_TYPE_P(z_index) != IS_STRING) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftAggregate: builder toArray() must return an 'index' string",
                             0);
        return 0;
    }

    zval* z_query = zend_hash_str_find(result_ht, "query", sizeof("query") - 1);
    if (!z_query || Z_TYPE_P(z_query) != IS_STRING) {
        zval_dtor(&builder_result);
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftAggregate: builder toArray() must return a 'query' string",
                             0);
        return 0;
    }

    zval*      z_options = zend_hash_str_find(result_ht, "options", sizeof("options") - 1);
    HashTable* opts_ht =
        (z_options && Z_TYPE_P(z_options) == IS_ARRAY) ? Z_ARRVAL_P(z_options) : NULL;

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    if (!build_ft_aggregate_args(Z_STRVAL_P(z_index),
                                 Z_STRLEN_P(z_index),
                                 Z_STRVAL_P(z_query),
                                 Z_STRLEN_P(z_query),
                                 opts_ht,
                                 &args,
                                 &lens,
                                 &count,
                                 &allocated,
                                 &alloc_n)) {
        zval_dtor(&builder_result);
        return 0;
    }

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             FtAggregate,
                                             args,
                                             lens,
                                             count,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    free_ft_collected(args, lens, allocated, alloc_n);
    zval_dtor(&builder_result);
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
        return 0;
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
