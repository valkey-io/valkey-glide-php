#!/usr/bin/env php
<?php
/**
 * Valkey GLIDE PHP Client - PHPRedis Compatibility Report
 *
 * Compares the Glide PHP client API against PHPRedis to calculate compatibility percentage.
 *
 * Categories:
 * 1. Compatible APIs - Methods implemented in Glide that match PHPRedis
 * 2. Not Compatible APIs - Methods in PHPRedis with different signatures/behavior in Glide
 * 3. Excluded from Comparison - PHPRedis methods not applicable (Redis/KeyDB-only or Valkey-only)
 * 4. Not Implemented - Commands supported by Valkey but not yet implemented in Glide
 *
 * Usage: php compatibility_check.php
 */

// ============================================================================
// PHPRedis public methods (from redis.stub.php on develop branch)
// Excludes: __construct, __destruct, internal helpers (_compress, _uncompress,
// _prefix, _serialize, _unserialize, _pack, _unpack, _digest)
// Excludes: deprecated aliases (open, popen, delete, sortAsc, sortAscAlpha, sortDesc, sortDescAlpha)
// Excludes: connection management internals (getHost, getPort, getAuth, getDBNum,
// getReadTimeout, getTimeout, getPersistentID, getMode, isConnected,
// getTransferredBytes, clearTransferredBytes)
// ============================================================================

$phpredis_commands = [
    // String commands
    'append', 'decr', 'decrBy', 'get', 'getBit', 'getEx', 'getDel', 'getRange',
    'getset', 'incr', 'incrBy', 'incrByFloat', 'lcs', 'mget', 'mset', 'msetnx',
    'psetex', 'set', 'setBit', 'setRange', 'setex', 'setnx', 'strlen',

    // Key commands
    'del', 'dump', 'exists', 'expire', 'expireAt', 'expiretime', 'keys',
    'move', 'object', 'persist', 'pexpire', 'pexpireAt', 'pexpiretime',
    'pttl', 'randomKey', 'rename', 'renameNx', 'restore', 'scan', 'sort',
    'sort_ro', 'touch', 'ttl', 'type', 'unlink', 'copy', 'wait',

    // Hash commands
    'hDel', 'hExists', 'hGet', 'hGetAll', 'hIncrBy', 'hIncrByFloat', 'hKeys',
    'hLen', 'hMget', 'hMset', 'hRandField', 'hSet', 'hSetNx', 'hStrLen', 'hVals',
    'hscan', 'hexpire', 'hpexpire', 'hexpireat', 'hpexpireat', 'httl', 'hpttl',
    'hexpiretime', 'hpexpiretime', 'hpersist', 'hgetex', 'hsetex', 'hgetdel',

    // List commands
    'blPop', 'brPop', 'brpoplpush', 'lInsert', 'lLen', 'lMove', 'blmove',
    'lPop', 'lPos', 'lPush', 'rPush', 'lPushx', 'rPushx', 'lSet', 'lindex',
    'lrange', 'lrem', 'ltrim', 'rPop', 'rpoplpush', 'blmpop', 'lmpop',

    // Set commands
    'sAdd', 'scard', 'sDiff', 'sDiffStore', 'sInter', 'sintercard', 'sInterStore',
    'sMembers', 'sMisMember', 'sMove', 'sPop', 'sRandMember', 'sUnion', 'sUnionStore',
    'sismember', 'srem', 'sscan',

    // Sorted Set commands
    'bzPopMax', 'bzPopMin', 'bzmpop', 'zmpop', 'zAdd', 'zCard', 'zCount', 'zIncrBy',
    'zLexCount', 'zMscore', 'zPopMax', 'zPopMin', 'zRange', 'zRangeByLex',
    'zRangeByScore', 'zrangestore', 'zRandMember', 'zRank', 'zRem', 'zRemRangeByLex',
    'zRemRangeByRank', 'zRemRangeByScore', 'zRevRange', 'zRevRangeByLex',
    'zRevRangeByScore', 'zRevRank', 'zScore', 'zdiff', 'zdiffstore', 'zinter',
    'zintercard', 'zinterstore', 'zscan', 'zunion', 'zunionstore',

    // HyperLogLog
    'pfadd', 'pfcount', 'pfmerge',

    // Geo commands
    'geoadd', 'geodist', 'geohash', 'geopos', 'georadius', 'georadius_ro',
    'georadiusbymember', 'georadiusbymember_ro', 'geosearch', 'geosearchstore',

    // Stream commands
    'xack', 'xadd', 'xautoclaim', 'xclaim', 'xdel', 'xgroup', 'xinfo', 'xlen',
    'xpending', 'xrange', 'xread', 'xreadgroup', 'xrevrange', 'xtrim',

    // Scripting
    'eval', 'eval_ro', 'evalsha', 'evalsha_ro', 'script', 'fcall', 'fcall_ro',
    'function',

    // Pub/Sub
    'publish', 'subscribe', 'psubscribe', 'unsubscribe', 'punsubscribe', 'pubsub',
    'ssubscribe', 'sunsubscribe',

    // Transaction
    'multi', 'exec', 'discard', 'watch', 'unwatch', 'pipeline',

    // Server
    'bgSave', 'bgrewriteaof', 'config', 'dbSize', 'flushAll', 'flushDB', 'info',
    'lastSave', 'save', 'select', 'slowlog', 'time', 'waitaof',

    // Connection
    'auth', 'client', 'close', 'echo', 'ping', 'rawcommand', 'setOption', 'getOption',

    // Error introspection (required by PHPRedis-compatible libraries, e.g. Symfony
    // Lock RedisStore calls clearLastError()/getLastError() on every evaluate())
    'getLastError', 'clearLastError',

    // Bit commands
    'bitcount', 'bitop', 'bitpos',

    // Other
    'acl', 'failover', 'migrate', 'role', 'slaveof', 'replicaof', 'swapdb', 'reset',

    // Valkey/Redis 8+ specific in PHPRedis
    'msetex', 'delex', 'delifeq', 'xdelex', 'digest', 'getWithMeta', 'hGetWithMeta',

    // Vector set commands (Redis 8+)
    'vadd', 'vsim', 'vcard', 'vdim', 'vinfo', 'vismember', 'vemb', 'vrandmember',
    'vrange', 'vrem', 'vsetattr', 'vgetattr', 'vlinks',

    // Rate limiting
    'gcra',

    // KeyDB specific
    'expiremember', 'expirememberat',

    // Misc
    'sAddArray', 'pconnect',
];

// ============================================================================
// Glide PHP implemented methods (derived dynamically via reflection)
// ============================================================================

$glide_internal_methods = ['__construct', '__destruct', 'connect', 'registerPHPRedisAliases', 'setOtelSamplePercentage'];

if (!extension_loaded('valkey_glide')) {
    fprintf(STDERR, "Error: valkey_glide extension is not loaded. Install it first.\n");
    exit(1);
}

$glide_commands = array_values(array_diff(
    array_map(fn($m) => $m->getName(), (new ReflectionClass('ValkeyGlide'))->getMethods(ReflectionMethod::IS_PUBLIC)),
    $glide_internal_methods
));

// ============================================================================
// Commands excluded from compatibility comparison
// These PHPRedis methods are not meaningful compatibility targets because they
// are either: not available in Valkey (Redis 8+/KeyDB-only), or available in
// Valkey but not present in PHPRedis (Valkey-specific commands).
// ============================================================================

$excluded_from_comparison = [
    // Vector set commands (Redis 8+ only, not in Valkey)
    'vadd', 'vsim', 'vcard', 'vdim', 'vinfo', 'vismember', 'vemb', 'vrandmember',
    'vrange', 'vrem', 'vsetattr', 'vgetattr', 'vlinks',

    // Redis 8+ specific commands (not in Valkey)
    'msetex',       // MSETEX - Redis 8+
    'delex',        // DELEX - Redis 8+
    'xdelex',       // XDELEX - Redis 8+
    'digest',       // DIGEST - Redis 8+
    'getWithMeta',  // GETWITHMETA - Redis 8+
    'hGetWithMeta', // HGETWITHMETA - Redis 8+

    // Valkey-specific commands (supported in Valkey but not in PHPRedis)
    'delifeq',      // DELIFEQ - Valkey 9+
    'waitaof',      // WAITAOF - available in Valkey, not a PHPRedis compat target

    // KeyDB-specific commands
    'expiremember',   // KeyDB only
    'expirememberat', // KeyDB only

    // Rate limiting (Redis 8+ module)
    'gcra',
];

// ============================================================================
// Commands with known incompatibilities (different signature/behavior)
// These exist in both but have meaningful differences
// ============================================================================

$not_compatible = [
    // PHPRedis: hExpire(key, fields_array, ttl, mode) - fields as array, mode after ttl
    // Glide:    hExpire(key, ttl, mode, field, ...other_fields) - mode before fields, variadic fields
    // Differences: (1) argument order (mode before fields), (2) variadic fields instead of array
    'hexpire', 'hpexpire', 'hexpireat', 'hpexpireat',

    // PHPRedis: hTtl(key, fields_array) - fields as array
    // Glide:    hTtl(key, field, ...other_fields) - variadic fields instead of array
    'httl', 'hpttl', 'hexpiretime', 'hpexpiretime',

    // PHPRedis: hPersist(key, fields_array) - fields as array
    // Glide:    hPersist(key, field, ...other_fields) - variadic fields instead of array
    'hpersist',

    // PHPRedis: hGetEx(key, fields_array, expiry) - expiry as positional args
    // Glide:    hGetEx(key, fields_array, options) - options as associative array
    'hgetex',

    // PHPRedis: hSetEx(key, fields_array, expiry) - fields as associative array
    // Glide:    hSetEx(key, seconds, mode, field, value, ...) - variadic field/value pairs
    'hsetex',
];

// ============================================================================
// Normalize command names for case-insensitive comparison
// ============================================================================

function normalize($commands) {
    $normalized = [];
    foreach ($commands as $cmd) {
        $normalized[strtolower($cmd)] = $cmd;
    }
    return $normalized;
}

$phpredis_norm = normalize($phpredis_commands);
$glide_norm = normalize($glide_commands);
$not_supported_norm = normalize($excluded_from_comparison);
$not_compatible_norm = normalize($not_compatible);

// ============================================================================
// Categorize each PHPRedis command
// ============================================================================

$compatible = [];
$incompatible = [];
$unsupported = [];
$not_implemented = [];

foreach ($phpredis_norm as $lower => $original) {
    if (isset($not_supported_norm[$lower])) {
        $unsupported[] = $original;
    } elseif (isset($not_compatible_norm[$lower])) {
        $incompatible[] = $original;
    } elseif (isset($glide_norm[$lower])) {
        $compatible[] = $original;
    } else {
        $not_implemented[] = $original;
    }
}

// ============================================================================
// Output Report
// ============================================================================

$total = count($phpredis_norm);

sort($compatible);
sort($incompatible);
sort($unsupported);
sort($not_implemented);

echo "╔══════════════════════════════════════════════════════════════════════╗\n";
echo "║     Valkey GLIDE PHP - PHPRedis Compatibility Report               ║\n";
echo "╠══════════════════════════════════════════════════════════════════════╣\n";
echo "║                                                                    ║\n";
printf("║  Total PHPRedis commands analyzed: %-32d║\n", $total);
echo "║                                                                    ║\n";
printf("║  1. Compatible APIs:              %3d / %3d  (%5.1f%%)              ║\n",
    count($compatible), $total, count($compatible) / $total * 100);
printf("║  2. Not Compatible APIs:          %3d / %3d  (%5.1f%%)              ║\n",
    count($incompatible), $total, count($incompatible) / $total * 100);
printf("║  3. Excluded from Comparison:     %3d / %3d  (%5.1f%%)              ║\n",
    count($unsupported), $total, count($unsupported) / $total * 100);
printf("║  4. Not Implemented:              %3d / %3d  (%5.1f%%)              ║\n",
    count($not_implemented), $total, count($not_implemented) / $total * 100);
echo "║                                                                    ║\n";
printf("║  Sum: %5.1f%%                                                      ║\n",
    (count($compatible) + count($incompatible) + count($unsupported) + count($not_implemented)) / $total * 100);
echo "║                                                                    ║\n";
echo "╚══════════════════════════════════════════════════════════════════════╝\n";

echo "\n";

// Detailed breakdown
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "1. COMPATIBLE APIs (" . count($compatible) . ")\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo wordwrap(implode(', ', $compatible), 70) . "\n\n";

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "2. NOT COMPATIBLE APIs (" . count($incompatible) . ") - Different signatures\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
foreach ($incompatible as $cmd) {
    echo "  - $cmd\n";
}
echo "\n";

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "3. EXCLUDED FROM COMPARISON (" . count($unsupported) . ") - Redis/KeyDB-only or Valkey-only\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
foreach ($unsupported as $cmd) {
    echo "  - $cmd\n";
}
echo "\n";

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "4. NOT IMPLEMENTED (" . count($not_implemented) . ") - Supported by Valkey, not yet in Glide\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
foreach ($not_implemented as $cmd) {
    echo "  - $cmd\n";
}
echo "\n";

// ============================================================================
// Generate Markdown report
// ============================================================================

$md = "# Valkey GLIDE PHP - PHPRedis Compatibility Report\n\n";
$md .= "Generated: " . date('Y-m-d H:i:s') . "\n\n";
$md .= "## Summary\n\n";
$md .= "| Category | Count | Percentage |\n";
$md .= "|----------|------:|-----------:|\n";
$md .= sprintf("| Compatible APIs | %d | %.1f%% |\n", count($compatible), count($compatible) / $total * 100);
$md .= sprintf("| Not Compatible APIs | %d | %.1f%% |\n", count($incompatible), count($incompatible) / $total * 100);
$md .= sprintf("| Excluded from Comparison | %d | %.1f%% |\n", count($unsupported), count($unsupported) / $total * 100);
$md .= sprintf("| Not Implemented | %d | %.1f%% |\n", count($not_implemented), count($not_implemented) / $total * 100);
$md .= sprintf("| **Total** | **%d** | **100.0%%** |\n", $total);

$md .= "\n## 1. Compatible APIs (" . count($compatible) . ")\n\n";
$md .= "Commands implemented in Glide with matching PHPRedis signatures.\n\n";
$md .= "`" . implode('`, `', $compatible) . "`\n";

$md .= "\n## 2. Not Compatible APIs (" . count($incompatible) . ")\n\n";
$md .= "Commands that exist in both but have different argument signatures.\n\n";
foreach ($incompatible as $cmd) {
    $md .= "- `$cmd`\n";
}

$md .= "\n## 3. Excluded from Comparison (" . count($unsupported) . ")\n\n";
$md .= "PHPRedis commands excluded from compatibility comparison (Redis 8+/KeyDB-only, or Valkey-only commands not in PHPRedis).\n\n";
foreach ($unsupported as $cmd) {
    $md .= "- `$cmd`\n";
}

$md .= "\n## 4. Not Implemented (" . count($not_implemented) . ")\n\n";
$md .= "Commands supported by Valkey AND present in PHPRedis, but not yet implemented in Glide.\n\n";
foreach ($not_implemented as $cmd) {
    $md .= "- `$cmd`\n";
}

// ============================================================================
// CLUSTER COMPATIBILITY SECTION
// PHPRedis RedisCluster methods vs ValkeyGlideCluster
// ============================================================================

$phpredis_cluster_commands = [
    // Data commands (same as standalone mostly)
    'append', 'bitcount', 'bitop', 'bitpos', 'blpop', 'brpop', 'brpoplpush',
    'lmove', 'blmove', 'bzpopmax', 'bzpopmin', 'bzmpop', 'zmpop', 'blmpop', 'lmpop',
    'client', 'close', 'config', 'copy', 'dbsize', 'decr', 'decrby', 'del',
    'discard', 'dump', 'echo', 'eval', 'eval_ro', 'evalsha', 'evalsha_ro',
    'exec', 'exists', 'touch', 'expire', 'expireat', 'expiretime', 'pexpiretime',
    'flushall', 'flushdb', 'geoadd', 'geodist', 'geohash', 'geopos',
    'georadius', 'georadius_ro', 'georadiusbymember', 'georadiusbymember_ro',
    'geosearch', 'geosearchstore', 'get', 'getdel', 'getex', 'getbit',
    'getrange', 'lcs', 'getset', 'hdel', 'hexists', 'hget', 'hgetall',
    'hincrby', 'hincrbyfloat', 'hkeys', 'hlen', 'hmget', 'hmset', 'hscan',
    'hrandfield', 'hset', 'hsetnx', 'hstrlen', 'hvals', 'hexpire', 'hpexpire',
    'hexpireat', 'hpexpireat', 'httl', 'hpttl', 'hexpiretime', 'hpexpiretime',
    'hpersist', 'hgetex', 'hsetex', 'hgetdel', 'incr', 'incrby', 'incrbyfloat',
    'info', 'keys', 'lastsave', 'lindex', 'linsert', 'llen', 'lpop', 'lpos',
    'lpush', 'lpushx', 'lrange', 'lrem', 'lset', 'ltrim', 'mget', 'mset',
    'msetnx', 'multi', 'object', 'persist', 'pexpire', 'pexpireat', 'pfadd',
    'pfcount', 'pfmerge', 'ping', 'psetex', 'psubscribe', 'pttl', 'publish',
    'pubsub', 'punsubscribe', 'randomkey', 'rawcommand', 'rename', 'renamenx',
    'restore', 'rpop', 'rpush', 'rpushx', 'rpoplpush', 'sadd', 'saddarray',
    'save', 'scan', 'scard', 'script', 'sdiff', 'sdiffstore', 'set', 'setbit',
    'setex', 'setnx', 'setrange', 'sinter', 'sintercard', 'sinterstore',
    'sismember', 'smismember', 'smembers', 'smove', 'sort', 'sort_ro', 'spop',
    'srandmember', 'srem', 'sscan', 'strlen', 'subscribe', 'sunion',
    'sunionstore', 'time', 'ttl', 'type', 'unsubscribe', 'unlink', 'unwatch',
    'watch', 'wait', 'xack', 'xadd', 'xautoclaim', 'xclaim', 'xdel', 'xgroup',
    'xinfo', 'xlen', 'xpending', 'xrange', 'xread', 'xreadgroup', 'xrevrange',
    'xtrim', 'zadd', 'zcard', 'zcount', 'zincrby', 'zinterstore', 'zintercard',
    'zlexcount', 'zpopmax', 'zpopmin', 'zrange', 'zrangestore', 'zrandmember',
    'zrangebylex', 'zrangebyscore', 'zrank', 'zrem', 'zremrangebylex',
    'zremrangebyrank', 'zremrangebyscore', 'zrevrangebyscore', 'zrevrank',
    'zscan', 'zscore', 'zmscore', 'zunionstore', 'zinter', 'zdiffstore',
    'zunion', 'zdiff', 'fcall', 'fcall_ro', 'function',
    // Cluster-specific
    'cluster', 'command', 'role', 'acl', 'bgsave', 'bgrewriteaof', 'waitaof',
    'setoption', 'getoption',
    // Error introspection (RedisCluster exposes the same getLastError/clearLastError API)
    'getLastError', 'clearLastError',
    // Redis 8+ / KeyDB
    'delex', 'delifeq', 'msetex', 'expiremember', 'expirememberat',
    'getWithMeta', 'hgetWithMeta',
];

$glide_cluster_commands = array_values(array_diff(
    array_map(fn($m) => $m->getName(), (new ReflectionClass('ValkeyGlideCluster'))->getMethods(ReflectionMethod::IS_PUBLIC)),
    ['__construct', '__destruct']
));

$cluster_excluded_from_comparison = [
    'delex', 'delifeq', 'msetex', 'expiremember', 'expirememberat',
    'waitaof', 'getWithMeta', 'hgetWithMeta',
];

$cluster_not_compatible = [
    // PHPRedis: hExpire(key, fields_array, ttl, mode) - fields as array, mode after ttl
    // Glide:    hExpire(key, ttl, mode, field, ...other_fields) - mode before fields, variadic fields
    'hexpire', 'hpexpire', 'hexpireat', 'hpexpireat',
    // PHPRedis: hTtl(key, fields_array) / hPersist(key, fields_array) - fields as array
    // Glide:    hTtl(key, field, ...other_fields) - variadic fields instead of array
    'httl', 'hpttl', 'hexpiretime', 'hpexpiretime', 'hpersist',
    // PHPRedis: hGetEx(key, fields_array, expiry) / hSetEx(key, fields_array, expiry)
    // Glide:    hGetEx(key, fields, options) / hSetEx(key, seconds, mode, field, value, ...)
    'hgetex', 'hsetex',
];

$phpredis_cluster_norm = normalize($phpredis_cluster_commands);
$glide_cluster_norm = normalize($glide_cluster_commands);
$cluster_unsupported_norm = normalize($cluster_excluded_from_comparison);
$cluster_incompat_norm = normalize($cluster_not_compatible);

$cl_compatible = [];
$cl_incompatible = [];
$cl_unsupported = [];
$cl_not_implemented = [];

foreach ($phpredis_cluster_norm as $lower => $original) {
    if (isset($cluster_unsupported_norm[$lower])) {
        $cl_unsupported[] = $original;
    } elseif (isset($cluster_incompat_norm[$lower])) {
        $cl_incompatible[] = $original;
    } elseif (isset($glide_cluster_norm[$lower])) {
        $cl_compatible[] = $original;
    } else {
        $cl_not_implemented[] = $original;
    }
}

$cl_total = count($phpredis_cluster_norm);
sort($cl_compatible);
sort($cl_incompatible);
sort($cl_unsupported);
sort($cl_not_implemented);

echo "\n";
echo "╔══════════════════════════════════════════════════════════════════════╗\n";
echo "║     CLUSTER: RedisCluster vs ValkeyGlideCluster                    ║\n";
echo "╠══════════════════════════════════════════════════════════════════════╣\n";
echo "║                                                                    ║\n";
printf("║  Total RedisCluster commands analyzed: %-28d║\n", $cl_total);
echo "║                                                                    ║\n";
printf("║  1. Compatible APIs:              %3d / %3d  (%5.1f%%)              ║\n",
    count($cl_compatible), $cl_total, count($cl_compatible) / $cl_total * 100);
printf("║  2. Not Compatible APIs:          %3d / %3d  (%5.1f%%)              ║\n",
    count($cl_incompatible), $cl_total, count($cl_incompatible) / $cl_total * 100);
printf("║  3. Excluded from Comparison:     %3d / %3d  (%5.1f%%)              ║\n",
    count($cl_unsupported), $cl_total, count($cl_unsupported) / $cl_total * 100);
printf("║  4. Not Implemented:              %3d / %3d  (%5.1f%%)              ║\n",
    count($cl_not_implemented), $cl_total, count($cl_not_implemented) / $cl_total * 100);
echo "║                                                                    ║\n";
printf("║  Sum: %5.1f%%                                                      ║\n",
    (count($cl_compatible) + count($cl_incompatible) + count($cl_unsupported) + count($cl_not_implemented)) / $cl_total * 100);
echo "║                                                                    ║\n";
echo "╚══════════════════════════════════════════════════════════════════════╝\n";
echo "\n";

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "CLUSTER: NOT IMPLEMENTED (" . count($cl_not_implemented) . ") - Supported by Valkey, not yet in Glide\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
foreach ($cl_not_implemented as $cmd) {
    echo "  - $cmd\n";
}
echo "\n";

// ============================================================================
// Append cluster section to markdown
// ============================================================================

$md .= "\n---\n\n# Cluster: RedisCluster vs ValkeyGlideCluster\n\n";
$md .= "## Summary\n\n";
$md .= "| Category | Count | Percentage |\n";
$md .= "|----------|------:|-----------:|\n";
$md .= sprintf("| Compatible APIs | %d | %.1f%% |\n", count($cl_compatible), count($cl_compatible) / $cl_total * 100);
$md .= sprintf("| Not Compatible APIs | %d | %.1f%% |\n", count($cl_incompatible), count($cl_incompatible) / $cl_total * 100);
$md .= sprintf("| Excluded from Comparison | %d | %.1f%% |\n", count($cl_unsupported), count($cl_unsupported) / $cl_total * 100);
$md .= sprintf("| Not Implemented | %d | %.1f%% |\n", count($cl_not_implemented), count($cl_not_implemented) / $cl_total * 100);
$md .= sprintf("| **Total** | **%d** | **100.0%%** |\n", $cl_total);

$md .= "\n## Compatible APIs (" . count($cl_compatible) . ")\n\n";
$md .= "`" . implode('`, `', $cl_compatible) . "`\n";

$md .= "\n## Not Compatible APIs (" . count($cl_incompatible) . ")\n\n";
foreach ($cl_incompatible as $cmd) {
    $md .= "- `$cmd`\n";
}

$md .= "\n## Excluded from Comparison (" . count($cl_unsupported) . ")\n\n";
foreach ($cl_unsupported as $cmd) {
    $md .= "- `$cmd`\n";
}

$md .= "\n## Not Implemented (" . count($cl_not_implemented) . ")\n\n";
$md .= "Commands supported by Valkey AND present in PHPRedis RedisCluster, but not yet implemented in GlideCluster.\n\n";
foreach ($cl_not_implemented as $cmd) {
    $md .= "- `$cmd`\n";
}

$report_path = __DIR__ . '/COMPATIBILITY_REPORT.md';
file_put_contents($report_path, $md);
echo "Report written to: $report_path\n";
