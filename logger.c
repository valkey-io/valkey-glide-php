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

static bool logger_initialized = false;
static int  current_log_level  = VALKEY_LOG_LEVEL_DEFAULT;

static enum Level current_ffi_log_level = WARN; /* FFI level tracking */


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
            return ERROR;
        case VALKEY_LOG_LEVEL_WARN:
            return WARN;
        case VALKEY_LOG_LEVEL_INFO:
            return INFO;
        case VALKEY_LOG_LEVEL_DEBUG:
            return DEBUG;
        case VALKEY_LOG_LEVEL_TRACE:
            return TRACE;
        case VALKEY_LOG_LEVEL_OFF:
            return OFF;
        default:
            return WARN; /* Default fallback */
    }
}

/**
 * Convert FFI Level enum to C integer log level
 */
static int ffi_level_to_int(enum Level level) {
    switch (level) {
        case ERROR:
            return VALKEY_LOG_LEVEL_ERROR;
        case WARN:
            return VALKEY_LOG_LEVEL_WARN;
        case INFO:
            return VALKEY_LOG_LEVEL_INFO;
        case DEBUG:
            return VALKEY_LOG_LEVEL_DEBUG;
        case TRACE:
            return VALKEY_LOG_LEVEL_TRACE;
        case OFF:
            return VALKEY_LOG_LEVEL_OFF;
        default:
            return VALKEY_LOG_LEVEL_WARN; /* Default fallback */
    }
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */
void valkey_glide_log_wrapper(enum Level level, const char* identifier, const char* message) {
    struct LogResult* log_result = glide_log(level, identifier, message);
    if (log_result != NULL) {
        if (log_result->log_error != NULL) {
            if (level == ERROR) {
                fprintf(stderr, "Log error: %s\n", log_result->log_error);
            } else {
                valkey_glide_log_wrapper(ERROR, identifier, "Failed to log message");
            }
        }
        free_log_result(log_result);
    }
}


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

    /* Call the FFI init function */
    struct LogResult* log_result = init(&ffi_level, filename);

    if (log_result == NULL) {
        initialization_in_progress = false;
        fprintf(stderr, "Failed to initialize logger: NULL result\n");
        return -1; /* Failed to get result */
    }

    /* Check for initialization error */
    if (log_result->log_error != NULL) {
        /* Initialization failed */
        fprintf(stderr, "Failed to initialize logger: ERROR result, %s\n", log_result->log_error);
        free_log_result(log_result);
        initialization_in_progress = false;
        return -1;
    }

    /* Update our state with the actual level set by FFI */
    current_ffi_log_level = log_result->level;
    current_log_level     = ffi_level_to_int(log_result->level);
    logger_initialized    = true;

    /* Clean up the LogResult */
    free_log_result(log_result);

    initialization_in_progress = false;

    return 0; /* Success */
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

    int level_int = valkey_glide_logger_level_from_string(level);

    enum Level ffi_level = int_to_ffi_level(level_int);

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || ffi_level > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(ffi_level, identifier, message);
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

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || ERROR > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(ERROR, identifier, message);
}

void valkey_glide_c_log_warn(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || WARN > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(WARN, identifier, message);
}

void valkey_glide_c_log_info(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || INFO > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(INFO, identifier, message);
}

void valkey_glide_c_log_debug(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || DEBUG > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(DEBUG, identifier, message);
}

void valkey_glide_c_log_trace(const char* identifier, const char* message) {
    /* Auto-initialize if needed */
    ensure_logger_initialized();

    if (identifier == NULL || message == NULL) {
        return;
    }

    /* Check if message level is at or above current log level */
    if (current_ffi_log_level == OFF || TRACE > current_ffi_log_level) {
        return; /* Don't log if level is below threshold or logging is off */
    }

    /* Call the FFI log function and handle result */
    valkey_glide_log_wrapper(TRACE, identifier, message);
}
