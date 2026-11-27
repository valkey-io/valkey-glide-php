/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */
#ifndef VALKEY_GLIDE_CREDENTIAL_PROVIDER_H
#define VALKEY_GLIDE_CREDENTIAL_PROVIDER_H

#include <stdbool.h>
#include <stdint.h>

#include "include/glide_bindings.h"
#include "php.h"

/* Key of the PHP callable inside the iamConfig array */
#define VALKEY_GLIDE_IAM_CONFIG_CREDENTIAL_PROVIDER "credentialProvider"

/* Largest accepted size of one credential field, and of all three together (the core's limit) */
#define VALKEY_GLIDE_CREDENTIAL_MAX_BYTES (1024 * 1024)

/*
 * A custom IAM credential provider: a PHP callable returning
 * ['accessKeyId' => string, 'secretAccessKey' => string,
 *  'sessionToken' => ?string, 'expiresAt' => ?int (Unix epoch milliseconds)].
 *
 * The core asks for credentials on its own threads, where PHP code must not
 * run. So the callable only runs on the PHP thread that created the client, and
 * its last result is kept as a snapshot in a process-global registry that
 * valkey_glide_credential_callback() serves without touching Zend. The PHP
 * thread renews the snapshot before core calls once it is due, and in
 * refreshIamToken().
 */
typedef struct valkey_glide_credential_provider valkey_glide_credential_provider_t;

/*
 * Call the callable, validate its result and register the snapshot.
 * Returns NULL with a ValkeyGlideException pending on failure.
 * refresh_interval_seconds is the iamConfig value; 0 means the core's default.
 */
valkey_glide_credential_provider_t* valkey_glide_credential_provider_create(
    zval* callable, int refresh_interval_seconds);

/* The client_id to pass to create_client_with_credential_provider() */
uintptr_t valkey_glide_credential_provider_id(const valkey_glide_credential_provider_t* provider);

/* The callable the provider holds, NULL if none: for the client's get_gc handler, so PHP can
 * collect a client that its own callable references */
zval* valkey_glide_credential_provider_callable(valkey_glide_credential_provider_t* provider);

/*
 * Call the callable now and replace the snapshot. With throw_on_error a failure
 * throws ValkeyGlideException; otherwise the exception is cleared and the
 * previous snapshot stays. Returns whether a new snapshot was installed.
 */
bool valkey_glide_credential_provider_fetch(valkey_glide_credential_provider_t* provider,
                                            bool                                throw_on_error);

/*
 * Before a core call: renew the snapshot of the provider of the client that
 * owns glide_client if it is due. Never throws. Costs one compare when this
 * thread has no provider.
 */
void valkey_glide_credential_refresh_if_due(const void* glide_client);

/* Whether the client that owns glide_client has a provider whose callable can still run, so a
 * long wait must wake up to renew its snapshot. */
bool valkey_glide_credential_client_has_provider(const void* glide_client);

/*
 * Remove the snapshot from the registry (a later core call gets no credentials)
 * and free the provider. PHP thread only. Safe to call with NULL.
 */
void valkey_glide_credential_provider_free(valkey_glide_credential_provider_t* provider);

/*
 * Whether a provider callable is running on this thread. A client closed from
 * inside one is closed after the core call that was about to run, like one
 * closed inside an address resolver callable.
 */
bool valkey_glide_credential_in_callable(void);

/* RSHUTDOWN: release this thread's callables and close their snapshots. The
 * providers themselves are freed when their clients are. */
void valkey_glide_credential_request_shutdown(void);

/*
 * The CredentialProviderCallback the core calls on its own threads. Serves the
 * snapshot registered under client_id and never touches Zend.
 * Returns 1 on success, 2 when a buffer is too small (all three lengths set,
 * nothing written), 0 when there is no usable snapshot.
 */
uint8_t valkey_glide_credential_callback(uintptr_t  client_id,
                                         uint8_t*   access_key_id_buf,
                                         uintptr_t  access_key_id_buf_len,
                                         uintptr_t* access_key_id_len,
                                         uint8_t*   secret_access_key_buf,
                                         uintptr_t  secret_access_key_buf_len,
                                         uintptr_t* secret_access_key_len,
                                         uint8_t*   session_token_buf,
                                         uintptr_t  session_token_buf_len,
                                         uintptr_t* session_token_len,
                                         int64_t*   expires_at_epoch_millis);

#endif /* VALKEY_GLIDE_CREDENTIAL_PROVIDER_H */
