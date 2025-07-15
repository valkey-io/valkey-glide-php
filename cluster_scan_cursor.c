/*
  +----------------------------------------------------------------------+
  | ValkeyGlide ClusterScanCursor Implementation                         |
  +----------------------------------------------------------------------+
  | Copyright (c) 2023-2025 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  +----------------------------------------------------------------------+
*/

#include "cluster_scan_cursor.h"

#include <zend_exceptions.h>

#include <ext/standard/info.h>

#include "cluster_scan_cursor_arginfo.h"

/* Global variables */
zend_class_entry*    cluster_scan_cursor_ce;
zend_object_handlers cluster_scan_cursor_object_handlers;

/* Constants */
static const char* FINISHED_SCAN_CURSOR = "finished";

/* Object creation and destruction */
zend_object* create_cluster_scan_cursor_object(zend_class_entry* ce) {
    cluster_scan_cursor_object* cursor_obj =
        ecalloc(1, sizeof(cluster_scan_cursor_object) + zend_object_properties_size(ce));

    zend_object_std_init(&cursor_obj->std, ce);
    object_properties_init(&cursor_obj->std, ce);

    cursor_obj->cursor_id = NULL;

    memcpy(&cluster_scan_cursor_object_handlers,
           zend_get_std_object_handlers(),
           sizeof(cluster_scan_cursor_object_handlers));
    cluster_scan_cursor_object_handlers.offset   = XtOffsetOf(cluster_scan_cursor_object, std);
    cluster_scan_cursor_object_handlers.free_obj = free_cluster_scan_cursor_object;
    cursor_obj->std.handlers                     = &cluster_scan_cursor_object_handlers;

    return &cursor_obj->std;
}

void free_cluster_scan_cursor_object(zend_object* object) {
    cluster_scan_cursor_object* cursor_obj = CLUSTER_SCAN_CURSOR_GET_OBJECT(object);


    /* Call FFI function to clean up Rust-side cursor */
    remove_cluster_scan_cursor(cursor_obj->cursor_id);

    /* Free cursor string */
    if (cursor_obj->cursor_id) {
        efree(cursor_obj->cursor_id);
        cursor_obj->cursor_id = NULL;
    }

    /* Free cursor string */
    if (cursor_obj->next_cursor_id) {
        efree(cursor_obj->next_cursor_id);
        cursor_obj->next_cursor_id = NULL;
    }

    /* Clean up the standard object */
    zend_object_std_dtor(&cursor_obj->std);
}

/* Class methods implementation */

/**
 * Constructor: new ClusterScanCursor($cursorId = "0")
 */
PHP_METHOD(ClusterScanCursor, __construct) {
    char*                       cursor_id     = NULL;
    size_t                      cursor_id_len = 0;
    cluster_scan_cursor_object* cursor_obj;

    /* Parse optional cursor ID parameter - allow NULL */
    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_STRING_OR_NULL(cursor_id, cursor_id_len)
    ZEND_PARSE_PARAMETERS_END();

    cursor_obj = CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(getThis());

    /* Set cursor ID - default to "0" if not provided or NULL */
    if (cursor_id && cursor_id_len > 0) {
        cursor_obj->cursor_id = estrndup(cursor_id, cursor_id_len);
    } else {
        cursor_obj->cursor_id = estrdup("0");
    }
    cursor_obj->next_cursor_id = NULL; /* Initialize next_cursor_id to NULL */
}

/**
 * Destructor
 */
PHP_METHOD(ClusterScanCursor, __destruct) {
    /* Cleanup is handled in free_cluster_scan_cursor_object */
}

/**
 * getCursor(): Returns the cursor ID string
 */
PHP_METHOD(ClusterScanCursor, getCursor) {
    cluster_scan_cursor_object* cursor_obj;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    cursor_obj = CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(getThis());

    if (cursor_obj->cursor_id) {
        RETURN_STRING(cursor_obj->cursor_id);
    }

    RETURN_STRING("0");
}

PHP_METHOD(ClusterScanCursor, getNextCursor) {
    cluster_scan_cursor_object* cursor_obj;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    cursor_obj = CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(getThis());

    if (cursor_obj->next_cursor_id) {
        RETURN_STRING(cursor_obj->next_cursor_id);
    }

    RETURN_STRING("0");
}
/**
 * isFinished(): Checks if cursor equals "finished" or "0"
 */
PHP_METHOD(ClusterScanCursor, isFinished) {
    cluster_scan_cursor_object* cursor_obj;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    cursor_obj = CLUSTER_SCAN_CURSOR_ZVAL_GET_OBJECT(getThis());

    if (cursor_obj->cursor_id && (strcmp(cursor_obj->cursor_id, FINISHED_SCAN_CURSOR) == 0 ||
                                  strcmp(cursor_obj->cursor_id, "0") == 0)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}

/* Class registration function using generated arginfo */
void register_cluster_scan_cursor_class(void) {
    /* Use the generated registration function */
    cluster_scan_cursor_ce                = register_class_ClusterScanCursor();
    cluster_scan_cursor_ce->create_object = create_cluster_scan_cursor_object;
}

/* Getter function for the class entry */
zend_class_entry* get_cluster_scan_cursor_ce(void) {
    return cluster_scan_cursor_ce;
}
