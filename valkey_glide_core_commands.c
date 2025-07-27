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
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_list_common.h"

extern zend_class_entry* ce;
extern zend_class_entry* get_valkey_glide_exception_ce();

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* Create a connection request in protobuf format */
static uint8_t* create_connection_request(const char*                          host,
                                          int                                  port,
                                          const char*                          user,
                                          const char*                          pass,
                                          size_t*                              len,
                                          valkey_glide_client_configuration_t* config,
                                          bool                                 is_cluster) {
    /* Create a connection request */
    ConnectionRequest__ConnectionRequest conn_req = CONNECTION_REQUEST__CONNECTION_REQUEST__INIT;

    /* Set up the node address */
    ConnectionRequest__NodeAddress node_addr = CONNECTION_REQUEST__NODE_ADDRESS__INIT;
    node_addr.host                           = (char*) host;
    node_addr.port                           = port;

    /* Add the node address to the connection request */
    ConnectionRequest__NodeAddress* addresses[1] = {&node_addr};
    conn_req.n_addresses                         = 1;
    conn_req.addresses                           = addresses;

    /* Set up authentication if provided */
    ConnectionRequest__AuthenticationInfo auth_info = CONNECTION_REQUEST__AUTHENTICATION_INFO__INIT;
    if (user && pass) {
        auth_info.username           = (char*) user;
        auth_info.password           = (char*) pass;
        conn_req.authentication_info = &auth_info;
    }

    /* Set values from configuration */
    conn_req.tls_mode             = config->base.use_tls ? CONNECTION_REQUEST__TLS_MODE__SecureTls
                                                         : CONNECTION_REQUEST__TLS_MODE__NoTls;
    conn_req.cluster_mode_enabled = is_cluster;
    conn_req.request_timeout      = config->base.request_timeout > 0 ? config->base.request_timeout
                                                                     : 5000; /* Default 5 seconds */

    /* Map read_from configuration */
    if (config->base.read_from == VALKEY_GLIDE_READ_FROM_PREFER_REPLICA) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__PreferReplica;
    } else if (config->base.read_from == VALKEY_GLIDE_READ_FROM_AZ_AFFINITY) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__AZAffinity;
    } else if (config->base.read_from == VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY) {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__AZAffinityReplicasAndPrimary;
    } else {
        conn_req.read_from = CONNECTION_REQUEST__READ_FROM__Primary;
    }

    /* Set database ID for standalone clients */
    if (!is_cluster && config->database_id >= 0) {
        conn_req.database_id = config->database_id;
    } else {
        conn_req.database_id = 0;
    }

    conn_req.protocol = CONNECTION_REQUEST__PROTOCOL_VERSION__RESP3;

    /* Set client name */
    conn_req.client_name = config->base.client_name ? config->base.client_name : "valkey-glide-php";

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

/* Create a Valkey Glide client */
const ConnectionResponse* create_glide_client(valkey_glide_client_configuration_t* config,
                                              bool                                 is_cluster) {
    /* Create a connection request using first address or default */
    size_t      len;
    const char* host     = "localhost";
    int         port     = 6379;
    const char* username = NULL;
    const char* password = NULL;

    /* Use first address if available */
    if (config->base.addresses && config->base.addresses_count > 0) {
        host = config->base.addresses[0].host;
        port = config->base.addresses[0].port;
    }

    /* Use credentials if available */
    if (config->base.credentials) {
        username = config->base.credentials->username;
        password = config->base.credentials->password;
    }

    uint8_t* request_bytes =
        create_connection_request(host, port, username, password, &len, config, is_cluster);

    if (!request_bytes) {
        return NULL;
    }

    /* Set up client type for synchronous operation */
    ClientType client_type;
    client_type.tag = SyncClient;

    /* Create the client */
    const ConnectionResponse* conn_resp =
        create_client(request_bytes, len, &client_type, NULL /* No PubSub callback */
        );

    /* Free the request bytes as they're no longer needed */
    efree(request_bytes);

    /* Check if there was an error */
    if (conn_resp->connection_error_message) {
        printf("Error creating client: %s\n", conn_resp->connection_error_message);
    }

    return conn_resp;
}

/* Custom result processor for SET commands with GET option support */
struct set_result_data {
    char**  old_val;
    size_t* old_val_len;
    int     has_get;
};

static int process_set_result(CommandResult* result, void* output) {
    struct set_result_data* data = (struct set_result_data*) output;

    if (!result || !result->response) {
        return 0;
    }

    switch (result->response->response_type) {
        case Ok:
            return 1; /* Success */
        case Null:
            return 0; /* Not set (NX/XX condition not met) */
        case String:
            /* GET option returned a value */
            if (data->has_get && data->old_val && data->old_val_len &&
                result->response->string_value) {
                *data->old_val = emalloc(result->response->string_value_len + 1);
                if (*data->old_val) {
                    memcpy(*data->old_val,
                           result->response->string_value,
                           result->response->string_value_len);
                    (*data->old_val)[result->response->string_value_len] = '\0';
                    *data->old_val_len = result->response->string_value_len;
                }
            }
            return 2; /* GET option returned a value */
        default:
            return 0; /* Error */
    }
}

/* Custom result processor for PING command */
static int process_ping_result(CommandResult* result, void* output) {
    struct {
        char**  result;
        size_t* result_len;
    }* string_output = output;

    if (!result || !result->response || !string_output) {
        return 0;
    }
    if (result->response->response_type == Ok) {
        /* PONG response with no message */
        *string_output->result     = estrdup("PONG");
        *string_output->result_len = 4;
        return 1;
    } else if (result->response->response_type == String) {
        /* PING with message - echo the message back */
        if (result->response->string_value_len == 0) {
            *string_output->result = emalloc(1);
            if (*string_output->result) {
                (*string_output->result)[0] = '\0';
            }
            *string_output->result_len = 0;
        } else {
            *string_output->result = emalloc(result->response->string_value_len + 1);
            if (*string_output->result) {
                memcpy(*string_output->result,
                       result->response->string_value,
                       result->response->string_value_len);
                (*string_output->result)[result->response->string_value_len] = '\0';
            }
            *string_output->result_len = result->response->string_value_len;
        }
        return *string_output->result ? 1 : 0;
    } else if (result->response->response_type == Null) {
        *string_output->result     = NULL;
        *string_output->result_len = 0;
        return 0;
    }

    return 0;
}

/* These functions are now defined in command_response.c */

/* Execute a BITCOUNT command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_bitcount_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    zend_long            start = 0, end = -1;
    zend_bool            bybit = 0;
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    /* Add source keys as remaining arguments */
    for (int i = 0; i < keys_count && i < 7; i++) {
        args.args[i + 1].type                  = CORE_ARG_TYPE_STRING;
        args.args[i + 1].data.string_arg.value = Z_STRVAL(keys[i]);
        args.args[i + 1].data.string_arg.len   = Z_STRLEN(keys[i]);
    }
    args.arg_count = 1 + keys_count; /* operation + source keys */

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    int result = execute_set_command_internal(valkey_glide->glide_client,
                                              key,
                                              key_len,
                                              val,
                                              val_len,
                                              expire_int,
                                              z_set_opts,
                                              &old_val,
                                              &old_val_len);

    /* Free the allocated string if needed */
    if (free_val) {
        efree(val);
    }

    /* Process the result */
    switch (result) {
        case 1: /* Success */
            ZVAL_TRUE(return_value);
            return 1;
        case 0: /* Not set (NX/XX/IFEQ condition not met) */
            ZVAL_FALSE(return_value);
            return 1;
        case 2: /* GET option returned a value */
            /* If GET option was used and old value was returned */
            if (old_val != NULL) {
                /* Return the old value */
                ZVAL_STRINGL(return_value, old_val, old_val_len);
                efree(old_val); /* Free the allocated old value */
                return 1;
            }
            /* Fallback to returning TRUE when GET is used but handling fails */
            ZVAL_TRUE(return_value);
            return 1;
        default: /* Error */
            ZVAL_FALSE(return_value);
            return 0;
    }
    return 0; /* Should not reach here, but just in case */
}

/* Execute a SET command using the Valkey Glide client - INTERNAL HELPER FUNCTION */
int execute_set_command_internal(const void* glide_client,
                                 const char* key,
                                 size_t      key_len,
                                 const char* val,
                                 size_t      val_len,
                                 long        expire,
                                 zval*       opts,
                                 char**      old_val,
                                 size_t*     old_val_len) {
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
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
    struct set_result_data result_data = {old_val, old_val_len, args.options.get_old_value};

    return execute_core_command(&args, &result_data, process_set_result);
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
        valkey_glide->glide_client, key, key_len, val, val_len, expire, NULL, NULL, NULL);

    if (result == 1) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
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
        valkey_glide->glide_client, key, key_len, val, val_len, 0, &options, NULL, NULL);

    /* Clean up options array */
    zval_dtor(&options);

    if (result == 1) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
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
        valkey_glide->glide_client, key, key_len, val, val_len, 0, &options, NULL, NULL);

    /* Clean up options array */
    zval_dtor(&options);

    if (result == 1) {
        ZVAL_TRUE(return_value);
        return 1;
    } else {
        ZVAL_FALSE(return_value);
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

    /* Use string result processor */
    struct {
        char**  result;
        size_t* result_len;
    } output = {&response, &response_len};

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, &output, process_core_string_result)) {
        if (response != NULL) {
            ZVAL_STRINGL(return_value, response, response_len);
            efree(response);
            return 1;
        }
    }

    /* Error or empty response */
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
        char**  result;
        size_t* result_len;
    } output = {&response, &response_len};

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, &output, process_ping_result)) {
        if (response != NULL) {
            /* Check if message was provided */
            int has_message = (msg != NULL && msg_len > 0);

            /* If no message was provided and response is "PONG", return true */
            if (!has_message && response_len == 4 && strncmp(response, "PONG", 4) == 0) {
                efree(response);
                ZVAL_TRUE(return_value);
                return 1;
            }
            /* Otherwise, return the actual response string */
            ZVAL_STRINGL(return_value, response, response_len);
            efree(response);
            return 1;
        } else {
            /* Success but no response (should return TRUE for PONG) */
            ZVAL_TRUE(return_value);
            return 1;
        }
    }

    /* Error or empty response */
    return 0;
}

#define _NL "\r\n"

static void valkey_glide_parse_info_response(char* response, zval* z_ret) {
    char *p1, *s1 = NULL;

    ZVAL_FALSE(z_ret);
    if ((p1 = php_strtok_r(response, _NL, &s1)) != NULL) {
        array_init(z_ret);
        do {
            if (*p1 == '#')
                continue;
            char*      p;
            zend_uchar type;
            zend_long  lval;
            double     dval;
            if ((p = strchr(p1, ':')) != NULL) {
                type = is_numeric_string(p + 1, strlen(p + 1), &lval, &dval, 0);
                switch (type) {
                    case IS_LONG:
                        add_assoc_long_ex(z_ret, p1, p - p1, lval);
                        break;
                    case IS_DOUBLE:
                        add_assoc_double_ex(z_ret, p1, p - p1, dval);
                        break;
                    default:
                        add_assoc_string_ex(z_ret, p1, p - p1, p + 1);
                }
            } else {
                add_next_index_string(z_ret, p1);
            }
        } while ((p1 = php_strtok_r(NULL, _NL, &s1)) != NULL);
    }
}
static int parse_info_multi_node_response(CommandResult* cmd_result, zval* return_value) {
    int result = 0;
    if (cmd_result && cmd_result->response) {
        zval temp_result;
        if (command_response_to_zval(
                cmd_result->response, &temp_result, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false)) {
            if (Z_TYPE(temp_result) == IS_ARRAY) {
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
                        valkey_glide_parse_info_response(Z_STRVAL_P(entry), &parsed_info);
                        add_next_index_zval(return_value, &parsed_info);
                    }
                }
                ZEND_HASH_FOREACH_END();
                result = 1;
            } else if (Z_TYPE(temp_result) == IS_STRING) {
                /* Single node response */
                valkey_glide_parse_info_response(Z_STRVAL(temp_result), return_value);
                result = 1;
            }
            zval_dtor(&temp_result);
        }
    }
    free_command_result(cmd_result);
    return result;
}
/* Execute an INFO command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_info_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args         = NULL;
    int                  args_count   = 0;
    char*                response     = NULL;
    size_t               response_len = 0;
    int                  result       = 0;
    zend_bool            is_cluster   = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
        FAILURE) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = Info;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        if (args_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* If no sections are specified, call with NULL section */
        if (args_count == 1) {
            CommandResult* cmd_result;

            /* Execute the command with the route bytes */
            cmd_result = execute_command_with_route(
                valkey_glide->glide_client, Info, 0, NULL, NULL, &args[0]);

            /* Use command_response_to_zval to handle both single and array responses */
            result = parse_info_multi_node_response(cmd_result, return_value);
            /* If we processed the result above, return early */
            if (result == 1) {
                return 1;
            }
        } else {
            /* One or more sections specified */
            unsigned long  arg_count = args_count - 1; /* Subtract 1 for route parameter */
            uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
            unsigned long* cmd_args_len =
                (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

            if (!cmd_args || !cmd_args_len) {
                if (cmd_args)
                    efree(cmd_args);
                if (cmd_args_len)
                    efree(cmd_args_len);

                return 0;
            }

            /* Process each section argument (start from index 1, after route) */
            for (int i = 1; i < args_count; i++) {
                zval* section = &args[i];
                int   arg_idx = i - 1;

                /* Check if the section is a string */
                if (Z_TYPE_P(section) != IS_STRING) {
                    /* Convert to string if needed */
                    zval temp;
                    ZVAL_COPY(&temp, section);
                    convert_to_string(&temp);

                    cmd_args[arg_idx]     = (uintptr_t) Z_STRVAL(temp);
                    cmd_args_len[arg_idx] = Z_STRLEN(temp);

                    /* Free the temporary zval */
                    zval_dtor(&temp);
                } else {
                    /* It's already a string */
                    cmd_args[arg_idx]     = (uintptr_t) Z_STRVAL_P(section);
                    cmd_args_len[arg_idx] = Z_STRLEN_P(section);
                }
            }

            /* Execute the command with the route information */
            CommandResult* cmd_result = execute_command_with_route(valkey_glide->glide_client,
                                                                   Info,
                                                                   arg_count,
                                                                   cmd_args,
                                                                   cmd_args_len,
                                                                   &args[0]); /* Route parameter */

            /* Free the argument arrays */
            efree(cmd_args);
            efree(cmd_args_len);

            /* Use the generic handler to process the result */
            /* Use command_response_to_zval to handle both single and array responses */
            result = parse_info_multi_node_response(cmd_result, return_value);
            /* If we processed the result above, return early */
            if (result == 1) {
                return 1;
            }
        }
    } else {
        /* Non-cluster case - parse parameters as before */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        /* Handle different cases based on number of arguments */
        if (args_count == 0) {
            /* No sections specified, call with NULL section */
            CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                        Info, /* command type */
                                                        0,    /* number of arguments */
                                                        NULL, /* arguments */
                                                        NULL  /* argument lengths */

            );

            /* Use the generic handler to process the result */
            result = handle_string_response(cmd_result, &response, &response_len);
        } else {
            /* One or more sections specified */
            unsigned long  arg_count = args_count;
            uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
            unsigned long* cmd_args_len =
                (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

            if (!cmd_args || !cmd_args_len) {
                if (cmd_args)
                    efree(cmd_args);
                if (cmd_args_len)
                    efree(cmd_args_len);
                return 0;
            }

            /* Process each section argument */
            for (int i = 0; i < args_count; i++) {
                zval* section = &args[i];

                /* Check if the section is a string */
                if (Z_TYPE_P(section) != IS_STRING) {
                    /* Convert to string if needed */
                    zval temp;
                    ZVAL_COPY(&temp, section);
                    convert_to_string(&temp);

                    cmd_args[i]     = (uintptr_t) Z_STRVAL(temp);
                    cmd_args_len[i] = Z_STRLEN(temp);

                    /* Free the temporary zval */
                    zval_dtor(&temp);
                } else {
                    /* It's already a string */
                    cmd_args[i]     = (uintptr_t) Z_STRVAL_P(section);
                    cmd_args_len[i] = Z_STRLEN_P(section);
                }
            }

            /* Execute the command */
            CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                        Info,        /* command type */
                                                        arg_count,   /* number of arguments */
                                                        cmd_args,    /* arguments */
                                                        cmd_args_len /* argument lengths */

            );

            /* Free the argument arrays */
            efree(cmd_args);
            efree(cmd_args_len);

            /* Use the generic handler to process the result */
            result = handle_string_response(cmd_result, &response, &response_len);
        }
    }

    /* Process the result */
    if (result == 1 && response != NULL) {
        zval z_ret;
        ZVAL_UNDEF(&z_ret);

        /* Parse the INFO response into a zval array */
        valkey_glide_parse_info_response(response, &z_ret);

        /* Free the response string */
        efree(response);

        /* Return the parsed array */
        ZVAL_COPY_VALUE(return_value, &z_ret);
        return 1;
    }

    /* Error or empty response */
    return 0;
}

/* Execute a GET command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_get_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    char*                response     = NULL;
    size_t               response_len = 0;

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

    /* Use string result processor */
    struct {
        char**  result;
        size_t* result_len;
    } output = {&response, &response_len};

    if (execute_core_command(&args, &output, process_core_string_result)) {
        if (response != NULL) {
            ZVAL_STRINGL(return_value, response, response_len);
            efree(response);
            return 1;
        } else {
            ZVAL_FALSE(return_value);
            return 1;
        }
    }

    return 0;
}

/* Execute a RANDOMKEY command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_randomkey_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args         = NULL;
    int                  args_count   = 0;
    char*                response     = NULL;
    size_t               response_len = 0;
    int                  result       = 0;
    zend_bool            is_cluster   = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
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
        result = handle_string_response(cmd_result, &response, &response_len);
    } else {
        /* Non-cluster case - parse parameters as before */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }

        /* Execute using core framework */
        core_command_args_t core_args = {0};
        core_args.glide_client        = valkey_glide->glide_client;
        core_args.cmd_type            = RandomKey;

        /* Use string result processor */
        struct {
            char**  result;
            size_t* result_len;
        } output = {&response, &response_len};

        if (execute_core_command(&core_args, &output, process_core_string_result)) {
            result = 1;
        }
    }

    /* Process the result */
    if (result == 1) {
        if (response != NULL) {
            ZVAL_STRINGL(return_value, response, response_len);
            efree(response);
            return 1;
        } else {
            ZVAL_NULL(return_value);
            return 1;
        }
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
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
        return 1;
    }

    return 0;
}

/* Helper function to execute del_command with arrays - MIGRATED TO CORE FRAMEWORK */
int execute_del_array(const void* glide_client, HashTable* keys_hash, long* output_value) {
    /* Convert HashTable to zval array for core framework */
    if (!glide_client || !keys_hash || zend_hash_num_elements(keys_hash) <= 0) {
        return 0;
    }

    /* Create temporary zval array from HashTable */
    zval keys_array;
    array_init(&keys_array);

    zval* key;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        add_next_index_zval(&keys_array, key);
    }
    ZEND_HASH_FOREACH_END();

    /* Use core framework with converted array */
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Del;

    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = &keys_array;
    args.args[0].data.array_arg.count = zend_hash_num_elements(keys_hash);
    args.arg_count                    = 1;

    int result = execute_core_command(&args, output_value, process_core_int_result);

    return result;
}

/* Execute a DEL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_del_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 result_value = 0;
    zval*                keys         = NULL;
    int                  keys_count   = 0;

    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &keys, &keys_count) ==
        FAILURE) {
        return 0;
    }

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (keys_count == 1 && Z_TYPE(keys[0]) == IS_ARRAY) {
        if (execute_del_array(valkey_glide->glide_client, Z_ARRVAL(keys[0]), &result_value)) {
            ZVAL_LONG(return_value, result_value);
            return 1;
        }
    } else {
        if (execute_multi_key_command(
                valkey_glide->glide_client, Del, keys, keys_count, &result_value)) {
            ZVAL_LONG(return_value, result_value);
            return 1;
        }
    }
    return 0;
}

/* Helper function to execute unlink_command with arrays - MIGRATED TO CORE FRAMEWORK */
int execute_unlink_array(const void* glide_client, HashTable* keys_hash, long* output_value) {
    /* Convert HashTable to zval array for core framework */
    if (!glide_client || !keys_hash || zend_hash_num_elements(keys_hash) <= 0) {
        return 0;
    }

    /* Create temporary zval array from HashTable */
    zval keys_array;
    array_init(&keys_array);

    zval* key;
    ZEND_HASH_FOREACH_VAL(keys_hash, key) {
        add_next_index_zval(&keys_array, key);
    }
    ZEND_HASH_FOREACH_END();

    /* Use core framework with converted array */
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Unlink;

    args.args[0].type                 = CORE_ARG_TYPE_ARRAY;
    args.args[0].data.array_arg.array = &keys_array;
    args.args[0].data.array_arg.count = zend_hash_num_elements(keys_hash);
    args.arg_count                    = 1;

    return execute_core_command(&args, output_value, process_core_int_result);
}

/* Execute a STRLEN command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_strlen_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
        return 1;
    }

    return 0;
}

/* Execute a single-key DEL command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_del_single_key(const void* glide_client,
                           const char* key,
                           size_t      key_len,
                           long*       output_value) {
    core_command_args_t args = {0};
    args.glide_client        = glide_client;
    args.cmd_type            = Del;
    args.key                 = key;
    args.key_len             = key_len;
    /* arg_count = 0 for single key mode */

    return execute_core_command(&args, output_value, process_core_int_result);
}

/* Execute a PTTL command using the Valkey Glide client - UNIFIED IMPLEMENTATION */
int execute_pttl_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;
    long                 result_value;

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

    if (execute_core_command(&args, &result_value, process_core_int_result)) {
        ZVAL_LONG(return_value, result_value);
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
    int result = execute_set_command_internal(valkey_glide->glide_client,
                                              key,
                                              key_len,
                                              val,
                                              val_len,
                                              0,       /* No expiry */
                                              &z_opts, /* Use GET option */
                                              &response,
                                              &response_len);

    /* Free the zval array */
    zval_dtor(&z_opts);

    /* Process the result */
    if ((result == 1 || result == 2) && response != NULL) {
        /* Return the old value */
        ZVAL_STRINGL(return_value, response, response_len);
        efree(response);
        return 1;
    } else if (result == 0 || (result == 2 && response == NULL)) {
        /* Key didn't exist */
        ZVAL_FALSE(return_value);
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
    uintptr_t
        args[7]; /* Maximum 7 arguments: key1, key2, LEN, IDX, MINMATCHLEN, value, WITHMATCHLEN */
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

    /* Add MINMATCHLEN option if specified */
    if (has_minmatchlen) {
        args[arg_count]     = (uintptr_t) "MINMATCHLEN";
        args_len[arg_count] = 11;
        arg_count++;

        /* Add the minmatchlen value */
        size_t minmatchlen_len;
        char*  minmatchlen_str = long_to_string(minmatchlen_value, &minmatchlen_len);
        if (!minmatchlen_str) {
            return 0;
        }
        args[arg_count]     = (uintptr_t) minmatchlen_str;
        args_len[arg_count] = minmatchlen_len;
        arg_count++;
    }

    /* Add WITHMATCHLEN option if specified */
    if (has_withmatchlen) {
        args[arg_count]     = (uintptr_t) "WITHMATCHLEN";
        args_len[arg_count] = 12;
        arg_count++;
    }

    /* Execute the command */
    CommandResult* cmd_result = execute_command(valkey_glide->glide_client,
                                                LCS,       /* command type */
                                                arg_count, /* number of arguments */
                                                args,      /* arguments */
                                                args_len   /* argument lengths */
    );

    /* Check if the command was successful */
    if (!cmd_result) {
        return 0;
    }

    /* Process the result based on the response type */
    int ret_val = 0;
    if (cmd_result->response) {
        /* Force Map handling if IDX option was requested, regardless of the response type */

        {
            switch (cmd_result->response->response_type) {
                case String:
                    /* If no options were specified, LCS returns the longest common substring as a
                     * string */
                    command_response_to_zval(cmd_result->response,
                                             return_value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                             false);
                    ret_val = 1;
                    break;

                case Int:

                    /* If LEN option was specified, LCS returns the length as an integer */
                    ZVAL_LONG(return_value, cmd_result->response->int_value);
                    ret_val = 1;
                    break;

                case Map:
                    /* If IDX option was specified, LCS returns a map structure */
                    ret_val = handle_map_response(cmd_result, return_value);
                    return ret_val; /* handle_map_response already frees cmd_result */

                default:
                    printf("default response\n");
                    /* Unsupported response type */
                    ret_val = 0;
                    break;
            }
        }
    }

    /* Free the result */
    free_command_result(cmd_result);

    return ret_val;
}
