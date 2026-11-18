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

/* ====================================================================
 * FT.HYBRID (valkey-search 1.3.0)
 * Usage: $client->ftHybrid(FtHybridBuilder $builder)
 *
 * Dispatched via CustomCommand, not a typed RequestType. Typed
 * RequestType.FtHybrid does not exist in the glide-core commit this
 * submodule is pinned to (the request-type core PR, valkey-glide#7456,
 * has not merged upstream yet) -- see this port's PR description's
 * Limitations section. CustomCommand requires the full command name as
 * argv[0]; build_ft_hybrid_args() does not include it, so it is
 * prepended here.
 * ==================================================================== */
int execute_ft_hybrid_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
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
                "ftHybrid: argument must be an FtHybridBuilder with a toArray() method",
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
                             "ftHybrid: builder toArray() must return an 'index' string",
                             0);
        return 0;
    }

    const char** args      = NULL;
    size_t*      lens      = NULL;
    int          count     = 0;
    char**       allocated = NULL;
    int          alloc_n   = 0;

    if (!build_ft_hybrid_args(Z_STRVAL_P(z_index),
                              Z_STRLEN_P(z_index),
                              result_ht,
                              &args,
                              &lens,
                              &count,
                              &allocated,
                              &alloc_n)) {
        zval_dtor(&builder_result);
        return 0;
    }

    /* CustomCommand requires argv[0] to be the literal command name. */
    const char** full_args = emalloc((count + 1) * sizeof(const char*));
    size_t*      full_lens = emalloc((count + 1) * sizeof(size_t));
    full_args[0]           = "FT.HYBRID";
    full_lens[0]           = sizeof("FT.HYBRID") - 1;
    memcpy(full_args + 1, args, count * sizeof(const char*));
    memcpy(full_lens + 1, lens, count * sizeof(size_t));

    int status = execute_ft_command_internal(valkey_glide->glide_client,
                                             CustomCommand,
                                             full_args,
                                             full_lens,
                                             count + 1,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE);

    efree(full_args);
    efree(full_lens);
    free_ft_collected(args, lens, allocated, alloc_n);
    zval_dtor(&builder_result);
    return status;
}

/* ====================================================================
 * FT.CURSOR READ
 * Usage: $client->ftCursorRead(string $index, int $cursorId, ?int $count = null)
 *
 * Dispatched via CustomCommand -- see execute_ft_hybrid_command()'s
 * docstring for why (typed RequestType.FtCursorRead is not reachable in
 * this submodule's pinned glide-core commit).
 * ==================================================================== */
int execute_ft_cursor_read_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    zend_long            cursor_id      = 0;
    zend_long            count          = 0;
    zend_bool            count_is_null  = 1;

    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osl|l!",
                                     &object,
                                     ce,
                                     &index_name,
                                     &index_name_len,
                                     &cursor_id,
                                     &count,
                                     &count_is_null) == FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    char cursor_id_str[32];
    int  cursor_id_len = snprintf(cursor_id_str, sizeof(cursor_id_str), ZEND_LONG_FMT, cursor_id);
    char count_str[32];
    int  count_len = 0;
    if (!count_is_null) {
        count_len = snprintf(count_str, sizeof(count_str), ZEND_LONG_FMT, count);
    }

    /* FT.CURSOR READ <index> <cursor_id> [COUNT <count>] -- the COUNT
     * keyword itself was missing here (H-2): the server's grammar
     * requires the literal token before the count value
     * (valkey-search src/commands/ft_cursor.cc), and without it the
     * trailing number is rejected with "Unexpected argument `<count>`".
     */
    const char* args[6];
    size_t      lens[6];
    int         argn;
    args[0] = "FT.CURSOR";
    lens[0] = sizeof("FT.CURSOR") - 1;
    args[1] = "READ";
    lens[1] = sizeof("READ") - 1;
    args[2] = index_name;
    lens[2] = index_name_len;
    args[3] = cursor_id_str;
    lens[3] = (size_t) cursor_id_len;
    if (count_is_null) {
        argn = 4;
    } else {
        args[4] = "COUNT";
        lens[4] = sizeof("COUNT") - 1;
        args[5] = count_str;
        lens[5] = (size_t) count_len;
        argn    = 6;
    }

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       CustomCommand,
                                       args,
                                       lens,
                                       argn,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}

/* ====================================================================
 * FT.CURSOR DEL
 * Usage: $client->ftCursorDel(string $index, int $cursorId)
 *
 * Dispatched via CustomCommand -- see execute_ft_hybrid_command()'s
 * docstring for why.
 * ==================================================================== */
int execute_ft_cursor_del_command(zval*             object,
                                  int               argc,
                                  zval*             return_value,
                                  zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                index_name     = NULL;
    size_t               index_name_len = 0;
    zend_long            cursor_id      = 0;

    if (zend_parse_method_parameters(
            argc, object, "Osl", &object, ce, &index_name, &index_name_len, &cursor_id) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    VALIDATE_FT_CLIENT(valkey_glide && valkey_glide->glide_client);

    char cursor_id_str[32];
    int  cursor_id_len = snprintf(cursor_id_str, sizeof(cursor_id_str), ZEND_LONG_FMT, cursor_id);

    const char* args[] = {"FT.CURSOR", "DEL", index_name, cursor_id_str};
    size_t      lens[] = {
        sizeof("FT.CURSOR") - 1, sizeof("DEL") - 1, index_name_len, (size_t) cursor_id_len};

    return execute_ft_command_internal(valkey_glide->glide_client,
                                       CustomCommand,
                                       args,
                                       lens,
                                       4,
                                       return_value,
                                       COMMAND_RESPONSE_NOT_ASSOSIATIVE);
}
