# Welcome to Valkey GLIDE PHP!

Valkey General Language Independent Driver for the Enterprise (GLIDE) is the official open-source Valkey client library, proudly part of the Valkey organization. Our mission is to make your experience with Valkey and Redis OSS seamless and enjoyable. Whether you're a seasoned developer or just starting out, Valkey GLIDE is here to support you every step of the way.

**`valkey-glide-php`** is the PHP binding for Valkey GLIDE. It brings the power and flexibility of the Valkey GLIDE core to the PHP ecosystem, with a familiar and convenient interface based on the popular [PHPRedis](https://github.com/phpredis/phpredis) API. By staying mostly API-compatible with PHPRedis, this client offers an easy migration path and minimal learning curve—while adding the features of Valkey GLIDE.

> We chose [PHPRedis](https://github.com/phpredis/phpredis) because it is a powerful and widely adopted Redis client for PHP.

⚠️ **Note:** This client is currently under active development. Not all features are available yet, but a public preview with a subset of core functionality will be released soon.

# Why Choose Valkey GLIDE?

- **Community and Open Source**: Join our vibrant community and contribute to the project. We are always here to respond, and the client is for the community.
- **Reliability**: Built with best practices learned from over a decade of operating Redis OSS-compatible services.
- **Performance**: Optimized for high performance and low latency.
- **High Availability**: Designed to ensure your applications are always up and running.
- **Cross-Language Support**: Implemented using a core driver framework written in Rust, with language-specific extensions to ensure consistency and reduce complexity.
- **Stability and Fault Tolerance**: We brought our years of experience to create a bulletproof client.
- **Backed and Supported by AWS and GCP**: Ensuring robust support and continuous improvement of the project.

## Key Features
- **[AZ Affinity](https://valkey.io/blog/az-affinity-strategy/)** – Ensures low-latency connections and minimal cross-zone costs by routing read traffic to replicas in the clients availability zone. **(Requires Valkey server version 8.0+ or AWS ElastiCache for Valkey 7.2+)**.
- **[PubSub Auto-Reconnection](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#pubsub-support:~:text=PubSub%20Support,Receiving%2C%20and%20Unsubscribing.)** – Seamless background resubscription on topology updates or disconnection.
- **[Sharded PubSub](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#pubsub-support:~:text=Receiving%2C%20and%20Unsubscribing.-,Subscribing,routed%20to%20the%20server%20holding%20the%20slot%20for%20the%20command%27s%20channel.,-Receiving)** – Native support for sharded PubSub across cluster slots.
- **[Cluster-Aware MGET/MSET/DEL/FLUSHALL](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#multi-slot-command-handling:~:text=Multi%2DSlot%20Command%20Execution,JSON.MGET)** – Execute multi-key commands across cluster slots without manual key grouping.
- **[Cluster Scan](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#cluster-scan)** – Unified key iteration across shards using a consistent, high-level API for cluster environments.
- **[Batching (Pipeline and Transaction)](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#batching-pipeline-and-transaction)** – Efficiently execute multiple commands in a single network roundtrip, significantly reducing latency and improving throughput.
- **[OpenTelemetry](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#opentelemetry)** – Integrated tracing support for enhanced observability and easier debugging in distributed environments.

## Supported Engine Versions

Valkey GLIDE is API-compatible with the following engine versions:

| Engine Type           |  6.2  |  7.0  |   7.1  |  7.2  |  8.0  |  8.1  |
|-----------------------|-------|-------|--------|-------|-------|-------|
| Valkey                |   -   |   -   |   -    |   V   |   V   |   V   |
| Redis                 |   V   |   V   |   V    |   V   |   -   |   -   |

# Getting Started - PHP Wrapper

## System Requirements

The release of Valkey GLIDE was tested on the following platforms:

Linux:

-   Ubuntu 20 (x86_64/amd64 and arm64/aarch64)

**Note: Currently Alpine Linux / MUSL is NOT supported.**

macOS:

-   macOS 14.7 (Apple silicon/aarch_64)

## PHP Supported Versions

| PHP Version |
|-------------|
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
brew install protobuf-c
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
    git clone --recurse-submodules https://github.com/valkey-io/valkey-glide-php.git
    cd valkey-glide-php
    ```

2. Initialize submodules (if not cloned with --recurse-submodules):
    ```bash
    git submodule update --init --recursive
    ```

3. Build the FFI library (required dependency):
    ```bash
    python3 utils/remove_optional_from_proto.py
    cd valkey-glide/ffi
    cargo build --release
    cd ../../
    ```

4. Build the extension:
    ```bash
    phpize
    ./configure --enable-valkey-glide
    make build-modules-pre
    make install
    ```

5. Enable the extension by adding it to your `php.ini` file:
    ```ini
    extension=valkey_glide
    ```

6. Execute the tests:
    ```
    php -n -d extension=./modules/valkey_glide.so tests/TestValkeyGlide.php
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
        $addresses,                       // addresses
        false,                            // use_tls
        null,                             // credentials  
        ValkeyGlide::READ_FROM_PRIMARY,   // read_from (PRIMARY)
        500                               // request_timeout (500ms)
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
        $addresses,                          // addresses
        false,                               // use_tls
        null,                                // credentials
        ValkeyGlide::READ_FROM_PRIMARY,      // read_from (PRIMARY)
        500                                  // request_timeout (500ms)
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

### Contributing

All contributions are automatically validated through our CI pipeline, ensuring:
- Code style compliance
- All tests passing across supported PHP versions
- Memory leak detection and performance benchmarks

## Building & Testing

Development instructions for local building & testing the package are in the [DEVELOPER.md](DEVELOPER.md) file.

## Contributing

GitHub is a platform for collaborative coding. If you're interested in writing code, we encourage you to contribute by submitting pull requests from forked copies of this repository. Additionally, please consider creating GitHub issues for reporting bugs and suggesting new features. Feel free to comment on issues that interest. For more info see [Contributing](./CONTRIBUTING.md).

## Get Involved!

We invite you to join our open-source community and contribute to Valkey GLIDE. Whether it's reporting bugs, suggesting new features, or submitting pull requests, your contributions are highly valued. Check out our [Contributing Guidelines](./CONTRIBUTING.md) to get started.

If you have any questions or need assistance, don't hesitate to reach out. Open a GitHub issue, and our community and contributors will be happy to help you.

## Community Support and Feedback

We encourage you to join our community to support, share feedback, and ask questions. You can approach us for anything on our Valkey Slack: [Join Valkey Slack](https://join.slack.com/t/valkey-oss-developer/shared_invite/zt-2nxs51chx-EB9hu9Qdch3GMfRcztTSkQ).

## License
* [Apache License 2.0](./LICENSE)
