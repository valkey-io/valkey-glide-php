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
    /* Default cache configuration values */
    private const DEFAULT_MAX_CACHE_KB = 1;
    private const DEFAULT_ENTRY_TTL_MS = 60000;
    private const LARGE_MAX_CACHE_KB   = 1024;

    /* TTL for expiration tests (short enough to test within sleep) */
    private const SHORT_TTL_MS         = 2000;
    private const TTL_EXPIRY_WAIT_SECS = 3;

    /* Value size used to force eviction in a 1 KB cache */
    private const EVICTION_VALUE_SIZE  = 250;

    /* Frequency counts for LFU eviction test */
    private const LFU_HIGH_FREQUENCY   = 5;
    private const LFU_MEDIUM_FREQUENCY = 2;

    /* Number of keys for multi-key tests */
    private const MULTI_KEY_COUNT      = 3;

    /* Invalid eviction policy value for validation test */
    private const INVALID_EVICTION_POLICY = 99;

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
     * Helper to build a ClientSideCache config with common defaults.
     *
     * @param int $maxCacheKb     Maximum cache size in KB.
     * @param int $entryTtlMs     Entry TTL in milliseconds.
     * @param bool $enableMetrics Whether to enable metrics.
     * @param int|null $evictionPolicy Eviction policy constant, or null for default.
     * @return ClientSideCache
     */
    protected function buildCache(
        int $maxCacheKb = self::DEFAULT_MAX_CACHE_KB,
        int $entryTtlMs = self::DEFAULT_ENTRY_TTL_MS,
        bool $enableMetrics = true,
        ?int $evictionPolicy = null
    ): ClientSideCache {
        $builder = ClientSideCache::builder()
            ->maxCacheKb($maxCacheKb)
            ->entryTtlMs($entryTtlMs)
            ->enableMetrics($enableMetrics);

        if ($evictionPolicy !== null) {
            $builder->evictionPolicy($evictionPolicy);
        }

        return $builder->build();
    }

    /**
     * Assert that all metrics-dependent methods throw on the given client.
     *
     * @param ValkeyGlide $client The client to test.
     * @param string $pattern     Regex pattern to match against the exception message.
     */
    protected function assertAllMetricsThrow(ValkeyGlide $client, string $pattern): void
    {
        $metricsMethods = [
            'getCacheHitRate',
            'getCacheMissRate',
            'getCacheTotalLookups',
            'getCacheEvictions',
            'getCacheExpirations',
        ];

        foreach ($metricsMethods as $method) {
            $this->assertThrowsMatch(null, function () use ($client, $method) {
                $client->$method();
            }, $pattern);
        }
    }

    /**
     * Test basic cache hit/miss behavior with metrics tracking.
     */
    public function testBasicCacheHitWithMetrics()
    {
        $cache = $this->buildCache();
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
        $totalLookups = $client->getCacheTotalLookups();

        $this->assertBetween($hitRate, 0.60, 0.70);
        $this->assertBetween($missRate, 0.30, 0.40);
        // Rates should sum to ~1.0
        $this->assertBetween($hitRate + $missRate, 0.99, 1.01);
        $this->assertEquals(3, $totalLookups);

        $client->close();
    }

    /**
     * Test that cache works but metrics throw when disabled.
     */
    public function testCacheWithoutMetrics()
    {
        $cache = $this->buildCache(enableMetrics: false);
        $client = $this->newCachedInstance($cache->toArray());

        $this->assertTrue($client->set('key', 'value'));
        $this->assertEquals('value', $client->get('key'));
        $this->assertEquals('value', $client->get('key'));

        // Metrics should throw
        $this->assertAllMetricsThrow($client, '/metrics/i');

        // Entry count should still work
        $this->assertEquals(1, $client->getCacheEntryCount());

        $client->close();
    }

    /**
     * Test that NIL values are not cached.
     */
    public function testCacheNilValuesNotCached()
    {
        $cache = $this->buildCache();
        $client = $this->newCachedInstance($cache->toArray());

        $this->assertFalse($client->get('nonexistent_key'));
        $this->assertEquals(0, $client->getCacheEntryCount());

        $this->assertFalse($client->get('nonexistent_key'));

        $missRate = $client->getCacheMissRate();
        $this->assertBetween($missRate, 0.99, 1.01);
        $this->assertEquals(2, $client->getCacheTotalLookups());

        $client->close();
    }

    /**
     * Test that cache entries expire after TTL.
     */
    public function testCacheTtlExpiration()
    {
        $cache = $this->buildCache(entryTtlMs: self::SHORT_TTL_MS);
        $client = $this->newCachedInstance($cache->toArray());

        $this->assertTrue($client->set('ttl_key', 'ttl_value'));
        $this->assertEquals('ttl_value', $client->get('ttl_key'));
        $this->assertEquals(1, $client->getCacheEntryCount());

        // Second GET - from cache
        $this->assertEquals('ttl_value', $client->get('ttl_key'));

        // Wait for TTL to expire
        sleep(self::TTL_EXPIRY_WAIT_SECS);

        // GET after expiration - should fetch from server again
        $this->assertEquals('ttl_value', $client->get('ttl_key'));

        $this->assertEquals(1, $client->getCacheExpirations());
        $this->assertEquals(3, $client->getCacheTotalLookups());

        $client->close();
    }

    /**
     * Test caching of multiple keys.
     */
    public function testCacheMultipleKeys()
    {
        $cache = $this->buildCache();
        $client = $this->newCachedInstance($cache->toArray());

        for ($i = 1; $i <= self::MULTI_KEY_COUNT; $i++) {
            $this->assertTrue($client->set("key{$i}", "value{$i}"));
        }

        // GET each key twice (miss + hit)
        for ($i = 1; $i <= self::MULTI_KEY_COUNT; $i++) {
            $this->assertEquals("value{$i}", $client->get("key{$i}"));
            $this->assertEquals("value{$i}", $client->get("key{$i}"));
        }

        $this->assertEquals(self::MULTI_KEY_COUNT, $client->getCacheEntryCount());

        // MULTI_KEY_COUNT misses + MULTI_KEY_COUNT hits = 50% hit rate
        $hitRate = $client->getCacheHitRate();
        $this->assertBetween($hitRate, 0.45, 0.55);
        $this->assertEquals(self::MULTI_KEY_COUNT * 2, $client->getCacheTotalLookups());

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

        // All metrics (including entry count) should throw when cache is not configured
        $this->assertAllMetricsThrow($client, '/not enabled/i');

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
        $cache = $this->buildCache(evictionPolicy: ClientSideCache::EVICTION_LRU);
        $client = $this->newCachedInstance($cache->toArray());

        $value = str_repeat('x', self::EVICTION_VALUE_SIZE);

        for ($i = 1; $i <= self::MULTI_KEY_COUNT; $i++) {
            $this->assertTrue($client->set("lru_key{$i}", $value));
            $this->assertEquals($value, $client->get("lru_key{$i}"));
        }

        $this->assertEquals(self::MULTI_KEY_COUNT, $client->getCacheEntryCount());

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
        $cache = $this->buildCache(evictionPolicy: ClientSideCache::EVICTION_LFU);
        $client = $this->newCachedInstance($cache->toArray());

        $value = str_repeat('x', self::EVICTION_VALUE_SIZE);

        // key1: high frequency
        $this->assertTrue($client->set('key1', $value));
        for ($j = 0; $j < self::LFU_HIGH_FREQUENCY; $j++) {
            $this->assertEquals($value, $client->get('key1'));
        }

        // key2: medium frequency
        $this->assertTrue($client->set('key2', $value));
        for ($j = 0; $j < self::LFU_MEDIUM_FREQUENCY; $j++) {
            $this->assertEquals($value, $client->get('key2'));
        }

        // key3: low frequency
        $this->assertTrue($client->set('key3', $value));
        $this->assertEquals($value, $client->get('key3'));

        $this->assertEquals(self::MULTI_KEY_COUNT, $client->getCacheEntryCount());

        // key4 should trigger eviction of key3 (lowest frequency)
        $this->assertTrue($client->set('key4', $value));
        $this->assertEquals($value, $client->get('key4'));

        $this->assertEquals(self::MULTI_KEY_COUNT, $client->getCacheEntryCount());
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

        // entryTtlMs must be non-negative
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->maxCacheKb(self::DEFAULT_MAX_CACHE_KB)->entryTtlMs(-1)->build();
        });

        // entryTtlMs is required
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->maxCacheKb(self::DEFAULT_MAX_CACHE_KB)->build();
        });

        // maxCacheKb is required
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()->entryTtlMs(self::DEFAULT_ENTRY_TTL_MS)->build();
        });

        // Invalid eviction policy
        $this->assertThrowsMatch(null, function () {
            ClientSideCache::builder()
                ->maxCacheKb(self::DEFAULT_MAX_CACHE_KB)
                ->entryTtlMs(self::DEFAULT_ENTRY_TTL_MS)
                ->evictionPolicy(self::INVALID_EVICTION_POLICY)
                ->build();
        });
    }

    /**
     * Test that cache IDs are unique across builder instances.
     */
    public function testUniqueCacheIds()
    {
        $cache1 = $this->buildCache();
        $cache2 = $this->buildCache();

        $id1 = $cache1->getCacheId();
        $id2 = $cache2->getCacheId();

        $this->assertNotEquals($id1, $id2);
    }

    /**
     * Test toArray() produces the expected structure.
     */
    public function testToArray()
    {
        $cache = $this->buildCache(
            maxCacheKb: self::LARGE_MAX_CACHE_KB,
            evictionPolicy: ClientSideCache::EVICTION_LFU
        );

        $arr = $cache->toArray();

        $this->assertArrayHasKey('cache_id', $arr);
        $this->assertEquals(self::LARGE_MAX_CACHE_KB, $arr['max_cache_kb']);
        $this->assertEquals(self::DEFAULT_ENTRY_TTL_MS, $arr['entry_ttl_ms']);
        $this->assertEquals(ClientSideCache::EVICTION_LFU, $arr['eviction_policy']);
        $this->assertTrue($arr['enable_metrics']);
    }
}
