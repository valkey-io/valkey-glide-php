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

#include <math.h>
#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_strtod.h>

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
        throw_command_error(result->command_error);
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

static void ft_buf_add_zval(ft_arg_buf_t* buf, zval* z);

/*
 * Format a double the way Python's str(float) does: the shortest
 * decimal string that round-trips back to the same double, with at
 * least one digit after a literal decimal point for non-exponential
 * values (Python str(60.0) == "60.0", never "60"). Uses zend_dtoa's
 * mode 0 (the classic David Gay dtoa() "shortest round-trip digit
 * string" mode -- the same primitive var_export()/serialize_precision
 * = -1 ultimately call through to), rather than the fixed %.6g used by
 * ft_buf_add_double() above, which truncates past 6 significant digits
 * and silently diverges from Python beyond that (confirmed:
 * 0.123456789 -> "0.123457" under %.6g vs Python's "0.123456789").
 * zend_gcvt(value, 0, ...) was tried first and rejected: its `ndigit`
 * parameter is a literal significant-digit count, not a "shortest
 * mode" flag, so 0 silently clamped to a minimal digit count instead of
 * requesting the shortest round-trip string (confirmed via a live
 * FT.HYBRID argv capture: ALPHA 0.123456789 came out as "0.1", CONSTANT
 * 1234567.0 as "1e+6" -- both wrong). zend_dtoa mode 0 does not have
 * this problem: ndigits is ignored in mode 0, and decpt/sign come back
 * separately for the caller to place the digits correctly instead of
 * baking in a fixed format.
 *
 * Used only where argv must byte-match Python's FT.HYBRID output
 * (CONSTANT, ALPHA, BETA, SHARD_K_RATIO), confirmed via a direct argv
 * capture (MONITOR) compared against the real FtHybridOptions classes.
 */
static void ft_buf_add_double_py_str(ft_arg_buf_t* buf, double val) {
    char tmp[64];
    int  out_len = 0;

    if (val == 0.0) {
        /* zend_dtoa's digit string for 0.0 is "0" with decpt 1; Python
         * prints "-0.0" for negative zero and "0.0" otherwise -- handle
         * the sign bit directly rather than relying on dtoa's decpt=1
         * convention for this one special case. */
        if (signbit(val)) {
            tmp[out_len++] = '-';
        }
        tmp[out_len++] = '0';
        tmp[out_len++] = '.';
        tmp[out_len++] = '0';
        ft_buf_add_copy(buf, tmp, out_len);
        return;
    }

    int   decpt;
    bool  sign;
    char* rve;
    char* digits  = zend_dtoa(val, 0, 0, &decpt, &sign, &rve);
    int   ndigits = (int) (rve - digits);

    if (sign) {
        tmp[out_len++] = '-';
    }

    /* Python's float repr switches to scientific notation outside
     * roughly [1e-4, 1e17) in decimal-point terms; dtoa's own decpt
     * (the power-of-ten position of the decimal point relative to the
     * first digit) maps directly onto that same threshold. */
    if (decpt < -3 || decpt > 16) {
        /* Scientific: d.dddde+NN (always a sign, lowercase 'e', and at
         * least one digit after the point even if ndigits == 1 --
         * confirmed Python never omits it: str(1e20) == '1e+20' has no
         * forced ".0", so only force a trailing ".0" when ndigits > 1). */
        tmp[out_len++] = digits[0];
        if (ndigits > 1) {
            tmp[out_len++] = '.';
            for (int i = 1; i < ndigits; i++) {
                tmp[out_len++] = digits[i];
            }
        }
        tmp[out_len++] = 'e';
        int exp        = decpt - 1;
        tmp[out_len++] = (exp < 0) ? '-' : '+';
        char expbuf[8];
        int  explen = snprintf(expbuf, sizeof(expbuf), "%02d", exp < 0 ? -exp : exp);
        memcpy(tmp + out_len, expbuf, (size_t) explen);
        out_len += explen;
    } else if (decpt <= 0) {
        /* 0.000ddd form. */
        tmp[out_len++] = '0';
        tmp[out_len++] = '.';
        for (int i = 0; i < -decpt; i++) {
            tmp[out_len++] = '0';
        }
        memcpy(tmp + out_len, digits, (size_t) ndigits);
        out_len += ndigits;
    } else if (decpt >= ndigits) {
        /* All significant digits are before the point; pad with
         * trailing zeros, then the mandatory ".0" Python always keeps
         * for a whole-number float. */
        memcpy(tmp + out_len, digits, (size_t) ndigits);
        out_len += ndigits;
        for (int i = 0; i < decpt - ndigits; i++) {
            tmp[out_len++] = '0';
        }
        tmp[out_len++] = '.';
        tmp[out_len++] = '0';
    } else {
        /* The point falls inside the digit string. */
        memcpy(tmp + out_len, digits, (size_t) decpt);
        out_len += decpt;
        tmp[out_len++] = '.';
        memcpy(tmp + out_len, digits + decpt, (size_t) (ndigits - decpt));
        out_len += ndigits - decpt;
    }

    zend_freedtoa(digits);
    ft_buf_add_copy(buf, tmp, out_len);
}

/* Like ft_buf_add_double_py_str(), but takes a zval (LONG or DOUBLE;
 * anything else falls back to the generic zval stringifier). */
static void ft_buf_add_zval_py_float(ft_arg_buf_t* buf, zval* z) {
    if (Z_TYPE_P(z) == IS_DOUBLE) {
        ft_buf_add_double_py_str(buf, Z_DVAL_P(z));
    } else if (Z_TYPE_P(z) == IS_LONG) {
        /* An integer-typed PHP value (e.g. constant: 60, not 60.0) has
         * no float representation to match -- Python's own FtHybridRrf
         * etc. take a plain float parameter, so an int argument there
         * would already be int-typed on the Python side too and
         * stringify the same way as PHP's IS_LONG path. */
        ft_buf_add_long(buf, Z_LVAL_P(z));
    } else {
        ft_buf_add_zval(buf, z);
    }
}

/* Serialize a zval of any scalar type as an argv token: strings and
 * already-stringified longs/doubles are added directly; any other type goes
 * through convert_to_string() first (PHP's own scalar-to-string rules). */
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

/* Free every allocation ft_buf_add_zval()/ft_buf_add_double_py_str() made into
 * buf->allocated, then the buffer's own arrays. Does not free buf itself. */
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

/* Serialize one FT.CREATE SCHEMA field entry (name, optional AS alias, type,
 * and type-specific options) into buf. Returns 0 and throws if 'name' or
 * 'type' is missing or not a string; returns 1 on success. */
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
        /* TYPE: FLOAT32 (default), FLOAT16, or BFLOAT16 (valkey-search
         * 1.3.0, src/indexes/vector_base.h). FtVectorField::toArray()
         * always sends 'vector_type' (defaulting to 'FLOAT32' itself),
         * so this always has a value; the FLOAT32 literal fallback stays
         * only in case a hand-built options array omits the key. */
        z = ft_find(field, "vector_type");
        if (z && Z_TYPE_P(z) == IS_STRING) {
            FT_BUF_LIT(&attrs, "TYPE");
            ft_buf_add(&attrs, Z_STRVAL_P(z), Z_STRLEN_P(z));
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

    /* SORTABLE (common to TEXT, TAG, NUMERIC, VECTOR). UNF is only
     * recognised by the server directly after SORTABLE (valkey-search
     * 1.3.0, src/commands/ft_create_parser.cc: "UNF is only recognized
     * directly after SORTABLE, as in Redis") -- so it is emitted here,
     * immediately after the SORTABLE token, and only when 'sortable' is
     * also true. VECTOR fields accept SORTABLE for RediSearch-client
     * compatibility (it has no effect) but never UNF; the PHP builder
     * (FtVectorField) has no unf() method, so 'unf' never appears
     * alongside a VECTOR schema entry in practice. */
    if ((z = ft_find(field, "sortable")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "SORTABLE");
        if ((z = ft_find(field, "unf")) && zend_is_true(z)) {
            FT_BUF_LIT(buf, "UNF");
        }
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

/* Serialize the FT.CREATE options array (ON, PREFIX, FILTER, LANGUAGE,
 * SCORE_FIELD, NOHL, and the FT 1.2-era text-indexing flags) into buf. */
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

    /* FILTER, SCORE_FIELD, NOHL (valkey-search 1.3.0,
     * src/commands/ft_create_parser.cc). */
    if ((z = ft_find(opts, "FILTER")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "FILTER");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(opts, "SCORE_FIELD")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "SCORE_FIELD");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(opts, "NOHL")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "NOHL");
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
 *     'COUNT'        => true,       // mutually exclusive with LIMIT; maps to LIMIT 0 0
 *     'SORTBY'       => ['price', 'ASC'],
 *     'WITHSORTKEYS' => true,
 *     'RETURN'       => ['title', 'price'],
 *     'TIMEOUT'      => 5000,
 *     'PARAMS'       => ['query_vec' => $vec],
 *     'DIALECT'      => 2,
 *     'ALLSHARDS'    => true,       // or 'SOMESHARDS' => true
 *     'CONSISTENT'   => true,       // or 'INCONSISTENT' => true
 *     'INKEYS'       => ['key1', 'key2'],   // valkey-search 1.3.0
 *     'WITHSCORES'   => true,               // valkey-search 1.3.0
 *     'SCORER'       => 'BM25STD',          // valkey-search 1.3.0
 * ];
 * ==================================================================== */

/* Serialize the FT.SEARCH options array (NOCONTENT, VERBATIM, INORDER,
 * SLOP, INKEYS, WITHSCORES, SCORER, and the other FT.SEARCH flags) into
 * buf. */
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

    /* valkey-search 1.3.0: INKEYS restricts the search to this key set.
     * An explicit empty array is distinct from omitting the option
     * entirely: [] sends INKEYS 0 (excludes every result), while a
     * missing/absent key omits INKEYS altogether (no restriction). */
    if ((z = ft_find(opts, "INKEYS")) && Z_TYPE_P(z) == IS_ARRAY) {
        HashTable* inkeys = Z_ARRVAL_P(z);
        FT_BUF_LIT(buf, "INKEYS");
        ft_buf_add_long(buf, zend_hash_num_elements(inkeys));
        zval* k;
        ZEND_HASH_FOREACH_VAL(inkeys, k) {
            ft_buf_add_zval(buf, k);
        }
        ZEND_HASH_FOREACH_END();
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

    /* valkey-search 1.3.0: COUNT is not a real FT.SEARCH keyword -- the
     * server rejects a bare COUNT token outright. The count-only form it
     * actually recognises is LIMIT 0 0. Mutually exclusive with LIMIT
     * (both control the same clause); the PHP-side builder is responsible
     * for rejecting both being set together before this runs. */
    if ((z = ft_find(opts, "COUNT")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "LIMIT");
        FT_BUF_LIT(buf, "0");
        FT_BUF_LIT(buf, "0");
    }

    if ((z = ft_find(opts, "WITHSCORES")) && zend_is_true(z)) {
        FT_BUF_LIT(buf, "WITHSCORES");
    }

    if ((z = ft_find(opts, "SCORER")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "SCORER");
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

/* Serialize the FT.INFO options array (scope, ALLSHARDS/SOMESHARDS,
 * CONSISTENT/INCONSISTENT) into buf. */
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

/* ====================================================================
 * FT.HYBRID ARGUMENT BUILDER (valkey-search 1.3.0)
 *
 * $options (from FtHybridBuilder::toArray()) shape:
 *   [
 *     'search' => ['query' => string, 'scorer' => ?string, 'yieldScoreAs' => ?string],
 *     'vsim'   => [
 *         'field' => string, 'vector' => mixed, 'isParamRef' => bool,
 *         'k' => ?int, 'efRuntime' => ?int, 'shardKRatio' => ?float,
 *         'yieldScoreAs' => ?string,
 *         'filter' => ?['expression' => string, 'policy' => ?string, 'batchSize' => ?int],
 *     ],
 *     'combine'      => ?['method' => 'RRF'|'LINEAR'|'FUNCTION', ...],
 *     'loadAll'      => bool,
 *     'loadFields'   => ?array,   // each entry: string, or ['field'=>..,'as'=>..]
 *     'clauseTokens' => array<string>,  // FILTER/LIMIT/GROUPBY/SORTBY/APPLY, pre-flattened
 *     'withCursor'   => ?['count' => ?int, 'maxidle' => ?int],
 *     'localOnly'    => bool,
 *     'timeout'      => ?int,
 *     'params'       => ?array,   // name => value
 *     'dialect'      => ?int,
 *   ]
 *
 * Param reference by type (port rule): whether a VSIM
 * vector value is a "$name" reference or raw vector bytes is decided
 * ENTIRELY by 'isParamRef', which FtHybridBuilder::vsim() sets based on
 * WHICH of its two mutually exclusive parameters the caller passed
 * ($vector for raw bytes, $paramRef for an explicit reference) -- never
 * by inspecting the string's content. PHP has no separate "binary"
 * string type (IS_STRING covers both text and opaque binary data), so a
 * packed vector's bytes can legitimately start with 0x24 ('$') by pure
 * coincidence; a "does this start with $" content check would
 * misclassify that vector as a parameter reference, exactly the bug the
 * port rules warn about. This C layer never re-inspects 'vector's byte
 * content to make that decision -- it trusts 'isParamRef' verbatim. If
 * 'isParamRef' is false, this builder always synthesizes an internal
 * PARAMS entry, regardless of what the first byte of 'vector' is.
 * ==================================================================== */

#define FT_HYBRID_GENERATED_VSIM_PARAM_NAME "__glide_vsim_vector"

/* Serialize the FILTER sub-clause of a VSIM arm into `buf`. */
static void ft_hybrid_build_vsim_filter(ft_arg_buf_t* buf, HashTable* filter_ht) {
    zval* z_expr = ft_find(filter_ht, "expression");
    if (!z_expr) {
        return;
    }
    zval* z_policy = ft_find(filter_ht, "policy");
    zval* z_batch  = ft_find(filter_ht, "batchSize");

    int has_policy = z_policy && Z_TYPE_P(z_policy) == IS_STRING && Z_STRLEN_P(z_policy) > 0;
    int has_batch  = z_batch && Z_TYPE_P(z_batch) != IS_NULL;

    if (!has_policy && !has_batch) {
        /* Uncounted extension: a bare expression. */
        FT_BUF_LIT(buf, "FILTER");
        ft_buf_add_zval(buf, z_expr);
        return;
    }

    /* Counted form: FILTER <token_count> <expr> [POLICY p] [BATCH_SIZE n] */
    ft_arg_buf_t sub;
    ft_buf_init(&sub);
    if (has_policy) {
        FT_BUF_LIT(&sub, "POLICY");
        ft_buf_add(&sub, Z_STRVAL_P(z_policy), Z_STRLEN_P(z_policy));
    }
    if (has_batch) {
        FT_BUF_LIT(&sub, "BATCH_SIZE");
        ft_buf_add_zval(&sub, z_batch);
    }
    FT_BUF_LIT(buf, "FILTER");
    /* token_count = 1 (the expression) + len(sub) */
    ft_buf_add_long(buf, 1 + sub.count);
    ft_buf_add_zval(buf, z_expr);
    for (int i = 0; i < sub.count; i++) {
        ft_buf_add_copy(buf, sub.strings[i], sub.lengths[i]);
    }
    ft_buf_destroy(&sub);
}

/* Serialize the SEARCH arm: SEARCH <query> [SCORER s] [YIELD_SCORE_AS alias] */
static void ft_hybrid_build_search_arm(ft_arg_buf_t* buf, HashTable* search_ht) {
    FT_BUF_LIT(buf, "SEARCH");
    zval* z_query = ft_find(search_ht, "query");
    ft_buf_add_zval(buf, z_query);

    zval* z;
    if ((z = ft_find(search_ht, "scorer")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "SCORER");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }
    if ((z = ft_find(search_ht, "yieldScoreAs")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "YIELD_SCORE_AS");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }
}

/*
 * Serialize the VSIM arm: VSIM @field <$param> [KNN n [K k] [EF_RUNTIME e]
 * [SHARD_K_RATIO r]] [YIELD_SCORE_AS alias] [FILTER ...]
 *
 * Returns 1 if a PARAMS entry must be synthesized for the vector (i.e.
 * 'isParamRef' was false), 0 if the caller already supplied a "$name"
 * reference. The generated param's raw value is written to
 * *out_generated_value (borrowed from the input zval; not copied).
 */
static int ft_hybrid_build_vsim_arm(ft_arg_buf_t* buf,
                                    HashTable*    vsim_ht,
                                    zval**        out_generated_value) {
    zval* z_field        = ft_find(vsim_ht, "field");
    zval* z_vector       = ft_find(vsim_ht, "vector");
    zval* z_is_param_ref = ft_find(vsim_ht, "isParamRef");
    int   is_param_ref   = z_is_param_ref && zend_is_true(z_is_param_ref);

    FT_BUF_LIT(buf, "VSIM");

    /* @field, with the leading @ always added here (builder stores the
     * bare identifier without it). */
    if (z_field && Z_TYPE_P(z_field) == IS_STRING) {
        char* at_field = emalloc(Z_STRLEN_P(z_field) + 2);
        at_field[0]    = '@';
        memcpy(at_field + 1, Z_STRVAL_P(z_field), Z_STRLEN_P(z_field));
        size_t at_len = Z_STRLEN_P(z_field) + 1;
        ft_buf_add_copy(buf, at_field, at_len);
        efree(at_field);
    }

    int generated = 0;
    if (is_param_ref && z_vector && Z_TYPE_P(z_vector) == IS_STRING) {
        /* Caller already wrote an explicit "$name" string -- never
         * re-derive this from byte content. */
        ft_buf_add(buf, Z_STRVAL_P(z_vector), Z_STRLEN_P(z_vector));
    } else {
        /* Raw vector data (bytes), regardless of its first byte --
         * synthesize the internal PARAMS reference. */
        FT_BUF_LIT(buf, "$" FT_HYBRID_GENERATED_VSIM_PARAM_NAME);
        generated = 1;
        if (out_generated_value) {
            *out_generated_value = z_vector;
        }
    }

    zval* z_k           = ft_find(vsim_ht, "k");
    zval* z_ef_runtime  = ft_find(vsim_ht, "efRuntime");
    zval* z_shard_ratio = ft_find(vsim_ht, "shardKRatio");
    int   has_knn       = (z_k && Z_TYPE_P(z_k) != IS_NULL) ||
                  (z_ef_runtime && Z_TYPE_P(z_ef_runtime) != IS_NULL) ||
                  (z_shard_ratio && Z_TYPE_P(z_shard_ratio) != IS_NULL);
    if (has_knn) {
        ft_arg_buf_t sub;
        ft_buf_init(&sub);
        if (z_k && Z_TYPE_P(z_k) != IS_NULL) {
            FT_BUF_LIT(&sub, "K");
            ft_buf_add_zval(&sub, z_k);
        }
        if (z_ef_runtime && Z_TYPE_P(z_ef_runtime) != IS_NULL) {
            FT_BUF_LIT(&sub, "EF_RUNTIME");
            ft_buf_add_zval(&sub, z_ef_runtime);
        }
        if (z_shard_ratio && Z_TYPE_P(z_shard_ratio) != IS_NULL) {
            FT_BUF_LIT(&sub, "SHARD_K_RATIO");
            ft_buf_add_zval_py_float(&sub, z_shard_ratio);
        }
        FT_BUF_LIT(buf, "KNN");
        ft_buf_add_long(buf, sub.count);
        for (int i = 0; i < sub.count; i++) {
            ft_buf_add_copy(buf, sub.strings[i], sub.lengths[i]);
        }
        ft_buf_destroy(&sub);
    }

    zval* z;
    if ((z = ft_find(vsim_ht, "yieldScoreAs")) && Z_TYPE_P(z) == IS_STRING) {
        FT_BUF_LIT(buf, "YIELD_SCORE_AS");
        ft_buf_add(buf, Z_STRVAL_P(z), Z_STRLEN_P(z));
    }

    if ((z = ft_find(vsim_ht, "filter")) && Z_TYPE_P(z) == IS_ARRAY) {
        ft_hybrid_build_vsim_filter(buf, Z_ARRVAL_P(z));
    }

    return generated;
}

/* Serialize the optional COMBINE clause. */
static void ft_hybrid_build_combine(ft_arg_buf_t* buf, HashTable* combine_ht) {
    zval* z_method = ft_find(combine_ht, "method");
    if (!z_method || Z_TYPE_P(z_method) != IS_STRING) {
        return;
    }
    const char* method = Z_STRVAL_P(z_method);

    ft_arg_buf_t sub;
    ft_buf_init(&sub);
    zval* z;

    if (strcasecmp(method, "RRF") == 0) {
        if ((z = ft_find(combine_ht, "constant")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "CONSTANT");
            ft_buf_add_zval_py_float(&sub, z);
        }
        if ((z = ft_find(combine_ht, "window")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "WINDOW");
            ft_buf_add_zval(&sub, z);
        }
        if ((z = ft_find(combine_ht, "yieldScoreAs")) && Z_TYPE_P(z) == IS_STRING) {
            FT_BUF_LIT(&sub, "YIELD_SCORE_AS");
            ft_buf_add(&sub, Z_STRVAL_P(z), Z_STRLEN_P(z));
        }
        FT_BUF_LIT(buf, "COMBINE");
        FT_BUF_LIT(buf, "RRF");
    } else if (strcasecmp(method, "LINEAR") == 0) {
        if ((z = ft_find(combine_ht, "alpha")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "ALPHA");
            ft_buf_add_zval_py_float(&sub, z);
        }
        if ((z = ft_find(combine_ht, "beta")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "BETA");
            ft_buf_add_zval_py_float(&sub, z);
        }
        if ((z = ft_find(combine_ht, "window")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "WINDOW");
            ft_buf_add_zval(&sub, z);
        }
        if ((z = ft_find(combine_ht, "yieldScoreAs")) && Z_TYPE_P(z) == IS_STRING) {
            FT_BUF_LIT(&sub, "YIELD_SCORE_AS");
            ft_buf_add(&sub, Z_STRVAL_P(z), Z_STRLEN_P(z));
        }
        FT_BUF_LIT(buf, "COMBINE");
        FT_BUF_LIT(buf, "LINEAR");
    } else if (strcasecmp(method, "FUNCTION") == 0) {
        zval* z_expr = ft_find(combine_ht, "expr");
        FT_BUF_LIT(&sub, "EXPR");
        if (z_expr) {
            ft_buf_add_zval(&sub, z_expr);
        }
        if ((z = ft_find(combine_ht, "window")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&sub, "WINDOW");
            ft_buf_add_zval(&sub, z);
        }
        if ((z = ft_find(combine_ht, "yieldScoreAs")) && Z_TYPE_P(z) == IS_STRING) {
            FT_BUF_LIT(&sub, "YIELD_SCORE_AS");
            ft_buf_add(&sub, Z_STRVAL_P(z), Z_STRLEN_P(z));
        }
        FT_BUF_LIT(buf, "COMBINE");
        FT_BUF_LIT(buf, "FUNCTION");
    } else {
        ft_buf_destroy(&sub);
        return;
    }

    ft_buf_add_long(buf, sub.count);
    for (int i = 0; i < sub.count; i++) {
        ft_buf_add_copy(buf, sub.strings[i], sub.lengths[i]);
    }
    ft_buf_destroy(&sub);
}

/* Serialize the LOAD clause (shared grammar with FT.AGGREGATE). */
static void ft_hybrid_build_load(ft_arg_buf_t* buf, zval* z_load_all, HashTable* load_fields_ht) {
    if (z_load_all && zend_is_true(z_load_all)) {
        FT_BUF_LIT(buf, "LOAD");
        FT_BUF_LIT(buf, "*");
        return;
    }
    if (!load_fields_ht || zend_hash_num_elements(load_fields_ht) == 0) {
        return;
    }

    ft_arg_buf_t sub;
    ft_buf_init(&sub);
    zval* entry;
    ZEND_HASH_FOREACH_VAL(load_fields_ht, entry) {
        if (Z_TYPE_P(entry) == IS_ARRAY) {
            HashTable* entry_ht = Z_ARRVAL_P(entry);
            zval*      z_field  = ft_find(entry_ht, "field");
            zval*      z_as     = ft_find(entry_ht, "as");
            if (z_field) {
                ft_buf_add_zval(&sub, z_field);
            }
            if (z_as && Z_TYPE_P(z_as) == IS_STRING) {
                FT_BUF_LIT(&sub, "AS");
                ft_buf_add(&sub, Z_STRVAL_P(z_as), Z_STRLEN_P(z_as));
            }
        } else {
            ft_buf_add_zval(&sub, entry);
        }
    }
    ZEND_HASH_FOREACH_END();

    FT_BUF_LIT(buf, "LOAD");
    ft_buf_add_long(buf, sub.count);
    for (int i = 0; i < sub.count; i++) {
        ft_buf_add_copy(buf, sub.strings[i], sub.lengths[i]);
    }
    ft_buf_destroy(&sub);
}

/*
 * PUBLIC: build_ft_hybrid_args
 *
 * Wire order (port rule, confirmed live in ft_hybrid_parser.cc):
 *   index, SEARCH arm, VSIM arm, [COMBINE], [LOCALONLY],
 *   [LOAD], [TIMEOUT], PARAMS (always present -- the VSIM vector always
 *   needs either a generated or caller-supplied entry), <clauseTokens>,
 *   [WITHCURSOR], [DIALECT].
 *
 * LOCALONLY must come before PARAMS (the server rejects it once PARAMS
 * has been parsed, regardless of what follows). clauseTokens must come
 * AFTER PARAMS -- a leading FILTER clause emitted right after the VSIM
 * arm would otherwise be claimed by the server's VSIM parser as a VSIM
 * pre-filter instead of an aggregate-pipeline FILTER (the #7335 FT.HYBRID
 * tail-order port rule).
 */
int build_ft_hybrid_args(const char*   index_name,
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

    zval* z_search = ft_find(options_ht, "search");
    if (!z_search || Z_TYPE_P(z_search) != IS_ARRAY) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "ftHybrid: options must include a 'search' array", 0);
        ft_buf_destroy(&buf);
        return 0;
    }
    if (!ft_find(Z_ARRVAL_P(z_search), "query")) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftHybrid: 'search' array must include a 'query' string",
                             0);
        ft_buf_destroy(&buf);
        return 0;
    }
    ft_hybrid_build_search_arm(&buf, Z_ARRVAL_P(z_search));

    zval* z_vsim = ft_find(options_ht, "vsim");
    if (!z_vsim || Z_TYPE_P(z_vsim) != IS_ARRAY) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "ftHybrid: options must include a 'vsim' array", 0);
        ft_buf_destroy(&buf);
        return 0;
    }
    zval* generated_vsim_value = NULL;
    int   needs_generated_param =
        ft_hybrid_build_vsim_arm(&buf, Z_ARRVAL_P(z_vsim), &generated_vsim_value);

    zval* z_combine = ft_find(options_ht, "combine");
    if (z_combine && Z_TYPE_P(z_combine) == IS_ARRAY) {
        ft_hybrid_build_combine(&buf, Z_ARRVAL_P(z_combine));
    }

    zval* z_local_only = ft_find(options_ht, "localOnly");
    if (z_local_only && zend_is_true(z_local_only)) {
        FT_BUF_LIT(&buf, "LOCALONLY");
    }

    zval* z_load_all    = ft_find(options_ht, "loadAll");
    zval* z_load_fields = ft_find(options_ht, "loadFields");
    ft_hybrid_build_load(
        &buf,
        z_load_all,
        (z_load_fields && Z_TYPE_P(z_load_fields) == IS_ARRAY) ? Z_ARRVAL_P(z_load_fields) : NULL);

    zval* z_timeout = ft_find(options_ht, "timeout");
    if (z_timeout && Z_TYPE_P(z_timeout) != IS_NULL) {
        FT_BUF_LIT(&buf, "TIMEOUT");
        ft_buf_add_zval(&buf, z_timeout);
    }

    /* PARAMS: merge caller-supplied params with the generated VSIM entry
     * (if needed). A name collision between the generated name and a
     * caller-supplied entry is an error, mirroring Python's
     * _merge_params(). */
    zval*      z_params = ft_find(options_ht, "params");
    HashTable* params_ht =
        (z_params && Z_TYPE_P(z_params) == IS_ARRAY) ? Z_ARRVAL_P(z_params) : NULL;
    int nparams     = params_ht ? zend_hash_num_elements(params_ht) : 0;
    int total_pairs = nparams + (needs_generated_param ? 1 : 0);

    if (needs_generated_param && params_ht &&
        zend_hash_str_exists(params_ht,
                             FT_HYBRID_GENERATED_VSIM_PARAM_NAME,
                             sizeof(FT_HYBRID_GENERATED_VSIM_PARAM_NAME) - 1)) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "ftHybrid: params contains a name that collides with "
                             "the internally generated VSIM parameter name; rename it",
                             0);
        ft_buf_destroy(&buf);
        return 0;
    }

    /* PARAMS is always present for FT.HYBRID -- the VSIM arm always
     * references one entry, generated or caller-supplied. */
    FT_BUF_LIT(&buf, "PARAMS");
    ft_buf_add_long(&buf, total_pairs * 2);
    if (params_ht) {
        zend_ulong   num_key;
        zend_string* key;
        zval*        val;
        ZEND_HASH_FOREACH_KEY_VAL(params_ht, num_key, key, val) {
            if (key) {
                ft_buf_add(&buf, ZSTR_VAL(key), ZSTR_LEN(key));
            } else {
                ft_buf_add_long(&buf, (zend_long) num_key);
            }
            ft_buf_add_zval(&buf, val);
        }
        ZEND_HASH_FOREACH_END();
    }
    if (needs_generated_param) {
        FT_BUF_LIT(&buf, FT_HYBRID_GENERATED_VSIM_PARAM_NAME);
        if (generated_vsim_value) {
            ft_buf_add_zval(&buf, generated_vsim_value);
        } else {
            FT_BUF_LIT(&buf, "");
        }
    }

    /* Aggregate-suffix clause tokens (FILTER/LIMIT/GROUPBY/SORTBY/APPLY),
     * pre-flattened by the PHP builder. Must come after PARAMS. */
    zval* z_clause_tokens = ft_find(options_ht, "clauseTokens");
    if (z_clause_tokens && Z_TYPE_P(z_clause_tokens) == IS_ARRAY) {
        zval* token;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_clause_tokens), token) {
            ft_buf_add_zval(&buf, token);
        }
        ZEND_HASH_FOREACH_END();
    }

    zval* z_with_cursor = ft_find(options_ht, "withCursor");
    if (z_with_cursor && Z_TYPE_P(z_with_cursor) == IS_ARRAY) {
        HashTable* wc_ht = Z_ARRVAL_P(z_with_cursor);
        FT_BUF_LIT(&buf, "WITHCURSOR");
        zval* z;
        if ((z = ft_find(wc_ht, "count")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&buf, "COUNT");
            ft_buf_add_zval(&buf, z);
        }
        if ((z = ft_find(wc_ht, "maxidle")) && Z_TYPE_P(z) != IS_NULL) {
            FT_BUF_LIT(&buf, "MAXIDLE");
            ft_buf_add_zval(&buf, z);
        }
    }

    zval* z_dialect = ft_find(options_ht, "dialect");
    if (z_dialect && Z_TYPE_P(z_dialect) != IS_NULL) {
        FT_BUF_LIT(&buf, "DIALECT");
        ft_buf_add_zval(&buf, z_dialect);
    }

    *out_strings     = buf.strings;
    *out_lengths     = buf.lengths;
    *out_count       = buf.count;
    *out_allocated   = buf.allocated;
    *out_alloc_count = buf.alloc_count;
    return 1;
}
