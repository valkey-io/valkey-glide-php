/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor.h"

#include <zend_exceptions.h>

#include <ext/json/php_json.h>
#include <ext/standard/info.h>

#include "logger.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_monitor_arginfo.h"

/* Globals */
zend_class_entry*    valkey_glide_monitor_ce;
zend_class_entry*    valkey_glide_monitor_line_ce;
zend_object_handlers valkey_glide_monitor_object_handlers;

/* ------------------------------------------------------------------ */
/* Value object construction                                          */
/* ------------------------------------------------------------------ */

/* Build a ValkeyGlideMonitorLine object from a raw queue node.
 * Runs on the PHP thread (safe to touch Zend). Returns the new object in
 * `return_value` style via the provided out zval. */
static void build_monitor_line_zval(zval* out, const monitor_message* msg) {
    object_init_ex(out, valkey_glide_monitor_line_ce);

    zend_update_property_double(valkey_glide_monitor_line_ce,
                                Z_OBJ_P(out),
                                "timestamp",
                                sizeof("timestamp") - 1,
                                msg->timestamp);
    zend_update_property_long(
        valkey_glide_monitor_line_ce, Z_OBJ_P(out), "db", sizeof("db") - 1, msg->db);
    zend_update_property_string(valkey_glide_monitor_line_ce,
                                Z_OBJ_P(out),
                                "clientAddr",
                                sizeof("clientAddr") - 1,
                                msg->client_addr ? msg->client_addr : "");
    zend_update_property_string(valkey_glide_monitor_line_ce,
                                Z_OBJ_P(out),
                                "command",
                                sizeof("command") - 1,
                                msg->command ? msg->command : "");

    zval args;
    bool decoded = false;
    if (msg->args_json && msg->args_json_len > 0) {
        /* Decode the raw JSON args array with PHP's own json parser (safe here
         * on the PHP main thread). Falls back to an empty array on any decode
         * error or unexpected type. */
        if (php_json_decode(
                &args, msg->args_json, msg->args_json_len, 1, PHP_JSON_PARSER_DEFAULT_DEPTH) ==
                SUCCESS &&
            Z_TYPE(args) == IS_ARRAY) {
            decoded = true;
        } else {
            zval_ptr_dtor(&args);
        }
    }
    if (!decoded) {
        array_init(&args);
    }
    zend_update_property(
        valkey_glide_monitor_line_ce, Z_OBJ_P(out), "args", sizeof("args") - 1, &args);
    zval_ptr_dtor(&args);
}

/* ------------------------------------------------------------------ */
/* Object lifecycle                                                   */
/* ------------------------------------------------------------------ */

zend_object* create_valkey_glide_monitor_object(zend_class_entry* ce) {
    valkey_glide_monitor_object* obj =
        ecalloc(1, sizeof(valkey_glide_monitor_object) + zend_object_properties_size(ce));

    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);

    obj->monitor_client_ptr = NULL;
    obj->info               = NULL;
    obj->dropped_count      = 0;

    obj->std.handlers = &valkey_glide_monitor_object_handlers;
    return &obj->std;
}

/* Stop the monitor and release its connection + queue state. Idempotent. */
static void valkey_glide_monitor_teardown(valkey_glide_monitor_object* obj) {
    if (obj->monitor_client_ptr) {
        /* Close the FFI connection FIRST so the background producer stops
         * before we free the callback state it writes into. */
        close_monitor_client(obj->monitor_client_ptr);
        if (obj->info) {
            /* Preserve the cumulative drop count on the object before freeing
             * `info`, so getDroppedCount() still reports losses after close(). */
            mutex_lock(&obj->info->queue_mutex);
            obj->dropped_count = obj->info->dropped_count;
            mutex_unlock(&obj->info->queue_mutex);

            php_unregister_monitor_callback((uintptr_t) obj->monitor_client_ptr, obj->info);
        }
        obj->monitor_client_ptr = NULL;
        obj->info               = NULL;
    }
}

void free_valkey_glide_monitor_object(zend_object* object) {
    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_GET_OBJECT(object);
    valkey_glide_monitor_teardown(obj);
    zend_object_std_dtor(&obj->std);
}

/* ------------------------------------------------------------------ */
/* Constructor                                                        */
/* ------------------------------------------------------------------ */

PHP_METHOD(ValkeyGlideMonitor, __construct) {
    zval*     addresses           = NULL;
    zend_bool use_tls             = 0;
    zval*     credentials         = NULL;
    zval*     database_id_zv      = NULL;
    char*     client_name         = NULL;
    size_t    client_name_len     = 0;
    zval*     context             = NULL;
    char*     lib_name            = NULL;
    size_t    lib_name_len        = 0;
    char*     client_info_tag     = NULL;
    size_t    client_info_tag_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 8)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(addresses)
    Z_PARAM_BOOL(use_tls)
    Z_PARAM_ARRAY_OR_NULL(credentials)
    Z_PARAM_ZVAL_OR_NULL(database_id_zv)
    Z_PARAM_STRING_OR_NULL(client_name, client_name_len)
    Z_PARAM_ZVAL_OR_NULL(context)
    Z_PARAM_STRING_OR_NULL(lib_name, lib_name_len)
    Z_PARAM_STRING_OR_NULL(client_info_tag, client_info_tag_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());

    /* Build the shared connection configuration from the parsed parameters. */
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);

    common_params.use_tls     = use_tls;
    common_params.credentials = credentials;

    if (database_id_zv != NULL && Z_TYPE_P(database_id_zv) != IS_NULL) {
        common_params.database_id         = Z_LVAL_P(database_id_zv);
        common_params.database_id_is_null = false;
    }
    common_params.client_name         = client_name;
    common_params.client_name_len     = client_name_len;
    common_params.context             = context;
    common_params.lib_name            = lib_name;
    common_params.lib_name_len        = lib_name_len;
    common_params.client_info_tag     = client_info_tag;
    common_params.client_info_tag_len = client_info_tag_len;

    /* Default to localhost:6379 when no addresses are supplied. */
    zval      addresses_array;
    zend_bool created_addresses = false;
    if (addresses == NULL) {
        array_init(&addresses_array);
        zval address_entry;
        array_init(&address_entry);
        add_assoc_string(&address_entry, "host", "localhost");
        add_assoc_long(&address_entry, "port", 6379);
        add_next_index_zval(&addresses_array, &address_entry);
        common_params.addresses = &addresses_array;
        created_addresses       = true;
    } else {
        common_params.addresses = addresses;
    }

    if (common_params.database_id_is_null == false && common_params.database_id < 0) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Database ID must be non-negative.", 0);
        if (created_addresses)
            zval_ptr_dtor(&addresses_array);
        RETURN_THROWS();
    }

    /* Validate lib_name and client_info_tag charset, as the standalone and cluster
     * constructors do, so a monitor client reports the same effective library name
     * and rejects the same values. */
    {
        const char* metadata_error =
            client_metadata_validation_error(common_params.lib_name,
                                             common_params.lib_name_len,
                                             common_params.client_info_tag,
                                             common_params.client_info_tag_len);
        if (metadata_error != NULL) {
            zend_throw_exception(get_valkey_glide_exception_ce(), metadata_error, 0);
            if (created_addresses)
                zval_ptr_dtor(&addresses_array);
            RETURN_THROWS();
        }
    }

    valkey_glide_base_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));
    if (valkey_glide_build_client_config_base(&common_params, &client_config, false) == FAILURE) {
        if (created_addresses)
            zval_ptr_dtor(&addresses_array);
        RETURN_THROWS();
    }

    /* Serialize the config to protobuf bytes for the monitor connection. */
    size_t   req_len   = 0;
    uint8_t* req_bytes = create_connection_request(
        &req_len, &client_config, VALKEY_GLIDE_PERIODIC_CHECKS_DISABLED, false, false);

    if (created_addresses)
        zval_ptr_dtor(&addresses_array);

    if (!req_bytes || req_len == 0) {
        valkey_glide_cleanup_client_config(&client_config);
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to build monitor connection request", 0);
        RETURN_THROWS();
    }

    /* Open the dedicated MONITOR connection.
     *
     * Ensure the native registry mutex is initialized BEFORE starting the Rust
     * producer. create_monitor_client() activates valkey_glide_monitor_callback,
     * which locks monitor_registry_mutex; if an event arrived before
     * php_register_monitor_callback() (which also initializes the mutex) ran,
     * the callback would lock an uninitialized mutex. init is idempotent. */
    init_monitor_callbacks();

    const struct ConnectionResponse* resp =
        create_monitor_client(req_bytes, req_len, valkey_glide_monitor_callback);

    /* The request bytes may contain credentials — zero before freeing. */
    memset(req_bytes, 0, req_len);
    efree(req_bytes);
    valkey_glide_cleanup_client_config(&client_config);

    if (!resp) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to create monitor client: null response", 0);
        RETURN_THROWS();
    }
    if (resp->connection_error_message) {
        const char* err = resp->connection_error_message;
        VALKEY_LOG_ERROR("monitor", err);
        zend_throw_exception(get_valkey_glide_exception_ce(), err, 0);
        free_connection_response((struct ConnectionResponse*) resp);
        RETURN_THROWS();
    }

    const void* monitor_client_ptr = resp->conn_ptr;
    free_connection_response((struct ConnectionResponse*) resp);

    if (!monitor_client_ptr) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to create monitor client: null client ptr", 0);
        RETURN_THROWS();
    }

    /* Register pull-mode callback state (NULL callback; listen() sets one). */
    monitor_callback_info* info =
        php_register_monitor_callback((uintptr_t) monitor_client_ptr, NULL, getThis());
    if (!info) {
        close_monitor_client(monitor_client_ptr);
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to register monitor callback", 0);
        RETURN_THROWS();
    }

    obj->monitor_client_ptr = monitor_client_ptr;
    obj->info               = info;
}

/* ------------------------------------------------------------------ */
/* Pull API                                                           */
/* ------------------------------------------------------------------ */

static void monitor_pull(INTERNAL_FUNCTION_PARAMETERS, long timeout_ms) {
    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());
    if (!obj->info) {
        RETURN_NULL();
    }

    monitor_message* msg = valkey_glide_monitor_dequeue(obj->info, timeout_ms);
    if (!msg) {
        RETURN_NULL();
    }

    build_monitor_line_zval(return_value, msg);
    valkey_glide_monitor_free_message(msg);
}

PHP_METHOD(ValkeyGlideMonitor, getMonitorMessage) {
    zval* timeout_zv = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
    Z_PARAM_OPTIONAL
    Z_PARAM_ZVAL_OR_NULL(timeout_zv)
    ZEND_PARSE_PARAMETERS_END();

    long timeout_ms = -1; /* wait indefinitely by default */
    if (timeout_zv != NULL && Z_TYPE_P(timeout_zv) != IS_NULL) {
        double secs = zval_get_double(timeout_zv);
        timeout_ms  = (long) (secs * 1000.0);
        if (timeout_ms < 0)
            timeout_ms = 0;
    }

    monitor_pull(INTERNAL_FUNCTION_PARAM_PASSTHRU, timeout_ms);
}

PHP_METHOD(ValkeyGlideMonitor, tryGetMonitorMessage) {
    ZEND_PARSE_PARAMETERS_NONE();
    monitor_pull(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

/* ------------------------------------------------------------------ */
/* Callback loop (PHP-specific convenience)                            */
/* ------------------------------------------------------------------ */

PHP_METHOD(ValkeyGlideMonitor, listen) {
    zend_fcall_info       fci;
    zend_fcall_info_cache fcc;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());
    if (!obj->info) {
        RETURN_FALSE;
    }

    while (true) {
        /* The callback may have called close() on a previous iteration, which
         * tears down and clears obj->info. Stop before touching freed state. */
        if (!obj->info) {
            break;
        }

        /* Honor PHP interruption (max_execution_time, signals) between waits. */
#ifdef ZEND_ATOMIC_BOOL_INIT
        if (zend_atomic_bool_load_ex(&EG(vm_interrupt))) {
#else
        if (EG(vm_interrupt)) {
#endif
            break;
        }

        monitor_message* msg = valkey_glide_monitor_dequeue(obj->info, 1000);
        if (!msg) {
            /* Timeout with no message: re-check active state and loop. */
            bool active;
            mutex_lock(&obj->info->queue_mutex);
            active = obj->info->is_active;
            mutex_unlock(&obj->info->queue_mutex);
            if (!active)
                break;
            continue;
        }

        zval line;
        build_monitor_line_zval(&line, msg);
        valkey_glide_monitor_free_message(msg);

        zval retval;
        ZVAL_UNDEF(&retval);
        fci.retval      = &retval;
        fci.param_count = 1;
        fci.params      = &line;

        bool stop = false;
        if (zend_call_function(&fci, &fcc) == SUCCESS) {
            if (EG(exception)) {
                stop = true;
            } else if (Z_TYPE(retval) != IS_NULL) {
                stop = true; /* non-null return exits (PHPRedis semantics) */
            }
        } else {
            stop = true;
        }

        zval_ptr_dtor(&retval);
        zval_ptr_dtor(&line);

        /* The callback may have called close() (freeing obj->info) even while
         * returning null; stop rather than dereferencing freed state above. */
        if (stop || !obj->info)
            break;
    }

    if (EG(exception)) {
        RETURN_THROWS();
    }
    RETURN_TRUE;
}

/* ------------------------------------------------------------------ */
/* Introspection + teardown                                           */
/* ------------------------------------------------------------------ */

PHP_METHOD(ValkeyGlideMonitor, getDroppedCount) {
    ZEND_PARSE_PARAMETERS_NONE();

    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());

    /* While active, report the live count (and refresh the cached value).
     * After close(), the cached value preserved during teardown is returned. */
    if (obj->info) {
        mutex_lock(&obj->info->queue_mutex);
        obj->dropped_count = obj->info->dropped_count;
        mutex_unlock(&obj->info->queue_mutex);
    }
    RETURN_LONG((zend_long) obj->dropped_count);
}

PHP_METHOD(ValkeyGlideMonitor, close) {
    ZEND_PARSE_PARAMETERS_NONE();
    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());
    valkey_glide_monitor_teardown(obj);
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void register_valkey_glide_monitor_classes(void) {
    /* Initialize the native registry mutex at module startup so it is always
     * ready before any monitor client (and its background producer) can be
     * created. This closes the window where a MONITOR event could reach the
     * callback before the mutex was initialized. */
    init_monitor_callbacks();

    valkey_glide_monitor_line_ce = register_class_ValkeyGlideMonitorLine();

    valkey_glide_monitor_ce                = register_class_ValkeyGlideMonitor();
    valkey_glide_monitor_ce->create_object = create_valkey_glide_monitor_object;

    memcpy(&valkey_glide_monitor_object_handlers,
           zend_get_std_object_handlers(),
           sizeof(valkey_glide_monitor_object_handlers));
    valkey_glide_monitor_object_handlers.offset   = XtOffsetOf(valkey_glide_monitor_object, std);
    valkey_glide_monitor_object_handlers.free_obj = free_valkey_glide_monitor_object;
}
