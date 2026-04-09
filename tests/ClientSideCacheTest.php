<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";
require_once __DIR__ . "/../ClientSideCache.php";
require_once __DIR__ . "/../ClientSideCacheBuilder.php";

use ValkeyGlide\Cache\ClientSideCache;

/**
 * Client-Side Cache Test
 * Tests client-side caching configuration, metrics, and behavior.
 */
class ClientSideCacheTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    /**
     * Helper to create a connected client with client-side cache enabled.
     */
    protected function newCachedInstance(array $cacheConfig): ValkeyGlide
    {
        $client = new ValkeyGlide();
        $addresses = [[
            'host' => $this->getHost(),
            'port' => $this->getPort()
        ]];

        if ($this->getTLS()) {
            $client->connect(
                addresses: $addresses,
                use_tls: true,
                advanced_config: ['tls_config' => ['use_insecure_tls' => true]],
                client_side_cache: $cacheConfig
            );
        } else {
            $client->connect(
                addresses: $addresses,
                client_side_cache: $cacheConfig
            );
        }

        if ($this->getAuth()) {
            $this->assertTrue($client->auth($this->getAuth()));
        }

        return $client;
    }

    /**
     * Test basic cache hit/miss behavior with metrics tracking.
     */
    public function testBasicCacheHitWithMetrics()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->entryTtlSeconds(60)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        $this->assertTrue($client->set('cache_test_key', 'cache_test_value'));

        // First GET - cache miss
        $this->assertEquals('cache_test_value', $client->get('cache_test_key'));
        $this->assertEquals(1, $client->getCacheEntryCount());

        // Second and third GET - cache hits
        $this->assertEquals('cache_test_value', $client->get('cache_test_key'));
        $this->assertEquals('cache_test_value', $client->get('cache_test_key'));

        // Verify metrics: 1 miss + 2 hits = 3 total, hit rate ~66.67%
        $hitRate = $client->getCacheHitRate();
        $missRate = $client->getCacheMissRate();

        $this->assertBetween($hitRate, 0.60, 0.70);
        $this->assertBetween($missRate, 0.30, 0.40);
        // Rates should sum to ~1.0
        $this->assertBetween($hitRate + $missRate, 0.99, 1.01);

        $client->close();
    }

    /**
     * Test that cache works but metrics throw when disabled.
     */
    public function testCacheWithoutMetrics()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->entryTtlSeconds(60)
            ->build(); // enableMetrics defaults to false

        $client = $this->newCachedInstance($cache->toArray());

        $this->assertTrue($client->set('key', 'value'));
        $this->assertEquals('value', $client->get('key'));
        $this->assertEquals('value', $client->get('key'));

        // Metrics should throw
        $this->assertThrowsMatch(null, function () use ($client) {
            $client->getCacheHitRate();
        }, '/metrics/i');

        // Entry count should still work
        $this->assertEquals(1, $client->getCacheEntryCount());

        $client->close();
    }

    /**
     * Test that NIL values are not cached.
     */
    public function testCacheNilValuesNotCached()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->entryTtlSeconds(60)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        $this->assertFalse($client->get('nonexistent_key'));
        $this->assertEquals(0, $client->getCacheEntryCount());

        $this->assertFalse($client->get('nonexistent_key'));

        $missRate = $client->getCacheMissRate();
        $this->assertBetween($missRate, 0.99, 1.01);

        $client->close();
    }

    /**
     * Test that cache entries expire after TTL.
     */
    public function testCacheTtlExpiration()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->entryTtlSeconds(2)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        $this->assertTrue($client->set('ttl_key', 'ttl_value'));
        $this->assertEquals('ttl_value', $client->get('ttl_key'));
        $this->assertEquals(1, $client->getCacheEntryCount());

        // Second GET - from cache
        $this->assertEquals('ttl_value', $client->get('ttl_key'));

        // Wait for TTL to expire
        sleep(3);

        // GET after expiration - should fetch from server again
        $this->assertEquals('ttl_value', $client->get('ttl_key'));

        $this->assertEquals(1, $client->getCacheExpirations());

        $client->close();
    }

    /**
     * Test caching of multiple keys.
     */
    public function testCacheMultipleKeys()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->entryTtlSeconds(60)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        for ($i = 1; $i <= 3; $i++) {
            $this->assertTrue($client->set("key{$i}", "value{$i}"));
        }

        // GET each key twice (miss + hit)
        for ($i = 1; $i <= 3; $i++) {
            $this->assertEquals("value{$i}", $client->get("key{$i}"));
            $this->assertEquals("value{$i}", $client->get("key{$i}"));
        }

        $this->assertEquals(3, $client->getCacheEntryCount());

        // 3 misses + 3 hits = 50% hit rate
        $hitRate = $client->getCacheHitRate();
        $this->assertBetween($hitRate, 0.45, 0.55);

        $client->close();
    }

    /**
     * Test that without cache, metrics are not available.
     */
    public function testNoCacheMetrics()
    {
        $client = $this->newInstance();

        $this->assertTrue($client->set('key', 'value'));
        $this->assertEquals('value', $client->get('key'));

        $this->assertThrowsMatch(null, function () use ($client) {
            $client->getCacheHitRate();
        }, '/not enabled/i');

        $this->assertThrowsMatch(null, function () use ($client) {
            $client->getCacheEntryCount();
        }, '/not enabled/i');

        $client->close();
    }

    /**
     * Test LRU eviction policy.
     */
    public function testCacheEvictionPolicyLru()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->evictionPolicy(ClientSideCache::EVICTION_LRU)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        $value = str_repeat('x', 250);

        for ($i = 1; $i <= 3; $i++) {
            $this->assertTrue($client->set("lru_key{$i}", $value));
            $this->assertEquals($value, $client->get("lru_key{$i}"));
        }

        $this->assertEquals(3, $client->getCacheEntryCount());

        // Access key1 to make it recently used
        $this->assertEquals($value, $client->get('lru_key1'));

        // Add 2 more keys - should evict key2 and key3
        for ($i = 4; $i <= 5; $i++) {
            $this->assertTrue($client->set("lru_key{$i}", $value));
            $this->assertEquals($value, $client->get("lru_key{$i}"));
        }

        $this->assertEquals(2, $client->getCacheEvictions());

        $client->close();
    }

    /**
     * Test LFU eviction policy.
     */
    public function testCacheEvictionPolicyLfu()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1)
            ->evictionPolicy(ClientSideCache::EVICTION_LFU)
            ->enableMetrics()
            ->build();

        $client = $this->newCachedInstance($cache->toArray());

        $value = str_repeat('x', 250);

        // key1: high frequency
        $this->assertTrue($client->set('key1', $value));
        for ($j = 0; $j < 5; $j++) {
            $this->assertEquals($value, $client->get('key1'));
        }

        // key2: medium frequency
        $this->assertTrue($client->set('key2', $value));
        for ($j = 0; $j < 2; $j++) {
            $this->assertEquals($value, $client->get('key2'));
        }

        // key3: low frequency
        $this->assertTrue($client->set('key3', $value));
        $this->assertEquals($value, $client->get('key3'));

        $this->assertEquals(3, $client->getCacheEntryCount());

        // key4 should trigger eviction of key3 (lowest frequency)
        $this->assertTrue($client->set('key4', $value));
        $this->assertEquals($value, $client->get('key4'));

        $this->assertEquals(3, $client->getCacheEntryCount());
        $this->assertEquals(1, $client->getCacheEvictions());

        $client->close();
    }

    /**
     * Test the ClientSideCache builder validation.
     */
    public function testBuilderValidation()
    {
        // maxCacheKb must be positive
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->maxCacheKb(0)->build();
        });

        // entryTtlSeconds must be positive
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->maxCacheKb(1)->entryTtlSeconds(-1)->build();
        });

        // maxCacheKb is required
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->build();
        });

        // Invalid eviction policy
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->maxCacheKb(1)->evictionPolicy(99)->build();
        });
    }

    /**
     * Test that cache IDs are unique across builder instances.
     */
    public function testUniqueCacheIds()
    {
        $cache1 = ClientSideCache::builder()->maxCacheKb(1)->build();
        $cache2 = ClientSideCache::builder()->maxCacheKb(1)->build();

        $id1 = $cache1->getCacheId();
        $id2 = $cache2->getCacheId();

        if ($id1 === $id2) {
            $this->assert('Cache IDs should be unique but got: %s and %s', $id1, $id2);
        }
    }

    /**
     * Test toArray() produces the expected structure.
     */
    public function testToArray()
    {
        $cache = ClientSideCache::builder()
            ->maxCacheKb(1024)
            ->entryTtlSeconds(60)
            ->evictionPolicy(ClientSideCache::EVICTION_LFU)
            ->enableMetrics()
            ->build();

        $arr = $cache->toArray();

        $this->assertArrayHasKey('cache_id', $arr);
        $this->assertEquals(1024, $arr['max_cache_kb']);
        $this->assertEquals(60, $arr['entry_ttl_seconds']);
        $this->assertEquals(ClientSideCache::EVICTION_LFU, $arr['eviction_policy']);
        $this->assertTrue($arr['enable_metrics']);
    }
}
