/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_MONITOR_COMMON_H
#define VALKEY_GLIDE_MONITOR_COMMON_H

#include "common.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_pubsub_common.h"

// Monitor message queue node
typedef struct monitor_message {
    char*                   line;
    int64_t                 line_len;
    struct monitor_message* next;
} monitor_message;

// Monitor callback info structure
typedef struct {
    zval             callback;
    zval             client_obj;
    bool             is_active;
    monitor_message* queue_head;
    monitor_message* queue_tail;
    size_t           queue_depth;
    size_t  dropped_count;  // Messages dropped due to a full queue (protected by queue_mutex)
    mutex_t queue_mutex;
    cond_t  queue_cond;
    bool    in_monitor_mode;
} monitor_callback_info;

// Maximum number of messages allowed in the queue before dropping new ones.
// Prevents unbounded memory growth if the PHP callback is slower than traffic.
#define MONITOR_QUEUE_MAX_DEPTH 10000

// FFI function declarations (from include/glide_bindings.h)
// MonitorCallback is already typedef'd in glide_bindings.h:
//   typedef void (*MonitorCallback)(uintptr_t client_ptr, double timestamp, int64_t db,
//                                   const uint8_t *client_addr, int64_t client_addr_len,
//                                   const uint8_t *command, int64_t command_len,
//                                   const uint8_t *args_json, int64_t args_json_len);
extern const struct ConnectionResponse* create_monitor_client(
    const uint8_t*  connection_request_bytes,
    uintptr_t       connection_request_len,
    MonitorCallback monitor_callback);

extern void close_monitor_client(const void* client_ptr);

// Monitor management functions
void  init_monitor_callbacks(void);
void  cleanup_monitor_callback_info(zval* zv);
void  php_register_monitor_callback(uintptr_t client_ptr, zval* callback, zval* client_obj);
void  php_unregister_monitor_callback(uintptr_t client_ptr);
zval* find_monitor_callback(const char* client_key);

// The C callback that matches MonitorCallback signature and is passed to create_monitor_client
void valkey_glide_monitor_callback(uintptr_t      client_ptr,
                                   double         timestamp,
                                   int64_t        db,
                                   const uint8_t* client_addr,
                                   int64_t        client_addr_len,
                                   const uint8_t* command,
                                   int64_t        command_len,
                                   const uint8_t* args_json,
                                   int64_t        args_json_len);

// Shutdown monitor subsystem
void valkey_glide_monitor_shutdown(void);

// Common monitor method implementation
void valkey_glide_monitor_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);

#endif  // VALKEY_GLIDE_MONITOR_COMMON_H
