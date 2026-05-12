/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */

#include "valkey_glide_address_resolver.h"

#include <string.h>
#include <zend_exceptions.h>

static zval g_address_resolver;
static bool g_has_address_resolver = false;

void valkey_glide_set_address_resolver(zval* callable) {
    if (g_has_address_resolver) {
        zval_ptr_dtor(&g_address_resolver);
    }
    ZVAL_COPY(&g_address_resolver, callable);
    g_has_address_resolver = true;
}

void valkey_glide_clear_address_resolver(void) {
    if (g_has_address_resolver) {
        zval_ptr_dtor(&g_address_resolver);
        g_has_address_resolver = false;
    }
}

uint16_t valkey_glide_address_resolver_callback(const uint8_t* host,
                                                uintptr_t      host_len,
                                                uint16_t       port,
                                                uint8_t*       resolved_host_buf,
                                                uintptr_t      resolved_host_buf_len,
                                                uintptr_t*     resolved_host_len) {
    if (!g_has_address_resolver || resolved_host_buf_len == 0 || !resolved_host_len) {
        return 0; /* fallback */
    }

    zval args[2], retval;
    ZVAL_STRINGL(&args[0], (const char*) host, host_len);
    ZVAL_LONG(&args[1], port);

    int call_result = call_user_function(NULL, NULL, &g_address_resolver, &retval, 2, args);

    zval_ptr_dtor(&args[0]);
    zval_ptr_dtor(&args[1]);

    if (EG(exception)) {
        zend_clear_exception();
    }

    if (call_result != SUCCESS || Z_TYPE(retval) != IS_ARRAY) {
        zval_ptr_dtor(&retval);
        return 0; /* fallback */
    }

    zval* resolved_host_zval = zend_hash_str_find(Z_ARRVAL(retval), "host", sizeof("host") - 1);
    if (!resolved_host_zval) {
        resolved_host_zval = zend_hash_index_find(Z_ARRVAL(retval), 0);
    }
    zval* resolved_port_zval = zend_hash_str_find(Z_ARRVAL(retval), "port", sizeof("port") - 1);
    if (!resolved_port_zval) {
        resolved_port_zval = zend_hash_index_find(Z_ARRVAL(retval), 1);
    }

    if (!resolved_host_zval || !resolved_port_zval || Z_TYPE_P(resolved_host_zval) != IS_STRING) {
        zval_ptr_dtor(&retval);
        return 0; /* fallback */
    }

    size_t rhost_len = Z_STRLEN_P(resolved_host_zval);
    if (rhost_len == 0 || rhost_len >= resolved_host_buf_len) {
        zval_ptr_dtor(&retval);
        return 0; /* fallback */
    }

    memcpy(resolved_host_buf, Z_STRVAL_P(resolved_host_zval), rhost_len);
    *resolved_host_len = rhost_len;

    zend_long resolved_port = zval_get_long(resolved_port_zval);
    zval_ptr_dtor(&retval);

    if (resolved_port <= 0 || resolved_port > 65535) {
        return 0; /* fallback */
    }

    return (uint16_t) resolved_port;
}
