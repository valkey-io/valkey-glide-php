<?php

/**
 * Comprehensive OpenTelemetry Public API Example for Valkey GLIDE PHP
 * 
 * This example demonstrates the public OTEL APIs:
 * - Static initialization and configuration
 * - Runtime sample percentage control
 * - Manual span creation and management
 * - Integration with automatic command tracing
 */

require_once __DIR__ . '/../vendor/autoload.php';

echo "=== Valkey GLIDE PHP OpenTelemetry Public API Example ===" . PHP_EOL . PHP_EOL;

try {
    // 1. Check initial state
    echo "1. Initial OpenTelemetry state:" . PHP_EOL;
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . PHP_EOL;
    echo "   Sample percentage: " . (ValkeyGlide::getOpenTelemetrySamplePercentage() ?? "N/A") . "%" . PHP_EOL . PHP_EOL;

    // 2. Initialize OpenTelemetry with comprehensive configuration
    echo "2. Initializing OpenTelemetry..." . PHP_EOL;
    $config = [
        'traces' => [
            'endpoint' => 'file:///tmp/valkey_glide_traces.json',
            'sample_percentage' => 100  // Sample all requests for demo
        ],
        'metrics' => [
            'endpoint' => 'file:///tmp/valkey_glide_metrics.json'
        ],
        'flush_interval_ms' => 3000  // Flush every 3 seconds
    ];

    $initialized = ValkeyGlide::initOpenTelemetry($config);
    echo "   Result: " . ($initialized ? "Success" : "Already initialized") . PHP_EOL;

    // Verify initialization
    echo "   Post-init state:" . PHP_EOL;
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . PHP_EOL;
    echo "   Sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%" . PHP_EOL . PHP_EOL;

    // 3. Test runtime sample percentage control
    echo "3. Testing runtime sample percentage control:" . PHP_EOL;
    echo "   Setting sample percentage to 50%..." . PHP_EOL;
    ValkeyGlide::setOpenTelemetrySamplePercentage(50);
    echo "   New sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%" . PHP_EOL . PHP_EOL;

    // 4. Create manual spans for custom tracing
    echo "4. Creating manual spans for custom operations:" . PHP_EOL;
    
    // Create a parent span for the entire user operation
    $userOperationSpan = ValkeyGlide::createOpenTelemetrySpan("user-checkout-process");
    echo "   Created parent span: " . ($userOperationSpan ? "ID $userOperationSpan" : "Failed") . PHP_EOL;

    // Create child spans for sub-operations
    $validationSpan = ValkeyGlide::createOpenTelemetrySpan("validate-user-data");
    echo "   Created validation span: " . ($validationSpan ? "ID $validationSpan" : "Failed") . PHP_EOL;

    // Simulate some work
    usleep(100000); // 100ms

    // End validation span
    ValkeyGlide::endOpenTelemetrySpan($validationSpan);
    echo "   Ended validation span" . PHP_EOL;

    // 5. Create Valkey client and perform operations (automatic tracing)
    echo PHP_EOL . "5. Creating Valkey client and performing operations:" . PHP_EOL;
    
    $addresses = [
        ['host' => 'localhost', 'port' => 6379]
    ];
    
    try {
        $client = new ValkeyGlide(
            $addresses,
            false,  // use_tls
            null,   // credentials
            null,   // read_from
            null,   // request_timeout
            null,   // reconnect_strategy
            null,   // client_name
            null,   // periodic_checks
            null,   // advanced_config
            null,   // lazy_connect
            0       // database_id
        );

        echo "   Client created successfully" . PHP_EOL;

        // These operations will be automatically traced
        $client->set('otel:demo:key1', 'value1');
        echo "   SET operation completed" . PHP_EOL;

        $value = $client->get('otel:demo:key1');
        echo "   GET operation completed, value: $value" . PHP_EOL;

        // Create another manual span for batch operations
        $batchSpan = ValkeyGlide::createOpenTelemetrySpan("batch-operations");
        
        // Perform multiple operations
        $client->set('otel:demo:key2', 'value2');
        $client->set('otel:demo:key3', 'value3');
        $client->mget(['otel:demo:key1', 'otel:demo:key2', 'otel:demo:key3']);
        
        ValkeyGlide::endOpenTelemetrySpan($batchSpan);
        echo "   Batch operations completed" . PHP_EOL;

        $client->close();
        echo "   Client closed" . PHP_EOL;

    } catch (Exception $e) {
        echo "   Client operations failed: " . $e->getMessage() . PHP_EOL;
        echo "   (This is expected if Valkey server is not running)" . PHP_EOL;
    }

    // End the parent span
    ValkeyGlide::endOpenTelemetrySpan($userOperationSpan);
    echo "   Ended parent span" . PHP_EOL . PHP_EOL;

    // 6. Test error conditions
    echo "6. Testing error conditions:" . PHP_EOL;
    
    try {
        ValkeyGlide::setOpenTelemetrySamplePercentage(150);  // Invalid percentage
    } catch (Exception $e) {
        echo "   Expected error for invalid percentage: " . $e->getMessage() . PHP_EOL;
    }

    try {
        ValkeyGlide::createOpenTelemetrySpan("");  // Empty name
    } catch (Exception $e) {
        echo "   Expected error for empty span name: " . $e->getMessage() . PHP_EOL;
    }

    try {
        ValkeyGlide::createOpenTelemetrySpan(str_repeat("a", 300));  // Too long name
    } catch (Exception $e) {
        echo "   Expected error for long span name: " . $e->getMessage() . PHP_EOL;
    }

    // 7. Test duplicate initialization
    echo PHP_EOL . "7. Testing duplicate initialization:" . PHP_EOL;
    $secondInit = ValkeyGlide::initOpenTelemetry($config);
    echo "   Second initialization result: " . ($secondInit ? "Success" : "Ignored (expected)") . PHP_EOL;

    // 8. Final state
    echo PHP_EOL . "8. Final OpenTelemetry state:" . PHP_EOL;
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . PHP_EOL;
    echo "   Sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%" . PHP_EOL;

    echo PHP_EOL . "=== Example completed successfully! ===" . PHP_EOL;
    echo "Check the following files for telemetry data:" . PHP_EOL;
    echo "- /tmp/valkey_glide_traces.json (traces)" . PHP_EOL;
    echo "- /tmp/valkey_glide_metrics.json (metrics)" . PHP_EOL;

} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . PHP_EOL;
    echo "Stack trace:" . PHP_EOL . $e->getTraceAsString() . PHP_EOL;
}
