/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_MONITOR_COMMON_H
#define VALKEY_GLIDE_MONITOR_COMMON_H

#include "common.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_pubsub_common.h"

// Monitor message queue node.
//
// Stores the RAW decoded fields delivered by the background Rust thread rather
// than a pre-formatted string. The PHP-side consumer builds either a
// ValkeyGlideMonitorLine object (pull/callback) or a formatted string
// (__toString) from these fields. Keeping raw fields avoids fragile C-side
// string reconstruction and JSON re-escaping.
typedef struct monitor_message {
    double                  timestamp;   // Server timestamp (seconds.microseconds)
    int64_t                 db;          // Database index
    char*                   client_addr; // "host:port" (owned, may be NULL)
    char*                   command;     // Command name (owned, may be NULL)
    char**                  args;        // Array of argument strings (owned)
    size_t                  args_count;  // Number of entries in args
    struct monitor_message* next;
} monitor_message;

// Monitor callback info structure.
//
// The callback zval is optional: it is only populated transiently by
// ValkeyGlideMonitor::listen(). Pull-mode consumers (getMonitorMessage/
// tryGetMonitorMessage) read the queue directly and never touch callback.
typedef struct {
    zval             callback;   // Set only during listen(); IS_UNDEF otherwise
    zval             client_obj; // The owning ValkeyGlideMonitor zval
    bool             is_active;
    monitor_message* queue_head;
    monitor_message* queue_tail;
    size_t           queue_depth;
    size_t  dropped_count;  // Messages dropped due to a full queue (protected by queue_mutex)
    mutex_t queue_mutex;
    cond_t  queue_cond;
    bool    in_monitor_mode;
} monitor_callback_info;

// Free a single queue node and all its owned fields.
void valkey_glide_monitor_free_message(monitor_message* msg);

// Dequeue a single message, blocking up to timeout_ms for one to arrive.
// timeout_ms == 0 means non-blocking (return NULL immediately if empty).
// timeout_ms < 0 means wait indefinitely (until a message or is_active clears).
// Returns a node the caller owns (must free with valkey_glide_monitor_free_message),
// or NULL on timeout / inactive / empty.
monitor_message* valkey_glide_monitor_dequeue(monitor_callback_info* info, long timeout_ms);

// Maximum number of messages allowed in the queue before dropping new ones.
// Prevents unbounded memory growth if the consumer is slower than traffic.
// Drops are not silent: valkey_glide_monitor_callback() records them in
// monitor_callback_info.dropped_count, which is surfaced to the user via
// ValkeyGlideMonitor::getDroppedCount().
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
void                   init_monitor_callbacks(void);
void                   cleanup_monitor_callback_info(monitor_callback_info* info);
monitor_callback_info* php_register_monitor_callback(uintptr_t client_ptr,
                                                     zval*     callback,
                                                     zval*     client_obj);
void php_unregister_monitor_callback(uintptr_t client_ptr, monitor_callback_info* info);

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

#endif  // VALKEY_GLIDE_MONITOR_COMMON_H
