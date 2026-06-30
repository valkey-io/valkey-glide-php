# Changelog

## Unreleased

### Changes

* PHP: Fix pie install from Packagist using PIE pre-packaged-source download method ([#233](https://github.com/valkey-io/valkey-glide-php/pull/233))
* PHP: Fix pie install ([#229](https://github.com/valkey-io/valkey-glide-php/pull/229))
* PHP: Set CLIENT SETINFO lib-name=GlidePHP and lib-ver to extension version for proper client identification
* PHP: Add address resolver support for standalone and cluster clients, allowing custom host/port remapping at connection time ([#196](https://github.com/valkey-io/valkey-glide-php/pull/196))

## 1.1.0

### Changes (1.1.0)

* PHP: Fix script injection vulnerability in create-version-pr action (CWE-829) ([#206](https://github.com/valkey-io/valkey-glide-php/pull/206))
* PHP: Add Sigstore-based artifact attestation for published PECL packages ([#189](https://github.com/valkey-io/valkey-glide-php/pull/189))
* PHP: Pin all third-party GitHub Actions to SHA commit hashes to mitigate supply chain attacks (CWE-829) ([#188](https://github.com/valkey-io/valkey-glide-php/pull/188))
* PHP: Remove repo-level SECURITY.md to use org-level security policy consistently across all valkey-io repositories ([#187](https://github.com/valkey-io/valkey-glide-php/pull/187))
* PHP: Implement client-side caching with TTL-based expiration ([#180](https://github.com/valkey-io/valkey-glide-php/pull/180))
* PHP: Fix memory leaks and resource management issues ([#174](https://github.com/valkey-io/valkey-glide-php/pull/174))
* PHP: Add transparent compression support for string values ([#186](https://github.com/valkey-io/valkey-glide-php/pull/186))
* PHP: Add Modules Testing CI ([#173](https://github.com/valkey-io/valkey-glide-php/pull/173))
* PHP: Add FT.* (Vector Search) commands: ftCreate, ftDropIndex, ftList, ftSearch, ftAggregate, ftInfo, ftAliasAdd, ftAliasDel, ftAliasUpdate, ftAliasList for standalone and cluster clients ([#171](https://github.com/valkey-io/valkey-glide-php/pull/171))
* PHP: Add JSON commands ([#185](https://github.com/valkey-io/valkey-glide-php/pull/185))
* PHP: Add JSON.SET and JSON.GET commands for standalone and cluster clients ([#184](https://github.com/valkey-io/valkey-glide-php/pull/184))
* PHP: Refactor IAM authentication tests ([#151](https://github.com/valkey-io/valkey-glide-php/pull/151))
* PHP: Add DNS, TLS, and IP address tests ([#157](https://github.com/valkey-io/valkey-glide-php/pull/157))
* PHP: Update approved license for aws-lc-sys:0.39.0 ([#176](https://github.com/valkey-io/valkey-glide-php/pull/176))
* PHP: Update PECL installation readme ([#160](https://github.com/valkey-io/valkey-glide-php/pull/160))
* PHP: Update valkey-glide submodule to latest main ([#154](https://github.com/valkey-io/valkey-glide-php/pull/154))
* PHP: Implement transparent compression feature ([#150](https://github.com/valkey-io/valkey-glide-php/pull/150))
* PHP: Add OPT_REPLY_LITERAL option for PHPRedis compatibility ([#120](https://github.com/valkey-io/valkey-glide-php/pull/120))
* PHP: Add soak tests ([#130](https://github.com/valkey-io/valkey-glide-php/pull/130))

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

* PHP: Add OPT_REPLY_LITERAL option for PHPRedis compatibility - allows commands returning OK to return the string "OK" instead of boolean true via setOption()/getOption() API
* PHP: Implement TLS support for PHP client ([#2983](https://github.com/valkey-io/valkey-glide/pull/2983))
* PHP: Add refresh topology configuration
* PHP: Add Multi-Database Support for Cluster Mode Valkey 9.0 - Added `database_id` parameter to `ValkeyGlideCluster` constructor and support for SELECT, COPY, and MOVE commands in cluster mode. The COPY command can now specify a `database_id` parameter for cross-database operations. This feature requires Valkey 9.0+ with `cluster-databases > 1` configuration.

### Documentation

* PHP: Updated README with multi-database cluster examples and inline documentation for database selection requirements and best practices
