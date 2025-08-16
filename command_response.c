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

#include "command_response.h"

#include "ext/standard/php_var.h"
#include "include/glide/command_request.pb-c.h"
#include "include/glide/response.pb-c.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"

/* Parse a cluster route from a zval parameter */
typedef struct {
    enum {
        ROUTE_TYPE_KEY,       /* Route by key */
        ROUTE_TYPE_HOST_PORT, /* Route by host:port */
        ROUTE_TYPE_SIMPLE     /* Simple route: "randomNode", "allPrimaries", "allNodes" */
    } type;

    union {
        struct {
            char*  key;
            size_t key_len;
            int    key_allocated; /* Flag to indicate if key was dynamically allocated */
        } key_route;

        struct {
            char* host;
            int   port;
        } host_port_route;

        int simple_route_type; /* Using SimpleRoutes_C enum values */
    } data;
} cluster_route_t;

/* Parse a cluster route parameter from a zval */
int parse_cluster_route(zval* route_zval, cluster_route_t* route) {
    /* Default to route by key */
    route->type                         = ROUTE_TYPE_KEY;
    route->data.key_route.key_allocated = 0;

    if (Z_TYPE_P(route_zval) == IS_STRING) {
        char*  route_str = Z_STRVAL_P(route_zval);
        size_t route_len = Z_STRLEN_P(route_zval);

        /* Check for special routing keywords */
        if (route_len == 10 && strncasecmp(route_str, "randomNode", 10) == 0) {
            route->type = ROUTE_TYPE_SIMPLE;
            route->data.simple_route_type =
                COMMAND_REQUEST__SIMPLE_ROUTES__Random; /* SimpleRoutes_Random */
            return 1;
        } else if (route_len == 12 && strncasecmp(route_str, "allPrimaries", 12) == 0) {
            route->type                   = ROUTE_TYPE_SIMPLE;
            route->data.simple_route_type = COMMAND_REQUEST__SIMPLE_ROUTES__AllPrimaries;
            return 1;
        } else if (route_len == 8 && strncasecmp(route_str, "allNodes", 8) == 0) {
            route->type                   = ROUTE_TYPE_SIMPLE;
            route->data.simple_route_type = COMMAND_REQUEST__SIMPLE_ROUTES__AllNodes;
            return 1;
        }

        /* String parameter - use as key */
        route->type                   = ROUTE_TYPE_KEY;
        route->data.key_route.key     = route_str;
        route->data.key_route.key_len = route_len;
        return 1;
    } else if (Z_TYPE_P(route_zval) == IS_ARRAY) {
        HashTable* route_ht = Z_ARRVAL_P(route_zval);
        zval *     type_zv = NULL, *key_zv = NULL, *host_zv = NULL, *port_zv = NULL;

        /* Check if we have a type-based routing config */
        type_zv = zend_hash_str_find(route_ht, "type", sizeof("type") - 1);
        if (type_zv && Z_TYPE_P(type_zv) == IS_STRING) {
            /* Type-based routing */
            char* type_str = Z_STRVAL_P(type_zv);

            if (strcasecmp(type_str, "primarySlotKey") == 0 ||
                strcasecmp(type_str, "slotKey") == 0) {
                /* Slot key routing */
                key_zv = zend_hash_str_find(route_ht, "key", sizeof("key") - 1);
                if (key_zv && (Z_TYPE_P(key_zv) == IS_STRING || Z_TYPE_P(key_zv) == IS_LONG)) {
                    route->type                         = ROUTE_TYPE_KEY;
                    route->data.key_route.key_allocated = 0; /* Initialize flag */

                    if (Z_TYPE_P(key_zv) == IS_STRING) {
                        /* String key - use directly */
                        route->data.key_route.key     = Z_STRVAL_P(key_zv);
                        route->data.key_route.key_len = Z_STRLEN_P(key_zv);
                    } else if (Z_TYPE_P(key_zv) == IS_LONG) {
                        /* Integer key - convert to string */
                        route->data.key_route.key =
                            long_to_string(Z_LVAL_P(key_zv), &route->data.key_route.key_len);
                        if (!route->data.key_route.key) {
                            return 0; /* Memory allocation failed */
                        }
                        route->data.key_route.key_allocated = 1; /* Mark as allocated */
                    }
                    return 1;
                }
            } else if (strcasecmp(type_str, "routeByAddress") == 0) {
                /* Route by address */
                host_zv = zend_hash_str_find(route_ht, "host", sizeof("host") - 1);
                port_zv = zend_hash_str_find(route_ht, "port", sizeof("port") - 1);

                if (host_zv && port_zv && Z_TYPE_P(host_zv) == IS_STRING) {
                    route->type                      = ROUTE_TYPE_HOST_PORT;
                    route->data.host_port_route.host = Z_STRVAL_P(host_zv);

                    /* Get port from array */
                    if (Z_TYPE_P(port_zv) == IS_LONG) {
                        route->data.host_port_route.port = Z_LVAL_P(port_zv);
                    } else {
                        zval temp;
                        ZVAL_COPY(&temp, port_zv);
                        convert_to_long(&temp);
                        route->data.host_port_route.port = Z_LVAL(temp);
                        zval_dtor(&temp);
                    }
                    return 1;
                }
            }

            return 0; /* Invalid type-based routing */
        }

        /* Try direct host/port keys */
        host_zv = zend_hash_str_find(route_ht, "host", sizeof("host") - 1);
        port_zv = zend_hash_str_find(route_ht, "port", sizeof("port") - 1);

        if (!host_zv || !port_zv) {
            /* Try numeric keys (indexed array approach) */
            host_zv = zend_hash_index_find(route_ht, 0);
            port_zv = zend_hash_index_find(route_ht, 1);
        }

        if (host_zv && port_zv && Z_TYPE_P(host_zv) == IS_STRING) {
            /* Set route type to host:port */
            route->type = ROUTE_TYPE_HOST_PORT;

            /* Get host from array */
            route->data.host_port_route.host = Z_STRVAL_P(host_zv);

            /* Get port from array */
            if (Z_TYPE_P(port_zv) == IS_LONG) {
                route->data.host_port_route.port = Z_LVAL_P(port_zv);
            } else {
                zval temp;
                ZVAL_COPY(&temp, port_zv);
                convert_to_long(&temp);
                route->data.host_port_route.port = Z_LVAL(temp);
                zval_dtor(&temp);
            }

            return 1;
        }
    }

    /* Could not parse route properly */
    return 0;
}

/* Create serialized route bytes from a cluster_route_t structure */
uint8_t* create_route_bytes_from_route(cluster_route_t* route, size_t* route_bytes_len) {
    /* Initialize route structure */
    CommandRequest__Routes routes      = COMMAND_REQUEST__ROUTES__INIT;
    uint8_t*               route_bytes = NULL;

    /* Validate input parameters */
    if (!route || !route_bytes_len) {
        if (route_bytes_len)
            *route_bytes_len = 0;
        return NULL;
    }

    /* Declare protobuf structures outside switch to keep them in scope */
    CommandRequest__SlotKeyRoute   slot_key_route   = COMMAND_REQUEST__SLOT_KEY_ROUTE__INIT;
    CommandRequest__ByAddressRoute by_address_route = COMMAND_REQUEST__BY_ADDRESS_ROUTE__INIT;
    CommandRequest__SimpleRoutes   simple_route;

    switch (route->type) {
        case ROUTE_TYPE_KEY:
            /* Validate key data */
            if (!route->data.key_route.key || route->data.key_route.key_len == 0) {
                printf("Error: Invalid key data for route\n");
                *route_bytes_len = 0;
                return NULL;
            }

            /* Configure slot key route */
            slot_key_route.slot_type = COMMAND_REQUEST__SLOT_TYPES__Primary;
            slot_key_route.slot_key  = route->data.key_route.key;

            routes.value_case     = COMMAND_REQUEST__ROUTES__VALUE_SLOT_KEY_ROUTE;
            routes.slot_key_route = &slot_key_route;
            break;

        case ROUTE_TYPE_HOST_PORT:
            // printf("Creating route by address: %s:%d\n",
            //       route->data.host_port_route.host, route->data.host_port_route.port);
            /* Configure by address route */
            by_address_route.host = route->data.host_port_route.host;
            by_address_route.port = route->data.host_port_route.port;

            routes.value_case       = COMMAND_REQUEST__ROUTES__VALUE_BY_ADDRESS_ROUTE;
            routes.by_address_route = &by_address_route;
            break;

        case ROUTE_TYPE_SIMPLE:
            // printf("Creating simple route type: %d\n", route->data.simple_route_type);
            /* Configure simple route */
            simple_route = route->data.simple_route_type;

            routes.value_case    = COMMAND_REQUEST__ROUTES__VALUE_SIMPLE_ROUTES;
            routes.simple_routes = simple_route;
            break;

        default:
            /* Unknown route type */
            printf("Error: Unknown route type: %d\n", route->type);
            *route_bytes_len = 0;
            return NULL;
    }

    /* Get serialized size and allocate buffer */
    *route_bytes_len = command_request__routes__get_packed_size(&routes);
    if (*route_bytes_len == 0) {
        printf("Error: Failed to get packed size for route\n");
        return NULL;
    }

    route_bytes = (uint8_t*) emalloc(*route_bytes_len);

    if (!route_bytes) {
        printf("Error: Failed to allocate memory for route bytes\n");
        *route_bytes_len = 0;
        return NULL;
    }

    /* Serialize the routes */
    size_t packed_size = command_request__routes__pack(&routes, route_bytes);
    if (packed_size != *route_bytes_len) {
        printf(
            "Error: Packed size mismatch: expected %zu, got %zu\n", *route_bytes_len, packed_size);
        efree(route_bytes);
        *route_bytes_len = 0;
        return NULL;
    }

    // printf("Successfully created route bytes: %zu bytes\n", *route_bytes_len);
    return route_bytes;
}

/* Execute a command and handle common error checking */
CommandResult* execute_command_with_route(const void*          glide_client,
                                          enum RequestType     command_type,
                                          unsigned long        arg_count,
                                          const uintptr_t*     args,
                                          const unsigned long* args_len,
                                          zval*                arg_route) {
    /* Check if client is valid */
    if (!glide_client) {
        printf("Error: glide_client is NULL\n");
        return NULL;
    }

    /* Validate route parameter */
    if (!arg_route) {
        printf("Error: arg_route is NULL\n");
        return NULL;
    }

    /* Parse the route from the first parameter */
    cluster_route_t route;
    memset(&route, 0, sizeof(cluster_route_t));
    if (!parse_cluster_route(arg_route, &route)) {
        /* Failed to parse the route */
        printf("Error: Failed to parse cluster route\n");
        return NULL;
    }

    /* Create serialized route bytes */
    size_t   route_bytes_len = 0;
    uint8_t* route_bytes     = create_route_bytes_from_route(&route, &route_bytes_len);
    if (!route_bytes) {
        printf("Error: Failed to create route bytes\n");
        /* Free dynamically allocated key if needed before returning */
        if (route.type == ROUTE_TYPE_KEY && route.data.key_route.key_allocated) {
            efree(route.data.key_route.key);
        }
        return NULL;
    }

    /* Validate all parameters before FFI call */
    if (!glide_client) {
        printf("ERROR: glide_client is NULL\n");
        return NULL;
    }

    if (arg_count > 0) {
        if (!args) {
            printf("ERROR: args is NULL but arg_count is %lu\n", arg_count);
            return NULL;
        }
        if (!args_len) {
            printf("ERROR: args_len is NULL but arg_count is %lu\n", arg_count);
            return NULL;
        }
    }

    if (route_bytes_len > 0) {
        if (!route_bytes) {
            printf("ERROR: route_bytes is NULL but route_bytes_len is %zu\n", route_bytes_len);
            return NULL;
        }
    }

    /* Execute the command */
    CommandResult* result = command(glide_client,
                                    0,               /* channel */
                                    command_type,    /* command type */
                                    arg_count,       /* number of arguments */
                                    args,            /* arguments */
                                    args_len,        /* argument lengths */
                                    route_bytes,     /* route bytes */
                                    route_bytes_len, /* route bytes length */
                                    0                /* span pointer */
    );

    /* Free route bytes */
    if (route_bytes) {
        efree(route_bytes);
    }

    /* Free dynamically allocated key if needed */
    if (route.type == ROUTE_TYPE_KEY && route.data.key_route.key_allocated) {
        efree(route.data.key_route.key);
    }

    /* Validate result before returning */
    if (!result) {
        printf("Error: Command execution returned NULL result\n");
    } else if (result->command_error) {
        printf("Error: Command execution failed: %s\n",
               result->command_error->command_error_message
                   ? result->command_error->command_error_message
                   : "Unknown error");
    }

    return result;
}

/* Execute a command and handle common error checking */
CommandResult* execute_command(const void*          glide_client,
                               enum RequestType     command_type,
                               unsigned long        arg_count,
                               const uintptr_t*     args,
                               const unsigned long* args_len) {
    /* Check if client is valid */
    if (!glide_client) {
        return NULL;
    }

    /* Execute the command */
    CommandResult* result = command(glide_client,
                                    0,            /* channel */
                                    command_type, /* command type */
                                    arg_count,    /* number of arguments */
                                    args,         /* arguments */
                                    args_len,     /* argument lengths */
                                    NULL,         /* route bytes */
                                    0,            /* route bytes length */
                                    0             /* span pointer */
    );

    return result;
}

/* Handle an integer response */
long handle_int_response(CommandResult* result, long* output_value) {
    /* Check if the command was successful */
    if (!result) {
        return 0; /* False - failure */
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return 0; /* False - failure */
    }

    /* Get the result value */
    if (result->response && result->response->response_type == Int) {
        *output_value = result->response->int_value;

        /* Free the result */
        free_command_result(result);
        return 1; /* True - success */
    } else {
        // printf("%s:%d - Unexpected response type for integer command\n", __FILE__, __LINE__);
        assert(0); /* This should never happen */
    }

    /* Unexpected response type */
    free_command_result(result);
    return 0; /* False - failure */
}

/* Handle a string response */
int handle_string_response(CommandResult* result, char** output, size_t* output_len) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Process the result */
    int ret_val = -1;
    if (result->response) {
        switch (result->response->response_type) {
            case String:

                /* Command returns a string/binary data */
                if (result->response->string_value_len == 0) {
                    *output = emalloc(1);  // Allocate at least one byte using PHP's memory manager
                    if (*output) {
                        (*output)[0] = '\0';  // Empty string is still null-terminated
                    }
                    *output_len = 0;
                } else {
                    // Allocate exact size needed for binary data using PHP's memory manager
                    *output = emalloc(result->response->string_value_len + 1);
                    (*output)[result->response->string_value_len] = '\0';
                    if (*output) {
                        // Copy binary data without assuming null-termination
                        memcpy(*output,
                               result->response->string_value,
                               result->response->string_value_len);
                    }
                    *output_len = result->response->string_value_len;
                }
                // Check if allocation failed
                if (!*output) {
                    ret_val = -1;  // Memory allocation failed
                } else {
                    ret_val = 1;  // Success
                }
                break;
            case Null:

                /* Key didn't exist, return NULL */
                *output     = NULL;
                *output_len = 0;
                ret_val     = 0;
                break;
            default:

                ret_val = -1;
                break;
        }
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Handle a boolean response */
int handle_bool_response(CommandResult* result) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Get the result value */
    int ret_val = -1;
    if (result->response && result->response->response_type == Bool) {
        ret_val = result->response->bool_value ? 1 : 0;
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Handle an OK response */
int handle_ok_response(CommandResult* result) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Get the result value */
    int ret_val = -1;
    if (result->response && result->response->response_type == Ok) {
        ret_val = 1;
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Helper function to convert a CommandResponse to a PHP value
 * use_associative_array:
 * - 0: regular array processing
 * - 1: convert Map elements to associative array format (for ZMPOP/sorted sets)
 */
int command_response_to_zval(CommandResponse* response,
                             zval*            output,
                             int              use_associative_array,
                             bool             use_false_if_null) {
    if (!response) {
        ZVAL_NULL(output);
        return 0;
    }

    switch (response->response_type) {
        case Null:
            // printf("%s:%d - CommandResponse is NULL\n", __FILE__, __LINE__);
            if (use_false_if_null) {
                // printf("%s:%d - CommandResponse is converted to false\n", __FILE__, __LINE__);
                ZVAL_FALSE(output);
            } else {
                ZVAL_NULL(output);
            }

            return 0;
        case Int:
            // printf("%s:%d - CommandResponse is Int: %ld\n", __FILE__, __LINE__,
            // response->int_value);
            ZVAL_LONG(output, response->int_value);
            return 1;
        case Float:
            // printf("%s:%d - CommandResponse is Float: %f\n", __FILE__, __LINE__,
            // response->float_value);
            ZVAL_DOUBLE(output, response->float_value);
            ////printf("%s:%d - Converted CommandResponse to double: %f\n", __FILE__, __LINE__,
            /// Z_DVAL_P(output));
            return 1;
        case Bool:
            ////printf("%s:%d - CommandResponse is Bool: %d\n", __FILE__, __LINE__,
            /// response->bool_value);
            ZVAL_BOOL(output, response->bool_value);
            return 1;
        case String:
            // printf("%s:%d - CommandResponse is String with length: %ld string = %s\n", __FILE__,
            // __LINE__, response->string_value_len, response->string_value);
            ZVAL_STRINGL(output, response->string_value, response->string_value_len);
            return 1;
        case Array:
            //  printf("%s:%d - CommandResponse is Array with length: %ld, use_associative_array =
            //  %d\n", __FILE__, __LINE__, response->array_value_len, use_associative_array);
            array_init(output);
            if (use_associative_array == COMMAND_RESPONSE_SCAN_ASSOSIATIVE_ARRAY) {
                for (int64_t i = 0; i + 1 < response->array_value_len; i += 2) {
                    zval field, value;

                    command_response_to_zval(&response->array_value[i],
                                             &field,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                             use_false_if_null);
                    command_response_to_zval(&response->array_value[i + 1],
                                             &value,
                                             COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                             use_false_if_null);

                    if (Z_TYPE(field) == IS_STRING) {
                        add_assoc_zval(output, Z_STRVAL(field), &value);
                        zval_dtor(&field);
                    } else {
                        zval_dtor(&field);
                        zval_dtor(&value);
                    }
                }
            } else if (response->array_value_len == 2 &&
                       use_associative_array == COMMAND_RESPONSE_STREAM_ARRAY_ASSOCIATIVE) {
                zval field, value;
                // printf("%s:%d - response->array_value[0]->command_response_type = %d,
                // response->array_value[1]->command_response_type = %d\n",
                //       __FILE__, __LINE__, response->array_value[0].response_type,
                //       response->array_value[1].response_type);
                command_response_to_zval(
                    &response->array_value[0], &field, use_associative_array, use_false_if_null);
                command_response_to_zval(
                    &response->array_value[1], &value, use_associative_array, use_false_if_null);
                // printf("%s:%d - DEBUG: Adding field \n", __FILE__, __LINE__);
                // php_var_dump(&field, 2);
                // printf("%s:%d - DEBUG: Adding value \n", __FILE__, __LINE__);
                // php_var_dump(&value, 2);

                if (Z_TYPE(field) == IS_STRING) {
                    // printf("%s:%d - DEBUG: Adding field %s with value %s\n", __FILE__, __LINE__,
                    // Z_STRVAL(field), Z_STRVAL(value));
                    add_assoc_zval(output, Z_STRVAL(field), &value);
                    zval_dtor(&field);
                } else if (Z_TYPE(value) == IS_ARRAY && Z_TYPE(field) == IS_ARRAY) {
                    {
                        // Iterate through the field array and add each key-value pair to output
                        HashTable*   field_ht = Z_ARRVAL(field);
                        zend_string* key;
                        zval*        val;
                        ZEND_HASH_FOREACH_STR_KEY_VAL(field_ht, key, val) {
                            zval copy;
                            ZVAL_COPY(&copy, val);
                            add_assoc_str(output, ZSTR_VAL(key), Z_STR(copy));
                        }
                        ZEND_HASH_FOREACH_END();
                    }

                    // Do the same for the value array

                    {
                        HashTable*   value_ht = Z_ARRVAL(value);
                        zend_string* key;
                        zval*        val;
                        ZEND_HASH_FOREACH_STR_KEY_VAL(value_ht, key, val) {
                            zval copy;
                            ZVAL_COPY(&copy, val);
                            add_assoc_str(output, ZSTR_VAL(key), Z_STR(copy));
                        }
                        ZEND_HASH_FOREACH_END();
                    }
                } else {
                    zval_dtor(&field);
                    zval_dtor(&value);
                }
                // php_var_dump(output, 2);

            } else {
                for (int64_t i = 0; i < response->array_value_len; i++) {
                    zval value;

                    command_response_to_zval(&response->array_value[i],
                                             &value,
                                             use_associative_array,
                                             use_false_if_null);
                    // printf("%s:%d - DEBUG: Adding array value %d\n", __FILE__, __LINE__, i);
                    //   php_var_dump(&value, 2); // No need to modify this as it's not printf

                    add_next_index_zval(output, &value);
                    // printf("%s:%d - DEBUG: Added array value %d\n", __FILE__, __LINE__, i);
                    // php_var_dump(output, 2); // No need to modify this as it's not printf
                }
            }
            // printf("%s:%d - DEBUG: Finished processing array response\n", __FILE__, __LINE__);
            return 1;

        case Map:
            // printf("%s:%d - CommandResponse is Map with length: %ld\n", __FILE__, __LINE__,
            // response->array_value_len);
            array_init(output);

            // Special handling for FUNCTION command - skip server address wrapper
            if (use_associative_array == COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP_FUNCTION &&
                response->array_value_len == 1) {
                CommandResponse* element = &response->array_value[0];
                // Remove the first field of server address (contains ":")
                if (element->map_key != NULL && element->map_key->response_type == String) {
                    char* key_str = element->map_key->string_value;
                    if (strchr(key_str, ':') != NULL) {
                        // Skip the server key and process only the value
                        if (element->map_value != NULL) {
                            return command_response_to_zval(element->map_value,
                                                            output,
                                                            use_associative_array,
                                                            use_false_if_null);
                        }
                    }
                }
            }

            // Normal Map processing
            for (int i = 0; i < response->array_value_len; i++) {
                zval             key, value;
                CommandResponse* element = &response->array_value[i];

                // Process the key
                if (element->map_key != NULL) {
                    command_response_to_zval(
                        element->map_key, &key, use_associative_array, use_false_if_null);
                } else {
                    ZVAL_NULL(&key);
                }

                // Process the value
                if (element->map_value != NULL) {
                    // printf("%s:%d - DEBUG: Processing map value %d\n", __FILE__, __LINE__, i);
                    command_response_to_zval(
                        element->map_value, &value, use_associative_array, use_false_if_null);
                    // printf("%s:%d - DEBUG: Map value %d processed\n", __FILE__, __LINE__, i);
                } else {
                    ZVAL_NULL(&value);
                }

                if (use_associative_array != COMMAND_RESPONSE_NOT_ASSOSIATIVE &&
                    Z_TYPE(key) == IS_STRING) {
                    // printf("%s:%d - DEBUG: Adding key %s \n", __FILE__, __LINE__, Z_STRVAL(key));
                    // php_var_dump(&value, 2); // No need to modify this as it's not printf
                    add_assoc_zval(output, Z_STRVAL(key), &value);
                    zval_dtor(&key);  // Clean up the key since we're using it as an index
                } else {
                    // Add the key as a separate array element (original behavior)
                    add_next_index_zval(output, &key);
                    // Add the value as the next array element
                    add_next_index_zval(output, &value);
                }
            }
            // php_var_dump(output, 2); // No need to modify this as it's not printf
            return 1;

        case Sets:
            array_init(output);
            for (int i = 0; i < response->sets_value_len; i++) {
                zval             value;
                CommandResponse* set_item = &response->sets_value[i];

                if (set_item->response_type == String) {
                    ZVAL_STRINGL(&value, set_item->string_value, set_item->string_value_len);
                    add_next_index_zval(output, &value);
                }
            }
            return 1;
        case Ok:
            // ZVAL_STRING(output, "OK");
            ZVAL_BOOL(output, true);
            return 1;
        default:
            ZVAL_NULL(output);
            return -1;
    }
}

/* Handle an array response */
int handle_array_response(CommandResult* result, zval* output) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Process the result */
    int ret_val = -1;
    if (result->response) {
        if (result->response->response_type == Null) {
            ZVAL_NULL(output);
            ret_val = 0;
        } else if (result->response->response_type == Array) {
            ret_val = command_response_to_zval(
                result->response, output, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        } else {
            ret_val = -1;
        }
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Handle a map response */
int handle_map_response(CommandResult* result, zval* output) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Process the result */
    int ret_val = -1;
    if (result->response) {
        if (result->response->response_type == Null) {
            ZVAL_NULL(output);
            ret_val = 0;
        } else if (result->response->response_type == Map) {
            ret_val = command_response_to_zval(
                result->response, output, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        } else {
            ret_val = -1;
        }
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Handle a set response */
int handle_set_response(CommandResult* result, zval* output) {
    /* Check if the command was successful */
    if (!result) {
        return -1;
    }

    /* Check if there was an error */
    if (result->command_error) {
        printf("%s:%d - Error executing command: %s\n",
               __FILE__,
               __LINE__,
               result->command_error->command_error_message);
        free_command_result(result);
        return -1;
    }

    /* Process the result */
    int ret_val = -1;
    if (result->response) {
        if (result->response->response_type == Null) {
            ZVAL_NULL(output);
            ret_val = 0;
        } else if (result->response->response_type == Sets) {
            ret_val = command_response_to_zval(
                result->response, output, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        } else {
            ret_val = -1;
        }
    }

    /* Free the result */
    free_command_result(result);

    return ret_val;
}

/* Convert a long value to a string */
char* long_to_string(long value, size_t* len) {
    char buffer[32];
    *len      = snprintf(buffer, sizeof(buffer), "%ld", value);
    char* str = (char*) emalloc(*len + 1);
    if (str) {
        memcpy(str, buffer, *len);
        str[*len] = '\0';
    }
    return str;
}

/* Convert a double value to a string */
char* double_to_string(double value, size_t* len) {
    char buffer[64];
    /* Use %.6g format to get a more user-friendly representation */
    *len      = snprintf(buffer, sizeof(buffer), "%.6g", value);
    char* str = (char*) emalloc(*len + 1);
    if (str) {
        memcpy(str, buffer, *len);
        str[*len] = '\0';
    }
    return str;
}
/* Helper function to convert a CommandResponse to a PHP stream format
 * This is specifically for XRANGE/XREVRANGE commands that return stream entries
 * We need to handle both Array and Map response types
 * The output should be: ["stream_id" => ["field1" => "value1", "field2" => "value2", ...]]
 */
int command_response_to_stream_zval(CommandResponse* response, zval* output) {
    // printf("%s:%d - ------------------------------------------------\n", __FILE__, __LINE__);
    if (!response) {
        // printf("%s:%d - DEBUG: Response is NULL\n", __FILE__, __LINE__);
        ZVAL_NULL(output);
        return 0;
    }
    array_init(output);

    /* Handle different response types */
    // printf("%s:%d - DEBUG: Processing command response of type %d\n", __FILE__, __LINE__,
    // response->response_type);
    switch (response->response_type) {
        case Map:
            /* Process map response where keys are stream IDs and values are field-value pairs */
            for (int i = 0; i < response->array_value_len; i++) {
                CommandResponse* element = &response->array_value[i];

                /* Skip if we don't have both key and value */
                if (!element->map_key || !element->map_value) {
                    continue;
                }

                /* Extract stream ID from key */
                if (element->map_key->response_type != String) {
                    continue;
                }

                char*  stream_id     = element->map_key->string_value;
                size_t stream_id_len = element->map_key->string_value_len;
                // printf("%s:%d - NEW STREAM ID: %.*s\n", __FILE__, __LINE__, (int)stream_id_len,
                // stream_id);

                /* Create associative array for field-value pairs */
                zval field_array;
                array_init(&field_array);
                // printf("%s:%d - DEBUG: Processing stream ID: %.*s,
                // element->map_value->response_type = %d\n", __FILE__, __LINE__,
                // (int)stream_id_len, stream_id, element->map_value->response_type);
                /* Process nested field-value pairs - add safety check */
                if (element->map_value->response_type == Array) {
                    /* Safe version that checks array bounds */
                    if (element->map_value->array_value_len > 0) {
                        // printf("%s:%d - DEBUG: Processing Array response for stream ID: %.*s,
                        // array_value_len = %ld\n", __FILE__, __LINE__, (int)stream_id_len,
                        // stream_id, element->map_value->array_value_len);
                        CommandResponse* field_resp1 = &element->map_value->array_value[0];

                        if (field_resp1->response_type == Array &&
                            field_resp1->array_value_len == 2) {
                            zval field, value;
                            command_response_to_zval(&field_resp1->array_value[0],
                                                     &field,
                                                     COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                                     false);
                            command_response_to_zval(&field_resp1->array_value[1],
                                                     &value,
                                                     COMMAND_RESPONSE_NOT_ASSOSIATIVE,
                                                     false);

                            if (Z_TYPE(field) == IS_STRING) {
                                // printf("%s:%d - DEBUG: Adding field %s with value %s\n",
                                // __FILE__, __LINE__, Z_STRVAL(field), Z_STRVAL(value));
                                add_assoc_zval(&field_array, Z_STRVAL(field), &value);
                                zval_dtor(&field);
                            } else {
                                zval_dtor(&field);
                                zval_dtor(&value);
                            }
                        }
                    }
                    /* Add the stream entry to the output array */
                    add_assoc_zval_ex(output, stream_id, stream_id_len, &field_array);
                } else if (element->map_value->response_type == Map) {
                    CommandResponse* map = element->map_value;
                    // printf("%s:%d - DEBUG: Processing Map response for stream ID: %.*s,
                    // map->array_value_len = %ld\n", __FILE__, __LINE__, (int)stream_id_len,
                    // stream_id, map->array_value_len); printf("%s:%d - DEBUG: Map response type =
                    // %d\n", __FILE__, __LINE__, map->response_type);
                    zval output1;
                    command_response_to_zval(
                        map, &output1, COMMAND_RESPONSE_STREAM_ARRAY_ASSOCIATIVE, false);
                    add_assoc_zval_ex(output, stream_id, stream_id_len, &output1);
                } else {
                    // printf("%s:%d - DEBUG: Unexpected response type for stream fields: %d\n",
                    // __FILE__, __LINE__, element->map_value->response_type);
                }
            }
            break;
        case Null:
            /* If the response is Null, set output to NULL */
            // printf("%s:%d - DEBUG: Response is Null\n", __FILE__, __LINE__);
            array_init(output);
            break;

        default:
            zval_dtor(output); /* Clean up the initialized array */
            ZVAL_NULL(output);
            return 0;
    }

    return 1;
}

/**
 * Safe zval to string conversion with memory management
 * Returns allocated string that must be freed, or NULL on error
 * Sets need_free to 1 if returned string must be freed
 */
char* zval_to_string_safe(zval* z, size_t* len, int* need_free) {
    char* str  = NULL;
    *need_free = 0;

    if (!z || !len) {
        return NULL;
    }

    switch (Z_TYPE_P(z)) {
        case IS_STRING:
            str  = Z_STRVAL_P(z);
            *len = Z_STRLEN_P(z);
            break;

        case IS_LONG:
            str        = long_to_string(Z_LVAL_P(z), len);
            *need_free = 1;
            break;

        case IS_DOUBLE:
            str        = double_to_string(Z_DVAL_P(z), len);
            *need_free = 1;
            break;

        case IS_TRUE:
            str        = estrdup("1");
            *len       = 1;
            *need_free = 1;
            break;

        case IS_FALSE:
            str        = estrdup("0");
            *len       = 1;
            *need_free = 1;
            break;

        default: {
            /* Convert other types to string */
            zval copy;
            ZVAL_COPY(&copy, z);
            convert_to_string(&copy);
            str  = estrndup(Z_STRVAL(copy), Z_STRLEN(copy));
            *len = Z_STRLEN(copy);
            zval_dtor(&copy);
            *need_free = 1;
        } break;
    }

    return str;
}

/**
 * Free array of allocated strings
 */
void free_allocated_strings(char** strings, int count) {
    if (!strings)
        return;

    for (int i = 0; i < count; i++) {
        if (strings[i]) {
            efree(strings[i]);
        }
    }
}
