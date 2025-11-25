<?php

namespace ValkeyGlide\OpenTelemetry;

use ValkeyGlideException;

/**
 * Builder for MetricsConfig.
 */
class MetricsConfigBuilder
{
    private ?string $endpoint = null;

    /**
     * Sets the endpoint.
     *
     * @param string $endpoint The metrics endpoint URL.
     * @return self This builder instance for method chaining.
     */
    public function endpoint(string $endpoint): self
    {
        if (empty($endpoint)) {
            throw new ValkeyGlideException("Metrics endpoint cannot be empty");
        }
        $this->endpoint = $endpoint;
        return $this;
    }

    /**
     * Gets the endpoint.
     *
     * @return string The metrics endpoint URL.
     */
    public function getEndpoint(): string
    {
        if ($this->endpoint === null) {
            throw new ValkeyGlideException("Metrics endpoint is required when metrics config is provided");
        }
        return $this->endpoint;
    }

    /**
     * Builds the MetricsConfig.
     *
     * @return MetricsConfig The immutable metrics configuration.
     */
    public function build(): MetricsConfig
    {
        if ($this->endpoint === null) {
            throw new ValkeyGlideException("Metrics endpoint is required when metrics config is provided");
        }

        return new MetricsConfig($this);
    }
}
