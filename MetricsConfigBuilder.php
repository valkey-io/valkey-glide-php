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
     * Builds the MetricsConfig.
     */
    public function build(): MetricsConfig
    {
        if ($this->endpoint === null) {
            throw new ValkeyGlideException("Metrics endpoint is required when metrics config is provided");
        }

        $config = new MetricsConfig();
        $config->setEndpoint($this->endpoint);
        
        return $config;
    }
}
