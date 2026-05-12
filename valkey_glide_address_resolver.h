/** Copyright Valkey GLIDE Project Contributors - SPDX-Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_ADDRESS_RESOLVER_H
#define VALKEY_GLIDE_ADDRESS_RESOLVER_H

#include "include/glide_bindings.h"
#include "php.h"

void     valkey_glide_set_address_resolver(zval* callable);
void     valkey_glide_clear_address_resolver(void);
uint16_t valkey_glide_address_resolver_callback(const uint8_t* host,
                                                uintptr_t      host_len,
                                                uint16_t       port,
                                                uint8_t*       resolved_host_buf,
                                                uintptr_t      resolved_host_buf_len,
                                                uintptr_t*     resolved_host_len);

#endif /* VALKEY_GLIDE_ADDRESS_RESOLVER_H */
