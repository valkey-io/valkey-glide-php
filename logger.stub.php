<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

/**
 * Initialize the ValkeyGlide logger if it wasn't initialized before.
 * 
 * This function matches Node.js Logger.init() behavior - it configures the logger
 * only if it wasn't previously configured. The logger will be used by all components
 * (PHP, C extension, and Rust core) for unified logging.
 *
 * @param string|null $level Log level: "error", "warn", "info", "debug", "trace", "off"
 *                           If null, defaults to "warn"
 * @param string|null $filename Optional filename for file logging. If null, logs to console
 * @return bool True on success, false on error
 * 
 * @see https://github.com/valkey-io/valkey-glide/blob/main/node/src/Logger.ts
 */
function valkey_glide_logger_init(?string $level = null, ?string $filename = null): bool{}

/**
 * Set/replace the logger configuration.
 * 
 * This function matches Node.js Logger.setLoggerConfig() behavior - it replaces
 * the existing configuration, meaning new logs will not be saved with logs sent
 * before the call. The previous logs will remain unchanged.
 *
 * @param string $level Log level: "error", "warn", "info", "debug", "trace", "off"
 * @param string|null $filename Optional filename for file logging. If null, logs to console
 * @return bool True on success, false on error
 * 
 * @see https://github.com/valkey-io/valkey-glide/blob/main/node/src/Logger.ts
 */
function valkey_glide_logger_set_config(string $level, ?string $filename = null): bool{}

/**
 * Log a message from PHP code.
 * 
 * This function matches Node.js Logger.log() behavior. It will automatically
 * initialize the logger with default configuration if not already initialized.
 *
 * @param string $level Log level: "error", "warn", "info", "debug", "trace"
 * @param string $identifier Context identifier for the log message
 * @param string $message The log message content
 * @return void
 * 
 * @see https://github.com/valkey-io/valkey-glide/blob/main/node/src/Logger.ts
 */
function valkey_glide_logger_log(string $level, string $identifier, string $message): void{}

/**
 * Log an error message from PHP code.
 * 
 * Convenience function that matches Node.js Logger.error() behavior.
 * Automatically initializes logger if needed.
 *
 * @param string $identifier Context identifier for the log message
 * @param string $message The error message content
 * @return void
 */
function valkey_glide_logger_error(string $identifier, string $message): void{}

/**
 * Log a warning message from PHP code.
 * 
 * Convenience function that matches Node.js Logger.warn() behavior.
 * Automatically initializes logger if needed.
 *
 * @param string $identifier Context identifier for the log message
 * @param string $message The warning message content
 * @return void
 */
function valkey_glide_logger_warn(string $identifier, string $message): void{}

/**
 * Log an info message from PHP code.
 * 
 * Convenience function that matches Node.js Logger.info() behavior.
 * Automatically initializes logger if needed.
 *
 * @param string $identifier Context identifier for the log message
 * @param string $message The info message content
 * @return void
 */
function valkey_glide_logger_info(string $identifier, string $message): void{}

/**
 * Log a debug message from PHP code.
 * 
 * Convenience function that matches Node.js Logger.debug() behavior.
 * Automatically initializes logger if needed.
 *
 * @param string $identifier Context identifier for the log message
 * @param string $message The debug message content
 * @return void
 */
function valkey_glide_logger_debug(string $identifier, string $message): void{}

/**
 * Check if the logger has been initialized.
 * 
 * Used to implement Node.js-style initialization behavior where the first
 * log attempt initializes the logger with default configuration.
 *
 * @return bool True if logger is initialized, false otherwise
 */
function valkey_glide_logger_is_initialized(): bool{}

/**
 * Get the current log level as an integer.
 * 
 * Returns the current log level:
 * - 0 = Error
 * - 1 = Warn  
 * - 2 = Info
 * - 3 = Debug
 * - 4 = Trace
 * - 5 = Off
 *
 * @return int Current log level constant
 */
function valkey_glide_logger_get_level(): int{}
