<?php

/**
 * Comprehensive OpenTelemetry Public API Example for Valkey GLIDE PHP
 * 
 * This example demonstrates the Java-inspired public OTEL APIs:
 * - Static initialization and configuration
 * - Runtime sample percentage control
 * - Manual span creation and management
 * - Integration with automatic command tracing
 */

require_once __DIR__ . '/../vendor/autoload.php';

echo "=== Valkey GLIDE PHP OpenTelemetry Public API Example ===\n\n";

try {
    // 1. Check initial state
    echo "1. Initial OpenTelemetry state:\n";
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . "\n";
    echo "   Sample percentage: " . (ValkeyGlide::getOpenTelemetrySamplePercentage() ?? "N/A") . "%\n\n";

    // 2. Initialize OpenTelemetry with comprehensive configuration
    echo "2. Initializing OpenTelemetry...\n";
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
    echo "   Result: " . ($initialized ? "Success" : "Already initialized") . "\n";

    // 3. Verify initialization
    echo "   Post-init state:\n";
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . "\n";
    echo "   Sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%\n\n";

    // 4. Test runtime sample percentage control
    echo "3. Testing runtime sample percentage control:\n";
    echo "   Setting sample percentage to 50%...\n";
    ValkeyGlide::setOpenTelemetrySamplePercentage(50);
    echo "   New sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%\n\n";

    // 5. Create manual spans for custom tracing
    echo "4. Creating manual spans for custom operations:\n";
    
    // Create a parent span for the entire user operation
    $userOperationSpan = ValkeyGlide::createOpenTelemetrySpan("user-checkout-process");
    echo "   Created parent span: " . ($userOperationSpan ? "ID $userOperationSpan" : "Failed") . "\n";

    // Create child spans for sub-operations
    $validationSpan = ValkeyGlide::createOpenTelemetrySpan("validate-user-data");
    echo "   Created validation span: " . ($validationSpan ? "ID $validationSpan" : "Failed") . "\n";

    // Simulate some work
    usleep(100000); // 100ms

    // End validation span
    ValkeyGlide::endOpenTelemetrySpan($validationSpan);
    echo "   Ended validation span\n";

    // 6. Create Valkey client and perform operations (automatic tracing)
    echo "\n5. Creating Valkey client and performing operations:\n";
    
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

        echo "   Client created successfully\n";

        // These operations will be automatically traced
        $client->set('otel:demo:key1', 'value1');
        echo "   SET operation completed\n";

        $value = $client->get('otel:demo:key1');
        echo "   GET operation completed, value: $value\n";

        // Create another manual span for batch operations
        $batchSpan = ValkeyGlide::createOpenTelemetrySpan("batch-operations");
        
        // Perform multiple operations
        $client->set('otel:demo:key2', 'value2');
        $client->set('otel:demo:key3', 'value3');
        $client->mget(['otel:demo:key1', 'otel:demo:key2', 'otel:demo:key3']);
        
        ValkeyGlide::endOpenTelemetrySpan($batchSpan);
        echo "   Batch operations completed\n";

        $client->close();
        echo "   Client closed\n";

    } catch (Exception $e) {
        echo "   Client operations failed: " . $e->getMessage() . "\n";
        echo "   (This is expected if Valkey server is not running)\n";
    }

    // 7. End the parent span
    ValkeyGlide::endOpenTelemetrySpan($userOperationSpan);
    echo "   Ended parent span\n\n";

    // 8. Test error conditions
    echo "6. Testing error conditions:\n";
    
    try {
        ValkeyGlide::setOpenTelemetrySamplePercentage(150);  // Invalid percentage
    } catch (Exception $e) {
        echo "   Expected error for invalid percentage: " . $e->getMessage() . "\n";
    }

    try {
        ValkeyGlide::createOpenTelemetrySpan("");  // Empty name
    } catch (Exception $e) {
        echo "   Expected error for empty span name: " . $e->getMessage() . "\n";
    }

    try {
        ValkeyGlide::createOpenTelemetrySpan(str_repeat("a", 300));  // Too long name
    } catch (Exception $e) {
        echo "   Expected error for long span name: " . $e->getMessage() . "\n";
    }

    // 9. Test duplicate initialization
    echo "\n7. Testing duplicate initialization:\n";
    $secondInit = ValkeyGlide::initOpenTelemetry($config);
    echo "   Second initialization result: " . ($secondInit ? "Success" : "Ignored (expected)") . "\n";

    // 10. Final state
    echo "\n8. Final OpenTelemetry state:\n";
    echo "   Initialized: " . (ValkeyGlide::isOpenTelemetryInitialized() ? "Yes" : "No") . "\n";
    echo "   Sample percentage: " . ValkeyGlide::getOpenTelemetrySamplePercentage() . "%\n";

    echo "\n=== Example completed successfully! ===\n";
    echo "Check the following files for telemetry data:\n";
    echo "- /tmp/valkey_glide_traces.json (traces)\n";
    echo "- /tmp/valkey_glide_metrics.json (metrics)\n";

} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    echo "Stack trace:\n" . $e->getTraceAsString() . "\n";
}
