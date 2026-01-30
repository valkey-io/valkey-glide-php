# Valkey GLIDE PHP Soak Test

## Overview

**Endurance Testing (Soak Testing)** runs the Valkey Glide PHP client under sustained load for extended periods (hours/days) to detect:

This test simulates real-world usage patterns by executing a mix of Valkey commands continuously and monitoring system health.

## What is Tested

### Command Categories

The test executes commands from six categories with weighted probability distribution:

#### 1. String Operations
- `SET` - Write data
- `GET` - Read data
- `INCR` - Atomic increment

#### 2. Hash Operations
- `HSET` - Set hash field
- `HGET` - Get hash field
- `HGETALL` - Get all fields

#### 3. List Operations
- `LPUSH` - Push to list
- `RPOP` - Pop from list
- `LRANGE` - Get range

#### 4. Set Operations
- `SADD` - Add to set
- `SMEMBERS` - Get all members

#### 5. Sorted Set Operations
- `ZADD` - Add with score
- `ZRANGE` - Get range

#### 6. Key Management
- `DEL` - Delete keys
- `EXISTS` - Check existence
- `EXPIRE` - Set TTL

## Usage

### Basic Usage

```bash
# Run for 24 hours (default) on standalone server
php run.php

# Run for specific duration (in hours)
php run.php --duration=48

# Run on cluster mode
php run.php --cluster

# Run on custom host/port
php run.php --host=redis.example.com --port=6380
```

### Command-line Options

- `--duration` - Test duration in hours (default: `24`)
- `--cluster` - Use cluster mode instead of standalone
- `--host` - Server hostname (default: `localhost`)
- `--port` - Server port (default: `6379`)

### Examples

**24-hour standalone test:**
```bash
php run.php --duration=24
```

**48-hour cluster test:**
```bash
php run.php --duration=48 --cluster
```

**1-hour quick test:**
```bash
php run.php --duration=1
```

**Custom server:**
```bash
php run.php --host=redis-prod.example.com --port=6380 --duration=72
```

## Monitoring

### Progress Reports

The test reports progress every 5 minutes with:

- **Elapsed Time** - Hours and minutes since start
- **Total Operations** - Number of commands executed
- **Operations/sec** - Current throughput
- **Errors** - Error count and percentage
- **Memory Usage** - Current PHP memory consumption

Example output:
```
=== Progress Report ===
Elapsed: 2h 15m
Total Operations: 1,234,567
Operations/sec: 152.41
Errors: 0 (0.00%)
Memory: 45.23 MB
======================
```

### Final Report

At test completion, a comprehensive report includes:

- Overall statistics
- Command distribution (which commands were executed and how often)
- Error types (if any errors occurred)
- Total runtime and throughput


## Configuration

### Adjusting Test Parameters

Edit `run.php` to modify:

```php
// Test duration (hours)
const TEST_DURATION_HOURS = 24;

// Operations per cycle
const OPERATIONS_PER_CYCLE = 1000;

// Report interval (seconds)
const REPORT_INTERVAL_SECONDS = 300;

// Command weights (must sum to 100)
const COMMAND_WEIGHTS = [
    'string_ops' => 40,
    'hash_ops' => 20,
    'list_ops' => 15,
    'set_ops' => 10,
    'zset_ops' => 10,
    'key_mgmt' => 5,
];
```

### Memory Alert Threshold

Modify the threshold in `checkMemory()`:

```php
if ($memoryMB > 500) {  // Alert at 500MB
    error_log("WARNING: High memory usage: {$memoryMB} MB");
}
```
