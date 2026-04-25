/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

/**
 * Shared utilities and argument builders for FT.* commands.
 *
 * Contains the core FFI dispatch wrapper, array collection helpers,
 * and structured argument builders that convert associative PHP arrays
 * into the flat string token lists the FFI layer expects.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include "valkey_glide_ft_common.h"

/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

int execute_ft_command_internal(const void*      glide_client,
                                enum RequestType cmd_type,
                                const char**     strings,
                                size_t*          lengths,
                                int              count,
                                zval*            return_value,
                                int              assoc_flag) {
    uintptr_t*     cmd_args = NULL;
    unsigned long* args_len = NULL;

    if (count > 0) {
        cmd_args = (uintptr_t*) emalloc(count * sizeof(uintptr_t));
        args_len = (unsigned long*) emalloc(count * sizeof(unsigned long));

        for (int i = 0; i < count; i++) {
            cmd_args[i] = (uintptr_t) strings[i];
            args_len[i] = (unsigned long) lengths[i];
        }
    }

    CommandResult* result = execute_command(glide_client, cmd_type, count, cmd_args, args_len);

    if (cmd_args)
        efree(cmd_args);
    if (args_len)
        efree(args_len);

    if (!result) {
        return 0;
    }
    if (result->command_error) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
        free_command_result(result);
        return 0;
    }
    if (!result->response) {
        free_command_result(result);
        return 0;
    }

    int status = command_response_to_zval(result->response, return_value, assoc_flag, false);
    free_command_result(result);
    return status;
}

/* ====================================================================
 * ARGUMENT COLLECTION UTILITIES
 * ==================================================================== */

void free_ft_collected(const char** strings,
                       size_t*      lengths,
                       char**       allocated,
                       int          allocated_count) {
    for (int i = 0; i < allocated_count; i++) {
        efree(allocated[i]);
    }
    efree((void*) strings);
    efree(lengths);
    efree(allocated);
}

/* ====================================================================
 * DYNAMIC STRING BUFFER
 * ==================================================================== */

#define FT_BUF_INIT_CAP 32

typedef struct {
    const char** strings;
    size_t*      lengths;
    char**       allocated;
    int          count;
    int          alloc_count;
    int          capacity;
} ft_arg_buf_t;

static void ft_buf_init(ft_arg_buf_t* buf) {
    buf->capacity    = FT_BUF_INIT_CAP;
    buf->count       = 0;
    buf->alloc_count = 0;
    buf->strings     = emalloc(buf->capacity * sizeof(const char*));
    buf->lengths     = emalloc(buf->capacity * sizeof(size_t));
    buf->allocated   = emalloc(buf->capacity * sizeof(char*));
}

static void ft_buf_grow(ft_arg_buf_t* buf) {
    buf->capacity *= 2;
    buf->strings   = erealloc(buf->strings, buf->capacity * sizeof(const char*));
    buf->lengths   = erealloc(buf->lengths, buf->capacity * sizeof(size_t));
    buf->allocated = erealloc(buf->allocated, buf->capacity * sizeof(char*));
}

/* Caller must ensure str outlives the buffer (no copy made) */
static void ft_buf_add(ft_arg_buf_t* buf, const char* str, size_t len) {
    if (buf->count >= buf->capacity) {
        ft_buf_grow(buf);
    }
    buf->strings[buf->count] = str;
    buf->lengths[buf->count] = len;
    buf->count++;
}

#define FT_BUF_LIT(buf, literal) ft_buf_add((buf), (literal), sizeof(literal) - 1)

static void ft_buf_add_copy(ft_arg_buf_t* buf, const char* str, size_t len) {
    char* copy = emalloc(len + 1);
    memcpy(copy, str, len);
    copy[len] = '\0';

    if (buf->count >= buf->capacity) {
        ft_buf_grow(buf);
    }
    buf->strings[buf->count] = copy;
    buf->lengths[buf->count] = len;
    buf->count++;
    buf->allocated[buf->alloc_count++] = copy;
}

static void ft_buf_add_long(ft_arg_buf_t* buf, zend_long val) {
    char tmp[24];
    int  len = snprintf(tmp, sizeof(tmp), ZEND_LONG_FMT, val);
    ft_buf_add_copy(buf, tmp, len);
}

static void ft_buf_add_double(ft_arg_buf_t* buf, double val) {
    char tmp[64];
    int  len = snprintf(tmp, sizeof(tmp), "%.6g", val);
    ft_buf_add_copy(buf, tmp, len);
}

static void ft_buf_add_zval(ft_arg_buf_t* buf, zval* z) {
    if (Z_TYPE_P(z) == IS_STRING) {
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    } else if (Z_TYPE_P(z) == IS_LONG) {
        ft_buf_add_long(buf, Z_LVAL_P(z));
    } else if (Z_TYPE_P(z) == IS_DOUBLE) {
        ft_buf_add_double(buf, Z_DVAL_P(z));
    } else {
        zval copy;
        ZVAL_DUP(&copy, z);
        convert_to_string(&copy);
        ft_buf_add_copy(buf, Z_STRVAL(copy), Z_STRLEN(copy));
        zval_dtor(&copy);
    }
}

static void ft_buf_destroy(ft_arg_buf_t* buf) {
    for (int i = 0; i < buf->alloc_count; i++) {
        efree(buf->allocated[i]);
    }
    efree(buf->strings);
    efree(buf->lengths);
    efree(buf->allocated);
}

/* ====================================================================
 * HELPER: find a key in a HashTable (case-insensitive for convenience)
 * ==================================================================== */

static zval* ft_find(HashTable* ht, const char* key) {
    return zend_hash_str_find(ht, key, strlen(key));
}

/* ====================================================================
 * FT.CREATE SCHEMA BUILDER
 *
 * Each element of $schema is an associative array:
 *   ['name' => 'title', 'type' => 'TEXT', 'sortable' => true, ...]
 * ==================================================================== */

static int ft_build_field(ft_arg_buf_t* buf, HashTable* field) {
    zval* z;

    /* name (required) */
    z = ft_find(field, "name");
    if (!z || Z_TYPE_P(z) != IS_STRING) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftCreate: each schema field must have a 'name' key",
                             0);
        return 0;
    }
    ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));

    /* AS alias (optional) */
    z = ft_find(field, "alias");
    if (z && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "AS");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    /* type (required) */
    z = ft_find(field, "type");
    if (!z || Z_TYPE_P(z) != IS_STRING) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftCreate: each schema field must have a 'type' key",
                             0);
        return 0;
    }
    const char* type = Z_STRVAL_P(z);
    ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));

    /* --- TEXT options --- */
    if (strcasecmp(type, "TEXT") == 0) {
        if ((z = ft_find(field, "nostem")) && zend_is_true(z)) {
            FT_BUF_LIT(buf, "NOSTEM");
        }
        if ((z = ft_find(field, "weight"))) {
            FT_BUF_LIT(buf, "WEIGHT");
            ft_buf_add_zval(buf, z);
        }
        if ((z = ft_find(field, "withsuffixtrie")) && zend_is_true(z)) {
            FT_BUF_LIT(buf, "WITHSUFFIXTRIE");
        } else if ((z = ft_find(field, "nosuffixtrie")) && zend_is_true(z)) {
            FT_BUF_LIT(buf, "NOSUFFIXTRIE");
        }
    }

    /* --- TAG options --- */
    if (strcasecmp(type, "TAG") == 0) {
        if ((z = ft_find(field, "separator")) && Z_TYPE_P(z) == IS_STRING) {
            FT_BUF_LIT(buf, "SEPARATOR");
            ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
        }
        if ((z = ft_find(field, "casesensitive")) && zend_is_true(z)) {
            FT_BUF_LIT(buf, "CASESENSITIVE");
        }
    }

    /* --- VECTOR options --- */
    if (strcasecmp(type, "VECTOR") == 0) {
        /* algorithm (required) */
        z = ft_find(field, "algorithm");
        if (!z || Z_TYPE_P(z) != IS_STRING) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "ftCreate: VECTOR field must have 'algorithm' (FLAT or HNSW)",
                                 0);
            return 0;
        }
        const char* algorithm = Z_STRVAL_P(z);
        ft_buf_add(buf, algorithm, Z_STRLEN_P(z));

        int is_hnsw = (strcasecmp(algorithm, "HNSW") == 0);

        /* Build the attribute list in a temp buffer to get the count */
        ft_arg_buf_t attrs;
        ft_buf_init(&attrs);

        z = ft_find(field, "dim");
        if (z) {
            FT_BUF_LIT(&attrs, "DIM");
            ft_buf_add_zval(&attrs, z);
        }
        z = ft_find(field, "metric");
        if (z) {
            FT_BUF_LIT(&attrs, "DISTANCE_METRIC");
            ft_buf_add_zval(&attrs, z);
        }
        /* TYPE defaults to FLOAT32 */
        z = ft_find(field, "datatype");
        if (z) {
            FT_BUF_LIT(&attrs, "TYPE");
            ft_buf_add_zval(&attrs, z);
        } else {
            FT_BUF_LIT(&attrs, "TYPE");
            FT_BUF_LIT(&attrs, "FLOAT32");
        }
        if ((z = ft_find(field, "initial_cap"))) {
            FT_BUF_LIT(&attrs, "INITIAL_CAP");
            ft_buf_add_zval(&attrs, z);
        }
        /* M, EF_CONSTRUCTION, EF_RUNTIME are HNSW-only */
        if (is_hnsw) {
            if ((z = ft_find(field, "m"))) {
                FT_BUF_LIT(&attrs, "M");
                ft_buf_add_zval(&attrs, z);
            }
            if ((z = ft_find(field, "ef_construction"))) {
                FT_BUF_LIT(&attrs, "EF_CONSTRUCTION");
                ft_buf_add_zval(&attrs, z);
            }
            if ((z = ft_find(field, "ef_runtime"))) {
                FT_BUF_LIT(&attrs, "EF_RUNTIME");
                ft_buf_add_zval(&attrs, z);
            }
        }

        /* Emit count then attributes (must come before the attributes) */
        ft_buf_add_long(buf, attrs.count);
        for (int i = 0; i < attrs.count; i++) {
            ft_buf_add_copy(buf, attrs.strings[i], attrs.lengths[i]);
        }
        ft_buf_destroy(&attrs);
    }

    /* SORTABLE (common to TEXT, TAG, NUMERIC) */
    if ((z = ft_find(field, "sortable")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "SORTABLE");
    }

    return 1;
}

/* ====================================================================
 * FT.CREATE OPTIONS BUILDER
 *
 * $options = [
 *     'ON'              => 'HASH' | 'JSON',
 *     'PREFIX'          => ['docs:', 'blog:'],
 *     'SCORE'           => 1.0,
 *     'LANGUAGE'        => 'english',
 *     'SKIPINITIALSCAN' => true,
 *     'MINSTEMSIZE'     => 6,
 *     'WITHOFFSETS'     => true (default),
 *     'NOOFFSETS'       => true,
 *     'NOSTOPWORDS'     => true,
 *     'STOPWORDS'       => ['the', 'a'],
 *     'PUNCTUATION'     => ',.<>{}[]"':;!@#$%^&\*()-+=~/\|?',
 * ];
 * ==================================================================== */

static void ft_build_create_options(ft_arg_buf_t* buf, HashTable* opts) {
    zval* z;

    if ((z = ft_find(opts, "ON")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "ON");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(opts, "PREFIX")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* prefixes = Z_ARRVAL_P(z);
        FT_BUF_LIT(buf, "PREFIX");
        ft_buf_add_long(buf, zend_hash_num_elements(prefixes));
        zval* pfx;
        ZEND_HASH_FOREACH_VAL(prefixes, pfx) {
            ft_buf_add_zval(buf, pfx);
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "SCORE"))) {
        FT_BUF_LIT(buf, "SCORE");
        ft_buf_add_zval(buf, z);
    }

    if ((z = ft_find(opts, "LANGUAGE")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "LANGUAGE");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(opts, "SKIPINITIALSCAN")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "SKIPINITIALSCAN");
    }

    if ((z = ft_find(opts, "MINSTEMSIZE"))) {
        FT_BUF_LIT(buf, "MINSTEMSIZE");
        ft_buf_add_zval(buf, z);
    }

    // Mutually exclusive
    if ((z = ft_find(opts, "WITHOFFSETS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "WITHOFFSETS");
    } else if ((z = ft_find(opts, "NOOFFSETS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "NOOFFSETS");
    }

    // Mutually exclusive
    if ((z = ft_find(opts, "NOSTOPWORDS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "NOSTOPWORDS");
    } else if ((z = ft_find(opts, "STOPWORDS")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* words = Z_ARRVAL_P(z);
        FT_BUF_LIT(buf, "STOPWORDS");
        ft_buf_add_long(buf, zend_hash_num_elements(words));
        zval* w;
        ZEND_HASH_FOREACH_VAL(words, w) {
            ft_buf_add_zval(buf, w);
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "PUNCTUATION")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "PUNCTUATION");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }
}

/* ====================================================================
 * FT.SEARCH OPTIONS BUILDER
 *
 * $options = [
 *     'NOCONTENT'    => true,
 *     'VERBATIM'     => true,
 *     'INORDER'      => true,
 *     'SLOP'         => 2,
 *     'LIMIT'        => [0, 10],
 *     'SORTBY'       => ['price', 'ASC'],
 *     'WITHSORTKEYS' => true,
 *     'RETURN'       => ['title', 'price'],
 *     'TIMEOUT'      => 5000,
 *     'PARAMS'       => ['query_vec' => $vec],
 *     'DIALECT'      => 2,
 *     'ALLSHARDS'    => true,       // or 'SOMESHARDS' => true
 *     'CONSISTENT'   => true,       // or 'INCONSISTENT' => true
 * ];
 * ==================================================================== */

static void ft_build_search_options(ft_arg_buf_t* buf, HashTable* opts) {
    zval* z;

    if ((z = ft_find(opts, "NOCONTENT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "NOCONTENT");
    }

    if ((z = ft_find(opts, "VERBATIM")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "VERBATIM");
    }

    if ((z = ft_find(opts, "INORDER")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "INORDER");
    }

    if ((z = ft_find(opts, "SLOP"))) {
        FT_BUF_LIT(buf, "SLOP");
        ft_buf_add_zval(buf, z);
    }

    if ((z = ft_find(opts, "RETURN")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* fields = Z_ARRVAL_P(z);
        /* Count total tokens: string key entries emit 3 (field AS alias),
         * numeric key entries emit 1 (field) */
        int          token_count = 0;
        zend_string* key;
        zval*        val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(fields, key, val) {
            token_count += key ? 3 : 1;
        }
        ZEND_HASH_FOREACH_END();

        FT_BUF_LIT(buf, "RETURN");
        ft_buf_add_long(buf, token_count);

        ZEND_HASH_FOREACH_STR_KEY_VAL(fields, key, val) {
            if (key) {
                /* String key => field name, value => alias */
                ft_buf_add(buf, ZSTR_VAL(key), ZSTR_LEN(key));
                FT_BUF_LIT(buf, "AS");
                ft_buf_add_zval(buf, val);
            } else {
                /* Numeric key => value is the field name */
                ft_buf_add_zval(buf, val);
            }
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "SORTBY")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* sortby = Z_ARRVAL_P(z);
        FT_BUF_LIT(buf, "SORTBY");
        zval* s;
        ZEND_HASH_FOREACH_VAL(sortby, s) {
            ft_buf_add_zval(buf, s);
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "WITHSORTKEYS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "WITHSORTKEYS");
    }

    if ((z = ft_find(opts, "TIMEOUT"))) {
        FT_BUF_LIT(buf, "TIMEOUT");
        ft_buf_add_zval(buf, z);
    }

    if ((z = ft_find(opts, "PARAMS")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* params  = Z_ARRVAL_P(z);
        int        nparams = zend_hash_num_elements(params);
        FT_BUF_LIT(buf, "PARAMS");
        ft_buf_add_long(buf, nparams * 2);
        zend_string* key;
        zval*        val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(params, key, val) {
            if (key) {
                ft_buf_add(buf, ZSTR_VAL(key), ZSTR_LEN(key));
            }
            ft_buf_add_zval(buf, val);
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "LIMIT")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* limit = Z_ARRVAL_P(z);
        FT_BUF_LIT(buf, "LIMIT");
        zval* v;
        ZEND_HASH_FOREACH_VAL(limit, v) {
            ft_buf_add_zval(buf, v);
        }
        ZEND_HASH_FOREACH_END();
    }

    if ((z = ft_find(opts, "ALLSHARDS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "ALLSHARDS");
    } else if ((z = ft_find(opts, "SOMESHARDS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "SOMESHARDS");
    }

    if ((z = ft_find(opts, "CONSISTENT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "CONSISTENT");
    } else if ((z = ft_find(opts, "INCONSISTENT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "INCONSISTENT");
    }

    if ((z = ft_find(opts, "DIALECT"))) {
        FT_BUF_LIT(buf, "DIALECT");
        ft_buf_add_zval(buf, z);
    }
}

/* ====================================================================
 * FT.INFO OPTIONS BUILDER
 *
 * $options = [
 *     'scope'         => 'LOCAL',       // 'LOCAL', 'PRIMARY', or 'CLUSTER'
 *     'ALLSHARDS'     => true,          // or 'SOMESHARDS' => true
 *     'CONSISTENT'    => true,          // or 'INCONSISTENT' => true
 * ]
 * ==================================================================== */

static void ft_build_info_options(ft_arg_buf_t* buf, HashTable* opts) {
    zval* z;

    if ((z = ft_find(opts, "scope")) && Z_TYPE_P(z) == IS_STRING) {
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(opts, "ALLSHARDS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "ALLSHARDS");
    } else if ((z = ft_find(opts, "SOMESHARDS")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "SOMESHARDS");
    }

    if ((z = ft_find(opts, "CONSISTENT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "CONSISTENT");
    } else if ((z = ft_find(opts, "INCONSISTENT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "INCONSISTENT");
    }
}

/* ====================================================================
 * PUBLIC: build_ft_create_args
 *
 * Builds the full argument list for FT.CREATE from structured PHP
 * arrays and writes the result into the provided output pointers.
 * Returns 1 on success, 0 on failure (exception thrown).
 * ==================================================================== */

int build_ft_create_args(const char*   index_name,
                         size_t        index_name_len,
                         HashTable*    schema_ht,
                         HashTable*    options_ht,
                         const char*** out_strings,
                         size_t**      out_lengths,
                         int*          out_count,
                         char***       out_allocated,
                         int*          out_alloc_count) {
    ft_arg_buf_t buf;
    ft_buf_init(&buf);

    /* index name */
    ft_buf_add(&buf, index_name, index_name_len);

    /* options (before SCHEMA) */
    if (options_ht) {
        ft_build_create_options(&buf, options_ht);
    }

    /* SCHEMA keyword */
    FT_BUF_LIT(&buf, "SCHEMA");

    /* schema fields */
    zval* field_zv;
    ZEND_HASH_FOREACH_VAL(schema_ht, field_zv) {
        if (Z_TYPE_P(field_zv) != IS_ARRAY) {
            zend_throw_exception(get_valkey_glide_exception_ce(),
                                 "ftCreate: each schema element must be an array",
                                 0);
            ft_buf_destroy(&buf);
            return 0;
        }
        if (!ft_build_field(&buf, Z_ARRVAL_P(field_zv))) {
            ft_buf_destroy(&buf);
            return 0;
        }
    }
    ZEND_HASH_FOREACH_END();

    /* Transfer ownership to caller */
    *out_strings     = buf.strings;
    *out_lengths     = buf.lengths;
    *out_count       = buf.count;
    *out_allocated   = buf.allocated;
    *out_alloc_count = buf.alloc_count;
    return 1;
}

/* ====================================================================
 * PUBLIC: build_ft_search_args
 * ==================================================================== */

int build_ft_search_args(const char*   index_name,
                         size_t        index_name_len,
                         const char*   query,
                         size_t        query_len,
                         HashTable*    options_ht,
                         const char*** out_strings,
                         size_t**      out_lengths,
                         int*          out_count,
                         char***       out_allocated,
                         int*          out_alloc_count) {
    ft_arg_buf_t buf;
    ft_buf_init(&buf);

    ft_buf_add(&buf, index_name, index_name_len);
    ft_buf_add(&buf, query, query_len);

    if (options_ht) {
        ft_build_search_options(&buf, options_ht);
    }

    *out_strings     = buf.strings;
    *out_lengths     = buf.lengths;
    *out_count       = buf.count;
    *out_allocated   = buf.allocated;
    *out_alloc_count = buf.alloc_count;
    return 1;
}

/* ====================================================================
 * PUBLIC: build_ft_aggregate_args
 *
 * Builds the argument list for FT.AGGREGATE: index, query, then
 * the flat options array appended as-is.
 * ==================================================================== */

int build_ft_aggregate_args(const char*   index_name,
                            size_t        index_name_len,
                            const char*   query,
                            size_t        query_len,
                            HashTable*    options_ht,
                            const char*** out_strings,
                            size_t**      out_lengths,
                            int*          out_count,
                            char***       out_allocated,
                            int*          out_alloc_count) {
    ft_arg_buf_t buf;
    ft_buf_init(&buf);

    ft_buf_add(&buf, index_name, index_name_len);
    ft_buf_add(&buf, query, query_len);

    if (options_ht) {
        zval* entry;
        ZEND_HASH_FOREACH_VAL(options_ht, entry) {
            ft_buf_add_zval(&buf, entry);
        }
        ZEND_HASH_FOREACH_END();
    }

    *out_strings     = buf.strings;
    *out_lengths     = buf.lengths;
    *out_count       = buf.count;
    *out_allocated   = buf.allocated;
    *out_alloc_count = buf.alloc_count;
    return 1;
}

/* ====================================================================
 * PUBLIC: build_ft_info_args
 * ==================================================================== */

int build_ft_info_args(const char*   index_name,
                       size_t        index_name_len,
                       HashTable*    options_ht,
                       const char*** out_strings,
                       size_t**      out_lengths,
                       int*          out_count,
                       char***       out_allocated,
                       int*          out_alloc_count) {
    ft_arg_buf_t buf;
    ft_buf_init(&buf);

    ft_buf_add(&buf, index_name, index_name_len);

    if (options_ht) {
        ft_build_info_options(&buf, options_ht);
    }

    *out_strings     = buf.strings;
    *out_lengths     = buf.lengths;
    *out_count       = buf.count;
    *out_allocated   = buf.allocated;
    *out_alloc_count = buf.alloc_count;
    return 1;
}
