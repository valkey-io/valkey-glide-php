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

    /**
     * Internal method to set endpoint.
     */
    public function setEndpoint(string $endpoint): void
    {
        $this->endpoint = $endpoint;
    }

    /**
     * Internal method to set sample percentage.
     */
    public function setSamplePercentage(int $samplePercentage): void
    {
        $this->samplePercentage = $samplePercentage;
    }
}
