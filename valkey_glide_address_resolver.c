/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */

#include "valkey_glide_address_resolver.h"

#include <ffi.h>
#include <string.h>
#include <zend_exceptions.h>

typedef struct resolver_closure {
    zval                     callable;
    ffi_cif                  cif;
    ffi_closure*             closure;
    void*                    fn_ptr;
    struct resolver_closure* next;
} resolver_closure_t;

static resolver_closure_t* g_closures = NULL;

static ffi_type* arg_types[6] = {
    &ffi_type_pointer, /* const uint8_t *host */
    &ffi_type_pointer, /* uintptr_t host_len */
    &ffi_type_uint16,  /* uint16_t port */
    &ffi_type_pointer, /* uint8_t *resolved_host_buf */
    &ffi_type_pointer, /* uintptr_t resolved_host_buf_len */
    &ffi_type_pointer, /* uintptr_t *resolved_host_len */
};

static void generic_resolver_callback(ffi_cif* cif, void* ret, void** args, void* userdata) {
    /*
     * THREAD SAFETY: The Rust FFI layer invokes this callback synchronously
     * during create_client() and during reconnection attempts. PHP's Zend Engine
     * is not thread-safe; all Zend API calls here (call_user_function, EG, etc.)
     * must only execute on the PHP request thread. This is guaranteed by the
     * Glide sync client (SyncClient type) which blocks the calling thread for
     * all operations including reconnects.
     */
    resolver_closure_t* ctx = (resolver_closure_t*) userdata;

    const uint8_t* host     = *(const uint8_t**) args[0];
    uintptr_t      host_len = *(uintptr_t*) args[1];
    uint16_t       port     = *(uint16_t*) args[2];
    uint8_t*       out_buf  = *(uint8_t**) args[3];
    uintptr_t      buf_len  = *(uintptr_t*) args[4];
    uintptr_t*     written  = *(uintptr_t**) args[5];

    *(uint16_t*) ret = 0;

    if (buf_len == 0 || !written)
        return;

    zval args_z[2], retval;
    ZVAL_STRINGL(&args_z[0], (const char*) host, host_len);
    ZVAL_LONG(&args_z[1], port);

    int call_result = call_user_function(NULL, NULL, &ctx->callable, &retval, 2, args_z);
    zval_ptr_dtor(&args_z[0]);
    zval_ptr_dtor(&args_z[1]);

    if (EG(exception))
        zend_clear_exception();
    if (call_result != SUCCESS || Z_TYPE(retval) != IS_ARRAY) {
        zval_ptr_dtor(&retval);
        return;
    }

    zval* host_zv = zend_hash_str_find(Z_ARRVAL(retval), "host", 4);
    if (!host_zv)
        host_zv = zend_hash_index_find(Z_ARRVAL(retval), 0);
    zval* port_zv = zend_hash_str_find(Z_ARRVAL(retval), "port", 4);
    if (!port_zv)
        port_zv = zend_hash_index_find(Z_ARRVAL(retval), 1);

    if (!host_zv || !port_zv || Z_TYPE_P(host_zv) != IS_STRING) {
        zval_ptr_dtor(&retval);
        return;
    }

    size_t rlen = Z_STRLEN_P(host_zv);
    if (rlen == 0 || rlen >= buf_len) {
        zval_ptr_dtor(&retval);
        return;
    }

    memcpy(out_buf, Z_STRVAL_P(host_zv), rlen);
    *written = rlen;

    zend_long rport = zval_get_long(port_zv);
    zval_ptr_dtor(&retval);
    if (rport <= 0 || rport > 65535)
        return;

    *(uint16_t*) ret = (uint16_t) rport;
}

AddressResolverCallback valkey_glide_resolver_acquire(zval* callable) {
    resolver_closure_t* ctx = pemalloc(sizeof(resolver_closure_t), 1);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(*ctx));

    ZVAL_COPY(&ctx->callable, callable);

    ctx->closure = ffi_closure_alloc(sizeof(ffi_closure), &ctx->fn_ptr);
    if (!ctx->closure)
        goto fail;

    if (ffi_prep_cif(&ctx->cif, FFI_DEFAULT_ABI, 6, &ffi_type_uint16, arg_types) != FFI_OK)
        goto fail;

    if (ffi_prep_closure_loc(
            ctx->closure, &ctx->cif, generic_resolver_callback, ctx, ctx->fn_ptr) != FFI_OK)
        goto fail;

    ctx->next  = g_closures;
    g_closures = ctx;

    return (AddressResolverCallback) ctx->fn_ptr;

fail:
    if (ctx->closure)
        ffi_closure_free(ctx->closure);
    zval_ptr_dtor(&ctx->callable);
    pefree(ctx, 1);
    return NULL;
}

void valkey_glide_resolver_release(AddressResolverCallback cb) {
    if (!cb)
        return;

    resolver_closure_t** pp = &g_closures;
    while (*pp) {
        resolver_closure_t* cur = *pp;
        if ((AddressResolverCallback) cur->fn_ptr == cb) {
            *pp = cur->next;
            ffi_closure_free(cur->closure);
            zval_ptr_dtor(&cur->callable);
            pefree(cur, 1);
            return;
        }
        pp = &cur->next;
    }
}

void valkey_glide_resolver_shutdown(void) {
    resolver_closure_t* cur = g_closures;
    while (cur) {
        resolver_closure_t* next = cur->next;
        ffi_closure_free(cur->closure);
        zval_ptr_dtor(&cur->callable);
        pefree(cur, 1);
        cur = next;
    }
    g_closures = NULL;
}
