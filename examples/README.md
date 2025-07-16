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

### 🎯 Real-world Patterns (`patterns/`)
- **`caching.php`** - Web application caching patterns



## Running Examples

### Individual Examples
```bash
# Run specific example
php basic/standalone_client.php

```

### All Examples
```bash
# Run all basic examples
find basic/ -name "*.php" -exec php {} \;

```

## Configuration

Most examples can be configured via environment variables:


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
