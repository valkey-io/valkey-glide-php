# Valkey GLIDE PHP Examples

This directory contains comprehensive examples demonstrating how to use the Valkey GLIDE PHP client effectively.

## Prerequisites

Before running these examples, ensure you have:

1. **Valkey GLIDE PHP extension installed** - See the main README.md for installation instructions
2. **Valkey server running** - Use the provided Docker setup or install locally
3. **PHP 8.1+** with the extension loaded

## Quick Start

1. **Start Valkey servers using Docker:**
   ```bash
   cd examples/utils
   ./create-valkey-cluster.sh
   ./start_valkey_with_replicas.sh
   ```

2. **Run a basic example:**
   ```bash
   php basic/standalone_client.php
   ```

## Directory Structure

### 📚 Basic Examples (`basic/`)
- **`standalone_client.php`** - Basic standalone server connection
- **`cluster_client.php`** - Basic cluster connection setup
- **`configuration.php`** - Client configuration options

### 🏗️ Data Structures (`data_structures/`)
- **`strings.php`** - String operations (GET, SET, INCR, etc.)
- **`lists.php`** - List operations (LPUSH, RPOP, LRANGE, etc.)
- **`sets.php`** - Set operations (SADD, SREM, SINTER, etc.)
- **`hashes.php`** - Hash operations (HSET, HGET, HGETALL, etc.)
- **`sorted_sets.php`** - Sorted set operations (ZADD, ZRANGE, etc.)
- **`streams.php`** - Stream operations (XADD, XREAD, etc.)

### 🚀 Advanced Features (`advanced/`)
- **`transactions.php`** - Multi-command transactions
- **`pub_sub.php`** - Publish/Subscribe messaging
- **`scripting.php`** - Lua script execution
- **`pipelines.php`** - Command pipelining for performance

### 🎯 Real-world Patterns (`patterns/`)
- **`caching.php`** - Web application caching patterns
- **`session_store.php`** - Session storage implementation
- **`rate_limiting.php`** - Rate limiting with sliding windows
- **`distributed_lock.php`** - Distributed locking mechanisms

### ⚠️ Error Handling (`error_handling/`)
- **`connection_errors.php`** - Handle connection failures
- **`timeout_handling.php`** - Manage timeouts gracefully
- **`retry_logic.php`** - Implement retry mechanisms

### 🛠️ Development Utils (`utils/`)
- **`docker-compose.yml`** - Local Valkey server setup
- **`setup_servers.sh`** - Helper script for manual setup

## Running Examples

### Individual Examples
```bash
# Run specific example
php basic/standalone_client.php

# Run with custom server
php basic/standalone_client.php --host=127.0.0.1 --port=6380
```

### All Examples
```bash
# Run all basic examples
find basic/ -name "*.php" -exec php {} \;

# Run examples with error checking
./utils/run_all_examples.sh
```

## Configuration

Most examples can be configured via environment variables:

```bash
export VALKEY_HOST=localhost
export VALKEY_PORT=6379
export VALKEY_PASSWORD=""
export VALKEY_USE_TLS=false
```

## Common Patterns

### Error Handling
All examples demonstrate proper error handling:

```php
try {
    $client = new ValkeyGlide($addresses);
    // ... operations
} catch (ValkeyGlideException $e) {
    echo "Valkey error: " . $e->getMessage() . "\n";
} catch (Exception $e) {
    echo "General error: " . $e->getMessage() . "\n";
} finally {
    if (isset($client)) {
        $client->close();
    }
}
```

### Resource Management
Always close connections:

```php
$client = new ValkeyGlide($addresses);
try {
    // ... operations
} finally {
    $client->close();
}
```

## Performance Tips

1. **Use connection pooling** for high-traffic applications
2. **Batch operations** with pipelining when possible
3. **Set appropriate timeouts** based on your use case
4. **Monitor memory usage** with large data sets
5. **Use transactions** for atomic operations

## Troubleshooting

### Common Issues

**Extension not loaded:**
```bash
php -m | grep valkey_glide
# Should show: valkey_glide
```

**Connection refused:**
- Check if Valkey server is running
- Verify host/port configuration
- Check firewall settings

**Memory issues:**
- Increase PHP memory limit
- Use streaming for large data sets
- Monitor extension memory usage

### Debug Mode
Enable debug output in examples:

```bash
DEBUG=1 php basic/standalone_client.php
```

## Contributing

When adding new examples:

1. Include comprehensive error handling
2. Add clear documentation and comments
3. Follow PSR-12 coding standards
4. Test with both standalone and cluster modes
5. Include performance considerations

## Support

- **Documentation**: See main README.md and DEVELOPER.md
- **Issues**: Report on GitHub
- **Community**: Join the Valkey Slack workspace

---

**Note**: These examples are for demonstration purposes. In production, implement additional security, monitoring, and error handling as needed.
