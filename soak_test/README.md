# Valkey GLIDE PHP Soak Test

## Overview

**Endurance Testing (Soak Testing)** runs the ValkeyGlide PHP client under sustained load for extended periods (hours/days) to detect:

- Memory leaks
- Resource exhaustion
- Connection pool issues
- Performance degradation over time
- Stability under continuous operation

This test simulates real-world usage patterns by executing a mix of Redis commands continuously and monitoring system health.

## What is Tested

### Command Categories

The test executes commands from six categories with weighted probability distribution:

#### 1. String Operations (40%)
- `SET` - Write data
- `GET` - Read data
- `INCR` - Atomic increment

#### 2. Hash Operations (20%)
- `HSET` - Set hash field
- `HGET` - Get hash field
- `HGETALL` - Get all fields

#### 3. List Operations (15%)
- `LPUSH` - Push to list
- `RPOP` - Pop from list
- `LRANGE` - Get range

#### 4. Set Operations (10%)
- `SADD` - Add to set
- `SMEMBERS` - Get all members

#### 5. Sorted Set Operations (10%)
- `ZADD` - Add with score
- `ZRANGE` - Get range

#### 6. Key Management (5%)
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

## Key Metrics to Monitor

### 1. Memory Usage
- **Expected**: Stable memory consumption
- **Warning**: Continuous growth indicates memory leak
- **Threshold**: Alert if exceeds 500MB

### 2. Error Rate
- **Expected**: 0% or near 0%
- **Warning**: Any sustained error rate indicates issues

### 3. Operations/sec (Throughput)
- **Expected**: Consistent rate throughout test
- **Warning**: Degradation over time indicates performance issues

### 4. Connection Stability
- **Expected**: No connection drops
- **Warning**: Connection errors indicate network or server issues

## What to Look For

### Signs of Issues

❌ **Memory Leak**
- Memory usage grows continuously
- Does not stabilize after initial ramp-up

❌ **Performance Degradation**
- Operations/sec decreases over time
- Response times increase

❌ **Connection Issues**
- Connection reset errors
- Timeout errors
- Authentication failures

❌ **Resource Exhaustion**
- File descriptor errors
- Socket errors
- Out of memory errors

### Signs of Health

✅ **Stable Memory**
- Memory usage plateaus after initial operations
- No continuous growth

✅ **Consistent Throughput**
- Operations/sec remains steady
- No degradation over time

✅ **Zero Errors**
- No exceptions or connection issues
- All operations complete successfully

✅ **Predictable Behavior**
- Command distribution matches expected weights
- No unexpected patterns

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

## Best Practices

### 1. Run on Dedicated Test Environment
- Don't run on production servers
- Use isolated test instances

### 2. Monitor System Resources
- Watch server CPU, memory, disk I/O
- Monitor network connections
- Check server logs

### 3. Start with Short Duration
- Run 1-hour test first to verify setup
- Gradually increase to 24+ hours

### 4. Run Multiple Tests
- Test both standalone and cluster modes
- Test with different workload patterns
- Test under various server configurations

### 5. Document Results
- Save final reports
- Track memory usage over time
- Compare results across versions

## Troubleshooting

### High Memory Usage

**Symptom**: Memory exceeds 500MB or grows continuously

**Possible Causes**:
- Memory leak in client code
- Large result sets not being freed
- Connection pool not releasing resources

**Actions**:
- Check PHP memory_limit setting
- Review error logs for warnings
- Profile memory usage with Xdebug

### Connection Errors

**Symptom**: Connection reset or timeout errors

**Possible Causes**:
- Server max connections reached
- Network instability
- Server restart/maintenance

**Actions**:
- Check server connection limits
- Verify network stability
- Review server logs

### Performance Degradation

**Symptom**: Operations/sec decreases over time

**Possible Causes**:
- Server resource exhaustion
- Database fragmentation
- Memory pressure

**Actions**:
- Monitor server CPU/memory
- Check server slow log
- Review server configuration

## Output Files

The test writes to:
- **stdout** - Progress reports and final summary
- **stderr** - Error logs (via `error_log()`)

Redirect output to save results:

```bash
# Save all output
php run.php --duration=24 > soak_test_results.log 2>&1

# Save only errors
php run.php --duration=24 2> errors.log
```

## Requirements

- PHP 8.2 or 8.3
- ValkeyGlide PHP extension installed
- Running Valkey/Redis server
- Sufficient system resources (memory, connections)

## Notes

- The test runs continuously until the specified duration
- Press Ctrl+C to stop early (will not generate final report)
- Each command category uses a separate key namespace to avoid conflicts
- Keys are randomly distributed to simulate real-world access patterns
- Small delays (1ms) between cycles prevent server overload
