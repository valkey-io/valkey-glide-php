# PHP Benchmarks

This directory contains benchmark scripts for comparing Valkey GLIDE PHP client performance against phpredis.

## Requirements

- PHP 8.2 or 8.3
- Valkey GLIDE PHP extension installed
- phpredis extension installed (for comparison)
- Running Valkey/Redis server

## Usage

### Basic Usage

```bash
php php_benchmark.php
```

### Command-line Options

- `--resultsFile` - Output file path (default: `../results/php-results.json`)
- `--dataSize` - Size of data in bytes (default: `100`)
- `--clients` - Which clients to run: `all`, `glide`, or `phpredis` (default: `all`)
- `--host` - Server hostname (default: `localhost`)
- `--port` - Server port (default: `6379`)
- `--iterationLevel` - Iteration scale levels (default: `1,10,100,1000`)
  - Controls the number of operations: `level × 10,000`
  - `1` = 100K operations, `10` = 100K operations, `100` = 1M operations, `1000` = 5M operations
  - **Note**: PHP benchmark runs sequentially (no true concurrency)
- `--tls` - Enable TLS connection
- `--clusterModeEnabled` - Benchmark cluster mode

### Examples

**Standalone server:**
```bash
php php_benchmark.php --host=localhost --port=6379
```

**Cluster mode:**
```bash
php php_benchmark.php --host=localhost --port=7001 --clusterModeEnabled
```

**With TLS:**
```bash
php php_benchmark.php --host=localhost --port=6379 --tls
```

**Custom data size and concurrency:**
```bash
php php_benchmark.php --dataSize=1000 --iterationLevel=1,10,100
```

**Only test Valkey GLIDE:**
```bash
php php_benchmark.php --clients=glide
```

**Quick test (low iterations):**
```bash
php php_benchmark.php --iterationLevel=1
```

## Benchmark Methodology

The benchmark tests three operations with weighted probabilities:
- **GET (existing key)**: 64% - Retrieve keys from a 3M key set
- **GET (non-existing key)**: 16% - Query keys that don't exist (3M-3.75M range, outside SET keyspace to guarantee cache misses)
- **SET**: 20% - Write to the 3M key set

### Metrics Collected

For each operation type:
- P50, P90, P99 latency (milliseconds)
- Average latency (milliseconds)
- Standard deviation
- Overall TPS (transactions per second)

### Output Format

Results are saved as JSON with the following structure:

```json
[
  {
    "client": "phpredis",
    "num_of_tasks": 1,
    "data_size": 100,
    "tps": 45000,
    "client_count": 1,
    "is_cluster": false,
    "get_existing_p50_latency": 0.12,
    "get_existing_p90_latency": 0.25,
    "get_existing_p99_latency": 0.45,
    "get_existing_average_latency": 0.15,
    "get_existing_std_dev": 0.08,
    "get_non_existing_p50_latency": 0.11,
    "get_non_existing_p90_latency": 0.23,
    "get_non_existing_p99_latency": 0.42,
    "get_non_existing_average_latency": 0.14,
    "get_non_existing_std_dev": 0.07,
    "set_p50_latency": 0.18,
    "set_p90_latency": 0.35,
    "set_p99_latency": 0.58,
    "set_average_latency": 0.22,
    "set_std_dev": 0.12
  }
]
```

## Current Limitations

- **Single-process only**: Multi-process concurrency is not supported due to ValkeyGlide's Tokio runtime incompatibility with `pcntl_fork()`. The benchmark runs sequentially, measuring per-operation latency rather than true concurrent throughput.
- **No connection pooling**: Each benchmark run uses a single client connection.
- **phpredis comparison**: Requires phpredis extension to be installed separately.

