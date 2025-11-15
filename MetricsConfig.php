<?php

namespace ValkeyGlide\OpenTelemetry;

/**
 * Configuration for OpenTelemetry metrics.
 */
class MetricsConfig
{
    private string $endpoint;

    /**
     * Creates a new MetricsConfig builder.
     */
    public static function builder(): MetricsConfigBuilder
    {
        return new MetricsConfigBuilder();
    }

    /**
     * Gets the endpoint.
     */
    public function getEndpoint(): string
    {
        return $this->endpoint;
    }

    /**
     * Internal method to set endpoint.
     */
    public function setEndpoint(string $endpoint): void
    {
        $this->endpoint = $endpoint;
    }
}
