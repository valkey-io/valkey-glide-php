# Welcome to Valkey GLIDE!

Valkey General Language Independent Driver for the Enterprise (GLIDE) is the official open-source Valkey client library, proudly part of the Valkey organization. Our mission is to make your experience with Valkey and Redis OSS seamless and enjoyable. Whether you're a seasoned developer or just starting out, Valkey GLIDE is here to support you every step of the way.

# Why Choose Valkey GLIDE?

- **Community and Open Source**: Join our vibrant community and contribute to the project. We are always here to respond, and the client is for the community.
- **Reliability**: Built with best practices learned from over a decade of operating Redis OSS-compatible services.
- **Performance**: Optimized for high performance and low latency.
- **High Availability**: Designed to ensure your applications are always up and running.
- **Cross-Language Support**: Implemented using a core driver framework written in Rust, with language-specific extensions to ensure consistency and reduce complexity.
- **Stability and Fault Tolerance**: We brought our years of experience to create a bulletproof client.
- **Backed and Supported by AWS and GCP**: Ensuring robust support and continuous improvement of the project.

## Supported Engine Versions

Refer to the [Supported Engine Versions table](https://github.com/valkey-io/valkey-glide/blob/main/README.md#supported-engine-versions) for details.

# Getting Started - PHP Wrapper

## System Requirements

The release of Valkey GLIDE was tested on the following platforms:

Linux:

-   Ubuntu 20 (x86_64/amd64 and arm64/aarch64)
-   Amazon Linux 2 (AL2) and 2023 (AL2023) (x86_64)

**Note: Currently Alpine Linux / MUSL is NOT supported.**

macOS:

-   macOS 14.7 (Apple silicon/aarch_64)
-   macOS 13.7 (x86_64/amd64)

## PHP Supported Versions

| PHP Version |
|-------------|
| 8.1         |
| 8.2         |
| 8.3         |

## Installation and Setup

### Prerequisites

Before installing Valkey GLIDE PHP extension, ensure you have the following dependencies:

- PHP development headers (`php-dev` or `php-devel`)
- Build tools (`gcc`, `make`, `autotools`)
- Git
- pkg-config
- protoc (protobuf compiler) >= v3.20.0
- openssl and openssl-dev
- rustup (Rust toolchain)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update -y
sudo apt install -y php-dev php-cli git gcc make autotools-dev pkg-config openssl libssl-dev unzip
# Install rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

**CentOS/RHEL:**
```bash
sudo yum update -y
sudo yum install -y php-devel php-cli git gcc make pkgconfig openssl openssl-devel unzip
# Install rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

**macOS:**
```bash
brew update
brew install php git gcc make pkgconfig protobuf openssl
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

### Installing protobuf compiler

**For macOS:**
```bash
brew install protobuf
# Verify installation
protoc --version
```

**For Linux:**
```bash
PB_REL="https://github.com/protocolbuffers/protobuf/releases"
curl -LO $PB_REL/download/v3.20.3/protoc-3.20.3-linux-x86_64.zip
unzip protoc-3.20.3-linux-x86_64.zip -d $HOME/.local
export PATH="$PATH:$HOME/.local/bin"
# Verify installation (minimum version 3.20.0 required)
protoc --version
```

### Building and Installing the Extension

1. Clone the repository:
    ```bash
    git clone https://github.com/valkey-io/valkey-glide.git
    cd valkey-glide
    ```

2. Build the FFI library (required dependency):
    ```bash
    cd ffi
    cargo build --release
    cd ../php
    ```

3. Build the extension:
    ```bash
    phpize
    ./configure --enable-valkey-glide
    make build-modules-pre
    make install
    ```

4. Enable the extension by adding it to your `php.ini` file:
    ```ini
    extension=valkey_glide
    ```

5. Verify the extension is loaded:
    ```bash
    php -m | grep valkey_glide
    ```

## Basic Examples

### Standalone Valkey:

```php
<?php
try {
    // Create client configuration
    $addresses = [
        ['host' => 'localhost', 'port' => 6379]
    ];
    
    // Create ValkeyGlide client
    $client = new ValkeyGlide(
        $addresses,           // addresses
        false,               // use_tls
        null,                // credentials  
        0,                   // read_from (PRIMARY)
        500                  // request_timeout (500ms)
    );
    
    // Basic operations
    $setResult = $client->set('foo', 'bar');
    echo "SET result: " . $setResult . "\n";
    
    $getValue = $client->get('foo');
    echo "GET result: " . $getValue . "\n";
    
    $pingResult = $client->ping();
    echo "PING result: " . $pingResult . "\n";
    
    // Close the connection
    $client->close();
    
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
```

### Cluster Valkey:

```php
<?php
try {
    // Create cluster client configuration
    $addresses = [
        ['host' => 'localhost', 'port' => 7001],
        ['host' => 'localhost', 'port' => 7002],
        ['host' => 'localhost', 'port' => 7003]
    ];
    
    // Create ValkeyGlideCluster client
    $client = new ValkeyGlideCluster(
        $addresses,           // addresses
        false,               // use_tls
        null,                // credentials
        0,                   // read_from (PRIMARY)
        500                  // request_timeout (500ms)
    );
    
    // Basic operations
    $setResult = $client->set('foo', 'bar');
    echo "SET result: " . $setResult . "\n";
    
    $getValue = $client->get('foo');
    echo "GET result: " . $getValue . "\n";
    
    $pingResult = $client->ping();
    echo "PING result: " . $pingResult . "\n";
    
    // Close the connection
    $client->close();
    
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
```

### Configuration Options

The Valkey GLIDE PHP extension supports various configuration options:

```php
<?php
// Advanced configuration example
$client = new ValkeyGlide(
    $addresses,                    // Required: Array of server addresses
    true,                         // use_tls: Enable TLS encryption
    [                            // credentials: Authentication credentials
        'username' => 'myuser',
        'password' => 'mypass'
    ],
    1,                           // read_from: 0=PRIMARY, 1=PREFER_REPLICA, 2=AZ_AFFINITY
    1000,                        // request_timeout: Request timeout in milliseconds
    [                            // reconnect_strategy: Backoff strategy for reconnections
        'num_of_retries' => 3,
        'factor' => 2.0,
        'exponent_base' => 2
    ],
    0,                           // database_id: Database number (0-15 for standalone)
    'my-client',                 // client_name: Client identifier
    250,                         // inflight_requests_limit: Max concurrent requests
    'us-east-1a',               // client_az: Client availability zone
    [                            // advanced_config: Advanced configuration options
        'connection_timeout' => 5000
    ],
    false                        // lazy_connect: Whether to connect lazily
);
?>
```

## Development & Continuous Integration

The Valkey GLIDE PHP project includes comprehensive development infrastructure:

### Automated CI/CD Pipeline

- **GitHub Actions Integration**: Automated testing across PHP versions (8.1, 8.2, 8.3), multiple engine versions, and host platforms
- **Matrix Testing**: Comprehensive testing on Ubuntu, macOS, and containerized environments
- **Code Quality Enforcement**: Automated linting and static analysis for both C and PHP code
- **Benchmark Testing**: Performance regression testing integrated into the CI pipeline

### Code Quality Standards

- **PHP Standards**: PSR-12 coding standards with PHPStan static analysis (level 6)
- **C Code Standards**: Google-based formatting with comprehensive static analysis
- **Automated Formatting**: Pre-commit hooks and CI enforcement of code formatting
- **Comprehensive Testing**: Unit tests, integration tests, and memory leak detection

### Local Development

```bash
# Install development dependencies
make install-lint-tools

# Run all quality checks
make lint

# Fix formatting issues
make lint-fix

#Run tests
make install
cd tests
php -n -d extension=../modules/valkey_glide.so TestValkeyGlide.php
```

### Contributing

All contributions are automatically validated through our CI pipeline, ensuring:
- Code style compliance (PSR-12 for PHP, Google style for C)
- Static analysis passing (PHPStan level 6)
- All tests passing across supported PHP versions
- Memory leak detection and performance benchmarks

## Building & Testing

Development instructions for local building & testing the package are in the [DEVELOPER.md](DEVELOPER.md) file.

## Community and Feedback

We encourage you to join our community to support, share feedback, and ask questions. You can approach us for anything on our Valkey Slack: [Join Valkey Slack](https://join.slack.com/t/valkey-oss-developer/shared_invite/zt-2nxs51chx-EB9hu9Qdch3GMfRcztTSkQ).
