<?php

/**
 * Basic Cluster Client Example
 *
 * This example demonstrates how to connect to a Valkey cluster
 * and perform operations across multiple nodes.
 */

// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Check if extension is loaded
if (!extension_loaded('valkey_glide')) {
    echo "Valkey GLIDE extension is not loaded!\n";
    echo "Please install and enable the valkey_glide extension.\n";
    exit(1);
}

echo " Valkey GLIDE PHP - Cluster Client Example\n";
echo "===========================================\n\n";

// Configuration - can be customized via environment variables
$host = getenv('VALKEY_HOST') ?: 'localhost';
$base_port = (int)(getenv('VALKEY_CLUSTER_PORT') ?: 7001);
$password = getenv('VALKEY_PASSWORD') ?: null;
$use_tls = filter_var(getenv('VALKEY_USE_TLS'), FILTER_VALIDATE_BOOLEAN);

// Cluster addresses configuration
// Typically a cluster has multiple nodes - we'll specify 3 common ports
$addresses = [
    ['host' => $host, 'port' => $base_port],
    ['host' => $host, 'port' => $base_port + 1],
    ['host' => $host, 'port' => $base_port + 2]
];

echo "📡 Connecting to Valkey cluster:\n";
foreach ($addresses as $addr) {
    echo "  - {$addr['host']}:{$addr['port']}\n";
}
echo "\n";

$client = null;
try {
    // Create Valkey GLIDE cluster client
    $client = new ValkeyGlideCluster(
        addresses: $addresses,
        use_tls: $use_tls,
        credentials: $password ? ['password' => $password] : null,
        read_from: 0,
        request_timeout: 5000
    );

    echo "Connected to cluster successfully!\n\n";

    // Test connection with PING
    echo " Testing cluster connection...\n";
    $pong = $client->ping();
    echo "PING response: {$pong}\n\n";

    // Cluster-specific operations
    echo " Cluster Information:\n";
    echo "----------------------\n";


    // Hash slot distribution example
    echo "🎯 Hash Slot Distribution:\n";
    echo "-------------------------\n";

    // Demonstrate how keys are distributed across the cluster
    $testKeys = [
        'user:1001',
        'user:1002',
        'session:abc123',
        'session:def456',
        'cache:homepage',
        'cache:products'
    ];

    echo "Setting keys across cluster nodes...\n";
    foreach ($testKeys as $key) {
        $value = "value_for_{$key}_" . time();
        $client->set($key, $value);
        echo "  SET {$key} = {$value}\n";
    }
    echo "\n";

    // Retrieve the keys
    echo "Getting keys from cluster...\n";
    foreach ($testKeys as $key) {
        $value = $client->get($key);
        echo "  GET {$key} = {$value}\n";
    }
    echo "\n";

    // Multi-key operations in cluster
    echo "🎯 Multi-key Operations:\n";
    echo "------------------------\n";

    // MGET - Note: In cluster mode, keys might be on different nodes
    try {
        $values = $client->mget($testKeys);
        echo "MGET results:\n";
        foreach ($testKeys as $index => $key) {
            $value = $values[$index] ?? 'null';
            echo "  {$key} -> {$value}\n";
        }
    } catch (Exception $e) {
        echo "Note: MGET might not work if keys are on different nodes: " . $e->getMessage() . "\n";
    }
    echo "\n";

    // Hash tags for ensuring keys are on the same slot
    echo "🏷️  Hash Tags (Same Slot Keys):\n";
    echo "------------------------------\n";

    // Using hash tags {...} to ensure keys go to the same slot
    $sameSlotKeys = [
        'user:{1001}:profile',
        'user:{1001}:settings',
        'user:{1001}:sessions'
    ];

    // Set keys with hash tags
    foreach ($sameSlotKeys as $key) {
        $value = "data_for_{$key}";
        $client->set($key, $value);
        echo "SET {$key} = {$value}\n";
    }

    // Now MGET should work since all keys are on the same slot
    try {
        $values = $client->mget($sameSlotKeys);
        echo "MGET with hash tags (same slot):\n";
        foreach ($sameSlotKeys as $index => $key) {
            $value = $values[$index] ?? 'null';
            echo "  {$key} -> {$value}\n";
        }
    } catch (Exception $e) {
        echo "Error with hash tags: " . $e->getMessage() . "\n";
    }
    echo "\n";

    // Counter operations across cluster
    echo "🔢 Distributed Counter Example:\n";
    echo "-------------------------------\n";

    // Create counters on different nodes
    $counters = [
        'counter:node1',
        'counter:node2',
        'counter:node3'
    ];

    foreach ($counters as $counter) {
        $client->set($counter, '0');
        echo "Initialized {$counter} to 0\n";
    }

    // Increment counters
    foreach ($counters as $counter) {
        for ($i = 1; $i <= 3; $i++) {
            $newValue = $client->incr($counter);
            echo "INCR {$counter} -> {$newValue}\n";
        }
    }
    echo "\n";

    // Cross-slot operations (might fail in strict cluster mode)
    echo "  Cross-slot Operations:\n";
    echo "-------------------------\n";

    try {
        // This might fail if keys are on different slots
        $client->mset([
            'cross:key1' => 'value1',
            'cross:key2' => 'value2'
        ]);
        echo "MSET across different slots succeeded\n";
    } catch (Exception $e) {
        echo "MSET failed (expected in cluster): " . $e->getMessage() . "\n";

        // Alternative: Set keys individually
        echo "Setting keys individually instead...\n";
        $client->set('cross:key1', 'value1');
        $client->set('cross:key2', 'value2');
        echo "Individual SET operations successful\n";
    }
    echo "\n";

    // Cluster-aware patterns
    echo " Recommended Cluster Patterns:\n";
    echo "-------------------------------\n";

    // Pattern 1: User-specific operations with hash tags
    $userId = '12345';
    $userKeys = [
        "user:{{$userId}}:profile" => json_encode(['name' => 'John Doe', 'email' => 'john@example.com']),
        "user:{{$userId}}:settings" => json_encode(['theme' => 'dark', 'lang' => 'en']),
        "user:{{$userId}}:last_login" => date('Y-m-d H:i:s')
    ];

    foreach ($userKeys as $key => $value) {
        $client->set($key, $value);
        echo "SET {$key}\n";
    }

    // All user data is on the same slot, so we can use transactions
    echo "All user data is on the same slot - can use multi-key operations\n\n";

    // Cleanup
    echo "🧹 Cleanup:\n";
    echo "----------\n";

    $allKeys = array_merge($testKeys, $sameSlotKeys, $counters, ['cross:key1', 'cross:key2'], array_keys($userKeys));

    // Delete keys individually (safer in cluster mode)
    $deletedCount = 0;
    foreach ($allKeys as $key) {
        try {
            $result = $client->del([$key]);
            $deletedCount += $result;
        } catch (Exception $e) {
            echo "Could not delete {$key}: " . $e->getMessage() . "\n";
        }
    }
    echo "Deleted {$deletedCount} keys\n";

    echo "\nCluster example completed successfully!\n";
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    echo "Error details: " . $e->getTraceAsString() . "\n";

    // Common cluster connection issues
    echo "\n💡 Troubleshooting Tips:\n";
    echo "- Ensure cluster nodes are running and accessible\n";
    echo "- Check if cluster is properly configured (cluster meet, slots assigned)\n";
    echo "- Verify network connectivity to all cluster nodes\n";
    echo "- For local testing, use: valkey-cli --cluster create 127.0.0.1:7001 127.0.0.1:7002 127.0.0.1:7003\n";

    exit(1);
} finally {
    // Always close the connection
    if ($client) {
        $client->close();
        echo " Cluster connection closed.\n";
    }
}

echo "\n Next Steps:\n";
echo "- Learn about configuration options: php basic/configuration.php\n";
echo "- Try advanced cluster features in the advanced/ directory\n";
echo "- Explore data structure examples optimized for cluster mode\n";

echo "\n💡 Cluster Best Practices:\n";
echo "- Use hash tags {...} to keep related keys on the same slot\n";
echo "- Avoid cross-slot operations when possible\n";
echo "- Consider data locality when designing your key structure\n";
echo "- Monitor cluster health and node distribution\n";
