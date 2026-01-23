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
php run.php
```

### Command-line Options

- `--resultsFile` - Output file path (default: `../results/php-results.json`)
- `--dataSize` - Size of data in bytes (default: `100`)
- `--clients` - Which clients to run: `all`, `glide`, or `phpredis` (default: `all`)
- `--host` - Server hostname (default: `localhost`)
- `--port` - Server port (default: `6379`)
- `--iterations` - Number of operations to run (default: `100_000,1_000_000,5_000_000`)
  - Accepts comma-separated values for multiple runs
  - Min: 100,000 operations, Max: 5,000,000 operations
- `--tls` - Enable TLS connection
- `--clusterModeEnabled` - Benchmark cluster mode

### Examples

**Standalone server:**

```bash
php run.php --host=localhost --port=6379
```

**Cluster mode:**

```bash
php run.php --host=localhost --port=7001 --clusterModeEnabled
```

**With TLS:**

```bash
php run.php --host=localhost --port=6379 --tls
```

**Custom data size and iterations:**

```bash
php run.php --dataSize=1000 --iterations=100000,1000000
```

**Only test Valkey GLIDE:**

```bash
php run.php --clients=glide
```

**Quick test (low iterations):**

```bash
php run.php --iterations=100000
```

## Benchmark Methodology

The benchmark tests three operations with weighted probabilities:

- **GET (existing key)**: 64% - Retrieve keys that exist in the database
- **GET (non-existing key)**: 16% - Query keys that don't exist (guaranteed cache misses)
- **SET**: 20% - Write operations

The database is pre-populated with 3 million keys before benchmarking to ensure GET operations hit realistic data.

### Metrics Collected

For each operation type:

- P50, P90, P99 latency (milliseconds)
- Average latency (milliseconds)
- Standard deviation (milliseconds)
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
