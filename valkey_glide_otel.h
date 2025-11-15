/*
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

#ifndef VALKEY_GLIDE_OTEL_H
#define VALKEY_GLIDE_OTEL_H

#include "include/glide_bindings.h"
#include "php.h"

/* OTEL configuration structure for PHP */
typedef struct {
    bool                               enabled;
    struct OpenTelemetryConfig*        config;
    struct OpenTelemetryTracesConfig*  traces_config;
    struct OpenTelemetryMetricsConfig* metrics_config;
    int32_t                            current_sample_percentage;
} valkey_glide_otel_config_t;

/* Global OTEL configuration */
extern valkey_glide_otel_config_t g_otel_config;

/* Function declarations */
int      valkey_glide_otel_init(zval* config_array);
void     valkey_glide_otel_shutdown(void);
uint64_t valkey_glide_create_span(enum RequestType request_type);
void     valkey_glide_drop_span(uint64_t span_ptr);

/* Public API functions */
bool     valkey_glide_otel_is_initialized(void);
int32_t  valkey_glide_otel_get_sample_percentage(void);
bool     valkey_glide_otel_set_sample_percentage(int32_t percentage);
bool     valkey_glide_otel_should_sample(void);
uint64_t valkey_glide_otel_create_named_span(const char* name);
bool     valkey_glide_otel_end_span(uint64_t span_ptr);

/* Helper functions */
int  parse_otel_config(zval* config_obj, valkey_glide_otel_config_t* otel_config);
void cleanup_otel_config(valkey_glide_otel_config_t* otel_config);

#endif /* VALKEY_GLIDE_OTEL_H */
