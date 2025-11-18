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

/* Global OTEL configuration */
valkey_glide_otel_config_t g_otel_config      = {0};
static bool                g_otel_initialized = false;

/* Sentinel value for invalid/uninitialized sample percentage */
#define OTEL_SAMPLE_PERCENTAGE_INVALID -1

/**
 * Initialize OpenTelemetry with the given configuration
 * Can only be initialized once per process
 */
int valkey_glide_otel_init(zval* config_obj) {
    if (g_otel_initialized) {
        VALKEY_LOG_WARN("otel_init",
                        "OpenTelemetry already initialized, ignoring subsequent calls");
        return 1; /* Success - already initialized */
    }

    if (!config_obj || Z_TYPE_P(config_obj) != IS_OBJECT) {
        VALKEY_LOG_DEBUG("otel_init", "No OTEL configuration provided");
        return 1; /* Success - OTEL is optional */
    }

    /* Parse configuration */
    if (!parse_otel_config(config_obj, &g_otel_config)) {
        VALKEY_LOG_ERROR("otel_init", "Failed to parse OTEL configuration");
        cleanup_otel_config(&g_otel_config);
        return 0;
    }

    /* Initialize OTEL with Rust FFI */
    const char* error = init_open_telemetry(g_otel_config.config);
    if (error) {
        VALKEY_LOG_ERROR_FMT("otel_init", "Failed to initialize OTEL: %s", error);
        free_c_string((char*) error);
        cleanup_otel_config(&g_otel_config);
        return 0;
    }

    g_otel_config.enabled = true;
    g_otel_initialized    = true;
    VALKEY_LOG_INFO("otel_init", "OpenTelemetry initialized successfully");
    return 1;
}

/**
 * Shutdown OpenTelemetry
 */
static void valkey_glide_otel_shutdown(void) {
    if (g_otel_config.enabled) {
        g_otel_config.enabled = false;
        g_otel_initialized    = false;
        VALKEY_LOG_INFO("otel_shutdown", "OpenTelemetry shutdown complete");
    }
}

/**
 * Create a span for command tracing
 */
uint64_t valkey_glide_create_span(enum RequestType request_type) {
    if (!g_otel_config.enabled) {
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
static int parse_otel_config(zval* config_obj, valkey_glide_otel_config_t* otel_config) {
    if (Z_TYPE_P(config_obj) != IS_OBJECT) {
        VALKEY_LOG_ERROR("otel_config",
                         "OpenTelemetry configuration must be an OpenTelemetryConfig object");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "OpenTelemetry configuration must be an OpenTelemetryConfig object",
                             0);
        return 0;
    }

    return parse_otel_config_object(config_obj, otel_config);
}

/**
 * Parse OTEL configuration from OpenTelemetryConfig object
 */
static int parse_otel_config_object(zval* config_obj, valkey_glide_otel_config_t* otel_config) {
    zval    method_name, retval, *traces_obj, *metrics_obj;
    int64_t flush_interval_ms  = 0;
    bool    has_flush_interval = false;

    // Get traces configuration
    ZVAL_STRING(&method_name, "getTraces");
    if (call_user_function(NULL, config_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_OBJECT) {
            traces_obj = &retval;
            if (!parse_traces_config_object(traces_obj, otel_config)) {
                zval_dtor(&method_name);
                zval_dtor(&retval);
                return 0;
            }
        }
        zval_dtor(&retval);
    }
    zval_dtor(&method_name);

    // Get metrics configuration
    ZVAL_STRING(&method_name, "getMetrics");
    if (call_user_function(NULL, config_obj, &method_name, &retval, 0, NULL) == SUCCESS) {
        if (Z_TYPE(retval) == IS_OBJECT) {
            metrics_obj = &retval;
            if (!parse_metrics_config_object(metrics_obj, otel_config)) {
                zval_dtor(&method_name);
                zval_dtor(&retval);
                return 0;
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
    if (!otel_config->traces_config && !otel_config->metrics_config) {
        VALKEY_LOG_ERROR("otel_config", "At least one of traces or metrics must be provided");
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "At least one of traces or metrics must be provided",
                             0);
        return 0;
    }

    // Create main FFI config struct
    struct OpenTelemetryConfig* main_config = emalloc(sizeof(struct OpenTelemetryConfig));
    main_config->traces                     = otel_config->traces_config;
    main_config->metrics                    = otel_config->metrics_config;
    main_config->has_flush_interval_ms      = has_flush_interval;
    main_config->flush_interval_ms          = flush_interval_ms;

    otel_config->config = main_config;

    return 1;
}

/**
 * Parse traces configuration from TracesConfig object
 */
static int parse_traces_config_object(zval* traces_obj, valkey_glide_otel_config_t* otel_config) {
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
        return 0;
    }

    // Allocate and populate FFI traces config struct
    struct OpenTelemetryTracesConfig* traces_config =
        emalloc(sizeof(struct OpenTelemetryTracesConfig));
    traces_config->endpoint              = endpoint;  // Transfer ownership
    traces_config->has_sample_percentage = has_sample_percentage;
    traces_config->sample_percentage     = (uint32_t) sample_percentage;

    otel_config->traces_config             = traces_config;
    otel_config->current_sample_percentage = sample_percentage;

    return 1;
}

/**
 * Parse metrics configuration from MetricsConfig object
 */
static int parse_metrics_config_object(zval* metrics_obj, valkey_glide_otel_config_t* otel_config) {
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
        return 0;
    }

    // Allocate and populate FFI metrics config struct
    struct OpenTelemetryMetricsConfig* metrics_config =
        emalloc(sizeof(struct OpenTelemetryMetricsConfig));
    metrics_config->endpoint = endpoint;  // Transfer ownership

    otel_config->metrics_config = metrics_config;

    return 1;
}

/**
 * Cleanup OTEL configuration
 */
static void cleanup_otel_config(valkey_glide_otel_config_t* otel_config) {
    if (otel_config->traces_config) {
        if (otel_config->traces_config->endpoint) {
            efree((void*) otel_config->traces_config->endpoint);
            otel_config->traces_config->endpoint = NULL;
        }
        efree(otel_config->traces_config);
        otel_config->traces_config = NULL;
    }

    if (otel_config->metrics_config) {
        if (otel_config->metrics_config->endpoint) {
            efree((void*) otel_config->metrics_config->endpoint);
            otel_config->metrics_config->endpoint = NULL;
        }
        efree(otel_config->metrics_config);
        otel_config->metrics_config = NULL;
    }

    if (otel_config->config) {
        /* Reset pointers to avoid double-free */
        otel_config->config->traces  = NULL;
        otel_config->config->metrics = NULL;
        efree(otel_config->config);
        otel_config->config = NULL;
    }

    otel_config->enabled                   = false;
    otel_config->current_sample_percentage = 0;
}

static bool valkey_glide_otel_is_initialized(void) {
    return g_otel_config.enabled;
}

static int32_t valkey_glide_otel_get_sample_percentage(void) {
    if (!g_otel_config.enabled || !g_otel_config.traces_config) {
        return OTEL_SAMPLE_PERCENTAGE_INVALID;
    }
    return (int32_t) g_otel_config.traces_config->sample_percentage;
}

static bool valkey_glide_otel_set_sample_percentage(uint32_t percentage) {
    if (!g_otel_config.enabled || !g_otel_config.traces_config) {
        return false;  // Not initialized or no traces config
    }

    g_otel_config.traces_config->sample_percentage = percentage;
    return true;
}

static bool valkey_glide_otel_should_sample(void) {
    int32_t percentage = valkey_glide_otel_get_sample_percentage();
    return percentage != OTEL_SAMPLE_PERCENTAGE_INVALID && percentage > 0 &&
           (percentage == 100 || (rand() % 100) < percentage);
}

static void valkey_glide_otel_end_span(uint64_t span_ptr) {
    if (span_ptr == 0) {
        return;  // Safe no-op for zero pointer
    }

    drop_otel_span(span_ptr);
}
