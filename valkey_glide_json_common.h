/*
  +----------------------------------------------------------------------+
  | Valkey Glide JSON Commands                                           |
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

#ifndef VALKEY_GLIDE_JSON_COMMON_H
#define VALKEY_GLIDE_JSON_COMMON_H

#include "valkey_glide_commands_common.h"

/* ====================================================================
 * JSON COMMAND FUNCTIONS
 * ==================================================================== */

int execute_json_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_get_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* ====================================================================
 * JSON METHOD MACROS
 * ==================================================================== */

#define JSON_SET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, jsonSet) {                                               \
        if (execute_json_set_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#define JSON_GET_METHOD_IMPL(class_name)                                            \
    PHP_METHOD(class_name, jsonGet) {                                               \
        if (execute_json_get_command(getThis(),                                     \
                                     ZEND_NUM_ARGS(),                               \
                                     return_value,                                  \
                                     strcmp(#class_name, "ValkeyGlideCluster") == 0 \
                                         ? get_valkey_glide_cluster_ce()            \
                                         : get_valkey_glide_ce())) {                \
            return;                                                                 \
        }                                                                           \
        zval_dtor(return_value);                                                    \
        RETURN_FALSE;                                                               \
    }

#endif /* VALKEY_GLIDE_JSON_COMMON_H */
