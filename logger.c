/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "logger.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/glide_bindings.h"

/* ============================================================================
 * Internal State Management - Singleton Pattern like Node.js Logger
 * ============================================================================ */

static bool       logger_initialized    = false;
static int        current_log_level     = VALKEY_LOG_LEVEL_DEFAULT;
static enum Level current_ffi_log_level = LEVEL_WARN; /* FFI level tracking */

/* Simple mutex simulation using static variable for initialization protection */
static volatile bool initialization_in_progress = false;

/* ============================================================================
 * Level Conversion Functions
 * ============================================================================ */

/**
 * Convert C integer log level to FFI Level enum
 */
static enum Level int_to_ffi_level(int level) {
    switch (level) {
        case VALKEY_LOG_LEVEL_ERROR:
            return LEVEL_ERROR;
        case VALKEY_LOG_LEVEL_WARN:
            return LEVEL_WARN;
        case VALKEY_LOG_LEVEL_INFO:
            return LEVEL_INFO;
        case VALKEY_LOG_LEVEL_DEBUG:
            return LEVEL_DEBUG;
        case VALKEY_LOG_LEVEL_TRACE:
            return LEVEL_TRACE;
        case VALKEY_LOG_LEVEL_OFF:
            return LEVEL_OFF;
        default:
            return LEVEL_WARN; /* Default fallback */
    }
}

/**
 * Convert FFI Level enum to C integer log level
 */
static int ffi_level_to_int(enum Level level) {
    switch (level) {
        case LEVEL_ERROR:
            return VALKEY_LOG_LEVEL_ERROR;
        case LEVEL_WARN:
            return VALKEY_LOG_LEVEL_WARN;
        case LEVEL_INFO:
            return VALKEY_LOG_LEVEL_INFO;
        case LEVEL_DEBUG:
            return VALKEY_LOG_LEVEL_DEBUG;
        case LEVEL_TRACE:
            return VALKEY_LOG_LEVEL_TRACE;
        case LEVEL_OFF:
            return VALKEY_LOG_LEVEL_OFF;
        default:
            return VALKEY_LOG_LEVEL_WARN; /* Default fallback */
    }
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

int valkey_glide_logger_level_from_string(const char* level_str) {
    if (level_str == NULL) {
        return VALKEY_LOG_LEVEL_DEFAULT;
    }

    if (strcmp(level_str, VALKEY_LOG_LEVEL_ERROR_STR) == 0) {
        return VALKEY_LOG_LEVEL_ERROR;
    } else if (strcmp(level_str, VALKEY_LOG_LEVEL_WARN_STR) == 0) {
        return VALKEY_LOG_LEVEL_WARN;
    } else if (strcmp(level_str, VALKEY_LOG_LEVEL_INFO_STR) == 0) {
        return VALKEY_LOG_LEVEL_INFO;
    } else if (strcmp(level_str, VALKEY_LOG_LEVEL_DEBUG_STR) == 0) {
        return VALKEY_LOG_LEVEL_DEBUG;
    } else if (strcmp(level_str, VALKEY_LOG_LEVEL_TRACE_STR) == 0) {
        return VALKEY_LOG_LEVEL_TRACE;
    } else if (strcmp(level_str, VALKEY_LOG_LEVEL_OFF_STR) == 0) {
        return VALKEY_LOG_LEVEL_OFF;
    }

    /* Invalid level string - return default */
    return VALKEY_LOG_LEVEL_DEFAULT;
}

bool valkey_glide_logger_is_initialized(void) {
    return logger_initialized;
}

int valkey_glide_logger_get_level(void) {
    return current_log_level;
}

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/**
 * Internal function to actually initialize the logger via FFI.
 * This centralizes the FFI call and state management.
 */
static int internal_init_logger(const char* level, const char* filename) {
    /* Prevent concurrent initialization attempts */
    if (initialization_in_progress) {
        return -1;
    }

    initialization_in_progress = true;

    int        level_int = valkey_glide_logger_level_from_string(level);
    enum Level ffi_level = int_to_ffi_level(level_int);

    /* Call the new FFI init function */

    enum Level actual_ffi_level = init(&ffi_level, filename);

    /* Update our state with the actual level set by FFI */
    current_ffi_log_level = actual_ffi_level;
    current_log_level     = ffi_level_to_int(actual_ffi_level);
    logger_initialized    = true;

    initialization_in_progress = false;
    return 0; /* FFI init always succeeds */
}

/**
 * Internal function to handle auto-initialization on first use.
 * This implements the Node.js Logger behavior where the first log attempt
 * initializes a new logger with default configuration if none exists.
 */
static void ensure_logger_initialized(void) {
    if (!logger_initialized && !initialization_in_progress) {
        /* Auto-initialize with default configuration like Node.js Logger */
        internal_init_logger(NULL, NULL);
    }
}

/* ============================================================================
 * PHP Interface Functions - matching Node.js Logger API
 * ============================================================================ */

int valkey_glide_logger_init(const char* level, const char* filename) {
    /*
     * This matches Node.js Logger.init() behavior:
     * Initialize only if it wasn't initialized before
     */

    if (logger_initialized) {
        return 0; /* Already initialized, return success */
    }

    return internal_init_logger(level, filename);
}

int valkey_glide_logger_set_config(const char* level, const char* filename) {
    /*
     * This matches Node.js Logger.setLoggerConfig() behavior:
     * Replace the existing configuration - always reinitialize
     */

    /* Reset state to allow reinitialization */
    logger_initialized = false;

    return internal_init_logger(level, filename);
}

void valkey_glide_logger_log(const char* level, const char* identifier, const char* message) {
    /* Auto-initialize if needed (Node.js Logger behavior) */
    ensure_logger_initialized();

    if (level == NULL || identifier == NULL || message == NULL) {
        return;
    }

    int        level_int = valkey_glide_logger_level_from_string(level);
    enum Level ffi_level = int_to_ffi_level(level_int);

    /* Call the new FFI log function with current logger level */
    valkey_log(ffi_level, current_ffi_log_level, identifier, message);
}

/* ============================================================================
 * Convenience Functions for PHP - matching Node.js Logger methods
 * ============================================================================ */

void valkey_glide_logger_error(const char* identifier, const char* message) {
    valkey_glide_logger_log(VALKEY_LOG_LEVEL_ERROR_STR, identifier, message);
}

void valkey_glide_logger_warn(const char* identifier, const char* message) {
    valkey_glide_logger_log(VALKEY_LOG_LEVEL_WARN_STR, identifier, message);
}

void valkey_glide_logger_info(const char* identifier, const char* message) {
    valkey_glide_logger_log(VALKEY_LOG_LEVEL_INFO_STR, identifier, message);
}

void valkey_glide_logger_debug(const char* identifier, const char* message) {
    valkey_glide_logger_log(VALKEY_LOG_LEVEL_DEBUG_STR, identifier, message);
}

/* ============================================================================
 * C Extension Interface Functions - Direct access for C code
 * ============================================================================ */

void valkey_glide_c_log_error(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Call the new FFI log function directly */
    valkey_log(LEVEL_ERROR, current_ffi_log_level, identifier, message);
}

void valkey_glide_c_log_warn(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Call the new FFI log function directly */
    valkey_log(LEVEL_WARN, current_ffi_log_level, identifier, message);
}

void valkey_glide_c_log_info(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Call the new FFI log function directly */
    valkey_log(LEVEL_INFO, current_ffi_log_level, identifier, message);
}

void valkey_glide_c_log_debug(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Call the new FFI log function directly */
    valkey_log(LEVEL_DEBUG, current_ffi_log_level, identifier, message);
}

void valkey_glide_c_log_trace(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Call the new FFI log function directly */
    valkey_log(LEVEL_TRACE, current_ffi_log_level, identifier, message);
}
