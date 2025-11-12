#include "valkey_glide_core_common.h"
#include "valkey_glide_otel.h"
#include "zend_exceptions.h"

/* PHP method implementations for OpenTelemetry public APIs */

/* ValkeyGlide::initOpenTelemetry */
PHP_METHOD(ValkeyGlide, initOpenTelemetry) {
    zval* config_array;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_ARRAY(config_array)
    ZEND_PARSE_PARAMETERS_END();

    if (valkey_glide_otel_is_initialized()) {
        RETURN_BOOL(false);  // Already initialized
    }

    int result = valkey_glide_otel_init(config_array);
    if (result == 0) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Failed to initialize OpenTelemetry", 0);
        RETURN_BOOL(false);
    }

    RETURN_BOOL(true);
}

/* ValkeyGlide::isOpenTelemetryInitialized */
PHP_METHOD(ValkeyGlide, isOpenTelemetryInitialized) {
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_BOOL(valkey_glide_otel_is_initialized());
}

/* ValkeyGlide::getOpenTelemetrySamplePercentage */
PHP_METHOD(ValkeyGlide, getOpenTelemetrySamplePercentage) {
    ZEND_PARSE_PARAMETERS_NONE();

    int32_t percentage = valkey_glide_otel_get_sample_percentage();
    if (percentage < 0) {
        RETURN_NULL();
    }

    RETURN_LONG(percentage);
}

/* ValkeyGlide::setOpenTelemetrySamplePercentage */
PHP_METHOD(ValkeyGlide, setOpenTelemetrySamplePercentage) {
    zend_long percentage;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(percentage)
    ZEND_PARSE_PARAMETERS_END();

    if (percentage < 0 || percentage > 100) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Sample percentage must be between 0 and 100", 0);
        RETURN_BOOL(false);
    }

    bool result = valkey_glide_otel_set_sample_percentage((int32_t) percentage);
    if (!result) {
        zend_throw_exception(get_valkey_glide_exception_ce(),
                             "OpenTelemetry not initialized or traces not configured",
                             0);
        RETURN_BOOL(false);
    }

    RETURN_BOOL(true);
}

/* ValkeyGlide::shouldSample */
PHP_METHOD(ValkeyGlide, shouldSample) {
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_BOOL(valkey_glide_otel_should_sample());
}

/* ValkeyGlide::createOpenTelemetrySpan */
PHP_METHOD(ValkeyGlide, createOpenTelemetrySpan) {
    zend_string* name;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    if (ZSTR_LEN(name) == 0) {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Span name cannot be empty", 0);
        RETURN_NULL();
    }

    if (ZSTR_LEN(name) > 256) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), "Span name too long (maximum 256 characters)", 0);
        RETURN_NULL();
    }

    uint64_t span_ptr = valkey_glide_otel_create_named_span(ZSTR_VAL(name));
    if (span_ptr == 0) {
        RETURN_NULL();
    }

    RETURN_LONG((zend_long) span_ptr);
}

/* ValkeyGlide::endOpenTelemetrySpan */
PHP_METHOD(ValkeyGlide, endOpenTelemetrySpan) {
    zend_long span_ptr;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(span_ptr)
    ZEND_PARSE_PARAMETERS_END();

    bool result = valkey_glide_otel_end_span((uint64_t) span_ptr);
    RETURN_BOOL(result);
}

/* ValkeyGlideCluster method implementations - identical to ValkeyGlide */

/* ValkeyGlideCluster::initOpenTelemetry */
PHP_METHOD(ValkeyGlideCluster, initOpenTelemetry) {
    zval* config_array;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_ARRAY(config_array)
    ZEND_PARSE_PARAMETERS_END();

    if (valkey_glide_otel_is_initialized()) {
        RETURN_BOOL(false);  // Already initialized
    }

    int result = valkey_glide_otel_init(config_array);
    if (result == 0) {
        zend_throw_exception(
            get_valkey_glide_cluster_exception_ce(), "Failed to initialize OpenTelemetry", 0);
        RETURN_BOOL(false);
    }

    RETURN_BOOL(true);
}

/* ValkeyGlideCluster::isOpenTelemetryInitialized */
PHP_METHOD(ValkeyGlideCluster, isOpenTelemetryInitialized) {
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_BOOL(valkey_glide_otel_is_initialized());
}

/* ValkeyGlideCluster::getOpenTelemetrySamplePercentage */
PHP_METHOD(ValkeyGlideCluster, getOpenTelemetrySamplePercentage) {
    ZEND_PARSE_PARAMETERS_NONE();

    int32_t percentage = valkey_glide_otel_get_sample_percentage();
    if (percentage < 0) {
        RETURN_NULL();
    }

    RETURN_LONG(percentage);
}

/* ValkeyGlideCluster::setOpenTelemetrySamplePercentage */
PHP_METHOD(ValkeyGlideCluster, setOpenTelemetrySamplePercentage) {
    zend_long percentage;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(percentage)
    ZEND_PARSE_PARAMETERS_END();

    if (percentage < 0 || percentage > 100) {
        zend_throw_exception(get_valkey_glide_cluster_exception_ce(),
                             "Sample percentage must be between 0 and 100",
                             0);
        RETURN_BOOL(false);
    }

    bool result = valkey_glide_otel_set_sample_percentage((int32_t) percentage);
    if (!result) {
        zend_throw_exception(get_valkey_glide_cluster_exception_ce(),
                             "OpenTelemetry not initialized or traces not configured",
                             0);
        RETURN_BOOL(false);
    }

    RETURN_BOOL(true);
}

/* ValkeyGlideCluster::shouldSample */
PHP_METHOD(ValkeyGlideCluster, shouldSample) {
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_BOOL(valkey_glide_otel_should_sample());
}

/* ValkeyGlideCluster::createOpenTelemetrySpan */
PHP_METHOD(ValkeyGlideCluster, createOpenTelemetrySpan) {
    zend_string* name;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    if (ZSTR_LEN(name) == 0) {
        zend_throw_exception(
            get_valkey_glide_cluster_exception_ce(), "Span name cannot be empty", 0);
        RETURN_NULL();
    }

    if (ZSTR_LEN(name) > 256) {
        zend_throw_exception(get_valkey_glide_cluster_exception_ce(),
                             "Span name too long (maximum 256 characters)",
                             0);
        RETURN_NULL();
    }

    uint64_t span_ptr = valkey_glide_otel_create_named_span(ZSTR_VAL(name));
    if (span_ptr == 0) {
        RETURN_NULL();
    }

    RETURN_LONG((zend_long) span_ptr);
}

/* ValkeyGlideCluster::endOpenTelemetrySpan */
PHP_METHOD(ValkeyGlideCluster, endOpenTelemetrySpan) {
    zend_long span_ptr;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(span_ptr)
    ZEND_PARSE_PARAMETERS_END();

    bool result = valkey_glide_otel_end_span((uint64_t) span_ptr);
    RETURN_BOOL(result);
}
