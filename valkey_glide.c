/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "cluster_scan_cursor.h"          // Include ClusterScanCursor class
#include "cluster_scan_cursor_arginfo.h"  // Include ClusterScanCursor arginfo header
#include "common.h"
#include "include/glide_bindings.h"
#include "logger.h"          // Include logger functionality
#include "logger_arginfo.h"  // Include logger functions arginfo - MUST BE LAST for ext_functions
#include "valkey_glide_arginfo.h"          // Include generated arginfo header
#include "valkey_glide_cluster_arginfo.h"  // Include generated arginfo header
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_pubsub_common.h"
#include "valkey_glide_pubsub_introspection.h"

// FFI function declarations
extern struct CommandResult* command(const void*          client_adapter_ptr,
                                     uintptr_t            request_id,
                                     enum RequestType     command_type,
                                     unsigned long        arg_count,
                                     const uintptr_t*     args,
                                     const unsigned long* args_len,
                                     const uint8_t*       route_bytes,
                                     uintptr_t            route_bytes_len,
                                     uint64_t             span_ptr);

extern void free_command_result(struct CommandResult* command_result_ptr);

#include "valkey_glide_otel.h"  // Include OTEL support

/* Enum support includes - must be BEFORE arginfo includes */
#if PHP_VERSION_ID >= 80100
#include "zend_API.h"
#include "zend_compile.h"
#include "zend_enum.h"
#include "zend_object_handlers.h"
#include "zend_objects.h"
#endif

#include <main/php_globals.h>
#include <php_streams.h>
#include <stdbool.h>
#include <zend_exceptions.h>

#include <ext/spl/spl_exceptions.h>
#include <ext/standard/file.h>
#include <ext/standard/info.h>

/* Include configuration parsing */
extern int parse_valkey_glide_client_configuration(
    zval* config_obj, valkey_glide_base_client_configuration_t* config);
extern void free_valkey_glide_client_configuration(
    valkey_glide_base_client_configuration_t* config);

void register_mock_constructor_class(void);

/* Default values for addresses */
static const char* const DEFAULT_HOST            = "localhost";
static const int         DEFAULT_PORT_STANDALONE = 6379;
static const int         DEFAULT_PORT_CLUSTER    = 7001;

/* TLS/SSL prefix constants */
static const char* const TLS_PREFIX     = "tls://";
static const char* const SSL_PREFIX     = "ssl://";
static const size_t      TLS_PREFIX_LEN = 6;
static const size_t      SSL_PREFIX_LEN = 6;

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

// Forward declarations for helper functions
static HashTable* _get_stream_context_ssl_options_ht(
    valkey_glide_php_common_constructor_params_t* params);
static HashTable* _get_advanced_config_ht(valkey_glide_php_common_constructor_params_t* params);
static HashTable* _get_advanced_tls_config_ht(valkey_glide_php_common_constructor_params_t* params);

static int  _determine_connection_timeout(valkey_glide_php_common_constructor_params_t* params);
static bool _determine_use_insecure_tls(valkey_glide_php_common_constructor_params_t* params);
static bool _determine_use_tls(valkey_glide_php_common_constructor_params_t* params);

static int _load_certificate_file(const char* path, uint8_t** data, size_t* length);

static valkey_glide_tls_advanced_configuration_t* _build_advanced_tls_config(
    valkey_glide_php_common_constructor_params_t* params, bool is_cluster);
static valkey_glide_advanced_base_client_configuration_t* _build_advanced_config(
    valkey_glide_php_common_constructor_params_t* params, bool is_cluster);

static void _initialize_open_telemetry(valkey_glide_php_common_constructor_params_t* params,
                                       bool                                          is_cluster);
static bool _load_data_from_file(const char* path, uint8_t** data, size_t* length);

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
    params->database_id             = 0;
    params->database_id_is_null     = 1;
    params->context                 = NULL;
}

void valkey_glide_build_client_config_base(valkey_glide_php_common_constructor_params_t* params,
                                           valkey_glide_base_client_configuration_t*     config,
                                           bool is_cluster) {
    /* Basic configuration */
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

    /* Set database_id */
    config->database_id =
        params->database_id_is_null ? -1 : params->database_id; /* -1 means not set */

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

    /* Process addresses array - handle multiple addresses
     * Address array is non-empty - see ValkeyGlide::__construct */
    HashTable* addresses_ht  = Z_ARRVAL_P(params->addresses);
    zend_ulong num_addresses = zend_hash_num_elements(addresses_ht);

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
            config->addresses[i].host = (char*) DEFAULT_HOST;

            zval* host_val = zend_hash_str_find(
                addr_ht, VALKEY_GLIDE_ADDRESS_HOST, sizeof(VALKEY_GLIDE_ADDRESS_HOST) - 1);
            if (host_val && Z_TYPE_P(host_val) == IS_STRING) {
                const char* host = Z_STRVAL_P(host_val);

                /* Strip TLS/SSL prefix if present */
                if (strncmp(host, TLS_PREFIX, TLS_PREFIX_LEN) == 0) {
                    host += TLS_PREFIX_LEN;
                } else if (strncmp(host, SSL_PREFIX, SSL_PREFIX_LEN) == 0) {
                    host += SSL_PREFIX_LEN;
                }

                config->addresses[i].host = (char*) host;
            }

            /* Extract port */
            config->addresses[i].port = is_cluster ? DEFAULT_PORT_CLUSTER : DEFAULT_PORT_STANDALONE;

            zval* port_val = zend_hash_str_find(
                addr_ht, VALKEY_GLIDE_ADDRESS_PORT, sizeof(VALKEY_GLIDE_ADDRESS_PORT) - 1);
            if (port_val && Z_TYPE_P(port_val) == IS_LONG) {
                config->addresses[i].port = Z_LVAL_P(port_val);
            }

            i++;
        } else {
            /* Invalid address format */
            const char* error_message =
                "Invalid address format. Expected array with 'host' and 'port' keys.";
            VALKEY_LOG_ERROR("php_construct", error_message);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
            valkey_glide_cleanup_client_config(config);
            return;
        }
    }
    ZEND_HASH_FOREACH_END();

    /* Process credentials if provided */
    if (params->credentials && Z_TYPE_P(params->credentials) == IS_ARRAY) {
        HashTable* cred_ht = Z_ARRVAL_P(params->credentials);

        /* Allocate credentials structure */
        config->credentials             = ecalloc(1, sizeof(valkey_glide_server_credentials_t));
        config->credentials->iam_config = NULL;

        /* Check for username */
        zval* username_val = zend_hash_str_find(
            cred_ht, VALKEY_GLIDE_AUTH_USERNAME, strlen(VALKEY_GLIDE_AUTH_USERNAME));
        if (username_val && Z_TYPE_P(username_val) == IS_STRING) {
            config->credentials->username = Z_STRVAL_P(username_val);
        } else {
            config->credentials->username = NULL;
        }

        /* Check for password */
        zval* password_val = zend_hash_str_find(
            cred_ht, VALKEY_GLIDE_AUTH_PASSWORD, strlen(VALKEY_GLIDE_AUTH_PASSWORD));
        if (password_val && Z_TYPE_P(password_val) == IS_STRING) {
            config->credentials->password = Z_STRVAL_P(password_val);
        } else {
            config->credentials->password = NULL;
        }

        /* Check for IAM config (mutually exclusive with password) */
        zval* iam_config_val = zend_hash_str_find(
            cred_ht, VALKEY_GLIDE_AUTH_IAM_CONFIG, strlen(VALKEY_GLIDE_AUTH_IAM_CONFIG));
        if (iam_config_val && Z_TYPE_P(iam_config_val) == IS_ARRAY) {
            HashTable* iam_ht = Z_ARRVAL_P(iam_config_val);

            /* Allocate IAM config structure */
            config->credentials->iam_config = ecalloc(1, sizeof(valkey_glide_iam_config_t));

            /* Parse cluster_name (required) */
            zval* cluster_name_val =
                zend_hash_str_find(iam_ht,
                                   VALKEY_GLIDE_IAM_CONFIG_CLUSTER_NAME,
                                   strlen(VALKEY_GLIDE_IAM_CONFIG_CLUSTER_NAME));
            if (cluster_name_val && Z_TYPE_P(cluster_name_val) == IS_STRING) {
                config->credentials->iam_config->cluster_name = Z_STRVAL_P(cluster_name_val);
            } else {
                config->credentials->iam_config->cluster_name = NULL;
            }

            /* Parse region (required) */
            zval* region_val = zend_hash_str_find(
                iam_ht, VALKEY_GLIDE_IAM_CONFIG_REGION, strlen(VALKEY_GLIDE_IAM_CONFIG_REGION));
            if (region_val && Z_TYPE_P(region_val) == IS_STRING) {
                config->credentials->iam_config->region = Z_STRVAL_P(region_val);
            } else {
                config->credentials->iam_config->region = NULL;
            }

            /* Parse service type (required) */
            zval* service_val = zend_hash_str_find(
                iam_ht, VALKEY_GLIDE_IAM_CONFIG_SERVICE, strlen(VALKEY_GLIDE_IAM_CONFIG_SERVICE));
            if (service_val && Z_TYPE_P(service_val) == IS_STRING) {
                const char* service_str = Z_STRVAL_P(service_val);
                if (strcasecmp(service_str, VALKEY_GLIDE_IAM_SERVICE_MEMORYDB) == 0) {
                    config->credentials->iam_config->service_type =
                        VALKEY_GLIDE_SERVICE_TYPE_MEMORYDB;
                } else {
                    config->credentials->iam_config->service_type =
                        VALKEY_GLIDE_SERVICE_TYPE_ELASTICACHE;
                }
            } else {
                config->credentials->iam_config->service_type =
                    VALKEY_GLIDE_SERVICE_TYPE_ELASTICACHE;
            }

            /* Parse refresh interval (optional, defaults to 300 seconds) */
            zval* refresh_val =
                zend_hash_str_find(iam_ht,
                                   VALKEY_GLIDE_IAM_CONFIG_REFRESH_INTERVAL,
                                   strlen(VALKEY_GLIDE_IAM_CONFIG_REFRESH_INTERVAL));
            if (refresh_val && Z_TYPE_P(refresh_val) == IS_LONG) {
                config->credentials->iam_config->refresh_interval_seconds = Z_LVAL_P(refresh_val);
            } else {
                config->credentials->iam_config->refresh_interval_seconds =
                    0; /* 0 means use default */
            }

            /* Clear password when using IAM */
            config->credentials->password = NULL;

            /* Validate that username is provided for IAM */
            if (!config->credentials->username) {
                zend_throw_exception(
                    get_valkey_glide_exception_ce(), "IAM authentication requires a username", 0);
            }
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
        zval* retries_val = zend_hash_str_find(
            reconnect_ht, VALKEY_GLIDE_NUM_OF_RETRIES, strlen(VALKEY_GLIDE_NUM_OF_RETRIES));
        if (retries_val && Z_TYPE_P(retries_val) == IS_LONG) {
            config->reconnect_strategy->num_of_retries = Z_LVAL_P(retries_val);
        } else {
            config->reconnect_strategy->num_of_retries = VALKEY_GLIDE_DEFAULT_NUM_OF_RETRIES;
        }

        /* Check for factor */
        zval* factor_val =
            zend_hash_str_find(reconnect_ht, VALKEY_GLIDE_FACTOR, strlen(VALKEY_GLIDE_FACTOR));
        if (factor_val && Z_TYPE_P(factor_val) == IS_LONG) {
            config->reconnect_strategy->factor = Z_LVAL_P(factor_val);
        } else {
            config->reconnect_strategy->factor = VALKEY_GLIDE_DEFAULT_FACTOR;
        }

        /* Check for exponent_base */
        zval* exponent_val = zend_hash_str_find(
            reconnect_ht, VALKEY_GLIDE_EXPONENT_BASE, strlen(VALKEY_GLIDE_EXPONENT_BASE));
        if (exponent_val && Z_TYPE_P(exponent_val) == IS_LONG) {
            config->reconnect_strategy->exponent_base = Z_LVAL_P(exponent_val);
        } else {
            config->reconnect_strategy->exponent_base = VALKEY_GLIDE_DEFAULT_EXPONENT_BASE;
        }

        /* Check for jitter_percent - optional */
        zval* jitter_val = zend_hash_str_find(
            reconnect_ht, VALKEY_GLIDE_JITTER_PERCENT, strlen(VALKEY_GLIDE_JITTER_PERCENT));
        if (jitter_val && Z_TYPE_P(jitter_val) == IS_LONG) {
            config->reconnect_strategy->jitter_percent = Z_LVAL_P(jitter_val);
        } else {
            config->reconnect_strategy->jitter_percent = VALKEY_GLIDE_DEFAULT_JITTER_PERCENTAGE;
        }
    } else {
        config->reconnect_strategy = NULL;
    }

    config->use_tls         = _determine_use_tls(params);
    config->advanced_config = _build_advanced_config(params, is_cluster);

    // Raise an exception if insecure TLS is requested but TLS is not enabled
    if (!config->use_tls && config->advanced_config && config->advanced_config->tls_config &&
        config->advanced_config->tls_config->use_insecure_tls) {
        VALKEY_LOG_ERROR("insecure_tls_with_tls_disabled",
                         "Cannot configure insecure TLS when TLS is disabled.");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "Cannot configure insecure TLS when TLS is disabled.",
                             0);
    }

    _initialize_open_telemetry(params, is_cluster);
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

PHP_MSHUTDOWN_FUNCTION(valkey_glide) {
    valkey_glide_pubsub_shutdown();
    return SUCCESS;
}

zend_module_entry valkey_glide_module_entry = {STANDARD_MODULE_HEADER,
                                               "valkey_glide",
                                               ext_functions,
                                               PHP_MINIT(valkey_glide),
                                               PHP_MSHUTDOWN(valkey_glide),
                                               NULL,
                                               NULL,
                                               NULL,
                                               VALKEY_GLIDE_PHP_VERSION,
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
        if (config->credentials->iam_config) {
            efree(config->credentials->iam_config);
            config->credentials->iam_config = NULL;
        }
        efree(config->credentials);
        config->credentials = NULL;
    }

    if (config->reconnect_strategy) {
        efree(config->reconnect_strategy);
        config->reconnect_strategy = NULL;
    }

    if (config->advanced_config) {
        if (config->advanced_config->tls_config) {
            if (config->advanced_config->tls_config->root_certs) {
                efree(config->advanced_config->tls_config->root_certs);
                config->advanced_config->tls_config->root_certs = NULL;
            }
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
   ?array $advanced_config, ?bool $lazy_connect, ?array $context) Public constructor */
PHP_METHOD(ValkeyGlide, __construct) {
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);
    valkey_glide_object* valkey_glide;

    ZEND_PARSE_PARAMETERS_START(1, 12)
    Z_PARAM_ARRAY(common_params.addresses)
    Z_PARAM_OPTIONAL
    Z_PARAM_BOOL(common_params.use_tls)
    Z_PARAM_ARRAY_OR_NULL(common_params.credentials)
    Z_PARAM_LONG(common_params.read_from)
    Z_PARAM_LONG_OR_NULL(common_params.request_timeout, common_params.request_timeout_is_null)
    Z_PARAM_ARRAY_OR_NULL(common_params.reconnect_strategy)
    Z_PARAM_LONG_OR_NULL(common_params.database_id, common_params.database_id_is_null)
    Z_PARAM_STRING_OR_NULL(common_params.client_name, common_params.client_name_len)
    Z_PARAM_STRING_OR_NULL(common_params.client_az, common_params.client_az_len)
    Z_PARAM_ARRAY_OR_NULL(common_params.advanced_config)
    Z_PARAM_BOOL_OR_NULL(common_params.lazy_connect, common_params.lazy_connect_is_null)
    Z_PARAM_RESOURCE_EX(
        common_params.context, 1, 0) /* Use Z_PARAM_RESOURCE_OR_NULL with PHP 8.5+ */
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    VALKEY_LOG_DEBUG("php_construct", "Starting ValkeyGlide construction");

    /* Validate database_id range early */
    if (!common_params.database_id_is_null) {
        if (common_params.database_id < 0) {
            const char* error_message = "Database ID must be non-negative.";
            VALKEY_LOG_ERROR("php_construct", error_message);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
            return;
        }
    }

    /* Validate addresses array */
    if (!common_params.addresses ||
        zend_hash_num_elements(Z_ARRVAL_P(common_params.addresses)) == 0) {
        const char* error_message = "Addresses array cannot be empty";
        VALKEY_LOG_ERROR("php_construct", error_message);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
        return;
    }

    /* Build client configuration from individual parameters */
    valkey_glide_base_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));

    /* Populate configuration parameters shared between client and cluster connections. */
    valkey_glide_build_client_config_base(&common_params, &client_config, false);

    /* Issue the connection request. */
    const ConnectionResponse* conn_resp = create_glide_client(&client_config);

    if (conn_resp->connection_error_message) {
        VALKEY_LOG_ERROR("php_construct", conn_resp->connection_error_message);
        zend_throw_exception(
            get_valkey_glide_exception_ce(), conn_resp->connection_error_message, 0);
    } else {
        VALKEY_LOG_INFO("php_construct", "ValkeyGlide client created successfully");
        valkey_glide->glide_client = conn_resp->conn_ptr;
    }

    free_connection_response((ConnectionResponse*) conn_resp);

    /* Clean up temporary configuration structures */
    valkey_glide_cleanup_client_config(&client_config);
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

PHP_METHOD(ValkeyGlide, setOtelSamplePercentage) {
    zend_long percentage;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(percentage)
    ZEND_PARSE_PARAMETERS_END();

    valkey_glide_otel_set_sample_percentage((uint32_t) percentage);
}
/* }}} */

PHP_METHOD(ValkeyGlide, getOtelSamplePercentage) {
    uint32_t percentage;

    if (valkey_glide_otel_get_sample_percentage(&percentage)) {
        RETURN_LONG((zend_long) percentage);
    }
    RETURN_NULL();
}
/* }}} */

/* {{{ proto string ValkeyGlide::updateConnectionPassword(string $password, bool $immediateAuth =
 * false)
 */
UPDATE_CONNECTION_PASSWORD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::clearConnectionPassword(bool $immediateAuth = false)
 */
CLEAR_CONNECTION_PASSWORD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* Basic method stubs - these need to be implemented with ValkeyGlide */

PHP_METHOD(ValkeyGlide, ssubscribe) { /* TODO: Implement */
}
PHP_METHOD(ValkeyGlide, subscribe) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_subscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}

PHP_METHOD(ValkeyGlide, psubscribe) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_psubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}

PHP_METHOD(ValkeyGlide, unsubscribe) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_unsubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}

PHP_METHOD(ValkeyGlide, punsubscribe) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_punsubscribe_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}


PHP_METHOD(ValkeyGlide, publish) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_publish_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}

PHP_METHOD(ValkeyGlide, pubsub) {
    valkey_glide_object* valkey_glide =
        VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());
    if (!valkey_glide->glide_client) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Client not connected", 0);
        RETURN_FALSE;
    }
    valkey_glide_pubsub_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, valkey_glide->glide_client);
}

/* Script commands */

/* {{{ proto array ValkeyGlide::scriptExists(array sha1s) */
SCRIPT_EXISTS_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::scriptKill() */
SCRIPT_KILL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::scriptShow(string sha1) */
SCRIPT_SHOW_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::scriptFlush([string mode]) */
SCRIPT_FLUSH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto ValkeyGlide|bool|string|array ValkeyGlide::script(string operation, mixed ...$args) */
SCRIPT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::fcall(string fn, [array keys], [array args]) */
FCALL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::fcall_ro(string fn, [array keys], [array args]) */
FCALL_RO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::eval(string script, [array args], [int num_keys]) */
EVAL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::evalsha(string sha1, [array args], [int num_keys]) */
EVALSHA_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::eval_ro(string script, [array args], [int num_keys]) */
EVAL_RO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::evalsha_ro(string sha1, [array args], [int num_keys]) */
EVALSHA_RO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* Function commands */
/* {{{ proto string ValkeyGlide::functionLoad(string code, [bool replace]) */
PHP_METHOD(ValkeyGlide, functionLoad) {
    execute_function_load_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto array ValkeyGlide::functionList([string library_name]) */
PHP_METHOD(ValkeyGlide, functionList) {
    execute_function_list_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto bool ValkeyGlide::functionFlush() */
PHP_METHOD(ValkeyGlide, functionFlush) {
    execute_function_flush_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto bool ValkeyGlide::functionDelete(string library_name) */
PHP_METHOD(ValkeyGlide, functionDelete) {
    execute_function_delete_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto string ValkeyGlide::functionDump() */
PHP_METHOD(ValkeyGlide, functionDump) {
    execute_function_dump_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto bool ValkeyGlide::functionRestore(string payload) */
PHP_METHOD(ValkeyGlide, functionRestore) {
    execute_function_restore_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto bool ValkeyGlide::functionKill() */
PHP_METHOD(ValkeyGlide, functionKill) {
    execute_function_kill_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto array ValkeyGlide::functionStats() */
PHP_METHOD(ValkeyGlide, functionStats) {
    execute_function_stats_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

/* {{{ proto ValkeyGlide|bool|string|array ValkeyGlide::function(string operation, mixed ...$args)
 */
PHP_METHOD(ValkeyGlide, function) {
    execute_function_command(getThis(), ZEND_NUM_ARGS(), return_value, valkey_glide_ce);
}
/* }}} */

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


// Individual HFE methods that call unified layer

// HFE methods are implemented in valkey_z_php_methods.c

/**
 * Extracts the SSL options HashTable from a PHP stream context resource.
 * Returns NULL if no stream context is provided or no SSL options are configured.
 *
 * @param params Pointer to the common constructor parameters structure.
 * @return       Pointer to the SSL options HashTable, or NULL if not found.
 */
static HashTable* _get_stream_context_ssl_options_ht(
    valkey_glide_php_common_constructor_params_t* params) {
    if (!params->context || Z_TYPE_P(params->context) != IS_RESOURCE)
        return NULL;

    void* resource =
        zend_fetch_resource_ex(params->context, "Stream-Context", php_le_stream_context());
    if (!resource)
        return NULL;

    php_stream_context* stream_ctx = (php_stream_context*) resource;
    if (Z_TYPE(stream_ctx->options) != IS_ARRAY)
        return NULL;

    zval* ssl_options = zend_hash_str_find(Z_ARRVAL(stream_ctx->options),
                                           VALKEY_GLIDE_SSL_OPTIONS,
                                           sizeof(VALKEY_GLIDE_SSL_OPTIONS) - 1);
    if (!ssl_options || Z_TYPE_P(ssl_options) != IS_ARRAY)
        return NULL;

    return Z_ARRVAL_P(ssl_options);
}

/**
 * Returns the advanced configuration hash table from the given constructor parameters.
 * Returns NULL if no advanced configuration is specified.
 *
 * @param params Pointer to the common constructor parameters structure.
 * @return       Pointer to the advanced configuration hash table, or NULL if not found.
 */
static HashTable* _get_advanced_config_ht(valkey_glide_php_common_constructor_params_t* params) {
    if (!params->advanced_config || Z_TYPE_P(params->advanced_config) != IS_ARRAY)
        return NULL;

    return Z_ARRVAL_P(params->advanced_config);
}

/**
 * Returns the advanced TLS configuration hash table from the given constructor parameters.
 * Returns NULL if no advanced TLS configuration is specified.
 *
 * @param params Pointer to the common constructor parameters structure.
 * @return       Pointer to the advanced TLS configuration hash table, or NULL if not found.
 */
static HashTable* _get_advanced_tls_config_ht(
    valkey_glide_php_common_constructor_params_t* params) {
    HashTable* advanced_config_ht = _get_advanced_config_ht(params);
    if (!advanced_config_ht) {
        return NULL;
    }

    zval* tls_config_val = zend_hash_str_find(
        advanced_config_ht, VALKEY_GLIDE_TLS_CONFIG, sizeof(VALKEY_GLIDE_TLS_CONFIG) - 1);
    if (!tls_config_val || Z_TYPE_P(tls_config_val) != IS_ARRAY) {
        return NULL;
    }

    return Z_ARRVAL_P(tls_config_val);
}

/**
 * Determines the connection timeout from the given constructor parameters.
 *
 * @param params Pointer to the common constructor parameters structure.
 * @return       Connection timeout in milliseconds.
 */
static int _determine_connection_timeout(valkey_glide_php_common_constructor_params_t* params) {
    HashTable* advanced_config_ht = _get_advanced_config_ht(params);
    if (!advanced_config_ht) {
        return VALKEY_GLIDE_DEFAULT_CONNECTION_TIMEOUT;
    }

    zval* conn_timeout_val = zend_hash_str_find(advanced_config_ht,
                                                VALKEY_GLIDE_CONNECTION_TIMEOUT,
                                                sizeof(VALKEY_GLIDE_CONNECTION_TIMEOUT) - 1);
    if (!conn_timeout_val || Z_TYPE_P(conn_timeout_val) != IS_LONG) {
        return VALKEY_GLIDE_DEFAULT_CONNECTION_TIMEOUT;
    }

    return Z_LVAL_P(conn_timeout_val);
}

/**
 * Determines whether to use TLS from the given constructor parameters.
 *
 * @param params Pointer to the common constructor parameters structure.
 * @return       true if TLS should be used, false otherwise.
 */
static bool _determine_use_tls(valkey_glide_php_common_constructor_params_t* params) {
    // 1. Enable TLS if the 'use_tls' parameter is set to true.
    if (params->use_tls)
        return true;

    // 2. Enable TLS if the stream context is specified.
    if (_get_stream_context_ssl_options_ht(params))
        return true;

    // 3. Enable TLS if any address has a host that starts with "tls://" or "ssl://".
    HashTable* addresses_ht = Z_ARRVAL_P(params->addresses);

    zval* addr_val;
    ZEND_HASH_FOREACH_VAL(addresses_ht, addr_val) {
        if (Z_TYPE_P(addr_val) == IS_ARRAY) {
            HashTable* addr_ht  = Z_ARRVAL_P(addr_val);
            zval*      host_val = zend_hash_str_find(
                addr_ht, VALKEY_GLIDE_ADDRESS_HOST, sizeof(VALKEY_GLIDE_ADDRESS_HOST) - 1);

            if (host_val && Z_TYPE_P(host_val) == IS_STRING) {
                const char* host = Z_STRVAL_P(host_val);

                if (strncmp(host, TLS_PREFIX, TLS_PREFIX_LEN) == 0 ||
                    strncmp(host, SSL_PREFIX, SSL_PREFIX_LEN) == 0) {
                    return true;
                }
            }
        }
    }
    ZEND_HASH_FOREACH_END();

    return false;
}

/**
 * Builds the advanced TLS configuration from the given constructor parameters.
 * Returns NULL if no advanced TLS configuration is specified.
 *
 * @param params     Pointer to the common constructor parameters structure.
 * @param is_cluster Whether this is a cluster client
 * @return           Pointer to the constructed TLS advanced configuration structure.
 */
static valkey_glide_tls_advanced_configuration_t* _build_advanced_tls_config(
    valkey_glide_php_common_constructor_params_t* params, bool is_cluster) {
    // Allocate memory with default values.
    valkey_glide_tls_advanced_configuration_t* tls_advanced_config =
        ecalloc(1, sizeof(valkey_glide_tls_advanced_configuration_t));

    tls_advanced_config->use_insecure_tls = false;
    tls_advanced_config->root_certs       = NULL;
    tls_advanced_config->root_certs_len   = 0;

    // TLS configuration can be specified either from
    // the stream context or from the advanced TLS config.
    HashTable* stream_context_ht = _get_stream_context_ssl_options_ht(params);
    HashTable* advanced_tls_ht   = _get_advanced_tls_config_ht(params);

    // Raise an exception if both are provided.
    if (stream_context_ht && advanced_tls_ht) {
        const char* error_msg =
            "At most one of stream context or advanced TLS config can be specified.";
        VALKEY_LOG_ERROR("tls_config_conflict", error_msg);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
    }

    // Set TLS configuration from stream context.
    // Reference: https://www.php.net/manual/en/context.ssl.php
    else if (stream_context_ht) {
        // Insecure TLS
        zval* verify_peer_val = zend_hash_str_find(
            stream_context_ht, VALKEY_GLIDE_VERIFY_PEER, sizeof(VALKEY_GLIDE_VERIFY_PEER) - 1);
        if (verify_peer_val && Z_TYPE_P(verify_peer_val) == IS_FALSE) {
            tls_advanced_config->use_insecure_tls = true;
        }

        // Root certificate
        zval* cafile_val = zend_hash_str_find(
            stream_context_ht, VALKEY_GLIDE_CAFILE, sizeof(VALKEY_GLIDE_CAFILE) - 1);
        if (cafile_val && Z_TYPE_P(cafile_val) == IS_STRING) {
            const char* cafile_path = Z_STRVAL_P(cafile_val);
            uint8_t*    cert_data;
            size_t      cert_len;

            if (_load_data_from_file(cafile_path, &cert_data, &cert_len)) {
                tls_advanced_config->root_certs     = cert_data;
                tls_advanced_config->root_certs_len = cert_len;
            } else {
                const char* error_message = "Failed to load root certificate from file";
                VALKEY_LOG_ERROR("tls_config_cafile", error_message);
                zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
            }
        }
    }

    // Set TLS configuration from advanced TLS config.
    else if (advanced_tls_ht) {
        // Insecure TLS
        zval* use_insecure_tls_val = zend_hash_str_find(advanced_tls_ht,
                                                        VALKEY_GLIDE_USE_INSECURE_TLS,
                                                        sizeof(VALKEY_GLIDE_USE_INSECURE_TLS) - 1);
        if (use_insecure_tls_val && Z_TYPE_P(use_insecure_tls_val) == IS_TRUE) {
            tls_advanced_config->use_insecure_tls = true;
        }

        // Root certificate
        zval* root_certs = zend_hash_str_find(
            advanced_tls_ht, VALKEY_GLIDE_ROOT_CERTS, sizeof(VALKEY_GLIDE_ROOT_CERTS) - 1);
        if (root_certs && Z_TYPE_P(root_certs) == IS_STRING) {
            uint8_t* cert_data = (uint8_t*) Z_STRVAL_P(root_certs);
            size_t   cert_len  = Z_STRLEN_P(root_certs);

            tls_advanced_config->root_certs_len = cert_len;
            tls_advanced_config->root_certs     = emalloc(cert_len);
            memcpy(tls_advanced_config->root_certs, Z_STRVAL_P(root_certs), cert_len);
        }
    }

    return tls_advanced_config;
}

/**
 * Builds the advanced configuration from the given constructor parameters.
 * Returns NULL if no advanced configuration is specified.
 *
 * @param params     Pointer to the common constructor parameters structure.
 * @param is_cluster Whether this is a cluster client
 * @return           Pointer to the constructed advanced configuration structure.
 */
static valkey_glide_advanced_base_client_configuration_t* _build_advanced_config(
    valkey_glide_php_common_constructor_params_t* params, bool is_cluster) {
    valkey_glide_advanced_base_client_configuration_t* advanced_config =
        ecalloc(1, sizeof(valkey_glide_advanced_base_client_configuration_t));

    advanced_config->connection_timeout = _determine_connection_timeout(params);
    advanced_config->tls_config         = _build_advanced_tls_config(params, is_cluster);

    return advanced_config;
}

/**
 * Initializes OpenTelemetry from the given constructor parameters, if configured.
 * Throws an exception if initialization fails.
 *
 * @param params     Pointer to the common constructor parameters structure.
 * @param is_cluster Whether this is a cluster client
 */
static void _initialize_open_telemetry(valkey_glide_php_common_constructor_params_t* params,
                                       bool                                          is_cluster) {
    if (!params->advanced_config || Z_TYPE_P(params->advanced_config) != IS_ARRAY)
        return;

    HashTable* advanced_config_ht = Z_ARRVAL_P(params->advanced_config);

    zval* otel_config_val = zend_hash_str_find(advanced_config_ht, "otel", sizeof("otel") - 1);
    if (!otel_config_val)
        return;

    /* Validate that OTEL config is an object, not an array or other type */
    if (Z_TYPE_P(otel_config_val) != IS_NULL && Z_TYPE_P(otel_config_val) != IS_OBJECT) {
        VALKEY_LOG_ERROR("otel_config",
                         "OpenTelemetry configuration must be an OpenTelemetryConfig object");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "OpenTelemetry configuration must be an OpenTelemetryConfig object",
                             0);
        return;
    }

    VALKEY_LOG_DEBUG("otel_config", "Processing OTEL configuration from advanced_config");

    if (!valkey_glide_otel_init(otel_config_val)) {
        VALKEY_LOG_ERROR("otel_config", "Failed to initialize OTEL");
        /* Exception already thrown by valkey_glide_otel_init if validation failed */
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to initialize OpenTelemetry", 0);
    }

    return;
}

/**
 * Loads data from the file at the specified path.
 * Returns true if successful, false otherwise.
 *
 * @param path   Path to the file
 * @param data   Pointer to store the data (caller must free)
 * @param length Pointer to store the data length
 * @return       true if successful, false otherwise
 */
static bool _load_data_from_file(const char* path, uint8_t** data, size_t* length) {
    /* Open the file */
    FILE* f = fopen(path, "rb");
    if (!f)
        return false;

    /* Get file size using fstat */
    struct stat st;
    if (fstat(fileno(f), &st) != 0 || st.st_size <= 0) {
        fclose(f);
        return false;
    }

    /* Allocate data */
    *length = (size_t) st.st_size;
    *data   = emalloc(*length);

    /* Read file contents */
    size_t bytes_read = fread(*data, 1, *length, f);
    fclose(f);

    /* Verify the read succeeded */
    if (bytes_read != *length) {
        efree(*data);
        *data   = NULL;
        *length = 0;
        return false;
    }

    return true;
}
