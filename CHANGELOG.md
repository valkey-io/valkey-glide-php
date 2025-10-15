## Unreleased

#### Features

* PHP: Add comprehensive Pub/Sub support - Added `subscribe()`, `psubscribe()`, `unsubscribe()`, `punsubscribe()`, `ssubscribe()`, `sunsubscribe()`, `publish()`, `spublish()`, and `pubSub()` methods for both standalone and cluster clients. Includes support for message callbacks, pattern subscriptions, shard-level pub/sub, and introspection commands (CHANNELS, NUMSUB, NUMPAT, SHARDCHANNELS, SHARDNUMSUB). Thread-safe callback management with single callback per client limitation matching standard Redis behavior.

#### Changes

* PHP: Add Multi-Database Support for Cluster Mode Valkey 9.0 - Added `database_id` parameter to `ValkeyGlideCluster` constructor and support for SELECT, COPY, and MOVE commands in cluster mode. The COPY command can now specify a `database_id` parameter for cross-database operations. This feature requires Valkey 9.0+ with `cluster-databases > 1` configuration.

#### Documentation

* PHP: Updated README with multi-database cluster examples and inline documentation for database selection requirements and best practices

## 0.9.0
