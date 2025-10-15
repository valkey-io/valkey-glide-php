/*
  +----------------------------------------------------------------------+
  | Valkey Glide Commands Common Framework                               |
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

#ifndef VALKEY_GLIDE_PUBSUB_H
#define VALKEY_GLIDE_PUBSUB_H

#include "common.h"

// Safe callback wrapper with reference counting
typedef struct {
    zval*        callback;
    _Atomic int  refcount;
    _Atomic bool valid;
} safe_callback_t;

// Execute command function declarations
int execute_subscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_psubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_unsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_punsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_ssubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_sunsubscribe_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_publish_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_spublish_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_pubsub_introspection_command(zval*             object,
                                         int               argc,
                                         zval*             return_value,
                                         zend_class_entry* ce);

// PubSub method implementation macros
#define SUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, subscribe) {                                              \
        if (execute_subscribe_command(getThis(),                                     \
                                      ZEND_NUM_ARGS(),                               \
                                      return_value,                                  \
                                      strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                          ? get_valkey_glide_cluster_ce()            \
                                          : get_valkey_glide_ce())) {                \
            return;                                                                  \
        }                                                                            \
        RETURN_FALSE;                                                                \
    }

#define PSUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, psubscribe) {                                              \
        if (execute_psubscribe_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        RETURN_FALSE;                                                                 \
    }

#define UNSUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, unsubscribe) {                                              \
        if (execute_unsubscribe_command(getThis(),                                     \
                                        ZEND_NUM_ARGS(),                               \
                                        return_value,                                  \
                                        strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                            ? get_valkey_glide_cluster_ce()            \
                                            : get_valkey_glide_ce())) {                \
            return;                                                                    \
        }                                                                              \
        RETURN_FALSE;                                                                  \
    }

#define PUNSUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, punsubscribe) {                                              \
        if (execute_punsubscribe_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce())) {                \
            return;                                                                     \
        }                                                                               \
        RETURN_FALSE;                                                                   \
    }

#define PUBLISH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, publish) {                                              \
        if (execute_publish_command(getThis(),                                     \
                                    ZEND_NUM_ARGS(),                               \
                                    return_value,                                  \
                                    strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                        ? get_valkey_glide_cluster_ce()            \
                                        : get_valkey_glide_ce())) {                \
            return;                                                                \
        }                                                                          \
        RETURN_FALSE;                                                              \
    }

#define SSUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, ssubscribe) {                                              \
        if (execute_ssubscribe_command(getThis(),                                     \
                                       ZEND_NUM_ARGS(),                               \
                                       return_value,                                  \
                                       strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                           ? get_valkey_glide_cluster_ce()            \
                                           : get_valkey_glide_ce())) {                \
            return;                                                                   \
        }                                                                             \
        RETURN_FALSE;                                                                 \
    }

#define SUNSUBSCRIBE_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, sunsubscribe) {                                              \
        if (execute_sunsubscribe_command(getThis(),                                     \
                                         ZEND_NUM_ARGS(),                               \
                                         return_value,                                  \
                                         strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                             ? get_valkey_glide_cluster_ce()            \
                                             : get_valkey_glide_ce())) {                \
            return;                                                                     \
        }                                                                               \
        RETURN_FALSE;                                                                   \
    }

#define SPUBLISH_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, spublish) {                                              \
        if (execute_spublish_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        RETURN_FALSE;                                                               \
    }

#define PUBSUB_METHOD_IMPL(class_name)                                                          \
    PHP_METHOD(class_name, pubSub) {                                                            \
        if (execute_pubsub_introspection_command(getThis(),                                     \
                                                 ZEND_NUM_ARGS(),                               \
                                                 return_value,                                  \
                                                 strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                                     ? get_valkey_glide_cluster_ce()            \
                                                     : get_valkey_glide_ce())) {                \
            return;                                                                             \
        }                                                                                       \
        RETURN_FALSE;                                                                           \
    }

// Function declarations
void standalone_pubsub_callback(uintptr_t      client_ptr,
                                enum PushKind  kind,
                                const uint8_t* message,
                                int64_t        message_len,
                                const uint8_t* channel,
                                int64_t        channel_len,
                                const uint8_t* pattern,
                                int64_t        pattern_len);

void cluster_pubsub_callback(uintptr_t      client_ptr,
                             enum PushKind  kind,
                             const uint8_t* message,
                             int64_t        message_len,
                             const uint8_t* channel,
                             int64_t        channel_len,
                             const uint8_t* pattern,
                             int64_t        pattern_len);

void register_standalone_client_mapping(const void* rust_client_ptr, valkey_glide_object* obj);
void register_cluster_client_mapping(const void* rust_client_ptr, valkey_glide_object* obj);
void unregister_standalone_client_mapping(const void* rust_client_ptr);
void unregister_cluster_client_mapping(const void* rust_client_ptr);
void unregister_client_mapping(const void* rust_client_ptr);  // Backward-compatible
void unregister_client_mapping_typed(const void* rust_client_ptr, bool is_cluster);  // Optimized
void invalidate_callback(valkey_glide_object* obj);

#endif  // VALKEY_GLIDE_PUBSUB_H
