/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "cluster_scan_cursor.h"          // Include ClusterScanCursor class
#include "cluster_scan_cursor_arginfo.h"  // Include ClusterScanCursor arginfo header
#include "common.h"
#include "logger.h"          // Include logger functionality
#include "logger_arginfo.h"  // Include logger functions arginfo - MUST BE LAST for ext_functions
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

void register_mock_constructor_class(void);

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

void valkey_glide_init_common_constructor_params(
    valkey_glide_php_common_constructor_params_t* params) {
    params->addresses               = NULL;
    params->use_tls                 = 0;
    params->credentials             = NULL;
    params->read_from               = 0; /* PRIMARY by default */
    params->request_timeout         = 0;
    params->request_timeout_is_null = 1;
    params->reconnect_strategy      = NULL;
    params->client_name             = NULL;
    params->client_name_len         = 0;
    params->client_az               = NULL;
    params->client_az_len           = 0;
    params->advanced_config         = NULL;
    params->lazy_connect            = 0;
    params->lazy_connect_is_null    = 1;
}

void valkey_glide_build_client_config_base(valkey_glide_php_common_constructor_params_t* params,
                                           valkey_glide_base_client_configuration_t*     config,
                                           bool is_cluster) {
    /* Basic configuration */
    config->use_tls = params->use_tls;
    config->request_timeout =
        params->request_timeout_is_null ? -1 : params->request_timeout; /* -1 means not set */
    config->client_name = params->client_name ? params->client_name : NULL;

    /* Set inflight requests limit to -1 (unset). A synchronous API does not need a request limit
       since it is effectively one-request-at-a-time. */
    config->inflight_requests_limit = -1;

    /* Set client availability zone */
    config->client_az = (params->client_az && params->client_az_len > 0) ? params->client_az : NULL;

    /* Set lazy connect option */
    config->lazy_connect = params->lazy_connect_is_null ? false : params->lazy_connect;

    /* Map read_from enum value to client's ReadFrom enum */
    switch (params->read_from) {
        case 1: /* PREFER_REPLICA */
            config->read_from = VALKEY_GLIDE_READ_FROM_PREFER_REPLICA;
            break;
        case 2: /* AZ_AFFINITY */
            config->read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY;
            break;
        case 3: /* AZ_AFFINITY_REPLICAS_AND_PRIMARY */
            config->read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY;
            break;
        case 0: /* PRIMARY */
        default:
            config->read_from = VALKEY_GLIDE_READ_FROM_PRIMARY;
            break;
    }

    /* Process addresses array - handle multiple addresses */
    HashTable* addresses_ht  = Z_ARRVAL_P(params->addresses);
    zend_ulong num_addresses = zend_hash_num_elements(addresses_ht);

    int default_port = is_cluster ? 7001 : 6379;
    if (num_addresses > 0) {
        /* Allocate addresses array */
        config->addresses       = ecalloc(num_addresses, sizeof(valkey_glide_node_address_t));
        config->addresses_count = num_addresses;

        /* Process each address */
        zend_ulong i = 0;
        zval*      addr_val;
        ZEND_HASH_FOREACH_VAL(addresses_ht, addr_val) {
            if (Z_TYPE_P(addr_val) == IS_ARRAY) {
                HashTable* addr_ht = Z_ARRVAL_P(addr_val);

                /* Extract host */
                zval* host_val = zend_hash_str_find(addr_ht, "host", 4);
                if (host_val && Z_TYPE_P(host_val) == IS_STRING) {
                    config->addresses[i].host = Z_STRVAL_P(host_val);
                } else {
                    config->addresses[i].host = "localhost";
                }

                /* Extract port */
                zval* port_val = zend_hash_str_find(addr_ht, "port", 4);
                if (port_val && Z_TYPE_P(port_val) == IS_LONG) {
                    config->addresses[i].port = Z_LVAL_P(port_val);
                } else {
                    config->addresses[i].port = default_port;
                }

                i++;
            } else {
                /* Invalid address format */
                const char* error_message =
                    "Invalid address format. Expected array with 'host' and 'port' keys.";
                zend_throw_exception(valkey_glide_exception_ce, error_message, 0);
                valkey_glide_cleanup_client_config(config);
                return;
            }
        }
        ZEND_HASH_FOREACH_END();
    } else {
        /* No addresses provided - set default */
        config->addresses         = ecalloc(1, sizeof(valkey_glide_node_address_t));
        config->addresses_count   = 1;
        config->addresses[0].host = "localhost";
        config->addresses[0].port = default_port;
    }

    /* Process credentials if provided */
    if (params->credentials && Z_TYPE_P(params->credentials) == IS_ARRAY) {
        HashTable* cred_ht = Z_ARRVAL_P(params->credentials);

        /* Allocate credentials structure */
        config->credentials = ecalloc(1, sizeof(valkey_glide_server_credentials_t));

        /* Check for username */
        zval* username_val = zend_hash_str_find(cred_ht, "username", 8);
        if (username_val && Z_TYPE_P(username_val) == IS_STRING) {
            config->credentials->username = Z_STRVAL_P(username_val);
        } else {
            config->credentials->username = NULL;
        }

        /* Check for password */
        zval* password_val = zend_hash_str_find(cred_ht, "password", 8);
        if (password_val && Z_TYPE_P(password_val) == IS_STRING) {
            config->credentials->password = Z_STRVAL_P(password_val);
        } else {
            config->credentials->password = NULL;
        }
    } else {
        config->credentials = NULL;
    }

    /* Process reconnect strategy if provided */
    if (params->reconnect_strategy && Z_TYPE_P(params->reconnect_strategy) == IS_ARRAY) {
        HashTable* reconnect_ht = Z_ARRVAL_P(params->reconnect_strategy);

        /* Allocate reconnect strategy structure */
        config->reconnect_strategy = ecalloc(1, sizeof(valkey_glide_backoff_strategy_t));

        /* Check for num_of_retries */
        zval* retries_val = zend_hash_str_find(reconnect_ht, "num_of_retries", 14);
        if (retries_val && Z_TYPE_P(retries_val) == IS_LONG) {
            config->reconnect_strategy->num_of_retries = Z_LVAL_P(retries_val);
        } else {
            config->reconnect_strategy->num_of_retries = VALKEY_GLIDE_DEFAULT_NUM_OF_RETRIES;
        }

        /* Check for factor */
        zval* factor_val = zend_hash_str_find(reconnect_ht, "factor", 6);
        if (factor_val && Z_TYPE_P(factor_val) == IS_LONG) {
            config->reconnect_strategy->factor = Z_LVAL_P(factor_val);
        } else {
            config->reconnect_strategy->factor = VALKEY_GLIDE_DEFAULT_FACTOR;
        }

        /* Check for exponent_base */
        zval* exponent_val = zend_hash_str_find(reconnect_ht, "exponent_base", 13);
        if (exponent_val && Z_TYPE_P(exponent_val) == IS_LONG) {
            config->reconnect_strategy->exponent_base = Z_LVAL_P(exponent_val);
        } else {
            config->reconnect_strategy->exponent_base = VALKEY_GLIDE_DEFAULT_EXPONENT_BASE;
        }

        /* Check for jitter_percent - optional */
        zval* jitter_val = zend_hash_str_find(reconnect_ht, "jitter_percent", 14);
        if (jitter_val && Z_TYPE_P(jitter_val) == IS_LONG) {
            config->reconnect_strategy->jitter_percent = Z_LVAL_P(jitter_val);
        } else {
            config->reconnect_strategy->jitter_percent = VALKEY_GLIDE_DEFAULT_JITTER_PERCENTAGE;
        }
    } else {
        config->reconnect_strategy = NULL;
    }

    /* Process advanced config if provided */
    if (params->advanced_config && Z_TYPE_P(params->advanced_config) == IS_ARRAY) {
        HashTable* advanced_ht = Z_ARRVAL_P(params->advanced_config);

        /* Allocate advanced config structure */
        config->advanced_config =
            ecalloc(1, sizeof(valkey_glide_advanced_base_client_configuration_t));

        /* Check for connection_timeout */
        zval* conn_timeout_val = zend_hash_str_find(advanced_ht, "connection_timeout", 18);
        if (conn_timeout_val && Z_TYPE_P(conn_timeout_val) == IS_LONG) {
            config->advanced_config->connection_timeout = Z_LVAL_P(conn_timeout_val);
        } else {
            config->advanced_config->connection_timeout = VALKEY_GLIDE_DEFAULT_CONNECTION_TIMEOUT;
        }

        /* Check for TLS config */
        zval* tls_config_val = zend_hash_str_find(advanced_ht, "tls_config", 10);
        if (tls_config_val && Z_TYPE_P(tls_config_val) == IS_ARRAY) {
            HashTable* tls_ht = Z_ARRVAL_P(tls_config_val);
            config->advanced_config->tls_config =
                ecalloc(1, sizeof(valkey_glide_tls_advanced_configuration_t));

            zval* use_insecure_tls_val = zend_hash_str_find(tls_ht, "use_insecure_tls", 16);
            if (use_insecure_tls_val && Z_TYPE_P(use_insecure_tls_val) == IS_TRUE) {
                config->advanced_config->tls_config->use_insecure_tls = true;
            } else {
                config->advanced_config->tls_config->use_insecure_tls = false;
            }
        } else {
            config->advanced_config->tls_config = NULL;
        }
    } else {
        config->advanced_config = NULL;
    }
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
    /* Initialize the logger system early to prevent crashes */
    int logger_result = valkey_glide_logger_init("warn", NULL);
    if (logger_result != 0) {
        /* Log initialization failed, but continue - logger will auto-init on first use */
        php_error_docref(
            NULL,
            E_WARNING,
            "Failed to initialize ValkeyGlide logger, will auto-initialize on first use");
    }
    valkey_glide_logger_debug("php_init", "Initializing Valkey Glide PHP extension");
    /* ValkeyGlide class - use generated registration function */
    valkey_glide_ce = register_class_ValkeyGlide();

    /* ValkeyGlideCluster class - manual registration for now */

    valkey_glide_cluster_ce = register_class_ValkeyGlideCluster();

    /* Register ClusterScanCursor class */
    register_cluster_scan_cursor_class();

    /* Register mock constructor class used for testing only. */
    register_mock_constructor_class();

    /* ValkeyGlideException class */
    valkey_glide_exception_ce = register_class_ValkeyGlideException(spl_ce_RuntimeException);
    if (!valkey_glide_exception_ce) {
        php_error_docref(NULL, E_ERROR, "Failed to register ValkeyGlideException class");
        return FAILURE;
    }

    /* Set object creation handlers */
    if (valkey_glide_ce) {
        valkey_glide_ce->create_object = create_valkey_glide_object;
    }
    if (valkey_glide_cluster_ce) {
        valkey_glide_cluster_ce->create_object = create_valkey_glide_cluster_object;
    }

    return SUCCESS;
}


zend_module_entry valkey_glide_module_entry = {STANDARD_MODULE_HEADER,
                                               "valkey_glide",
                                               ext_functions,
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
void valkey_glide_cleanup_client_config(valkey_glide_base_client_configuration_t* config) {
    if (config->addresses) {
        efree(config->addresses);
        config->addresses = NULL;
    }

    if (config->credentials) {
        efree(config->credentials);
        config->credentials = NULL;
    }

    if (config->reconnect_strategy) {
        efree(config->reconnect_strategy);
        config->reconnect_strategy = NULL;
    }

    if (config->advanced_config) {
        if (config->advanced_config->tls_config) {
            efree(config->advanced_config->tls_config);
            config->advanced_config->tls_config = NULL;
        }
        efree(config->advanced_config);
        config->advanced_config = NULL;
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
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);
    zend_long            database_id         = 0;
    zend_bool            database_id_is_null = 1;
    valkey_glide_object* valkey_glide;

    ZEND_PARSE_PARAMETERS_START(1, 11)
    Z_PARAM_ARRAY(common_params.addresses)
    Z_PARAM_OPTIONAL
    Z_PARAM_BOOL(common_params.use_tls)
    Z_PARAM_ARRAY_OR_NULL(common_params.credentials)
    Z_PARAM_LONG(common_params.read_from)
    Z_PARAM_LONG_OR_NULL(common_params.request_timeout, common_params.request_timeout_is_null)
    Z_PARAM_ARRAY_OR_NULL(common_params.reconnect_strategy)
    Z_PARAM_LONG_OR_NULL(database_id, database_id_is_null)
    Z_PARAM_STRING_OR_NULL(common_params.client_name, common_params.client_name_len)
    Z_PARAM_STRING_OR_NULL(common_params.client_az, common_params.client_az_len)
    Z_PARAM_ARRAY_OR_NULL(common_params.advanced_config)
    Z_PARAM_BOOL_OR_NULL(common_params.lazy_connect, common_params.lazy_connect_is_null)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    VALKEY_LOG_DEBUG("php_construct", "Starting ValkeyGlide construction");

    /* Validate addresses array */
    if (!common_params.addresses ||
        zend_hash_num_elements(Z_ARRVAL_P(common_params.addresses)) == 0) {
        const char* error_message = "Addresses array cannot be empty";
        zend_throw_exception(valkey_glide_exception_ce, error_message, 0);
        return;
    }

    /* Build client configuration from individual parameters */
    valkey_glide_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));
    client_config.database_id = database_id_is_null ? -1 : database_id; /* -1 means not set */

    /* Validate database_id range */
    if (client_config.database_id != -1 &&
        (client_config.database_id < 0 || client_config.database_id > 15)) {
        const char* error_message = "Database ID must be between 0 and 15 inclusive.";
        zend_throw_exception(valkey_glide_exception_ce, error_message, 0);
        valkey_glide_cleanup_client_config(&client_config.base);
        return;
    }

    /* Populate configuration parameters shared between client and cluster connections. */
    valkey_glide_build_client_config_base(&common_params, &client_config.base, false);

    /* Issue the connection request. */
    const ConnectionResponse* conn_resp = create_glide_client(&client_config);

    if (conn_resp->connection_error_message) {
        VALKEY_LOG_ERROR("php_construct", conn_resp->connection_error_message);
        zend_throw_exception(valkey_glide_exception_ce, conn_resp->connection_error_message, 0);
    } else {
        VALKEY_LOG_INFO("php_construct", "ValkeyGlide client created successfully");
        valkey_glide->glide_client = conn_resp->conn_ptr;
    }

    free_connection_response((ConnectionResponse*) conn_resp);

    /* Clean up temporary configuration structures */
    valkey_glide_cleanup_client_config(&client_config.base);
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

/* ============================================================================
 * Logger PHP Functions - Bridge between PHP stub and C implementation
 * ============================================================================ */

/**
 * PHP function: valkey_glide_logger_init(?string $level = null, ?string $filename = null): bool
 */
PHP_FUNCTION(valkey_glide_logger_init) {
    char*  level        = NULL;
    size_t level_len    = 0;
    char*  filename     = NULL;
    size_t filename_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 2)
    Z_PARAM_OPTIONAL
    Z_PARAM_STRING_OR_NULL(level, level_len)
    Z_PARAM_STRING_OR_NULL(filename, filename_len)
    ZEND_PARSE_PARAMETERS_END();

    int result = valkey_glide_logger_init(level, filename);
    RETURN_BOOL(result == 0);
}

/**
 * PHP function: valkey_glide_logger_set_config(string $level, ?string $filename = null): bool
 */
PHP_FUNCTION(valkey_glide_logger_set_config) {
    char*  level;
    size_t level_len;
    char*  filename     = NULL;
    size_t filename_len = 0;

    ZEND_PARSE_PARAMETERS_START(1, 2)
    Z_PARAM_STRING(level, level_len)
    Z_PARAM_OPTIONAL
    Z_PARAM_STRING_OR_NULL(filename, filename_len)
    ZEND_PARSE_PARAMETERS_END();

    int result = valkey_glide_logger_set_config(level, filename);
    RETURN_BOOL(result == 0);
}

/**
 * PHP function: valkey_glide_logger_log(string $level, string $identifier, string $message): void
 */
PHP_FUNCTION(valkey_glide_logger_log) {
    char * level, *identifier, *message;
    size_t level_len, identifier_len, message_len;

    ZEND_PARSE_PARAMETERS_START(3, 3)
    Z_PARAM_STRING(level, level_len)
    Z_PARAM_STRING(identifier, identifier_len)
    Z_PARAM_STRING(message, message_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_logger_log(level, identifier, message);
}

/**
 * PHP function: valkey_glide_logger_error(string $identifier, string $message): void
 */
PHP_FUNCTION(valkey_glide_logger_error) {
    char * identifier, *message;
    size_t identifier_len, message_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STRING(identifier, identifier_len)
    Z_PARAM_STRING(message, message_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_logger_error(identifier, message);
}

/**
 * PHP function: valkey_glide_logger_warn(string $identifier, string $message): void
 */
PHP_FUNCTION(valkey_glide_logger_warn) {
    char * identifier, *message;
    size_t identifier_len, message_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STRING(identifier, identifier_len)
    Z_PARAM_STRING(message, message_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_logger_warn(identifier, message);
}

/**
 * PHP function: valkey_glide_logger_info(string $identifier, string $message): void
 */
PHP_FUNCTION(valkey_glide_logger_info) {
    char * identifier, *message;
    size_t identifier_len, message_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STRING(identifier, identifier_len)
    Z_PARAM_STRING(message, message_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_logger_info(identifier, message);
}

/**
 * PHP function: valkey_glide_logger_debug(string $identifier, string $message): void
 */
PHP_FUNCTION(valkey_glide_logger_debug) {
    char * identifier, *message;
    size_t identifier_len, message_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
    Z_PARAM_STRING(identifier, identifier_len)
    Z_PARAM_STRING(message, message_len)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_logger_debug(identifier, message);
}

/**
 * PHP function: valkey_glide_logger_is_initialized(): bool
 */
PHP_FUNCTION(valkey_glide_logger_is_initialized) {
    ZEND_PARSE_PARAMETERS_START(0, 0)
    ZEND_PARSE_PARAMETERS_END();

    RETURN_BOOL(valkey_glide_logger_is_initialized());
}

/**
 * PHP function: valkey_glide_logger_get_level(): int
 */
PHP_FUNCTION(valkey_glide_logger_get_level) {
    ZEND_PARSE_PARAMETERS_START(0, 0)
    ZEND_PARSE_PARAMETERS_END();

    RETURN_LONG(valkey_glide_logger_get_level());
}
