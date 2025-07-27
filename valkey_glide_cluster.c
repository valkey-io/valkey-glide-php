/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if 1
#include <SAPI.h>
#include <php_variables.h>
#include <zend_exceptions.h>

#include <ext/spl/spl_exceptions.h>

#include "common.h"
#include "ext/standard/info.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_geo_common.h"
#include "valkey_glide_hash_common.h" /* Include hash command framework */
#include "valkey_glide_list_common.h"
#include "valkey_glide_s_common.h"
#include "valkey_glide_x_common.h"
#include "valkey_glide_z_common.h"

#if PHP_VERSION_ID < 80000
#include "valkey_glide_cluster_legacy_arginfo.h"
#else
#include "valkey_glide_cluster_arginfo.h"
#include "zend_attributes.h"
#endif

/*f
 * PHP Methods
 */

/* Create a ValkeyGlideCluster Object */
PHP_METHOD(ValkeyGlideCluster, __construct) {
    zval*                addresses                       = NULL;
    zend_bool            use_tls                         = 0;
    zval*                credentials                     = NULL;
    zend_long            read_from                       = 0; /* PRIMARY by default */
    zend_long            request_timeout                 = 0;
    zend_bool            request_timeout_is_null         = 1;
    zval*                reconnect_strategy              = NULL;
    char*                client_name                     = NULL;
    size_t               client_name_len                 = 0;
    zend_long            periodic_checks                 = 0;
    zend_bool            periodic_checks_is_null         = 1;
    zend_long            inflight_requests_limit         = 1000;
    zend_bool            inflight_requests_limit_is_null = 1;
    char*                client_az                       = NULL;
    size_t               client_az_len                   = 0;
    zval*                advanced_config                 = NULL;
    zend_bool            lazy_connect                    = 0;
    zend_bool            lazy_connect_is_null            = 1;
    valkey_glide_object* valkey_glide;

    ZEND_PARSE_PARAMETERS_START(1, 12)
    Z_PARAM_ARRAY(addresses)
    Z_PARAM_OPTIONAL
    Z_PARAM_BOOL(use_tls)
    Z_PARAM_ARRAY_OR_NULL(credentials)
    Z_PARAM_LONG(read_from)
    Z_PARAM_LONG_OR_NULL(request_timeout, request_timeout_is_null)
    Z_PARAM_ARRAY_OR_NULL(reconnect_strategy)
    Z_PARAM_STRING_OR_NULL(client_name, client_name_len)
    Z_PARAM_LONG_OR_NULL(periodic_checks, periodic_checks_is_null)
    Z_PARAM_LONG_OR_NULL(inflight_requests_limit, inflight_requests_limit_is_null)
    Z_PARAM_STRING_OR_NULL(client_az, client_az_len)
    Z_PARAM_ARRAY_OR_NULL(advanced_config)
    Z_PARAM_BOOL_OR_NULL(lazy_connect, lazy_connect_is_null)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, getThis());

    /* Build cluster client configuration from individual parameters */
    valkey_glide_cluster_client_configuration_t client_config;
    memset(&client_config, 0, sizeof(client_config));

    /* Basic configuration */
    client_config.base.use_tls         = use_tls;
    client_config.base.request_timeout = request_timeout_is_null ? -1 : request_timeout;
    client_config.base.inflight_requests_limit =
        inflight_requests_limit_is_null ? -1 : inflight_requests_limit; /* -1 means not set */
    client_config.base.client_name = client_name ? client_name : "valkey-glide-cluster-php";

    /* Set periodic checks */
    client_config.periodic_checks_status =
        periodic_checks_is_null ? VALKEY_GLIDE_PERIODIC_CHECKS_ENABLED_DEFAULT : periodic_checks;
    client_config.base.lazy_connect      = lazy_connect_is_null ? false : lazy_connect;
    client_config.periodic_checks_manual = NULL;

    /* Map read_from enum value to client's ReadFrom enum */
    switch (read_from) {
        case 1: /* PREFER_REPLICA */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_PREFER_REPLICA;
            break;
        case 2: /* AZ_AFFINITY */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY;
            break;
        case 3: /* AZ_AFFINITY_REPLICAS_AND_PRIMARY */
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY;
            break;
        case 0: /* PRIMARY */
        default:
            client_config.base.read_from = VALKEY_GLIDE_READ_FROM_PRIMARY;
            break;
    }

    /* Process addresses array - handle multiple addresses */
    if (addresses && zend_hash_num_elements(Z_ARRVAL_P(addresses)) > 0) {
        HashTable* addresses_ht  = Z_ARRVAL_P(addresses);
        zend_ulong num_addresses = zend_hash_num_elements(addresses_ht);

        /* Allocate addresses array */
        client_config.base.addresses = ecalloc(num_addresses, sizeof(valkey_glide_node_address_t));
        client_config.base.addresses_count = num_addresses;

        /* Process each address */
        zend_ulong i = 0;
        zval*      addr_val;
        ZEND_HASH_FOREACH_VAL(addresses_ht, addr_val) {
            if (Z_TYPE_P(addr_val) == IS_ARRAY) {
                HashTable* addr_ht = Z_ARRVAL_P(addr_val);

                /* Extract host */
                zval* host_val = zend_hash_str_find(addr_ht, "host", 4);
                if (host_val && Z_TYPE_P(host_val) == IS_STRING) {
                    client_config.base.addresses[i].host = Z_STRVAL_P(host_val);
                } else {
                    client_config.base.addresses[i].host = "localhost";
                }

                /* Extract port */
                zval* port_val = zend_hash_str_find(addr_ht, "port", 4);
                if (port_val && Z_TYPE_P(port_val) == IS_LONG) {
                    client_config.base.addresses[i].port = Z_LVAL_P(port_val);
                } else {
                    client_config.base.addresses[i].port = 7001; /* Default cluster port */
                }

                i++;
            }
        }
        ZEND_HASH_FOREACH_END();
    } else {
        /* No addresses provided - set default */
        client_config.base.addresses         = ecalloc(1, sizeof(valkey_glide_node_address_t));
        client_config.base.addresses_count   = 1;
        client_config.base.addresses[0].host = "localhost";
        client_config.base.addresses[0].port = 7001;
    }

    /* Note: This should use a cluster-specific create function */
    /* For now, we'll cast to regular client config */
    const ConnectionResponse* conn_resp =
        create_glide_client((valkey_glide_client_configuration_t*) &client_config, true);

    if (conn_resp->connection_error_message) {
        zend_throw_exception(
            get_valkey_glide_exception_ce(), conn_resp->connection_error_message, 0);
    } else {
        valkey_glide->glide_client = conn_resp->conn_ptr;
    }

    free_connection_response((ConnectionResponse*) conn_resp);

    /* Clean up temporary configuration structures */
    if (client_config.base.addresses) {
        efree(client_config.base.addresses);
    }
}

static zend_function_entry valkey_glide_cluster_methods[] = {
    PHP_ME(ValkeyGlideCluster, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) PHP_FE_END};
/*
 * ValkeyGlideCluster method implementation
 */

/* {{{ proto bool ValkeyGlideCluster::close() */
PHP_METHOD(ValkeyGlideCluster, close) {
    RETURN_TRUE;
}

/* {{{ proto string ValkeyGlideCluster::get(string key) */
GET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::getdel(string key) */
GETDEL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::set(string key, string value) */
SET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* Generic handler for MGET/MSET/MSETNX */

/* {{{ proto array ValkeyGlideCluster::del(string key1, string key2, ... keyN) */
DEL_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::unlink(string key1, string key2, ... keyN) */
UNLINK_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::mget(array keys) */
MGET_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::mset(array keyvalues) */
MSET_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::msetnx(array keyvalues) */
MSETNX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

GETEX_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::setex(string key, string value, int expiry) */
SETEX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::psetex(string key, string value, int expiry) */
PSETEX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::setnx(string key, string value) */
SETNX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::getSet(string key, string value) */
GETSET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto int ValkeyGlideCluster::exists(string $key, string ...$more_keys) */
EXISTS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto int ValkeyGlideCluster::touch(string $key, string ...$more_keys) */
TOUCH_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto int ValkeyGlideCluster::type(string key) */
TYPE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::pop(string key, [int count = 0]) */
LPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

LPOS_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto string ValkeyGlideCluster::rpop(string key, [int count = 0]) */
RPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::lset(string key, long index, string val) */
LSET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::spop(string key) */
SPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string|array ValkeyGlideCluster::srandmember(string key, [long count]) */
SRANDMEMBER_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto string ValkeyGlideCluster::strlen(string key) */
STRLEN_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::lpush(string key, string val1, ... valN) */
LPUSH_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::rpush(string key, string val1, ... valN) */
RPUSH_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::blpop(string key1, ... keyN, long timeout) */
BLPOP_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::brpop(string key1, ... keyN, long timeout */
BRPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::rpushx(string key, mixed value) */
RPUSHX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::lpushx(string key, mixed value) */
LPUSHX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::linsert(string k,string pos,mix pvt,mix val) */
LINSERT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::lindex(string key, long index) */
LINDEX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::lrem(string key, long count, string val) */
LREM_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */


LMOVE_METHOD_IMPL(ValkeyGlideCluster)

BLMOVE_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::llen(string key)  */
LLEN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::scard(string key) */
SCARD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::smembers(string key) */
SMEMBERS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::sismember(string key) */
SISMEMBER_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::smismember(string key, string member0, ...memberN) */
SMISMEMBER_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::sadd(string key, string val1, ...) */
SADD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */


/* {{{ proto long ValkeyGlideCluster::srem(string key, string val1 [, ...]) */
SREM_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::sunion(string key1, ... keyN) */
SUNION_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::sunionstore(string dst, string k1, ... kN) */
SUNIONSTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ ptoto array ValkeyGlideCluster::sinter(string k1, ... kN) */
SINTER_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlideCluster::sintercard(array $keys, int $count = -1) */
SINTERCARD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* }}} */

/* {{{ ptoto long ValkeyGlideCluster::sinterstore(string dst, string k1, ... kN) */
SINTERSTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::sdiff(string k1, ... kN) */
SDIFF_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::sdiffstore(string dst, string k1, ... kN) */
SDIFFSTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::smove(string src, string dst, string mem) */
SMOVE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::persist(string key) */
PERSIST_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::ttl(string key) */
TTL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::pttl(string key) */
PTTL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::zcard(string key) */
ZCARD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto double ValkeyGlideCluster::zscore(string key) */
ZSCORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

ZMSCORE_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::zadd(string key,double score,string mem, ...) */
ZADD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto double ValkeyGlideCluster::zincrby(string key, double by, string mem) */
ZINCRBY_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zremrangebyscore(string k, string s, string e) */
ZREMRANGEBYSCORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zcount(string key, string s, string e) */
ZCOUNT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::zrank(string key, mixed member) */
ZRANK_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::zrevrank(string key, mixed member) */
ZREVRANK_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::hlen(string key) */
HLEN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::hkeys(string key) */
HKEYS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::hvals(string key) */
HVALS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::hget(string key, string mem) */
HGET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::hset(string key, string mem, string val) */
HSET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::hsetnx(string key, string mem, string val) */
HSETNX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::hgetall(string key) */
HGETALL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::hexists(string key, string member) */
HEXISTS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::hincr(string key, string mem, long val) */
HINCRBY_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto double ValkeyGlideCluster::hincrbyfloat(string k, string m, double v) */
HINCRBYFLOAT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::hmset(string key, array key_vals) */
HMSET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::hrandfield(string key, [array $options]) */
HRANDFIELD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::hdel(string key, string mem1, ... memN) */
HDEL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::hmget(string key, array members) */
HMGET_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::hstrlen(string key, string field) */
HSTRLEN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::dump(string key) */
DUMP_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::incr(string key) */
INCR_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::incrby(string key, long byval) */
INCRBY_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::decr(string key) */
DECR_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::decrby(string key, long byval) */
DECRBY_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto double ValkeyGlideCluster::incrbyfloat(string key, double val) */
INCRBYFLOAT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::expire(string key, long sec) */
EXPIRE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::expireat(string key, long ts) */
EXPIREAT_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::pexpire(string key, long ms) */
PEXPIRE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::pexpireat(string key, long ts) */
PEXPIREAT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ ValkeyGlide::expiretime(string $key): int */
EXPIRETIME_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ ValkeyGlide::pexpiretime(string $key): int */
PEXPIRETIME_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::append(string key, string val) */
APPEND_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::getbit(string key, long val) */
GETBIT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::setbit(string key, long offset, bool onoff) */
SETBIT_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto long ValkeyGlideCluster::bitop(string op,string key,[string key2,...]) */
BITOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::bitcount(string key, [int start, int end]) */
BITCOUNT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::bitpos(string key, int bit, [int s, int end]) */
BITPOS_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::getrange(string key, long start, long end) */
GETRANGE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ prot ValkeyGlideCluster::lcs(string $key1, string $key2, ?array $options = NULL): mixed; */
LCS_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::lmpop(array $keys, string $from, int $count = 1)
 */
LMPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::blmpop(double $timeout, array $keys, string $from,
 * int $count = 1) */
BLMPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::zmpop(array $keys, string $from, int $count = 1)
 */
ZMPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlide|array|false ValkeyGlide::bzmpop(double $timeout, array $keys, string $from,
 * int $count = 1) */
BZMPOP_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::ltrim(string key, long start, long end) */
LTRIM_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::lrange(string key, long start, long end) */
LRANGE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::zremrangebyrank(string k, long s, long e) */
ZREMRANGEBYRANK_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::publish(string key, string msg) */
PHP_METHOD(ValkeyGlideCluster, publish) {
}
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::rename(string key1, string key2) */
RENAME_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::renamenx(string key1, string key2) */
RENAMENX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::pfcount(string key) */
PFCOUNT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::pfadd(string key, array vals) */
PFADD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::pfmerge(string key, array keys) */
PFMERGE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto boolean ValkeyGlideCluster::restore(string key, long ttl, string val) */
RESTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::setrange(string key, long offset, string val) */
SETRANGE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto
 *     array ValkeyGlideCluster::zrange(string k, long s, long e, bool score = 0) */
ZRANGE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto
 *     array ValkeyGlideCluster::zrange(string $dstkey, string $srckey, long s, long e, array|bool
 * $options = false) */
ZRANGESTORE_METHOD_IMPL(ValkeyGlideCluster)


/* {{{ proto array
 *     ValkeyGlideCluster::zrangebyscore(string k, long s, long e, array opts) */
ZRANGEBYSCORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zunionstore(string dst, array keys, [array weights,
 *                                     string agg]) */
ZUNIONSTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

ZDIFF_METHOD_IMPL(ValkeyGlideCluster)

ZDIFFSTORE_METHOD_IMPL(ValkeyGlideCluster)

ZINTER_METHOD_IMPL(ValkeyGlideCluster)

ZUNION_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::zrandmember(string key, array options) */
ZRANDMEMBER_METHOD_IMPL(ValkeyGlideCluster)

/* }}} */
/* {{{ proto ValkeyGlideCluster::zinterstore(string dst, array keys, [array weights,
 *                                     string agg]) */
ZINTERSTORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zintercard(array $keys, int $count = -1) */
ZINTERCARD_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zrem(string key, string val1, ... valN) */
ZREM_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array
 *     ValkeyGlideCluster::zrevrangebyscore(string k, long s, long e, array opts) */
ZREVRANGEBYSCORE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::zrangebylex(string key, string min, string max,
 *                                           [offset, count]) */
ZRANGEBYLEX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */


/* {{{ proto long ValkeyGlideCluster::zlexcount(string key, string min, string max) */
ZLEXCOUNT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::zremrangebylex(string key, string min, string max) */
ZREMRANGEBYLEX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::zpopmax(string key) */
ZPOPMAX_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::zpopmin(string key) */
ZPOPMIN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::bzPopMin(Array keys [, timeout]) }}} */
BZPOPMAX_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::bzPopMax(Array keys [, timeout]) }}} */
BZPOPMIN_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlideCluster::sort(string key, array options) */
SORT_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlideCluster::sort_ro(string key, array options) */
SORT_RO_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlideCluster::object(string subcmd, string key) */
OBJECT_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto null ValkeyGlideCluster::subscribe(array chans, callable cb) */
PHP_METHOD(ValkeyGlideCluster, subscribe) {
}
/* }}} */

/* {{{ proto null ValkeyGlideCluster::psubscribe(array pats, callable cb) */
PHP_METHOD(ValkeyGlideCluster, psubscribe) {
}
/* }}} */

/* {{{ proto array ValkeyGlideCluster::unsubscribe(array chans) */
PHP_METHOD(ValkeyGlideCluster, unsubscribe) {
}
/* }}} */

/* {{{ proto array ValkeyGlideCluster::punsubscribe(array pats) */
PHP_METHOD(ValkeyGlideCluster, punsubscribe) {
}
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::eval(string script, [array args, int numkeys) */
PHP_METHOD(ValkeyGlideCluster, eval) {
}
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::eval_ro(string script, [array args, int numkeys) */
PHP_METHOD(ValkeyGlideCluster, eval_ro) {
}
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::evalsha(string sha, [array args, int numkeys]) */
PHP_METHOD(ValkeyGlideCluster, evalsha) {
}
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::evalsha_ro(string sha, [array args, int numkeys]) */
PHP_METHOD(ValkeyGlideCluster, evalsha_ro) {
}

/* }}} */
/* Commands that do not interact with ValkeyGlide, but just report stuff about
 * various options, etc */

/*
 * Transaction handling
 */

/* {{{ proto bool ValkeyGlideCluster::multi() */
MULTI_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::watch() */
WATCH_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::unwatch() */
UNWATCH_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::exec() */
EXEC_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto bool ValkeyGlideCluster::discard() */
DISCARD_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto ValkeyGlideCluster::scan(string master, long it [, string pat, long cnt]) */
SCAN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::sscan(string key, long it [string pat, long cnt]) */
SSCAN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::zscan(string key, long it [string pat, long cnt]) */
ZSCAN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::hscan(string key, long it [string pat, long cnt]) */
HSCAN_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::flushdb(string key, [bool async])
 *     proto ValkeyGlideCluster::flushdb(array host_port, [bool async]) */
FLUSHDB_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::flushall(string key, [bool async])
 *     proto ValkeyGlideCluster::flushall(array host_port, [bool async]) */
FLUSHALL_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto ValkeyGlideCluster::dbsize(string key)
 *     proto ValkeyGlideCluster::dbsize(array host_port) */
DBSIZE_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::info(string key, [string $arg])
 *     proto array ValkeyGlideCluster::info(array host_port, [string $arg]) */
INFO_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto array ValkeyGlideCluster::client('list')
 *     proto bool ValkeyGlideCluster::client('kill', $ipport)
 *     proto bool ValkeyGlideCluster::client('setname', $name)
 *     proto string ValkeyGlideCluster::client('getname')
 */
CLIENT_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::config(string key, ...)
 *     proto mixed ValkeyGlideCluster::config(array host_port, ...) */
CONFIG_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::pubsub(string key, ...)
 *     proto mixed ValkeyGlideCluster::pubsub(array host_port, ...) */
PHP_METHOD(ValkeyGlideCluster, pubsub) {
}
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster::script(string key, ...)
 *     proto mixed ValkeyGlideCluster::script(array host_port, ...) */
PHP_METHOD(ValkeyGlideCluster, script) {
}
/* }}} */

/* {{{ proto array ValkeyGlideCluster::geohash(string key, string mem1, [string mem2...]) */
GEOHASH_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto int ValkeyGlideCluster::geoadd(string key, float long float lat string mem, ...) */
GEOADD_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::geopos(string key, string mem1, [string mem2...]) */
GEOPOS_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::geodist(string key, string mem1, string mem2 [string unit])
 */
GEODIST_METHOD_IMPL(ValkeyGlideCluster)


GEOSEARCH_METHOD_IMPL(ValkeyGlideCluster)

GEOSEARCHSTORE_METHOD_IMPL(ValkeyGlideCluster)


/* {{{ proto array ValkeyGlideCluster::time(string key)
 *     proto array ValkeyGlideCluster::time(array host_port) */
TIME_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto string ValkeyGlideCluster::randomkey(string key)
 *     proto string ValkeyGlideCluster::randomkey(array host_port) */
RANDOMKEY_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto bool ValkeyGlideCluster::ping(string key| string msg)
 *     proto bool ValkeyGlideCluster::ping(array host_port| string msg) */
PING_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto long ValkeyGlideCluster::xack(string key, string group, array ids) }}} */
XACK_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto string ValkeyGlideCluster::xadd(string key, string id, array field_values) }}} */
XADD_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto array ValkeyGlideCluster::xclaim(string key, string group, string consumer,
 *                                      long min_idle_time, array ids, array options) */
XCLAIM_METHOD_IMPL(ValkeyGlideCluster)

XAUTOCLAIM_METHOD_IMPL(ValkeyGlideCluster)

XDEL_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto variant ValkeyGlideCluster::xgroup(string op, [string key, string arg1, string arg2])
 * }}} */
XGROUP_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto variant ValkeyGlideCluster::xinfo(string op, [string arg1, string arg2]); */
XINFO_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto string ValkeyGlideCluster::xlen(string key) }}} */
XLEN_METHOD_IMPL(ValkeyGlideCluster)

XPENDING_METHOD_IMPL(ValkeyGlideCluster)

XRANGE_METHOD_IMPL(ValkeyGlideCluster)

XREVRANGE_METHOD_IMPL(ValkeyGlideCluster)

XREAD_METHOD_IMPL(ValkeyGlideCluster)

XREADGROUP_METHOD_IMPL(ValkeyGlideCluster)

XTRIM_METHOD_IMPL(ValkeyGlideCluster)

/* {{{ proto string ValkeyGlideCluster::echo(string key, string msg)
 *     proto string ValkeyGlideCluster::echo(array host_port, string msg) */
ECHO_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */

/* {{{ proto mixed ValkeyGlideCluster:: command(string $key, string $cmd, [ $argv1 .. $argvN])
 *     proto mixed ValkeyGlideCluster::rawcommand(array $host_port, string $cmd, [ $argv1 ..
 * $argvN]) */
RAWCOMMAND_METHOD_IMPL(ValkeyGlideCluster)
/* }}} */


COPY_METHOD_IMPL(ValkeyGlideCluster)
#endif /* PHP_REDIS_CLUSTER_C */
/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
