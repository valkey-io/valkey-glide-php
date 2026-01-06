#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_script_common.h"
#include "zend_exceptions.h"

// Helper to convert string array to FFI format
static void prepare_ffi_args(zval*           array,
                             uintptr_t**     ptrs,
                             unsigned long** lens,
                             unsigned long*  count) {
    if (Z_TYPE_P(array) != IS_ARRAY) {
        *count = 0;
        *ptrs  = NULL;
        *lens  = NULL;
        return;
    }

    *count = zend_hash_num_elements(Z_ARRVAL_P(array));
    if (*count == 0) {
        *ptrs = NULL;
        *lens = NULL;
        return;
    }

    *ptrs = emalloc(sizeof(uintptr_t) * (*count));
    *lens = emalloc(sizeof(unsigned long) * (*count));

    zval* entry;
    int   i = 0;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
        convert_to_string(entry);
        (*ptrs)[i] = (uintptr_t) Z_STRVAL_P(entry);
        (*lens)[i] = Z_STRLEN_P(entry);
        i++;
    }
    ZEND_HASH_FOREACH_END();
}

// Script execution using invoke_script FFI (following Go pattern)
void execute_invoke_script_command(valkey_glide_object* valkey_glide,
                                   const char*          script_hash,
                                   zval*                keys,
                                   zval*                args,
                                   zval*                return_value,
                                   bool                 use_false_if_null) {
    // Prepare FFI arguments
    uintptr_t *    key_ptrs, *arg_ptrs;
    unsigned long *key_lens, *arg_lens;
    unsigned long  key_count, arg_count;

    prepare_ffi_args(keys, &key_ptrs, &key_lens, &key_count);
    prepare_ffi_args(args, &arg_ptrs, &arg_lens, &arg_count);

    // Call invoke_script (like Go's executeScriptWithRoute)
    struct CommandResult* result = invoke_script(valkey_glide->glide_client,
                                                 0,  // request_id
                                                 script_hash,
                                                 key_count,
                                                 key_ptrs,
                                                 key_lens,
                                                 arg_count,
                                                 arg_ptrs,
                                                 arg_lens,
                                                 NULL,  // route_bytes
                                                 0      // route_bytes_len
    );

    // Cleanup FFI arguments
    if (key_ptrs)
        efree(key_ptrs);
    if (key_lens)
        efree(key_lens);
    if (arg_ptrs)
        efree(arg_ptrs);
    if (arg_lens)
        efree(arg_lens);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    if (!result->response) {
        free_command_result(result);
        RETURN_FALSE;
    }

    command_response_to_zval(result->response, return_value, 0, use_false_if_null);
    free_command_result(result);
}

// Helper to store script and get hash (like Go's storeScript)
char* store_script_and_get_hash(const char* script) {
    struct ScriptHashBuffer* hash_buffer = store_script((const uint8_t*) script, strlen(script));
    if (!hash_buffer || !hash_buffer->ptr) {
        if (hash_buffer) {
            free_script_hash_buffer(hash_buffer);
        }
        return NULL;
    }

    char* hash = emalloc(hash_buffer->len + 1);
    memcpy(hash, hash_buffer->ptr, hash_buffer->len);
    hash[hash_buffer->len] = '\0';
    free_script_hash_buffer(hash_buffer);
    return hash;
}

// Helper function for script flush command
void execute_script_flush_command(zval* object, zval* return_value, bool is_cluster) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    CommandResult* result = execute_command(valkey_glide->glide_client, ScriptFlush, 0, NULL, NULL);

    if (!result) {
        RETURN_FALSE;
    }

    if (result->command_error) {
        free_command_result(result);
        RETURN_FALSE;
    }

    free_command_result(result);

    if (is_cluster) {
        // Cluster scriptFlush returns "OK" string for PHPRedis compatibility
        RETURN_STRING("OK");
    } else {
        // PHPRedis scriptFlush returns boolean true
        RETURN_TRUE;
    }
}
