#include "php.h"
#include "php_ini.h"

#ifndef VALKEY_GLIDE_COMMON_H
#define VALKEY_GLIDE_COMMON_H

#include <stdio.h>
#include <zend_smart_str.h>

#include "include/glide_bindings.h"

/* ValkeyGlidePHP version */
#define VALKEY_GLIDE_PHP_VERSION "0.10.0"

#define VALKEY_GLIDE_PHP_GET_OBJECT(class_entry, o) \
    (class_entry*) ((char*) o - XtOffsetOf(class_entry, std))
#define VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(class_entry, z) \
    VALKEY_GLIDE_PHP_GET_OBJECT(class_entry, Z_OBJ_P(z))

/* NULL check so Eclipse doesn't go crazy */
#ifndef NULL
#define NULL ((void*) 0)
#endif

/* We'll fallthrough if we want to */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/* ValkeyGlide data types for compatibility */
#define VALKEY_GLIDE_NOT_FOUND 0
#define VALKEY_GLIDE_STRING 1
#define VALKEY_GLIDE_SET 2
#define VALKEY_GLIDE_LIST 3
#define VALKEY_GLIDE_ZSET 4
#define VALKEY_GLIDE_HASH 5
#define VALKEY_GLIDE_STREAM 6

/* Transaction modes */
#define MULTI 0
#define PIPELINE 1

#define VALKEY_GLIDE_MAX_OPTIONS 64

/* ValkeyGlide Configuration Enums */
typedef enum {
    VALKEY_GLIDE_READ_FROM_PRIMARY                          = 0,
    VALKEY_GLIDE_READ_FROM_PREFER_REPLICA                   = 1,
    VALKEY_GLIDE_READ_FROM_AZ_AFFINITY                      = 2,
    VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY = 3
} valkey_glide_read_from_t;

typedef enum {
    VALKEY_GLIDE_PERIODIC_CHECKS_ENABLED_DEFAULT = 0,
    VALKEY_GLIDE_PERIODIC_CHECKS_DISABLED        = 1
} valkey_glide_periodic_checks_status_t;

/* ValkeyGlide Configuration Structures */
typedef struct {
    char* host;
    int   port;
} valkey_glide_node_address_t;

typedef enum {
    VALKEY_GLIDE_SERVICE_TYPE_ELASTICACHE = 0,
    VALKEY_GLIDE_SERVICE_TYPE_MEMORYDB    = 1
} valkey_glide_service_type_t;

typedef struct {
    char*                       cluster_name;
    char*                       region;
    valkey_glide_service_type_t service_type;
    int                         refresh_interval_seconds; /* 0 means use default (300s) */
} valkey_glide_iam_config_t;

typedef struct {
    char*                      password;
    char*                      username;   /* Optional for password auth, REQUIRED for IAM */
    valkey_glide_iam_config_t* iam_config; /* NULL if using password auth */
} valkey_glide_server_credentials_t;

/* Address Constants */
#define VALKEY_GLIDE_ADDRESS_HOST "host"
#define VALKEY_GLIDE_ADDRESS_PORT "port"

/* Authentication Constants */
#define VALKEY_GLIDE_AUTH_PASSWORD "password"
#define VALKEY_GLIDE_AUTH_USERNAME "username"
#define VALKEY_GLIDE_AUTH_IAM_CONFIG "iamConfig"

/* IAM Authentication Constants */
#define VALKEY_GLIDE_IAM_SERVICE_ELASTICACHE "Elasticache"
#define VALKEY_GLIDE_IAM_SERVICE_MEMORYDB "MemoryDB"
#define VALKEY_GLIDE_IAM_CONFIG_CLUSTER_NAME "clusterName"
#define VALKEY_GLIDE_IAM_CONFIG_REGION "region"
#define VALKEY_GLIDE_IAM_CONFIG_SERVICE "service"
#define VALKEY_GLIDE_IAM_CONFIG_REFRESH_INTERVAL "refreshIntervalSeconds"

/* Connection Configuration Constants */
#define VALKEY_GLIDE_NUM_OF_RETRIES "num_of_retries"
#define VALKEY_GLIDE_FACTOR "factor"
#define VALKEY_GLIDE_EXPONENT_BASE "exponent_base"
#define VALKEY_GLIDE_JITTER_PERCENT "jitter_percent"
#define VALKEY_GLIDE_CONNECTION_TIMEOUT "connection_timeout"

#define VALKEY_GLIDE_DEFAULT_NUM_OF_RETRIES 5
#define VALKEY_GLIDE_DEFAULT_FACTOR 100
#define VALKEY_GLIDE_DEFAULT_EXPONENT_BASE 2
#define VALKEY_GLIDE_DEFAULT_JITTER_PERCENTAGE 20
#define VALKEY_GLIDE_DEFAULT_CONNECTION_TIMEOUT 250

/* TLS Constants */
#define VALKEY_GLIDE_TLS_PREFIX "tls://"
#define VALKEY_GLIDE_SSL_PREFIX "ssl://"
#define VALKEY_GLIDE_SSL_OPTIONS "ssl"
#define VALKEY_GLIDE_VERIFY_PEER "verify_peer"
#define VALKEY_GLIDE_CAFILE "cafile"

#define VALKEY_GLIDE_USE_TLS "use_tls"
#define VALKEY_GLIDE_TLS_CONFIG "tls_config"
#define VALKEY_GLIDE_USE_INSECURE_TLS "use_insecure_tls"
#define VALKEY_GLIDE_ROOT_CERTS "root_certs"

typedef struct {
    int num_of_retries;
    int factor;
    int exponent_base;
    int jitter_percent;
} valkey_glide_backoff_strategy_t;

typedef struct {
    uint8_t* root_certs;       /* Certificate data bytes */
    size_t   root_certs_len;   /* Length of certificate data */
    bool     use_insecure_tls; /* Whether to use insecure TLS (skips certificate verification) */
} valkey_glide_tls_advanced_configuration_t;

typedef struct {
    valkey_glide_tls_advanced_configuration_t* tls_config;         /* NULL if not set */
    int                                        connection_timeout; /* In milliseconds. */
} valkey_glide_advanced_base_client_configuration_t;

typedef struct {
    int duration_in_sec;
} valkey_glide_periodic_checks_manual_interval_t;

typedef struct {
    valkey_glide_node_address_t*                       addresses;
    valkey_glide_server_credentials_t*                 credentials;        /* NULL if not set */
    valkey_glide_backoff_strategy_t*                   reconnect_strategy; /* NULL if not set */
    char*                                              client_name;        /* NULL if not set */
    char*                                              client_az;          /* NULL if not set */
    valkey_glide_advanced_base_client_configuration_t* advanced_config;    /* NULL if not set */
    valkey_glide_read_from_t                           read_from;
    int                                                addresses_count;
    int                                                request_timeout;         /* -1 if not set */
    int                                                inflight_requests_limit; /* -1 if not set */
    int                                                database_id;             /* -1 if not set */
    bool                                               use_tls;
    bool                                               lazy_connect; /* false if not set */
} valkey_glide_base_client_configuration_t;

typedef struct {
    valkey_glide_base_client_configuration_t base;
    valkey_glide_periodic_checks_manual_interval_t*
                                          periodic_checks_manual; /* NULL if using status */
    valkey_glide_periodic_checks_status_t periodic_checks_status;
    bool refresh_topology_from_initial_nodes; /* false if not set */
} valkey_glide_cluster_client_configuration_t;

/* Configuration parsing functions */
int parse_valkey_glide_client_configuration(zval*                                     config_obj,
                                            valkey_glide_base_client_configuration_t* config);
int parse_valkey_glide_cluster_client_configuration(
    zval* config_obj, valkey_glide_cluster_client_configuration_t* config);
void free_valkey_glide_client_configuration(valkey_glide_base_client_configuration_t* config);
void free_valkey_glide_cluster_client_configuration(
    valkey_glide_cluster_client_configuration_t* config);

typedef struct {
    zval*     addresses;
    zval*     credentials;
    zval*     reconnect_strategy;
    zval*     advanced_config;
    zval*     context; /* Stream context for TLS */
    char*     client_name;
    char*     client_az;
    size_t    client_name_len;
    size_t    client_az_len;
    zend_long read_from; /* PRIMARY by default */
    zend_long request_timeout;
    zend_long database_id;
    zend_bool use_tls;
    zend_bool request_timeout_is_null;
    zend_bool lazy_connect;
    zend_bool lazy_connect_is_null;
    zend_bool database_id_is_null;
} valkey_glide_php_common_constructor_params_t;

void valkey_glide_init_common_constructor_params(
    valkey_glide_php_common_constructor_params_t* params);
void valkey_glide_build_client_config_base(valkey_glide_php_common_constructor_params_t* params,
                                           valkey_glide_base_client_configuration_t*     config,
                                           bool is_cluster);
void valkey_glide_cleanup_client_config(valkey_glide_base_client_configuration_t* config);

#if PHP_VERSION_ID < 80000
#define Z_PARAM_ARRAY_HT_OR_NULL(dest) Z_PARAM_ARRAY_HT_EX(dest, 1, 0)
#define Z_PARAM_STR_OR_NULL(dest) Z_PARAM_STR_EX(dest, 1, 0)
#define Z_PARAM_ZVAL_OR_NULL(dest) Z_PARAM_ZVAL_EX(dest, 1, 0)
#define Z_PARAM_BOOL_OR_NULL(dest, is_null) Z_PARAM_BOOL_EX(dest, is_null, 1, 0)
#endif

/**
 * Result processing callback type
 */

typedef int (*z_result_processor_t)(CommandResponse* response, void* output, zval* return_value);

/* Batch command structure for buffering commands - FFI aligned */
struct batch_command {
    uint8_t**            args;        /* FFI expects uint8_t** */
    uintptr_t*           arg_lengths; /* FFI expects uintptr_t* */
    void*                result_ptr;  /* Pointer to store result */
    z_result_processor_t process_result;
    uintptr_t            arg_count; /* FFI expects uintptr_t */
    enum RequestType     request_type;
};

typedef struct {
    const void*           glide_client; /* Valkey Glide client pointer */
    struct batch_command* buffered_commands;
    size_t                command_count;
    size_t                command_capacity;
    int                   batch_type; /* ATOMIC, MULTI, or PIPELINE */
    bool                  is_in_batch_mode;

    zend_object std;
} valkey_glide_object;

/* For convenience we store the salt as a printable hex string which requires 2
 * characters per byte + 1 for the NULL terminator */
#define REDIS_SALT_BYTES 32
#define REDIS_SALT_SIZE ((2 * REDIS_SALT_BYTES) + 1)

ZEND_BEGIN_MODULE_GLOBALS(redis)
char salt[REDIS_SALT_SIZE];
ZEND_END_MODULE_GLOBALS(redis)

ZEND_EXTERN_MODULE_GLOBALS(redis)
#define REDIS_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(redis, v)

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(redis);
PHP_MSHUTDOWN_FUNCTION(redis);
PHP_MINFO_FUNCTION(redis);

zend_class_entry* get_valkey_glide_ce(void);
zend_class_entry* get_valkey_glide_exception_ce(void);

zend_class_entry* get_valkey_glide_cluster_ce(void);

#endif  // VALKEY_GLIDE
