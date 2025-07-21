# Developer Guide

This document describes how to set up your development environment to build and test the Valkey GLIDE PHP wrapper.

### Development Overview

The Valkey GLIDE PHP wrapper is implemented as a PHP extension written in C that interfaces with the Rust-based Glide core library. The PHP extension communicates with the core using:

1. Using the [protobuf](https://github.com/protocolbuffers/protobuf) protocol for message passing.
2. Using C FFI bindings to interface with the Rust core library compiled as a shared object.

The extension follows standard PHP extension development practices and uses the Zend API to expose Valkey GLIDE functionality to PHP applications.

**Important**: The PHP extension depends on the FFI (Foreign Function Interface) library located in the `valkey-glide/ffi` directory. This FFI library provides the bridge between the PHP extension and the Rust-based `valkey-glide/glide-core`. You must build the FFI library before attempting to build the PHP extension.

### Build from source

#### Prerequisites

Software Dependencies

- PHP development headers (`php-dev` or `php-devel`)
- PHP CLI
- git
- GCC
- make
- autotools (autoconf, automake, libtool)
- pkg-config
- protoc (protobuf compiler) >= v3.20.0
- openssl
- openssl-dev
- rustup
- ziglang and zigbuild (for GNU Linux only)
- valkey (for testing)

**Valkey installation**

See the [Valkey installation guide](https://valkey.io/topics/installation/) to install the Valkey server and CLI.

**Dependencies installation for Ubuntu**

```bash
sudo apt update -y
sudo apt install -y php-dev php-cli git gcc make autotools-dev libtool pkg-config openssl libssl-dev unzip libprotobuf-c-dev libprotobuf-c1
# Install rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
# Check that the Rust compiler is installed
rustc --version
```

Continue with **Install protobuf compiler** and **Install `ziglang` and `zigbuild`** below.

**Dependencies installation for CentOS**

```bash
sudo yum update -y
sudo yum install -y php-devel php-cli git gcc make autoconf automake libtool pkgconfig openssl openssl-devel unzip
# Install rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
# Check that the Rust compiler is installed
rustc --version
```

Continue with **Install protobuf compiler** and **Install `ziglang` and `zigbuild`** below.

**Dependencies installation for MacOS**

```bash
brew update
brew install php git gcc make autoconf automake libtool pkgconfig protobuf@3 openssl
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
# Check that the Rust compiler is installed
rustc --version
```

**Install protobuf compiler**

To install protobuf for MacOS, run:

```bash
brew install protobuf@3
# Verify the Protobuf compiler installation
protoc --version

# If protoc is not found or does not work correctly, update the PATH
echo 'export PATH="/opt/homebrew/opt/protobuf@3/bin:$PATH"' >> /Users/$USER/.bash_profile
source /Users/$USER/.bash_profile
protoc --version
```

For the remaining systems, do the following:

```bash
PB_REL="https://github.com/protocolbuffers/protobuf/releases"
curl -LO $PB_REL/download/v3.20.3/protoc-3.20.3-linux-x86_64.zip
unzip protoc-3.20.3-linux-x86_64.zip -d $HOME/.local
export PATH="$PATH:$HOME/.local/bin"
# Check that the protobuf compiler is installed. A minimum version of 3.20.0 is required.
protoc --version
```

**Install `ziglang` and `zigbuild`**

```bash
pip3 install ziglang
cargo install --locked cargo-zigbuild
```

#### Building and installation steps

Before starting this step, make sure you've installed all software requirements.

1. Clone the repository:

    ```bash
    VERSION=2.0.0 # You can modify this to other released version or set it to "main" to get the unstable branch
    git clone --recurse-submodules --branch ${VERSION} https://github.com/valkey-io/valkey-glide-php.git
    cd valkey-glide-php
    ```

1a. Initialize submodules (if not cloned with --recurse-submodules):

    ```bash
    git submodule update --init --recursive
    ```

2. Build the FFI library (required dependency):

    ```bash
    # Build the FFI library that the PHP extension depends on
    python3 utils/remove_optional_from_proto.py
    cd valkey-glide/ffi
    cargo build --release
    cd ../../
    ```

3. Prepare the build environment:

    ```bash
    # Initialize the extension build system
    phpize
    ```

4. Configure the build:

    ```bash
    # Configure with Valkey Glide support enabled
    ./configure --enable-valkey-glide
    ```

5. Build the extension:

    ```bash
    # Pre-build step to prepare modules
    make generate-bindings generate-proto
    
    # Build the extension
    make
    ```

6. Install the extension:

    ```bash
    # Install to PHP extensions directory
    make install
    ```

7. Enable the extension:

    Add the following line to your `php.ini` file:
    ```ini
    extension=valkey_glide
    ```

8. Verify installation:

    ```bash
    # Check if the extension is loaded
    php -m | grep valkey_glide
    
    # Get extension information
    php --ri valkey_glide
    ```

### Test

We run our tests using PHP's testing framework and custom test suites for extension functionality.

#### Test Categories

The Valkey GLIDE PHP wrapper has several categories of tests:

1. **Unit Tests**: Tests that verify individual PHP extension functions
2. **Integration Tests**: Tests that verify integration with Valkey/Redis servers
3. **Extension Tests**: Tests specific to PHP extension functionality
4. **Memory Tests**: Tests for memory leaks and proper resource cleanup

To run unit tests, use:

```bash
make install
cd tests
php -n -d extension=../modules/valkey_glide.so TestValkeyGlide.php
```

To run tests with Valkey server:

```bash
# Ensure Valkey server is running on localhost:6379
valkey-server &

# Run integration tests
php run-tests.php tests/
```

To run specific tests:

```bash
# Run tests matching a pattern
php run-tests.php tests/*basic*
```

#### Running Tests with Custom Server Configuration

Integration tests accept server configuration parameters:

```bash
# Test against custom standalone server
php run-tests.php -d valkey.host=localhost -d valkey.port=6380 tests/

# Test against cluster
php run-tests.php -d valkey.cluster=true -d valkey.port=7000 tests/
```

#### Test Reports and Results

Test reports are generated in the `tests/` folder. Failed tests will generate `.diff`, `.exp`, `.log`, and `.out` files for debugging.

### Generate protobuf files

During the initial build, C protobuf files were created in `src/`. If modifications are made to the protobuf definition files (.proto files located in `glide-core/src/protobuf`), it becomes necessary to regenerate the C protobuf files. To do so, run:

```bash
make generate-protobuf
```

### Linters

Development on the PHP wrapper involves changes in both C and PHP code. We have comprehensive linting infrastructure to ensure code quality and consistency. All linting checks are automatically run in our GitHub Actions CI pipeline.

#### Language-specific Linters

**C Code:**
- **clang-format**: Code formatting with Google-based style (4-space indentation, 100-char line limit)
- **cppcheck**: Static analysis for potential bugs and code quality issues
- **valgrind**: Memory leak detection during testing

**PHP Code:**
- **PHP_CodeSniffer (phpcs)**: PSR-12 coding standards compliance with 120-char line limit
- **PHPStan**: Static analysis at level 6 for type safety and code quality
- **PHP Code Beautifier (phpcbf)**: Automatic code formatting

#### Tools Installation

**Install all development tools:**
```bash
# Install both build and linting tools
make install-tools
```

**Install build tools only:**
```bash
# Install cbindgen and other build tools
make install-build-tools
```

**Install linting tools only:**
```bash
# Install PHP linting tools via Composer
make install-lint-tools
```

**Manual installation:**

*PHP linting tools via Composer:*
```bash
composer install --dev
# Or install globally
composer global require squizlabs/php_codesniffer phpstan/phpstan
```

*C linting tools:*
```bash
# Ubuntu/Debian
sudo apt-get install clang-format cppcheck

# macOS
brew install clang-format cppcheck
```

*Build tools:*
```bash
# Install cbindgen via Cargo
cargo install cbindgen
```

#### Running the Linters

**Using Make targets (recommended):**
```bash
# Run all linters (C + PHP)
make lint

# Run only C code linting
make lint-c

# Run only PHP linting  
make lint-php

# Fix formatting issues automatically
make lint-fix
```

**Using the convenience script:**
```bash
# Run all linting checks with detailed output
./lint.sh
```

**Using Composer scripts:**
```bash
# PHP CodeSniffer
composer run lint

# PHPStan static analysis
composer run analyze

# Fix PHP formatting
composer run lint:fix

# Run both lint and analyze
composer run check
```

**Manual commands:**
```bash
# Check C code formatting
find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | xargs clang-format --dry-run --Werror

# Fix C code formatting
find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | xargs clang-format -i

# Run C static analysis
cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --error-exitcode=1 .

# Check PHP code standards
phpcs --standard=phpcs.xml

# Fix PHP formatting
phpcbf --standard=phpcs.xml

# Run PHP static analysis
phpstan analyze
```

#### Linting Configuration Files

The project includes comprehensive linting configuration:

- **`phpcs.xml`**: PHP_CodeSniffer configuration with PSR-12 standards
- **`phpstan.neon`**: PHPStan configuration at level 6
- **`.clang-format`**: C code formatting rules (Google-based style)
- **`composer.json`**: Development dependencies and linting scripts
- **`Makefile.frag`**: Make targets for linting (integrated into autogenerated Makefile)

#### IDE Integration

VSCode is configured to automatically use the project's linting tools:

- Real-time PHP and C code linting
- Format on save for both languages
- Integration with project-specific linting configurations
- Extensions recommended: PHP, C/C++, phpcs, phpstan, clangd

### Memory Management and Debugging

#### Memory Leak Detection

Use Valgrind to detect memory leaks:

```bash
# Run PHP with Valgrind
valgrind --tool=memcheck --leak-check=full php test_script.php
```

#### Debugging with GDB

```bash
# Compile with debug symbols
make clean
./configure --enable-valkey-glide --enable-debug
make

# Debug with GDB
gdb php
(gdb) run test_script.php
```

### Extension Development Guidelines

#### PHP Extension Conventions

When adding new functionality to the PHP extension:

1. **Function Naming**: Use `valkey_glide_` prefix for internal functions
2. **Memory Management**: Always use `emalloc`/`efree` for request-scoped memory
3. **Error Handling**: Use PHP's exception system via `zend_throw_exception`
4. **Parameter Parsing**: Use `ZEND_PARSE_PARAMETERS_START` macros
5. **Return Values**: Use appropriate `RETURN_*` macros

#### Example Function Implementation

```c
PHP_METHOD(ValkeyGlide, myCommand)
{
    char *key;
    size_t key_len;
    zend_long value;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(key, key_len)
        Z_PARAM_LONG(value)
    ZEND_PARSE_PARAMETERS_END();
    
    // Implementation here
    
    RETURN_TRUE;
}
```

#### Adding New Commands

1. Define the method in the appropriate `.c` file
2. Add the method signature to the arginfo header
3. Register the method in the class methods table
4. Add protobuf message handling if needed
5. Write comprehensive tests

### Documentation

#### Function Documentation

Document all PHP methods using the following format:

```c
/* {{{ proto mixed ValkeyGlide::myCommand(string key, int value)
   Description of what the command does */
PHP_METHOD(ValkeyGlide, myCommand)
{
    // Implementation
}
/* }}} */
```

#### PHPDoc Comments

For stub files, use proper PHPDoc format:

```php
/**
 * Description of the method
 * 
 * @param string $key The key parameter
 * @param int $value The value parameter
 * @return mixed The return value
 * @throws ValkeyGlideException On error
 */
public function myCommand(string $key, int $value): mixed;
```

### Benchmarks

To run benchmarks for the PHP extension:

```bash
# Simple benchmark script
php -d extension=valkey_glide benchmark.php

# Compare with other PHP Valkey clients
php benchmark_comparison.php
```

### Troubleshooting

Common issues and solutions:

#### FFI Build Issues

- **FFI library not found**: Ensure you have built the FFI library with `cargo build --release` in the `valkey-glide/ffi` directory
- **Rust compiler not found**: Install rustup and ensure `cargo` is in your PATH
- **FFI build fails on Linux**: Install ziglang and zigbuild: `pip3 install ziglang && cargo install --locked cargo-zigbuild`
- **Missing protobuf-c libraries**: Install `libprotobuf-c-dev` (Ubuntu) or `protobuf-c-devel` (CentOS/RHEL)
- **cbindgen not found**: Install with `cargo install cbindgen`

#### Build Issues

- **phpize not found**: Install `php-dev` or `php-devel` package
- **configure fails**: Check if all dependencies are installed
- **make fails**: Ensure you have the correct PHP headers version
- **Link errors with libglide_ffi**: Verify the FFI library was built successfully and exists at `valkey-glide/ffi/target/release/libglide_ffi.a`

#### Submodule Issues

- **Submodule not initialized**: Run `git submodule update --init --recursive` to initialize all submodules
- **Submodule out of date**: Run `git submodule update --recursive` to update submodules to the correct versions
- **Missing valkey-glide directory**: Ensure you cloned with `--recurse-submodules` or manually initialize submodules

#### Runtime Issues

- **Extension not loading**: Check `php.ini` configuration and file permissions
- **Segmentation faults**: Use GDB and Valgrind for debugging
- **Memory leaks**: Run with Valgrind to identify leaks

#### Installation Issues

- **Extension installed but not found**: Check PHP extension directory with `php-config --extension-dir`
- **Version conflicts**: Ensure PHP development headers match your PHP version

### Recommended extensions for VS Code

- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - C/C++ language support
- [PHP](https://marketplace.visualstudio.com/items?itemName=DEVSENSE.phptools-vscode) - PHP language support
- [PHP Intelephense](https://marketplace.visualstudio.com/items?itemName=bmewburn.vscode-intelephense-client) - Advanced PHP intellisense
- [rust-analyzer](https://marketplace.visualstudio.com/items?itemName=rust-lang.rust-analyzer) - For Rust core debugging
- [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) - C/C++ language server

## Community and Feedback

We encourage you to join our community to support, share feedback, and ask questions. You can approach us for anything on our Valkey Slack: [Join Valkey Slack](https://join.slack.com/t/valkey-oss-developer/shared_invite/zt-2nxs51chx-EB9hu9Qdch3GMfRcztTSkQ).
