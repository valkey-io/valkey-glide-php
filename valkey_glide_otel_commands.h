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

#ifndef VALKEY_GLIDE_OTEL_COMMANDS_H
#define VALKEY_GLIDE_OTEL_COMMANDS_H

#include "command_response.h"
#include "include/glide_bindings.h"

/* Command execution wrappers with OTEL span support */
CommandResult* execute_command_with_span(const void*          client_adapter_ptr,
                                         enum RequestType     command_type,
                                         unsigned long        arg_count,
                                         const uintptr_t*     args,
                                         const unsigned long* args_len,
                                         uint64_t             span_ptr);

CommandResult* execute_command_with_route_and_span(const void*          client_adapter_ptr,
                                                   enum RequestType     command_type,
                                                   unsigned long        arg_count,
                                                   const uintptr_t*     args,
                                                   const unsigned long* args_len,
                                                   zval*                route,
                                                   uint64_t             span_ptr);

#endif /* VALKEY_GLIDE_OTEL_COMMANDS_H */
