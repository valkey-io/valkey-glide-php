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

    /**
     * Internal method to set traces configuration.
     */
    public function setTraces(?TracesConfig $traces): void
    {
        $this->traces = $traces;
    }

    /**
     * Internal method to set metrics configuration.
     */
    public function setMetrics(?MetricsConfig $metrics): void
    {
        $this->metrics = $metrics;
    }

    /**
     * Internal method to set flush interval.
     */
    public function setFlushIntervalMs(?int $flushIntervalMs): void
    {
        $this->flushIntervalMs = $flushIntervalMs;
    }
}
