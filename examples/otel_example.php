<?php
/**
 * Example demonstrating OpenTelemetry integration with Valkey GLIDE PHP
 * Shows both array-based (legacy) and class-based (Java-style) configuration
 */

require_once __DIR__ . '/../vendor/autoload.php';

// Import OpenTelemetry classes
use ValkeyGlide\OpenTelemetry\{OpenTelemetryConfig, TracesConfig, MetricsConfig};

try {
    echo "=== Class-based OpenTelemetry Configuration (Java-style) ===" . PHP_EOL;
    
    // Create OpenTelemetry configuration using builder pattern (like Java)
    $otelConfig = OpenTelemetryConfig::builder()
        ->traces(
            TracesConfig::builder()
                ->endpoint('file:///tmp/valkey_glide_traces.json')
                ->samplePercentage(10)
                ->build()
        )
        ->metrics(
            MetricsConfig::builder()
                ->endpoint('file:///tmp/valkey_glide_metrics.json')
                ->build()
        )
        ->flushIntervalMs(5000)
        ->build();

    // Create ValkeyGlide client with class-based OTEL configuration
    $client = new ValkeyGlide(
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

    echo "ValkeyGlide client created with class-based OpenTelemetry support" . PHP_EOL;
    echo "- Sample percentage: 10%" . PHP_EOL;
    echo "- Flush interval: 5000ms" . PHP_EOL;
    echo "- Traces endpoint: file:///tmp/valkey_glide_traces.json" . PHP_EOL;
    echo "- Metrics endpoint: file:///tmp/valkey_glide_metrics.json" . PHP_EOL . PHP_EOL;

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

    echo "=== Array-based OpenTelemetry Configuration (Legacy) ===" . PHP_EOL;
    
    // Legacy array-based configuration (still supported)
    $legacyOtelConfig = [
        'traces' => [
            'endpoint' => 'file:///tmp/valkey_glide_traces_legacy.json',
            'sample_percentage' => 5
        ],
        'metrics' => [
            'endpoint' => 'file:///tmp/valkey_glide_metrics_legacy.json'
        ],
        'flush_interval_ms' => 3000
    ];

    $legacyClient = new ValkeyGlide(
        addresses: [
            ['host' => 'localhost', 'port' => 6379]
        ],
        use_tls: false,
        credentials: null,
        read_from: ValkeyGlide::READ_FROM_PRIMARY,
        request_timeout: null,
        reconnect_strategy: null,
        database_id: 0,
        client_name: 'otel-array-example-client',
        client_az: null,
        advanced_config: [
            'connection_timeout' => 5000,
            'otel' => $legacyOtelConfig  // Array-based configuration
        ]
    );

    echo "ValkeyGlide client created with array-based OpenTelemetry support" . PHP_EOL;
    echo "- Sample percentage: 5%" . PHP_EOL;
    echo "- Flush interval: 3000ms" . PHP_EOL;

    $legacyClient->set('otel:array:test', 'legacy_value');
    $legacyValue = $legacyClient->get('otel:array:test');
    echo "Legacy client operation completed: $legacyValue" . PHP_EOL;

    $legacyClient->del('otel:array:test');
    $legacyClient->close();
    echo "Legacy client closed" . PHP_EOL;

} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . PHP_EOL;
    exit(1);
}

echo PHP_EOL . "OpenTelemetry example completed successfully!" . PHP_EOL;
echo "Both class-based and array-based configurations work." . PHP_EOL;
echo "Check the following files for telemetry data:" . PHP_EOL;
echo "- /tmp/valkey_glide_traces.json (class-based traces)" . PHP_EOL;
echo "- /tmp/valkey_glide_metrics.json (class-based metrics)" . PHP_EOL;
echo "- /tmp/valkey_glide_traces_legacy.json (array-based traces)" . PHP_EOL;
echo "- /tmp/valkey_glide_metrics_legacy.json (array-based metrics)" . PHP_EOL;
?>
