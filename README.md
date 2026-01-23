# Welcome to Valkey GLIDE PHP

Valkey General Language Independent Driver for the Enterprise (GLIDE) is the official open-source Valkey client library, proudly part of the Valkey organization. Our mission is to make your experience with Valkey and Redis OSS seamless and enjoyable. Whether you're a seasoned developer or just starting out, Valkey GLIDE is here to support you every step of the way.

**`valkey-glide-php`** is the PHP binding for Valkey GLIDE. It brings the power and flexibility of the Valkey GLIDE core to the PHP ecosystem, with a familiar and convenient interface based on the popular [PHPRedis](https://github.com/phpredis/phpredis) API. By staying mostly API-compatible with PHPRedis, this client offers an easy migration path and minimal learning curve—while adding the features of Valkey GLIDE.

> We chose [PHPRedis](https://github.com/phpredis/phpredis) because it is a powerful and widely adopted Redis client for PHP.

⚠️ **Note:** This client is currently under active development. Not all features are available yet, but a public preview with a subset of core functionality will be released soon.

## Why Choose Valkey GLIDE?

- **Community and Open Source**: Join our vibrant community and contribute to the project. We are always here to respond, and the client is for the community.
- **Reliability**: Built with best practices learned from over a decade of operating Redis OSS-compatible services.
- **Performance**: Optimized for high performance and low latency.
- **High Availability**: Designed to ensure your applications are always up and running.
- **Cross-Language Support**: Implemented using a core driver framework written in Rust, with language-specific extensions to ensure consistency and reduce complexity.
- **Stability and Fault Tolerance**: We brought our years of experience to create a bulletproof client.
- **Backed and Supported by AWS and GCP**: Ensuring robust support and continuous improvement of the project.

## Key Features

- **[Cluster-Aware MGET/MSET/DEL/FLUSHALL](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#multi-slot-command-handling:~:text=Multi%2DSlot%20Command%20Execution,JSON.MGET)** – Execute multi-key commands across cluster slots without manual key grouping.
- **[Cluster Scan](https://github.com/valkey-io/valkey-glide/wiki/General-Concepts#cluster-scan)** – Unified key iteration across shards using a consistent, high-level API for cluster environments.

## Supported Engine Versions

Valkey GLIDE is API-compatible with the following engine versions:

| Engine Type           |  6.2  |  7.0  |   7.1  |  7.2  |  8.0  |  8.1  |
|-----------------------|-------|-------|--------|-------|-------|-------|
| Valkey                |   -   |   -   |   -    |   V   |   V   |   V   |
| Redis                 |   V   |   V   |   V    |   V   |   -   |   -   |

## Getting Started - PHP Wrapper

### System Requirements

The release of Valkey GLIDE was tested on the following platforms:

Linux:

- Ubuntu 20 (x86_64/amd64 and arm64/aarch64)

**Note: Currently Alpine Linux / MUSL is NOT supported.**

macOS:

- macOS 14.7 (Apple silicon/aarch_64)

### PHP Supported Versions

| PHP Version |
|-------------|
| 8.2         |
| 8.3         |

### Installation and Setup

#### Prerequisites

Before installing Valkey GLIDE PHP extension, ensure you have the following dependencies:

- PHP development headers (`php-dev` or `php-devel`)
- Build tools (`gcc`, `make`, `autotools`)
- Git
- pkg-config
- protoc (protobuf compiler) >= v3.20.0
- openssl and openssl-dev
- rustup (Rust toolchain)
- php-bcmath (Protobuf PHP dependency, needed only for testing)

#### Installing Dependencies

**Ubuntu/Debian:**

```bash
sudo apt update -y
sudo apt install -y php-dev php-cli git gcc make autotools-dev pkg-config openssl libssl-dev unzip php-bcmath
# Install rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

**CentOS/RHEL:**

```bash
sudo yum update -y
sudo yum install -y php-devel php-cli git gcc make pkgconfig openssl openssl-devel unzip php-bcmath
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

#### Installing protobuf compiler

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

### Installing from Packagist

You can use pie to install the extension from the Packagist repository.
See: <https://packagist.org/packages/valkey-io/valkey-glide-php>

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

### Installing from PECL

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

#### Building and Installing the Extension from source

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
    python3 utils/patch_proto_and_rust.py
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

6. Generate PHP protobuf classes used for testing purposes:

   ```bash
   protoc --proto_path=./valkey-glide/glide-core/src/protobuf --php_out=./tests/ ./valkey-glide/glide-core/src/protobuf/connection_request.proto
   ```

7. Install PHP dependencies with composer:

   ```bash
   composer install --no-interaction --prefer-dist --optimize-autoloader
   ```

8. Execute the tests:

    ```bash
    make test
    ```

## Basic Examples

### Standalone Valkey

```php
// Create ValkeyGlide client
$client = new ValkeyGlide();
$client->connect(addresses: [['host' => 'localhost', 'port' => 6379]]);

// Basic operations
$setResult = $client->set('foo', 'bar');
echo "SET result: " . $setResult . "\n";

$getValue = $client->get('foo');
echo "GET result: " . $getValue . "\n";

$pingResult = $client->ping();
echo "PING result: " . $pingResult . "\n";

// Close the connection
$client->close();
```

### PHPRedis-Compatible Class Names

For easier migration from PHPRedis, you can use PHPRedis-compatible class names:

**Standalone:**

```php
// Load the PHPRedis compatibility aliases
require_once 'vendor/valkey-io/valkey-glide-php/phpredis_aliases.php';

// Now you can use Redis instead of ValkeyGlide
$client = new Redis();
$client->connect(addresses: [['host' => 'localhost', 'port' => 6379]]);

$client->set('foo', 'bar');
$value = $client->get('foo');

// Exceptions are also aliased
try {
    // Operations that may fail
} catch (RedisException $e) {
    echo "Caught RedisException: " . $e->getMessage() . "\n";
}

$client->close();
```

**Cluster:**

```php
// Load the PHPRedis compatibility aliases
require_once 'vendor/valkey-io/valkey-glide-php/phpredis_aliases.php';

// Now you can use RedisCluster instead of ValkeyGlideCluster
$cluster = new RedisCluster(
    addresses: [
        ['host' => 'localhost', 'port' => 7001],
        ['host' => 'localhost', 'port' => 7002]
    ]
);

$cluster->set('foo', 'bar');
$value = $cluster->get('foo');

$cluster->close();
```

**Requirements:**

- PHP 8.3 or higher (required for `class_alias()` support with internal classes)

### With IAM Authentication for AWS ElastiCache

```php
// Create ValkeyGlide client with IAM authentication.
$client = new ValkeyGlide();
$client->connect(
    addresses: [['host' => 'my-cluster.xxxxx.use1.cache.amazonaws.com', 'port' => 6379]],
    use_tls: true,  // REQUIRED for IAM authentication
    credentials: [
        'username' => 'my-iam-user',  // REQUIRED for IAM
        'iamConfig' => [
            ValkeyGlide::IAM_CONFIG_CLUSTER_NAME => 'my-cluster',
            ValkeyGlide::IAM_CONFIG_REGION => 'us-east-1',
            ValkeyGlide::IAM_CONFIG_SERVICE => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
            ValkeyGlide::IAM_CONFIG_REFRESH_INTERVAL => 300  // Optional, defaults to 300 seconds
        ]
    ]
);

// Use the client normally - IAM tokens are managed automatically
$client->set('key', 'value');
$value = $client->get('key');
echo "Value: " . $value . "\n";

$client->close();
```

### With TLS

```php
// Create ValkeyGlide client with TLS configuration
$client = new ValkeyGlide();
$client->connect(
    addresses: [['host' => 'localhost', 'port' => 6379]],
    use_tls: true,
    advanced_config: ['tls_config' => ['root_certs' => $root_certs_data]]
);

// Create ValkeyGlide client with TLS stream context
$client = new ValkeyGlide();
$client->connect(
    addresses: [['host' => 'localhost', 'port' => 6379]],
    context: stream_context_create(['ssl' => 'ca-cert.pem'])   
)
```

### Cluster Valkey

```php
// Create cluster client configuration
$addresses = [
    ['host' => 'localhost', 'port' => 7001],
    ['host' => 'localhost', 'port' => 7002],
    ['host' => 'localhost', 'port' => 7003]
];

// Create ValkeyGlideCluster client with multi-database support (Valkey 9.0+)
$client = new ValkeyGlideCluster(
    addresses: $addresses,
    read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA, 
    database_id: 1 // Requires Valkey 9.0+ with cluster-databases > 1
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
```

## Building & Testing

Development instructions for local building & testing the package are in the [DEVELOPER.md](DEVELOPER.md) file.

## Contributing

GitHub is a platform for collaborative coding. If you're interested in writing code, we encourage you to contribute by submitting pull requests from forked copies of this repository. Additionally, please consider creating GitHub issues for reporting bugs and suggesting new features. Feel free to comment on issues that interest. For more info see [Contributing](./CONTRIBUTING.md).

All contributions are automatically validated through our CI pipeline, ensuring:

- Code style compliance
- All tests passing across supported PHP versions
- Memory leak detection and performance benchmarks

## Get Involved

We invite you to join our open-source community and contribute to Valkey GLIDE. Whether it's reporting bugs, suggesting new features, or submitting pull requests, your contributions are highly valued. Check out our [Contributing Guidelines](./CONTRIBUTING.md) to get started.

If you have any questions or need assistance, don't hesitate to reach out. Open a GitHub issue, and our community and contributors will be happy to help you.

## Community Support and Feedback

We encourage you to join our community to support, share feedback, and ask questions. You can approach us for anything on our Valkey Slack: [Join Valkey Slack](https://join.slack.com/t/valkey-oss-developer/shared_invite/zt-2nxs51chx-EB9hu9Qdch3GMfRcztTSkQ).

## License

- [Apache License 2.0](./LICENSE)
