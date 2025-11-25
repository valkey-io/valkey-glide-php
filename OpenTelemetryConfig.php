<?php

namespace ValkeyGlide\OpenTelemetry;

/**
 * Configuration for OpenTelemetry integration.
 */
class OpenTelemetryConfig
{
    private ?TracesConfig $traces = null;
    private ?MetricsConfig $metrics = null;
    private ?int $flushIntervalMs = null;

    /**
     * Creates a new OpenTelemetryConfig from a builder.
     *
     * @param OpenTelemetryConfigBuilder $builder The builder containing configuration values.
     */
    public function __construct(OpenTelemetryConfigBuilder $builder)
    {
        $this->traces = $builder->getTraces();
        $this->metrics = $builder->getMetrics();
        $this->flushIntervalMs = $builder->getFlushIntervalMs();
    }

    /**
     * Creates a new OpenTelemetryConfig builder.
     *
     * @return OpenTelemetryConfigBuilder A new builder instance.
     */
    public static function builder(): OpenTelemetryConfigBuilder
    {
        return new OpenTelemetryConfigBuilder();
    }

    /**
     * Gets the traces configuration.
     *
     * @return TracesConfig|null The traces configuration, or null if not configured.
     */
    public function getTraces(): ?TracesConfig
    {
        return $this->traces;
    }

    /**
     * Gets the metrics configuration.
     *
     * @return MetricsConfig|null The metrics configuration, or null if not configured.
     */
    public function getMetrics(): ?MetricsConfig
    {
        return $this->metrics;
    }

    /**
     * Gets the flush interval in milliseconds.
     *
     * @return int|null The flush interval in milliseconds.
     */
    public function getFlushIntervalMs(): ?int
    {
        return $this->flushIntervalMs;
    }
}
