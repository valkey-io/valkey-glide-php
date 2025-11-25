<?php

namespace ValkeyGlide\OpenTelemetry;

use ValkeyGlideException;

/**
 * Builder for TracesConfig.
 */
class TracesConfigBuilder
{
    private ?string $endpoint = null;
    private int $samplePercentage = 1; // Default value

    /**
     * Sets the endpoint.
     *
     * @param string $endpoint The traces endpoint URL.
     * @return self This builder instance for method chaining.
     */
    public function endpoint(string $endpoint): self
    {
        if (empty($endpoint)) {
            throw new ValkeyGlideException("Traces endpoint cannot be empty");
        }
        $this->endpoint = $endpoint;
        return $this;
    }

    /**
     * Sets the sample percentage.
     *
     * @param int $samplePercentage The sample percentage (0-100).
     * @return self This builder instance for method chaining.
     */
    public function samplePercentage(int $samplePercentage): self
    {
        if ($samplePercentage < 0 || $samplePercentage > 100) {
            throw new ValkeyGlideException("Sample percentage must be between 0 and 100");
        }
        $this->samplePercentage = $samplePercentage;
        return $this;
    }

    /**
     * Gets the endpoint.
     *
     * @return string The traces endpoint URL.
     */
    public function getEndpoint(): string
    {
        if ($this->endpoint === null) {
            throw new ValkeyGlideException("Traces endpoint is required when traces config is provided");
        }
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

    /**
     * Builds the TracesConfig.
     *
     * @return TracesConfig The immutable traces configuration.
     */
    public function build(): TracesConfig
    {
        if ($this->endpoint === null) {
            throw new ValkeyGlideException("Traces endpoint is required when traces config is provided");
        }

        return new TracesConfig($this);
    }
}
