/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2023-2025 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#include "valkey_glide_prefix.h"

#include <strings.h>

/* ====================================================================
 * CLIENT REGISTRY
 * ==================================================================== */

/* GLIDE client handle -> valkey_glide_object*. Request-scoped: destroyed in
 * RSHUTDOWN; objects freed after that find it NULL and skip removal. */
ZEND_TLS HashTable* tracked_clients = NULL;

void valkey_glide_prefix_track_client(const void* glide_client, valkey_glide_object* valkey_glide) {
    if (!glide_client || !valkey_glide) {
        return;
    }
    if (!tracked_clients) {
        ALLOC_HASHTABLE(tracked_clients);
        zend_hash_init(tracked_clients, 8, NULL, NULL, 0);
    }
    zend_hash_index_update_ptr(tracked_clients, (zend_ulong) (uintptr_t) glide_client, valkey_glide);
}

void valkey_glide_prefix_untrack_client(const void* glide_client) {
    if (tracked_clients && glide_client) {
        zend_hash_index_del(tracked_clients, (zend_ulong) (uintptr_t) glide_client);
    }
}

valkey_glide_object* valkey_glide_prefix_find_object(const void* glide_client) {
    if (!tracked_clients || !glide_client) {
        return NULL;
    }
    return zend_hash_index_find_ptr(tracked_clients, (zend_ulong) (uintptr_t) glide_client);
}

void valkey_glide_prefix_shutdown(void) {
    if (tracked_clients) {
        zend_hash_destroy(tracked_clients);
        FREE_HASHTABLE(tracked_clients);
        tracked_clients = NULL;
    }
}

/* ====================================================================
 * PREFIX VALUE
 * ==================================================================== */

bool valkey_glide_set_prefix(valkey_glide_object* valkey_glide, zval* value) {
    /* Convert first: if __toString() throws, keep the current prefix */
    zend_string* prefix = zval_try_get_string(value);
    if (!prefix) {
        return false;
    }

    if (valkey_glide->prefix) {
        zend_string_release(valkey_glide->prefix);
        valkey_glide->prefix = NULL;
    }

    if (ZSTR_LEN(prefix) > 0) {
        valkey_glide->prefix = prefix;
    } else {
        zend_string_release(prefix);
    }
    return true;
}

zend_string* valkey_glide_prefix_key(valkey_glide_object* valkey_glide,
                                     const char*          key,
                                     size_t               key_len) {
    if (!valkey_glide || !valkey_glide->prefix) {
        return zend_string_init(key, key_len, 0);
    }
    return zend_string_concat2(
        ZSTR_VAL(valkey_glide->prefix), ZSTR_LEN(valkey_glide->prefix), key, key_len);
}

/* ====================================================================
 * KEY POSITIONS
 * ==================================================================== */

#define ARG_IS(i, literal)                                 \
    (args_len[i] == sizeof(literal) - 1 &&                 \
     strncasecmp((const char*) args[i], literal, sizeof(literal) - 1) == 0)

static void mark_range(bool* is_key, unsigned long arg_count, unsigned long from, unsigned long to) {
    for (unsigned long i = from; i < to && i < arg_count; i++) {
        is_key[i] = true;
    }
}

/* Parse a decimal numkeys argument; returns 0 if it is not a valid count. */
static unsigned long parse_numkeys(const uintptr_t*     args,
                                   const unsigned long* args_len,
                                   unsigned long        idx) {
    char   buf[32];
    size_t len = args_len[idx];
    if (len == 0 || len >= sizeof(buf)) {
        return 0;
    }
    memcpy(buf, (const char*) args[idx], len);
    buf[len] = '\0';

    char* end = NULL;
    long  n   = strtol(buf, &end, 10);
    return (end && *end == '\0' && n > 0) ? (unsigned long) n : 0;
}

/* `numkeys` at numkeys_idx, followed by that many keys */
static void mark_numkeys(bool*                is_key,
                         unsigned long        arg_count,
                         const uintptr_t*     args,
                         const unsigned long* args_len,
                         unsigned long        numkeys_idx) {
    if (numkeys_idx >= arg_count) {
        return;
    }
    unsigned long numkeys = parse_numkeys(args, args_len, numkeys_idx);
    mark_range(is_key, arg_count, numkeys_idx + 1, numkeys_idx + 1 + numkeys);
}

/* SORT key [BY pattern] [LIMIT offset count] [GET pattern ...] [ASC|DESC] [ALPHA]
 * [STORE destination]. BY and GET patterns are not prefixed, as in PHPRedis. */
static void mark_sort(bool*                is_key,
                      unsigned long        arg_count,
                      const uintptr_t*     args,
                      const unsigned long* args_len) {
    mark_range(is_key, arg_count, 0, 1);
    for (unsigned long i = 1; i < arg_count; i++) {
        if (ARG_IS(i, "BY") || ARG_IS(i, "GET")) {
            i += 1;
        } else if (ARG_IS(i, "LIMIT")) {
            i += 2;
        } else if (ARG_IS(i, "STORE") && i + 1 < arg_count) {
            is_key[++i] = true;
        }
    }
}

/* XREAD / XREADGROUP [GROUP g c] [COUNT n] [BLOCK ms] [NOACK] STREAMS k1..kN id1..idN */
static void mark_xread(bool*                is_key,
                       unsigned long        arg_count,
                       const uintptr_t*     args,
                       const unsigned long* args_len) {
    for (unsigned long i = 0; i < arg_count; i++) {
        if (ARG_IS(i, "GROUP")) {
            i += 2;
        } else if (ARG_IS(i, "COUNT") || ARG_IS(i, "BLOCK")) {
            i += 1;
        } else if (ARG_IS(i, "STREAMS")) {
            unsigned long remaining = arg_count - i - 1;
            mark_range(is_key, arg_count, i + 1, i + 1 + remaining / 2);
            return;
        }
    }
}

/* MIGRATE host port key|"" db timeout [COPY] [REPLACE] [AUTH pw | AUTH2 user pw] [KEYS k...] */
static void mark_migrate(bool*                is_key,
                         unsigned long        arg_count,
                         const uintptr_t*     args,
                         const unsigned long* args_len) {
    if (arg_count > 2 && args_len[2] > 0) {
        is_key[2] = true;
    }
    for (unsigned long i = 5; i < arg_count; i++) {
        if (ARG_IS(i, "AUTH")) {
            i += 1;
        } else if (ARG_IS(i, "AUTH2")) {
            i += 2;
        } else if (ARG_IS(i, "KEYS")) {
            mark_range(is_key, arg_count, i + 1, arg_count);
            return;
        }
    }
}

/* SCAN cursor [MATCH pattern] [COUNT n] [TYPE t]: only the pattern, and only
 * with OPT_SCAN = SCAN_PREFIX, matching PHPRedis. */
static void mark_scan_match(bool*                is_key,
                            unsigned long        arg_count,
                            const uintptr_t*     args,
                            const unsigned long* args_len) {
    for (unsigned long i = 1; i + 1 < arg_count; i++) {
        if (ARG_IS(i, "MATCH")) {
            is_key[i + 1] = true;
            return;
        }
    }
}

/* Mark which arguments of a command are keys. Arguments exclude the command
 * name (and subcommand for two-word request types). Request types that are
 * not listed carry no keys and are sent unchanged. */
static void mark_keys(valkey_glide_object* valkey_glide,
                      enum RequestType     cmd_type,
                      bool*                is_key,
                      unsigned long        arg_count,
                      const uintptr_t*     args,
                      const unsigned long* args_len) {
    switch (cmd_type) {
        /* key at position 0 */
        case Append:
        case BitCount:
        case BitPos:
        case Decr:
        case DecrBy:
        case Dump:
        case Expire:
        case ExpireAt:
        case ExpireTime:
        case GeoAdd:
        case GeoDist:
        case GeoHash:
        case GeoPos:
        case GeoSearch:
        case Get:
        case GetBit:
        case GetDel:
        case GetEx:
        case GetRange:
        case GetSet:
        case HDel:
        case HExists:
        case HExpire:
        case HExpireAt:
        case HExpireTime:
        case HGet:
        case HGetAll:
        case HGetDel:
        case HGetEx:
        case HIncrBy:
        case HIncrByFloat:
        case HKeys:
        case HLen:
        case HMGet:
        case HMSet:
        case HPExpire:
        case HPExpireAt:
        case HPExpireTime:
        case HPTtl:
        case HPersist:
        case HRandField:
        case HScan:
        case HSet:
        case HSetEx:
        case HSetNX:
        case HStrlen:
        case HTtl:
        case HVals:
        case Incr:
        case IncrBy:
        case IncrByFloat:
        case JsonArrAppend:
        case JsonArrIndex:
        case JsonArrInsert:
        case JsonArrLen:
        case JsonArrPop:
        case JsonArrTrim:
        case JsonClear:
        case JsonDel:
        case JsonForget:
        case JsonGet:
        case JsonNumIncrBy:
        case JsonNumMultBy:
        case JsonObjKeys:
        case JsonObjLen:
        case JsonResp:
        case JsonSet:
        case JsonStrAppend:
        case JsonStrLen:
        case JsonToggle:
        case JsonType:
        case LIndex:
        case LInsert:
        case LLen:
        case LPop:
        case LPos:
        case LPush:
        case LPushX:
        case LRange:
        case LRem:
        case LSet:
        case LTrim:
        case MemoryUsage:
        case Move:
        case ObjectEncoding:
        case ObjectFreq:
        case ObjectIdleTime:
        case ObjectRefCount:
        case PExpire:
        case PExpireAt:
        case PExpireTime:
        case PSetEx:
        case PTTL:
        case Persist:
        case PfAdd:
        case RPop:
        case RPush:
        case RPushX:
        case Restore:
        case SAdd:
        case SCard:
        case SIsMember:
        case SMIsMember:
        case SMembers:
        case SPop:
        case SRandMember:
        case SRem:
        case SScan:
        case Set:
        case SetBit:
        case SetEx:
        case SetNX:
        case SetRange:
        case Strlen:
        case TTL:
        case Type:
        case XAck:
        case XAdd:
        case XAutoClaim:
        case XClaim:
        case XDel:
        case XGroupCreate:
        case XGroupCreateConsumer:
        case XGroupDelConsumer:
        case XGroupDestroy:
        case XGroupSetId:
        case XInfoConsumers:
        case XInfoGroups:
        case XInfoStream:
        case XLen:
        case XPending:
        case XRange:
        case XRevRange:
        case XTrim:
        case ZAdd:
        case ZCard:
        case ZCount:
        case ZIncrBy:
        case ZLexCount:
        case ZMScore:
        case ZPopMax:
        case ZPopMin:
        case ZRandMember:
        case ZRange:
        case ZRangeByLex:
        case ZRangeByScore:
        case ZRank:
        case ZRem:
        case ZRemRangeByLex:
        case ZRemRangeByRank:
        case ZRemRangeByScore:
        case ZRevRange:
        case ZRevRangeByLex:
        case ZRevRangeByScore:
        case ZRevRank:
        case ZScan:
        case ZScore:
            mark_range(is_key, arg_count, 0, 1);
            break;

        /* JSON.DEBUG <subcommand> key [path] */
        case JsonDebug:
            mark_range(is_key, arg_count, 1, 2);
            break;

        /* every argument is a key (KEYS takes a pattern, prefixed as in PHPRedis) */
        case Del:
        case Exists:
        case Keys:
        case MGet:
        case PfCount:
        case PfMerge:
        case RPopLPush:
        case Rename:
        case RenameNX:
        case SDiff:
        case SDiffStore:
        case SInter:
        case SInterStore:
        case SUnion:
        case SUnionStore:
        case Touch:
        case Unlink:
        case Watch:
            mark_range(is_key, arg_count, 0, arg_count);
            break;

        /* source and destination keys first, options after */
        case BLMove:
        case Copy:
        case GeoSearchStore:
        case LCS:
        case LMove:
        case SMove:
        case ZRangeStore:
            mark_range(is_key, arg_count, 0, 2);
            break;

        /* keys followed by a trailing timeout or path */
        case BLPop:
        case BRPop:
        case BRPopLPush:
        case BZPopMax:
        case BZPopMin:
        case JsonMGet:
            if (arg_count > 0) {
                mark_range(is_key, arg_count, 0, arg_count - 1);
            }
            break;

        /* key value [key value ...] */
        case MSet:
        case MSetNX:
            for (unsigned long i = 0; i < arg_count; i += 2) {
                is_key[i] = true;
            }
            break;

        /* BITOP operation destkey key [key ...] */
        case BitOp:
            mark_range(is_key, arg_count, 1, arg_count);
            break;

        /* numkeys key [key ...] ... */
        case LMPop:
        case SInterCard:
        case ZDiff:
        case ZInter:
        case ZInterCard:
        case ZMPop:
        case ZUnion:
            mark_numkeys(is_key, arg_count, args, args_len, 0);
            break;

        /* timeout|function numkeys key [key ...] ... */
        case BLMPop:
        case BZMPop:
        case FCall:
        case FCallReadOnly:
            mark_numkeys(is_key, arg_count, args, args_len, 1);
            break;

        /* destination numkeys key [key ...] ... */
        case ZDiffStore:
        case ZInterStore:
        case ZUnionStore:
            mark_range(is_key, arg_count, 0, 1);
            mark_numkeys(is_key, arg_count, args, args_len, 1);
            break;

        case Sort:
        case SortReadOnly:
            mark_sort(is_key, arg_count, args, args_len);
            break;

        case XRead:
        case XReadGroup:
            mark_xread(is_key, arg_count, args, args_len);
            break;

        case Migrate:
            mark_migrate(is_key, arg_count, args, args_len);
            break;

        case Scan:
            if (valkey_glide->scan_prefix) {
                mark_scan_match(is_key, arg_count, args, args_len);
            }
            break;

        default:
            break;
    }
}

/* ====================================================================
 * ARGUMENT REWRITING
 * ==================================================================== */

static void build_prefixed_args(valkey_glide_object*          valkey_glide,
                                const bool*                   is_key,
                                unsigned long                 arg_count,
                                const uintptr_t*              args,
                                const unsigned long*          args_len,
                                valkey_glide_prefixed_args_t* out) {
    const char* prefix     = ZSTR_VAL(valkey_glide->prefix);
    size_t      prefix_len = ZSTR_LEN(valkey_glide->prefix);

    out->args        = emalloc(arg_count * sizeof(uintptr_t));
    out->args_len    = emalloc(arg_count * sizeof(unsigned long));
    out->owned       = emalloc(arg_count * sizeof(char*));
    out->owned_count = 0;

    for (unsigned long i = 0; i < arg_count; i++) {
        if (!is_key[i]) {
            out->args[i]     = args[i];
            out->args_len[i] = args_len[i];
            continue;
        }

        size_t len = prefix_len + args_len[i];
        char*  buf = emalloc(len + 1);
        memcpy(buf, prefix, prefix_len);
        if (args_len[i] > 0) {
            memcpy(buf + prefix_len, (const char*) args[i], args_len[i]);
        }
        buf[len] = '\0';

        out->owned[out->owned_count++] = buf;
        out->args[i]                   = (uintptr_t) buf;
        out->args_len[i]               = len;
    }
}

bool valkey_glide_prefix_command_args(valkey_glide_object*          valkey_glide,
                                      enum RequestType              cmd_type,
                                      unsigned long                 arg_count,
                                      const uintptr_t*              args,
                                      const unsigned long*          args_len,
                                      valkey_glide_prefixed_args_t* out) {
    memset(out, 0, sizeof(*out));
    if (!valkey_glide || !valkey_glide->prefix || arg_count == 0 || !args || !args_len) {
        return false;
    }

    bool* is_key = ecalloc(arg_count, sizeof(bool));
    mark_keys(valkey_glide, cmd_type, is_key, arg_count, args, args_len);

    bool any = false;
    for (unsigned long i = 0; i < arg_count && !any; i++) {
        any = is_key[i];
    }
    if (any) {
        build_prefixed_args(valkey_glide, is_key, arg_count, args, args_len, out);
    }

    efree(is_key);
    return any;
}

bool valkey_glide_prefix_client_args(const void*                   glide_client,
                                     enum RequestType              cmd_type,
                                     unsigned long                 arg_count,
                                     const uintptr_t*              args,
                                     const unsigned long*          args_len,
                                     valkey_glide_prefixed_args_t* out) {
    return valkey_glide_prefix_command_args(valkey_glide_prefix_find_object(glide_client),
                                            cmd_type,
                                            arg_count,
                                            args,
                                            args_len,
                                            out);
}

bool valkey_glide_prefix_arg_range(valkey_glide_object*          valkey_glide,
                                   unsigned long                 arg_count,
                                   const uintptr_t*              args,
                                   const unsigned long*          args_len,
                                   unsigned long                 first_key,
                                   unsigned long                 key_count,
                                   valkey_glide_prefixed_args_t* out) {
    memset(out, 0, sizeof(*out));
    if (!valkey_glide || !valkey_glide->prefix || key_count == 0 || first_key >= arg_count) {
        return false;
    }

    bool* is_key = ecalloc(arg_count, sizeof(bool));
    mark_range(is_key, arg_count, first_key, first_key + key_count);
    build_prefixed_args(valkey_glide, is_key, arg_count, args, args_len, out);
    efree(is_key);
    return true;
}

void valkey_glide_prefixed_args_free(valkey_glide_prefixed_args_t* out) {
    if (!out || !out->args) {
        return;
    }
    for (unsigned long i = 0; i < out->owned_count; i++) {
        efree(out->owned[i]);
    }
    efree(out->owned);
    efree(out->args);
    efree(out->args_len);
    memset(out, 0, sizeof(*out));
}
