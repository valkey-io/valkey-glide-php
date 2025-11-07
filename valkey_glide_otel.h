#ifndef VALKEY_GLIDE_OTEL_H
#define VALKEY_GLIDE_OTEL_H

#include "php.h"
#include "include/glide_bindings.h"

/* OTEL configuration structure for PHP */
typedef struct {
    bool                                 enabled;
    struct OpenTelemetryConfig*          config;
    struct OpenTelemetryTracesConfig*    traces_config;
    struct OpenTelemetryMetricsConfig*   metrics_config;
} valkey_glide_otel_config_t;

/* Global OTEL configuration */
extern valkey_glide_otel_config_t g_otel_config;

/* Function declarations */
int  valkey_glide_otel_init(zval* config_array);
void valkey_glide_otel_shutdown(void);
uint64_t valkey_glide_create_span(enum RequestType request_type);
void valkey_glide_drop_span(uint64_t span_ptr);

/* Helper functions */
int parse_otel_config_array(zval* config_array, valkey_glide_otel_config_t* otel_config);
void cleanup_otel_config(valkey_glide_otel_config_t* otel_config);

#endif /* VALKEY_GLIDE_OTEL_H */
