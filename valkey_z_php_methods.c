/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend.h>
#include <zend_API.h>
#include <zend_exceptions.h>

#include <ext/hash/php_hash.h>
#include <ext/spl/spl_exceptions.h>
#include <ext/standard/info.h>

#include "command_response.h" /* Include command_response.h for string conversion functions */
#include "valkey_glide_commands_common.h"
#include "valkey_glide_geo_common.h"
#include "valkey_glide_hash_common.h" /* Include hash command framework */
#include "valkey_glide_list_common.h"
#include "valkey_glide_s_common.h"
#include "valkey_glide_x_common.h"
#include "valkey_glide_z_common.h"

#if PHP_VERSION_ID < 80400
#include <ext/standard/php_random.h>
#else
#include <ext/random/php_random.h>
#endif

#ifdef PHP_SESSION
#include <ext/session/php_session.h>
#endif

/* Import the string conversion functions from command_response.c */
extern char* long_to_string(long value, size_t* len);
extern char* double_to_string(double value, size_t* len);

/* {{{ proto mixed ValkeyGlide::object(string subcommand, string key) */
OBJECT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zRange(string key, mixed start, mixed end [, bool|array options]) */
ZRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto int ValkeyGlide::zRangeStore(string dest, string src, mixed start, mixed end [, array
 * options]) */
ZRANGESTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto array ValkeyGlide::zRangeByScore(string key, mixed min, mixed max [, array options]) */
ZRANGEBYSCORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zRevRangeByScore(string key, mixed max, mixed min [, array options])
 */
ZREVRANGEBYSCORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zRangeByLex(string key, mixed min, mixed max [, array options | long
 * offset, long count]) */
ZRANGEBYLEX_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto long ValkeyGlide::zLexCount(string key, mixed min, mixed max) */
ZLEXCOUNT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRemRangeByLex(string key, mixed min, mixed max) */
ZREMRANGEBYLEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRem(string key, string member, ...) */
ZREM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRemRangeByScore(string key, mixed min, mixed max) */
ZREMRANGEBYSCORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRemRangeByRank(string key, long start, long end) */
ZREMRANGEBYRANK_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zCount(string key, mixed min, mixed max) */
ZCOUNT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zCard(string key) */
ZCARD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto double ValkeyGlide::zScore(string key, string member) */
ZSCORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zMscore(string key, string member, string member2...)
   proto array ValkeyGlide::zMscore(string key, array members) */
ZMSCORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRank(string key, string member) */
ZRANK_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zRevRank(string key, string member) */
ZREVRANK_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto double ValkeyGlide::zIncrBy(string key, double value, string member) */
ZINCRBY_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zdiff(array keys [, array options]) */
ZDIFF_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zinter(array keys [, array weights] [, array options]) */
ZINTER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::bzPopMax(string|array key [, string otherkeys, ...,], float timeout)
 */
BZPOPMAX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::bzPopMin(string|array key [, string otherkeys, ...,], float timeout)
 */
BZPOPMIN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zintercard(array $keys, int|array $limit_or_options = null) */
ZINTERCARD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zunion(array $keys, array $weights = null, array $options = null) */
ZUNION_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zdiffstore(string dst, array keys) */
ZDIFFSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zinterstore(string dst, array keys) */
ZINTERSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zunionstore(string dst, array keys) */
ZUNIONSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zPopMax(string key, [int count]) */
ZPOPMAX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zPopMin(string key, [int count]) */
ZPOPMIN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zscan(string key, long &iterator, [string pattern, long count]) */
ZSCAN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::zmpop(array $keys, string $from, int $count = 1)
 */
ZMPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::zadd(string key, double score, string member, ...) */
ZADD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::zRandMember(string key [, int|array options [, bool withscores]]) */
ZRANDMEMBER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::bzmpop(double $timeout, array $keys, string $from,
 * int $count = 1) */
BZMPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

// GEO commands

/* {{{ proto long ValkeyGlide::geoadd(string key, float longitude, float latitude, string member,
 * ...) */
GEOADD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto double ValkeyGlide::geodist(string key, string src, string dst [, string unit]) */
GEODIST_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::geohash(string key, string member [, string ...]) */
GEOHASH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::geopos(string key, string member [, string ...]) */
GEOPOS_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto array ValkeyGlide::geosearch(string key, array|string from, array|string by,
 * string|null radius_unit, string|null count, string|null sorting, string|null pattern) */
GEOSEARCH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::geosearchstore(string dst, string src, array|string from,
 * array|string by, string|null radius_unit, string|null count, string|null sorting, string|null
 * storedist) */
GEOSEARCHSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::xack(string key, string group, array ids) */
XACK_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::xadd(string key, string id, array field_values [, int maxlen [,
 * bool approximate]]) */
XADD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xautoclaim(string key, string group, string consumer, int
 * min_idle_time, string start [, array options]) */
XAUTOCLAIM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xclaim(string key, string group, string consumer, int min_idle_time,
 * array ids [, array options]) */
XCLAIM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::xdel(string key, array ids) */
XDEL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::xgroup(string op, [string key, string group, ...]) */
XGROUP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::xinfo(string op, [string key, string group, ...]) */
XINFO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::xlen(string key) */
XLEN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xpending(string key, string group [, array options OR string start,
 * string end, int count [, string consumer]]) */
XPENDING_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xrange(string key, string start, string end [, int count [, array
 * options]]) */
XRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xread(array streams_and_ids [, int count [, int block]]) */
XREAD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xreadgroup(string group, string consumer, array streams [, int count
 * [, array options]]) */
XREADGROUP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::xrevrange(string key, string end, string start [, int count [, array
 * options]]) */
XREVRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::xtrim(string key, string threshold, bool approx = false, bool minid =
 * false, int limit = -1) */
XTRIM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::sAdd(string key, string member, ...) */
SADD_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto long ValkeyGlide::scard(string key) */
SCARD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::srem(string key, string member, ...) */
SREM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::sMove(string src, string dst, string member) */
SMOVE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string|array ValkeyGlide::sPop(string key, [long count]) */
SPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string|array ValkeyGlide::sRandMember(string key, [long count]) */
SRANDMEMBER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::sismember(string key, string member) */
SISMEMBER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sMembers(string key) */
SMEMBERS_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sMisMember(string key, array members) */
SMISMEMBER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sInter(string key, ...) */
SINTER_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::sintercard(array keys, [long limit]) */
SINTERCARD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::sInterStore(string dst, string key1, ...) */
SINTERSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sUnion(string key, ...) */
SUNION_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::sUnionStore(string dst, string key1, ...) */
SUNIONSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sDiff(string key, ...) */
SDIFF_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::sDiffStore(string dst, string key1, ...) */
SDIFFSTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::lPush(string key, mixed value1, mixed value2, mixed valueN) */
LPUSH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::lPushx(string key, mixed value) */
LPUSHX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::rPushx(string key, mixed value) */
RPUSHX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string|array ValkeyGlide::lPop(string key [, int count]) */
LPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string|array ValkeyGlide::rPop(string key [, int count]) */
RPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::blPop(array keys, double timeout) */
BLPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::brPop(array keys, double timeout) */
BRPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto boolean ValkeyGlide::rPush(string key, string value)
 */
RPUSH_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto array ValkeyGlide::lrange(string key, long start, long end) */
LRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto ValkeyGlide|array|false ValkeyGlide::lmpop(array $keys, string $from, int $count = 1)
 */
LMPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::blmpop(double $timeout, array $keys, string $from,
 * int $count = 1) */
BLMPOP_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::lInsert(string key, string position, string pivot, string value) */
LINSERT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::lPos(string key, mixed value, [array options = null]) */
LPOS_METHOD_IMPL(ValkeyGlide)

/* }}} */

/* {{{ proto int ValkeyGlide::lLen(string key) */
LLEN_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto string ValkeyGlide::blmove(string src, string dst, string wherefrom, string whereto,
 * int timeout) */
BLMOVE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::lMove(string src, string dst, string wherefrom, string whereto) */
LMOVE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::lrem(string key, string value [, long count = 0]) */
LREM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::ltrim(string key, long start, long end) */
LTRIM_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::lindex(string key, long index) */
LINDEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::lSet(string key, long index, string value) */
LSET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::hSet(string key, string field, string value) */
HSET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::hSetNx(string key, string field, string value) */
HSETNX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::hGet(string key, string field) */
HGET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::hLen(string key) */
HLEN_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto long ValkeyGlide::hDel(string key, string field1, ... fieldN) */
HDEL_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto bool ValkeyGlide::hExists(string key, string field) */
HEXISTS_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto array ValkeyGlide::hKeys(string key) */
HKEYS_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto array ValkeyGlide::hVals(string key) */
HVALS_METHOD_IMPL(ValkeyGlide);

/* }}} */

/* {{{ proto long ValkeyGlide::hSetEx(string key, long seconds, string field, string value, ...) */
HSETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::hPSetEx(string key, long milliseconds, string field, string value,
 * ...) */
HPSETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::hSetExAt(string key, int timestamp, string field, mixed value, mixed
 * ...$fields_and_vals) */

/* {{{ proto mixed ValkeyGlide::hPSetExAt(string key, int timestamp, string field, mixed value,
 * mixed ...$fields_and_vals) */

/* Hash field expiration NX variants */

/* Hash field expiration XX variants */

/* Hash field expiration NX/XX variants */
/* }}} */


/* {{{ proto array ValkeyGlide::hTtl(string key, string field, ...) */
HTTL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hPersist(string key, string field, ...) */
HPERSIST_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hExpire(string key, long seconds, string field, ...) */
HEXPIRE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hPExpire(string key, long milliseconds, string field, ...) */
HPEXPIRE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hExpireAt(string key, long timestamp, string field, ...) */
HEXPIREAT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hPExpireAt(string key, long timestamp_ms, string field, ...) */
HPEXPIREAT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hExpireTime(string key, string field, ...) */
HEXPIRETIME_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hPExpireTime(string key, string field, ...) */
HPEXPIRETIME_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hPTtl(string key, string field, ...) */
HPTTL_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto mixed ValkeyGlide::hGetEx(string key, string field, long seconds, ...) */
HGETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hGetAll(string key) */
HGETALL_METHOD_IMPL(ValkeyGlide);

/* }}} */

/* {{{ proto double ValkeyGlide::hIncrByFloat(string key, string field, double increment) */
HINCRBYFLOAT_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto long ValkeyGlide::hIncrBy(string key, string field, long increment) */
HINCRBY_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto array ValkeyGlide::hMget(string key, array fields) */
HMGET_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto boolean ValkeyGlide::hMset(string key, array key_values) */
HMSET_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto array|string ValkeyGlide::hRandField(string key [, array options]) */
HRANDFIELD_METHOD_IMPL(ValkeyGlide);

/* }}} */

/* {{{ proto long ValkeyGlide::hStrLen(string key, string field) */
HSTRLEN_METHOD_IMPL(ValkeyGlide);
/* }}} */

/* {{{ proto string ValkeyGlide::echo(string msg) */
ECHO_METHOD_IMPL(ValkeyGlide)
/* }}} */

BITOP_METHOD_IMPL(ValkeyGlide)

/* }}} */

/* {{{ proto long ValkeyGlide::getBit(string key, long offset) */
GETBIT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::setBit(string key, long offset, int value) */
SETBIT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::del(string key, ...) or ValkeyGlide::del(array keys) */
DEL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::bitcount(string key, [int start], [int end])
 */
BITCOUNT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto integer ValkeyGlide::bitpos(string key, int bit, [int start, int end]) */
BITPOS_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::set(string key, mixed val, double|int|array timeout,
 *                              [array opt) */
SET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::setex(string key, long expire, string value)
 */
SETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::psetex(string key, long expire, string value)
 */
PSETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::setnx(string key, string value)
 */
SETNX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::getSet(string key, string value)
 */
GETSET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::get(string key) */
GET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::randomKey()
 */
RANDOMKEY_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::lcs(string $key1, string $key2, ?array $options = NULL); */
LCS_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::setRange(string key, long start, string value) */
SETRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::strlen(string key) */
STRLEN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::info([string section [, string section...]]) */
INFO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::ttl(string key) */
TTL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::pttl(string key) */
PTTL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::ping([string message])
 */
PING_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto boolean ValkeyGlide::select(int dbindex) */
SELECT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::move(string key, int dbindex) */
MOVE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::client(string cmd, ...) */
CLIENT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::rawcommand(string cmd, ...) */
RAWCOMMAND_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::dbSize() */
DBSIZE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto int ValkeyGlide::wait(int numreplicas, int timeout) */
WAIT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::config(string operation, mixed key [, mixed value]) */
CONFIG_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto ValkeyGlide ValkeyGlide::multi() */
MULTI_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::pipeline() */
PIPELINE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::discard() */
DISCARD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::exec() */
EXEC_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::dump(string key) */
DUMP_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::restore(string key, int ttl, string serialized_value [, array
 * options]) */
RESTORE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::expire(string key, long seconds [, string mode]) */
EXPIRE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::expireAt(string key, long timestamp [, string mode]) */
EXPIREAT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::pexpire(string key, long milliseconds [, string mode]) */
PEXPIRE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::pexpireAt(string key, long milliseconds_timestamp [, string mode]) */
PEXPIREAT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::persist(string key) */
PERSIST_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::expiretime(string key) */
EXPIRETIME_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::pexpiretime(string key) */
PEXPIRETIME_METHOD_IMPL(ValkeyGlide)
/* }}} */


/* {{{ proto bool ValkeyGlide::mset(array key_values) */
MSET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::msetnx(array key_values) */
MSETNX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::type(string key) */
TYPE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::append(string key, string value) */
APPEND_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto string ValkeyGlide::getRange(string key, long start, long end) */
GETRANGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sort(string key [, array options]) */
SORT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sort_ro(string key [, array options]) */
SORT_RO_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::watch(string key1, string key2...) */
WATCH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::unwatch() */
UNWATCH_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto double ValkeyGlide::incrByFloat(string key, double value) */
INCRBYFLOAT_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::incrBy(string key, long value) */
INCRBY_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::incr(string key, [long value]) */
INCR_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto string ValkeyGlide::getEx(string key, array opts) */
GETEX_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto string ValkeyGlide::getDel(string key) */
GETDEL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::renameNx(string key_src, string key_dst) */
RENAMENX_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::rename(string key_src, string key_dst) */
RENAME_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::unlink(string key | array keys) */
UNLINK_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::touch(string key | array keys) */
TOUCH_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::exists(string key | array keys) */
EXISTS_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::decrBy(string key, long value) */
DECRBY_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto long ValkeyGlide::decr(string key, [long value]) */
DECR_METHOD_IMPL(ValkeyGlide)
/* }}} */
/* {{{ proto array ValkeyGlide::mget(array keys) */
MGET_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::flushDB([boolean async]) */
FLUSHDB_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto boolean ValkeyGlide::flushAll([boolean async]) */
FLUSHALL_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::time() */
TIME_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::scan(long &iterator [, string pattern, long count]) */
SCAN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::sscan(string key, long &iterator [, string pattern, long count]) */
SSCAN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::copy(string $source, string $destination, array $options = null) */
COPY_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto array ValkeyGlide::hscan(string key, long &iterator, [string pattern, [long count]]) */
HSCAN_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::pfadd(string key, array elements) */
PFADD_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto long ValkeyGlide::pfcount(string key[, string key2, string key3...]) */
PFCOUNT_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::pfmerge(string dst, array keys) */
PFMERGE_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto bool ValkeyGlide::setOption(int option, mixed value) */
SETOPTION_METHOD_IMPL(ValkeyGlide)
/* }}} */

/* {{{ proto mixed ValkeyGlide::getOption(int option) */
GETOPTION_METHOD_IMPL(ValkeyGlide)
/* }}} */
