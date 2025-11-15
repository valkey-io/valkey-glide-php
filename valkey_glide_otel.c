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

/**
 * Initialize OpenTelemetry with the given configuration
 * Can only be initialized once per process
 */
int valkey_glide_otel_init(zval* config_array) {
    if (g_otel_initialized) {
        VALKEY_LOG_WARN("otel_init",
                        "OpenTelemetry already initialized, ignoring subsequent calls");
        return 1; /* Success - already initialized */
    }

    if (!config_array || Z_TYPE_P(config_array) != IS_OBJECT) {
        VALKEY_LOG_DEBUG("otel_init", "No OTEL configuration provided");
        return 1; /* Success - OTEL is optional */
    }

    /* Parse configuration */
    if (!parse_otel_config(config_array, &g_otel_config)) {
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
void valkey_glide_otel_shutdown(void) {
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
int parse_otel_config(zval* config_obj, valkey_glide_otel_config_t* otel_config) {
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
int parse_otel_config_object(zval* config_obj, valkey_glide_otel_config_t* otel_config) {
    /* For now, just return an error since we're removing array support */
    /* TODO: Implement proper object method calling */
    VALKEY_LOG_ERROR("otel_config", "OpenTelemetry object configuration not yet implemented");
    zend_throw_exception(get_valkey_glide_exception_ce(),
                         "OpenTelemetry object configuration not yet implemented",
                         0);
    return 0;
}

/**
 * Parse traces configuration from TracesConfig object
 */
int parse_traces_config_object(zval* traces_obj, valkey_glide_otel_config_t* otel_config) {
    /* TODO: Implement proper object method calling */
    VALKEY_LOG_ERROR("otel_config", "Traces object configuration not yet implemented");
    return 0;
}

/**
 * Parse metrics configuration from MetricsConfig object
 */
int parse_metrics_config_object(zval* metrics_obj, valkey_glide_otel_config_t* otel_config) {
    /* TODO: Implement proper object method calling */
    VALKEY_LOG_ERROR("otel_config", "Metrics object configuration not yet implemented");
    return 0;
}

/**
 * Cleanup OTEL configuration
 */
void cleanup_otel_config(valkey_glide_otel_config_t* otel_config) {
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

/* Public API function implementations */

bool valkey_glide_otel_is_initialized(void) {
    return g_otel_config.enabled;
}

int32_t valkey_glide_otel_get_sample_percentage(void) {
    if (!g_otel_config.enabled || !g_otel_config.traces_config) {
        return -1;  // Not initialized or no traces config
    }
    return (int32_t) g_otel_config.traces_config->sample_percentage;
}

bool valkey_glide_otel_set_sample_percentage(int32_t percentage) {
    if (!g_otel_config.enabled || !g_otel_config.traces_config) {
        return false;  // Not initialized or no traces config
    }

    if (percentage < 0 || percentage > 100) {
        return false;  // Invalid percentage
    }

    g_otel_config.traces_config->sample_percentage = (uint32_t) percentage;
    return true;
}

bool valkey_glide_otel_should_sample(void) {
    int32_t percentage = valkey_glide_otel_get_sample_percentage();
    return percentage > 0 && (percentage == 100 || (rand() % 100) < percentage);
}

uint64_t valkey_glide_otel_create_named_span(const char* name) {
    if (!g_otel_config.enabled || !name || strlen(name) == 0) {
        return 0;  // Not initialized or invalid name
    }

    if (strlen(name) > 256) {
        return 0;  // Name too long
    }

    return create_named_otel_span(name);
}

bool valkey_glide_otel_end_span(uint64_t span_ptr) {
    if (span_ptr == 0) {
        return true;  // Safe no-op for zero pointer
    }

    drop_otel_span(span_ptr);
    return true;
}
