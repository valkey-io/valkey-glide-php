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
- `--clientCount` - Comma-separated list of client counts (default: `1`)
- `--concurrentTasks` - Comma-separated list of concurrent task counts (default: `1,10,100,1000`)
- `--tls` - Enable TLS connection
- `--clusterModeEnabled` - Benchmark cluster mode
- `--minimal` - Run minimal benchmark (1000 iterations)

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
php php_benchmark.php --dataSize=1000 --concurrentTasks=1,10,100
```

**Only test Valkey GLIDE:**
```bash
php php_benchmark.php --clients=glide
```

**Minimal benchmark:**
```bash
php php_benchmark.php --minimal
```

## Benchmark Methodology

The benchmark tests three operations with weighted probabilities:
- **GET (existing key)**: 64% - Retrieve keys from a 3M key set
- **GET (non-existing key)**: 16% - Query keys that don't exist (3M-3.75M range)
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

### Why No Multi-Process Support?

ValkeyGlide uses a Rust core with the Tokio async runtime, which maintains file descriptors and event loops that cannot be safely duplicated across forked processes. This is a fundamental limitation of combining `pcntl_fork()` with async Rust runtimes.

**Impact**: The `--concurrentTasks` parameter controls the number of iterations, not true parallelism. Results show sequential performance characteristics.

### Why PHP is Not Multi-Threaded

PHP **can** be multi-threaded, but most installations are **not** because:

#### Default Build: Non-Thread-Safe (NTS)

```bash
# Check your PHP build
php -v
# Output: PHP 8.3.x (cli) (built: ...) (NTS)
                                          ^^^
                                    Non-Thread-Safe
```

**Two PHP builds exist:**
- **NTS (Non-Thread-Safe)**: Default, faster, single-threaded
- **ZTS (Zend Thread Safe)**: Optional, slower, supports threading via `pthreads` extension

#### Historical Design

PHP was designed for Apache's prefork MPM (multi-process, not multi-threaded):
- Each request = separate process
- No shared memory between requests
- Simple, stable, but resource-heavy
- "Share nothing" architecture

#### Extension Compatibility

Many popular PHP extensions are not thread-safe. If one extension isn't thread-safe, the entire PHP build must be NTS.

#### Threading Options and Their Limitations

**Option 1: pthreads/parallel extensions**
- Requires recompiling PHP with `--enable-zts`
- Not available in standard distributions
- ValkeyGlide extension built for NTS

**Option 2: pcntl_fork() (multi-process)**
- Works for pure PHP code
- **Breaks with ValkeyGlide**: Rust/Tokio runtime maintains file descriptors and event loops that become corrupted when forked
- Results in "Bad file descriptor" panics

**Option 3: PHP Fibers (8.1+)**
- Cooperative, not parallel (still single-threaded)
- No true concurrency

#### Comparison with Python/Node.js

**Python**: Uses threading with GIL (Global Interpreter Lock). Allows concurrent I/O operations even though only one thread executes Python code at a time.

**Node.js**: Single-threaded event loop with native async/await. Handles concurrency through non-blocking I/O.

**PHP**: No native async/await (Fibers are cooperative), no safe threading without ZTS, fork breaks Rust runtime. Result: Sequential execution only.

#### Why This Matters for Benchmarks

- **Python/Node.js benchmarks**: Measure true concurrent throughput (multiple operations running simultaneously)
- **PHP benchmark**: Measures sequential latency (operations run one after another)
- Both are valid metrics, but they measure different characteristics
- PHP results reflect typical PHP usage pattern (one request = one process)

## Future Enhancements

- Multi-process support for true concurrency testing
- Connection pool benchmarking
- Pipeline/batch operation benchmarks
- Memory usage profiling
