/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */
#ifndef VALKEY_GLIDE_ADDRESS_RESOLVER_H
#define VALKEY_GLIDE_ADDRESS_RESOLVER_H

#include "include/glide_bindings.h"
#include "php.h"

/*
 * Allocate a libffi closure that, when called by Rust, dispatches to the
 * PHP callable. Returns NULL on allocation failure.
 * The caller must store the returned cb and pass it to close() when done.
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
 * references dropped). Safe to call with NULL.
 */
void valkey_glide_resolver_release(AddressResolverCallback cb);

/*
 * Release all remaining closures (called from MSHUTDOWN/RSHUTDOWN).
 */
void valkey_glide_resolver_shutdown(void);

#endif /* VALKEY_GLIDE_ADDRESS_RESOLVER_H */
