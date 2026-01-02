/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_script_common.h"

#include "common.h"

// Helper function to process array arguments
char** process_array_to_args(zval* array, int* count) {
    if (Z_TYPE_P(array) != IS_ARRAY) {
        *count = 0;
        return NULL;
    }

    *count = zend_hash_num_elements(Z_ARRVAL_P(array));
    if (*count == 0) {
        return NULL;
    }

    char** args = emalloc(sizeof(char*) * (*count));
    zval*  entry;
    int    i = 0;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
        convert_to_string(entry);
        args[i] = estrdup(Z_STRVAL_P(entry));
        i++;
    }
    ZEND_HASH_FOREACH_END();

    return args;
}
