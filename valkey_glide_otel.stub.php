<?php

/**
 * OpenTelemetry configuration for Valkey GLIDE PHP client
 */
class ValkeyGlideOtel
{
    /**
     * Initialize OpenTelemetry with configuration
     * 
     * @param array $config OTEL configuration array
     * @return bool True on success, false on failure
     */
    public static function init(array $config): bool {}
    
    /**
     * Shutdown OpenTelemetry
     */
    public static function shutdown(): void {}
    
    /**
     * Check if OpenTelemetry is enabled
     * 
     * @return bool True if OTEL is enabled
     */
    public static function isEnabled(): bool {}
}

/**
 * OpenTelemetry traces configuration
 */
class ValkeyGlideOtelTracesConfig
{
    /**
     * Create traces configuration
     * 
     * @param string $endpoint Traces endpoint (grpc://host:port, http://host:port, file:///path)
     * @param int|null $sample_percentage Sampling percentage (0-100)
     */
    public function __construct(string $endpoint, ?int $sample_percentage = null) {}
}

/**
 * OpenTelemetry metrics configuration  
 */
class ValkeyGlideOtelMetricsConfig
{
    /**
     * Create metrics configuration
     * 
     * @param string $endpoint Metrics endpoint (grpc://host:port, http://host:port, file:///path)
     */
    public function __construct(string $endpoint) {}
}

/**
 * OpenTelemetry configuration
 */
class ValkeyGlideOtelConfig
{
    /**
     * Create OTEL configuration
     * 
     * @param ValkeyGlideOtelTracesConfig|null $traces Traces configuration
     * @param ValkeyGlideOtelMetricsConfig|null $metrics Metrics configuration
     * @param int|null $flush_interval_ms Flush interval in milliseconds
     */
    public function __construct(
        ?ValkeyGlideOtelTracesConfig $traces = null,
        ?ValkeyGlideOtelMetricsConfig $metrics = null,
        ?int $flush_interval_ms = null
    ) {}
}
