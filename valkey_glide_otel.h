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

/* Global OTEL configuration */
extern struct OpenTelemetryConfig* g_otel_config;

/* Function declarations */
int      valkey_glide_otel_init(zval* config_obj);
void     valkey_glide_otel_set_sample_percentage(uint32_t percentage);
int      valkey_glide_otel_get_sample_percentage(uint32_t* percentage);
uint64_t valkey_glide_create_span(enum RequestType request_type);
void     valkey_glide_drop_span(uint64_t span_ptr);

#endif /* VALKEY_GLIDE_OTEL_H */
