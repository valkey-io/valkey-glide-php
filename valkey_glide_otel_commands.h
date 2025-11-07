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
