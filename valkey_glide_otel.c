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

    /* Parse configuration - caller has validated it's a non-null object */
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
 * Set the OpenTelemetry sample percentage at runtime
 * Updates the PHP/C side configuration
 */
void valkey_glide_otel_set_sample_percentage(uint32_t percentage) {
    if (!g_otel_config || !g_otel_config->traces) {
        VALKEY_LOG_ERROR("otel_set_sample",
                         "OpenTelemetry not initialized or traces not configured");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "OpenTelemetry not initialized or traces not configured",
                             0);
        return;
    }

    if (percentage > 100) {
        VALKEY_LOG_ERROR_FMT(
            "otel_set_sample", "Sample percentage must be 0-100, got %u", percentage);
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Sample percentage must be between 0 and 100", 0);
        return;
    }

    /* Cast away const to modify the sample_percentage field */
    struct OpenTelemetryTracesConfig* mutable_traces =
        (struct OpenTelemetryTracesConfig*) g_otel_config->traces;
    mutable_traces->sample_percentage = percentage;

    VALKEY_LOG_DEBUG_FMT("otel_set_sample", "Sample percentage updated to %u", percentage);
}

/**
 * Get the OpenTelemetry sample percentage
 * Returns 1 if successful, 0 if not initialized
 */
int valkey_glide_otel_get_sample_percentage(uint32_t* percentage) {
    if (!g_otel_config || !g_otel_config->traces) {
        return 0;
    }
    *percentage = g_otel_config->traces->sample_percentage;
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
 * Helper function to call a method on an object and return the result
 * Caller is responsible for cleaning up retval if function returns true
 */
static bool call_method(zval* obj, const char* method_name, zval* retval) {
    zval method_name_zval;
    ZVAL_STRING(&method_name_zval, method_name);
    bool success = call_user_function(NULL, obj, &method_name_zval, retval, 0, NULL) == SUCCESS;
    zval_dtor(&method_name_zval);
    return success;
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

    zval                               retval, *traces_obj, *metrics_obj;
    int64_t                            flush_interval_ms  = 0;
    bool                               has_flush_interval = false;
    struct OpenTelemetryTracesConfig*  traces_config      = NULL;
    struct OpenTelemetryMetricsConfig* metrics_config     = NULL;

    // Get traces configuration
    if (call_method(config_obj, "getTraces", &retval)) {
        if (Z_TYPE(retval) == IS_OBJECT) {
            traces_obj    = &retval;
            traces_config = parse_traces_config_object(traces_obj);
            if (!traces_config) {
                zval_dtor(&retval);
                return NULL;
            }
        }
        zval_dtor(&retval);
    }

    // Get metrics configuration
    if (call_method(config_obj, "getMetrics", &retval)) {
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
                zval_dtor(&retval);
                return NULL;
            }
        }
        zval_dtor(&retval);
    }

    // Get flush interval
    if (call_method(config_obj, "getFlushIntervalMs", &retval)) {
        if (Z_TYPE(retval) == IS_LONG) {
            flush_interval_ms  = Z_LVAL(retval);
            has_flush_interval = true;
        }
        zval_dtor(&retval);
    }

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
    zval  retval;
    char* endpoint              = NULL;
    int   sample_percentage     = 0;
    bool  has_sample_percentage = false;

    // Get endpoint
    if (call_method(traces_obj, "getEndpoint", &retval)) {
        if (Z_TYPE(retval) == IS_STRING) {
            endpoint = estrdup(Z_STRVAL(retval));
        }
        zval_dtor(&retval);
    }

    // Get sample percentage
    if (call_method(traces_obj, "getSamplePercentage", &retval)) {
        if (Z_TYPE(retval) == IS_LONG) {
            sample_percentage     = (int) Z_LVAL(retval);
            has_sample_percentage = true;
        }
        zval_dtor(&retval);
    }

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
    zval  retval;
    char* endpoint = NULL;

    // Get endpoint
    if (call_method(metrics_obj, "getEndpoint", &retval)) {
        if (Z_TYPE(retval) == IS_STRING) {
            endpoint = estrdup(Z_STRVAL(retval));
        }
        zval_dtor(&retval);
    }

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

static bool valkey_glide_otel_should_sample(void) {
    if (!g_otel_config || !g_otel_config->traces) {
        return false;
    }
    uint32_t percentage = g_otel_config->traces->sample_percentage;
    return percentage > 0 && (percentage == 100 || (rand() % 100) < percentage);
}
