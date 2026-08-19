/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_MONITOR_H
#define VALKEY_GLIDE_MONITOR_H

#include "common.h"
#include "php.h"
#include "valkey_glide_monitor_common.h"

/* ValkeyGlideMonitor object structure */
typedef struct {
    const void*            monitor_client_ptr; /* FFI monitor connection (NULL if closed) */
    monitor_callback_info* info;               /* Queue/callback state (NULL if closed) */
    size_t                 dropped_count;      /* Cached drop count, preserved across close() */
    zend_object            std;                /* MUST be last */
} valkey_glide_monitor_object;

/* Class entries and handlers */
extern zend_class_entry*    valkey_glide_monitor_ce;
extern zend_class_entry*    valkey_glide_monitor_line_ce;
extern zend_object_handlers valkey_glide_monitor_object_handlers;

/* Object lifecycle */
zend_object* create_valkey_glide_monitor_object(zend_class_entry* ce);
void         free_valkey_glide_monitor_object(zend_object* object);

/* Class registration (called from MINIT) */
void register_valkey_glide_monitor_classes(void);

/* Methods */
PHP_METHOD(ValkeyGlideMonitor, __construct);
PHP_METHOD(ValkeyGlideMonitor, getMonitorMessage);
PHP_METHOD(ValkeyGlideMonitor, tryGetMonitorMessage);
PHP_METHOD(ValkeyGlideMonitor, listen);
PHP_METHOD(ValkeyGlideMonitor, getDroppedCount);
PHP_METHOD(ValkeyGlideMonitor, close);
PHP_METHOD(ValkeyGlideMonitorLine, __toString);

/* Helper macros */
#define VALKEY_GLIDE_MONITOR_GET_OBJECT(obj) \
    VALKEY_GLIDE_PHP_GET_OBJECT(valkey_glide_monitor_object, obj)
#define VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(zv) \
    VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_monitor_object, zv)

#endif /* VALKEY_GLIDE_MONITOR_H */
