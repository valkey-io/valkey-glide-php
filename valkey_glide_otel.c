#include "valkey_glide_otel.h"

#include <string.h>

#include "logger.h"

/* Global OTEL configuration */
static valkey_glide_otel_config_t g_otel_config      = {0};
static bool                       g_otel_initialized = false;

/**
 * Initialize OpenTelemetry with the given configuration
 * Following Java/Go pattern: can only be initialized once per process
 */
int valkey_glide_otel_init(zval* config_array) {
    if (g_otel_initialized) {
        VALKEY_LOG_WARN("otel_init",
                        "OpenTelemetry already initialized, ignoring subsequent calls");
        return 1; /* Success - already initialized */
    }

    if (!config_array || Z_TYPE_P(config_array) != IS_ARRAY) {
        VALKEY_LOG_DEBUG("otel_init", "No OTEL configuration provided");
        return 1; /* Success - OTEL is optional */
    }

    /* Parse configuration */
    if (!parse_otel_config_array(config_array, &g_otel_config)) {
        VALKEY_LOG_ERROR("otel_init", "Failed to parse OTEL configuration");
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
        cleanup_otel_config(&g_otel_config);
        memset(&g_otel_config, 0, sizeof(g_otel_config));
        g_otel_initialized = false;
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
 * Parse OTEL configuration from PHP array
 * Following Java/Go validation patterns
 */
int parse_otel_config_array(zval* config_array, valkey_glide_otel_config_t* otel_config) {
    HashTable* ht = Z_ARRVAL_P(config_array);

    /* Allocate main config */
    otel_config->config = ecalloc(1, sizeof(struct OpenTelemetryConfig));

    /* Set default flush interval (5000ms like Java/Go) */
    otel_config->config->has_flush_interval_ms = true;
    otel_config->config->flush_interval_ms     = 5000;

    /* Parse traces configuration */
    zval* traces_val = zend_hash_str_find(ht, "traces", sizeof("traces") - 1);
    if (traces_val && Z_TYPE_P(traces_val) == IS_ARRAY) {
        otel_config->traces_config = ecalloc(1, sizeof(struct OpenTelemetryTracesConfig));
        HashTable* traces_ht       = Z_ARRVAL_P(traces_val);

        /* Parse endpoint */
        zval* endpoint_val = zend_hash_str_find(traces_ht, "endpoint", sizeof("endpoint") - 1);
        if (endpoint_val && Z_TYPE_P(endpoint_val) == IS_STRING) {
            otel_config->traces_config->endpoint = estrdup(Z_STRVAL_P(endpoint_val));
        } else {
            VALKEY_LOG_ERROR("otel_config",
                             "Traces endpoint is required when traces config is provided");
            return 0;
        }

        /* Parse sample_percentage with default of 1% (like Java/Go) */
        zval* sample_val =
            zend_hash_str_find(traces_ht, "sample_percentage", sizeof("sample_percentage") - 1);
        if (sample_val && Z_TYPE_P(sample_val) == IS_LONG) {
            long sample_pct = Z_LVAL_P(sample_val);
            if (sample_pct < 0 || sample_pct > 100) {
                VALKEY_LOG_ERROR("otel_config", "Sample percentage must be between 0 and 100");
                return 0;
            }
            otel_config->traces_config->has_sample_percentage = true;
            otel_config->traces_config->sample_percentage     = (uint32_t) sample_pct;
        } else {
            /* Default to 1% like Java/Go */
            otel_config->traces_config->has_sample_percentage = true;
            otel_config->traces_config->sample_percentage     = 1;
        }

        otel_config->config->traces = otel_config->traces_config;
    }

    /* Parse metrics configuration */
    zval* metrics_val = zend_hash_str_find(ht, "metrics", sizeof("metrics") - 1);
    if (metrics_val && Z_TYPE_P(metrics_val) == IS_ARRAY) {
        otel_config->metrics_config = ecalloc(1, sizeof(struct OpenTelemetryMetricsConfig));
        HashTable* metrics_ht       = Z_ARRVAL_P(metrics_val);

        /* Parse endpoint */
        zval* endpoint_val = zend_hash_str_find(metrics_ht, "endpoint", sizeof("endpoint") - 1);
        if (endpoint_val && Z_TYPE_P(endpoint_val) == IS_STRING) {
            otel_config->metrics_config->endpoint = estrdup(Z_STRVAL_P(endpoint_val));
        } else {
            VALKEY_LOG_ERROR("otel_config",
                             "Metrics endpoint is required when metrics config is provided");
            return 0;
        }

        otel_config->config->metrics = otel_config->metrics_config;
    }

    /* Parse flush_interval_ms (override default if provided) */
    zval* flush_val = zend_hash_str_find(ht, "flush_interval_ms", sizeof("flush_interval_ms") - 1);
    if (flush_val && Z_TYPE_P(flush_val) == IS_LONG) {
        long flush_ms = Z_LVAL_P(flush_val);
        if (flush_ms <= 0) {
            VALKEY_LOG_ERROR("otel_config", "Flush interval must be a positive integer");
            return 0;
        }
        otel_config->config->flush_interval_ms = (uint32_t) flush_ms;
    }

    /* Validate at least one of traces or metrics is configured (like Java/Go) */
    if (!otel_config->config->traces && !otel_config->config->metrics) {
        VALKEY_LOG_ERROR("otel_config", "At least one of traces or metrics must be configured");
        return 0;
    }

    return 1;
}

/**
 * Cleanup OTEL configuration
 */
void cleanup_otel_config(valkey_glide_otel_config_t* otel_config) {
    if (otel_config->traces_config) {
        if (otel_config->traces_config->endpoint) {
            efree((void*) otel_config->traces_config->endpoint);
        }
        efree(otel_config->traces_config);
        otel_config->traces_config = NULL;
    }

    if (otel_config->metrics_config) {
        if (otel_config->metrics_config->endpoint) {
            efree((void*) otel_config->metrics_config->endpoint);
        }
        efree(otel_config->metrics_config);
        otel_config->metrics_config = NULL;
    }

    if (otel_config->config) {
        efree(otel_config->config);
        otel_config->config = NULL;
    }

    otel_config->enabled = false;
}
