<?php

declare(strict_types=1);

namespace ValkeyGlide\Cache;

/**
 * Configuration for client-side caching with TTL-based expiration.
 *
 * This class configures a local cache that stores read command responses
 * on the client side to reduce network round-trips and server load. The cache
 * uses Time-To-Live (TTL) based expiration, where entries are automatically
 * removed after a specified duration.
 *
 * Cached entries expire based on TTL. Server-side key changes are not propagated
 * to the cache, so values may become stale before TTL expires.
 * Expiration is lazy — entries are removed when accessed after their TTL, not
 * proactively in the background.
 * Supported read commands: GET, HGETALL, SMEMBERS.
 *
 * In order for 2 clients to share the same cache, they must be created with the
 * same ClientSideCache instance.
 *
 * Clients with different ClientSideCache instances will have separate caches,
 * even if the configurations are identical.
 * Clients using different DBs cannot share the same cache.
 * Clients using different ACL users cannot share the same cache.
 */
class ClientSideCache
{
    /** @var int Eviction policy: Least Recently Used */
    public const EVICTION_LRU = 0;

    /** @var int Eviction policy: Least Frequently Used */
    public const EVICTION_LFU = 1;

    private string $cacheId;
    private int $maxCacheKb;
    private int $entryTtlMs;
    private ?int $evictionPolicy;
    private bool $enableMetrics;

    /**
     * Creates a new ClientSideCache from a builder.
     *
     * @param ClientSideCacheBuilder $builder The builder containing configuration values.
     */
    public function __construct(ClientSideCacheBuilder $builder)
    {
        $this->cacheId = $builder->getCacheId();
        $this->maxCacheKb = $builder->getMaxCacheKb();
        $this->entryTtlMs = $builder->getEntryTtlMs();
        $this->evictionPolicy = $builder->getEvictionPolicy();
        $this->enableMetrics = $builder->getEnableMetrics();
    }

    /**
     * Creates a new ClientSideCache builder.
     *
     * @return ClientSideCacheBuilder A new builder instance.
     */
    public static function builder(): ClientSideCacheBuilder
    {
        return new ClientSideCacheBuilder();
    }

    /**
     * Gets the unique cache identifier.
     *
     * @return string The cache ID.
     */
    public function getCacheId(): string
    {
        return $this->cacheId;
    }

    /**
     * Gets the maximum cache size in kilobytes.
     *
     * @return int The maximum cache size in KB.
     */
    public function getMaxCacheKb(): int
    {
        return $this->maxCacheKb;
    }

    /**
     * Gets the entry TTL in milliseconds.
     *
     * @return int The entry TTL in milliseconds (0 = no expiration).
     */
    public function getEntryTtlMs(): int
    {
        return $this->entryTtlMs;
    }

    /**
     * Gets the eviction policy.
     *
     * @return int|null The eviction policy constant, or null for default (LRU).
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
     * Converts the configuration to an associative array suitable for passing
     * to ValkeyGlide::connect() or ValkeyGlideCluster::__construct().
     *
     * @return array The configuration as an associative array.
     */
    public function toArray(): array
    {
        $result = [
            'cache_id' => $this->cacheId,
            'max_cache_kb' => $this->maxCacheKb,
            'entry_ttl_ms' => $this->entryTtlMs,
            'enable_metrics' => $this->enableMetrics,
        ];

        if ($this->evictionPolicy !== null) {
            $result['eviction_policy'] = $this->evictionPolicy;
        }

        return $result;
    }
}
