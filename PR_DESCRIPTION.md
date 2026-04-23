### Features / Behaviour Changes

This PR adds comprehensive compression tests for the PHP client.

**Supported Commands Tests:**
- MSET / MGET with compression
- MSETNX with compression
- GETEX with compression
- GETDEL with compression
- SETEX, PSETEX, SETNX via rawCommand with compression

**Blocked Commands Tests:**
When compression is enabled, the following commands return `false` (maintaining PHPRedis compatibility):
- String manipulation: APPEND, GETRANGE, SETRANGE, STRLEN
- Numeric operations: INCR, INCRBY, INCRBYFLOAT, DECR, DECRBY
- Bit operations: GETBIT, SETBIT, BITCOUNT

These commands also return `false` when invoked via `rawCommand()`.

### Implementation

**Error Handling (`valkey_glide_core_common.c`):**
- Added check for `command_error` in `execute_core_command()` before processing the response
- When a command error is detected, the function returns `false` to maintain PHPRedis compatibility
- Memory is properly managed before returning

**rawCommand Error Handling (`valkey_glide_commands_3.c`):**
- Updated `execute_rawcommand_command_internal()` to return `false` when `command_error` is present
- Previously, errors were silently ignored

**Submodule Update:**
- Updated `valkey-glide` submodule to latest main which includes the compression blocked commands feature

### Limitations

- Cluster compression tests require a running cluster and were not executed in this PR

### Testing

All compression tests pass:

```
testCompressionBasicZSTD                      [PASSED]
testCompressionBasicLZ4                       [PASSED]
testCompressionStatistics                     [PASSED]
testCompressionMsetMget                       [PASSED]
testCompressionMsetnx                         [PASSED]
testCompressionGetex                          [PASSED]
testCompressionGetdel                         [PASSED]
testCompressionSetexViaRawCommand             [PASSED]
testCompressionPsetexViaRawCommand            [PASSED]
testCompressionSetnxViaRawCommand             [PASSED]
testCompressionBlockedAppend                  [PASSED]
testCompressionBlockedGetrange                [PASSED]
testCompressionBlockedSetrange                [PASSED]
testCompressionBlockedStrlen                  [PASSED]
testCompressionBlockedIncr                    [PASSED]
testCompressionBlockedIncrby                  [PASSED]
testCompressionBlockedIncrbyfloat             [PASSED]
testCompressionBlockedDecr                    [PASSED]
testCompressionBlockedDecrby                  [PASSED]
testCompressionBlockedGetbit                  [PASSED]
testCompressionBlockedSetbit                  [PASSED]
testCompressionBlockedBitcount                [PASSED]
testCompressionBlockedIncrViaRawCommand       [PASSED]
testCompressionBlockedAppendViaRawCommand     [PASSED]
testCompressionBlockedStrlenViaRawCommand     [PASSED]
```

Blocked command tests verify that incompatible commands return `false` when compression is enabled.
