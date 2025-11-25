<?php

namespace ValkeyGlide\OpenTelemetry;

/**
 * Configuration for OpenTelemetry metrics.
 */
class MetricsConfig
{
    private string $endpoint;

    /**
     * Creates a new MetricsConfig from a builder.
     */
    public function __construct(MetricsConfigBuilder $builder)
    {
        $this->endpoint = $builder->getEndpoint();
    }

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
}
