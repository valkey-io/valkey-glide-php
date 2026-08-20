/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor_common.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zend_exceptions.h>

#include "logger.h"

// Monotonic clock in milliseconds, used to bound waits against an absolute
// deadline (immune to wall-clock adjustments). Native only — no Zend APIs.
static int64_t now_monotonic_ms(void) {
#ifdef _WIN32
    return (int64_t) GetTickCount64();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

// Native monitor callback registry — accessed from background Rust thread.
// Uses a simple linked list protected by a native mutex (NOT Zend APIs).
typedef struct monitor_registry_entry {
    uintptr_t                      client_ptr;
    monitor_callback_info*         info;
    struct monitor_registry_entry* next;
} monitor_registry_entry;

static monitor_registry_entry* monitor_registry_head = NULL;
static mutex_t                 monitor_registry_mutex;
static bool                    monitor_registry_initialized = false;

// Initialize the native registry. Callback state is owned by the active
// monitor invocation, not by a process-global Zend HashTable, so it remains
// request-local and does not share request zvals across ZTS threads.
void init_monitor_callbacks(void) {
    if (!monitor_registry_initialized) {
        mutex_init(&monitor_registry_mutex);
        monitor_registry_initialized = true;
    }
}

// Note: there is intentionally no standalone native_registry_find() helper.
// valkey_glide_monitor_callback() performs the lookup inline while holding
// monitor_registry_mutex for the duration of its use of the returned `info`,
// to close a use-after-free race with concurrent unregistration/cleanup (see
// the comment on valkey_glide_monitor_callback for details).

// Native registry: add entry (thread-safe). Returns false on allocation
// failure, in which case the caller must not proceed to rely on this monitor
// ever receiving events (the background producer can only find registrations
// via this native registry).
static bool native_registry_add(uintptr_t client_ptr, monitor_callback_info* info) {
    monitor_registry_entry* entry =
        (monitor_registry_entry*) malloc(sizeof(monitor_registry_entry));
    if (!entry) {
        return false;
    }
    entry->client_ptr = client_ptr;
    entry->info       = info;
    mutex_lock(&monitor_registry_mutex);
    entry->next           = monitor_registry_head;
    monitor_registry_head = entry;
    mutex_unlock(&monitor_registry_mutex);
    return true;
}

// Native registry: remove entry (thread-safe)
static void native_registry_remove(uintptr_t client_ptr) {
    mutex_lock(&monitor_registry_mutex);
    monitor_registry_entry** pp = &monitor_registry_head;
    while (*pp) {
        if ((*pp)->client_ptr == client_ptr) {
            monitor_registry_entry* to_free = *pp;
            *pp                             = to_free->next;
            free(to_free);
            break;
        }
        pp = &((*pp)->next);
    }
    mutex_unlock(&monitor_registry_mutex);
}

// Cleanup monitor callback info after the Rust producer has stopped and the
// native registry entry has been removed. The info is invocation-owned, so it
// never needs to live in a process-global Zend container.

// Free a single queue node and all its owned fields (native allocations).
void valkey_glide_monitor_free_message(monitor_message* msg) {
    if (!msg)
        return;
    if (msg->client_addr)
        free(msg->client_addr);
    if (msg->command)
        free(msg->command);
    if (msg->args_json)
        free(msg->args_json);
    free(msg);
}

void cleanup_monitor_callback_info(monitor_callback_info* info) {
    if (info) {
        // Free all messages in queue
        monitor_message* msg = info->queue_head;
        while (msg) {
            monitor_message* next = msg->next;
            valkey_glide_monitor_free_message(msg);
            msg = next;
        }

        mutex_destroy(&info->queue_mutex);
        cond_destroy(&info->queue_cond);
        if (!Z_ISUNDEF(info->callback))
            zval_ptr_dtor(&info->callback);
        zval_ptr_dtor(&info->client_obj);

        efree(info);
    }
}

// Duplicate a length-delimited byte buffer into a NUL-terminated native
// string. Returns NULL if src is NULL/empty or on allocation failure.
static char* dup_len_str(const uint8_t* src, int64_t len) {
    if (!src || len <= 0)
        return NULL;
    char* out = (char*) malloc((size_t) len + 1);
    if (!out)
        return NULL;
    memcpy(out, src, (size_t) len);
    out[len] = '\0';
    return out;
}

// Build a queue node from the raw callback fields (native only, no Zend APIs).
// Returns NULL on allocation failure. The args are kept as raw JSON bytes and
// decoded later on the PHP main thread (see build_monitor_line_zval).
static monitor_message* build_monitor_message(double         timestamp,
                                              int64_t        db,
                                              const uint8_t* client_addr,
                                              int64_t        client_addr_len,
                                              const uint8_t* command_ptr,
                                              int64_t        command_len,
                                              const uint8_t* args_json,
                                              int64_t        args_json_len) {
    monitor_message* msg = (monitor_message*) calloc(1, sizeof(monitor_message));
    if (!msg)
        return NULL;
    msg->timestamp   = timestamp;
    msg->db          = db;
    msg->client_addr = dup_len_str(client_addr, client_addr_len);
    msg->command     = dup_len_str(command_ptr, command_len);
    msg->args_json   = dup_len_str(args_json, args_json_len);
    if (msg->args_json)
        msg->args_json_len = (size_t) args_json_len;
    return msg;
}

// Try to append msg to info's queue. Must be called WITHOUT holding
// info->queue_mutex. Returns true if the queue took ownership of msg; false if
// the message was rejected (monitor inactive or queue full), in which case the
// caller still owns msg and must free it. Full-queue rejections are recorded in
// dropped_count (surfaced via getDroppedCount()) rather than growing unbounded.
static bool try_enqueue_monitor_message(monitor_callback_info* info, monitor_message* msg) {
    bool enqueued = false;
    mutex_lock(&info->queue_mutex);
    // Re-check active state under queue_mutex: unregister/shutdown can
    // deactivate this monitor after the initial lookup.
    if (info->is_active) {
        if (info->queue_depth >= MONITOR_QUEUE_MAX_DEPTH) {
            info->dropped_count++;
        } else {
            if (info->queue_tail)
                info->queue_tail->next = msg;
            else
                info->queue_head = msg;
            info->queue_tail = msg;
            info->queue_depth++;
            cond_signal(&info->queue_cond);
            enqueued = true;
        }
    }
    mutex_unlock(&info->queue_mutex);
    return enqueued;
}

// C callback handler invoked by the FFI/Rust layer from a background thread
// when a new parsed MONITOR line arrives.
//
// CRITICAL: This runs on a background Rust thread — NO Zend APIs allowed here.
// We use only native C: malloc, mutex, and the native registry.
//
// This matches the MonitorCallback signature from glide_bindings.h:
//   void (*MonitorCallback)(uintptr_t client_ptr, double timestamp, int64_t db,
//                           const uint8_t *client_addr, int64_t client_addr_len,
//                           const uint8_t *command, int64_t command_len,
//                           const uint8_t *args_json, int64_t args_json_len)
//
// We store the RAW fields on the queue node. The PHP-side consumer builds a
// ValkeyGlideMonitorLine object (or formatted string) on the PHP thread.
//
// LIFETIME SAFETY: the registry lookup and all use of `info` below happen
// while holding monitor_registry_mutex for the *entire* function, not just
// the lookup. php_unregister_monitor_callback() also acquires this same
// mutex before removing the registry entry and before freeing `info`.
// Serializing on the single registry mutex guarantees this callback can never
// be left holding a pointer to `info` that is concurrently freed.
void valkey_glide_monitor_callback(uintptr_t      client_ptr,
                                   double         timestamp,
                                   int64_t        db,
                                   const uint8_t* client_addr,
                                   int64_t        client_addr_len,
                                   const uint8_t* command_ptr,
                                   int64_t        command_len,
                                   const uint8_t* args_json,
                                   int64_t        args_json_len) {
    mutex_lock(&monitor_registry_mutex);

    monitor_callback_info*  info  = NULL;
    monitor_registry_entry* entry = monitor_registry_head;
    while (entry) {
        if (entry->client_ptr == client_ptr) {
            info = entry->info;
            break;
        }
        entry = entry->next;
    }

    if (info) {
        // Snapshot active state before doing allocation work; try_enqueue
        // re-checks it under the queue lock to close the deactivation race.
        mutex_lock(&info->queue_mutex);
        bool is_active = info->is_active;
        mutex_unlock(&info->queue_mutex);

        if (is_active) {
            monitor_message* msg = build_monitor_message(timestamp,
                                                         db,
                                                         client_addr,
                                                         client_addr_len,
                                                         command_ptr,
                                                         command_len,
                                                         args_json,
                                                         args_json_len);
            // On rejection (inactive or full) we still own msg and must free it.
            if (msg && !try_enqueue_monitor_message(info, msg)) {
                valkey_glide_monitor_free_message(msg);
            }
        }
    }

    mutex_unlock(&monitor_registry_mutex);
}

// Dequeue one message, blocking up to timeout_ms. See header for semantics.
monitor_message* valkey_glide_monitor_dequeue(monitor_callback_info* info, long timeout_ms) {
    if (!info)
        return NULL;

    monitor_message* msg = NULL;
    mutex_lock(&info->queue_mutex);

    if (timeout_ms == 0) {
        // Non-blocking: take whatever is present.
    } else if (timeout_ms < 0) {
        // Wait indefinitely until a message arrives or the monitor stops.
        while (!info->queue_head && info->is_active) {
            cond_wait(&info->queue_cond, &info->queue_mutex);
        }
    } else {
        // Bounded wait against an absolute deadline. Looping re-checks the
        // predicate on spurious wakeups without ever waiting past the deadline.
        int64_t deadline_ms = now_monotonic_ms() + (int64_t) timeout_ms;
        while (!info->queue_head && info->is_active) {
            int64_t remaining = deadline_ms - now_monotonic_ms();
            if (remaining <= 0)
                break;
            (void) cond_timedwait(&info->queue_cond, &info->queue_mutex, (unsigned int) remaining);
        }
    }

    if (info->queue_head) {
        msg              = info->queue_head;
        info->queue_head = msg->next;
        if (!info->queue_head)
            info->queue_tail = NULL;
        info->queue_depth--;
        msg->next = NULL;
    }

    mutex_unlock(&info->queue_mutex);
    return msg;
}

// Register monitor callback. Returns the invocation-owned callback state, or
// NULL if native registration could not be allocated. The PHP monitor method
// owns this pointer for the duration of the synchronous monitor() call.
monitor_callback_info* php_register_monitor_callback(uintptr_t client_ptr,
                                                     zval*     callback,
                                                     zval*     client_obj) {
    init_monitor_callbacks();

    monitor_callback_info* info = emalloc(sizeof(monitor_callback_info));

    // Copy the callback (optional — NULL for pull-mode) and reference the
    // owning object.
    if (callback) {
        ZVAL_COPY(&info->callback, callback);
    } else {
        ZVAL_UNDEF(&info->callback);
    }
    ZVAL_COPY(&info->client_obj, client_obj);
    info->is_active = true;

    // Initialize message queue.
    info->queue_head    = NULL;
    info->queue_tail    = NULL;
    info->queue_depth   = 0;
    info->dropped_count = 0;
    mutex_init(&info->queue_mutex);
    cond_init(&info->queue_cond);

    info->in_monitor_mode = false;

    // The native registry is the only producer-side lookup path. If adding an
    // entry fails, report the failure to monitor() rather than entering a loop
    // that cannot receive events.
    if (!native_registry_add(client_ptr, info)) {
        mutex_destroy(&info->queue_mutex);
        cond_destroy(&info->queue_cond);
        if (!Z_ISUNDEF(info->callback))
            zval_ptr_dtor(&info->callback);
        zval_ptr_dtor(&info->client_obj);
        efree(info);
        return NULL;
    }

    return info;
}

// Unregister a monitor after close_monitor_client() has joined its Rust
// producer. This removes native lookup visibility before releasing PHP state.
void php_unregister_monitor_callback(uintptr_t client_ptr, monitor_callback_info* info) {
    if (!info)
        return;

    mutex_lock(&info->queue_mutex);
    info->is_active = false;
    cond_signal(&info->queue_cond);
    mutex_unlock(&info->queue_mutex);

    native_registry_remove(client_ptr);
    cleanup_monitor_callback_info(info);
}

// Shutdown monitor subsystem - called during module shutdown
void valkey_glide_monitor_shutdown(void) {
    if (monitor_registry_initialized) {
        // Mark every registered monitor inactive and wake any blocked
        // consumer, then release the lock BEFORE closing clients. This
        // avoids a deadlock where close_monitor_client() waits for the Rust
        // producer task to exit while that task's own registry lookup
        // (inside valkey_glide_monitor_callback) is blocked trying to
        // acquire monitor_registry_mutex.
        mutex_lock(&monitor_registry_mutex);
        for (monitor_registry_entry* entry = monitor_registry_head; entry; entry = entry->next) {
            if (entry->info) {
                mutex_lock(&entry->info->queue_mutex);
                entry->info->is_active = false;
                cond_signal(&entry->info->queue_cond);
                mutex_unlock(&entry->info->queue_mutex);
            }
        }
        mutex_unlock(&monitor_registry_mutex);

        // Close monitor clients WITHOUT holding the registry mutex. Module
        // shutdown implies no other PHP request thread can be concurrently
        // calling monitor()/unregister (which are the only writers of the
        // registry list), so it is safe to walk the list a second time here
        // using only the stable client_ptr/next fields without re-acquiring
        // the lock. This intentionally avoids taking a heap snapshot (and
        // the allocation-failure handling that would require) — a failure
        // to snapshot must never result in skipping close_monitor_client()
        // for any active producer, since that would let shutdown destroy
        // monitor_registry_mutex and callback state out from under it.
        for (monitor_registry_entry* entry = monitor_registry_head; entry; entry = entry->next) {
            close_monitor_client((const void*) entry->client_ptr);
        }
    }

    if (monitor_registry_initialized) {
        // Free the native registry entries.
        //
        // We intentionally free only the entry nodes here, NOT entry->info.
        // monitor_callback_info is emalloc'd (request heap) and its cleanup
        // (cleanup_monitor_callback_info) runs zval_ptr_dtor + efree — all
        // request-scoped operations. MSHUTDOWN runs after the request and its
        // memory manager are gone, so calling that here would be a
        // use-after-free. Cleanup is owned by valkey_glide_monitor_teardown(),
        // which every ValkeyGlideMonitor object's free handler invokes at
        // request shutdown (unregistering the entry and freeing info while the
        // heap is still valid). In normal operation this list is therefore
        // already empty; this loop only reclaims the native entry nodes should
        // any straggler remain.
        mutex_lock(&monitor_registry_mutex);
        monitor_registry_entry* entry = monitor_registry_head;
        while (entry) {
            monitor_registry_entry* next = entry->next;
            free(entry);
            entry = next;
        }
        monitor_registry_head = NULL;
        mutex_unlock(&monitor_registry_mutex);
        mutex_destroy(&monitor_registry_mutex);
        monitor_registry_initialized = false;
    }
}
