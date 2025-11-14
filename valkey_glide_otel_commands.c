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

#include "valkey_glide_otel_commands.h"

#include "command_response.h"
#include "logger.h"

/**
 * Execute command with OTEL span support
 */
CommandResult* execute_command_with_span(const void*          client_adapter_ptr,
                                         enum RequestType     command_type,
                                         unsigned long        arg_count,
                                         const uintptr_t*     args,
                                         const unsigned long* args_len,
                                         uint64_t             span_ptr) {
    VALKEY_LOG_DEBUG_FMT(
        "otel_command", "Executing command with span: %lu", (unsigned long) span_ptr);

    /* Use the FFI command function with span support */
    return command((void*) client_adapter_ptr,
                   (uintptr_t) client_adapter_ptr, /* request_id */
                   command_type,
                   arg_count,
                   args,
                   args_len,
                   NULL, /* route_bytes */
                   0,    /* route_bytes_len */
                   span_ptr);
}

/**
 * Execute command with routing and OTEL span support
 */
CommandResult* execute_command_with_route_and_span(const void*          client_adapter_ptr,
                                                   enum RequestType     command_type,
                                                   unsigned long        arg_count,
                                                   const uintptr_t*     args,
                                                   const unsigned long* args_len,
                                                   zval*                route,
                                                   uint64_t             span_ptr) {
    VALKEY_LOG_DEBUG_FMT(
        "otel_command", "Executing routed command with span: %lu", (unsigned long) span_ptr);

    /* For now, use the existing execute_command_with_route function */
    /* TODO: Add proper route serialization and span support */
    return execute_command_with_route(
        (void*) client_adapter_ptr, command_type, arg_count, args, args_len, route);
}
