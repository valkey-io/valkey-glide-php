/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_SCRIPT_COMMON_H
#define VALKEY_GLIDE_SCRIPT_COMMON_H

// Macro definitions for eval methods
#define EVAL_METHOD_IMPL(class_name) \
    PHP_METHOD(class_name, eval) { \
        char* script; \
        size_t script_len; \
        zval* keys = NULL; \
        zval* args = NULL; \
        \
        ZEND_PARSE_PARAMETERS_START(1, 3) \
            Z_PARAM_STRING(script, script_len) \
            Z_PARAM_OPTIONAL \
            Z_PARAM_ARRAY(keys) \
            Z_PARAM_ARRAY(args) \
        ZEND_PARSE_PARAMETERS_END(); \
        \
        valkey_glide_object* valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        \
        int keys_count = 0, args_count = 0; \
        char** keys_array = keys ? process_array_to_args(keys, &keys_count) : NULL; \
        char** args_array = args ? process_array_to_args(args, &args_count) : NULL; \
        \
        int count = keys_count + args_count; \
        uintptr_t* cmd_args = emalloc(sizeof(uintptr_t) * (count + 2)); \
        unsigned long* cmd_args_len = emalloc(sizeof(unsigned long) * (count + 2)); \
        \
        cmd_args[0] = (uintptr_t)script; \
        cmd_args_len[0] = script_len; \
        \
        char keys_count_str[32]; \
        snprintf(keys_count_str, sizeof(keys_count_str), "%d", keys_count); \
        cmd_args[1] = (uintptr_t)keys_count_str; \
        cmd_args_len[1] = strlen(keys_count_str); \
        \
        for (int i = 0; i < keys_count; i++) { \
            cmd_args[i + 2] = (uintptr_t)keys_array[i]; \
            cmd_args_len[i + 2] = strlen(keys_array[i]); \
        } \
        \
        for (int i = 0; i < args_count; i++) { \
            cmd_args[i + keys_count + 2] = (uintptr_t)args_array[i]; \
            cmd_args_len[i + keys_count + 2] = strlen(args_array[i]); \
        } \
        \
        CommandResult* result = execute_command(valkey_glide->glide_client, Eval, count + 2, cmd_args, cmd_args_len); \
        \
        efree(cmd_args); \
        efree(cmd_args_len); \
        if (keys_array) { \
            for (int i = 0; i < keys_count; i++) efree(keys_array[i]); \
            efree(keys_array); \
        } \
        if (args_array) { \
            for (int i = 0; i < args_count; i++) efree(args_array[i]); \
            efree(args_array); \
        } \
        \
        command_response_to_zval(result->response, return_value, 0, false); \
        free_command_result(result); \
    }

#define EVALSHA_METHOD_IMPL(class_name) \
    PHP_METHOD(class_name, evalsha) { \
        char* sha1; \
        size_t sha1_len; \
        zval* keys = NULL; \
        zval* args = NULL; \
        \
        ZEND_PARSE_PARAMETERS_START(1, 3) \
            Z_PARAM_STRING(sha1, sha1_len) \
            Z_PARAM_OPTIONAL \
            Z_PARAM_ARRAY(keys) \
            Z_PARAM_ARRAY(args) \
        ZEND_PARSE_PARAMETERS_END(); \
        \
        valkey_glide_object* valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis()); \
        \
        int keys_count = 0, args_count = 0; \
        char** keys_array = keys ? process_array_to_args(keys, &keys_count) : NULL; \
        char** args_array = args ? process_array_to_args(args, &args_count) : NULL; \
        \
        int count = keys_count + args_count; \
        uintptr_t* cmd_args = emalloc(sizeof(uintptr_t) * (count + 2)); \
        unsigned long* cmd_args_len = emalloc(sizeof(unsigned long) * (count + 2)); \
        \
        cmd_args[0] = (uintptr_t)sha1; \
        cmd_args_len[0] = sha1_len; \
        \
        char keys_count_str[32]; \
        snprintf(keys_count_str, sizeof(keys_count_str), "%d", keys_count); \
        cmd_args[1] = (uintptr_t)keys_count_str; \
        cmd_args_len[1] = strlen(keys_count_str); \
        \
        for (int i = 0; i < keys_count; i++) { \
            cmd_args[i + 2] = (uintptr_t)keys_array[i]; \
            cmd_args_len[i + 2] = strlen(keys_array[i]); \
        } \
        \
        for (int i = 0; i < args_count; i++) { \
            cmd_args[i + keys_count + 2] = (uintptr_t)args_array[i]; \
            cmd_args_len[i + keys_count + 2] = strlen(args_array[i]); \
        } \
        \
        CommandResult* result = execute_command(valkey_glide->glide_client, EvalSha, count + 2, cmd_args, cmd_args_len); \
        \
        efree(cmd_args); \
        efree(cmd_args_len); \
        if (keys_array) { \
            for (int i = 0; i < keys_count; i++) efree(keys_array[i]); \
            efree(keys_array); \
        } \
        if (args_array) { \
            for (int i = 0; i < args_count; i++) efree(args_array[i]); \
            efree(args_array); \
        } \
        \
        command_response_to_zval(result->response, return_value, 0, false); \
        free_command_result(result); \
    }

// Function declarations for helper functions
char** process_array_to_args(zval* array, int* count);

#endif /* VALKEY_GLIDE_SCRIPT_COMMON_H */
