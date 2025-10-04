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
    CORE_ARG_TYPE_ARRAY,
    CORE_ARG_TYPE_KEY_VALUE_PAIRS
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
    /* Expiry options */
    long expire_seconds;
    long expire_milliseconds;
    long expire_at_seconds;      /* EXAT - expire at unix timestamp in seconds */
    long expire_at_milliseconds; /* PXAT - expire at unix timestamp in milliseconds */
    int  has_expire;
    int  has_pexpire;
    int  has_exat;
    int  has_pxat;

    /* Conditional options */
    int nx; /* Only if not exists */
    int xx; /* Only if exists */
    int ch; /* Changed flag */

    /* Range/limit options */
    long start;
    long end;
    long offset;
    long count;
    int  has_range;
    int  has_limit;

    /* Special flags */
    int get_old_value; /* GET flag for SET commands */
    int keep_ttl;      /* KEEPTTL flag for SET commands */
    int bybit;         /* BYBIT flag for bit commands */
    int approximate;   /* ~ flag for approximate operations */
    int persist;       /* PERSIST flag for GETEX commands */

    /* SET command specific options */
    char*  ifeq_value; /* IFEQ comparison value */
    size_t ifeq_len;   /* IFEQ value length */
    int    has_ifeq;   /* IFEQ flag */
} core_options_t;

/* Argument allocation type */
typedef enum {
    CORE_ARGS_FIXED,   /* Fixed array (args[8]) - no cleanup needed */
    CORE_ARGS_DYNAMIC  /* Dynamic array - needs efree() cleanup */
} core_args_type_t;

/* Core command arguments structure */
typedef struct {
    const void*      glide_client;
    enum RequestType cmd_type;

    /* Primary key */
    const char* key;
    size_t      key_len;

    /* Flexible arguments - either fixed or dynamic */
    union {
        core_arg_t  fixed_args[8];  /* Fixed array for <= 8 args */
        core_arg_t* dynamic_args;   /* Dynamic array for > 8 args */
    };
    int             arg_count;
    core_args_type_t args_type;     /* FIXED or DYNAMIC */

    /* Routing support for cluster commands */
    zval*     route_param; /* Route parameter for cluster commands */
    zend_bool is_cluster;  /* Flag to indicate cluster mode */
    zend_bool has_route;   /* Flag to indicate route is provided */

    /* Options */
    core_options_t options;
    zval*          raw_options; /* Raw PHP options array for complex parsing */
} core_command_args_t;

/* Helper macros for accessing args uniformly */
#define CORE_ARGS(args_struct) \
    ((args_struct)->args_type == CORE_ARGS_FIXED ? (args_struct)->fixed_args : (args_struct)->dynamic_args)

#define CORE_ARG(args_struct, index) \
    (CORE_ARGS(args_struct)[index])

/* Initialize core_command_args_t for fixed args (default) */
#define INIT_CORE_ARGS(args_struct) \
    do { \
        memset((args_struct), 0, sizeof(*(args_struct))); \
        (args_struct)->args_type = CORE_ARGS_FIXED; \
    } while(0)

/* Convert to dynamic args when needed */
int convert_to_dynamic_args(core_command_args_t* args, int new_capacity);

/* Cleanup function that handles both fixed and dynamic */
void cleanup_core_args(core_command_args_t* args);

/* Helper functions for adding arguments */
int add_string_arg(core_command_args_t* args, const char* value, size_t len);
int add_long_arg(core_command_args_t* args, long value);
int add_array_arg(core_command_args_t* args, zval* array, int count);

/*
 * USAGE EXAMPLE:
 * 
 * core_command_args_t args;
 * INIT_CORE_ARGS(&args);
 * args.glide_client = client;
 * args.cmd_type = HSet;
 * args.key = "mykey";
 * args.key_len = 5;
 * 
 * // Add arguments easily (auto-converts to dynamic if needed)
 * add_string_arg(&args, "FIELDS", 6);        // String literals (safe)
 * add_string_arg(&args, condition, strlen(condition)); // PHP parameters (safe)
 * add_long_arg(&args, 3600);                 // Converted to string by core framework
 * add_array_arg(&args, fields_array, field_count);
 * 
 * // Execute command
 * execute_core_command(valkey_glide, &args, NULL, processor, return_value);
 * 
 * // Cleanup (handles both fixed and dynamic)
 * cleanup_core_args(&args);
 * 
 * STRING SAFETY:
 * - String literals and PHP parameters are safe (managed lifetimes)
 * - Core framework handles numeric→string conversion with proper tracking
 * - No manual string copying needed for current use cases
 */

/*
 * HFE COMMAND EXAMPLE (much simpler than h_command_args_t):
 * 
 * // HSetEx with unlimited field-value pairs
 * core_command_args_t args;
 * INIT_CORE_ARGS(&args);
 * args.glide_client = client;
 * args.cmd_type = HSetEx;
 * args.key = key;
 * args.key_len = key_len;
 * 
 * add_string_arg(&args, "FNX", 3);        // condition
 * add_string_arg(&args, "EX", 2);         // expiry type  
 * add_long_arg(&args, 3600);              // expiry value
 * add_string_arg(&args, "FIELDS", 6);     // keyword
 * add_long_arg(&args, field_count);       // field count
 * 
 * // Add unlimited field-value pairs (auto-converts to dynamic)
 * for (int i = 0; i < field_count; i++) {
 *     add_string_arg(&args, fields[i], field_lens[i]);
 *     add_string_arg(&args, values[i], value_lens[i]);
 * }
 * 
 * execute_core_command(valkey_glide, &args, NULL, processor, return_value);
 * cleanup_core_args(&args);
 */

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

#endif /* VALKEY_GLIDE_CORE_COMMON_H */
