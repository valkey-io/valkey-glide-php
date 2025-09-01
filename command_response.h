/*
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

#ifndef COMMAND_RESPONSE_H
#define COMMAND_RESPONSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "include/glide_bindings.h"
#include "php.h"
#include "zend.h"
#include "zend_API.h"

enum CommandResponseToZvalFlags {
    COMMAND_RESPONSE_NOT_ASSOSIATIVE        = 0,
    COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP  = 1,  // Use associative array format for Map elements
    COMMAND_RESPONSE_ARRAY_ASSOCIATIVE      = 2,  // Use associative array format for stream entries
    COMMAND_RESPONSE_SCAN_ASSOSIATIVE_ARRAY = 3,
    COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP_FUNCTION =
        4  // Use associative array format for FUNCTION command responses
};
/*
 * Execute a command and handle common error checking
 * Returns NULL if there was an error, otherwise returns the CommandResult
 * The caller is responsible for freeing the CommandResult using free_command_result()
 */
CommandResult* execute_command(const void*          glide_client,
                               enum RequestType     command_type,
                               unsigned long        arg_count,
                               const uintptr_t*     args,
                               const unsigned long* args_len);

CommandResult* execute_command_with_route(const void*          glide_client,
                                          enum RequestType     command_type,
                                          unsigned long        arg_count,
                                          const uintptr_t*     args,
                                          const unsigned long* args_len,
                                          zval*                arg_route);

/*
 * Handle an integer response
 * Returns 0 on error, 1 on success
 * The output_value parameter is set to the integer value on success
 * This function frees the CommandResult
 */
long handle_int_response(CommandResult* result, long* output_value);

/*
 * Handle a string response
 * Returns 1 on success, 0 if the key doesn't exist, -1 on error
 * The output and output_len parameters are set to the string value and length
 * The caller is responsible for freeing the output string using efree()
 * This function frees the CommandResult
 */
int handle_string_response(CommandResult* result, char** output, size_t* output_len);

/*
 * Handle a boolean response
 * Returns 1 for true, 0 for false, -1 on error
 * This function frees the CommandResult
 */
int handle_bool_response(CommandResult* result);

/*
 * Handle an OK response
 * Returns 1 on success, -1 on error
 * This function frees the CommandResult
 */
int handle_ok_response(CommandResult* result);

/*
 * Handle an array response
 * Returns 1 on success, 0 if null, -1 on error
 * The output parameter is set to a PHP array
 * This function frees the CommandResult
 */
int handle_array_response(CommandResult* result, zval* output);

/*
 * Handle a map response
 * Returns 1 on success, 0 if null, -1 on error
 * The output parameter is set to a PHP associative array
 * This function frees the CommandResult
 */
int handle_map_response(CommandResult* result, zval* output);

/*
 * Handle a set response
 * Returns 1 on success, 0 if null, -1 on error
 * The output parameter is set to a PHP array (with unique values)
 * This function frees the CommandResult
 */
int handle_set_response(CommandResult* result, zval* output);

/*
 * Helper function to convert a CommandResponse to a PHP value
 * Returns 1 on success, 0 if null, -1 on error
 * The output parameter is set to the PHP value
 * use_associative_array:
 * - 0: regular array processing
 * - 1: convert Map elements to associative array format (for ZMPOP/sorted sets)
 */
int command_response_to_zval(CommandResponse* response,
                             zval*            output,
                             int              use_associative_array,
                             bool             use_false_if_null);

/*
 * Helper function to convert a long value to a string
 * Returns a newly allocated string or NULL on error
 * The caller is responsible for freeing the string using efree()
 */
char* long_to_string(long value, size_t* len);

/*
 * Helper function to convert a double value to a string
 * Returns a newly allocated string or NULL on error
 * The caller is responsible for freeing the string using efree()
 */
char* double_to_string(double value, size_t* len);

/*
 * Helper function to convert a CommandResponse to a PHP stream format
 * This is specifically for XRANGE/XREVRANGE commands that return stream entries
 * Returns 1 on success, 0 if null, -1 on error
 * The output parameter is set to a PHP associative array with stream IDs as keys
 */
int command_response_to_stream_zval(CommandResponse* response, zval* output);

/* Utility functions */
/**
 * Safe zval to string conversion with memory management
 * Returns allocated string that must be freed, or NULL on error
 * Sets need_free to 1 if returned string must be freed
 */

char* zval_to_string_safe(zval* z, size_t* len, int* need_free);
void  free_allocated_strings(char** strings, int count);

#endif /* COMMAND_RESPONSE_H */
