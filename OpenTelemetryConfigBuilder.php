<?php

namespace ValkeyGlide\OpenTelemetry;

use ValkeyGlideException;

/**
 * Builder for OpenTelemetryConfig.
 */
class OpenTelemetryConfigBuilder
{
    private ?TracesConfig $traces = null;
    private ?MetricsConfig $metrics = null;
    private int $flushIntervalMs = 5000; // Default value

    /**
     * Sets the traces configuration.
     *
     * @param TracesConfig|null $traces The traces configuration, or null to disable traces.
     * @return self This builder instance for method chaining.
     */
    public function traces(?TracesConfig $traces): self
    {
        $this->traces = $traces;
        return $this;
    }

    /**
     * Sets the metrics configuration.
     *
     * @param MetricsConfig|null $metrics The metrics configuration, or null to disable metrics.
     * @return self This builder instance for method chaining.
     */
    public function metrics(?MetricsConfig $metrics): self
    {
        $this->metrics = $metrics;
        return $this;
    }

    /**
     * Sets the flush interval in milliseconds.
     *
     * @param int $flushIntervalMs The flush interval in milliseconds (must be positive).
     * @return self This builder instance for method chaining.
     */
    public function flushIntervalMs(int $flushIntervalMs): self
    {
        if ($flushIntervalMs <= 0) {
            throw new ValkeyGlideException("Flush interval must be a positive integer");
        }
        $this->flushIntervalMs = $flushIntervalMs;
        return $this;
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
     * @return int The flush interval in milliseconds.
     */
    public function getFlushIntervalMs(): int
    {
        return $this->flushIntervalMs;
    }

    /**
     * Builds the OpenTelemetryConfig.
     *
     * @return OpenTelemetryConfig The immutable OpenTelemetry configuration.
     */
    public function build(): OpenTelemetryConfig
    {
        if ($this->traces === null && $this->metrics === null) {
            throw new ValkeyGlideException("At least one of traces or metrics must be configured");
        }

        return new OpenTelemetryConfig($this);
    }
}
