<?php

namespace ValkeyGlide\OpenTelemetry;

/**
 * Configuration for OpenTelemetry traces.
 */
class TracesConfig
{
    private string $endpoint;
    private int $samplePercentage;

    /**
     * Creates a new TracesConfig from a builder.
     *
     * @param TracesConfigBuilder $builder The builder containing configuration values.
     */
    public function __construct(TracesConfigBuilder $builder)
    {
        $this->endpoint = $builder->getEndpoint();
        $this->samplePercentage = $builder->getSamplePercentage();
    }

    /**
     * Creates a new TracesConfig builder.
     *
     * @return TracesConfigBuilder A new builder instance.
     */
    public static function builder(): TracesConfigBuilder
    {
        return new TracesConfigBuilder();
    }

    /**
     * Gets the endpoint.
     *
     * @return string The traces endpoint URL.
     */
    public function getEndpoint(): string
    {
        return $this->endpoint;
    }

    /**
     * Gets the sample percentage.
     *
     * @return int The sample percentage (0-100).
     */
    public function getSamplePercentage(): int
    {
        return $this->samplePercentage;
    }
}
