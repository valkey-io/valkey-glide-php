<?php

declare(strict_types=1);

namespace ValkeyGlide\Cache;

use ValkeyGlideException;

/**
 * Builder for ClientSideCache.
 */
class ClientSideCacheBuilder
{
    private static string $uuidPrefix = '';
    private static int $counter = 0;

    private ?int $maxCacheKb = null;
    private ?int $entryTtlSeconds = null;
    private ?int $evictionPolicy = null;
    private bool $enableMetrics = false;

    /**
     * Sets the maximum cache size in kilobytes.
     *
     * This limits the total memory used by cached keys and values.
     * When this limit is reached, entries are evicted based on the eviction policy.
     *
     * @param int $maxCacheKb Maximum cache size in KB (must be positive).
     * @return self This builder instance for method chaining.
     */
    public function maxCacheKb(int $maxCacheKb): self
    {
        if ($maxCacheKb <= 0) {
            throw new ValkeyGlideException("maxCacheKb must be positive");
        }
        $this->maxCacheKb = $maxCacheKb;
        return $this;
    }

    /**
     * Sets the Time-To-Live for cached entries in seconds.
     *
     * After this duration, entries automatically expire and are removed
     * from the cache. If not specified, no expiration is applied.
     *
     * @param int $entryTtlSeconds TTL in seconds (must be positive).
     * @return self This builder instance for method chaining.
     */
    public function entryTtlSeconds(int $entryTtlSeconds): self
    {
        if ($entryTtlSeconds <= 0) {
            throw new ValkeyGlideException("entryTtlSeconds must be positive");
        }
        $this->entryTtlSeconds = $entryTtlSeconds;
        return $this;
    }

    /**
     * Sets the eviction policy for when the cache reaches its maximum size.
     *
     * @param int $evictionPolicy One of ClientSideCache::EVICTION_LRU or ClientSideCache::EVICTION_LFU.
     * @return self This builder instance for method chaining.
     */
    public function evictionPolicy(int $evictionPolicy): self
    {
        if (
            $evictionPolicy !== ClientSideCache::EVICTION_LRU
            && $evictionPolicy !== ClientSideCache::EVICTION_LFU
        ) {
            throw new ValkeyGlideException(
                "evictionPolicy must be ClientSideCache::EVICTION_LRU or ClientSideCache::EVICTION_LFU"
            );
        }
        $this->evictionPolicy = $evictionPolicy;
        return $this;
    }

    /**
     * Enables collection of cache metrics such as hit/miss rates.
     *
     * @param bool $enableMetrics Whether to enable metrics.
     * @return self This builder instance for method chaining.
     */
    public function enableMetrics(bool $enableMetrics = true): self
    {
        $this->enableMetrics = $enableMetrics;
        return $this;
    }

    /**
     * Gets the maximum cache size in kilobytes.
     *
     * @return int The maximum cache size in KB.
     */
    public function getMaxCacheKb(): int
    {
        if ($this->maxCacheKb === null) {
            throw new ValkeyGlideException("maxCacheKb is required");
        }
        return $this->maxCacheKb;
    }

    /**
     * Gets the entry TTL in seconds.
     *
     * @return int|null The entry TTL in seconds, or null if not set.
     */
    public function getEntryTtlSeconds(): ?int
    {
        return $this->entryTtlSeconds;
    }

    /**
     * Gets the eviction policy.
     *
     * @return int|null The eviction policy constant, or null for default.
     */
    public function getEvictionPolicy(): ?int
    {
        return $this->evictionPolicy;
    }

    /**
     * Gets whether metrics collection is enabled.
     *
     * @return bool True if metrics are enabled.
     */
    public function getEnableMetrics(): bool
    {
        return $this->enableMetrics;
    }

    /**
     * Gets the auto-generated unique cache ID.
     *
     * @return string The cache ID.
     */
    public function getCacheId(): string
    {
        if (self::$uuidPrefix === '') {
            self::$uuidPrefix = substr(bin2hex(random_bytes(4)), 0, 8);
        }
        $id = self::$uuidPrefix . '-' . self::$counter;
        self::$counter++;
        return $id;
    }

    /**
     * Builds the ClientSideCache configuration.
     *
     * @return ClientSideCache The immutable client-side cache configuration.
     */
    public function build(): ClientSideCache
    {
        if ($this->maxCacheKb === null) {
            throw new ValkeyGlideException("maxCacheKb is required");
        }

        return new ClientSideCache($this);
    }
}
