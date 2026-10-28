/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */

#include "valkey_glide_address_resolver.h"

#include <ffi.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <zend_exceptions.h>

/* Upper bound on cached resolutions per client; a cluster has far fewer nodes */
#define RESOLVER_CACHE_MAX 1024

/* An address the PHP callable resolved, replayed for calls from other threads */
typedef struct resolver_cache_entry {
    char*    host;
    size_t   host_len;
    uint16_t port;
    char*    resolved_host;
    size_t   resolved_host_len;
    uint16_t resolved_port;
} resolver_cache_entry_t;

typedef struct resolver_closure {
    zval                     callable;
    ffi_cif                  cif;
    ffi_closure*             closure;
    void*                    fn_ptr;
    atomic_int               closed; /* 1 = client closed, callback becomes no-op */
    pthread_t                owner;  /* PHP thread that created the client */
    pthread_mutex_t          cache_lock;
    resolver_cache_entry_t*  cache;
    size_t                   cache_len;
    size_t                   cache_cap;
    struct resolver_closure* next;
} resolver_closure_t;

/* Closures of this thread's clients (per thread under ZTS) */
ZEND_TLS resolver_closure_t* g_closures = NULL;

static ffi_type* arg_types[7] = {
    &ffi_type_pointer, /* usize client_id - using pointer type for ABI compat (same size on 64-bit)
                        */
    &ffi_type_pointer, /* const uint8_t *host */
    &ffi_type_pointer, /* usize host_len - using pointer type for ABI compat */
    &ffi_type_uint16,  /* uint16_t port */
    &ffi_type_pointer, /* uint8_t *resolved_host_buf */
    &ffi_type_pointer, /* usize resolved_host_buf_len - using pointer type for ABI compat */
    &ffi_type_pointer, /* uintptr_t *resolved_host_len */
};

/* Caller holds cache_lock */
static resolver_cache_entry_t* resolver_cache_find(resolver_closure_t* ctx,
                                                   const uint8_t*      host,
                                                   size_t              host_len,
                                                   uint16_t            port) {
    for (size_t i = 0; i < ctx->cache_len; i++) {
        resolver_cache_entry_t* entry = &ctx->cache[i];
        if (entry->port == port && entry->host_len == host_len &&
            memcmp(entry->host, host, host_len) == 0) {
            return entry;
        }
    }
    return NULL;
}

/* Record a resolution made on the owner thread. Uses malloc, not the Zend
 * allocator, because other threads read the cache. */
static void resolver_cache_store(resolver_closure_t* ctx,
                                 const uint8_t*      host,
                                 size_t              host_len,
                                 uint16_t            port,
                                 const char*         resolved_host,
                                 size_t              resolved_host_len,
                                 uint16_t            resolved_port) {
    char* resolved_copy = malloc(resolved_host_len);
    if (!resolved_copy)
        return;
    memcpy(resolved_copy, resolved_host, resolved_host_len);

    pthread_mutex_lock(&ctx->cache_lock);
    resolver_cache_entry_t* entry = resolver_cache_find(ctx, host, host_len, port);
    if (entry) {
        free(entry->resolved_host);
        entry->resolved_host     = resolved_copy;
        entry->resolved_host_len = resolved_host_len;
        entry->resolved_port     = resolved_port;
        pthread_mutex_unlock(&ctx->cache_lock);
        return;
    }

    char* host_copy = NULL;
    if (ctx->cache_len < RESOLVER_CACHE_MAX && (host_copy = malloc(host_len ? host_len : 1))) {
        if (ctx->cache_len == ctx->cache_cap) {
            size_t                  cap   = ctx->cache_cap ? ctx->cache_cap * 2 : 8;
            resolver_cache_entry_t* grown = realloc(ctx->cache, cap * sizeof(*grown));
            if (grown) {
                ctx->cache     = grown;
                ctx->cache_cap = cap;
            }
        }
        if (ctx->cache_len < ctx->cache_cap) {
            memcpy(host_copy, host, host_len);
            ctx->cache[ctx->cache_len++] = (resolver_cache_entry_t){
                .host              = host_copy,
                .host_len          = host_len,
                .port              = port,
                .resolved_host     = resolved_copy,
                .resolved_host_len = resolved_host_len,
                .resolved_port     = resolved_port,
            };
            pthread_mutex_unlock(&ctx->cache_lock);
            return;
        }
    }
    pthread_mutex_unlock(&ctx->cache_lock);
    free(host_copy);
    free(resolved_copy);
}

/* Answer a call from a thread other than the owner from the cache. Returns the
 * resolved port, or 0 (use the original address) when the address is not cached. */
static uint16_t resolver_cache_lookup(resolver_closure_t* ctx,
                                      const uint8_t*      host,
                                      size_t              host_len,
                                      uint16_t            port,
                                      uint8_t*            out_buf,
                                      size_t              buf_len,
                                      uintptr_t*          written) {
    uint16_t resolved_port = 0;
    pthread_mutex_lock(&ctx->cache_lock);
    resolver_cache_entry_t* entry = resolver_cache_find(ctx, host, host_len, port);
    /* Always true today: entries are only stored when they fit the buffer the
     * owner thread was given, and the core always passes 1024 bytes. Kept in
     * case the core ever passes buffers of different sizes. */
    if (entry && entry->resolved_host_len < buf_len) {
        memcpy(out_buf, entry->resolved_host, entry->resolved_host_len);
        *written      = entry->resolved_host_len;
        resolved_port = entry->resolved_port;
    }
    pthread_mutex_unlock(&ctx->cache_lock);
    return resolved_port;
}

/* Free the cache. Only safe once no core thread can call this resolver: a
 * background thread could otherwise read freed entries, and destroying
 * cache_lock while another thread holds it is undefined.
 *
 * Nothing here enforces that. Callers rely on the core having dropped the
 * client (close_client() returned, or create_client() failed), which drops its
 * Tokio runtime, and that Drop blocks until the runtime's worker threads have
 * stopped (https://docs.rs/tokio/latest/tokio/runtime/struct.Runtime.html#shutdown).
 * close_client() does not document that it joins the background threads; it
 * is a side effect. If the core ever shuts the runtime down without waiting
 * (shutdown_background(), shutdown_timeout()), a background thread can still
 * be inside resolver_cache_lookup() reading the memory freed here. */
static void resolver_cache_free(resolver_closure_t* ctx) {
    for (size_t i = 0; i < ctx->cache_len; i++) {
        free(ctx->cache[i].host);
        free(ctx->cache[i].resolved_host);
    }
    free(ctx->cache);
    pthread_mutex_destroy(&ctx->cache_lock);
}

static void generic_resolver_callback(ffi_cif* cif, void* ret, void** args, void* userdata) {
    resolver_closure_t* ctx = (resolver_closure_t*) userdata;

    /* If the client has been closed, return 0 (fallback to original address).
     * This is safe to call from any thread — atomic_load is lock-free.
     * This prevents use-after-free when Rust background threads call the
     * resolver after PHP has closed the client. */
    if (atomic_load(&ctx->closed)) {
        *(uint16_t*) ret = 0;
        return;
    }

    /* client_id is args[0] but we don't need it - PHP uses closure userdata for routing */
    const uint8_t* host     = *(const uint8_t**) args[1];
    uintptr_t      host_len = *(uintptr_t*) args[2];
    uint16_t       port     = *(uint16_t*) args[3];
    uint8_t*       out_buf  = *(uint8_t**) args[4];
    uintptr_t      buf_len  = *(uintptr_t*) args[5];
    uintptr_t*     written  = *(uintptr_t**) args[6];

    *(uint16_t*) ret = 0;

    if (buf_len == 0 || !written)
        return;

    /*
     * THREAD SAFETY: The core calls this during create_client() on the PHP
     * thread, but cluster clients also call it from background threads (the
     * periodic topology check, reconnects). PHP's Zend Engine is not
     * thread-safe, so the PHP callable (call_user_function, EG, the Zend
     * allocator) only runs on the thread that created the client. Other
     * threads get the result the callable last returned for the same address,
     * or the original address if it was never resolved on the PHP thread.
     */
    if (!pthread_equal(pthread_self(), ctx->owner)) {
        *(uint16_t*) ret =
            resolver_cache_lookup(ctx, host, host_len, port, out_buf, buf_len, written);
        return;
    }

    zval args_z[2], retval;
    ZVAL_UNDEF(&retval); /* Initialize retval before use */
    ZVAL_STRINGL(&args_z[0], (const char*) host, host_len);
    ZVAL_LONG(&args_z[1], port);

    int call_result = call_user_function(NULL, NULL, &ctx->callable, &retval, 2, args_z);
    zval_ptr_dtor(&args_z[0]);
    zval_ptr_dtor(&args_z[1]);

    if (EG(exception)) {
        zend_clear_exception();
        zval_ptr_dtor(&retval); /* Safe to call on ZVAL_UNDEF */
        return;
    }
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
    resolver_cache_store(ctx, host, host_len, port, (const char*) out_buf, rlen, (uint16_t) rport);
}

AddressResolverCallback valkey_glide_resolver_acquire(zval* callable) {
    resolver_closure_t* ctx = pemalloc(sizeof(resolver_closure_t), 1);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(*ctx));

    ZVAL_COPY(&ctx->callable, callable);
    atomic_init(&ctx->closed, 0);
    ctx->owner = pthread_self();
    pthread_mutex_init(&ctx->cache_lock, NULL);

    ctx->closure = ffi_closure_alloc(sizeof(ffi_closure), &ctx->fn_ptr);
    if (!ctx->closure)
        goto fail;

    if (ffi_prep_cif(&ctx->cif, FFI_DEFAULT_ABI, 7, &ffi_type_uint16, arg_types) != FFI_OK)
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
    resolver_cache_free(ctx);
    zval_ptr_dtor(&ctx->callable);
    pefree(ctx, 1);
    return NULL;
}

void valkey_glide_resolver_close(AddressResolverCallback cb) {
    if (!cb)
        return;

    resolver_closure_t* cur = g_closures;
    while (cur) {
        if ((AddressResolverCallback) cur->fn_ptr == cb) {
            atomic_store(&cur->closed, 1);
            return;
        }
        cur = cur->next;
    }
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
            /* Relies on the core having stopped its threads; see resolver_cache_free() */
            resolver_cache_free(cur);
            zval_ptr_dtor(&cur->callable);
            pefree(cur, 1);
            return;
        }
        pp = &cur->next;
    }
}

void valkey_glide_resolver_request_shutdown(void) {
    /* A client still open here (a static property, for example) is closed
     * later, by shutdown_executor(), and its core threads may call the
     * resolver until then: keep the closure, but make it fall back to the
     * original address and drop the callable while the request is alive.
     * The client's close frees the rest. */
    /* Releasing a callable can free a client it captured, which releases that
     * client's closure: restart from the head after each one */
    resolver_closure_t* cur = g_closures;
    while (cur) {
        atomic_store(&cur->closed, 1);
        if (Z_ISUNDEF(cur->callable)) {
            cur = cur->next;
            continue;
        }
        zval callable;
        ZVAL_COPY_VALUE(&callable, &cur->callable);
        ZVAL_UNDEF(&cur->callable);
        zval_ptr_dtor(&callable);
        cur = g_closures;
    }
}
