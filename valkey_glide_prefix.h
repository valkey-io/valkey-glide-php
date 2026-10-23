/*
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

#ifndef VALKEY_GLIDE_PREFIX_H
#define VALKEY_GLIDE_PREFIX_H

#include "common.h"

/*
 * Client-side key prefixing (PHPRedis OPT_PREFIX).
 *
 * The prefix is applied centrally, at the points where command arguments are
 * handed to the FFI layer (execute_command, execute_command_with_route and
 * buffer_command_for_batch), using a table of key positions per RequestType.
 * Those entry points only receive the GLIDE client handle, so connected
 * clients are tracked in a handle -> object registry to find the prefix.
 */

/* Arguments rewritten with prefixed keys. Owns every buffer it points to. */
typedef struct {
    uintptr_t*     args;
    unsigned long* args_len;
    char**         owned;
    unsigned long  owned_count;
} valkey_glide_prefixed_args_t;

/* Registry of connected clients, keyed by GLIDE client handle. */
void valkey_glide_prefix_track_client(const void* glide_client, valkey_glide_object* valkey_glide);
void valkey_glide_prefix_untrack_client(const void* glide_client);
valkey_glide_object* valkey_glide_prefix_find_object(const void* glide_client);
void                 valkey_glide_prefix_shutdown(void);

/* Set or clear the prefix. An empty string clears it, as in PHPRedis.
 * Returns false, leaving the prefix unchanged, if the value cannot be converted. */
bool valkey_glide_set_prefix(valkey_glide_object* valkey_glide, zval* value);

/* Return a new string holding prefix + key (a copy of key when no prefix is set). */
zend_string* valkey_glide_prefix_key(valkey_glide_object* valkey_glide,
                                     const char*          key,
                                     size_t               key_len);

/*
 * Prefix the key arguments of a command.
 * Returns true and fills `out` when at least one argument was rewritten; the
 * caller must then use out->args/out->args_len and release them with
 * valkey_glide_prefixed_args_free(). Returns false (and leaves `out` empty)
 * when nothing needs to change.
 */
bool valkey_glide_prefix_command_args(valkey_glide_object*          valkey_glide,
                                      enum RequestType              cmd_type,
                                      unsigned long                 arg_count,
                                      const uintptr_t*              args,
                                      const unsigned long*          args_len,
                                      valkey_glide_prefixed_args_t* out);

/* Same as above, looking the object up from the GLIDE client handle. */
bool valkey_glide_prefix_client_args(const void*                   glide_client,
                                     enum RequestType              cmd_type,
                                     unsigned long                 arg_count,
                                     const uintptr_t*              args,
                                     const unsigned long*          args_len,
                                     valkey_glide_prefixed_args_t* out);

/*
 * Prefix an explicit range of key arguments, for commands sent as
 * CustomCommand whose key positions the RequestType table cannot know
 * (EVAL / EVALSHA and their _RO variants).
 */
bool valkey_glide_prefix_arg_range(valkey_glide_object*          valkey_glide,
                                   unsigned long                 arg_count,
                                   const uintptr_t*              args,
                                   const unsigned long*          args_len,
                                   unsigned long                 first_key,
                                   unsigned long                 key_count,
                                   valkey_glide_prefixed_args_t* out);

void valkey_glide_prefixed_args_free(valkey_glide_prefixed_args_t* out);

#endif /* VALKEY_GLIDE_PREFIX_H */
