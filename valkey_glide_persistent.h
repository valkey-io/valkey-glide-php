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

#ifndef VALKEY_GLIDE_PERSISTENT_H
#define VALKEY_GLIDE_PERSISTENT_H

#include "common.h"

/*
 * Persistent clients (PHPRedis pconnect / RedisCluster $persistent).
 *
 * A persistent GLIDE client survives the end of a request and is reused by a
 * later request in the same worker process (e.g. a PHP-FPM worker), avoiding a
 * new connection, TLS handshake and cluster topology discovery per request.
 *
 * - Clients are kept in EG(persistent_list), keyed by client type,
 *   persistent_id and the serialized connection request, so a client is only
 *   reused for an identical configuration (addresses, credentials, database,
 *   TLS, ...).
 * - A persistent client is used by one object at a time; a second object with
 *   the same configuration in the same request gets an ordinary client.
 * - A client whose connection state the request changed (AUTH, RESET,
 *   CLIENT SETNAME, SUBSCRIBE, raw SELECT, ...) is closed instead of reused.
 *   After select() the configured database is selected again, and a WATCH
 *   still active at the end of the request is cleared with UNWATCH; the
 *   client is closed if either fails.
 * - A client is never reused or closed by a process other than the one that
 *   created it (fork).
 * - At most valkey_glide.max_persistent_clients clients are kept per process
 *   (per thread under ZTS, like EG(persistent_list));
 *   at the limit the least recently used idle client is closed to make room.
 */

/* Default for the valkey_glide.max_persistent_clients ini setting */
#define VALKEY_GLIDE_DEFAULT_MAX_PERSISTENT_CLIENTS "32"

/* Register the persistent resource type. Call from MINIT. */
void valkey_glide_persistent_minit(int module_number);

/*
 * Before creating a client with persistence requested: reuse a kept client if
 * one is available. Returns true if the object was attached to a kept client
 * (nothing left to do). Otherwise sets *out_key to the key to pass to
 * valkey_glide_persistent_store() after the client is created, or to NULL if
 * the new client must not be kept (address resolver configured, the kept client
 * is in use by another object, ...). The caller must check EG(exception).
 */
bool valkey_glide_persistent_begin(valkey_glide_object*                      valkey_glide,
                                   valkey_glide_base_client_configuration_t* config,
                                   valkey_glide_periodic_checks_status_t     periodic_checks,
                                   bool                                      is_cluster,
                                   bool          refresh_topology_from_initial_nodes,
                                   const char*   persistent_id,
                                   size_t        persistent_id_len,
                                   zend_string** out_key);

/* Keep the object's freshly created client under `key` (takes ownership),
 * unless the per-process (per-thread under ZTS) limit is reached and every kept client is in use.
 * `database_id` is the configured database (-1 if not set). */
void valkey_glide_persistent_store(valkey_glide_object* valkey_glide,
                                   zend_string*         key,
                                   int                  database_id);

/* Release the object's client at close() or object destruction: a persistent
 * client is kept for reuse unless `closing` is set or the request changed its
 * connection state; any other client is closed. */
void valkey_glide_release_client(valkey_glide_object* valkey_glide, bool closing);

/* Note that the object changed connection-level state that a later request
 * must not inherit, so its persistent client is not reused. */
void valkey_glide_mark_connection_state_changed(valkey_glide_object* valkey_glide);

/* Whether a raw command (name, and subcommand if any) changes connection-level
 * state. */
bool valkey_glide_command_changes_connection_state(const char* cmd,
                                                   size_t      cmd_len,
                                                   const char* sub,
                                                   size_t      sub_len);

#endif /* VALKEY_GLIDE_PERSISTENT_H */
