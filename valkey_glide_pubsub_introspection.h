/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_PUBSUB_INTROSPECTION_H
#define VALKEY_GLIDE_PUBSUB_INTROSPECTION_H

#include "php.h"

// PUBSUB introspection command implementation
void valkey_glide_pubsub_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection);

#endif  // VALKEY_GLIDE_PUBSUB_INTROSPECTION_H
