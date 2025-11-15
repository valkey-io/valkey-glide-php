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
     */
    public function traces(?TracesConfig $traces): self
    {
        $this->traces = $traces;
        return $this;
    }

    /**
     * Sets the metrics configuration.
     */
    public function metrics(?MetricsConfig $metrics): self
    {
        $this->metrics = $metrics;
        return $this;
    }

    /**
     * Sets the flush interval in milliseconds.
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
     * Builds the OpenTelemetryConfig.
     */
    public function build(): OpenTelemetryConfig
    {
        if ($this->traces === null && $this->metrics === null) {
            throw new ValkeyGlideException("At least one of traces or metrics must be configured");
        }

        $config = new OpenTelemetryConfig();
        $config->setTraces($this->traces);
        $config->setMetrics($this->metrics);
        $config->setFlushIntervalMs($this->flushIntervalMs);
        
        return $config;
    }
}
