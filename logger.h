/* -*- Mode: C; tab-width: 4 -*- */
/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifndef VALKEY_GLIDE_LOGGER_H
#define VALKEY_GLIDE_LOGGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Log Level Constants - matching Node.js Logger and Rust LoggerLevel enum
 * ============================================================================ */

#define VALKEY_LOG_LEVEL_ERROR 0
#define VALKEY_LOG_LEVEL_WARN 1
#define VALKEY_LOG_LEVEL_INFO 2
#define VALKEY_LOG_LEVEL_DEBUG 3
#define VALKEY_LOG_LEVEL_TRACE 4
#define VALKEY_LOG_LEVEL_OFF 5

/* String representations of log levels */
#define VALKEY_LOG_LEVEL_ERROR_STR "error"
#define VALKEY_LOG_LEVEL_WARN_STR "warn"
#define VALKEY_LOG_LEVEL_INFO_STR "info"
#define VALKEY_LOG_LEVEL_DEBUG_STR "debug"
#define VALKEY_LOG_LEVEL_TRACE_STR "trace"
#define VALKEY_LOG_LEVEL_OFF_STR "off"

/* Default log level */
#define VALKEY_LOG_LEVEL_DEFAULT VALKEY_LOG_LEVEL_WARN

/* ============================================================================
 * External FFI Function Declarations
 * ============================================================================ */

/* FFI functions implemented in Rust (from valkey-glide/ffi/src/lib.rs) */
extern int  php_init_logger(int level, const char* filename);
extern void php_log_message(int level, const char* identifier, const char* message);
extern void c_log_error(const char* identifier, const char* message);
extern void c_log_warn(const char* identifier, const char* message);
extern void c_log_info(const char* identifier, const char* message);
extern void c_log_debug(const char* identifier, const char* message);
extern void c_log_trace(const char* identifier, const char* message);
extern int  c_init_logger_from_c(int level, const char* filename);

/* ============================================================================
 * PHP Interface Functions - matching Node.js Logger API
 * ============================================================================ */

/**
 * Initialize the logger if it wasn't initialized before.
 * This function matches Node.js Logger.init() behavior - it configures the logger
 * only if it wasn't previously configured.
 *
 * @param level Log level string ("error", "warn", "info", "debug", "trace", "off")
 *              or NULL for default (warn)
 * @param filename Optional filename for file logging, or NULL for console logging
 * @return 0 on success, -1 on error
 */
int valkey_glide_logger_init(const char* level, const char* filename);

/**
 * Set/replace the logger configuration.
 * This function matches Node.js Logger.setLoggerConfig() behavior - it replaces
 * the existing configuration, meaning new logs will not be saved with logs sent
 * before the call.
 *
 * @param level Log level string ("error", "warn", "info", "debug", "trace", "off")
 * @param filename Optional filename for file logging, or NULL for console logging
 * @return 0 on success, -1 on error
 */
int valkey_glide_logger_set_config(const char* level, const char* filename);

/**
 * Log a message from PHP code.
 * This function matches Node.js Logger.log() behavior.
 *
 * @param level Log level string ("error", "warn", "info", "debug", "trace")
 * @param identifier Context identifier for the log message
 * @param message The log message content
 */
void valkey_glide_logger_log(const char* level, const char* identifier, const char* message);

/* ============================================================================
 * Convenience Functions for PHP - matching Node.js Logger methods
 * ============================================================================ */

/**
 * Log an error message from PHP code.
 * Matches Node.js Logger.error() convenience method.
 */
void valkey_glide_logger_error(const char* identifier, const char* message);

/**
 * Log a warning message from PHP code.
 * Matches Node.js Logger.warn() convenience method.
 */
void valkey_glide_logger_warn(const char* identifier, const char* message);

/**
 * Log an info message from PHP code.
 * Matches Node.js Logger.info() convenience method.
 */
void valkey_glide_logger_info(const char* identifier, const char* message);

/**
 * Log a debug message from PHP code.
 * Matches Node.js Logger.debug() convenience method.
 */
void valkey_glide_logger_debug(const char* identifier, const char* message);

/* ============================================================================
 * C Extension Interface Functions - Direct access for C code
 * ============================================================================ */

/**
 * Log an error message from C extension code.
 * Direct wrapper around FFI c_log_error function.
 */
void valkey_glide_c_log_error(const char* identifier, const char* message);

/**
 * Log a warning message from C extension code.
 * Direct wrapper around FFI c_log_warn function.
 */
void valkey_glide_c_log_warn(const char* identifier, const char* message);

/**
 * Log an info message from C extension code.
 * Direct wrapper around FFI c_log_info function.
 */
void valkey_glide_c_log_info(const char* identifier, const char* message);

/**
 * Log a debug message from C extension code.
 * Direct wrapper around FFI c_log_debug function.
 */
void valkey_glide_c_log_debug(const char* identifier, const char* message);

/**
 * Log a trace message from C extension code.
 * Direct wrapper around FFI c_log_trace function.
 */
void valkey_glide_c_log_trace(const char* identifier, const char* message);

/* ============================================================================
 * Convenience Macros for C Extension Code
 * ============================================================================ */

#define VALKEY_LOG_ERROR(identifier, message) valkey_glide_c_log_error(identifier, message)
#define VALKEY_LOG_WARN(identifier, message) valkey_glide_c_log_warn(identifier, message)
#define VALKEY_LOG_INFO(identifier, message) valkey_glide_c_log_info(identifier, message)
#define VALKEY_LOG_DEBUG(identifier, message) valkey_glide_c_log_debug(identifier, message)
#define VALKEY_LOG_TRACE(identifier, message) valkey_glide_c_log_trace(identifier, message)

/* Base macro for formatted logging with dynamic allocation */
#define VALKEY_LOG_FMT_BASE(level_constant, level_function, level_name, category, format, ...)  \
    do {                                                                                        \
        if (valkey_glide_logger_get_level() == VALKEY_LOG_LEVEL_OFF ||                          \
            level_constant > valkey_glide_logger_get_level())                                   \
            break;                                                                              \
        int   needed_size = snprintf(NULL, 0, format, __VA_ARGS__) + 1;                         \
        char* log_msg     = emalloc(needed_size);                                               \
        if (log_msg) {                                                                          \
            snprintf(log_msg, needed_size, format, __VA_ARGS__);                                \
            level_function(category, log_msg);                                                  \
            efree(log_msg);                                                                     \
        } else {                                                                                \
            VALKEY_LOG_ERROR(category, "Failed to allocate memory for " level_name " message"); \
        }                                                                                       \
    } while (0)

/* Dynamic allocation macro for formatted debug logging */
#define VALKEY_LOG_DEBUG_FMT(category, format, ...) \
    VALKEY_LOG_FMT_BASE(VALKEY_LOG_LEVEL_DEBUG,     \
                        VALKEY_LOG_DEBUG,           \
                        VALKEY_LOG_LEVEL_DEBUG_STR, \
                        category,                   \
                        format,                     \
                        __VA_ARGS__)

/* Dynamic allocation macro for formatted error logging */
#define VALKEY_LOG_ERROR_FMT(category, format, ...) \
    VALKEY_LOG_FMT_BASE(VALKEY_LOG_LEVEL_ERROR,     \
                        VALKEY_LOG_ERROR,           \
                        VALKEY_LOG_LEVEL_ERROR_STR, \
                        category,                   \
                        format,                     \
                        __VA_ARGS__)

/* Dynamic allocation macro for formatted warning logging */
#define VALKEY_LOG_WARN_FMT(category, format, ...) \
    VALKEY_LOG_FMT_BASE(VALKEY_LOG_LEVEL_WARN,     \
                        VALKEY_LOG_WARN,           \
                        VALKEY_LOG_LEVEL_WARN_STR, \
                        category,                  \
                        format,                    \
                        __VA_ARGS__)

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Convert string log level to integer constant.
 * Used internally for FFI layer communication.
 *
 * @param level_str Log level string
 * @return Integer log level constant, or VALKEY_LOG_LEVEL_DEFAULT if invalid
 */
int valkey_glide_logger_level_from_string(const char* level_str);

/**
 * Check if the logger has been initialized.
 * Used to implement Node.js-style initialization behavior.
 *
 * @return true if logger is initialized, false otherwise
 */
bool valkey_glide_logger_is_initialized(void);

/**
 * Get the current log level as an integer.
 *
 * @return Current log level constant
 */
int valkey_glide_logger_get_level(void);

#ifdef __cplusplus
}
#endif

#endif /* VALKEY_GLIDE_LOGGER_H */
