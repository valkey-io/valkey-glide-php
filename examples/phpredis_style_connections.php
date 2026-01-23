<?php

/**
 * PHPRedis-Style Connection Examples
 *
 * This example demonstrates PHPRedis-compatible connection patterns
 * that work with ValkeyGlide.
 */

error_reporting(E_ALL);
ini_set('display_errors', 1);

if (!extension_loaded('valkey_glide')) {
    echo "Valkey GLIDE extension is not loaded!\n";
    exit(1);
}

echo "PHPRedis-Style Connection Examples\n";
echo "=====================================\n\n";

// =============================================================================
// Example 1: Simple host/port connection (PHPRedis style)
// =============================================================================
echo "1. Simple host/port connection:\n";
try {
    $client = new ValkeyGlide();
    $result = $client->connect('localhost', 6379);
    
    if ($result) {
        echo "   Connected to localhost:6379\n";
        $client->set('phpredis_test', 'value1');
        echo "   SET operation successful\n";
        $client->close();
    }
} catch (Exception $e) {
    echo " Failed: " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// Example 2: Connection with timeout (PHPRedis style)
// =============================================================================
echo "2. Connection with timeout:\n";
try {
    $client = new ValkeyGlide();
    $result = $client->connect('localhost', 6379, 2.5); // 2.5 second timeout
    
    if ($result) {
        echo "   Connected with 2.5s timeout\n";
        $client->ping();
        echo "   PING successful\n";
        $client->close();
    }
} catch (Exception $e) {
    echo "   Failed: " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// Example 3: ValkeyGlide-style with addresses array
// =============================================================================
echo "3. ValkeyGlide-style with addresses array:\n";
try {
    $client = new ValkeyGlide();
    $result = $client->connect(addresses: [['host' => 'localhost', 'port' => 6379]]);
    
    if ($result) {
        echo "   Connected using addresses array\n";
        $value = $client->get('phpredis_test');
        echo "   GET returned: $value\n";
        $client->close();
    }
} catch (Exception $e) {
    echo "   Failed: " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// Example 4: Using Redis alias (PHPRedis compatibility)
// =============================================================================
echo "4. Using Redis alias:\n";
if (file_exists(__DIR__ . '/../phpredis_aliases.php')) {
    require_once __DIR__ . '/../phpredis_aliases.php';
    
    try {
        $redis = new Redis();
        $result = $redis->connect('localhost', 6379);
        
        if ($result) {
            echo "   Connected using Redis alias\n";
            $redis->set('redis_alias_test', 'works');
            echo "   Operations work with Redis alias\n";
            $redis->close();
        }
    } catch (RedisException $e) {
        echo "   Failed: " . $e->getMessage() . "\n";
    }
} else {
    echo "phpredis_aliases.php not found\n";
}
echo "\n";

// =============================================================================
// Example 5: Error handling - connection failure
// =============================================================================
echo "5. Error handling - connection failure:\n";
try {
    $client = new ValkeyGlide();
    $result = $client->connect('nonexistent-host', 9999, 1.0);
    
    if (!$result) {
        echo "   connect() returned false for invalid host\n";
    }
} catch (Exception $e) {
    echo " Exception caught: " . substr($e->getMessage(), 0, 50) . "...\n";
}
echo "\n";

echo "All examples completed!\n";
