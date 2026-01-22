<?php

/**
 * Example demonstrating OpenTelemetry integration with Valkey GLIDE PHP
 */

require_once __DIR__ . '/../vendor/autoload.php';

// Import OpenTelemetry classes
use ValkeyGlide\OpenTelemetry\{OpenTelemetryConfig, TracesConfig, MetricsConfig};

try {
    // Get system temp directory
    $tmpDir = sys_get_temp_dir();

    // Create OpenTelemetry configuration using builder pattern
    $otelConfig = OpenTelemetryConfig::builder()
        ->traces(
            TracesConfig::builder()
                ->endpoint("file://{$tmpDir}/valkey_glide_traces.json")
                ->samplePercentage(10)
                ->build()
        )
        ->metrics(
            MetricsConfig::builder()
                ->endpoint("file://{$tmpDir}/valkey_glide_metrics.json")
                ->build()
        )
        ->flushIntervalMs(5000)
        ->build();

    // Create ValkeyGlide client with configuration
    $client = new ValkeyGlide();
    $client->connect(
        addresses: [
            ['host' => 'localhost', 'port' => 6379]
        ],
        use_tls: false,
        credentials: null,
        read_from: ValkeyGlide::READ_FROM_PRIMARY,
        request_timeout: null,
        reconnect_strategy: null,
        database_id: 0,
        client_name: 'otel-class-example-client',
        client_az: null,
        advanced_config: [
            'connection_timeout' => 5000,
            'otel' => $otelConfig  // Class-based configuration
        ]
    );

    echo "ValkeyGlide client created with OpenTelemetry support" . PHP_EOL;
    echo "- Sample percentage: 10%" . PHP_EOL;
    echo "- Flush interval: 5000ms" . PHP_EOL;
    echo "- Traces endpoint: file://{$tmpDir}/valkey_glide_traces.json" . PHP_EOL;
    echo "- Metrics endpoint: file://{$tmpDir}/valkey_glide_metrics.json" . PHP_EOL . PHP_EOL;

    // Perform some operations that will be traced
    $client->set('otel:class:test:key1', 'value1');
    echo "SET operation completed" . PHP_EOL;

    $value = $client->get('otel:class:test:key1');
    echo "GET operation completed: $value" . PHP_EOL;

    $client->set('otel:class:test:key2', 'value2');
    $client->set('otel:class:test:key3', 'value3');

    // Batch operations will also be traced
    $results = $client->mget(['otel:class:test:key1', 'otel:class:test:key2', 'otel:class:test:key3']);
    echo "MGET operation completed: " . json_encode($results) . PHP_EOL;

    // Cleanup
    $client->del(['otel:class:test:key1', 'otel:class:test:key2', 'otel:class:test:key3']);
    echo "Cleanup completed" . PHP_EOL;

    $client->close();
    echo "Client closed" . PHP_EOL . PHP_EOL;
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . PHP_EOL;
    exit(1);
}

echo PHP_EOL . "OpenTelemetry example completed successfully!" . PHP_EOL;
echo "Class-based configuration demonstrated." . PHP_EOL;
echo "Check the following files for telemetry data:" . PHP_EOL;
echo "- {$tmpDir}/valkey_glide_traces.json (traces)" . PHP_EOL;
echo "- {$tmpDir}/valkey_glide_metrics.json (metrics)" . PHP_EOL;
