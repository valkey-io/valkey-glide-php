/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */
#ifndef VALKEY_GLIDE_ADDRESS_RESOLVER_H
#define VALKEY_GLIDE_ADDRESS_RESOLVER_H

#include "include/glide_bindings.h"
#include "php.h"

/*
 * Allocate a libffi closure that, when called by Rust, dispatches to the
 * PHP callable. Returns NULL on allocation failure.
 * The caller must store the returned cb and pass it to release() when done.
 */
AddressResolverCallback valkey_glide_resolver_acquire(zval* callable);

/*
 * Free the libffi closure allocated by acquire().
 * Safe to call with NULL.
 */
void valkey_glide_resolver_release(AddressResolverCallback cb);

/*
 * Release any leaked closures (called from MSHUTDOWN/RSHUTDOWN).
 * With per-client acquire/release properly wired, this is a no-op safety net.
 */
void valkey_glide_resolver_shutdown(void);

#endif /* VALKEY_GLIDE_ADDRESS_RESOLVER_H */
