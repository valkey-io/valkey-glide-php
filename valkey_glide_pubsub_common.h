/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_PUBSUB_COMMON_H
#define VALKEY_GLIDE_PUBSUB_COMMON_H

#include "common.h"
#include "valkey_glide_commands_common.h"

// Platform-agnostic mutex and condition variable
#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION   mutex_t;
typedef CONDITION_VARIABLE cond_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t  cond_t;
#endif

// Request type constants
#define REQUEST_TYPE_SUBSCRIBE SubscribeBlocking
#define REQUEST_TYPE_PSUBSCRIBE PSubscribeBlocking
#define REQUEST_TYPE_UNSUBSCRIBE UnsubscribeBlocking
#define REQUEST_TYPE_PUNSUBSCRIBE PUnsubscribeBlocking
#define REQUEST_TYPE_PUBLISH Publish

// Message queue node
typedef struct pubsub_message {
    uint8_t*               channel;
    int64_t                channel_len;
    uint8_t*               message;
    int64_t                message_len;
    uint8_t*               pattern;
    int64_t                pattern_len;
    int                    kind;
    struct pubsub_message* next;
} pubsub_message;

// Pubsub callback info structure
typedef struct {
    zval            callback;
    zval            client_obj;
    bool            is_active;
    pubsub_message* queue_head;
    pubsub_message* queue_tail;
    mutex_t         queue_mutex;
    cond_t          queue_cond;
    HashTable*      subscribed_channels;  // HashTable of subscribed channel/pattern names
    bool            in_subscribe_mode;
} pubsub_callback_info;

// FFI function declarations
extern struct CommandResult* command(const void*          client_adapter_ptr,
                                     uintptr_t            request_id,
                                     enum RequestType     command_type,
                                     unsigned long        arg_count,
                                     const uintptr_t*     args,
                                     const unsigned long* args_len,
                                     const uint8_t*       route_bytes,
                                     uintptr_t            route_bytes_len,
                                     uint64_t             span_ptr);

extern void free_command_result(struct CommandResult* command_result_ptr);

extern const char* register_pubsub_callback(const void*    client_adapter_ptr,
                                            PubSubCallback pubsub_callback);
extern const char* unregister_pubsub_callback(const void* client_adapter_ptr);

// Mutex wrapper functions
void mutex_init(mutex_t* m);
void mutex_lock(mutex_t* m);
void mutex_unlock(mutex_t* m);
void mutex_destroy(mutex_t* m);

// Condition variable wrapper functions
void cond_init(cond_t* c);
void cond_wait(cond_t* c, mutex_t* m);
void cond_signal(cond_t* c);
void cond_destroy(cond_t* c);

// Pubsub management functions
void  init_pubsub_callbacks(void);
void  cleanup_callback_info(zval* zv);
void  cleanup_callback_info_ptr(void* ptr);
void  php_register_pubsub_callback(uintptr_t client_ptr, zval* callback, zval* client_obj);
void  php_unregister_pubsub_callback(uintptr_t client_ptr);
zval* find_pubsub_callback(const char* client_key);
void  remove_pubsub_callback(const char* client_key);
bool  is_client_in_subscribe_mode(uintptr_t client_ptr);
void  pubsub_callback_handler(uintptr_t      client_ptr,
                              int            kind,
                              const uint8_t* message,
                              int64_t        message_len,
                              const uint8_t* channel,
                              int64_t        channel_len,
                              const uint8_t* pattern,
                              int64_t        pattern_len);
void  valkey_glide_pubsub_callback(uintptr_t      client_adapter_ptr,
                                   enum PushKind  kind,
                                   const uint8_t* message,
                                   int64_t        message_len,
                                   const uint8_t* channel,
                                   int64_t        channel_len,
                                   const uint8_t* pattern,
                                   int64_t        pattern_len);
void  valkey_glide_pubsub_shutdown(void);

// Common pubsub method implementations
void valkey_glide_subscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_psubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_unsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_punsubscribe_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);
void valkey_glide_publish_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);


#endif  // VALKEY_GLIDE_PUBSUB_COMMON_H
