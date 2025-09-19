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
sudo yum install -y php-devel php-cli git gcc make autoconf automake libtool pkgconfig openssl openssl-devel unzip php-bcmath
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

#### Installing from Packagist
You can use pie to install the extension from the Packagist repository.
See: https://packagist.org/packages/valkey-io/valkey-glide-php

Before starting this step, make sure you've installed all dependencies above.

Additionally, you will need to install the pie tool.

On Linux, you can download pie with curl. eg:
```bash
curl -L https://github.com/php/pie/releases/latest/download/pie.phar -o pie
chmod +x pie
sudo mv pie /usr/local/bin/pie
export PATH="$PATH:/usr/local/bin"
```

On MacOS, install with Homebrew:
```bash
brew install pie
```

To install the Valkey Glide extension, simply use the pie command:
```bash
# VERSION can be set to any release tag or branch at https://github.com/valkey-io/valkey-glide-php.git
export VERSION=1.0.0
pie install valkey-io/valkey-glide-php:$VERSION
```

#### Installing from PECL
You can install the extension using PECL from GitHub releases:

```bash
# Install from a specific release
pecl install https://github.com/valkey-io/valkey-glide-php/releases/download/v1.0.0/valkey_glide-1.0.0.tgz

# Or download and install manually
wget https://github.com/valkey-io/valkey-glide-php/releases/download/v1.0.0/valkey_glide-1.0.0.tgz
pecl install valkey_glide-1.0.0.tgz
```

After installation, add the extension to your php.ini:
```ini
extension=valkey_glide
```

Verify the installation:
```bash
php -m | grep valkey_glide
php -r "if (extension_loaded('valkey_glide')) echo 'SUCCESS: Extension loaded!'; else echo 'ERROR: Extension not loaded';"
```

#### Steps for building and installing from source

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

To run unit tests, use:

```bash
protoc --proto_path=./valkey-glide/glide-core/src/protobuf --php_out=./tests/ ./valkey-glide/glide-core/src/protobuf/connection_request.proto
composer install --no-interaction --prefer-dist --optimize-autoloader
make install
cd tests
./start_valkey_with_replicas.sh 
./create-valkey-cluster.sh 
php -n -d extension=../modules/valkey_glide.so TestValkeyGlide.php

```

### Linters

Development on the PHP wrapper involves changes in both C and PHP code. We have comprehensive linting infrastructure to ensure code quality and consistency. All linting checks are automatically run in our GitHub Actions CI pipeline.

#### Language-specific Linters

**C Code:**
- **clang-format**: Code formatting with Google-based style (4-space indentation, 100-char line limit)
- **valgrind**: Memory leak detection during testing

**PHP Code:**
- **PHP_CodeSniffer (phpcs)**:  coding standards.
- **PHP Code Beautifier (phpcbf)**: Automatic code formatting


#### Linting Configuration Files

The project includes comprehensive linting configuration:

- **`phpcs.xml`**: PHP_CodeSniffer configuration with PSR-12 standards
- **`.clang-format`**: C code formatting rules (Google-based style)
- **`composer.json`**: Development dependencies and linting scripts

#### IDE Integration

VSCode is configured to automatically use the project's linting tools:

- Real-time PHP and C code linting
- Format on save for both languages
- Integration with project-specific linting configurations
- Extensions recommended: PHP, C/C++, phpcs, phpstan, clangd



### Extension Development Guidelines

#### PHP Extension Conventions

When adding new functionality to the PHP extension:

1. **Function Naming**: Use `valkey_glide_` prefix for internal functions
2. **Memory Management**: Always use `emalloc`/`efree` for request-scoped memory
3. **Error Handling**: Use PHP's exception system via `zend_throw_exception`
4. **Parameter Parsing**: Use `ZEND_PARSE_PARAMETERS_START` macros
5. **Return Values**: Use appropriate `RETURN_*` macros

## Community and Feedback

We encourage you to join our community to support, share feedback, and ask questions. You can approach us for anything on our Valkey Slack: [Join Valkey Slack](https://join.slack.com/t/valkey-oss-developer/shared_invite/zt-2nxs51chx-EB9hu9Qdch3GMfRcztTSkQ).
