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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include <ext/hash/php_hash.h>
#include <ext/spl/spl_exceptions.h>
#include <ext/standard/info.h>

#include "command_response.h"
#include "common.h"
#include "include/glide_bindings.h"
#include "logger.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_list_common.h"
#include "valkey_glide_z_common.h"

extern zend_class_entry* ce;
extern zend_class_entry* get_valkey_glide_exception_ce();

#include "valkey_glide_pubsub_common.h"

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);


/* Create a connection request in protobuf format. Made visible for testing. */
uint8_t* create_connection_request(size_t*                                   len,
                                   valkey_glide_base_client_configuration_t* config,
                                   valkey_glide_periodic_checks_status_t     periodic_checks,
                                   bool                                      is_cluster,
                                   bool refresh_topology_from_initial_nodes) {
    /* Create a connection request */
    ConnectionRequest__ConnectionRequest conn_req = CONNECTION_REQUEST__CONNECTION_REQUEST__INIT;

    /* Addresses array is non-empty - see ValkeyGlide::__construct */
    size_t                       address_count = config->addresses_count;
    valkey_glide_node_address_t* addresses     = config->addresses;

    // Using variable-length arrays to avoid memory management.
    ConnectionRequest__NodeAddress  request_addresses[address_count];
    ConnectionRequest__NodeAddress* request_addresses_list[address_count];

    for (size_t i = 0; i < address_count; ++i) {
        request_addresses[i] =
            (ConnectionRequest__NodeAddress) CONNECTION_REQUEST__NODE_ADDRESS__INIT;
        request_addresses[i].host = (char*) addresses[i].host;
        request_addresses[i].port = addresses[i].port;
        request_addresses_list[i] = &request_addresses[i];
    }

    /* Add the node address to the connection request. */
    conn_req.n_addresses = config->addresses_count;
    conn_req.addresses   = request_addresses_list;

    /* Set up authentication if provided */
    ConnectionRequest__AuthenticationInfo auth_info = CONNECTION_REQUEST__AUTHENTICATION_INFO__INIT;
    ConnectionRequest__IamCredentials     iam_creds = CONNECTION_REQUEST__IAM_CREDENTIALS__INIT;

    if (config->credentials) {
        auth_info.username = (char*) config->credentials->username;
        auth_info.password = (char*) config->credentials->password;

        /* Set up IAM credentials if provided */
        if (config->credentials->iam_config) {
            iam_creds.cluster_name = (char*) config->credentials->iam_config->cluster_name;
            iam_creds.region       = (char*) config->credentials->iam_config->region;
            iam_creds.service_type = (config->credentials->iam_config->service_type ==
                                      VALKEY_GLIDE_SERVICE_TYPE_MEMORYDB)
                                         ? CONNECTION_REQUEST__SERVICE_TYPE__MEMORYDB
                                         : CONNECTION_REQUEST__SERVICE_TYPE__ELASTICACHE;
            iam_creds.refresh_interval_seconds =
                config->credentials->iam_config->refresh_interval_seconds;

            auth_info.iam_credentials = &iam_creds;
            /* Clear password when using IAM */
            auth_info.password = (char*) protobuf_c_empty_string;
        }

        conn_req.authentication_info = &auth_info;
    }

    /* Set TLS mode */
    if (!config->use_tls) {
        conn_req.tls_mode = CONNECTION_REQUEST__TLS_MODE__NoTls;
    } else if (config->advanced_config && config->advanced_config->tls_config &&
               config->advanced_config->tls_config->use_insecure_tls) {
        conn_req.tls_mode = CONNECTION_REQUEST__TLS_MODE__InsecureTls;
    } else {
        conn_req.tls_mode = CONNECTION_REQUEST__TLS_MODE__SecureTls;
    }

    /* Set root certificates */
    ProtobufCBinaryData root_cert_data;
    if (config->advanced_config && config->advanced_config->tls_config) {
        valkey_glide_tls_advanced_configuration_t* tls_config = config->advanced_config->tls_config;

        if (tls_config->root_certs && tls_config->root_certs_len > 0) {
            root_cert_data =
                (ProtobufCBinaryData){tls_config->root_certs_len, tls_config->root_certs};
            conn_req.n_root_certs = 1;
            conn_req.root_certs   = &root_cert_data;
        }
    }

    conn_req.cluster_mode_enabled = is_cluster;
    conn_req.request_timeout =
        config->request_timeout > 0 ? config->request_timeout : 5000; /* Default 5 seconds */

    if (config->advanced_config) {
        conn_req.connection_timeout = config->advanced_config->connection_timeout;
    }

    /* Set refresh topology from initial nodes for cluster mode */
    if (is_cluster) {
        conn_req.refresh_topology_from_initial_nodes = refresh_topology_from_initial_nodes;
    } else {
        conn_req.refresh_topology_from_initial_nodes = false;
    }

    conn_req.lazy_connect = config->lazy_connect;
    /* Map read_from configuration */
    if (config->read_from == VALKEY_GLIDE_READ_FROM_PREFER_REPLICA) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__PreferReplica;
    } else if (config->read_from == VALKEY_GLIDE_READ_FROM_AZ_AFFINITY) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__AZAffinity;
    } else if (config->read_from == VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__AZAffinityReplicasAndPrimary;
    } else {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__Primary;
    }

    /* Set database ID for standalone clients if it is valid. */
    if (config->database_id >= 0) {
        conn_req.database_id = config->database_id;
    } else {
        conn_req.database_id = 0;
    }

    /* Set the periodic checks for cluster clients. */
    ConnectionRequest__PeriodicChecksDisabled periodic_check_disabled_info =
        CONNECTION_REQUEST__PERIODIC_CHECKS_DISABLED__INIT;
    ConnectionRequest__PeriodicChecksManualInterval periodic_checks_manual_interval_info =
        CONNECTION_REQUEST__PERIODIC_CHECKS_MANUAL_INTERVAL__INIT;
    if (is_cluster && periodic_checks == VALKEY_GLIDE_PERIODIC_CHECKS_DISABLED) {
        conn_req.periodic_checks_disabled = &periodic_check_disabled_info;
        conn_req.periodic_checks_case =
            CONNECTION_REQUEST__CONNECTION_REQUEST__PERIODIC_CHECKS_PERIODIC_CHECKS_DISABLED;
    } else {
        conn_req.periodic_checks_manual_interval = &periodic_checks_manual_interval_info;
        conn_req.periodic_checks_case =
            CONNECTION_REQUEST__CONNECTION_REQUEST__PERIODIC_CHECKS_PERIODIC_CHECKS_MANUAL_INTERVAL;
    }

    conn_req.protocol = CONNECTION_REQUEST__PROTOCOL_VERSION__RESP3;

    /* Set the reconnect strategy */
    ConnectionRequest__ConnectionRetryStrategy retry_strategy =
        CONNECTION_REQUEST__CONNECTION_RETRY_STRATEGY__INIT;
    if (config->reconnect_strategy) {
        conn_req.connection_retry_strategy = &retry_strategy;
        retry_strategy.number_of_retries   = config->reconnect_strategy->num_of_retries;
        retry_strategy.factor              = config->reconnect_strategy->factor;
        retry_strategy.exponent_base       = config->reconnect_strategy->exponent_base;
        retry_strategy.jitter_percent      = config->reconnect_strategy->jitter_percent;
    }

    /* Set client name */
    conn_req.client_name = config->client_name ? config->client_name : "valkey-glide-php";

    /* Set client AZ */
    if (config->client_az) {
        conn_req.client_az = config->client_az;
    }

    /* Enable pubsub infrastructure for dynamic subscriptions */
    ConnectionRequest__PubSubSubscriptions pubsub_subs =
        CONNECTION_REQUEST__PUB_SUB_SUBSCRIPTIONS__INIT;
    pubsub_subs.n_channels_or_patterns_by_type = 0;
    pubsub_subs.channels_or_patterns_by_type   = NULL;
    conn_req.pubsub_subscriptions              = &pubsub_subs;

    /* Calculate the size of the serialized message */
    *len = connection_request__connection_request__get_packed_size(&conn_req);

    /* Allocate memory for the serialized message */
    uint8_t* buffer = (uint8_t*) emalloc(*len);
    if (!buffer) {
        *len = 0;
        return NULL;
    }

    /* Serialize the message */
    connection_request__connection_request__pack(&conn_req, buffer);

    return buffer;
}

/* Create a Valkey Glide client or Cluster client using shared properties. */
static const ConnectionResponse* create_base_glide_client(
    valkey_glide_base_client_configuration_t* config,
    valkey_glide_periodic_checks_status_t     periodic_checks,
    bool                                      is_cluster,
    bool                                      refresh_topology_from_initial_nodes) {
    size_t   len;
    uint8_t* request_bytes = create_connection_request(
        &len, config, periodic_checks, is_cluster, refresh_topology_from_initial_nodes);

    if (!request_bytes) {
        return NULL;
    }

    /* Set up client type for synchronous operation */
    ClientType client_type;
    client_type.tag = SyncClient;

    /* Create the client with pubsub callback registered at creation time */
    const ConnectionResponse* conn_resp =
        create_client(request_bytes, len, &client_type, valkey_glide_pubsub_callback);

    /* Free the request bytes as they're no longer needed */
    efree(request_bytes);

    /* Check if there was an error */
    if (conn_resp->connection_error_message) {
        VALKEY_LOG_ERROR("client_creation", conn_resp->connection_error_message);
    }

    return conn_resp;
}

/* Create a Valkey Glide client */
const ConnectionResponse* create_glide_client(valkey_glide_base_client_configuration_t* config) {
    return create_base_glide_client(config, VALKEY_GLIDE_PERIODIC_CHECKS_DISABLED, false, false);
}

const ConnectionResponse* create_glide_cluster_client(
    valkey_glide_cluster_client_configuration_t* config) {
    return create_base_glide_client(&config->base,
                                    config->periodic_checks_status,
                                    true,
                                    config->refresh_topology_from_initial_nodes);
}

/* Custom result processor for SET commands with GET option support */
struct set_result_data {
    int has_get;
};

static int process_set_result(CommandResponse* response, void* output, zval* return_value) {
    struct set_result_data* data = (struct set_result_data*) output;

    if (!response) {
        efree(output);
        return 0;
    }

    switch (response->response_type) {
        case Ok:
            efree(output);
            ZVAL_TRUE(return_value);
            return 1; /* Success */
        case Null:
            efree(output);
            ZVAL_FALSE(return_value);
            return 0; /* Not set (NX/XX condition not met) */
        case String:
            /* GET option returned a value */
            if (data->has_get && response->string_value) {
                char*  old_val     = emalloc(response->string_value_len + 1);
                size_t old_val_len = response->string_value_len;
                if (old_val) {
                    memcpy(old_val, response->string_value, response->string_value_len);
                    (old_val)[response->string_value_len] = '\0';
                }

                ZVAL_STRINGL(return_value, old_val, old_val_len);
                efree(old_val);
            }
            efree(output);
            return 2; /* GET option returned a value */
        default:
            efree(output);
            ZVAL_FALSE(return_value);
            return 0; /* Error */
    }
}

/* Custom result processor for PING command */
int process_ping_result(CommandResponse* response, void* output, zval* return_value) {
    struct {
        char*  msg;
        size_t msg_len;
    }* string_output = output;

    char*  result;
    size_t result_len;

    if (!response || !string_output) {
        efree(output);
        return 0;
    }
    int status = 0; /* 1 = success, 0 = failure */
    if (response->response_type == Ok) {
        /* PONG response with no message */
        result     = estrdup("PONG");
        result_len = 4;
        status     = 1;
    } else if (response->response_type == String) {
        /* PING with message - echo the message back */
        if (response->string_value_len == 0) {
            result = emalloc(1);
            if (result) {
                (result)[0] = '\0';
            }
            result_len = 0;
        } else {
            result = emalloc(response->string_value_len + 1);
            if (result) {
                memcpy(result, response->string_value, response->string_value_len);
                (result)[response->string_value_len] = '\0';
            }
            result_len = response->string_value_len;
        }
        status = result ? 1 : 0;
    } else if (response->response_type == Null) {
        result     = NULL;
        result_len = 0;
        status     = 0;
    }
    if (status == 1) {
        if (response != NULL) {
            /* Check if message was provided */
            int has_message = (string_output->msg != NULL && string_output->msg_len > 0);

            /* If no message was provided and response is "PONG", return true */
            if (!has_message && result_len == 4 && strncmp(result, "PONG", 4) == 0) {
                efree(result);
                ZVAL_TRUE(return_value);
                status = 1;
            } else {
                /* Otherwise, return the actual response string */
                ZVAL_STRINGL(return_value, result, result_len);
                efree(result);
                status = 1;
            }
        } else {
            /* Success but no response (should return TRUE for PONG) */
            ZVAL_TRUE(return_value);
            status = 1;
        }
    }
    efree(output);
    return status;
}

/* These functions are now defined in command_response.c */

/* Execute a BITCOUNT command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bitcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            start = 0, end = -1;
    zend_bool            bybit = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os|llb", &object, ce, &key, &key_len, &start, &end, &bybit) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = BitCount;
    args.key                 = key;
    args.key_len             = key_len;

    /* Set range options */
    args.options.start     = start;
    args.options.end       = end;
    args.options.has_range = 1;
    args.options.bybit     = bybit;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }


        return 1;
    }

    return 0;
}

/* Execute a BITOP command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bitop_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               op = NULL, *key = NULL;
    size_t               op_len, key_len;
    zval*                keys       = NULL;
    int                  keys_count = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss*", &object, ce, &op, &op_len, &key, &key_len, &keys, &keys_count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    zval keys_array;
    array_init_size(&keys_array, keys_count);

    for (int i = 0; i < keys_count; i++) {
        zval copy;
        ZVAL_COPY(&copy, &keys[i]);
        add_next_index_zval(&keys_array, &copy);
    }


    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = BitOp;
    args.key                 = key; /* destination key */
    args.key_len             = key_len;

    /* Add operation as first argument */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = op;
    args.args[0].data.string_arg.len   = op_len;

    /* Add source keys as array argument (no limit on number of keys) */
    args.args[1].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[1].data.array_arg.array = &keys_array;
    args.args[1].data.array_arg.count = keys_count;
    args.arg_count                    = 2; /* operation + source keys array */

    int result =
        execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value);
    zval_ptr_dtor(&keys_array);

    if (result) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}

/* Execute a BITPOS command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bitpos_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            bit, start = 0, end = -1;
    zend_bool            bybit = 0;


    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osl|llb", &object, ce, &key, &key_len, &bit, &start, &end, &bybit) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = BitPos;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add bit value argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = bit;
    args.arg_count                   = 1;

    /* Set range options */
    args.options.start     = start;
    args.options.end       = end;
    args.options.has_range = 1;
    args.options.bybit     = bybit;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }


        return 1;
    }

    return 0;
}

/* Execute a SET command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_set_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval *               z_value, *z_expire = NULL, *z_opts = NULL;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    double               expire     = 0;
    zend_long            expire_int = 0;
    zval*  z_set_opts  = NULL; /* Will hold our options either from z_expire or z_opts */
    int    free_val    = 0;    /* Flag to track if we need to free val */
    char*  old_val     = NULL; /* For storing GET response */
    size_t old_val_len = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osz|za", &object, ce, &key, &key_len, &z_value, &z_expire, &z_opts) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if expire parameter was provided */
    if (z_expire != NULL) {
        switch (Z_TYPE_P(z_expire)) {
            case IS_DOUBLE:
                /* Double - use as timeout */
                expire     = Z_DVAL_P(z_expire);
                expire_int = (zend_long) expire;
                break;
            case IS_LONG:
                /* Long - use as timeout */
                expire     = (double) Z_LVAL_P(z_expire);
                expire_int = Z_LVAL_P(z_expire);
                break;
            case IS_ARRAY:
                /* Array - use as options */
                z_set_opts = z_expire;
                break;
            case IS_NULL:
                /* NULL - ignore */
                break;
            default:
                /* Not a supported type - return false */
                return 0;
        }
    }

    /* If options were passed in z_opts, use those instead */
    if (z_opts != NULL && Z_TYPE_P(z_opts) == IS_ARRAY) {
        z_set_opts = z_opts;
    }

    /* Convert value based on its type */
    switch (Z_TYPE_P(z_value)) {
        case IS_STRING:
            /* It's already a string, use directly */
            val     = Z_STRVAL_P(z_value);
            val_len = Z_STRLEN_P(z_value);
            break;
        case IS_LONG:
            /* Convert integer to string */
            val      = long_to_string(Z_LVAL_P(z_value), &val_len);
            free_val = 1;  // We'll need to free this
            break;
        case IS_DOUBLE:
            /* Convert float to string */
            val      = double_to_string(Z_DVAL_P(z_value), &val_len);
            free_val = 1;  // We'll need to free this
            break;
        case IS_TRUE:
            /* Convert boolean TRUE to "1" */
            val      = estrdup("1");
            val_len  = 1;
            free_val = 1;
            break;
        case IS_FALSE:
            /* Convert boolean FALSE to "0" */
            val      = estrdup("0");
            val_len  = 1;
            free_val = 1;
            break;
        case IS_NULL:
            /* Convert NULL to empty string */
            val      = estrdup("");
            val_len  = 0;
            free_val = 1;
            break;
        default:
            /* Unsupported type */
            return 0;
    }

    /* Check if conversion succeeded */
    if (!val) {
        return 0;
    }

    /* Execute the SET command using the internal helper function */
    int result = execute_set_command_internal(valkey_glide,
                                              key,
                                              key_len,
                                              val,
                                              val_len,
                                              expire_int,
                                              z_set_opts,
                                              &old_val,
                                              &old_val_len,
                                              return_value);

    /* Free the allocated string if needed */
    if (free_val) {
        efree(val);
    }

    /* Check for batch mode after successful execution */
    if (result) {
        if (valkey_glide->is_in_batch_mode) {
            /* Clean up old_val if it was allocated */
            if (old_val) {
                efree(old_val);
            }
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
        }
        return 1;
    }


    return 0; /* Should not reach here, but just in case */
}

/* Execute a SET command using the Valkey Glide client - INTERNAL HELPER FUNCTION */
int execute_set_command_internal(valkey_glide_object* valkey_glide,
                                 const char*          key,
                                 size_t               key_len,
                                 const char*          val,
                                 size_t               val_len,
                                 long                 expire,
                                 zval*                opts,
                                 char**               old_val,
                                 size_t*              old_val_len,
                                 zval*                return_value) {
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Set;
    args.key                 = key;
    args.key_len             = key_len;
    args.raw_options         = opts;

    /* Add value argument */
    args.args[0].type                  = CORE_ARG_TYPE_STRING;
    args.args[0].data.string_arg.value = val;
    args.args[0].data.string_arg.len   = val_len;
    args.arg_count                     = 1;

    /* Parse options */
    if (opts) {
        if (parse_set_options(opts, &args.options) == 0) {
            /* If parsing failed, return error */
            return 0;
        }
    }

    /* Set expire if provided */
    if (expire > 0) {
        args.options.expire_seconds = expire;
        args.options.has_expire     = 1;
    }

    /* Prepare result data for GET option */
    struct set_result_data* result_data = emalloc(sizeof(struct set_result_data));
    result_data->has_get                = args.options.get_old_value;

    return execute_core_command(valkey_glide, &args, result_data, process_set_result, return_value);
}

/* Execute a SETEX command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_setex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    zend_long            expire;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osls", &object, ce, &key, &key_len, &expire, &val, &val_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Call execute_set_command_internal with expire in seconds (EX) and no special options */
    int result = execute_set_command_internal(
        valkey_glide, key, key_len, val, val_len, expire, NULL, NULL, NULL, return_value);

    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
        }

        return 1;
    } else {
        return 0;
    }
}

/* Execute a PSETEX command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_psetex_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    zend_long            expire;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osls", &object, ce, &key, &key_len, &expire, &val, &val_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Create options array for PX option */
    zval options;
    array_init(&options);

    /* Add PX option with expire value */
    add_assoc_long_ex(&options, "PX", sizeof("PX") - 1, expire);

    /* Call execute_set_command_internal with the PX option */
    int result = execute_set_command_internal(
        valkey_glide, key, key_len, val, val_len, 0, &options, NULL, NULL, return_value);

    /* Clean up options array */
    zval_dtor(&options);

    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
        }

        return 1;
    } else {
        return 0;
    }
}

/* Execute a SETNX command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_setnx_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &val, &val_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Create options array for NX option */
    zval options;
    array_init(&options);

    /* Add NX option as a numeric index */
    zval nx_option;
    ZVAL_STRING(&nx_option, "NX");
    add_next_index_zval(&options, &nx_option);

    /* Call execute_set_command_internal with the NX option and no expiration */
    int result = execute_set_command_internal(
        valkey_glide, key, key_len, val, val_len, 0, &options, NULL, NULL, return_value);

    /* Clean up options array */
    zval_dtor(&options);

    if (result == 1) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
        }
        return 1;
    } else {
        return 0;
    }
}

/* Function to close a Valkey Glide client */
void close_glide_client(const void* glide_client) {
    /* Check if client is valid */
    if (!glide_client) {
        return;
    }
    /* Close the client using the close_client function from glide_bindings.h */
    close_client(glide_client);
}

/* Execute an ECHO command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_echo_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    char*                msg        = NULL;
    size_t               msg_len    = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* ECHO cannot be used in batch mode */
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Echo;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - first parameter is route, second is message */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count < 2) {
            /* Need both route and message parameters */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];

        /* Get message parameter */
        zval* message = &args[1];
        if (Z_TYPE_P(message) == IS_STRING) {
            msg     = Z_STRVAL_P(message);
            msg_len = Z_STRLEN_P(message);
        } else {
            /* Convert to string if needed */
            convert_to_string(message);
            msg     = Z_STRVAL_P(message);
            msg_len = Z_STRLEN_P(message);
        }
    } else {
        /* Non-cluster case - parse message parameter only */
        if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &msg, &msg_len) ==
            FAILURE) {
            return 0;
        }
    }

    /* Add message argument to core framework */
    core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
    core_args.args[0].data.string_arg.value = msg;
    core_args.args[0].data.string_arg.len   = msg_len;
    core_args.arg_count                     = 1;


    /* Execute using unified core framework */
    if (execute_core_command(
            valkey_glide, &core_args, NULL, process_core_string_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            /* Note: output will be freed later in process_core_string_result */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }
    return 0;
}

/* Execute a PING command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_ping_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args         = NULL;
    int                  args_count   = 0;
    char*                msg          = NULL;
    size_t               msg_len      = 0;
    char*                response     = NULL;
    size_t               response_len = 0;
    zend_bool            is_cluster   = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* Ping cannot be used in batch mode */
        return 0;
    }


    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Ping;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - first parameter is route, optional second is message */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];

        /* Get optional message parameter */
        if (args_count > 1) {
            zval* message = &args[1];
            if (Z_TYPE_P(message) == IS_STRING) {
                msg     = Z_STRVAL_P(message);
                msg_len = Z_STRLEN_P(message);
            } else {
                /* Convert to string if needed */
                convert_to_string(message);
                msg     = Z_STRVAL_P(message);
                msg_len = Z_STRLEN_P(message);
            }
        }
    } else {
        /* Non-cluster case - parse optional message parameter */
        if (zend_parse_method_parameters(argc, object, "O|s", &object, ce, &msg, &msg_len) ==
            FAILURE) {
            return 0;
        }
    }

    /* Add optional message argument to core framework */
    if (msg && msg_len > 0) {
        core_args.args[0].type                  = CORE_ARG_TYPE_STRING;
        core_args.args[0].data.string_arg.value = msg;
        core_args.args[0].data.string_arg.len   = msg_len;
        core_args.arg_count                     = 1;
    }

    /* Use ping result processor */

    struct {
        char*  msg;
        size_t msg_len;
    }* output       = emalloc(sizeof(*output));
    output->msg     = msg;
    output->msg_len = msg_len;

    /* Execute using unified core framework */
    if (execute_core_command(valkey_glide, &core_args, output, process_ping_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    /* Error or empty response */
    return 0;
}

#define _NL "\r\n"


static void valkey_glide_parse_info_response(char* response, size_t response_len, zval* z_ret) {
    ZVAL_FALSE(z_ret);

    if (!response || response_len == 0) {
        return;
    }

    /* Create a null-terminated copy of the response to ensure safe string operations */
    char* safe_response = emalloc(response_len + 1);
    if (!safe_response) {
        return;
    }

    memcpy(safe_response, response, response_len);
    safe_response[response_len] = '\0';

    char* p1;
    char* s1 = NULL;

    if ((p1 = php_strtok_r(safe_response, _NL, &s1)) != NULL) {
        array_init(z_ret);
        do {
            /* Skip comment lines */
            if (*p1 == '#') {
                continue;
            }

            /* Find the colon separator */
            char* p = strchr(p1, ':');
            if (p != NULL) {
                char* value_start = p + 1;

                /* Calculate the key length */
                size_t key_len = p - p1;

                /* Skip if key is empty */
                if (key_len == 0) {
                    continue;
                }

                /* Calculate value length safely */
                size_t value_len = strlen(value_start);

                /* Process the value */
                if (value_len > 0) {
                    zend_uchar type;
                    zend_long  lval;
                    double     dval;

                    type = is_numeric_string(value_start, value_len, &lval, &dval, 0);
                    switch (type) {
                        case IS_LONG:
                            add_assoc_long_ex(z_ret, p1, key_len, lval);
                            break;
                        case IS_DOUBLE:
                            add_assoc_double_ex(z_ret, p1, key_len, dval);
                            break;
                        default:
                            add_assoc_stringl_ex(z_ret, p1, key_len, value_start, value_len);
                            break;
                    }
                } else {
                    /* Empty value - add as empty string */
                    add_assoc_stringl_ex(z_ret, p1, key_len, "", 0);
                }
            }
        } while ((p1 = php_strtok_r(NULL, _NL, &s1)) != NULL);
    }

    /* Free the safe copy */
    efree(safe_response);
}


/* Helper function to process INFO command sections into argument arrays */
static int process_info_sections(
    zval* args, int start_idx, int count, uintptr_t** cmd_args, unsigned long** cmd_args_len) {
    if (count == 0) {
        *cmd_args     = NULL;
        *cmd_args_len = NULL;
        return 0;
    }

    /* Allocate argument arrays */
    *cmd_args     = (uintptr_t*) emalloc(count * sizeof(uintptr_t));
    *cmd_args_len = (unsigned long*) emalloc(count * sizeof(unsigned long));

    /* Process each section argument */
    for (int i = 0; i < count; i++) {
        zval* section = &args[start_idx + i];

        zend_string* sec   = zval_get_string((zval*) section);
        char*        str   = estrndup(ZSTR_VAL(sec), ZSTR_LEN(sec));
        size_t       len   = ZSTR_LEN(sec);
        (*cmd_args)[i]     = (uintptr_t) str;
        (*cmd_args_len)[i] = len;
        zend_string_release(sec);
    }

    return count;
}

/* Custom result processor for INFO command - handles both single and multi-node responses */
int process_info_result(CommandResponse* response, void* output, zval* return_value) {
    if (!response) {
        return 0;
    }

    if (response->response_type == String) {
        /* Single node response - parse INFO string into associative array */
        valkey_glide_parse_info_response(
            response->string_value, response->string_value_len, return_value);
        return 1;
    } else {
        /* Multi-node response (cluster with AllNodes routing) */
        zval temp_result;
        command_response_to_zval(response, &temp_result, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        /* AllNodes routing - array of responses */
        array_init(return_value);
        HashTable* ht = Z_ARRVAL(temp_result);
        zval*      entry;
        int        index        = 0;
        char*      current_node = NULL;
        int        current_port = 0;

        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (Z_TYPE_P(entry) == IS_STRING) {
                zval parsed_info;
                valkey_glide_parse_info_response(
                    Z_STRVAL_P(entry), Z_STRLEN_P(entry), &parsed_info);
                add_next_index_zval(return_value, &parsed_info);
            }
        }
        ZEND_HASH_FOREACH_END();
        zval_ptr_dtor(&temp_result);
        return 1;
    }
    return 0;
}

/* Execute an INFO command using the Valkey Glide client - WITH BATCH SUPPORT */
int execute_info_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args         = NULL;
    int                  args_count   = 0;
    CommandResult*       cmd_result   = NULL;
    uintptr_t*           cmd_args     = NULL;
    unsigned long*       cmd_args_len = NULL;

    zend_bool is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* INFO cannot be used in batch mode with cluster */
        return 0;
    }

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
        FAILURE) {
        return 0;
    }

    /* Check if we're in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, buffer the command instead of executing it */
        int section_count = is_cluster ? (args_count - 1) : args_count;
        int start_idx     = is_cluster ? 1 : 0;

        /* Process sections using helper function */
        int processed_args =
            process_info_sections(args, start_idx, section_count, &cmd_args, &cmd_args_len);

        if (processed_args < 0) {
            /* Error in processing */
            return 0;
        }


        /* Buffer the command for batch execution */
        int buffer_result = buffer_command_for_batch(valkey_glide,
                                                     Info,
                                                     cmd_args,
                                                     cmd_args_len,
                                                     processed_args,
                                                     NULL, /* result_ptr */
                                                     process_info_result);

        /* Free the argument arrays */
        for (int i = 0; i < section_count; i++) {
            efree((char*) (cmd_args[i]));
        }

        if (cmd_args)
            efree(cmd_args);
        if (cmd_args_len)
            efree(cmd_args_len);

        if (buffer_result) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    /* Non-batch mode execution */
    if (is_cluster) {
        if (args_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* Process sections (if any) using helper function */
        int section_count = args_count - 1; /* Subtract 1 for route parameter */
        int processed_args =
            process_info_sections(args, 1, section_count, &cmd_args, &cmd_args_len);

        if (processed_args < 0) {
            /* Error in processing */
            return 0;
        }

        /* Execute the command with routing */
        cmd_result = execute_command_with_route(valkey_glide->glide_client,
                                                Info,
                                                processed_args,
                                                cmd_args,
                                                cmd_args_len,
                                                &args[0]); /* Route parameter */

        for (int i = 0; i < section_count; i++) {
            efree((char*) (cmd_args[i]));
        }

        /* Free the argument arrays */
        if (cmd_args)
            efree(cmd_args);
        if (cmd_args_len)
            efree(cmd_args_len);

    } else {
        /* Process sections using helper function */
        int processed_args = process_info_sections(args, 0, args_count, &cmd_args, &cmd_args_len);

        if (processed_args < 0) {
            /* Error in processing */
            return 0;
        }

        /* Execute the command */
        cmd_result = execute_command(
            valkey_glide->glide_client, Info, processed_args, cmd_args, cmd_args_len);

        for (int i = 0; i < args_count; i++) {
            efree((char*) (cmd_args[i]));
        }
        /* Free the argument arrays */
        if (cmd_args)
            efree(cmd_args);
        if (cmd_args_len)
            efree(cmd_args_len);
    }

    /* Process the result */
    if (cmd_result && cmd_result->response != NULL) {
        process_info_result(cmd_result->response, NULL, return_value);
        free_command_result(cmd_result);
        return 1;
    }

    if (cmd_result) {
        free_command_result(cmd_result);
    }
    return 0;
}

/* Execute a GET command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_get_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Get;
    args.key                 = key;
    args.key_len             = key_len;

    /* Allocate string result processor on heap for batch support */


    if (execute_core_command(valkey_glide, &args, NULL, process_core_string_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            /* Note: output will be freed later in process_core_string_result */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Execute a RANDOMKEY command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_randomkey_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    int                  result     = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* RANDOMKEY cannot be used in batch mode */
        return 0;
    }

    if (is_cluster) {
        /* Parse parameters for cluster - route parameter required */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need the route parameter */
            return 0;
        }

        /* Execute the command with the route bytes */
        CommandResult* cmd_result = execute_command_with_route(
            valkey_glide->glide_client, RandomKey, 0, NULL, NULL, &args[0]);

        /* Use the generic handler to process the result */
        char*  response     = NULL;
        size_t response_len = 0;
        result              = handle_string_response(cmd_result, &response, &response_len);
        if (result == 1) {
            if (response != NULL) {
                ZVAL_STRINGL(return_value, response, response_len);
                efree(response);
                return 1;
            } else {
                ZVAL_NULL(return_value);
                return 1;
            }
            return 1;
        }
    } else {
        /* Non-cluster case - parse parameters as before */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }

        /* Execute using core framework */
        core_command_args_t core_args = {0};
        core_args.glide_client        = valkey_glide->glide_client;
        core_args.cmd_type            = RandomKey;


        if (execute_core_command(
                valkey_glide, &core_args, NULL, process_core_string_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                /* Note: output will be freed later in process_core_string_result */
                ZVAL_COPY(return_value, object);
                return 1;
            }

            return 1;
        }
    }

    /* Process the result */
    if (result == 1) {
        return 1;
    }

    /* Error */
    return 0;
}

/* Execute a GETBIT command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_getbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            offset;


    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Osl", &object, ce, &key, &key_len, &offset) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = GetBit;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add offset argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = offset;
    args.arg_count                   = 1;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Execute a SETBIT command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_setbit_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            offset;
    zend_bool            value;


    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oslb", &object, ce, &key, &key_len, &offset, &value) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = SetBit;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add offset argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = offset;

    /* Add value argument (0 or 1) */
    args.args[1].type                = CORE_ARG_TYPE_LONG;
    args.args[1].data.long_arg.value = value ? 1 : 0;
    args.arg_count                   = 2;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }


        return 1;
    }

    return 0;
}

/* Helper function to execute del_command with arrays - MIGRATED TO CORE FRAMEWORK */
int execute_del_array(const void* glide_client,
                      HashTable*  keys_hash,
                      long*       output_value,
                      zval*       return_value) {
    /* Convert HashTable to zval array for core framework */
    if (!glide_client || !keys_hash || zend_hash_num_elements(keys_hash) <= 0) {
        return 0;
    }

    /* Create temporary zval array from HashTable */
    zval keys_array;
    ZVAL_ARR(&keys_array, keys_hash);
    /* Bump refcount so it stays alive while you pass it down */
    GC_TRY_ADDREF(keys_hash);

    /* Use core framework with converted array */
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Del;

    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = &keys_array;
    args.args[0].data.array_arg.count = zend_hash_num_elements(keys_hash);
    args.arg_count                    = 1;

    /* Use direct command execution for legacy function */
    uintptr_t*     cmd_args          = NULL;
    unsigned long* cmd_args_len      = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            result            = 0;

    arg_count =
        prepare_core_args(&args, &cmd_args, &cmd_args_len, &allocated_strings, &allocated_count);
    if (arg_count >= 0) {
        CommandResult* cmd_result =
            execute_command(args.glide_client, args.cmd_type, arg_count, cmd_args, cmd_args_len);
        if (cmd_result) {
            result = process_core_int_result(cmd_result->response, output_value, return_value);
            free_command_result(cmd_result);
        }
    }
    zval_ptr_dtor(&keys_array);
    free_core_args(cmd_args, cmd_args_len, allocated_strings, allocated_count);
    return result;
}

/* Execute a DEL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_del_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 result_value = 0;
    zval*                keys         = NULL;
    int                  keys_count   = 0;
    int                  result       = 0;

    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &keys, &keys_count) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (keys_count == 1 && Z_TYPE(keys[0]) == IS_ARRAY) {
        result = execute_del_array(
            valkey_glide->glide_client, Z_ARRVAL(keys[0]), &result_value, return_value);
    } else {
        result =
            execute_multi_key_command(valkey_glide, Del, keys, keys_count, object, return_value);
    }

    if (result) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Helper function to execute unlink_command with arrays - MIGRATED TO CORE FRAMEWORK */
int execute_unlink_array(const void* glide_client,
                         HashTable*  keys_hash,
                         long*       output_value,
                         zval*       return_value) {
    /* Convert HashTable to zval array for core framework */
    if (!glide_client || !keys_hash || zend_hash_num_elements(keys_hash) <= 0) {
        return 0;
    }

    /* Create temporary zval array from HashTable */
    zval keys_array;
    ZVAL_ARR(&keys_array, keys_hash);
    GC_TRY_ADDREF(keys_hash);  // keep alive

    /* Use core framework with converted array */
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Unlink;

    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = &keys_array;
    args.args[0].data.array_arg.count = zend_hash_num_elements(keys_hash);
    args.arg_count                    = 1;

    /* Use direct command execution for legacy function */
    uintptr_t*     cmd_args          = NULL;
    unsigned long* cmd_args_len      = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;
    int            arg_count         = 0;
    int            result            = 0;

    arg_count =
        prepare_core_args(&args, &cmd_args, &cmd_args_len, &allocated_strings, &allocated_count);
    if (arg_count >= 0) {
        CommandResult* cmd_result =
            execute_command(args.glide_client, args.cmd_type, arg_count, cmd_args, cmd_args_len);
        if (cmd_result) {
            result = process_core_int_result(cmd_result->response, output_value, return_value);
            free_command_result(cmd_result);
        }
    }
    zval_ptr_dtor(&keys_array);
    free_core_args(cmd_args, cmd_args_len, allocated_strings, allocated_count);
    return result;
}

/* Execute a STRLEN command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_strlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;


    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Strlen;
    args.key                 = key;
    args.key_len             = key_len;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }


        return 1;
    }

    return 0;
}

/* Execute a SETRANGE command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_setrange_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    zend_long            offset;


    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osls", &object, ce, &key, &key_len, &offset, &val, &val_len) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = SetRange;
    args.key                 = key;
    args.key_len             = key_len;

    /* Add offset argument */
    args.args[0].type                = CORE_ARG_TYPE_LONG;
    args.args[0].data.long_arg.value = offset;

    /* Add value argument */
    args.args[1].type                  = CORE_ARG_TYPE_STRING;
    args.args[1].data.string_arg.value = val;
    args.args[1].data.string_arg.len   = val_len;
    args.arg_count                     = 2;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }


        return 1;
    }

    return 0;
}

/* Process list elements from LMPOP/BLMPOP response */
static void process_list_elements(struct CommandResponse* elements_resp, zval* elements_array) {
    /* For lists, elements are just values */
    for (int i = 0; i < elements_resp->array_value_len; i++) {
        struct CommandResponse* element = &elements_resp->array_value[i];
        if (element->response_type == String) {
            add_next_index_stringl(
                elements_array, element->string_value, element->string_value_len);
        }
    }
}

/* Process sorted set elements from ZMPOP/BZMPOP response */
static void process_sorted_set_elements(struct CommandResponse* elements_resp,
                                        zval*                   elements_array) {
    /* For sorted sets, elements are pairs of member and score */
    for (int i = 0; i < elements_resp->array_value_len; i += 2) {
        if (i + 1 < elements_resp->array_value_len) {
            struct CommandResponse* member = &elements_resp->array_value[i];
            struct CommandResponse* score  = &elements_resp->array_value[i + 1];

            if (member->response_type == String && score->response_type == String) {
                /* Convert score string to double */
                double score_val = atof(score->string_value);

                /* Add member => score pair to elements array */
                add_assoc_double_ex(
                    elements_array, member->string_value, member->string_value_len, score_val);
            }
        }
    }
}

/* Execute a TTL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_ttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;


    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = TTL;
    args.key                 = key;
    args.key_len             = key_len;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Execute a PTTL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_pttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;


    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Execute using core framework */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = PTTL;
    args.key                 = key;
    args.key_len             = key_len;

    if (execute_core_command(valkey_glide, &args, NULL, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }

        return 1;
    }

    return 0;
}

/* Execute a GETSET command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_getset_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *val = NULL;
    size_t               key_len, val_len;
    char*                response     = NULL;
    size_t               response_len = 0;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss", &object, ce, &key, &key_len, &val, &val_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Create a zval array for the GET option */
    zval z_opts;
    array_init(&z_opts);
    add_next_index_string(&z_opts, "GET");

    /* Execute the SET command with GET option using the Glide client */
    int result = execute_set_command_internal(valkey_glide,
                                              key,
                                              key_len,
                                              val,
                                              val_len,
                                              0,       /* No expiry */
                                              &z_opts, /* Use GET option */
                                              &response,
                                              &response_len,
                                              return_value);

    /* Free the zval array */
    zval_dtor(&z_opts);

    /* Check for batch mode after successful execution */
    if (result && valkey_glide->is_in_batch_mode) {
        /* Clean up response if it was allocated */
        if (response) {
            efree(response);
        }
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }

    /* Process the result */
    if ((result == 1 || result == 2) && response != NULL) {
        /* Return the old value */
        //  ZVAL_STRINGL(return_value, response, response_len);
        efree(response);
        return 1;
    } else if (result == 0 || (result == 2 && response == NULL)) {
        /* Key didn't exist */
        //      ZVAL_FALSE(return_value);
        return 1;
    } else {
        /* Error */
        return 0;
    }
}

/* Check if a string value exists in an array */
static int array_has_string_value(HashTable* ht, const char* value, size_t value_len) {
    zval* entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (Z_TYPE_P(entry) == IS_STRING && Z_STRLEN_P(entry) == value_len &&
            memcmp(Z_STRVAL_P(entry), value, value_len) == 0) {
            return 1;
        }
    }
    ZEND_HASH_FOREACH_END();
    return 0;
}

static int process_lcs_result(CommandResponse* response, void* output, zval* return_value) {
    int ret_val = 0;
    /* Force Map handling if IDX option was requested, regardless of the response type */
    switch (response->response_type) {
        case String:
            /* If no options were specified, LCS returns the longest common substring as
             * a string */
            command_response_to_zval(
                response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
            ret_val = 1;
            break;

        case Int:
            /* If LEN option was specified, LCS returns the length as an integer */
            ZVAL_LONG(return_value, response->int_value);
            ret_val = 1;
            break;

        case Map:
            ret_val = command_response_to_zval(
                response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
            break;


        default:
            VALKEY_LOG_WARN("command_response", "Unexpected default response type");
            /* Unsupported response type */
            ZVAL_FALSE(return_value);
            ret_val = 0;
            break;
    }
    return ret_val;
}

/* Execute an LCS command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_lcs_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key1 = NULL, *key2 = NULL;
    size_t               key1_len, key2_len;
    zval*                options = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Oss|a", &object, ce, &key1, &key1_len, &key2, &key2_len, &options) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Prepare command arguments */
    unsigned long arg_count = 2; /* key1 + key2 */
    uintptr_t     args[7];       /* Maximum 7 arguments: key1, key2, LEN, IDX, MINMATCHLEN, value,
                                    WITHMATCHLEN */
    unsigned long args_len[7];

    /* First argument: key1 */
    args[0]     = (uintptr_t) key1;
    args_len[0] = key1_len;

    /* Second argument: key2 */
    args[1]     = (uintptr_t) key2;
    args_len[1] = key2_len;

    /* Parse options and flags */
    int  has_len           = 0;
    int  has_idx           = 0;
    int  has_minmatchlen   = 0;
    long minmatchlen_value = 0;
    int  has_withmatchlen  = 0;

    /* Add options if provided */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        /* Get option values from associative array */
        zval* z_len = zend_hash_str_find(Z_ARRVAL_P(options), "len", sizeof("len") - 1);
        zval* z_idx = zend_hash_str_find(Z_ARRVAL_P(options), "idx", sizeof("idx") - 1);
        zval* z_minmatchlen =
            zend_hash_str_find(Z_ARRVAL_P(options), "minmatchlen", sizeof("minmatchlen") - 1);
        zval* z_withmatchlen =
            zend_hash_str_find(Z_ARRVAL_P(options), "withmatchlen", sizeof("withmatchlen") - 1);
        /* Check associative array values */
        if (z_len && Z_TYPE_P(z_len) == IS_TRUE) {
            has_len = 1;
        }

        if (z_idx && Z_TYPE_P(z_idx) == IS_TRUE) {
            has_idx = 1;
        }

        if (z_minmatchlen && Z_TYPE_P(z_minmatchlen) == IS_LONG) {
            has_minmatchlen   = 1;
            minmatchlen_value = Z_LVAL_P(z_minmatchlen);
        }

        if (z_withmatchlen && Z_TYPE_P(z_withmatchlen) == IS_TRUE) {
            has_withmatchlen = 1;
        }

        /* ALSO check for string values in indexed array */
        if (!has_len && array_has_string_value(Z_ARRVAL_P(options), "len", 3)) {
            has_len = 1;
        }

        if (!has_idx && array_has_string_value(Z_ARRVAL_P(options), "idx", 3)) {
            has_idx = 1;
        }

        if (!has_withmatchlen && array_has_string_value(Z_ARRVAL_P(options), "withmatchlen", 12)) {
            has_withmatchlen = 1;
        }
    }

    /* Add LEN option if specified */
    if (has_len) {
        args[arg_count]     = (uintptr_t) "LEN";
        args_len[arg_count] = 3;
        arg_count++;
    }

    /* Add IDX option if specified */
    if (has_idx) {
        args[arg_count]     = (uintptr_t) "IDX";
        args_len[arg_count] = 3;
        arg_count++;
    }

    char minmatchlen_str[32];

    /* Add MINMATCHLEN option if specified */
    if (has_minmatchlen) {
        args[arg_count]     = (uintptr_t) "MINMATCHLEN";
        args_len[arg_count] = 11;
        arg_count++;

        /* Add the minmatchlen value */
        size_t minmatchlen_len;
        minmatchlen_len =
            snprintf(minmatchlen_str, sizeof(minmatchlen_str), "%ld", minmatchlen_value);
        minmatchlen_str[minmatchlen_len] = '\0';
        args[arg_count]                  = (uintptr_t) minmatchlen_str;
        args_len[arg_count]              = minmatchlen_len;
        arg_count++;
    }

    /* Add WITHMATCHLEN option if specified */
    if (has_withmatchlen) {
        args[arg_count]     = (uintptr_t) "WITHMATCHLEN";
        args_len[arg_count] = 12;
        arg_count++;
    }

    /* Execute the command */
    CommandResult* cmd_result = NULL;
    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* Create batch-compatible processor wrapper */
        int res = buffer_command_for_batch(
            valkey_glide, LCS, args, args_len, arg_count, NULL, process_lcs_result);
    } else {
        cmd_result = execute_command(valkey_glide->glide_client,
                                     LCS,       /* command type */
                                     arg_count, /* number of arguments */
                                     args,      /* arguments */
                                     args_len   /* argument lengths */
        );
    }


    /* Process the result based on the response type */
    int ret_val = 0;
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        ret_val = 1;
    } else {
        /* Check if the command was successful */
        if (!cmd_result) {
            return 0;
        }
        if (cmd_result->response) {
            ret_val = process_lcs_result(cmd_result->response, NULL, return_value);
        }
    }

    /* Free the result */
    free_command_result(cmd_result);

    return ret_val;
}


/* ============================================================================
 * Password Update Functions
 * ========================================================================= */

/**
 * Execute update_connection_password command
 *
 * @param object The ValkeyGlide or ValkeyGlideCluster object
 * @param password The new password (NULL to clear)
 * @param password_len Length of password
 * @param immediate_auth Whether to re-authenticate immediately
 * @param return_value The return value zval
 * @param ce The class entry
 */
void execute_update_connection_password(zval*             object,
                                        const char*       password,
                                        size_t            password_len,
                                        bool              immediate_auth,
                                        zval*             return_value,
                                        zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zend_bool            is_cluster = (ce != NULL && ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        VALKEY_LOG_ERROR("update_connection_password",
                         "Invalid client object or client connection handle is NULL");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "Invalid client object or client connection handle is NULL",
                             0);
        return;
    }

    /* Call FFI function */
    CommandResult* result =
        update_connection_password(valkey_glide->glide_client,
                                   0, /* request_id - using 0 for synchronous call */
                                   password,
                                   immediate_auth);

    /* Check result */
    if (!result) {
        VALKEY_LOG_ERROR("update_connection_password", "FFI call returned NULL result");
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to update connection password", 0);
        return;
    }

    /* Check for command error */
    if (result->command_error) {
        VALKEY_LOG_ERROR_FMT("update_connection_password",
                             "Command execution failed with error: %s",
                             result->command_error->command_error_message
                                 ? result->command_error->command_error_message
                                 : "Unknown error");
        zend_throw_exception(
            get_valkey_glide_exception_ce(), result->command_error->command_error_message, 0);
        free_command_result(result);
        return;
    }

    if (!result->response) {
        VALKEY_LOG_ERROR("update_connection_password", "No response received from server");
        free_command_result(result);
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "No response received from server", 0);
        return;
    }

    /* Process response */
    if (result->response->response_type == Ok) {
        ZVAL_STRING(return_value, "OK");
    } else if (result->response->response_type == Error) {
        VALKEY_LOG_ERROR_FMT(
            "update_connection_password",
            "Server returned error: %s",
            result->response->string_value ? result->response->string_value : "Unknown error");
        zend_throw_exception(
            get_valkey_glide_exception_ce(),
            result->response->string_value ? result->response->string_value : "Unknown error",
            0);
    } else {
        VALKEY_LOG_ERROR_FMT("update_connection_password",
                             "Unexpected response type: %d",
                             result->response->response_type);
        zend_throw_exception(get_valkey_glide_exception_ce(), "Unexpected response from server", 0);
    }

    free_command_result(result);
}
