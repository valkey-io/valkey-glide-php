<?php
/**
 * Example demonstrating OpenTelemetry integration with Valkey GLIDE PHP
 */

require_once __DIR__ . '/../vendor/autoload.php';

try {
    // OTEL configuration for traces and metrics
    $otelConfig = [
        'traces' => [
            'endpoint' => 'grpc://localhost:4317',  // OTEL collector endpoint
            'sample_percentage' => 10               // Sample 10% of requests (default is 1%)
        ],
        'metrics' => [
            'endpoint' => 'grpc://localhost:4317'   // OTEL collector endpoint
        ],
        'flush_interval_ms' => 5000                 // Flush every 5 seconds (default)
    ];

    // Create ValkeyGlide client with OTEL configuration
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
        client_name: 'otel-example-client',
        client_az: null,
        advanced_config: [
            'connection_timeout' => 5000,
            'otel' => $otelConfig  // Add OTEL configuration
        ]
    );

    echo "ValkeyGlide client created with OpenTelemetry support" . PHP_EOL;
    echo "- Sample percentage: 10% (higher than default 1% for demo)" . PHP_EOL;
    echo "- Flush interval: 5000ms (default)" . PHP_EOL;
    echo "- Traces endpoint: grpc://localhost:4317" . PHP_EOL;
    echo "- Metrics endpoint: grpc://localhost:4317" . PHP_EOL . PHP_EOL;

    // Perform some operations that will be traced
    $client->set('otel:test:key1', 'value1');
    echo "SET operation completed" . PHP_EOL;

    $value = $client->get('otel:test:key1');
    echo "GET operation completed: $value" . PHP_EOL;

    $client->set('otel:test:key2', 'value2');
    $client->set('otel:test:key3', 'value3');

    // Batch operations will also be traced
    $results = $client->mget(['otel:test:key1', 'otel:test:key2', 'otel:test:key3']);
    echo "MGET operation completed: " . json_encode($results) . PHP_EOL;

    // Cleanup
    $client->del(['otel:test:key1', 'otel:test:key2', 'otel:test:key3']);
    echo "Cleanup completed" . PHP_EOL;

    $client->close();
    echo "Client closed" . PHP_EOL;

} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . PHP_EOL;
    exit(1);
}

echo PHP_EOL . "OpenTelemetry example completed successfully!" . PHP_EOL;
echo "Check your OTEL collector for traces and metrics." . PHP_EOL;
echo PHP_EOL . "Note: OTEL can only be initialized once per process." . PHP_EOL;
echo "If you need to change configuration, restart the process." . PHP_EOL;
?>
