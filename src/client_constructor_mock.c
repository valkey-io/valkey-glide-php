/*
  +----------------------------------------------------------------------+
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

#include "common.h"
#include "ext/standard/info.h"
#include "logger.h"
#include "php.h"
#include "zend_exceptions.h"

#if PHP_VERSION_ID < 80000
#include "src/client_constructor_mock_legacy_arginfo.h"
#else
#include "src/client_constructor_mock_arginfo.h"
#include "zend_attributes.h"
#endif

#include "valkey_glide_commands_common.h"
#include "zend_API.h"
#include "zend_interfaces.h"

/* Global variables */
zend_class_entry* mock_constructor_ce;

void register_mock_constructor_class(void) {
    /* Use the generated registration function */
    mock_constructor_ce = register_class_ClientConstructorMock();
}

static zval* build_php_connection_request(uint8_t*                                  request_bytes,
                                          size_t                                    request_len,
                                          valkey_glide_base_client_configuration_t* base_config) {
    if (!request_bytes) {
        const char* error_message = "Protobuf memory allocation error.";
        VALKEY_LOG_ERROR("mock_construct", error_message);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
        valkey_glide_cleanup_client_config(base_config);
        return NULL;
    }

    zval buffer_param, callable, retval;
    ZVAL_UNDEF(&retval);
    ZVAL_STRINGL(&buffer_param, (char*) request_bytes, request_len);

    zval params[1];
    params[0] = buffer_param;
    array_init(&callable);
    add_next_index_string(&callable, "ConnectionRequestTest");
    add_next_index_string(&callable, "deserialize");

    zval* result = NULL;
    if (call_user_function(NULL, NULL, &callable, &retval, 1, params) == SUCCESS) {
        // Allocate return value
        result = emalloc(sizeof(zval));
        ZVAL_COPY(result, &retval);
    }

    zval_ptr_dtor(&callable);
    zval_ptr_dtor(&retval);
    zval_ptr_dtor(&buffer_param);

    efree(request_bytes);
    valkey_glide_cleanup_client_config(base_config);
    return result;
}

/*
 * PHP Methods
 */

PHP_METHOD(ClientConstructorMock, simulate_standalone_constructor) {
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);

    ZEND_PARSE_PARAMETERS_START(1, 11)
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
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    /* Validate addresses array */
    if (!common_params.addresses ||
        zend_hash_num_elements(Z_ARRVAL_P(common_params.addresses)) == 0) {
        const char* error_message = "Addresses array cannot be empty";
        VALKEY_LOG_ERROR("mock_construct", error_message);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
        return;
    }

    /* Validate database_id range before setting */
    if (!common_params.database_id_is_null && common_params.database_id < 0) {
        const char* error_message = "Database ID must be non-negative.";
        VALKEY_LOG_ERROR("mock_construct", error_message);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
        return;
    }

    /* Build client configuration from individual parameters */
    valkey_glide_base_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));

    /* Populate configuration parameters shared between client and cluster connections. */
    valkey_glide_build_client_config_base(&common_params, &client_config, false);

    /* Build the connection request. */
    size_t   protobuf_message_len;
    uint8_t* request_bytes = create_connection_request(
        "localhost", 6379, &protobuf_message_len, &client_config, 0, false);

    zval* php_request =
        build_php_connection_request(request_bytes, protobuf_message_len, &client_config);
    RETURN_ZVAL(php_request, 1, 1);
}

/* Simulates a ValkeyGlideCluster constructor */
PHP_METHOD(ClientConstructorMock, simulate_cluster_constructor) {
    zend_long                                    periodic_checks         = 0;
    zend_bool                                    periodic_checks_is_null = 1;
    valkey_glide_php_common_constructor_params_t common_params;
    valkey_glide_init_common_constructor_params(&common_params);

    ZEND_PARSE_PARAMETERS_START(1, 12)
    Z_PARAM_ARRAY(common_params.addresses)
    Z_PARAM_OPTIONAL
    Z_PARAM_BOOL(common_params.use_tls)
    Z_PARAM_ARRAY_OR_NULL(common_params.credentials)
    Z_PARAM_LONG(common_params.read_from)
    Z_PARAM_LONG_OR_NULL(common_params.request_timeout, common_params.request_timeout_is_null)
    Z_PARAM_ARRAY_OR_NULL(common_params.reconnect_strategy)
    Z_PARAM_STRING_OR_NULL(common_params.client_name, common_params.client_name_len)
    Z_PARAM_LONG_OR_NULL(periodic_checks, periodic_checks_is_null)
    Z_PARAM_STRING_OR_NULL(common_params.client_az, common_params.client_az_len)
    Z_PARAM_ARRAY_OR_NULL(common_params.advanced_config)
    Z_PARAM_BOOL_OR_NULL(common_params.lazy_connect, common_params.lazy_connect_is_null)
    Z_PARAM_LONG_OR_NULL(common_params.database_id, common_params.database_id_is_null)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    /* Validate database_id range before setting */
    if (!common_params.database_id_is_null && common_params.database_id < 0) {
        const char* error_message = "Database ID must be non-negative.";
        VALKEY_LOG_ERROR("mock_construct", error_message);
        zend_throw_exception(get_valkey_glide_exception_ce(), error_message, 0);
        return;
    }

    /* Build cluster client configuration from individual parameters */
    valkey_glide_cluster_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));

    /* Set periodic checks */
    client_config.periodic_checks_status =
        periodic_checks_is_null ? VALKEY_GLIDE_PERIODIC_CHECKS_ENABLED_DEFAULT : periodic_checks;
    client_config.periodic_checks_manual = NULL;

    /* Populate configuration parameters shared between client and cluster connections. */
    valkey_glide_build_client_config_base(&common_params, &client_config.base, true);

    /* Build the connection request. */
    size_t   protobuf_message_len;
    uint8_t* request_bytes = create_connection_request("localhost",
                                                       6379,
                                                       &protobuf_message_len,
                                                       &client_config.base,
                                                       client_config.periodic_checks_status,
                                                       true);

    zval* php_request =
        build_php_connection_request(request_bytes, protobuf_message_len, &client_config.base);
    RETURN_ZVAL(php_request, 1, 1);
}
