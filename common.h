#include "php.h"
#include "php_ini.h"

#ifndef VALKEY_GLIDE_COMMON_H
#define VALKEY_GLIDE_COMMON_H

#include <stdio.h>
#include <zend_smart_str.h>

#include <ext/standard/php_smart_string.h>

#include "include/glide_bindings.h"

/* ValkeyGlidePHP version */
#define VALKEY_GLIDE_PHP_VERSION "0.1"

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

typedef struct {
    char* password;
    char* username; /* Optional */
} valkey_glide_server_credentials_t;

/* Default values for connection configuration options. */
#define VALKEY_GLIDE_DEFAULT_NUM_OF_RETRIES 5
#define VALKEY_GLIDE_DEFAULT_FACTOR 100
#define VALKEY_GLIDE_DEFAULT_EXPONENT_BASE 2
#define VALKEY_GLIDE_DEFAULT_JITTER_PERCENTAGE 20

#define VALKEY_GLIDE_DEFAULT_CONNECTION_TIMEOUT 250


typedef struct {
    int num_of_retries;
    int factor;
    int exponent_base;
    int jitter_percent;
} valkey_glide_backoff_strategy_t;

typedef struct {
    bool use_insecure_tls; /* false if not set */
} valkey_glide_tls_advanced_configuration_t;

typedef struct {
    int                                        connection_timeout; /* In milliseconds. */
    valkey_glide_tls_advanced_configuration_t* tls_config;         /* NULL if not set */
} valkey_glide_advanced_base_client_configuration_t;

typedef struct {
    int duration_in_sec;
} valkey_glide_periodic_checks_manual_interval_t;

typedef struct {
    valkey_glide_node_address_t*                       addresses;
    int                                                addresses_count;
    bool                                               use_tls;
    valkey_glide_server_credentials_t*                 credentials; /* NULL if not set */
    valkey_glide_read_from_t                           read_from;
    int                                                request_timeout;    /* -1 if not set */
    valkey_glide_backoff_strategy_t*                   reconnect_strategy; /* NULL if not set */
    char*                                              client_name;        /* NULL if not set */
    int                                                inflight_requests_limit; /* -1 if not set */
    char*                                              client_az;       /* NULL if not set */
    valkey_glide_advanced_base_client_configuration_t* advanced_config; /* NULL if not set */
    bool                                               lazy_connect;    /* false if not set */
} valkey_glide_base_client_configuration_t;

typedef struct {
    valkey_glide_base_client_configuration_t base;
    int                                      database_id; /* -1 if not set */
} valkey_glide_client_configuration_t;

typedef struct {
    valkey_glide_base_client_configuration_t base;
    valkey_glide_periodic_checks_status_t    periodic_checks_status;
    valkey_glide_periodic_checks_manual_interval_t*
        periodic_checks_manual; /* NULL if using status */
} valkey_glide_cluster_client_configuration_t;

/* Configuration parsing functions */
int parse_valkey_glide_client_configuration(zval*                                config_obj,
                                            valkey_glide_client_configuration_t* config);
int parse_valkey_glide_cluster_client_configuration(
    zval* config_obj, valkey_glide_cluster_client_configuration_t* config);
void free_valkey_glide_client_configuration(valkey_glide_client_configuration_t* config);
void free_valkey_glide_cluster_client_configuration(
    valkey_glide_cluster_client_configuration_t* config);

typedef struct {
    zval*     addresses;
    zend_bool use_tls;
    zval*     credentials;
    zend_long read_from; /* PRIMARY by default */
    zend_long request_timeout;
    zend_bool request_timeout_is_null;
    zval*     reconnect_strategy;
    char*     client_name;
    size_t    client_name_len;
    char*     client_az;
    size_t    client_az_len;
    zval*     advanced_config;
    zend_bool lazy_connect;
    zend_bool lazy_connect_is_null;
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
    enum RequestType     request_type;
    uint8_t**            args;        /* FFI expects uint8_t** */
    uintptr_t*           arg_lengths; /* FFI expects uintptr_t* */
    uintptr_t            arg_count;   /* FFI expects uintptr_t */
    void*                result_ptr;  /* Pointer to store result */
    z_result_processor_t process_result;
};

typedef struct {
    const void* glide_client; /* Valkey Glide client pointer */

    /* Batch mode tracking */
    bool is_in_batch_mode;
    int  batch_type; /* ATOMIC, MULTI, or PIPELINE */

    /* Command buffering */
    struct batch_command* buffered_commands;
    size_t                command_count;
    size_t                command_capacity;

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
zend_class_entry* get_valkey_glide_cluster_exception_ce(void);

#endif  // VALKEY_GLIDE
