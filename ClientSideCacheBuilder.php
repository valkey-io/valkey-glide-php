<?php

declare(strict_types=1);

namespace ValkeyGlide\Cache;

use ValkeyGlideException;

/**
 * Builder for ClientSideCache.
 */
class ClientSideCacheBuilder
{
    private ?int $maxCacheKb = null;
    private ?int $entryTtlMs = null;
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
     * Sets the Time-To-Live for cached entries in milliseconds.
     *
     * After this duration, entries automatically expire and are removed
     * from the cache. Set to 0 to disable TTL expiration (entries remain
     * until evicted or invalidated).
     *
     * @param int $entryTtlMs TTL in milliseconds (must be non-negative, 0 = no expiration).
     * @return self This builder instance for method chaining.
     */
    public function entryTtlMs(int $entryTtlMs): self
    {
        if ($entryTtlMs < 0) {
            throw new ValkeyGlideException("entryTtlMs must be non-negative (0 = no expiration)");
        }
        $this->entryTtlMs = $entryTtlMs;
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
     * Gets the entry TTL in milliseconds.
     *
     * @return int The entry TTL in milliseconds (0 = no expiration).
     */
    public function getEntryTtlMs(): int
    {
        if ($this->entryTtlMs === null) {
            throw new ValkeyGlideException("entryTtlMs is required");
        }
        return $this->entryTtlMs;
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
     * Each call to build() generates a new unique cache ID using a GUID.
     *
     * @return string The cache ID.
     */
    public function getCacheId(): string
    {
        return bin2hex(random_bytes(16));
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
        if ($this->entryTtlMs === null) {
            throw new ValkeyGlideException("entryTtlMs is required");
        }

        return new ClientSideCache($this);
    }
}
