#ifndef CLUSTER_SCAN_CURSOR_H
#define CLUSTER_SCAN_CURSOR_H

#include "common.h"
#include "php.h"

/* ClusterScanCursor object structure */
typedef struct {
    char*       cursor_id;      /* The cursor ID string */
    char*       next_cursor_id; /* The cursor ID string */
    zend_object std;            /* Standard PHP object */
} cluster_scan_cursor_object;

/* Class entry and handlers */
extern zend_class_entry*    cluster_scan_cursor_ce;
extern zend_object_handlers cluster_scan_cursor_object_handlers;

/* Object creation and destruction */
zend_object* create_cluster_scan_cursor_object(zend_class_entry* ce);
void         free_cluster_scan_cursor_object(zend_object* object);

/* Class methods */
PHP_METHOD(ClusterScanCursor, __construct);
PHP_METHOD(ClusterScanCursor, __destruct);
PHP_METHOD(ClusterScanCursor, getCursor);
PHP_METHOD(ClusterScanCursor, getNextCursor);
PHP_METHOD(ClusterScanCursor, isFinished);

/* Helper macros */
#define CLUSTER_SCAN_CURSOR_GET_OBJECT(obj) \
    VALKEY_GLIDE_PHP_GET_OBJECT(cluster_scan_cursor_object, obj)
#define CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(zv) \
    VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(cluster_scan_cursor_object, zv)

/* FFI function declaration */
extern void remove_cluster_scan_cursor(const char* cursor_id);

/* Class registration function */
void register_cluster_scan_cursor_class(void);

/* Getter function for class entry */
zend_class_entry* get_cluster_scan_cursor_ce(void);

#endif /* CLUSTER_SCAN_CURSOR_H */
