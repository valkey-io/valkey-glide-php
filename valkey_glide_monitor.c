/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_monitor.h"

#include <zend_exceptions.h>

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
    array_init(&args);
    for (size_t i = 0; i < msg->args_count; i++) {
        add_next_index_string(&args, msg->args[i] ? msg->args[i] : "");
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
    zval*     addresses          = NULL;
    zend_bool use_tls            = 0;
    zval*     credentials        = NULL;
    zend_long read_from          = VALKEY_GLIDE_READ_FROM_PRIMARY;
    zval*     request_timeout_zv = NULL;
    zval*     reconnect_strategy = NULL;
    zval*     database_id_zv     = NULL;
    char*     client_name        = NULL;
    size_t    client_name_len    = 0;
    char*     client_az          = NULL;
    size_t    client_az_len      = 0;
    zval*     advanced_config    = NULL;
    zval*     lazy_connect_zv    = NULL;
    zval*     context            = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 12)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY_OR_NULL(addresses)
    Z_PARAM_BOOL(use_tls)
    Z_PARAM_ARRAY_OR_NULL(credentials)
    Z_PARAM_LONG(read_from)
    Z_PARAM_ZVAL_OR_NULL(request_timeout_zv)
    Z_PARAM_ARRAY_OR_NULL(reconnect_strategy)
    Z_PARAM_ZVAL_OR_NULL(database_id_zv)
    Z_PARAM_STRING_OR_NULL(client_name, client_name_len)
    Z_PARAM_STRING_OR_NULL(client_az, client_az_len)
    Z_PARAM_ARRAY_OR_NULL(advanced_config)
    Z_PARAM_ZVAL_OR_NULL(lazy_connect_zv)
    Z_PARAM_ZVAL_OR_NULL(context)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide_monitor_object* obj = VALKEY_GLIDE_MONITOR_ZVAL_GET_OBJECT(getThis());

    /* Build the shared connection configuration from the parsed parameters. */
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);

    common_params.use_tls     = use_tls;
    common_params.credentials = credentials;
    common_params.read_from   = read_from;

    if (request_timeout_zv != NULL && Z_TYPE_P(request_timeout_zv) != IS_NULL) {
        common_params.request_timeout         = Z_LVAL_P(request_timeout_zv);
        common_params.request_timeout_is_null = false;
    }
    common_params.reconnect_strategy = reconnect_strategy;
    if (database_id_zv != NULL && Z_TYPE_P(database_id_zv) != IS_NULL) {
        common_params.database_id         = Z_LVAL_P(database_id_zv);
        common_params.database_id_is_null = false;
    }
    common_params.client_name     = client_name;
    common_params.client_name_len = client_name_len;
    common_params.client_az       = client_az;
    common_params.client_az_len   = client_az_len;
    common_params.advanced_config = advanced_config;
    if (lazy_connect_zv != NULL && Z_TYPE_P(lazy_connect_zv) != IS_NULL) {
        common_params.lazy_connect         = Z_TYPE_P(lazy_connect_zv) == IS_TRUE;
        common_params.lazy_connect_is_null = false;
    }
    common_params.context = context;

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

    /* Open the dedicated MONITOR connection. */
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

        if (stop)
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
/* ValkeyGlideMonitorLine::__toString                                  */
/* ------------------------------------------------------------------ */

PHP_METHOD(ValkeyGlideMonitorLine, __toString) {
    ZEND_PARSE_PARAMETERS_NONE();

    zval* obj = getThis();
    zval  rv;

    zval* ts_z = zend_read_property(
        Z_OBJCE_P(obj), Z_OBJ_P(obj), "timestamp", sizeof("timestamp") - 1, 0, &rv);
    zval* db_z   = zend_read_property(Z_OBJCE_P(obj), Z_OBJ_P(obj), "db", sizeof("db") - 1, 0, &rv);
    zval* addr_z = zend_read_property(
        Z_OBJCE_P(obj), Z_OBJ_P(obj), "clientAddr", sizeof("clientAddr") - 1, 0, &rv);
    zval* cmd_z =
        zend_read_property(Z_OBJCE_P(obj), Z_OBJ_P(obj), "command", sizeof("command") - 1, 0, &rv);
    zval* args_z =
        zend_read_property(Z_OBJCE_P(obj), Z_OBJ_P(obj), "args", sizeof("args") - 1, 0, &rv);

    double      timestamp = ts_z ? zval_get_double(ts_z) : 0.0;
    zend_long   db        = db_z ? zval_get_long(db_z) : 0;
    const char* addr      = (addr_z && Z_TYPE_P(addr_z) == IS_STRING) ? Z_STRVAL_P(addr_z) : "";
    const char* command   = (cmd_z && Z_TYPE_P(cmd_z) == IS_STRING) ? Z_STRVAL_P(cmd_z) : "";

    smart_str out = {0};
    char      header[128];
    int       hn = snprintf(
        header, sizeof(header), "%.6f [%lld %s] \"%s\"", timestamp, (long long) db, addr, command);
    if (hn > 0)
        smart_str_appendl(
            &out, header, (size_t) hn < sizeof(header) ? (size_t) hn : sizeof(header) - 1);

    if (args_z && Z_TYPE_P(args_z) == IS_ARRAY) {
        zval* elem;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args_z), elem) {
            if (Z_TYPE_P(elem) != IS_STRING)
                continue;
            smart_str_appendl(&out, " \"", 2);
            /* Escape embedded quotes/backslashes to keep the line parseable. */
            const char* s   = Z_STRVAL_P(elem);
            size_t      len = Z_STRLEN_P(elem);
            for (size_t i = 0; i < len; i++) {
                char c = s[i];
                if (c == '"' || c == '\\') {
                    smart_str_appendc(&out, '\\');
                }
                smart_str_appendc(&out, c);
            }
            smart_str_appendc(&out, '"');
        }
        ZEND_HASH_FOREACH_END();
    }

    smart_str_0(&out);
    if (out.s) {
        RETVAL_STR(out.s);
    } else {
        RETVAL_EMPTY_STRING();
    }
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void register_valkey_glide_monitor_classes(void) {
    valkey_glide_monitor_line_ce = register_class_ValkeyGlideMonitorLine();

    valkey_glide_monitor_ce                = register_class_ValkeyGlideMonitor();
    valkey_glide_monitor_ce->create_object = create_valkey_glide_monitor_object;

    memcpy(&valkey_glide_monitor_object_handlers,
           zend_get_std_object_handlers(),
           sizeof(valkey_glide_monitor_object_handlers));
    valkey_glide_monitor_object_handlers.offset   = XtOffsetOf(valkey_glide_monitor_object, std);
    valkey_glide_monitor_object_handlers.free_obj = free_valkey_glide_monitor_object;
}
