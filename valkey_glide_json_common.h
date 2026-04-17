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
int execute_json_del_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_forget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_clear_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_mget_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_type_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_numincrby_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_json_nummultby_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_json_toggle_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_strappend_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_json_strlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_objlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_objkeys_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_resp_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_debug_memory_command(zval*             object,
                                      int               argc,
                                      zval*             return_value,
                                      zend_class_entry* ce);
int execute_json_debug_fields_command(zval*             object,
                                      int               argc,
                                      zval*             return_value,
                                      zend_class_entry* ce);
int execute_json_arrappend_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_json_arrinsert_command(zval*             object,
                                   int               argc,
                                   zval*             return_value,
                                   zend_class_entry* ce);
int execute_json_arrindex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_arrpop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_arrtrim_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);
int execute_json_arrlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce);

/* ====================================================================
 * JSON METHOD MACROS
 * ==================================================================== */

#define JSON_METHOD_CE(class_name)                                                  \
    (strcmp(#class_name, "ValkeyGlideCluster") == 0 ? get_valkey_glide_cluster_ce() \
                                                    : get_valkey_glide_ce())

#define JSON_METHOD_IMPL(class_name, method_name, execute_fn)                                   \
    PHP_METHOD(class_name, method_name) {                                                       \
        if (execute_fn(getThis(), ZEND_NUM_ARGS(), return_value, JSON_METHOD_CE(class_name))) { \
            return;                                                                             \
        }                                                                                       \
        zval_dtor(return_value);                                                                \
        RETURN_FALSE;                                                                           \
    }

#define JSON_SET_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonSet, execute_json_set_command)
#define JSON_GET_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonGet, execute_json_get_command)
#define JSON_DEL_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonDel, execute_json_del_command)
#define JSON_FORGET_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonForget, execute_json_forget_command)
#define JSON_CLEAR_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonClear, execute_json_clear_command)
#define JSON_MGET_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonMGet, execute_json_mget_command)
#define JSON_TYPE_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonType, execute_json_type_command)
#define JSON_NUMINCRBY_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonNumIncrBy, execute_json_numincrby_command)
#define JSON_NUMMULTBY_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonNumMultBy, execute_json_nummultby_command)
#define JSON_TOGGLE_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonToggle, execute_json_toggle_command)
#define JSON_STRAPPEND_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonStrAppend, execute_json_strappend_command)
#define JSON_STRLEN_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonStrLen, execute_json_strlen_command)
#define JSON_OBJLEN_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonObjLen, execute_json_objlen_command)
#define JSON_OBJKEYS_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonObjKeys, execute_json_objkeys_command)
#define JSON_RESP_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonResp, execute_json_resp_command)
#define JSON_DEBUG_MEMORY_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonDebugMemory, execute_json_debug_memory_command)
#define JSON_DEBUG_FIELDS_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonDebugFields, execute_json_debug_fields_command)
#define JSON_ARRAPPEND_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonArrAppend, execute_json_arrappend_command)
#define JSON_ARRINSERT_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonArrInsert, execute_json_arrinsert_command)
#define JSON_ARRINDEX_METHOD_IMPL(cn) \
    JSON_METHOD_IMPL(cn, jsonArrIndex, execute_json_arrindex_command)
#define JSON_ARRPOP_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonArrPop, execute_json_arrpop_command)
#define JSON_ARRTRIM_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonArrTrim, execute_json_arrtrim_command)
#define JSON_ARRLEN_METHOD_IMPL(cn) JSON_METHOD_IMPL(cn, jsonArrLen, execute_json_arrlen_command)

#endif /* VALKEY_GLIDE_JSON_COMMON_H */
