<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

/*
* --------------------------------------------------------------------
*                   The PHP License, version 3.01
* Copyright (c) 1999 - 2010 The PHP Group. All rights reserved.
* --------------------------------------------------------------------
*
* Redistribution and use in source and binary forms, with or without
* modification, is permitted provided that the following conditions
* are met:
*
*   1. Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in
*      the documentation and/or other materials provided with the
*      distribution.
*
*   3. The name "PHP" must not be used to endorse or promote products
*      derived from this software without prior written permission. For
*      written permission, please contact group@php.net.
*
*   4. Products derived from this software may not be called "PHP", nor
*      may "PHP" appear in their name, without prior written permission
*      from group@php.net.  You may indicate that your software works in
*      conjunction with PHP by saying "Foo for PHP" instead of calling
*      it "PHP Foo" or "phpfoo"
*
*   5. The PHP Group may publish revised and/or new versions of the
*      license from time to time. Each version will be given a
*      distinguishing version number.
*      Once covered code has been published under a particular version
*      of the license, you may always continue to use it under the terms
*      of that version. You may also choose to use such covered code
*      under the terms of any subsequent version of the license
*      published by the PHP Group. No one other than the PHP Group has
*      the right to modify the terms applicable to covered code created
*      under this License.
*
*   6. Redistributions of any form whatsoever must retain the following
*      acknowledgment:
*      "This product includes PHP software, freely available from
*      <http://www.php.net/software/>".
*
* THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
* ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
* DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* --------------------------------------------------------------------
*
* This software consists of voluntary contributions made by many
* individuals on behalf of the PHP Group.
*
* The PHP Group can be contacted via Email at group@php.net.
*
* For more information on the PHP Group and the PHP project,
* please see <http://www.php.net>.
*
* PHP includes the Zend Engine, freely available at
* <http://www.zend.com>.
*/

class ValkeyGlideCluster
{
    /**
     * Hash field condition constants
     * @var string
     */
    public const CONDITION_NX = "NX";  // Only if field doesn't exist

    /**
     * @var string
     */
    public const CONDITION_XX = "XX";  // Only if field exists

    /**
     * Time unit constants for hash field expiration
     * @var string
     */
    public const TIME_UNIT_SECONDS = "EX";           // Expire in seconds

    /**
     * @var string
     */
    public const TIME_UNIT_MILLISECONDS = "PX";      // Expire in milliseconds

    /**
     * @var string
     */
    public const TIME_UNIT_TIMESTAMP_SECONDS = "EXAT";   // Expire at timestamp (seconds)

    /**
     * @var string
     */
    public const TIME_UNIT_TIMESTAMP_MILLISECONDS = "PXAT"; // Expire at timestamp (milliseconds)

    /**
     * IAM Authentication Constants
     */

    /**
     * @var string
     * IAM service type for AWS ElastiCache
     */
    public const IAM_SERVICE_ELASTICACHE = 'Elasticache';

    /**
     * @var string
     * IAM service type for AWS MemoryDB
     */
    public const IAM_SERVICE_MEMORYDB = 'MemoryDB';

    /**
     * @var string
     * IAM config key for cluster name
     */
    public const IAM_CONFIG_CLUSTER_NAME = 'clusterName';

    /**
     * @var string
     * IAM config key for AWS region
     */
    public const IAM_CONFIG_REGION = 'region';

    /**
     * @var string
     * IAM config key for service type
     */
    public const IAM_CONFIG_SERVICE = 'service';

    /**
     * @var string
     * IAM config key for token refresh interval in seconds
     */
    public const IAM_CONFIG_REFRESH_INTERVAL = 'refreshIntervalSeconds';

    /**
     * @var string
     * Advanced config key for refresh topology from initial nodes option
     */
    public const ADVANCED_CONFIG_REFRESH_TOPOLOGY_FROM_INITIAL_NODES = 'refresh_topology_from_initial_nodes';

                    /**
                   *  @var int
         * Enables the periodic checks with the default configurations.
         */
    public const   PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS = 0;

        /**
         *   @var int
         * Disables the periodic checks.
         */
    public const    PERIODIC_CHECK_DISABLED = 1;

    /**
     * Create a new ValkeyGlideCluster instance with the provided configuration.
     * Supports both PHPRedis RedisCluster-style and ValkeyGlide-style parameters.
     *
     * PHPRedis RedisCluster-style parameters (positions 0-6):
     * @param string|null $name                 Cluster name (not used, for compatibility only).
     * @param array|null $seeds                 Seed nodes array [['host' => 'x', 'port' => y], ...].
     * @param float|null $timeout               Connection timeout in seconds.
     * @param float|null $read_timeout          Read timeout in seconds.
     * @param bool|null $persistent             Persistent connection (not supported).
     * @param mixed $auth                       Authentication - string (password) or array ['user', 'pass'].
     * @param resource|array|null $context      Stream context resource or array.
     *
     * ValkeyGlide-style parameters (positions 7-18):
     * @param array|null $addresses             Array of server addresses [['host' => '127.0.0.1', 'port' => 7001], ...].
     * @param bool|null $use_tls                Whether to use TLS encryption.
     * @param array|null $credentials           Authentication credentials. Can be either:
     *                                          - Password auth: ['password' => 'xxx', 'username' => 'yyy']
     *                                          - IAM auth: ['username' => 'yyy', 'iamConfig' => [
     *                                              ValkeyGlide::IAM_CONFIG_CLUSTER_NAME => 'my-cluster',
     *                                              ValkeyGlide::IAM_CONFIG_REGION => 'us-east-1',
     *                                              ValkeyGlide::IAM_CONFIG_SERVICE => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
     *                                              ValkeyGlide::IAM_CONFIG_REFRESH_INTERVAL => 300 // optional, defaults to 300
     *                                            ]]
     *                                          Note: username is REQUIRED for IAM authentication.
     * @param int|null $read_from               Read strategy for the client.
     * @param int|null $request_timeout         Request timeout in milliseconds.
     * @param array|null $reconnect_strategy    Reconnection strategy ['num_of_retries' => 3, 'factor' => 2,
     *                                          'exponent_base' => 10, 'jitter_percent' => 15].
     * @param string|null $client_name          Client name identifier.
     * @param int|null $periodic_checks         Periodic checks configuration.
     * @param string|null $client_az            Client availability zone.
     * @param array|null $advanced_config       Advanced configuration options:
     *                                          - 'connection_timeout' => 5000 (milliseconds)
     *                                          - 'tls_config' => ['use_insecure_tls' => false]
     *                                          - 'refresh_topology_from_initial_nodes' => false (default: false)
     *                                            When true, topology updates use only initial nodes instead of internal cluster view.
     *                                          - 'otel' => OpenTelemetryConfig::builder()
     *                                                        ->traces(TracesConfig::builder()
     *                                                          ->endpoint('grpc://localhost:4317')
     *                                                          ->samplePercentage(1)
     *                                                          ->build())
     *                                                        ->metrics(MetricsConfig::builder()
     *                                                          ->endpoint('grpc://localhost:4317')
     *                                                          ->build())
     *                                                        ->flushIntervalMs(5000)
     *                                                        ->build()
     * @param bool|null $lazy_connect           Whether to use lazy connection.
     * @param int|null $database_id             Index of the logical database to connect to. Must be non-negative
     *                                          and within the range supported by the server configuration.
     *                                          For cluster mode, requires Valkey 9.0+ with cluster-databases > 1.
     *                                          If not specified, defaults to database 0.
     *
     * Note: Cannot mix PHPRedis-style and ValkeyGlide-style parameters.
     */
    public function __construct(
        ?string $name = null,
        ?array $seeds = null,
        ?float $timeout = null,
        ?float $read_timeout = null,
        ?bool $persistent = null,
        mixed $auth = null,
        resource|array|null $context = null,
        ?array $addresses = null,
        ?bool $use_tls = null,
        ?array $credentials = null,
        ?int $read_from = null,
        ?int $request_timeout = null,
        ?array $reconnect_strategy = null,
        ?string $client_name = null,
        ?int $periodic_checks = null,
        ?string $client_az = null,
        ?array $advanced_config = null,
        ?bool $lazy_connect = null,
        ?int $database_id = null,
    ) {
    }

    /**
     * @see ValkeyGlide::append()
     */
    public function append(string $key, mixed $value): ValkeyGlideCluster|bool|int;

    /**
     * @see ValkeyGlide::bitcount
     */
    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): ValkeyGlideCluster|bool|int;

    /**
     * @see ValkeyGlide::bitop
     */
    public function bitop(string $operation, string $deskey, string $srckey, string ...$otherkeys): ValkeyGlideCluster|bool|int;

    /**
     * Return the position of the first bit set to 0 or 1 in a string.
     *
     * @see https://https://valkey.io/commands/bitpos/
     *
     * @param string $key   The key to check (must be a string)
     * @param bool   $bit   Whether to look for an unset (0) or set (1) bit.
     * @param int    $start Where in the string to start looking.
     * @param int    $end   Where in the string to stop looking.
     * @param bool   $bybit If true, ValkeyGlide will treat $start and $end as BIT values and not bytes, so if start
     *                      was 0 and end was 2, ValkeyGlide would only search the first two bits.
     */
    public function bitpos(string $key, bool $bit, int $start = 0, int $end = -1, bool $bybit = false): ValkeyGlideCluster|int|false;

    /**
     * See ValkeyGlide::blpop()
     */
    public function blPop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): ValkeyGlideCluster|array|null|false;

    /**
     * See ValkeyGlide::brpop()
     */
    public function brPop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): ValkeyGlideCluster|array|null|false;

    /**
     * Move an element from one list into another.
     *
     * @see ValkeyGlide::lmove
     */
    public function lMove(string $src, string $dst, string $wherefrom, string $whereto): ValkeyGlide|string|false;

    /**
     * Move an element from one list to another, blocking up to a timeout until an element is available.
     *
     * @see ValkeyGlide::blmove
     *
     */
    public function blmove(string $src, string $dst, string $wherefrom, string $whereto, float $timeout): ValkeyGlide|string|false;

    /**
     * @see ValkeyGlide::bzpopmax
     */
    public function bzPopMax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    /**
     * @see ValkeyGlide::bzpopmin
     */
    public function bzPopMin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    /**
     * @see ValkeyGlide::bzmpop
     */
    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): ValkeyGlideCluster|array|null|false;

    /**
     * @see ValkeyGlide::zmpop
     */
    public function zmpop(array $keys, string $from, int $count = 1): ValkeyGlideCluster|array|null|false;

    /**
     * @see ValkeyGlide::blmpop()
     */
    public function blmpop(float $timeout, array $keys, string $from, int $count = 1): ValkeyGlideCluster|array|null|false;

    /**
     * @see ValkeyGlide::lmpop()
     */
    public function lmpop(array $keys, string $from, int $count = 1): ValkeyGlideCluster|array|null|false;

    /**
     * @see ValkeyGlide::client
     */
    public function client(mixed $route, string $subcommand, ?string $arg = null): array|string|bool;

    /**
     * @see ValkeyGlide::close
     */
    public function close(): bool;

    /**
     * @see ValkeyGlide::updateConnectionPassword
     */
    public function updateConnectionPassword(string $password, bool $immediateAuth = false): string;

    /**
     * @see ValkeyGlide::clearConnectionPassword
     */
    public function clearConnectionPassword(bool $immediateAuth = false): string;

    /**
     * @see ValkeyGlide::config()
     */
   //TODO public function config(mixed $route, string $subcommand, mixed ...$extra_args): mixed;

    /**
     * @param mixed $route         The routing configuration that determines which node(s) to send the
     *                             command to. Can be:
     *                             - string "randomNode" to route to a random node
     *                             - string "allPrimaries" to route to all primary nodes
     *                             - string "allNodes" to route to all nodes (primaries and replicas)
     *                             - string containing a key name for slot-based routing
     *                             - array ['type' => 'primarySlotKey', 'key' => 'keyName'] for slot key routing
     *                             - array ['type' => 'routeByAddress', 'host' => 'hostname', 'port' => port]
     *                               for specific node routing
     * @see ValkeyGlide::dbsize()
     */
    public function dbSize(mixed $route): ValkeyGlideCluster|int;

    /**
     * @see ValkeyGlide::select
     */
    public function select(int $db): ValkeyGlideCluster|bool;

    /**
     * Move a key to a different database on the same valkey instance.
     * This command is supported in cluster mode starting with Valkey 9.0.
     *
     * @param string $key The key to move
     * @param int $index The database index to move the key to
     * @return ValkeyGlideCluster|bool True if the key was moved
     */
    public function move(string $key, int $index): ValkeyGlideCluster|bool;

    /**
     * @see https://valkey.io/commands/copy
     */
    public function copy(string $src, string $dst, ?array $options = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::decr()
     */
    public function decr(string $key, int $by = 1): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::decrBy()
     */
    public function decrBy(string $key, int $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::del()
     */
    public function del(array|string $key, string ...$other_keys): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::discard
     */
    public function discard(): bool;

    /**
     * @see ValkeyGlide::dump
     */
    public function dump(string $key): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::echo()
     */
    public function echo(mixed $route, string $msg): ValkeyGlideCluster|string|false;



    /**
     * @see ValkeyGlide::exec()
     */
    public function exec(): array|false;

    /**
     * @see ValkeyGlide::exists
     */
    public function exists(mixed $key, mixed ...$other_keys): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::touch()
     */
    public function touch(mixed $key, mixed ...$other_keys): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::expire
     */
    public function expire(string $key, int $timeout, ?string $mode = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::expireat
     */
    public function expireAt(string $key, int $timestamp, ?string $mode = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::expiretime()
     */
    public function expiretime(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::pexpiretime()
     */
    public function pexpiretime(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::flushall
     */
    public function flushAll(mixed $route, bool $async = false): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::flushdb
     */
    public function flushDB(mixed $route, bool $async = false): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::geoadd
     */
    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples_and_options): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::geodist
     */
    public function geodist(string $key, string $src, string $dest, ?string $unit = null): ValkeyGlideCluster|float|false;

    /**
     * @see ValkeyGlide::geohash
     */
    public function geohash(string $key, string $member, string ...$other_members): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::geopos
     */
    public function geopos(string $key, string $member, string ...$other_members): ValkeyGlideCluster|array|false;

    /**
     * @see https://valkey.io/commands/geosearch
     */
    public function geosearch(string $key, array|string $position, array|int|float $shape, string $unit, array $options = []): ValkeyGlideCluster|array;

    /**
     * @see https://valkey.io/commands/geosearchstore
     */
    public function geosearchstore(string $dst, string $src, array|string $position, array|int|float $shape, string $unit, array $options = []): ValkeyGlideCluster|array|int|false;

    /**
     * @see ValkeyGlide::get
     */
    public function get(string $key): mixed;

    /**
     * @see ValkeyGlide::getDel
     */
    public function getDel(string $key): mixed;

    /**
     * @see ValkeyGlide::getEx
     */
    public function getEx(string $key, array $options = []): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::getbit
     */
    public function getBit(string $key, int $idx): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::getrange
     */
    public function getRange(string $key, int $start, int $end): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::lcs
     */
    public function lcs(string $key1, string $key2, ?array $options = null): ValkeyGlideCluster|string|array|int|false;

    /**
     * @see ValkeyGlide::getset
     */
    public function getset(string $key, mixed $value): ValkeyGlideCluster|string|bool;

    /**
     * @see ValkeyGlide::hdel
     */
    public function hDel(string $key, string $member, string ...$other_members): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hexists
     */
    public function hExists(string $key, string $member): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::hget
     */
    public function hGet(string $key, string $member): mixed;

    /**
     * @see ValkeyGlide::hgetall
     */
    public function hGetAll(string $key): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hincrby
     */
    public function hIncrBy(string $key, string $member, int $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hincrbyfloat
     */
    public function hIncrByFloat(string $key, string $member, float $value): ValkeyGlideCluster|float|false;

    /**
     * @see ValkeyGlide::hkeys
     */
    public function hKeys(string $key): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hlen
     */
    public function hLen(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hmget
     */
    public function hMget(string $key, array $keys): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hmset
     */
    public function hMset(string $key, array $key_values): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::hscan
     */
    public function hscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): array|bool;

      /**
     * @see https://valkey.io/commands/hrandfield
     */
    public function hRandField(string $key, ?array $options = null): ValkeyGlideCluster|string|array;

    /**
     * @see ValkeyGlide::hset
     */
    public function hSet(string $key, string $member, mixed $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hsetnx
     */
    public function hSetNx(string $key, string $member, mixed $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::hstrlen
     */
    public function hStrLen(string $key, string $field): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hvals
     */
    public function hVals(string $key): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hSetEx
     */
    public function hSetEx(string $key, int $seconds, ?string $mode, string $field, mixed $value, mixed ...$fields_and_vals): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hPSetEx
     */
    public function hPSetEx(string $key, int $milliseconds, ?string $mode, string $field, mixed $value, mixed ...$fields_and_vals): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::hGetEx
     */
    public function hGetEx(string $key, array $fields, ?array $options = null): mixed;

    /**
     * @see ValkeyGlide::hExpire
     */
    public function hExpire(string $key, int $seconds, ?string $mode, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hPExpireAt
     */
    public function hPExpireAt(string $key, int $unix_timestamp_ms, ?string $mode, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hPExpire
     */
    public function hPExpire(string $key, int $milliseconds, ?string $mode, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hExpireAt
     */
    public function hExpireAt(string $key, int $unix_timestamp, ?string $mode, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hTtl
     */
    public function hTtl(string $key, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hPTtl
     */
    public function hPTtl(string $key, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hExpireTime
     */
    public function hExpireTime(string $key, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hPExpireTime
     */
    public function hPExpireTime(string $key, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::hPersist
     */
    public function hPersist(string $key, string $field, string ...$other_fields): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::incr
     */
    public function incr(string $key, int $by = 1): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::incrby
     */
    public function incrBy(string $key, int $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::incrbyfloat
     */
    public function incrByFloat(string $key, float $value): ValkeyGlideCluster|float|false;

    /**
     * Retrieve information about the connected valkey-server.  If no arguments are passed to
     * this function, valkey will return every info field.  Alternatively you may pass a specific
     * section you want returned (e.g. 'server', or 'memory') to receive only information pertaining
     * to that section.
     *
     * If connected to ValkeyGlide server >= 7.0.0 you may pass multiple optional sections.
     *
     * @see https://valkey.io/commands/info/
     *
     * @param mixed $route         The routing configuration that determines which node(s) to send the
     *                             command to. Can be:
     *                             - string "randomNode" to route to a random node
     *                             - string "allPrimaries" to route to all primary nodes
     *                             - string "allNodes" to route to all nodes (primaries and replicas)
     *                             - string containing a key name for slot-based routing
     *                             - array ['type' => 'primarySlotKey', 'key' => 'keyName'] for slot key routing
     *                             - array ['type' => 'routeByAddress', 'host' => 'hostname', 'port' => port]
     *                               for specific node routing
     * @param string $sections     Optional section(s) you wish ValkeyGlide server to return.
     *
     * @return ValkeyGlideCluster|array|false
     */
    public function info(mixed $route, string ...$sections): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::lindex
     */
    public function lindex(string $key, int $index): mixed;

    /**
     * @see ValkeyGlide::linsert
     */
    public function lInsert(string $key, string $pos, mixed $pivot, mixed $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::llen
     */
    public function lLen(string $key): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::lpop
     */
    public function lPop(string $key, int $count = 0): ValkeyGlideCluster|bool|string|array;

    /**
     * @see ValkeyGlide::lpos
     */
    public function lPos(string $key, mixed $value, ?array $options = null): ValkeyGlide|null|bool|int|array;

    /**
     * @see ValkeyGlide::lpush
     */
    public function lPush(string $key, mixed $value, mixed ...$other_values): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::lpushx
     */
    public function lPushx(string $key, mixed $value): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::lrange
     */
    public function lrange(string $key, int $start, int $end): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::lrem
     */
    public function lrem(string $key, mixed $value, int $count = 0): ValkeyGlideCluster|int|bool;

    /**
     * @see ValkeyGlide::lset
     */
    public function lSet(string $key, int $index, mixed $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::ltrim
     */
    public function ltrim(string $key, int $start, int $end): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::mget
     */
    public function mget(array $keys): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::mset
     */
    public function mset(array $key_values): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::msetnx
     */
    public function msetnx(array $key_values): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::multi
     */
    public function multi(int $value = ValkeyGlide::MULTI): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::pipeline
     */
    public function pipeline(): bool|ValkeyGlideCluster;

    /**
     * @see ValkeyGlide::object
     */
    public function object(string $subcommand, string $key): ValkeyGlideCluster|int|string|false;

    /**
     * @see ValkeyGlide::persist
     */
    public function persist(string $key): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::pexpire
     */
    public function pexpire(string $key, int $timeout, ?string $mode = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::pexpireat
     */
    public function pexpireAt(string $key, int $timestamp, ?string $mode = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::pfadd()
     */
    public function pfadd(string $key, array $elements): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::pfcount()
     */
    public function pfcount(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::pfmerge()
     */
    public function pfmerge(string $key, array $keys): ValkeyGlideCluster|bool;

    /**
     * PING an instance in the valkey cluster.
     *
     * @see ValkeyGlide::ping()
     *
     * @param mixed $route         The routing configuration that determines which node(s) to send the
     *                             command to. Can be:
     *                             - string "randomNode" to route to a random node
     *                             - string "allPrimaries" to route to all primary nodes
     *                             - string "allNodes" to route to all nodes (primaries and replicas)
     *                             - string containing a key name for slot-based routing
     *                             - array ['type' => 'primarySlotKey', 'key' => 'keyName'] for slot key routing
     *                             - array ['type' => 'routeByAddress', 'host' => 'hostname', 'port' => port]
     *                               for specific node routing
     *
     * @param string       $message        An optional message to send.
     *
     * @return mixed This method always returns `true` if no message was sent, and the message itself
     *               if one was.
     */
    public function ping(mixed $route, ?string $message = null): mixed;

    /**
     * @see ValkeyGlide::psetex
     */
    public function psetex(string $key, int $timeout, string $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::psubscribe
     */
    public function psubscribe(array $patterns, callable $callback): bool;

    /**
     * @see ValkeyGlide::pttl
     */
    public function pttl(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::publish
     */
    public function publish(string $channel, string $message): int;

    /**
     * @see ValkeyGlide::pubsub
     */
    public function pubsub(string $command, mixed $arg = null): mixed;

    /**
     * @see ValkeyGlide::punsubscribe
     */
    public function punsubscribe(?array $patterns = null): bool;

    /**
     * @see ValkeyGlide::randomkey
     */
    public function randomKey(mixed $route): ValkeyGlideCluster|bool|string;

    /**
     * @see ValkeyGlide::rawcommand
     */
    public function rawcommand(mixed $route, string $command, mixed ...$args): mixed;

    /**
     * @see ValkeyGlide::rename
     */
    public function rename(string $key_src, string $key_dst): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::renamenx
     */
    public function renameNx(string $key, string $newkey): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::restore
     */
    public function restore(string $key, int $timeout, string $value, ?array $options = null): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::rpop()
     */
    public function rPop(string $key, int $count = 0): ValkeyGlideCluster|bool|string|array;

    /**
     * @see ValkeyGlide::rpush
     */
    public function rPush(string $key, mixed ...$elements): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::rpushx
     */
    public function rPushx(string $key, string $value): ValkeyGlideCluster|bool|int;

    /**
     * @see ValkeyGlide::sadd()
     */
    public function sAdd(string $key, mixed $value, mixed ...$other_values): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::scan
     */
    public function scan(ClusterScanCursor $iterator, ?string $pattern = null, int $count = 0, ?string $type = null): bool|array;

    /**
     * @see ValkeyGlide::scard
     */
    public function scard(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::script
     */
    public function script(mixed $route, mixed ...$args): mixed;

    /**
     * @see ValkeyGlide::sdiff()
     */
    public function sDiff(string $key, string ...$other_keys): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::sdiffstore()
     */
    public function sDiffStore(string $dst, string $key, string ...$other_keys): ValkeyGlideCluster|int|false;

    /**
     * @see https://valkey.io/commands/set
     */
    public function set(string $key, mixed $value, mixed $options = null): ValkeyGlideCluster|string|bool;

    /**
     * @see ValkeyGlide::setbit
     */
    public function setBit(string $key, int $offset, bool $onoff): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::setex
     */
    public function setex(string $key, int $expire, mixed $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::setnx
     */
    public function setnx(string $key, mixed $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::setrange
     */
    public function setRange(string $key, int $offset, string $value): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::sinter()
     */
    public function sInter(array|string $key, string ...$other_keys): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::sintercard
     */
    public function sintercard(array $keys, int $limit = -1): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::sinterstore()
     */
    public function sInterStore(array|string $key, string ...$other_keys): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::sismember
     */
    public function sismember(string $key, mixed $value): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::smismember
     */
    public function sMisMember(string $key, string $member, string ...$other_members): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::smembers()
     */
    public function sMembers(string $key): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::smove()
     */
    public function sMove(string $src, string $dst, string $member): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::sort()
     */
    public function sort(string $key, ?array $options = null): ValkeyGlideCluster|array|bool|int|string;

    /**
     * @see ValkeyGlide::sort_ro()
     */
    public function sort_ro(string $key, ?array $options = null): ValkeyGlideCluster|array|bool|int|string;

    /**
     * @see ValkeyGlide::spop
     */
    public function sPop(string $key, int $count = 0): ValkeyGlideCluster|string|array|false;

    /**
     * @see ValkeyGlide::srandmember
     */
    public function sRandMember(string $key, int $count = 0): ValkeyGlideCluster|string|array|false;

    /**
     * @see ValkeyGlide::srem
     */
    public function srem(string $key, mixed $value, mixed ...$other_values): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::sscan
     */
    public function sscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): array|false;

    /**
     * @see ValkeyGlide::strlen
     */
    public function strlen(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::subscribe
     */
    public function subscribe(array $channels, callable $cb): bool;

    /**
     * @see ValkeyGlide::sunion()
     */
    public function sUnion(string $key, string ...$other_keys): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::sunionstore()
     */
    public function sUnionStore(string $dst, string $key, string ...$other_keys): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::time
     */
    public function time(mixed $route): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::ttl
     */
    public function ttl(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::type
     */
    public function type(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::unsubscribe
     */
    public function unsubscribe(?array $channels = null): bool;

    /**
     * @see ValkeyGlide::unlink
     */
    public function unlink(array|string $key, string ...$other_keys): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::unwatch
     */
    public function unwatch(): bool;

    /**
     * @see ValkeyGlide::watch
     */
    public function watch(string $key, string ...$other_keys): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::xack
     */
    public function xack(string $key, string $group, array $ids): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::xadd
     */
    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::xclaim
     */
    public function xclaim(string $key, string $group, string $consumer, int $min_iddle, array $ids, array $options): ValkeyGlideCluster|string|array|false;

    /**
     * @see ValkeyGlide::xdel
     */
    public function xdel(string $key, array $ids): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::xgroup
     */
    public function xgroup(
        string $operation,
        ?string $key = null,
        ?string $group = null,
        ?string $id_or_consumer = null,
        bool $mkstream = false,
        int $entries_read = -2
    ): mixed;

    /**
     * @see ValkeyGlide::xautoclaim
     */
    public function xautoclaim(string $key, string $group, string $consumer, int $min_idle, string $start, int $count = -1, bool $justid = false): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::xinfo
     */
    public function xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1): mixed;

    /**
     * @see ValkeyGlide::xlen
     */
    public function xlen(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::xpending
     */
    public function xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::xrange
     */
    public function xrange(string $key, string $start, string $end, int $count = -1): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::xread
     */
    public function xread(array $streams, int $count = -1, int $block = -1): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::xreadgroup
     */
    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::xrevrange
     */
    public function xrevrange(string $key, string $start, string $end, int $count = -1): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::xtrim
     */
    public function xtrim(string $key, int $maxlen, bool $approx = false, bool $minid = false, int $limit = -1): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zadd
     */
    public function zAdd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems): ValkeyGlideCluster|int|float|false;

    /**
     * @see ValkeyGlide::zcard
     */
    public function zCard(string $key): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zcount
     */
    public function zCount(string $key, string $start, string $end): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zincrby
     */
    public function zIncrBy(string $key, float $value, string $member): ValkeyGlideCluster|float|false;

    /**
     * @see ValkeyGlide::zinterstore
     */
    public function zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zintercard
     */
    public function zintercard(array $keys, int $limit = -1): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zlexcount
     */
    public function zLexCount(string $key, string $min, string $max): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zpopmax
     */
    public function zPopMax(string $key, ?int $value = null): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::zpopmin
     */
    public function zPopMin(string $key, ?int $value = null): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::zrange
     */
    public function zRange(string $key, mixed $start, mixed $end, array|bool|null $options = null): ValkeyGlideCluster|array|bool;

    /**
     * @see ValkeyGlide::zrangestore
     */
    public function zrangestore(
        string $dstkey,
        string $srckey,
        int $start,
        int $end,
        array|bool|null $options = null
    ): ValkeyGlideCluster|int|false;

    /**
     * @see https://valkey.io/commands/zrandmember
     */
    public function zRandMember(string $key, ?array $options = null): ValkeyGlideCluster|string|array;

    /**
     * @see ValkeyGlide::zrangebylex
     */
    public function zRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::zrangebyscore
     */
    public function zRangeByScore(string $key, string $start, string $end, array $options = []): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::zrank
     */
    public function zRank(string $key, mixed $member): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zrem
     */
    public function zRem(string $key, string $value, string ...$other_values): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zRemRangeByLex
     */
    public function zRemRangeByLex(string $key, string $min, string $max): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zremrangebyrank
     */
    public function zRemRangeByRank(string $key, string $min, string $max): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zRemRangeByScore
     */
    public function zRemRangeByScore(string $key, string $min, string $max): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zrevrangebyscore
     */
    public function zRevRangeByScore(string $key, string $min, string $max, ?array $options = null): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::zrevrank
     */
    public function zRevRank(string $key, mixed $member): ValkeyGlideCluster|int|false;

    /**
     * @see ValkeyGlide::zscan
     */
    public function zscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): ValkeyGlideCluster|bool|array;

    /**
     * @see ValkeyGlide::zScore
     */
    public function zScore(string $key, mixed $member): ValkeyGlideCluster|float|false;

    /**
     * @see https://valkey.io/commands/zmscore
     */
    public function zMscore(string $key, mixed $member, mixed ...$other_members): ValkeyGlide|array|false;

    /**
     * @see ValkeyGlide::zunionstore
     */
    public function zunionstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): ValkeyGlideCluster|int|false;

    /**
     * @see https://valkey.io/commands/zinter
     */
    public function zinter(array $keys, ?array $weights = null, ?array $options = null): ValkeyGlideCluster|array|false;

    /**
     * @see https://valkey.io/commands/zdiffstore
     */
    public function zdiffstore(string $dst, array $keys): ValkeyGlideCluster|int|false;

    /**
     * @see https://valkey.io/commands/zunion
     */
    public function zunion(array $keys, ?array $weights = null, ?array $options = null): ValkeyGlideCluster|array|false;

    /**
     * @see https://valkey.io/commands/zdiff
     */
    public function zdiff(array $keys, ?array $options = null): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::eval
     */
    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see ValkeyGlide::evalsha
     */
    public function evalsha(string $sha1, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see ValkeyGlide::eval_ro
     */
    public function eval_ro(string $script, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see ValkeyGlide::evalsha_ro
     */
    public function evalsha_ro(string $sha1, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see ValkeyGlide::fcall
     */
    public function fcall(string $fn, array $keys = [], array $args = []): mixed;

    /**
     * @see ValkeyGlide::fcall_ro
     */
    public function fcall_ro(string $fn, array $keys = [], array $args = []): mixed;

    /**
     * @see ValkeyGlide::scriptExists
     */
    public function scriptExists(array $sha1s): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::scriptFlush
     */
    public function scriptFlush(?string $mode = null): bool;

    /**
     * @see ValkeyGlide::scriptKill
     */
    public function scriptKill(): bool;

    /**
     * @see ValkeyGlide::scriptShow
     */
    public function scriptShow(string $sha1): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::functionLoad
     */
    public function functionLoad(string $code, bool $replace = false): ValkeyGlideCluster|string|false;

    /**
     * List function libraries.
     *
     * @param string|null $library_name Optional pattern for matching library names
     * @see https://valkey.io/commands/function-list
     *
     * @return ValkeyGlideCluster|array|false Array of library information
     */
    public function functionList(?string $library_name = null): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::functionFlush
     */
    public function functionFlush(): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::functionDelete
     */
    public function functionDelete(string $library_name): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::functionDump
     */
    public function functionDump(): ValkeyGlideCluster|string|false;

    /**
     * @see ValkeyGlide::functionRestore
     */
    public function functionRestore(string $payload): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::functionKill
     */
    public function functionKill(): ValkeyGlideCluster|bool;

    /**
     * @see ValkeyGlide::functionStats
     */
    public function functionStats(): ValkeyGlideCluster|array|false;

    /**
     * @see ValkeyGlide::function
     */
    public function function(string $operation, mixed ...$args): mixed;
}
