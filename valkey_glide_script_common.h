/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_SCRIPT_COMMON_H
#define VALKEY_GLIDE_SCRIPT_COMMON_H

#include "common.h"
#include "php.h"

// Function declarations for helper functions
char** process_array_to_args(zval* array, int* count);
void   execute_script_flush_command(zval* object, zval* return_value, bool is_cluster);
void   execute_invoke_script_command(valkey_glide_object* valkey_glide,
                                     const char*          hash,
                                     zval*                keys,
                                     zval*                args,
                                     zval*                return_value,
                                     bool                 is_cluster);
char*  store_script_and_get_hash(const char* script);

#endif /* VALKEY_GLIDE_SCRIPT_COMMON_H */
