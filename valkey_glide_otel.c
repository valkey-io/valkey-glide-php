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

#include "valkey_glide_otel.h"

#include <string.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include "common.h"
#include "logger.h"

/* Forward declarations for static functions */
static void                               cleanup_otel_config(void);
static bool                               valkey_glide_otel_should_sample(void);
static struct OpenTelemetryConfig*        parse_otel_config(zval* config_obj);
static struct OpenTelemetryTracesConfig*  parse_traces_config_object(zval* traces_obj);
static struct OpenTelemetryMetricsConfig* parse_metrics_config_object(zval* metrics_obj);

/* Global OTEL configuration */
struct OpenTelemetryConfig* g_otel_config = NULL;

/* Sentinel value for invalid/uninitialized sample percentage */
#define OTEL_SAMPLE_PERCENTAGE_INVALID -1

/**
 * Initialize OpenTelemetry with the given configuration
 * Can only be initialized once per process
 */
int valkey_glide_otel_init(zval* config_obj) {
    if (g_otel_config) {
        VALKEY_LOG_WARN("otel_init",
                        "OpenTelemetry already initialized, ignoring subsequent calls");
        return 1; /* Success - already initialized */
    }

    if (!config_obj || Z_TYPE_P(config_obj) != IS_OBJECT) {
        VALKEY_LOG_DEBUG("otel_init", "No OTEL configuration provided");
        return 1; /* Success - OTEL is optional */
    }

    /* Parse configuration */
    g_otel_config = parse_otel_config(config_obj);
    if (!g_otel_config) {
        VALKEY_LOG_ERROR("otel_init", "Failed to parse OTEL configuration");
        return 0;
    }

    /* Initialize OTEL with Rust FFI */
    const char* error = init_open_telemetry(g_otel_config);
    if (error) {
        VALKEY_LOG_ERROR_FMT("otel_init", "Failed to initialize OTEL: %s", error);
        free_c_string((char*) error);
        cleanup_otel_config();
        return 0;
    }

    VALKEY_LOG_INFO("otel_init", "OpenTelemetry initialized successfully");
    return 1;
}

/**
 * Shutdown OpenTelemetry and cleanup configuration.
 * For testing purposes only - allows resetting OTEL state between tests.
 */
static void valkey_glide_otel_shutdown(void) {
    if (g_otel_config) {
        cleanup_otel_config();
        VALKEY_LOG_INFO("otel_shutdown", "OpenTelemetry shutdown complete");
    }
}

/**
 * Create a span for command tracing
 */
uint64_t valkey_glide_create_span(enum RequestType request_type) {
    if (!g_otel_config) {
        return 0;
    }
    if (!valkey_glide_otel_should_sample()) {
        return 0;
    }
    return create_otel_span(request_type);
}

/**
 * Drop a span
 */
void valkey_glide_drop_span(uint64_t span_ptr) {
    if (span_ptr != 0) {
        drop_otel_span(span_ptr);
    }
}

/**
 * Parse OTEL configuration from OpenTelemetryConfig object
 */
static struct OpenTelemetryConfig* parse_otel_config(zval* config_obj) {
    if (Z_TYPE_P(config_obj) != IS_OBJECT) {
        VALKEY_LOG_ERROR("otel_config",
                         "OpenTelemetry configuration must be an OpenTelemetryConfig object");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "OpenTelemetry configuration must be an OpenTelemetryConfig object",
                             0);
        return NULL;
    }

    zval                               method_name, retval, *traces_obj, *metrics_obj;
    int64_t                            flush_interval_ms  = 0;
    bool                               has_flush_interval = false;
    struct OpenTelemetryTracesConfig*  traces_config      = NULL;
    struct OpenTelemetryMetricsConfig* metrics_config     = NULL;

    // Get traces configuration
    ZVAL_STRING(&method_name, "getTraces");
    if (call_user_function(NULL, config_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_OBJECT) {
            traces_obj    = &retval;
            traces_config = parse_traces_config_object(traces_obj);
            if (!traces_config) {
                zval_dtor(&method_name);
                zval_dtor(&retval);
                return NULL;
            }
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    // Get metrics configuration
    ZVAL_STRING(&method_name, "getMetrics");
    if (call_user_function(NULL, config_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_OBJECT) {
            metrics_obj    = &retval;
            metrics_config = parse_metrics_config_object(metrics_obj);
            if (!metrics_config) {
                if (traces_config) {
                    if (traces_config->endpoint) {
                        efree((void*) traces_config->endpoint);
                    }
                    efree(traces_config);
                }
                zval_dtor(&method_name);
                zval_dtor(&retval);
                return NULL;
            }
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    // Get flush interval
    ZVAL_STRING(&method_name, "getFlushIntervalMs");
    if (call_user_function(NULL, config_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_LONG) {
            flush_interval_ms  = Z_LVAL(retval);
            has_flush_interval = true;
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    // Validate at least one config is provided
    if (!traces_config && !metrics_config) {
        VALKEY_LOG_ERROR("otel_config", "At least one of traces or metrics must be provided");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "At least one of traces or metrics must be provided",
                             0);
        return NULL;
    }

    // Create main FFI config struct
    struct OpenTelemetryConfig* main_config = emalloc(sizeof(struct OpenTelemetryConfig));
    main_config->traces                     = traces_config;
    main_config->metrics                    = metrics_config;
    main_config->has_flush_interval_ms      = has_flush_interval;
    main_config->flush_interval_ms          = flush_interval_ms;

    return main_config;
}

/**
 * Parse traces configuration from TracesConfig object
 */
static struct OpenTelemetryTracesConfig* parse_traces_config_object(zval* traces_obj) {
    zval  method_name, retval;
    char* endpoint              = NULL;
    int   sample_percentage     = 0;
    bool  has_sample_percentage = false;

    // Get endpoint
    ZVAL_STRING(&method_name, "getEndpoint");
    if (call_user_function(NULL, traces_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_STRING) {
            endpoint = estrdup(Z_STRVAL(retval));
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    // Get sample percentage
    ZVAL_STRING(&method_name, "getSamplePercentage");
    if (call_user_function(NULL, traces_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_LONG) {
            sample_percentage     = (int) Z_LVAL(retval);
            has_sample_percentage = true;
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    if (!endpoint) {
        VALKEY_LOG_ERROR("otel_config", "Traces endpoint is required");
        return NULL;
    }

    // Allocate and populate FFI traces config struct
    struct OpenTelemetryTracesConfig* traces_config =
        emalloc(sizeof(struct OpenTelemetryTracesConfig));
    traces_config->endpoint              = endpoint;  // Transfer ownership
    traces_config->has_sample_percentage = has_sample_percentage;
    traces_config->sample_percentage     = (uint32_t) sample_percentage;

    return traces_config;
}

/**
 * Parse metrics configuration from MetricsConfig object
 */
static struct OpenTelemetryMetricsConfig* parse_metrics_config_object(zval* metrics_obj) {
    zval  method_name, retval;
    char* endpoint = NULL;

    // Get endpoint
    ZVAL_STRING(&method_name, "getEndpoint");
    if (call_user_function(NULL, metrics_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_STRING) {
            endpoint = estrdup(Z_STRVAL(retval));
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    if (!endpoint) {
        VALKEY_LOG_ERROR("otel_config", "Metrics endpoint is required");
        return NULL;
    }

    // Allocate and populate FFI metrics config struct
    struct OpenTelemetryMetricsConfig* metrics_config =
        emalloc(sizeof(struct OpenTelemetryMetricsConfig));
    metrics_config->endpoint = endpoint;  // Transfer ownership

    return metrics_config;
}

/**
 * Cleanup OTEL configuration
 */
static void cleanup_otel_config(void) {
    if (!g_otel_config) {
        return;
    }

    if (g_otel_config->traces) {
        if (g_otel_config->traces->endpoint) {
            efree((void*) g_otel_config->traces->endpoint);
        }
        efree((void*) g_otel_config->traces);
    }

    if (g_otel_config->metrics) {
        if (g_otel_config->metrics->endpoint) {
            efree((void*) g_otel_config->metrics->endpoint);
        }
        efree((void*) g_otel_config->metrics);
    }

    efree(g_otel_config);
    g_otel_config = NULL;
}

static bool valkey_glide_otel_is_initialized(void) {
    return g_otel_config != NULL;
}

static int32_t valkey_glide_otel_get_sample_percentage(void) {
    if (!g_otel_config || !g_otel_config->traces) {
        return OTEL_SAMPLE_PERCENTAGE_INVALID;
    }
    return (int32_t) g_otel_config->traces->sample_percentage;
}

static bool valkey_glide_otel_set_sample_percentage(uint32_t percentage) {
    if (!g_otel_config || !g_otel_config->traces) {
        return false;  // Not initialized or no traces config
    }

    // Cast away const to modify - we own this memory
    ((struct OpenTelemetryTracesConfig*) g_otel_config->traces)->sample_percentage = percentage;
    return true;
}

static bool valkey_glide_otel_should_sample(void) {
    int32_t percentage = valkey_glide_otel_get_sample_percentage();
    return percentage != OTEL_SAMPLE_PERCENTAGE_INVALID && percentage > 0 &&
           (percentage == 100 || (rand() % 100) < percentage);
}
