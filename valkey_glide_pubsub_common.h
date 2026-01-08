/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_PUBSUB_COMMON_H
#define VALKEY_GLIDE_PUBSUB_COMMON_H

#include "common.h"
#include "valkey_glide_commands_common.h"

// Request type constants
#define REQUEST_TYPE_SUBSCRIBE Subscribe
#define REQUEST_TYPE_PSUBSCRIBE PSubscribe
#define REQUEST_TYPE_UNSUBSCRIBE Unsubscribe
#define REQUEST_TYPE_PUNSUBSCRIBE PUnsubscribe
#define REQUEST_TYPE_PUBLISH Publish

// Pubsub callback info structure
typedef struct {
    zval callback;
    zval client_obj;
    bool is_active;
} pubsub_callback_info;

// FFI function declarations
extern struct CommandResult* command(
    const void* client_adapter_ptr,
    uintptr_t request_id,
    enum RequestType command_type,
    unsigned long arg_count,
    const uintptr_t* args,
    const unsigned long* args_len,
    const uint8_t* route_bytes,
    uintptr_t route_bytes_len,
    uint64_t span_ptr
);

extern void free_command_result(struct CommandResult* command_result_ptr);

// Pubsub management functions
void init_pubsub_callbacks(void);
void cleanup_callback_info(zval *zv);
void register_pubsub_callback(uintptr_t client_ptr, zval *callback, zval *client_obj);
void unregister_pubsub_callback(uintptr_t client_ptr);
zval* find_pubsub_callback(const char *client_key);
void remove_pubsub_callback(const char *client_key);
void pubsub_callback_handler(
    uintptr_t client_ptr,
    int kind,
    const uint8_t *message,
    int64_t message_len,
    const uint8_t *channel,
    int64_t channel_len,
    const uint8_t *pattern,
    int64_t pattern_len
);
void valkey_glide_pubsub_callback(
    uintptr_t client_adapter_ptr,
    enum PushKind kind,
    const uint8_t *message,
    int64_t message_len,
    const uint8_t *channel,
    int64_t channel_len,
    const uint8_t *pattern,
    int64_t pattern_len
);
void valkey_glide_pubsub_shutdown(void);

// Common pubsub method implementations
void valkey_glide_subscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_psubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_unsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_punsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_publish_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);

#endif // VALKEY_GLIDE_PUBSUB_COMMON_H
