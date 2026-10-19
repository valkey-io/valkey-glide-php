# AGENTS: PHP Client Context for Agentic Tools

This file provides AI agents and developers with the minimum but sufficient context to work productively with the Valkey GLIDE PHP client. It covers build commands, testing, contribution requirements, and essential guardrails specific to the PHP implementation.

## Repository Overview

This is the PHP client binding for Valkey GLIDE, providing a synchronous (blocking) client. The PHP client is intended to have the same programming interface as PHPRedis, differing only when constructing instances of the client (standalone and cluster). It is implemented as a PHP (Zend) extension.

**Primary Languages:** PHP, C (PHP (Zend) extension API)
**Build System:** Standard autoconf/libtool-based build for PHP extensions. phpize, configure, make, make install.
**Architecture:** PHP API stubs implemented as a C extension against the PHP (Zend) extension API. Implementations delegate communication
to the GLIDE Core written in Rust using C FFI headers.

**Key Components:**

- `./` - PHP API stubs (`*.stub.php` files),
- `src/` - Mock classes for testing, plus generated protobuf C sources (`*.pb-c.c/.h`) and arginfo headers.
- `tests/` - Test suite (primarily integration tests)
- `config.m4` and `Makefile.frag` - Templates for configure and Makefile
- `package.xml` - File manifest for PECL packages
- `composer.json` - Manifest for PIE packages
- `valkey-glide/` - submodule reference to the valkey-glide repo for the GLIDE core.

## Architecture Quick Facts

**Core Implementation:** C wrappers around the glide-core Rust library, exposed through the PHP (Zend) extension API.
**Client Types:** ValkeyGlide and ValkeyGlideCluster represent standalone and cluster clients respectively.
**API Style:** Blocking API, matching PHPRedis.
**Communication:** Direct FFI (sync)

**Supported Platforms:**

- Linux: Ubuntu 20+, Amazon Linux 2/2023 (x86_64, aarch64)
- Alpine Linux: 3.19+ (x86_64, aarch64) — uses musl libc
- macOS: 13.7+ (x86_64), 14.7+ (aarch64)

**PHP Versions:** 8.2, 8.3, 8.4, 8.5

**Packages:** `valkey-io/valkey-glide-php` (Packagist, via PIE); `valkey_glide` (PECL tarball from GitHub releases)

## Connection function implementation flow

- Client constructors are implemented with the by implementing `__construct` PHP method for a class using the
PHP_METHOD macro. See `valkey_glide.c` and `valkey_glide_cluster.c`
- The design of the clusters basically marshal PHP function arguments to C structs defined locally, then those get
written to protobufs messages defined by the FFI layer. Then create_client() is called in the FFI layer with the
protobuf message.
- The C structures representing the connection configuration are `valkey_glide_client_configuration_t` and
`valkey_glide_cluster_client_configuration_t`. Shared fields are in `valkey_glide_base_client_configuration_t`.
- The method which converts the configuration to protobuf is `create_connection_request()` defined in
`valkey_glide_core_commands.c`.

## Command Implementation Flow

- Define a method in `valkey_glide.stub.php` and/or `valkey_glide_cluster.stub.php` depending on if it should be
available in standalone or cluster.
- Write a helper function to execute the command through the FFI layer. See `execute_get_command` for an example.
These methods should work generically on a `ValkeyGlide` or `ValkeyGlideCluster`. The `valkey_glide_object` pointer represents either.
- Define a macro that invokes the PHP_METHOD macro, taking in a class name. Have that macro call the helper above. See `GET_METHOD_IMPL`.
- Invoke this macro in a .c file. `valkey_z_php_methods.c` for standalone and `valkey_glide_cluster.c` for cluster.

## File naming scheme

- Files are named `valkey_glide_<command_category>_commands.c`, possibly with a numerical suffix.
- Code to be shared across a command category go in `valkey_glide_<command_category>_common.c`.

## Test Framework

Tests are written using a proprietary test framework rather than an existing framework
such as PHPUnit. The base class for test suites is TestSuite and it is in tests/TestSuite.php;
the entry point is `tests/TestValkeyGlide.php`. It doesn't integrate into IDEs such as
PhpStorm, but the runner does accept `--class` and `--test` filters (the latter a
case-insensitive substring match on method names) when invoked directly — `make test`
simply has no pass-through for them. Switching to a well-known framework may still be
beneficial.

## Build and Test Rules (Agents)

### Standard autoconf build (used internally by PIE and PECL installers)

```bash
# Build commands
phpize
./configure
make

# Testing
make test
```

The build needs the **protobuf-c** library (`libprotobuf-c-dev`) and its `protoc-c`
compiler (`protobuf-c-compiler`), plus a Rust toolchain and `cbindgen` for the
glide-core FFI. On Linux `configure` aborts if the protobuf-c library is missing. When
`protoc-c` is on PATH, `configure` generates the `src/*.pb-c.c` sources itself; without
it, `configure` prints "protobuf files will be generated during make", but `make` cannot
generate them either and the build fails on the missing sources. The PECL tarball build
(has `.submodule-commits`) aborts at `configure` instead.

Before the first `make test`, run `composer install` and generate the PHP protobuf
classes the ConnectionRequest suite loads from `tests/Connection_request/`:
`protoc --proto_path=valkey-glide/glide-core/src/protobuf --php_out=tests/ valkey-glide/glide-core/src/protobuf/connection_request.proto`.
Neither is needed for the build itself.

`make test` starts the servers it needs — `tests/start_valkey_with_replicas.sh`
then `tests/create-valkey-cluster.sh` (these launch `valkey-server` directly for the
plaintext servers and use `valkey-glide/utils/cluster_manager.py` only for the
TLS/mTLS/auth variants) — then runs the default class list in `tests/TestValkeyGlide.php`.
It does **not** stop the servers afterward; run `tests/stop-servers.sh` yourself when
done. The default list omits the JSON, Search, ClientSideCache, and TLS suites — run
those with `--class` (the TLS suites also need the `--tls` flag, or TLS stays off and
the tests skip; the JSON and Search suites need a server with that module loaded, which
`make test` does not start — CI runs them against module-enabled containers on
`--port=6389` (standalone) / `--port=9001` (cluster), and the tests skip otherwise). To
filter, invoke the runner directly with the built extension loaded:
`php -n -d extension=./modules/valkey_glide.so tests/TestValkeyGlide.php --class=<ClassName> --test=<substring>`

## Contribution Requirements

### Developer Certificate of Origin (DCO) Signoff REQUIRED

All commits must include a `Signed-off-by` line:

```bash
# Add signoff to new commits
git commit -s -m "feat(php): add new command implementation"

# Configure automatic signoff
git config --global format.signOff true

# Add signoff to existing commit
git commit --amend --signoff --no-edit

# Add signoff to multiple commits
git rebase -i HEAD~n --signoff
```

### Conventional Commits

Use conventional commit format:

```text
<type>(<scope>): <description>

[optional body]
```

**Example:** `feat(php): implement cluster scan with routing options`

### Code Quality Requirements

#### Running Linters

```bash
# All linters
./lint.sh            # Check
./lint.sh --fix      # Fix

# C code only
./lint-c.sh          # Check
./lint-c.sh --fix    # Fix

# PHP code only
./lint-php.sh          # Check
./lint-php.sh --fix    # Fix

# Markdown only
./lint-md.sh           # Check
./lint-md.sh --fix     # Fix
```

## Guardrails & Policies

### Generated Outputs (Never Commit)

See the .gitignore

## Quality Gates (Agent Checklist)

- [ ] Build passes: `phpize, ./configure, make` succeeds
- [ ] All tests pass: `make test` succeeds
- [ ] Linting passes: `./lint.sh` does not report warnings (runs the C, PHP, and Markdown linters)
- [ ] No generated outputs committed (check `.gitignore`)
- [ ] DCO signoff present: `git log --format="%B" -n 1 | grep "Signed-off-by"`
- [ ] Conventional commit format used

## Quick Facts for Reasoners

See **Architecture Quick Facts** above for client types, API style, platforms, PHP versions, and packages. Additional facts:

**Implementation:** C extension (PHP (Zend) extension API) delegating to the glide-core Rust library over C FFI
**Testing:** Proprietary TestSuite framework (`tests/TestSuite.php`, entry point `tests/TestValkeyGlide.php`), run via `make test`
**Dependencies:** PHP dev headers (phpize), a C toolchain, Rust toolchain plus `cbindgen` (`cargo install cbindgen`) for the glide-core FFI build, the protobuf-c library + `protoc-c` compiler (`libprotobuf-c-dev` + `protobuf-c-compiler` on Debian/Ubuntu), `composer` for `./lint.sh` and `make test` (both install/require the `vendor/` toolchain), and `protoc` for `make test` only

## If You Need More

- **Getting Started:** [README.md](./README.md)
- **Development Setup:** [DEVELOPER.md](./DEVELOPER.md)
- **Contributing:** [CONTRIBUTING.md](./CONTRIBUTING.md)
- **Examples:** [examples/](./examples/)
- **Test Suites:** [tests/](./tests/) directory
- **Upstream FFI / core:** [valkey-glide](https://github.com/valkey-io/valkey-glide) (`ffi/`, `glide-core/`)
