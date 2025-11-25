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
     *
     * @param MetricsConfigBuilder $builder The builder containing configuration values.
     */
    public function __construct(MetricsConfigBuilder $builder)
    {
        $this->endpoint = $builder->getEndpoint();
    }

    /**
     * Creates a new MetricsConfig builder.
     *
     * @return MetricsConfigBuilder A new builder instance.
     */
    public static function builder(): MetricsConfigBuilder
    {
        return new MetricsConfigBuilder();
    }

    /**
     * Gets the endpoint.
     *
     * @return string The metrics endpoint URL.
     */
    public function getEndpoint(): string
    {
        return $this->endpoint;
    }
}
