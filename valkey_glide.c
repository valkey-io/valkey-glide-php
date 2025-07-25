/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "cluster_scan_cursor.h"          // Include ClusterScanCursor class
#include "cluster_scan_cursor_arginfo.h"  // Include ClusterScanCursor arginfo header
#include "common.h"
#include "php_valkey_glide.h"
#include "valkey_glide_arginfo.h"          // Include generated arginfo header
#include "valkey_glide_cluster_arginfo.h"  // Include generated arginfo header
#include "valkey_glide_commands_common.h"

/* Enum support includes - must be BEFORE arginfo includes */
#if PHP_VERSION_ID >= 80100
#include "zend_API.h"
#include "zend_compile.h"
#include "zend_enum.h"
#include "zend_object_handlers.h"
#include "zend_objects.h"
#endif
#include <zend_exceptions.h>

#include <ext/spl/spl_exceptions.h>
#include <ext/standard/info.h>

/* Include configuration parsing */
extern int  parse_valkey_glide_client_configuration(zval*                                config_obj,
                                                    valkey_glide_client_configuration_t* config);
extern void free_valkey_glide_client_configuration(valkey_glide_client_configuration_t* config);

zend_class_entry* valkey_glide_ce;
zend_class_entry* valkey_glide_exception_ce;

zend_class_entry* valkey_glide_cluster_ce;

/* Handlers for ValkeyGlideCluster */
zend_object_handlers valkey_glide_cluster_object_handlers;
zend_object_handlers valkey_glide_object_handlers;

zend_class_entry* get_valkey_glide_ce(void) {
    return valkey_glide_ce;
}

zend_class_entry* get_valkey_glide_exception_ce(void) {
    return valkey_glide_exception_ce;
}

zend_class_entry* get_valkey_glide_cluster_ce(void) {
    return valkey_glide_cluster_ce;
}
void free_valkey_glide_object(zend_object* object);
void free_valkey_glide_cluster_object(zend_object* object);
PHP_METHOD(ValkeyGlide, __construct);
PHP_METHOD(ValkeyGlideCluster, __construct);

// Use the generated method table from arginfo header
// const zend_function_entry valkey_glide_methods[] = {
//     PHP_ME(ValkeyGlide, __construct, arginfo_class_ValkeyGlide___construct, ZEND_ACC_PUBLIC |
//     ZEND_ACC_CTOR)
//         PHP_FE_END};
zend_object* create_valkey_glide_object(zend_class_entry* ce) {
    valkey_glide_object* valkey_glide =
        ecalloc(1, sizeof(valkey_glide_object) + zend_object_properties_size(ce));

    zend_object_std_init(&valkey_glide->std, ce);
    object_properties_init(&valkey_glide->std, ce);

    memcpy(&valkey_glide_object_handlers,
           zend_get_std_object_handlers(),
           sizeof(valkey_glide_object_handlers));
    valkey_glide_object_handlers.offset   = XtOffsetOf(valkey_glide_object, std);
    valkey_glide_object_handlers.free_obj = free_valkey_glide_object;
    valkey_glide->std.handlers            = &valkey_glide_object_handlers;

    return &valkey_glide->std;
}

zend_object* create_valkey_glide_cluster_object(zend_class_entry* ce)  // TODO can b remoed
{
    valkey_glide_object* valkey_glide =
        ecalloc(1, sizeof(valkey_glide_object) + zend_object_properties_size(ce));

    zend_object_std_init(&valkey_glide->std, ce);
    object_properties_init(&valkey_glide->std, ce);

    memcpy(&valkey_glide_cluster_object_handlers,
           zend_get_std_object_handlers(),
           sizeof(valkey_glide_cluster_object_handlers));
    valkey_glide_cluster_object_handlers.offset   = XtOffsetOf(valkey_glide_object, std);
    valkey_glide_cluster_object_handlers.free_obj = free_valkey_glide_object;
    valkey_glide->std.handlers                    = &valkey_glide_cluster_object_handlers;

    return &valkey_glide->std;
}

const zend_function_entry valkey_glide_cluster_methods[] = {
    PHP_ME(ValkeyGlideCluster,
           __construct,
           arginfo_class_ValkeyGlideCluster___construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) PHP_FE_END};

/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(valkey_glide) {
    /* ValkeyGlide class - use generated registration function */
    valkey_glide_ce = register_class_ValkeyGlide();

    /* ValkeyGlideCluster class - manual registration for now */

    valkey_glide_cluster_ce = register_class_ValkeyGlideCluster();

    /* Register ClusterScanCursor class */
    register_cluster_scan_cursor_class();

    /* ValkeyGlideException class */
    // TODO   valkey_glide_exception_ce =
    // register_class_ValkeyGlideException(spl_ce_RuntimeException);
    valkey_glide_ce->create_object         = create_valkey_glide_object;
    valkey_glide_cluster_ce->create_object = create_valkey_glide_cluster_object;

    return SUCCESS;
}

zend_module_entry valkey_glide_module_entry = {STANDARD_MODULE_HEADER,
                                               "valkey_glide",
                                               NULL,
                                               PHP_MINIT(valkey_glide),
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               PHP_VALKEY_GLIDE_VERSION,
                                               STANDARD_MODULE_PROPERTIES};

#ifdef COMPILE_DL_VALKEY_GLIDE
ZEND_GET_MODULE(valkey_glide)
#endif

void free_valkey_glide_object(zend_object* object) {
    valkey_glide_object* valkey_glide = VALKEY_GLIDE_PHP_GET_OBJECT(valkey_glide_object, object);

    /* Free the Valkey Glide client if it exists */
    if (valkey_glide->glide_client) {
        close_glide_client(valkey_glide->glide_client);
        valkey_glide->glide_client = NULL;
    }

    /* Clean up the standard object */
    zend_object_std_dtor(&valkey_glide->std);
}

/**
 * Helper function to clean up client configuration structures
 */
static void cleanup_client_config(valkey_glide_client_configuration_t* config) {
    if (config->base.addresses) {
        efree(config->base.addresses);
        config->base.addresses = NULL;
    }

    if (config->base.credentials) {
        efree(config->base.credentials);
        config->base.credentials = NULL;
    }

    if (config->base.reconnect_strategy) {
        efree(config->base.reconnect_strategy);
        config->base.reconnect_strategy = NULL;
    }

    if (config->base.advanced_config) {
        efree(config->base.advanced_config);
        config->base.advanced_config = NULL;
    }
}

/**
 * PHP_MINFO_FUNCTION

PHP_MINFO_FUNCTION(redis)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Valkey Glide Support", "enabled");
    php_info_print_table_row(2, "Valkey Glide Version", VALKEY_GLIDE_PHP_VERSION);
    php_info_print_table_end();
} TODO
*/
/* {{{ proto ValkeyGlide ValkeyGlide::__construct(array $addresses, bool $use_tls, ?array
   $credentials, ValkeyGlideReadFrom $read_from, ?int $request_timeout, ?array $reconnect_strategy,
   ?int $database_id, ?string $client_name, ?int $inflight_requests_limit, ?string $client_az,
   ?array $advanced_config, ?bool $lazy_connect) Public constructor */
PHP_METHOD(ValkeyGlide, __construct) {
    zval*                addresses                       = NULL;
    zend_bool            use_tls                         = 0;
    zval*                credentials                     = NULL;
    zend_long            read_from                       = 0; /* PRIMARY by default */
    zend_long            request_timeout                 = 0;
    zend_bool            request_timeout_is_null         = 1;
    zval*                reconnect_strategy              = NULL;
    zend_long            database_id                     = 0;
    zend_bool            database_id_is_null             = 1;
    char*                client_name                     = NULL;
    size_t               client_name_len                 = 0;
    zend_long            inflight_requests_limit         = 0;
    zend_bool            inflight_requests_limit_is_null = 1;
    char*                client_az                       = NULL;
    size_t               client_az_len                   = 0;
    zval*                advanced_config                 = NULL;
    zend_bool            lazy_connect                    = 0;
    zend_bool            lazy_connect_is_null            = 1;
    valkey_glide_object* valkey_glide;

    ZEND_PARSE_PARAMETERS_START(1, 12)
    Z_PARAM_ARRAY(addresses)
    Z_PARAM_OPTIONAL
    Z_PARAM_BOOL(use_tls)
    Z_PARAM_ARRAY_OR_NULL(credentials)
    Z_PARAM_LONG(read_from)
    Z_PARAM_LONG_OR_NULL(request_timeout, request_timeout_is_null)
    Z_PARAM_ARRAY_OR_NULL(reconnect_strategy)
    Z_PARAM_LONG_OR_NULL(database_id, database_id_is_null)
    Z_PARAM_STRING_OR_NULL(client_name, client_name_len)
    Z_PARAM_LONG_OR_NULL(inflight_requests_limit, inflight_requests_limit_is_null)
    Z_PARAM_STRING_OR_NULL(client_az, client_az_len)
    Z_PARAM_ARRAY_OR_NULL(advanced_config)
    Z_PARAM_BOOL_OR_NULL(lazy_connect, lazy_connect_is_null)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    /* Validate addresses array */
    if (!addresses || zend_hash_num_elements(Z_ARRVAL_P(addresses)) == 0) {
        // TODO zend_throw_exception(valkey_glide_exception_ce, "Addresses array cannot be empty",
        // 0);
        return;
    }

    /* Build client configuration from individual parameters */
    valkey_glide_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));

    /* Basic configuration */
    client_config.base.use_tls = use_tls;
    client_config.database_id  = database_id_is_null ? -1 : database_id; /* -1 means not set */
    client_config.base.request_timeout =
        request_timeout_is_null ? -1 : request_timeout; /* -1 means not set */
    client_config.base.client_name = client_name ? client_name : NULL;

    /* Set inflight requests limit */
    client_config.base.inflight_requests_limit =
        inflight_requests_limit_is_null ? -1 : inflight_requests_limit; /* -1 means not set */

    /* Set client availability zone */
    client_config.base.client_az = (client_az && client_az_len > 0) ? client_az : NULL;

    /* Set lazy connect option */
    client_config.base.lazy_connect = lazy_connect_is_null ? false : lazy_connect;

    /* Map read_from enum value to client's ReadFrom enum */
    switch (read_from) {
        case 1: /* PREFER_REPLICA */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_PREFER_REPLICA;
            break;
        case 2: /* AZ_AFFINITY */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY;
            break;
        case 3: /* AZ_AFFINITY_REPLICAS_AND_PRIMARY */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY;
            break;
        case 0: /* PRIMARY */
        default:
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_PRIMARY;
            break;
    }

    /* Process addresses array - handle multiple addresses */
    HashTable* addresses_ht  = Z_ARRVAL_P(addresses);
    zend_ulong num_addresses = zend_hash_num_elements(addresses_ht);

    if (num_addresses > 0) {
        /* Allocate addresses array */
        client_config.base.addresses = ecalloc(num_addresses, sizeof(valkey_glide_node_address_t));
        client_config.base.addresses_count = num_addresses;

        /* Process each address */
        zend_ulong i = 0;
        zval*      addr_val;
        ZEND_HASH_FOREACH_VAL(addresses_ht, addr_val) {
            if (Z_TYPE_P(addr_val) == IS_ARRAY) {
                HashTable* addr_ht = Z_ARRVAL_P(addr_val);

                /* Extract host */
                zval* host_val = zend_hash_str_find(addr_ht, "host", 4);
                if (host_val && Z_TYPE_P(host_val) == IS_STRING) {
                    client_config.base.addresses[i].host = Z_STRVAL_P(host_val);
                } else {
                    client_config.base.addresses[i].host = "localhost";
                }

                /* Extract port */
                zval* port_val = zend_hash_str_find(addr_ht, "port", 4);
                if (port_val && Z_TYPE_P(port_val) == IS_LONG) {
                    client_config.base.addresses[i].port = Z_LVAL_P(port_val);
                } else {
                    client_config.base.addresses[i].port = 6379;
                }

                i++;
            } else {
                /* Invalid address format */
                efree(client_config.base.addresses);
                // TODO zend_throw_exception(valkey_glide_exception_ce, "Invalid address format.
                // Expected array with 'host' and 'port' keys", 0);
                return;
            }
        }
        ZEND_HASH_FOREACH_END();
    } else {
        /* No addresses provided - set default */
        client_config.base.addresses         = ecalloc(1, sizeof(valkey_glide_node_address_t));
        client_config.base.addresses_count   = 1;
        client_config.base.addresses[0].host = "localhost";
        client_config.base.addresses[0].port = 6379;
    }

    /* Process credentials if provided */
    if (credentials && Z_TYPE_P(credentials) == IS_ARRAY) {
        HashTable* cred_ht = Z_ARRVAL_P(credentials);

        /* Allocate credentials structure */
        client_config.base.credentials = ecalloc(1, sizeof(valkey_glide_server_credentials_t));

        /* Check for username */
        zval* username_val = zend_hash_str_find(cred_ht, "username", 8);
        if (username_val && Z_TYPE_P(username_val) == IS_STRING) {
            client_config.base.credentials->username = Z_STRVAL_P(username_val);
        } else {
            client_config.base.credentials->username = NULL;
        }

        /* Check for password */
        zval* password_val = zend_hash_str_find(cred_ht, "password", 8);
        if (password_val && Z_TYPE_P(password_val) == IS_STRING) {
            client_config.base.credentials->password = Z_STRVAL_P(password_val);
        } else {
            client_config.base.credentials->password = NULL;
        }
    } else {
        client_config.base.credentials = NULL;
    }

    /* Process reconnect strategy if provided */
    if (reconnect_strategy && Z_TYPE_P(reconnect_strategy) == IS_ARRAY) {
        HashTable* reconnect_ht = Z_ARRVAL_P(reconnect_strategy);

        /* Allocate reconnect strategy structure */
        client_config.base.reconnect_strategy = ecalloc(1, sizeof(valkey_glide_backoff_strategy_t));

        /* Check for num_of_retries */
        zval* retries_val = zend_hash_str_find(reconnect_ht, "num_of_retries", 14);
        if (retries_val && Z_TYPE_P(retries_val) == IS_LONG) {
            client_config.base.reconnect_strategy->num_of_retries = Z_LVAL_P(retries_val);
        } else {
            client_config.base.reconnect_strategy->num_of_retries = 3; /* Default */
        }

        /* Check for factor */
        zval* factor_val = zend_hash_str_find(reconnect_ht, "factor", 6);
        if (factor_val && (Z_TYPE_P(factor_val) == IS_DOUBLE || Z_TYPE_P(factor_val) == IS_LONG)) {
            client_config.base.reconnect_strategy->factor = Z_TYPE_P(factor_val) == IS_DOUBLE
                                                                ? Z_DVAL_P(factor_val)
                                                                : (double) Z_LVAL_P(factor_val);
        } else {
            client_config.base.reconnect_strategy->factor = 2.0; /* Default */
        }

        /* Check for exponent_base */
        zval* exponent_val = zend_hash_str_find(reconnect_ht, "exponent_base", 13);
        if (exponent_val &&
            (Z_TYPE_P(exponent_val) == IS_DOUBLE || Z_TYPE_P(exponent_val) == IS_LONG)) {
            client_config.base.reconnect_strategy->exponent_base =
                Z_TYPE_P(exponent_val) == IS_DOUBLE ? Z_DVAL_P(exponent_val)
                                                    : (double) Z_LVAL_P(exponent_val);
        } else {
            client_config.base.reconnect_strategy->exponent_base = 2; /* Default */
        }

        /* Check for jitter_percent - optional */
        zval* jitter_val = zend_hash_str_find(reconnect_ht, "jitter_percent", 14);
        if (jitter_val && Z_TYPE_P(jitter_val) == IS_LONG) {
            client_config.base.reconnect_strategy->jitter_percent = Z_LVAL_P(jitter_val);
        } else {
            client_config.base.reconnect_strategy->jitter_percent = -1; /* Not set */
        }
    } else {
        client_config.base.reconnect_strategy = NULL;
    }

    /* Process advanced config if provided */
    if (advanced_config && Z_TYPE_P(advanced_config) == IS_ARRAY) {
        HashTable* advanced_ht = Z_ARRVAL_P(advanced_config);

        /* Allocate advanced config structure */
        client_config.base.advanced_config =
            ecalloc(1, sizeof(valkey_glide_advanced_base_client_configuration_t));

        /* Check for connection_timeout */
        zval* conn_timeout_val = zend_hash_str_find(advanced_ht, "connection_timeout", 18);
        if (conn_timeout_val && Z_TYPE_P(conn_timeout_val) == IS_LONG) {
            client_config.base.advanced_config->connection_timeout = Z_LVAL_P(conn_timeout_val);
        } else {
            client_config.base.advanced_config->connection_timeout = -1; /* Not set */
        }

        /* Check for TLS config - for now just set to NULL */
        client_config.base.advanced_config->tls_config = NULL;
    } else {
        client_config.base.advanced_config = NULL;
    }

    /* Validate database_id range */
    if (client_config.database_id != -1 &&
        (client_config.database_id < 0 || client_config.database_id > 15)) {
        // TODO zend_throw_exception(valkey_glide_exception_ce, "Database ID must be between 0 and
        // 15", 0);
        cleanup_client_config(&client_config);
        return;
    }

    const ConnectionResponse* conn_resp =
        create_glide_client((valkey_glide_client_configuration_t*) &client_config, false);

    if (conn_resp->connection_error_message) {
        zend_throw_exception(valkey_glide_exception_ce, conn_resp->connection_error_message,0);
    } else {
        valkey_glide->glide_client = conn_resp->conn_ptr;
    }

    free_connection_response((ConnectionResponse*) conn_resp);

    /* Clean up temporary configuration structures */
    cleanup_client_config(&client_config);
}
/* }}} */

/* {{{ proto ValkeyGlide ValkeyGlide::__destruct()
    Public Destructor
 */
PHP_METHOD(ValkeyGlide, __destruct) {
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto boolean ValkeyGlide::close()
 */
PHP_METHOD(ValkeyGlide, close) {
    /* TODO: Implement ValkeyGlide close */
    RETURN_TRUE;
}
/* }}} */

/* Basic method stubs - these need to be implemented with ValkeyGlide */
PHP_METHOD(ValkeyGlide, pipeline) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, publish) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, psubscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, ssubscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, subscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, unsubscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, punsubscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, sunsubscribe) { /* TODO: Implement */
}

PHP_METHOD(ValkeyGlide, pubsub) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, eval) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, eval_ro) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, evalsha) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, evalsha_ro) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, script) { /* TODO: Implement */
}
