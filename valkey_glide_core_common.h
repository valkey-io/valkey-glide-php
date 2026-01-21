/*
  +----------------------------------------------------------------------+
  | Valkey Glide Core Common Framework                                   |
  +----------------------------------------------------------------------+
  | Copyright (c) 2023-2025 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#ifndef VALKEY_GLIDE_CORE_COMMON_H
#define VALKEY_GLIDE_CORE_COMMON_H

#include "command_response.h"
#include "valkey_glide_commands_common.h"

/* ====================================================================
 * CORE COMMAND ARGUMENT STRUCTURES
 * ==================================================================== */

/* Argument types for flexible command handling */
typedef enum {
    CORE_ARG_TYPE_NONE = 0,
    CORE_ARG_TYPE_STRING,
    CORE_ARG_TYPE_LONG,
    CORE_ARG_TYPE_DOUBLE,
    CORE_ARG_TYPE_ARRAY
} core_arg_type_t;

/* Flexible argument container */
typedef struct {
    core_arg_type_t type;
    union {
        struct {
            const char* value;
            size_t      len;
        } string_arg;

        struct {
            long value;
        } long_arg;

        struct {
            double value;
        } double_arg;

        struct {
            zval* array;
            int   count;
        } array_arg;

        struct {
            const char** values;
            size_t*      lengths;
            int          count;
        } multi_string_arg;

        struct {
            HashTable* pairs;
        } key_value_arg;
    } data;
} core_arg_t;

/* Common command options */
typedef struct {
    char*  ifeq_value; /* IFEQ comparison value */
    size_t ifeq_len;   /* IFEQ value length */
    long   expire_seconds;
    long   expire_milliseconds;
    long   expire_at_seconds;      /* EXAT - expire at unix timestamp in seconds */
    long   expire_at_milliseconds; /* PXAT - expire at unix timestamp in milliseconds */
    long   start;
    long   end;
    long   offset;
    long   count;
    int    has_expire;
    int    has_pexpire;
    int    has_exat;
    int    has_pxat;
    int    nx; /* Only if not exists */
    int    xx; /* Only if exists */
    int    ch; /* Changed flag */
    int    has_range;
    int    has_limit;
    int    get_old_value; /* GET flag for SET commands */
    int    keep_ttl;      /* KEEPTTL flag for SET commands */
    int    bybit;         /* BYBIT flag for bit commands */
    int    approximate;   /* ~ flag for approximate operations */
    int    persist;       /* PERSIST flag for GETEX commands */
    int    has_ifeq;      /* IFEQ flag */
} core_options_t;

/* Argument allocation type */
/* Core command arguments structure - simplified without dynamic support */
typedef struct {
    const void*      glide_client;
    const char*      key;
    zval*            route_param; /* Route parameter for cluster commands */
    zval*            raw_options; /* Raw PHP options array for complex parsing */
    size_t           key_len;
    core_options_t   options;
    core_arg_t       args[8]; /* Fixed arguments array - sufficient for current usage */
    enum RequestType cmd_type;
    int              arg_count;
    zend_bool        is_cluster; /* Flag to indicate cluster mode */
    zend_bool        has_route;  /* Flag to indicate route is provided */
} core_command_args_t;

/* ====================================================================
 * CORE FRAMEWORK FUNCTIONS
 * ==================================================================== */

/* Main command execution framework */
int execute_core_command(valkey_glide_object* valkey_glide,
                         core_command_args_t* args,
                         void*                result_ptr,
                         z_result_processor_t processor,
                         zval*                return_value);

/* Command argument preparation utilities */
int prepare_core_args(core_command_args_t* args,
                      uintptr_t**          cmd_args,
                      unsigned long**      cmd_args_len,
                      char***              allocated_strings,
                      int*                 allocated_count);

void free_core_args(uintptr_t*     cmd_args,
                    unsigned long* cmd_args_len,
                    char**         allocated_strings,
                    int            allocated_count);

/* ====================================================================
 * ARGUMENT PREPARATION HELPERS
 * ==================================================================== */

/* Single key operations */
int prepare_key_only_args(core_command_args_t* args,
                          uintptr_t**          cmd_args,
                          unsigned long**      cmd_args_len);

/* Key-value operations */
int prepare_key_value_args(core_command_args_t* args,
                           uintptr_t**          cmd_args,
                           unsigned long**      cmd_args_len,
                           char***              allocated_strings,
                           int*                 allocated_count);

int prepare_key_value_pairs_args(core_command_args_t* args,
                                 uintptr_t**          cmd_args,
                                 unsigned long**      cmd_args_len,
                                 char***              allocated_strings,
                                 int*                 allocated_count);

/* Message operations (no key, just arguments) */
int prepare_message_args(core_command_args_t* args,
                         uintptr_t**          cmd_args,
                         unsigned long**      cmd_args_len,
                         char***              allocated_strings,
                         int*                 allocated_count);

/* Multi-key operations */
int prepare_multi_key_args(core_command_args_t* args,
                           uintptr_t**          cmd_args,
                           unsigned long**      cmd_args_len);

/* Bit operations */
int prepare_bit_operation_args(core_command_args_t* args,
                               uintptr_t**          cmd_args,
                               unsigned long**      cmd_args_len,
                               char***              allocated_strings,
                               int*                 allocated_count);

/* Expire operations */
int prepare_expire_args(core_command_args_t* args,
                        uintptr_t**          cmd_args,
                        unsigned long**      cmd_args_len,
                        char***              allocated_strings,
                        int*                 allocated_count);

/* Range operations */
int prepare_range_args(core_command_args_t* args,
                       uintptr_t**          cmd_args,
                       unsigned long**      cmd_args_len,
                       char***              allocated_strings,
                       int*                 allocated_count);

int prepare_zero_args(core_command_args_t* args,
                      uintptr_t**          cmd_args,
                      unsigned long**      cmd_args_len);

/* ====================================================================
 * RESULT PROCESSORS
 * ==================================================================== */

/* Integer result processor */
int process_core_int_result(CommandResponse* response, void* output, zval* return_value);

/* String result processor */
int process_core_string_result(CommandResponse* response, void* output, zval* return_value);

/* Boolean result processor */
int process_core_bool_result(CommandResponse* response, void* output, zval* return_value);

/* Array result processor */
int process_core_array_result(CommandResponse* response, void* output, zval* return_value);

/* Double result processor */
int process_core_double_result(CommandResponse* response, void* output, zval* return_value);


/* Core type result processor */
int process_core_type_result(CommandResponse* response, void* output, zval* return_value);

/* INFO command result processor - handles both single and multi-node responses */
int process_info_result(CommandResponse* response, void* output, zval* return_value);

/* ====================================================================
 * MEMORY MANAGEMENT UTILITIES
 * ==================================================================== */

/* Allocate command argument arrays */
int allocate_core_arg_arrays(int count, uintptr_t** args_out, unsigned long** args_len_out);

/* Track allocated strings for cleanup */
char** create_string_tracker(int max_strings);
void   add_tracked_string(char** tracker, int* count, char* str);
void   free_tracked_strings(char** tracker, int count);

/* Convert various types to string arguments */
char* core_long_to_string(long value, size_t* len);
char* core_double_to_string(double value, size_t* len);

/* ====================================================================
 * OPTION PARSING UTILITIES
 * ==================================================================== */

/* Parse common command options from zval */
int parse_core_options(zval* options, core_options_t* opts);

/* Parse SET command specific options */
int parse_set_options(zval* options, core_options_t* opts);


/* ====================================================================
 * SPECIALIZED COMMAND HELPERS
 * ==================================================================== */


/* Multi-key commands (DEL, UNLINK) with all usage patterns and batch support */
int execute_multi_key_command(valkey_glide_object* valkey_glide,
                              enum RequestType     cmd_type,
                              zval*                keys,
                              int                  keys_count,
                              zval*                object,
                              zval*                return_value);

/* ====================================================================
 * ERROR HANDLING AND DEBUGGING
 * ==================================================================== */

/* Debug helpers (only active in debug builds) */
#ifdef DEBUG
void debug_print_core_args(core_command_args_t* args);
void debug_print_command_result(CommandResult* result);
#else
#define debug_print_core_args(args) \
    do {                            \
    } while (0)
#define debug_print_command_result(result) \
    do {                                   \
    } while (0)
#endif

/**
 * Safely allocate and format an integer as a string
 * Uses exact buffer size to prevent overruns
 */
char* safe_format_int(int value, size_t* len_out);

/**
 * Safely allocate and format a long long as a string
 * Uses exact buffer size to prevent overruns
 */
char* safe_format_long_long(long long value, size_t* len_out);

/**
 * Add string to args array and track for cleanup
 * Combines add_tracked_string with argument array population
 */
void add_string_arg(char*           str,
                    size_t          len,
                    uintptr_t**     args_out,
                    unsigned long** args_len_out,
                    int*            arg_idx,
                    char***         allocated_strings,
                    int*            allocated_count);

/**
 * Execute update_connection_password command
 */
void execute_update_connection_password(zval*             object,
                                        const char*       password,
                                        size_t            password_len,
                                        bool              immediate_auth,
                                        zval*             return_value,
                                        zend_class_entry* ce);

/* Macro for updateConnectionPassword method implementation */
#define UPDATE_CONNECTION_PASSWORD_METHOD_IMPL(class_name)                                       \
    PHP_METHOD(class_name, updateConnectionPassword) {                                           \
        char*     password       = NULL;                                                         \
        size_t    password_len   = 0;                                                            \
        zend_bool immediate_auth = 0;                                                            \
        zval*     object         = ZEND_THIS;                                                    \
                                                                                                 \
        if (zend_parse_parameters(                                                               \
                ZEND_NUM_ARGS(), "s|b", &password, &password_len, &immediate_auth) == FAILURE) { \
            zend_throw_exception(get_valkey_glide_exception_ce(), "Invalid parameters", 0);      \
            return;                                                                              \
        }                                                                                        \
                                                                                                 \
        if (password_len == 0) {                                                                 \
            zend_throw_exception(get_valkey_glide_exception_ce(),                                \
                                 "Password cannot be empty. Use clearConnectionPassword() to "   \
                                 "remove password.",                                             \
                                 0);                                                             \
            return;                                                                              \
        }                                                                                        \
                                                                                                 \
        execute_update_connection_password(                                                      \
            object, password, password_len, immediate_auth, return_value, Z_OBJCE_P(object));    \
    }

/* Macro for clearConnectionPassword method implementation */
#define CLEAR_CONNECTION_PASSWORD_METHOD_IMPL(class_name)                                   \
    PHP_METHOD(class_name, clearConnectionPassword) {                                       \
        zend_bool immediate_auth = 0;                                                       \
        zval*     object         = ZEND_THIS;                                               \
                                                                                            \
        if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &immediate_auth) == FAILURE) {     \
            zend_throw_exception(get_valkey_glide_exception_ce(), "Invalid parameters", 0); \
            return;                                                                         \
        }                                                                                   \
                                                                                            \
        execute_update_connection_password(                                                 \
            object, "", 0, immediate_auth, return_value, Z_OBJCE_P(object));                \
    }

#endif /* VALKEY_GLIDE_CORE_COMMON_H */
