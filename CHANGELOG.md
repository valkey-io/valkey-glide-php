# Changelog

## 0.10.0

### Changes

* PHP: Implement TLS support for PHP client ([#2983](https://github.com/valkey-io/valkey-glide/pull/2983))
* PHP: Add refresh topology configuration
* PHP: Add Multi-Database Support for Cluster Mode Valkey 9.0 - Added `database_id` parameter to `ValkeyGlideCluster` constructor and support for SELECT, COPY, and MOVE commands in cluster mode. The COPY command can now specify a `database_id` parameter for cross-database operations. This feature requires Valkey 9.0+ with `cluster-databases > 1` configuration.

### Documentation

* PHP: Updated README with multi-database cluster examples and inline documentation for database selection requirements and best practices
