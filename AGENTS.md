# AGENTS: PHP Client Context for Agentic Tools

This file provides AI agents and developers with the minimum but sufficient context to work productively with the Valkey GLIDE PHP client. It covers build commands, testing, contribution requirements, and essential guardrails specific to the PHP implementation.

## Repository Overview

This is the PHP client binding for Valkey GLIDE, providing a sync client implementations. The PHP client is intended to have the same programming interface as PHPRedis, differing only when constructing instances of the client (standalone and cluster). It is implemented as a Zend extension.

**Primary Languages:** PHP, C (Zend Framework)
**Build System:** Standard automake-based system for Zend extensions. phpize, configure, make, make install.
**Architecture:** PHP API stubs implemented using the Zend Framework in C. Implementations delegate communication
to the GLIDE Core written in Rust using C FFI headers.

**Key Components:**
- `./` - PHP API stubs (*.php.stub files),
- `src/` - Mock classes for testing.
- `tests/` - Test suite (primarily integration tests)
- `config.m4` and `Makefile.frag` - Templates for configure and Makefile
- `package.xml` - File manifest for PECL packages
- `composer.json` - Manifest for PIE packages
- `valkey-glide/` - submodule reference to the valkey-glide repo for the GLIDE core.

## Architecture Quick Facts

**Core Implementation:** C wrappers around glide-core Rust library exposed through PHP's Zend framework.
**Client Types:** ValkeyGlide and ValkeyGlideCluster represent standalone and cluster clients respectively.
**API Styles:** Blocking API, matching PHPRedis.
**Communication:** Direct FFI (sync)

**Supported Platforms:**
- Linux: Ubuntu 20+, Amazon Linux 2/2023 (x86_64, aarch64)
- macOS: 13.7+ (x86_64), 14.7+ (aarch64)
- **Note:** Alpine Linux/MUSL not supported

**PHP Versions:** 8.1, 8.2

**Packages:** `valkey-io/valkey-glide-php`

## Connection function implementation flow
* Client constructors are implemented with the by implementing `__construct` PHP method for a class using the
PHP_METHOD macro. See `valkey_glide.c` and `valkey_glide_cluster.c`
* The design of the clusters basically marshal PHP function arguments to C structs defined locally, then those get
written to protobufs messages defined by the FFI layer. Then create_client() is called in the FFI layer with the 
protobuf message.
* The C structures representing the connection configuration are `valkey_glide_client_configuration_t` and
`valkey_glide_cluster_client_configuration_t`. Shared fields are in `valkey_glide_base_client_configuration_t`.
* The method which converts the configuration to protobuf is `create_connection_request()` defined in
`valkey_glide_core_commands.c`.

## Command Implementation Flow
* Define a method in `valkey_glide.stub.php` and/or `valkey_glide_cluster.stub.php` depending on if it should be
available in standalone or cluster.
* Write a helper function to execute the command through the FFI layer. See `execute_get_command` for an example.
These methods should work generically on a `ValkeyGlide` or `ValkeyGlideCluster`. The `valkey_glide_object` pointer represents either.
* Define a macro that invokes the PHP_METHOD macro, taking in a class name. Have that macro call the helper above. See `GET_METHOD_IMPL`.
* Invoke this macro in a .c file. `valkey_z_php_methods.c` for standalone and `valkey_glide_cluster.c` for cluster.

## File naming scheme
* Files are named `valkey_glide_<command_category>_commands.c`, possibly with a numerical suffix.
* Code to be shared across a command category go in `valkey_glide_<command_category>_common.c`.

## Test Framework
Tests are written using a proprietary test framework rather than an existing framework
such as PHPUnit. The base class for test suites is TestSuite and it is in tests/TestSuite.php. This framework doesn't integrate into IDEs such as PhpStorm and doesn't provide a way to run individual test methods or by pattern so switching to a well-known framework may be beneficial.

## Build and Test Rules (Agents)

### Standard autoconf build (used internally by PIE and PECL installers)
```bash
# Build commands
phpize
./configure
make

# Testing
make test

## Contribution Requirements

### Developer Certificate of Origin (DCO) Signoff REQUIRED

All commits must include a `Signed-off-by` line:

```bash
# Add signoff to new commits
git commit -s -m "feat(python): add new command implementation"

# Configure automatic signoff
git config --global format.signOff true

# Add signoff to existing commit
git commit --amend --signoff --no-edit

# Add signoff to multiple commits
git rebase -i HEAD~n --signoff
```

### Conventional Commits

Use conventional commit format:

```
<type>(<scope>): <description>

[optional body]
```

**Example:** `feat(python): implement async cluster scan with routing options`

### Code Quality Requirements

**C Linters:**
Run clang-format on any modified .c and .h files.

**PHP Linters:**
To be added.

## Guardrails & Policies

### Generated Outputs (Never Commit)
See the .gitignore

## Quality Gates (Agent Checklist)

- [ ] Build passes: `phpize, ./configure, make` succeeds
- [ ] All tests pass: `make install` succeeds
- [ ] Linting passes: `clang-format` does not report warnings on changed .c or .h files
- [ ] No generated outputs committed (check `.gitignore`)
- [ ] DCO signoff present: `git log --format="%B" -n 1 | grep "Signed-off-by"`
- [ ] Conventional commit format used
- [ ] Documentation follows Google Style format
- [ ] Shared logic properly isolated in `glide-shared/`

## Quick Facts for Reasoners

**Packages:** `valkey-glide` (async), `valkey-glide-sync` (sync) on PyPI
**API Styles:** Async (asyncio/trio), Sync (blocking)
**Client Types:** GlideClient (standalone), GlideClusterClient (cluster) for both async/sync
**Key Features:** Dual client architecture, shared logic, multi-async framework support
**Testing:** pytest with async backend selection, shared test suite
**Platforms:** Linux (Ubuntu, AL2/AL2023), macOS (Intel/Apple Silicon)
**Dependencies:** Python 3.9+, Rust toolchain, protobuf compiler

## If You Need More

- **Getting Started:** [README.md](./README.md)
- **Development Setup:** [DEVELOPER.md](./DEVELOPER.md)
- **Examples:** [../examples/](../examples/)
- **Test Suites:** [tests/](./tests/) directory
