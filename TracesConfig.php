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
     */
    public function __construct(TracesConfigBuilder $builder)
    {
        $this->endpoint = $builder->getEndpoint();
        $this->samplePercentage = $builder->getSamplePercentage();
    }

    /**
     * Creates a new TracesConfig builder.
     */
    public static function builder(): TracesConfigBuilder
    {
        return new TracesConfigBuilder();
    }

    /**
     * Gets the endpoint.
     */
    public function getEndpoint(): string
    {
        return $this->endpoint;
    }

    /**
     * Gets the sample percentage.
     */
    public function getSamplePercentage(): int
    {
        return $this->samplePercentage;
    }
}
