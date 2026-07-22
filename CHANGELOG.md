# Changelog

## Unreleased

### Changes

* Add `MIGRATE` command for standalone and cluster clients ([#275](https://github.com/valkey-io/valkey-glide-php/pull/275))
* Add `BGREWRITEAOF` command for standalone and cluster clients ([#271](https://github.com/valkey-io/valkey-glide-php/pull/271))
* Add `BGSAVE`, `BGSAVE SCHEDULE`, and `BGSAVE CANCEL` commands for standalone and cluster clients ([#236](https://github.com/valkey-io/valkey-glide-php/issues/236))
* Fix pie install from Packagist using PIE pre-packaged-source download method ([#233](https://github.com/valkey-io/valkey-glide-php/pull/233))
* Fix pie install from Packagist by setting preferred-install to source for submodule resolution ([#232](https://github.com/valkey-io/valkey-glide-php/pull/232))
* Fix pie install ([#229](https://github.com/valkey-io/valkey-glide-php/pull/229))
* Track getLastError/clearLastError as missing PHPRedis methods in the compatibility report instead of excluding them as connection management internals ([#220](https://github.com/valkey-io/valkey-glide-php/pull/220))
* Implement getLastError()/clearLastError() for PHPRedis compatibility ([#221](https://github.com/valkey-io/valkey-glide-php/pull/221))
* Set CLIENT SETINFO lib-name=GlidePHP and lib-ver to extension version for proper client identification
* Add address resolver support for standalone and cluster clients, allowing custom host/port remapping at connection time ([#196](https://github.com/valkey-io/valkey-glide-php/pull/196))

## 1.1.0

### Changes (1.1.0)

* Fix script injection vulnerability in create-version-pr action (CWE-829) ([#206](https://github.com/valkey-io/valkey-glide-php/pull/206))
* Add Sigstore-based artifact attestation for published PECL packages ([#189](https://github.com/valkey-io/valkey-glide-php/pull/189))
* Pin all third-party GitHub Actions to SHA commit hashes to mitigate supply chain attacks (CWE-829) ([#188](https://github.com/valkey-io/valkey-glide-php/pull/188))
* Remove repo-level SECURITY.md to use org-level security policy consistently across all valkey-io repositories ([#187](https://github.com/valkey-io/valkey-glide-php/pull/187))
* Implement client-side caching with TTL-based expiration ([#180](https://github.com/valkey-io/valkey-glide-php/pull/180))
* Fix memory leaks and resource management issues ([#174](https://github.com/valkey-io/valkey-glide-php/pull/174))
* Add transparent compression support for string values ([#186](https://github.com/valkey-io/valkey-glide-php/pull/186))
* Add Modules Testing CI ([#173](https://github.com/valkey-io/valkey-glide-php/pull/173))
* Add FT.* (Vector Search) commands: ftCreate, ftDropIndex, ftList, ftSearch, ftAggregate, ftInfo, ftAliasAdd, ftAliasDel, ftAliasUpdate, ftAliasList for standalone and cluster clients ([#171](https://github.com/valkey-io/valkey-glide-php/pull/171))
* Add JSON commands ([#185](https://github.com/valkey-io/valkey-glide-php/pull/185))
* Add JSON.SET and JSON.GET commands for standalone and cluster clients ([#184](https://github.com/valkey-io/valkey-glide-php/pull/184))
* Refactor IAM authentication tests ([#151](https://github.com/valkey-io/valkey-glide-php/pull/151))
* Add DNS, TLS, and IP address tests ([#157](https://github.com/valkey-io/valkey-glide-php/pull/157))
* Update approved license for aws-lc-sys:0.39.0 ([#176](https://github.com/valkey-io/valkey-glide-php/pull/176))
* Update PECL installation readme ([#160](https://github.com/valkey-io/valkey-glide-php/pull/160))
* Update valkey-glide submodule to latest main ([#154](https://github.com/valkey-io/valkey-glide-php/pull/154))
* Implement transparent compression feature ([#150](https://github.com/valkey-io/valkey-glide-php/pull/150))
* Add OPT_REPLY_LITERAL option for PHPRedis compatibility ([#120](https://github.com/valkey-io/valkey-glide-php/pull/120))
* Add soak tests ([#130](https://github.com/valkey-io/valkey-glide-php/pull/130))

## 1.0.0

### Changes (1.0.0)

* Fix mac development instructions ([#96](https://github.com/valkey-io/valkey-glide-php/pull/96))
* chore: bump valkey-glide submodule for root_certs support ([#99](https://github.com/valkey-io/valkey-glide-php/pull/99))
* ci: Add release automation workflows and version management ([#94](https://github.com/valkey-io/valkey-glide-php/pull/94))
* (dev): Update and improve linting infrastructure ([#104](https://github.com/valkey-io/valkey-glide-php/pull/104))
* (ci): test-modules job queuing indefinitely ([#106](https://github.com/valkey-io/valkey-glide-php/pull/106))
* (feat): Add TLS support for secure connections ([#100](https://github.com/valkey-io/valkey-glide-php/pull/100))
* feat: add markdown linting support for developers ([#110](https://github.com/valkey-io/valkey-glide-php/pull/110))
* perf(php): optimize struct member ordering to reduce padding ([#111](https://github.com/valkey-io/valkey-glide-php/pull/111))
* Add script and function commands ([#97](https://github.com/valkey-io/valkey-glide-php/pull/97))
* Pub/Sub Implementation ([#121](https://github.com/valkey-io/valkey-glide-php/pull/121))
* Remove ValkeyGlideClusterException ([#127](https://github.com/valkey-io/valkey-glide-php/pull/127))
* Add aliases to PHPRedis-compatible class names ([#126](https://github.com/valkey-io/valkey-glide-php/pull/126))
* Add connect method for ValkeyGlide client ([#131](https://github.com/valkey-io/valkey-glide-php/pull/131))
* Add benchmarks ([#124](https://github.com/valkey-io/valkey-glide-php/pull/124))
* Fix validation check for rc builds on PECL Package ([#135](https://github.com/valkey-io/valkey-glide-php/pull/135))

## 0.10.0

### Changes (0.10.0)

* Add OPT_REPLY_LITERAL option for PHPRedis compatibility - allows commands returning OK to return the string "OK" instead of boolean true via setOption()/getOption() API
* Implement TLS support for PHP client ([#2983](https://github.com/valkey-io/valkey-glide/pull/2983))
* Add refresh topology configuration
* Add Multi-Database Support for Cluster Mode Valkey 9.0 - Added `database_id` parameter to `ValkeyGlideCluster` constructor and support for SELECT, COPY, and MOVE commands in cluster mode. The COPY command can now specify a `database_id` parameter for cross-database operations. This feature requires Valkey 9.0+ with `cluster-databases > 1` configuration.

### Documentation

* Updated README with multi-database cluster examples and inline documentation for database selection requirements and best practices
