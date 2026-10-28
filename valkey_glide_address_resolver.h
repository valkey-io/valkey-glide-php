/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */
#ifndef VALKEY_GLIDE_ADDRESS_RESOLVER_H
#define VALKEY_GLIDE_ADDRESS_RESOLVER_H

#include "include/glide_bindings.h"
#include "php.h"

/*
 * Allocate a libffi closure that, when called by Rust, dispatches to the
 * PHP callable. Returns NULL on allocation failure.
 * The caller must store the returned cb and free it with release() once the
 * client is destroyed (valkey_glide_close_client() does this).
 *
 * The PHP callable only runs on the thread that called acquire(). Calls from
 * other threads (the core's background tasks) are answered from the results
 * the callable returned on that thread, or fall back to the original address.
 */
AddressResolverCallback valkey_glide_resolver_acquire(zval* callable);

/*
 * Mark the resolver as closed. After this call, any Rust background thread
 * that invokes the callback will get an immediate 0 return (fallback to
 * original address) without touching PHP internals. Thread-safe.
 */
void valkey_glide_resolver_close(AddressResolverCallback cb);

/*
 * Free the libffi closure allocated by acquire().
 * Must only be called after the Rust client is fully destroyed (all Arc
 * references dropped): close_client() returned, or create_client() failed.
 * Until then the core's background threads may still call it. Safe to call
 * with NULL.
 */
void valkey_glide_resolver_release(AddressResolverCallback cb);

/*
 * Called from RSHUTDOWN: mark every remaining resolver closed and release its
 * PHP callable. The closures themselves stay until their clients are closed,
 * which for clients PHP frees after RSHUTDOWN happens later.
 */
void valkey_glide_resolver_request_shutdown(void);

#endif /* VALKEY_GLIDE_ADDRESS_RESOLVER_H */
