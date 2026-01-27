# Changelog

## 1.0.0

### Changes (1.0.0)

* PHP: Fix mac development instructions ([#96](https://github.com/valkey-io/valkey-glide-php/pull/96))
* PHP: chore: bump valkey-glide submodule for root_certs support ([#99](https://github.com/valkey-io/valkey-glide-php/pull/99))
* PHP: ci: Add release automation workflows and version management ([#94](https://github.com/valkey-io/valkey-glide-php/pull/94))
* PHP: (dev): Update and improve linting infrastructure ([#104](https://github.com/valkey-io/valkey-glide-php/pull/104))
* PHP: (ci): test-modules job queuing indefinitely ([#106](https://github.com/valkey-io/valkey-glide-php/pull/106))
* PHP: (feat): Add TLS support for secure connections ([#100](https://github.com/valkey-io/valkey-glide-php/pull/100))
* PHP: feat: add markdown linting support for developers ([#110](https://github.com/valkey-io/valkey-glide-php/pull/110))
* PHP: perf(php): optimize struct member ordering to reduce padding ([#111](https://github.com/valkey-io/valkey-glide-php/pull/111))
* PHP: Add script and function commands ([#97](https://github.com/valkey-io/valkey-glide-php/pull/97))
* PHP: Pub/Sub Implementation ([#121](https://github.com/valkey-io/valkey-glide-php/pull/121))
* PHP: Remove ValkeyGlideClusterException ([#127](https://github.com/valkey-io/valkey-glide-php/pull/127))
* PHP: Add aliases to PHPRedis-compatible class names ([#126](https://github.com/valkey-io/valkey-glide-php/pull/126))
* PHP: Add connect method for ValkeyGlide client ([#131](https://github.com/valkey-io/valkey-glide-php/pull/131))
* PHP: Add benchmarks ([#124](https://github.com/valkey-io/valkey-glide-php/pull/124))
* PHP: Fix validation check for rc builds on PECL Package ([#135](https://github.com/valkey-io/valkey-glide-php/pull/135))

## 0.10.0

### Changes (0.10.0)

* PHP: Implement TLS support for PHP client ([#2983](https://github.com/valkey-io/valkey-glide/pull/2983))
* PHP: Add refresh topology configuration
* PHP: Add Multi-Database Support for Cluster Mode Valkey 9.0 - Added `database_id` parameter to `ValkeyGlideCluster` constructor and support for SELECT, COPY, and MOVE commands in cluster mode. The COPY command can now specify a `database_id` parameter for cross-database operations. This feature requires Valkey 9.0+ with `cluster-databases > 1` configuration.

### Documentation

* PHP: Updated README with multi-database cluster examples and inline documentation for database selection requirements and best practices
