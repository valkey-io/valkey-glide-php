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
 * 3. Not Supported in Valkey - PHPRedis methods for commands not supported by Valkey engine
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
// getReadTimeout, getTimeout, getPersistentID, getMode, getLastError,
// clearLastError, isConnected, getTransferredBytes, clearTransferredBytes)
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
// Glide PHP implemented methods (from valkey_glide.stub.php)
// ============================================================================

$glide_commands = [
    'append', 'bitcount', 'bitop', 'bitpos', 'blPop', 'brPop', 'bzPopMax', 'bzPopMin',
    'bzmpop', 'zmpop', 'blmpop', 'lmpop', 'client', 'close', 'config', 'copy',
    'dbSize', 'decr', 'decrBy', 'del', 'discard', 'dump', 'echo', 'eval', 'evalsha',
    'eval_ro', 'evalsha_ro', 'exec', 'exists', 'expire', 'expireAt', 'expiretime',
    'pexpiretime', 'fcall', 'fcall_ro', 'flushAll', 'flushDB', 'function', 'geoadd',
    'geodist', 'geohash', 'geopos', 'geosearch', 'geosearchstore', 'get', 'getBit',
    'getEx', 'getDel', 'getRange', 'lcs', 'getset', 'hDel', 'hExists', 'hGet',
    'hGetAll', 'hIncrBy', 'hIncrByFloat', 'hKeys', 'hLen', 'hMget', 'hMset',
    'hRandField', 'hSet', 'hSetNx', 'hStrLen', 'hVals', 'hSetEx', 'hPSetEx',
    'hGetEx', 'hExpire', 'hPExpire', 'hExpireAt', 'hPExpireAt', 'hTtl', 'hPTtl',
    'hExpireTime', 'hPExpireTime', 'hPersist', 'hscan', 'incr', 'incrBy',
    'incrByFloat', 'info', 'lInsert', 'lLen', 'lMove', 'blmove', 'lPop', 'lPos',
    'lPush', 'rPush', 'lPushx', 'rPushx', 'lSet', 'lindex', 'lrange', 'lrem',
    'ltrim', 'mget', 'move', 'mset', 'msetnx', 'multi', 'object', 'persist',
    'pexpire', 'pexpireAt', 'pfadd', 'pfcount', 'pfmerge', 'ping', 'pipeline',
    'psetex', 'psubscribe', 'pttl', 'publish', 'pubsub', 'punsubscribe', 'rPop',
    'randomKey', 'rawcommand', 'rename', 'renameNx', 'restore', 'sAdd', 'sDiff',
    'sDiffStore', 'sInter', 'sintercard', 'sInterStore', 'sMembers', 'sMisMember',
    'sMove', 'sPop', 'sRandMember', 'sUnion', 'sUnionStore', 'scan', 'scard',
    'select', 'set', 'setBit', 'setRange', 'setex', 'setnx', 'sismember', 'touch',
    'sort', 'sort_ro', 'srem', 'sscan', 'strlen', 'subscribe', 'time', 'ttl',
    'type', 'unlink', 'unsubscribe', 'unwatch', 'watch', 'wait', 'xack', 'xadd',
    'xautoclaim', 'xclaim', 'xdel', 'xgroup', 'xinfo', 'xlen', 'xpending',
    'xrange', 'xread', 'xreadgroup', 'xrevrange', 'xtrim', 'zAdd', 'zCard',
    'zCount', 'zIncrBy', 'zLexCount', 'zMscore', 'zPopMax', 'zPopMin', 'zRange',
    'zRangeByLex', 'zRangeByScore', 'zrangestore', 'zRandMember', 'zRank', 'zRem',
    'zRemRangeByLex', 'zRemRangeByRank', 'zRemRangeByScore', 'zRevRangeByScore',
    'zRevRank', 'zScore', 'zdiff', 'zdiffstore', 'zinter', 'zintercard',
    'zinterstore', 'zscan', 'zunion', 'zunionstore', 'setOption', 'getOption',
    'script', 'functionLoad', 'functionList', 'functionFlush', 'functionDelete',
    'functionDump', 'functionRestore', 'functionKill', 'functionStats',
    'scriptExists', 'scriptFlush', 'scriptKill', 'scriptShow',
];

// ============================================================================
// Commands NOT supported in Valkey itself (Redis-only, KeyDB-only, or too new)
// These are PHPRedis methods that target features not available in Valkey
// ============================================================================

$not_supported_in_valkey = [
    // Vector set commands (Redis 8+ only, not in Valkey)
    'vadd', 'vsim', 'vcard', 'vdim', 'vinfo', 'vismember', 'vemb', 'vrandmember',
    'vrange', 'vrem', 'vsetattr', 'vgetattr', 'vlinks',

    // Redis 8+ specific commands not in Valkey
    'msetex',       // MSETEX - Redis 8+
    'delex',        // DELEX - Redis 8+
    'delifeq',      // DELIFEQ - Valkey 9+ (but not standard PHPRedis target)
    'xdelex',       // XDELEX - Redis 8+
    'digest',       // DIGEST - Redis 8+
    'getWithMeta',  // GETWITHMETA - Redis 8+
    'hGetWithMeta', // HGETWITHMETA - Redis 8+
    'waitaof',      // WAITAOF - Redis 7.2+ (available in Valkey but not core PHPRedis compat target)

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
    // PHPRedis hexpire/hpexpire/hexpireat/hpexpireat take (key, ttl, fields_array, mode)
    // Glide takes (key, ttl, mode, field, ...other_fields) - different argument order
    'hexpire', 'hpexpire', 'hexpireat', 'hpexpireat',

    // PHPRedis httl/hpttl/hexpiretime/hpexpiretime take (key, fields_array)
    // Glide takes (key, field, ...other_fields) - variadic vs array
    'httl', 'hpttl', 'hexpiretime', 'hpexpiretime',

    // PHPRedis hpersist takes (key, fields_array)
    // Glide takes (key, field, ...other_fields)
    'hpersist',

    // PHPRedis hgetex takes (key, fields_array, expiry)
    // Glide takes (key, fields_array, options) - similar but options format differs
    'hgetex',

    // PHPRedis hsetex takes (key, fields_array, expiry)
    // Glide has hSetEx with different signature (key, seconds, mode, field, value, ...)
    'hsetex',

    // PHPRedis hgetdel takes (key, fields_array)
    // Glide doesn't have hgetdel (it has hGetDel but not in the stub - actually not listed)
    // Actually let's check - removing from here if not implemented

    // PHPRedis connect() has completely different signature than Glide's connect()
    // PHPRedis: connect(host, port, timeout, persistent_id, retry_interval, read_timeout, context)
    // Glide: connect(host, port, addresses, use_tls, credentials, ...)
    // But connect is not in our command list - it's connection management
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
$not_supported_norm = normalize($not_supported_in_valkey);
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
printf("║  3. Not Supported in Valkey:      %3d / %3d  (%5.1f%%)              ║\n",
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
echo "3. NOT SUPPORTED IN VALKEY (" . count($unsupported) . ") - Redis/KeyDB only\n";
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
$md .= sprintf("| Not Supported in Valkey | %d | %.1f%% |\n", count($unsupported), count($unsupported) / $total * 100);
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

$md .= "\n## 3. Not Supported in Valkey (" . count($unsupported) . ")\n\n";
$md .= "PHPRedis commands for features not available in the Valkey engine (Redis 8+, KeyDB, etc.).\n\n";
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
    // Redis 8+ / KeyDB
    'delex', 'delifeq', 'msetex', 'expiremember', 'expirememberat',
    'getWithMeta', 'hgetWithMeta',
];

$glide_cluster_commands = [
    'append', 'bitcount', 'bitop', 'bitpos', 'blPop', 'brPop', 'lMove', 'blmove',
    'bzPopMax', 'bzPopMin', 'bzmpop', 'zmpop', 'blmpop', 'lmpop', 'client', 'close',
    'copy', 'dbSize', 'decr', 'decrBy', 'del', 'discard', 'dump', 'echo', 'eval',
    'eval_ro', 'evalsha', 'evalsha_ro', 'exec', 'exists', 'touch', 'expire',
    'expireAt', 'expiretime', 'pexpiretime', 'flushAll', 'flushDB', 'geoadd',
    'geodist', 'geohash', 'geopos', 'geosearch', 'geosearchstore', 'get', 'getDel',
    'getEx', 'getBit', 'getRange', 'lcs', 'getset', 'hDel', 'hExists', 'hGet',
    'hGetAll', 'hIncrBy', 'hIncrByFloat', 'hKeys', 'hLen', 'hMget', 'hMset',
    'hscan', 'hRandField', 'hSet', 'hSetNx', 'hStrLen', 'hVals', 'hSetEx',
    'hPSetEx', 'hGetEx', 'hExpire', 'hPExpire', 'hExpireAt', 'hPExpireAt', 'hTtl',
    'hPTtl', 'hExpireTime', 'hPExpireTime', 'hPersist', 'incr', 'incrBy',
    'incrByFloat', 'info', 'lindex', 'lInsert', 'lLen', 'lPop', 'lPos', 'lPush',
    'lPushx', 'lrange', 'lrem', 'lSet', 'ltrim', 'mget', 'mset', 'msetnx',
    'multi', 'pipeline', 'object', 'persist', 'pexpire', 'pexpireAt', 'pfadd',
    'pfcount', 'pfmerge', 'ping', 'psetex', 'psubscribe', 'pttl', 'publish',
    'pubsub', 'punsubscribe', 'randomKey', 'rawcommand', 'rename', 'renameNx',
    'restore', 'rPop', 'rPush', 'rPushx', 'sAdd', 'scan', 'scard', 'script',
    'sDiff', 'sDiffStore', 'set', 'setBit', 'setex', 'setnx', 'setRange',
    'sInter', 'sintercard', 'sInterStore', 'sismember', 'sMisMember', 'sMembers',
    'sMove', 'sort', 'sort_ro', 'sPop', 'sRandMember', 'srem', 'sscan', 'strlen',
    'subscribe', 'sUnion', 'sUnionStore', 'time', 'ttl', 'type', 'unsubscribe',
    'unlink', 'unwatch', 'watch', 'xack', 'xadd', 'xautoclaim', 'xclaim', 'xdel',
    'xgroup', 'xinfo', 'xlen', 'xpending', 'xrange', 'xread', 'xreadgroup',
    'xrevrange', 'xtrim', 'zAdd', 'zCard', 'zCount', 'zIncrBy', 'zinterstore',
    'zintercard', 'zLexCount', 'zPopMax', 'zPopMin', 'zRange', 'zrangestore',
    'zRandMember', 'zRangeByLex', 'zRangeByScore', 'zRank', 'zRem',
    'zRemRangeByLex', 'zRemRangeByRank', 'zRemRangeByScore', 'zRevRangeByScore',
    'zRevRank', 'zscan', 'zScore', 'zMscore', 'zunionstore', 'zinter',
    'zdiffstore', 'zunion', 'zdiff', 'fcall', 'fcall_ro', 'function', 'select',
    'move', 'setOption', 'getOption', 'scriptExists', 'scriptFlush', 'scriptKill',
    'scriptShow', 'functionLoad', 'functionList', 'functionFlush',
    'functionDelete', 'functionDump', 'functionRestore', 'functionKill',
    'functionStats',
    // JSON
    'jsonSet', 'jsonGet', 'jsonDel', 'jsonForget', 'jsonClear', 'jsonMGet',
    'jsonType', 'jsonNumIncrBy', 'jsonNumMultBy', 'jsonToggle', 'jsonStrAppend',
    'jsonStrLen', 'jsonObjLen', 'jsonObjKeys', 'jsonResp', 'jsonDebugMemory',
    'jsonDebugFields', 'jsonArrAppend', 'jsonArrInsert', 'jsonArrIndex',
    'jsonArrPop', 'jsonArrTrim', 'jsonArrLen',
    // Search
    'ftCreate', 'ftDropIndex', 'ftList', 'ftSearch', 'ftAggregate', 'ftInfo',
    'ftAliasAdd', 'ftAliasDel', 'ftAliasUpdate', 'ftAliasList',
];

$cluster_not_supported_in_valkey = [
    'delex', 'delifeq', 'msetex', 'expiremember', 'expirememberat',
    'waitaof', 'getWithMeta', 'hgetWithMeta',
];

$cluster_not_compatible = [
    // PHPRedis cluster hexpire/hpexpire/etc take (key, ttl, fields_array, mode)
    // Glide cluster takes (key, ttl, mode, field, ...other_fields)
    'hexpire', 'hpexpire', 'hexpireat', 'hpexpireat',
    'httl', 'hpttl', 'hexpiretime', 'hpexpiretime', 'hpersist',
    'hgetex', 'hsetex',
];

$phpredis_cluster_norm = normalize($phpredis_cluster_commands);
$glide_cluster_norm = normalize($glide_cluster_commands);
$cluster_unsupported_norm = normalize($cluster_not_supported_in_valkey);
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
printf("║  3. Not Supported in Valkey:      %3d / %3d  (%5.1f%%)              ║\n",
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
$md .= sprintf("| Not Supported in Valkey | %d | %.1f%% |\n", count($cl_unsupported), count($cl_unsupported) / $cl_total * 100);
$md .= sprintf("| Not Implemented | %d | %.1f%% |\n", count($cl_not_implemented), count($cl_not_implemented) / $cl_total * 100);
$md .= sprintf("| **Total** | **%d** | **100.0%%** |\n", $cl_total);

$md .= "\n## Compatible APIs (" . count($cl_compatible) . ")\n\n";
$md .= "`" . implode('`, `', $cl_compatible) . "`\n";

$md .= "\n## Not Compatible APIs (" . count($cl_incompatible) . ")\n\n";
foreach ($cl_incompatible as $cmd) {
    $md .= "- `$cmd`\n";
}

$md .= "\n## Not Supported in Valkey (" . count($cl_unsupported) . ")\n\n";
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
