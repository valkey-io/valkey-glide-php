<?php

/**
 * Basic Standalone Client Example
 *
 * This example demonstrates how to connect to a standalone Valkey server
 * and perform basic operations.
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

echo " Valkey GLIDE PHP - Standalone Client Example\n";
echo "===============================================\n\n";

// Configuration - can be customized via environment variables
$host = getenv('VALKEY_HOST') ?: 'localhost';
$port = (int)(getenv('VALKEY_PORT') ?: 6379);
$password = getenv('VALKEY_PASSWORD') ?: null;
$use_tls = filter_var(getenv('VALKEY_USE_TLS'), FILTER_VALIDATE_BOOLEAN);

// Server addresses configuration
$addresses = [
    ['host' => $host, 'port' => $port]
];

echo "📡 Connecting to Valkey server at {$host}:{$port}\n";

$client = null;
try {
    // Create Valkey GLIDE client
    $client = new ValkeyGlide();
    $client->connect(
        addresses: $addresses,
        use_tls: $use_tls,
        credentials: $password ? ['password' => $password] : null,
        read_from: 0,
        request_timeout: 5000
    );

    echo "Connected successfully!\n\n";

    // Test connection with PING
    echo " Testing connection...\n";
    $pong = $client->ping();
    echo "PING response: {$pong}\n\n";

    // Basic string operations
    echo "📝 String Operations:\n";
    echo "--------------------\n";

    // SET operation
    $key = 'example:greeting';
    $value = 'Hello, Valkey GLIDE!';
    $setResult = $client->set($key, $value);
    echo "SET {$key} = '{$value}' -> {$setResult}\n";

    // GET operation
    $getValue = $client->get($key);
    echo "GET {$key} -> '{$getValue}'\n";

    // SET with expiration (EX = seconds)
    $tempKey = 'example:temp';
    $client->set($tempKey, 'This will expire', ['EX' => 10]);
    echo "SET {$tempKey} with 10s expiration\n";

    // Check TTL
    $ttl = $client->ttl($tempKey);
    echo "TTL {$tempKey} -> {$ttl} seconds\n\n";

    // Numeric operations
    echo "🔢 Numeric Operations:\n";
    echo "---------------------\n";

    $counterKey = 'example:counter';

    // Initialize counter
    $client->set($counterKey, '0');
    echo "Initialized counter to 0\n";

    // Increment operations
    for ($i = 1; $i <= 5; $i++) {
        $newValue = $client->incr($counterKey);
        echo "INCR {$counterKey} -> {$newValue}\n";
    }

    // Increment by specific amount
    $newValue = $client->incrby($counterKey, 10);
    echo "INCRBY {$counterKey} 10 -> {$newValue}\n";

    // Decrement
    $newValue = $client->decr($counterKey);
    echo "DECR {$counterKey} -> {$newValue}\n\n";

    // Key operations
    echo "🔑 Key Operations:\n";
    echo "-----------------\n";

    // Check if key exists
    $exists = $client->exists([$key]);
    echo "EXISTS {$key} -> " . ($exists ? 'true' : 'false') . "\n";

    // Get key type
    $type = $client->type($key);
    echo "TYPE {$key} -> {$type}\n";

    // Set expiration
    $client->expire($key, 3600); // 1 hour
    $ttl = $client->ttl($key);
    echo "Set expiration on {$key}, TTL -> {$ttl} seconds\n";

    // Remove expiration
    $client->persist($key);
    $ttl = $client->ttl($key);
    echo "Removed expiration from {$key}, TTL -> " . ($ttl == -1 ? 'no expiration' : "{$ttl} seconds") . "\n\n";

    // Multiple key operations
    echo "🎯 Multiple Key Operations:\n";
    echo "---------------------------\n";

    // MSET - set multiple keys
    $keyValues = [
        'example:key1' => 'value1',
        'example:key2' => 'value2',
        'example:key3' => 'value3'
    ];

    $client->mset($keyValues);
    echo "MSET: Set " . count($keyValues) . " keys\n";

    // MGET - get multiple keys
    $keys = array_keys($keyValues);
    $values = $client->mget($keys);
    echo "MGET results:\n";
    foreach ($keys as $index => $key) {
        $value = $values[$index] ?? 'null';
        echo "  {$key} -> '{$value}'\n";
    }
    echo "\n";

    // Server information
    echo "  Server Information:\n";
    echo "----------------------\n";

    // Get server info
    $info = $client->info();
    echo "  redis_version -> '{$info["redis_version"]}'\n";
    echo "  valkey_version -> '{$info["valkey_version"]}'\n";
    echo "  connected_clients -> '{$info["connected_clients"]}'\n";
    echo "  used_memory_human -> '{$info["used_memory_human"]}'\n";

    // Database size
    $dbsize = $client->dbsize();
    echo "  Database size: {$dbsize} keys\n\n";

    // Cleanup
    echo "🧹 Cleanup:\n";
    echo "----------\n";

    $keysToDelete = array_merge($keys, [$counterKey, $tempKey]);
    $deletedCount = $client->del($keysToDelete);
    echo "Deleted {$deletedCount} keys\n";

    echo "\nExample completed successfully!\n";
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    echo "Error details: " . $e->getTraceAsString() . "\n";
    exit(1);
} finally {
    // Always close the connection
    if ($client) {
        $client->close();
        echo " Connection closed.\n";
    }
}

echo "\n Next Steps:\n";
echo "- Try the cluster client example: php basic/cluster_client.php\n";
echo "- Explore configuration options: php basic/configuration.php\n";
echo "- Check out data structure examples in the data_structures/ directory\n";
