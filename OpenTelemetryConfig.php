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
     */
    public function __construct(OpenTelemetryConfigBuilder $builder)
    {
        $this->traces = $builder->getTraces();
        $this->metrics = $builder->getMetrics();
        $this->flushIntervalMs = $builder->getFlushIntervalMs();
    }

    /**
     * Creates a new OpenTelemetryConfig builder.
     */
    public static function builder(): OpenTelemetryConfigBuilder
    {
        return new OpenTelemetryConfigBuilder();
    }

    /**
     * Gets the traces configuration.
     */
    public function getTraces(): ?TracesConfig
    {
        return $this->traces;
    }

    /**
     * Gets the metrics configuration.
     */
    public function getMetrics(): ?MetricsConfig
    {
        return $this->metrics;
    }

    /**
     * Gets the flush interval in milliseconds.
     */
    public function getFlushIntervalMs(): ?int
    {
        return $this->flushIntervalMs;
    }
}
