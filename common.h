#include "php.h"
#include "php_ini.h"

#ifndef VALKEY_GLIDE_COMMON_H
#define VALKEY_GLIDE_COMMON_H

/* Deliberately no <ctype.h>: isgraph()/isprint() are locale-dependent and would
 * accept 0xA0-0xFF under a non-C LC_CTYPE, breaking the exact charset parity with
 * glide-core that validate_printable_ascii() below depends on. Use explicit byte
 * ranges instead. */
#include <stdio.h>
#include <zend_smart_str.h>

#include "include/glide_bindings.h"
#include "valkey_glide_address_resolver.h"

/**
 * Validate that a client metadata string (lib_name or client_info_tag) contains
 * only the characters glide-core accepts inside a single metadata field:
 * 0x21..0x27 and 0x2A..0x7E — printable ASCII excluding space, '(' and ')'.
 *
 * This is the per-field subset of the grammar enforced by glide-core's
 * validate_effective_lib_name() on the *composed* name. Parentheses are
 * excluded here because the binding itself introduces the only permitted pair
 * when composing "<base>(<tag>)"; a field that carried its own parenthesis
 * would compose into a name core rejects, so rejecting it early keeps this
 * check from accepting values that would fail downstream.
 *
 * Empty is NOT handled here: callers treat an empty field as absent (matching
 * core, where an empty effective name is treated as absent rather than an
 * error), so this function is only invoked for non-empty values.
 *
 * Rejects embedded NUL bytes, control characters, space, DEL (0x7f), any
 * non-ASCII (e.g. UTF-8) byte, and parentheses. Rejecting these before request
 * composition also prevents an embedded NUL from silently truncating the value
 * when it is later passed through strlen()/snprintf() during serialization.
 *
 * Returns 0 when every byte is acceptable, -1 otherwise.
 */
static inline int validate_printable_ascii(const char* s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) s[i];
        if (c < 0x21 || c > 0x7e || c == '(' || c == ')') {
            return -1;
        }
    }
    return 0;
}

/*
 * Single source of truth for the PHP binding's default library name. Used by
 * the lib-name resolver so the default identity is defined in exactly one place
 * (no drift versus values hardcoded at composition sites).
 */
#define VALKEY_GLIDE_LIB_NAME "GlidePHP"

/**
 * Validate the client metadata fields (lib_name and client_info_tag) that a
 * constructor received. This is a deliberate fail-early path in front of
 * glide-core's validate_effective_lib_name(): core is the authority on the
 * composed name, but validating per field here lets the error name the
 * offending parameter, which core (seeing only the composed string) cannot do.
 *
 * An empty (zero-length) field is treated as ABSENT, not as an error, matching
 * core's behaviour where an empty effective library name is treated as absent —
 * so an empty client_info_tag yields no "(tag)" suffix rather than throwing.
 *
 * Non-empty fields must satisfy validate_printable_ascii, whose accepted set is
 * the per-field subset of core's grammar (0x21-0x27, 0x2A-0x7E: printable ASCII
 * excluding space, '(' and ')'). Keeping the two in step is what stops this
 * check from accepting a value core would later reject.
 *
 * Returns NULL when both fields are acceptable, otherwise a ready-to-throw error
 * message naming the offending field. Centralizing this here removes the
 * copy-pasted validation block that previously lived at every constructor entry
 * point (standalone, cluster, and both mock constructors).
 */
static inline const char* client_metadata_validation_error(const char* lib_name,
                                                           size_t      lib_name_len,
                                                           const char* client_info_tag,
                                                           size_t      client_info_tag_len) {
    if (lib_name != NULL && lib_name_len > 0 &&
        validate_printable_ascii(lib_name, lib_name_len) != 0) {
        return "lib_name must contain only printable ASCII characters from '!' through '~', "
               "excluding spaces and parentheses";
    }
    if (client_info_tag != NULL && client_info_tag_len > 0 &&
        validate_printable_ascii(client_info_tag, client_info_tag_len) != 0) {
        return "client_info_tag must contain only printable ASCII characters from '!' through '~', "
               "excluding spaces and parentheses";
    }
    return NULL;
}

/* ValkeyGlidePHP version */
#define VALKEY_GLIDE_PHP_VERSION "1.1.2"

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

/* Controls how the client discovers node roles and topology in standalone mode. */
typedef enum {
    VALKEY_GLIDE_NODE_DISCOVERY_MODE_STANDARD     = 0,
    VALKEY_GLIDE_NODE_DISCOVERY_MODE_STATIC       = 1,
    VALKEY_GLIDE_NODE_DISCOVERY_MODE_DISCOVER_ALL = 2
} valkey_glide_node_discovery_mode_t;

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
#define VALKEY_GLIDE_CLIENT_CERT "client_cert"
#define VALKEY_GLIDE_CLIENT_KEY "client_key"
#define VALKEY_GLIDE_CLIENT_CERT_PATH "client_cert_path"
#define VALKEY_GLIDE_CLIENT_KEY_PATH "client_key_path"
#define VALKEY_GLIDE_CERT_RELOAD_INTERVAL "cert_reload_interval_seconds"

/* Maximum allowed size (in bytes) for a single certificate or private key.
 * Acts as a safeguard against accidentally loading oversized/incorrect files.
 * Matches the C# client's ConnectionConfiguration.CertificateMaxSize (10 MiB). */
#define VALKEY_GLIDE_CERTIFICATE_MAX_SIZE (10 * 1024 * 1024) /* 10 MiB */

/* Compression Constants */
#define VALKEY_GLIDE_COMPRESSION "compression"
#define VALKEY_GLIDE_COMPRESSION_ENABLED "enabled"
#define VALKEY_GLIDE_COMPRESSION_BACKEND "backend"
#define VALKEY_GLIDE_COMPRESSION_LEVEL "compression_level"
#define VALKEY_GLIDE_COMPRESSION_MIN_SIZE "min_compression_size"
#define VALKEY_GLIDE_COMPRESSION_MAX_DECOMPRESSED_SIZE "max_decompressed_size"

typedef enum {
    VALKEY_GLIDE_COMPRESSION_BACKEND_ZSTD = 0,
    VALKEY_GLIDE_COMPRESSION_BACKEND_LZ4  = 1
} valkey_glide_compression_backend_t;

typedef struct {
    uint32_t                           min_compression_size;
    uint64_t                           max_decompressed_size;
    int32_t                            compression_level;
    valkey_glide_compression_backend_t backend;
    bool                               enabled;
    bool has_compression_level;     /* true if user explicitly set compression_level */
    bool has_max_decompressed_size; /* true if user explicitly set max_decompressed_size */
} valkey_glide_compression_config_t;

/* Circuit breaker configuration */
#define VALKEY_GLIDE_CB_WINDOW_SIZE_MS "window_size_ms"
#define VALKEY_GLIDE_CB_FAILURE_RATE_THRESHOLD "failure_rate_threshold"
#define VALKEY_GLIDE_CB_MIN_ERRORS "min_errors"
#define VALKEY_GLIDE_CB_OPEN_TIMEOUT_MS "open_timeout_ms"
#define VALKEY_GLIDE_CB_COUNT_TIMEOUTS "count_timeouts"
#define VALKEY_GLIDE_CB_CONSECUTIVE_SUCCESSES "consecutive_successes"

#define VALKEY_GLIDE_CB_DEFAULT_WINDOW_SIZE_MS 10000
#define VALKEY_GLIDE_CB_DEFAULT_FAILURE_RATE_THRESHOLD 0.5
#define VALKEY_GLIDE_CB_DEFAULT_MIN_ERRORS 50
#define VALKEY_GLIDE_CB_DEFAULT_OPEN_TIMEOUT_MS 5000
#define VALKEY_GLIDE_CB_DEFAULT_COUNT_TIMEOUTS false
#define VALKEY_GLIDE_CB_DEFAULT_CONSECUTIVE_SUCCESSES 3
#define VALKEY_GLIDE_ERROR_TYPE_CIRCUIT_BREAKER_OPEN 4

typedef struct {
    uint32_t window_size_ms;
    float    failure_rate_threshold;
    uint32_t min_errors;
    uint32_t open_timeout_ms;
    bool     count_timeouts;
    uint32_t consecutive_successes;
} valkey_glide_circuit_breaker_config_t;

/* Client-side cache configuration */
typedef struct {
    char*  cache_id;     /* Unique cache identifier */
    size_t cache_id_len; /* Length of cache_id */
    long   max_cache_kb; /* Maximum cache size in KB */
    long   entry_ttl_ms; /* TTL for entries in milliseconds, 0 = no expiration */
    int eviction_policy; /* Eviction policy: use CONNECTION_REQUEST__EVICTION_POLICY__* constants */
    bool has_eviction_policy; /* true if user explicitly set eviction_policy */
    bool enable_metrics;      /* Whether to enable cache metrics */
} valkey_glide_client_side_cache_config_t;

typedef struct {
    int num_of_retries;
    int factor;
    int exponent_base;
    int jitter_percent;
} valkey_glide_backoff_strategy_t;

typedef struct {
    uint8_t* root_certs;      /* Certificate data bytes */
    size_t   root_certs_len;  /* Length of certificate data */
    uint8_t* client_cert;     /* Client certificate data bytes (byte-based mTLS) */
    size_t   client_cert_len; /* Length of client certificate data */
    uint8_t* client_key;      /* Client private key data bytes (byte-based mTLS) */
    size_t   client_key_len;  /* Length of client private key data */
    char*   client_cert_path; /* Path to client certificate file (path-based mTLS); NULL if unset */
    char*   client_key_path;  /* Path to client private key file (path-based mTLS); NULL if unset */
    int64_t cert_reload_interval; /* Reload cadence in seconds; -1 = unset (core default) */
    bool    use_insecure_tls;     /* Whether to use insecure TLS (skips certificate verification) */
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
    char*                                              lib_name;           /* NULL if not set */
    char*                                              client_info_tag;    /* NULL if not set */
    valkey_glide_advanced_base_client_configuration_t* advanced_config;    /* NULL if not set */
    valkey_glide_compression_config_t*                 compression_config; /* NULL if not set */
    valkey_glide_client_side_cache_config_t*           client_side_cache;  /* NULL if not set */
    valkey_glide_circuit_breaker_config_t*             circuit_breaker;    /* NULL if not set */
    valkey_glide_read_from_t                           read_from;
    valkey_glide_node_discovery_mode_t                 node_discovery_mode;
    int                                                addresses_count;
    int                                                request_timeout;         /* -1 if not set */
    int                                                inflight_requests_limit; /* -1 if not set */
    int                                                database_id;             /* -1 if not set */
    bool                                               use_tls;
    bool                                               lazy_connect; /* false if not set */
    zval* address_resolver; /* NULL if not set - PHP callable */
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
    zval*     context;           /* Stream context for TLS */
    zval*     compression;       /* Compression configuration */
    zval*     client_side_cache; /* Client-side cache configuration */
    zval*     circuit_breaker;   /* Circuit breaker configuration */
    char*     client_name;
    char*     client_az;
    char*     lib_name;
    char*     client_info_tag;
    size_t    client_name_len;
    size_t    client_az_len;
    size_t    lib_name_len;
    size_t    client_info_tag_len;
    zend_long read_from;           /* PRIMARY by default */
    zend_long node_discovery_mode; /* STANDARD by default */
    zend_long request_timeout;
    zend_long database_id;
    zend_bool use_tls;
    zend_bool request_timeout_is_null;
    zend_bool lazy_connect;
    zend_bool lazy_connect_is_null;
    zend_bool database_id_is_null;
    zval*     address_resolver; /* PHP callable or NULL */
} valkey_glide_php_common_constructor_params_t;

void valkey_glide_init_common_constructor_params(
    valkey_glide_php_common_constructor_params_t* params);
int  valkey_glide_build_client_config_base(valkey_glide_php_common_constructor_params_t* params,
                                           valkey_glide_base_client_configuration_t*     config,
                                           bool                                          is_cluster);
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

/* Client runtime options - matching PHPRedis behavior */
typedef enum {
    VALKEY_GLIDE_OPT_REPLY_LITERAL = 1 /* Return "OK" string instead of true for Ok responses */
} valkey_glide_option_t;

typedef struct {
    const void*           glide_client; /* Valkey Glide client pointer */
    struct batch_command* buffered_commands;
    size_t                command_count;
    size_t                command_capacity;
    int                   batch_type; /* ATOMIC, MULTI, or PIPELINE */
    bool                  is_in_batch_mode;

    /* Runtime options (like PHPRedis OPT_* settings) */
    bool opt_reply_literal; /* OPT_REPLY_LITERAL: return "OK" string instead of true */

    AddressResolverCallback resolver_cb; /* NULL if no address resolver */

    /* Last command error message (PHPRedis getLastError/clearLastError), NULL if none */
    zend_string* last_error;

    zend_object std; /* MUST be last - PHP allocates extra memory after this */
} valkey_glide_object;

void valkey_glide_clear_batch_state(valkey_glide_object* valkey_glide);
void valkey_glide_set_last_error(valkey_glide_object* valkey_glide, const char* msg);
void valkey_glide_clear_last_error(valkey_glide_object* valkey_glide);
void valkey_glide_record_command_error(valkey_glide_object* valkey_glide, CommandResult* result);

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
zend_class_entry* get_valkey_glide_circuit_breaker_exception_ce(void);

zend_class_entry* get_valkey_glide_cluster_ce(void);

#endif  // VALKEY_GLIDE
