/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor_common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zend_exceptions.h>

#include "logger.h"

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
    if (msg->args) {
        for (size_t i = 0; i < msg->args_count; i++) {
            free(msg->args[i]);
        }
        free(msg->args);
    }
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

// Decode a single JSON string token into a newly malloc'd, null-terminated
// buffer of raw bytes. `*pp` must point at the opening quote; on success it is
// advanced past the closing quote. Returns NULL on allocation failure or if the
// token is not a well-formed string. Runs on the background thread — native
// only, no Zend APIs.
static char* decode_json_string(const char** pp, const char* end) {
    const char* p = *pp;
    if (p >= end || *p != '"')
        return NULL;
    p++;  // skip opening quote

    // Worst case decoded length <= remaining bytes; allocate generously.
    size_t cap = (size_t)(end - p) + 1;
    char*  out = (char*) malloc(cap);
    if (!out)
        return NULL;
    size_t olen = 0;

    while (p < end && *p != '"') {
        if (*p == '\\' && (p + 1) < end) {
            p++;  // skip backslash
            switch (*p) {
                case '"': out[olen++] = '"'; break;
                case '\\': out[olen++] = '\\'; break;
                case '/': out[olen++] = '/'; break;
                case 'n': out[olen++] = '\n'; break;
                case 'r': out[olen++] = '\r'; break;
                case 't': out[olen++] = '\t'; break;
                case 'b': out[olen++] = '\b'; break;
                case 'f': out[olen++] = '\f'; break;
                case 'u': {
                    if ((p + 4) < end) {
                        unsigned int cp    = 0;
                        bool         valid = true;
                        for (int i = 1; i <= 4; i++) {
                            char ch = p[i];
                            cp <<= 4;
                            if (ch >= '0' && ch <= '9')
                                cp |= (ch - '0');
                            else if (ch >= 'a' && ch <= 'f')
                                cp |= (ch - 'a' + 10);
                            else if (ch >= 'A' && ch <= 'F')
                                cp |= (ch - 'A' + 10);
                            else {
                                valid = false;
                                break;
                            }
                        }
                        if (valid) {
                            p += 4;  // consume the 4 hex digits
                            // UTF-16 surrogate pair
                            if (cp >= 0xD800 && cp <= 0xDBFF && (p + 6) < end && p[1] == '\\' &&
                                p[2] == 'u') {
                                unsigned int low       = 0;
                                bool         low_valid = true;
                                for (int i = 3; i <= 6; i++) {
                                    char ch = p[i];
                                    low <<= 4;
                                    if (ch >= '0' && ch <= '9')
                                        low |= (ch - '0');
                                    else if (ch >= 'a' && ch <= 'f')
                                        low |= (ch - 'a' + 10);
                                    else if (ch >= 'A' && ch <= 'F')
                                        low |= (ch - 'A' + 10);
                                    else {
                                        low_valid = false;
                                        break;
                                    }
                                }
                                if (low_valid && low >= 0xDC00 && low <= 0xDFFF) {
                                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                                    p += 6;  // consume \uXXXX of low surrogate
                                }
                            }
                            // Encode code point as UTF-8
                            if (cp < 0x80) {
                                out[olen++] = (char) cp;
                            } else if (cp < 0x800) {
                                out[olen++] = (char) (0xC0 | (cp >> 6));
                                out[olen++] = (char) (0x80 | (cp & 0x3F));
                            } else if (cp < 0x10000) {
                                out[olen++] = (char) (0xE0 | (cp >> 12));
                                out[olen++] = (char) (0x80 | ((cp >> 6) & 0x3F));
                                out[olen++] = (char) (0x80 | (cp & 0x3F));
                            } else {
                                out[olen++] = (char) (0xF0 | (cp >> 18));
                                out[olen++] = (char) (0x80 | ((cp >> 12) & 0x3F));
                                out[olen++] = (char) (0x80 | ((cp >> 6) & 0x3F));
                                out[olen++] = (char) (0x80 | (cp & 0x3F));
                            }
                        }
                    }
                    break;
                }
                default:
                    // Unknown escape — emit the escaped char literally.
                    out[olen++] = *p;
                    break;
            }
            p++;
        } else {
            out[olen++] = *p++;
        }
    }

    if (p < end && *p == '"')
        p++;  // skip closing quote

    out[olen] = '\0';
    *pp       = p;
    return out;
}

// Parse a JSON array of strings ["a","b",...] into a newly-allocated char**
// array of decoded, null-terminated C strings. Sets *out_count. Returns NULL
// (count 0) if there are no elements. Native only — no Zend APIs.
static char** parse_json_string_array(const uint8_t* args_json,
                                       int64_t        args_json_len,
                                       size_t*        out_count) {
    *out_count = 0;
    if (!args_json || args_json_len <= 2)
        return NULL;

    const char* p   = (const char*) args_json;
    const char* end = p + args_json_len;

    if (*p == '[')
        p++;

    // First pass upper bound: at most args_json_len string slots.
    size_t cap  = 8;
    char** args = (char**) malloc(cap * sizeof(char*));
    if (!args)
        return NULL;
    size_t count = 0;

    while (p < end && *p != ']') {
        while (p < end && (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t'))
            p++;
        if (p >= end || *p == ']')
            break;

        if (*p == '"') {
            char* s = decode_json_string(&p, end);
            if (!s)
                break;  // malformed / OOM — stop
            if (count == cap) {
                size_t newcap  = cap * 2;
                char** newargs = (char**) realloc(args, newcap * sizeof(char*));
                if (!newargs) {
                    free(s);
                    break;
                }
                args = newargs;
                cap  = newcap;
            }
            args[count++] = s;
        } else {
            // Non-string token (shouldn't occur for command args) — skip it.
            while (p < end && *p != ',' && *p != ']')
                p++;
        }
    }

    if (count == 0) {
        free(args);
        return NULL;
    }
    *out_count = count;
    return args;
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
// We store the RAW decoded fields on the queue node. The PHP-side consumer
// builds a ValkeyGlideMonitorLine object (or formatted string) on the PHP
// thread. No string reconstruction happens here.
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
    monitor_message* msg = NULL;

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

    if (!info) {
        goto unlock_registry;
    }

    mutex_lock(&info->queue_mutex);
    bool is_active = info->is_active;
    mutex_unlock(&info->queue_mutex);
    if (!is_active) {
        goto unlock_registry;
    }

    {
        msg = (monitor_message*) calloc(1, sizeof(monitor_message));
        if (!msg)
            goto unlock_registry;

        msg->timestamp = timestamp;
        msg->db        = db;
        msg->next      = NULL;

        if (client_addr && client_addr_len > 0) {
            msg->client_addr = (char*) malloc((size_t) client_addr_len + 1);
            if (msg->client_addr) {
                memcpy(msg->client_addr, client_addr, (size_t) client_addr_len);
                msg->client_addr[client_addr_len] = '\0';
            }
        }

        if (command_ptr && command_len > 0) {
            msg->command = (char*) malloc((size_t) command_len + 1);
            if (msg->command) {
                memcpy(msg->command, command_ptr, (size_t) command_len);
                msg->command[command_len] = '\0';
            }
        }

        msg->args = parse_json_string_array(args_json, args_json_len, &msg->args_count);

        // Enqueue (thread-safe) with bounded depth. Re-check active state
        // under queue_mutex: unregister/shutdown can deactivate this monitor
        // after the initial lookup.
        mutex_lock(&info->queue_mutex);
        if (!info->is_active) {
            mutex_unlock(&info->queue_mutex);
            goto free_msg;
        }
        if (info->queue_depth >= MONITOR_QUEUE_MAX_DEPTH) {
            // Queue full — drop and record the loss (surfaced via
            // getDroppedCount()), rather than growing memory unbounded.
            info->dropped_count++;
            mutex_unlock(&info->queue_mutex);
            goto free_msg;
        }
        if (info->queue_tail) {
            info->queue_tail->next = msg;
        } else {
            info->queue_head = msg;
        }
        info->queue_tail = msg;
        info->queue_depth++;
        cond_signal(&info->queue_cond);
        mutex_unlock(&info->queue_mutex);
    }

    mutex_unlock(&monitor_registry_mutex);
    return;

free_msg:
    valkey_glide_monitor_free_message(msg);
unlock_registry:
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
        // Bounded wait. Loop in <=1s slices so PHP interruptions are honored
        // by the caller between calls; here we just wait up to timeout_ms.
        long remaining = timeout_ms;
        while (!info->queue_head && info->is_active && remaining > 0) {
            long slice = remaining < 1000 ? remaining : 1000;
            (void) cond_timedwait(&info->queue_cond, &info->queue_mutex, slice);
            remaining -= slice;
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
        // Free the native registry entries
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
