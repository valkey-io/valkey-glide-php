<?php

/**
 * Caching Patterns Example
 *
 * This example demonstrates various caching patterns using Valkey GLIDE,
 * including simple cache, cache-aside, write-through, and cache invalidation.
 */

// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Check if extension is loaded
if (!extension_loaded('valkey_glide')) {
    echo "Valkey GLIDE extension is not loaded!\n";
    exit(1);
}

echo " Valkey GLIDE PHP - Caching Patterns Example\n";
echo "=============================================\n\n";

// Configuration
$addresses = [['host' => 'localhost', 'port' => 6379]];

/**
 * Simulate a database class for demonstration
 */
class MockDatabase
{
    private $data = [
        1 => ['id' => 1, 'name' => 'John Doe', 'email' => 'john@example.com', 'created_at' => '2024-01-01'],
        2 => ['id' => 2, 'name' => 'Jane Smith', 'email' => 'jane@example.com', 'created_at' => '2024-01-02'],
        3 => ['id' => 3, 'name' => 'Bob Johnson', 'email' => 'bob@example.com', 'created_at' => '2024-01-03'],
    ];

    public function getUser($id)
    {
        // Simulate slow database query
        usleep(100000); // 100ms delay
        echo "    📊 Database query for user {$id}\n";
        return $this->data[$id] ?? null;
    }

    public function updateUser($id, $data)
    {
        usleep(50000); // 50ms delay
        echo "    📊 Database update for user {$id}\n";
        if (isset($this->data[$id])) {
            $this->data[$id] = array_merge($this->data[$id], $data);
            return true;
        }
        return false;
    }

    public function getUsersByPrefix($namePrefix)
    {
        usleep(200000); // 200ms delay
        echo "    📊 Database query for users with name prefix '{$namePrefix}'\n";
        $results = [];
        foreach ($this->data as $user) {
            if (stripos($user['name'], $namePrefix) === 0) {
                $results[] = $user;
            }
        }
        return $results;
    }
}

/**
 * Cache wrapper class demonstrating various caching patterns
 */
class CacheManager
{
    private $client;
    private $defaultTtl;

    public function __construct($client, $defaultTtl = 300)
    {
        $this->client = $client;
        $this->defaultTtl = $defaultTtl;
    }

    /**
     * Simple cache-aside pattern
     */
    public function get($key, $callback = null, $ttl = null)
    {
        $ttl = $ttl ?? $this->defaultTtl;

        // Try to get from cache first
        $cached = $this->client->get($key);
        if ($cached !== null) {
            echo "    Cache HIT for key: {$key}\n";
            return json_decode($cached, true);
        }

        echo "    Cache MISS for key: {$key}\n";

        // If callback provided, execute it and cache the result
        if ($callback && is_callable($callback)) {
            $data = $callback();
            if ($data !== null) {
                $this->set($key, $data, $ttl);
            }
            return $data;
        }

        return null;
    }

    /**
     * Set cache with TTL
     */
    public function set($key, $data, $ttl = null)
    {
        $ttl = $ttl ?? $this->defaultTtl;
        $value = json_encode($data);
        $this->client->set($key, $value, ['EX' => $ttl]);
        echo "    💾 Cached key: {$key} (TTL: {$ttl}s)\n";
    }

    /**
     * Delete cache entry
     */
    public function delete($key)
    {
        $result = $this->client->del([$key]);
        echo "    🗑️  Deleted cache key: {$key} (result: {$result})\n";
        return $result > 0;
    }
}

$client = null;
try {
    $client = new ValkeyGlide();
    $client->connect(addresses: $addresses, use_tls: false, request_timeout: 5000);
    echo "Connected to Valkey server\n\n";

    // Initialize our components
    $database = new MockDatabase();
    $cache = new CacheManager($client, 60); // 60 second default TTL

    // =============================================================================
    // BASIC CACHE-ASIDE PATTERN
    // =============================================================================
    echo "1️⃣  Basic Cache-Aside Pattern:\n";
    echo "------------------------------\n";

    $userId = 1;
    $cacheKey = "user:{$userId}";

    echo "First request (cache miss expected):\n";
    $user1 = $cache->get($cacheKey, function () use ($database, $userId) {
        return $database->getUser($userId);
    });
    echo "   Result: " . json_encode($user1) . "\n\n";

    echo "Second request (cache hit expected):\n";
    $user2 = $cache->get($cacheKey);
    echo "   Result: " . json_encode($user2) . "\n\n";

    // =============================================================================
    // WRITE-THROUGH CACHE PATTERN
    // =============================================================================
    echo "2️⃣  Write-Through Cache Pattern:\n";
    echo "-------------------------------\n";

    function updateUserWithCache($database, $cache, $userId, $updateData)
    {
        $cacheKey = "user:{$userId}";

        // Update database first
        $success = $database->updateUser($userId, $updateData);

        if ($success) {
            // Then update cache
            $updatedUser = $database->getUser($userId);
            $cache->set($cacheKey, $updatedUser);
            echo "   Write-through completed for user {$userId}\n";
            return $updatedUser;
        }

        return null;
    }

    $updatedUser = updateUserWithCache($database, $cache, 1, ['name' => 'John Updated']);
    echo "   Updated user: " . json_encode($updatedUser) . "\n\n";

    // Verify cache was updated
    echo "Verifying cache was updated:\n";
    $cachedUser = $cache->get("user:1");
    echo "   Cached user: " . json_encode($cachedUser) . "\n\n";

    // =============================================================================
    // CACHE WARMING
    // =============================================================================
    echo "3️⃣  Cache Warming:\n";
    echo "-----------------\n";

    echo "Warming cache for users 2 and 3:\n";
    foreach ([2, 3] as $userId) {
        $user = $database->getUser($userId);
        $cache->set("user:{$userId}", $user, 120); // 2 minute TTL
        echo "   Warmed cache for user {$userId}\n";
    }
    echo "\n";

    // =============================================================================
    // MULTI-LEVEL CACHING
    // =============================================================================
    echo "4️⃣  Multi-level Caching (with different TTLs):\n";
    echo "----------------------------------------------\n";

    function getWithMultiLevelCache($cache, $database, $userId)
    {
        // Level 1: Short-term cache (30 seconds)
        $shortTermKey = "user:short:{$userId}";
        $user = $cache->get($shortTermKey);

        if ($user !== null) {
            echo "    L1 Cache HIT (short-term)\n";
            return $user;
        }

        // Level 2: Long-term cache (5 minutes)
        $longTermKey = "user:long:{$userId}";
        $user = $cache->get($longTermKey);

        if ($user !== null) {
            echo "    L2 Cache HIT (long-term) - promoting to L1\n";
            $cache->set($shortTermKey, $user, 30); // Promote to L1
            return $user;
        }

        // Level 3: Database
        echo "   📊 Database fetch - caching in both levels\n";
        $user = $database->getUser($userId);
        if ($user) {
            $cache->set($shortTermKey, $user, 30);  // L1: 30 seconds
            $cache->set($longTermKey, $user, 300);  // L2: 5 minutes
        }

        return $user;
    }

    // Demonstrate multi-level caching
    for ($i = 1; $i <= 3; $i++) {
        echo "Request #{$i} for user 2:\n";
        $user = getWithMultiLevelCache($cache, $database, 2);
        sleep(1); // Brief pause between requests
    }
    echo "\n";

    // =============================================================================
    // CACHE INVALIDATION STRATEGIES
    // =============================================================================
    echo "5️⃣  Cache Invalidation Strategies:\n";
    echo "----------------------------------\n";

    // Tag-based invalidation
    echo "Setting up tagged cache entries:\n";
    $cache->set("post:1", ['id' => 1, 'title' => 'Post 1', 'author_id' => 1], 300);
    $cache->set("post:2", ['id' => 2, 'title' => 'Post 2', 'author_id' => 1], 300);
    $cache->set("comment:1", ['id' => 1, 'post_id' => 1, 'author_id' => 1], 300);

    // Create tag mappings
    $cache->set("tag:author:1", json_encode(['post:1', 'post:2', 'comment:1']), 300);

    echo "   Created posts and comments for author 1\n";

    // Invalidate all content by author
    echo "Invalidating all content by author 1:\n";
    $taggedKeys = json_decode($cache->get("tag:author:1") ?? '[]', true);
    foreach ($taggedKeys as $key) {
        $cache->delete($key);
    }
    $cache->delete("tag:author:1");
    echo "   Invalidated all tagged content\n\n";

    // =============================================================================
    // CACHE PERFORMANCE MONITORING
    // =============================================================================
    echo "6️⃣  Cache Performance Monitoring:\n";
    echo "---------------------------------\n";

    // Simulate some cache activity
    for ($i = 1; $i <= 5; $i++) {
        $cache->set("metric:test:{$i}", ['data' => "test{$i}"], 30);
    }



    // =============================================================================
    // CACHE WARMING STRATEGIES
    // =============================================================================
    echo "8️⃣  Cache Warming Strategies:\n";
    echo "----------------------------\n";

    // Proactive warming based on access patterns
    echo "Proactive cache warming for popular data:\n";
    $popularUserIds = [1, 2, 3]; // In reality, this might come from analytics

    foreach ($popularUserIds as $userId) {
        $warmKey = "user:warm:{$userId}";
        $user = $database->getUser($userId);
        $cache->set($warmKey, $user, 1800); // 30 minutes for popular data
        echo "   Warmed popular user {$userId}\n";
    }

    // Lazy warming with extended TTL
    echo "\nLazy warming with extended TTL:\n";
    function getLazyWarmed($cache, $database, $userId)
    {
        $key = "user:lazy:{$userId}";
        $extendedKey = "user:lazy:{$userId}:extended";

        // Try primary cache
        $user = $cache->get($key);
        if ($user !== null) {
            return $user;
        }

        // Try extended cache (stale data acceptable)
        $staleUser = $cache->get($extendedKey);
        if ($staleUser !== null) {
            echo "   📦 Serving stale data while refreshing...\n";

            // Asynchronously refresh in background (simulated)
            $freshUser = $database->getUser($userId);
            $cache->set($key, $freshUser, 60);              // Fresh cache
            $cache->set($extendedKey, $freshUser, 3600);    // Extended cache

            return $staleUser; // Return stale data immediately
        }

        // No cache at all, fetch fresh
        $user = $database->getUser($userId);
        $cache->set($key, $user, 60);
        $cache->set($extendedKey, $user, 3600);
        return $user;
    }

    $lazyUser = getLazyWarmed($cache, $database, 1);
    echo "   Retrieved user: {$lazyUser['name']}\n\n";
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    exit(1);
} finally {
    if ($client) {
        $client->close();
        echo " Connection closed.\n";
    }
}

echo "\n Caching Best Practices Summary:\n";
echo "---------------------------------\n";
echo "• Use cache-aside for read-heavy workloads\n";
echo "• Implement write-through for consistency requirements\n";
echo "• Use appropriate TTL values based on data sensitivity\n";
echo "• Implement cache warming for predictable access patterns\n";
echo "• Use multi-level caching for different access frequencies\n";
echo "• Implement stampede protection for expensive operations\n";
echo "• Use pattern-based invalidation for related data\n";
echo "• Monitor cache hit rates and adjust strategies accordingly\n";
echo "• Consider stale-while-revalidate for acceptable use cases\n";
echo "• Use consistent cache key naming conventions\n";

echo "\n🔗 Related Examples:\n";
echo "- Session Storage: php patterns/session_store.php\n";
echo "- Rate Limiting: php patterns/rate_limiting.php\n";
echo "- Error Handling: php error_handling/connection_errors.php\n";
