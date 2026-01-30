<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

/**
 * Endurance/Soak Test for ValkeyGlide PHP Client
 *
 * Tests client stability under sustained load over extended periods
 */

// Configuration
const TEST_DURATION_HOURS = 24;  // Default duration
const OPERATIONS_PER_CYCLE = 1000;
const REPORT_INTERVAL_SECONDS = 120;  // Report every 2 minutes
const MEMORY_CHECK_INTERVAL = 100;

// Command weights (probability distribution)
const COMMAND_WEIGHTS = [
    'string_ops' => 40,    // 40% - SET, GET, INCR
    'hash_ops' => 20,      // 20% - HSET, HGET, HGETALL
    'list_ops' => 15,      // 15% - LPUSH, RPOP, LRANGE
    'set_ops' => 10,       // 10% - SADD, SMEMBERS
    'zset_ops' => 10,      // 10% - ZADD, ZRANGE
    'key_mgmt' => 5,       // 5%  - DEL, EXISTS, EXPIRE
];

class SoakTest
{
    private object $client;
    private int $startTime;
    private array $startRusage;
    private array $stats = [
        'total_operations' => 0,
        'errors' => 0,
        'command_counts' => [],
        'error_types' => [],
    ];

    public function __construct(bool $isCluster = false, string $host = 'localhost', int $port = 6379)
    {
        // Create client
        if ($isCluster) {
            $this->client = new ValkeyGlideCluster(addresses: [['host' => $host, 'port' => $port]]);
        } else {
            $this->client = new ValkeyGlide();
            $this->client->connect(addresses: [['host' => $host, 'port' => $port]]);
        }

        $this->prePopulate();
        $this->startTime = time();
        $this->startRusage = getrusage();
    }

    public function run(int|float $durationHours): void
    {
        $endTime = $this->startTime + (int)($durationHours * 3600);
        $lastReport = time();
        $cycle = 0;

        echo "Starting soak test for " . number_format($durationHours, 2) . " hours...\n";
        echo "End time: " . date('Y-m-d H:i:s', $endTime) . "\n\n";

        while (time() < $endTime) {
            $cycle++;

            // Run operations
            for ($i = 0; $i < OPERATIONS_PER_CYCLE; $i++) {
                $this->executeRandomCommand();

                // Check memory periodically
                if ($i % MEMORY_CHECK_INTERVAL === 0) {
                    $this->checkMemory();
                }
            }

            // Report progress
            if (time() - $lastReport >= REPORT_INTERVAL_SECONDS) {
                $this->reportProgress();
                $lastReport = time();
            }

            // Small delay to avoid overwhelming the server
            usleep(1000); // 1ms
        }

        $this->finalReport();
    }

    private function executeRandomCommand(): void
    {
        $category = $this->selectCommandCategory();

        try {
            switch ($category) {
                case 'string_ops':
                    $this->executeStringOp();
                    break;
                case 'hash_ops':
                    $this->executeHashOp();
                    break;
                case 'list_ops':
                    $this->executeListOp();
                    break;
                case 'set_ops':
                    $this->executeSetOp();
                    break;
                case 'zset_ops':
                    $this->executeZSetOp();
                    break;
                case 'key_mgmt':
                    $this->executeKeyMgmt();
                    break;
            }

            $this->stats['total_operations']++;
        } catch (Exception $e) {
            $this->stats['errors']++;
            $errorType = get_class($e);
            $this->stats['error_types'][$errorType] =
                ($this->stats['error_types'][$errorType] ?? 0) + 1;

            // Log error but continue
            error_log("Error: {$errorType} - {$e->getMessage()}");
        }
    }

    private function executeStringOp(): void
    {
        $key = "soak:string:" . rand(1, 10000);
        $cmd = rand(1, 3);

        switch ($cmd) {
            case 1: // SET
                $this->client->set($key, "value_" . time());
                $this->incrementCommandCount('SET');
                break;
            case 2: // GET
                $this->client->get($key);
                $this->incrementCommandCount('GET');
                break;
            case 3: // INCR
                $this->client->incr("soak:counter:" . rand(1, 100));
                $this->incrementCommandCount('INCR');
                break;
        }
    }

    private function executeHashOp(): void
    {
        $key = "soak:hash:" . rand(1, 5000);
        $cmd = rand(1, 3);

        switch ($cmd) {
            case 1: // HSET
                $this->client->hset($key, "field_" . rand(1, 10), "value");
                $this->incrementCommandCount('HSET');
                break;
            case 2: // HGET
                $this->client->hget($key, "field_" . rand(1, 10));
                $this->incrementCommandCount('HGET');
                break;
            case 3: // HGETALL
                $this->client->hgetall($key);
                $this->incrementCommandCount('HGETALL');
                break;
        }
    }

    private function executeListOp(): void
    {
        $key = "soak:list:" . rand(1, 3000);
        $cmd = rand(1, 3);

        switch ($cmd) {
            case 1: // LPUSH
                $this->client->lpush($key, ["item_" . time()]);
                $this->incrementCommandCount('LPUSH');
                break;
            case 2: // RPOP
                $this->client->rpop($key);
                $this->incrementCommandCount('RPOP');
                break;
            case 3: // LRANGE
                $this->client->lrange($key, 0, 10);
                $this->incrementCommandCount('LRANGE');
                break;
        }
    }

    private function executeSetOp(): void
    {
        $key = "soak:set:" . rand(1, 2000);
        $cmd = rand(1, 2);

        switch ($cmd) {
            case 1: // SADD
                $this->client->sadd($key, "member_" . rand(1, 100));
                $this->incrementCommandCount('SADD');
                break;
            case 2: // SMEMBERS
                $this->client->smembers($key);
                $this->incrementCommandCount('SMEMBERS');
                break;
        }
    }

    private function executeZSetOp(): void
    {
        $key = "soak:zset:" . rand(1, 2000);
        $cmd = rand(1, 2);

        switch ($cmd) {
            case 1: // ZADD
                $this->client->zadd($key, ["member_" . rand(1, 100) => (float)rand(1, 1000)]);
                $this->incrementCommandCount('ZADD');
                break;
            case 2: // ZRANGE
                $this->client->zrange($key, 0, 10);
                $this->incrementCommandCount('ZRANGE');
                break;
        }
    }

    private function executeKeyMgmt(): void
    {
        $key = "soak:temp:" . rand(1, 1000);
        $cmd = rand(1, 3);

        switch ($cmd) {
            case 1: // DEL
                $this->client->del([$key]);
                $this->incrementCommandCount('DEL');
                break;
            case 2: // EXISTS
                $this->client->exists([$key]);
                $this->incrementCommandCount('EXISTS');
                break;
            case 3: // EXPIRE
                $this->client->expire($key, 3600);
                $this->incrementCommandCount('EXPIRE');
                break;
        }
    }

    private function selectCommandCategory(): string
    {
        $rand = rand(1, 100);
        $cumulative = 0;

        foreach (COMMAND_WEIGHTS as $category => $weight) {
            $cumulative += $weight;
            if ($rand <= $cumulative) {
                return $category;
            }
        }

        return 'string_ops'; // Fallback
    }

    private function incrementCommandCount(string $command): void
    {
        $this->stats['command_counts'][$command] =
            ($this->stats['command_counts'][$command] ?? 0) + 1;
    }

    private function prePopulate(): void
    {
        echo "Pre-populating keys for consistent cache hit rates...\n";

        // String keys
        for ($i = 1; $i <= 10000; $i++) {
            $this->client->set("soak:string:{$i}", "value_" . time());
        }

        // Hash keys
        for ($i = 1; $i <= 5000; $i++) {
            for ($j = 1; $j <= 10; $j++) {
                $this->client->hset("soak:hash:{$i}", "field_{$j}", "value");
            }
        }

        // List keys
        for ($i = 1; $i <= 3000; $i++) {
            $this->client->lpush("soak:list:{$i}", ["item_1", "item_2", "item_3"]);
        }

        // Set keys
        for ($i = 1; $i <= 2000; $i++) {
            for ($j = 1; $j <= 10; $j++) {
                $this->client->sadd("soak:set:{$i}", "member_{$j}");
            }
        }

        // Sorted set keys
        for ($i = 1; $i <= 2000; $i++) {
            $members = [];
            for ($j = 1; $j <= 10; $j++) {
                $members["member_{$j}"] = (float)$j;
            }
            $this->client->zadd("soak:zset:{$i}", $members);
        }

        echo "Pre-population complete.\n\n";
    }

    private function checkMemory(): void
    {
        $memoryMB = memory_get_usage(true) / 1024 / 1024;

        // Alert if memory exceeds threshold (e.g., 500MB)
        if ($memoryMB > 500) {
            error_log("WARNING: High memory usage: {$memoryMB} MB");
        }
    }

    /**
     * Calculate CPU usage since test start using getrusage()
     *
     * getrusage() returns resource usage statistics from the OS:
     * - ru_utime.tv_sec/tv_usec: User CPU time (application code execution)
     * - ru_stime.tv_sec/tv_usec: System CPU time (kernel operations, I/O, syscalls)
     *
     * Time is split into seconds (tv_sec) and microseconds (tv_usec) for precision.
     *
     * @return array ['user' => float, 'system' => float, 'total' => float] in seconds
     */
    private function getCpuUsage(): array
    {
        $current = getrusage();

        $userTime = ($current['ru_utime.tv_sec'] - $this->startRusage['ru_utime.tv_sec']) +
                    (($current['ru_utime.tv_usec'] - $this->startRusage['ru_utime.tv_usec']) / 1000000);

        $systemTime = ($current['ru_stime.tv_sec'] - $this->startRusage['ru_stime.tv_sec']) +
                      (($current['ru_stime.tv_usec'] - $this->startRusage['ru_stime.tv_usec']) / 1000000);

        return [
            'user' => $userTime,
            'system' => $systemTime,
            'total' => $userTime + $systemTime
        ];
    }

    private function reportProgress(): void
    {
        $elapsed = max(1, time() - $this->startTime);  // Avoid division by zero
        $hours = floor($elapsed / 3600);
        $minutes = floor(($elapsed % 3600) / 60);

        $opsPerSec = $this->stats['total_operations'] / $elapsed;
        $errorRate = $this->stats['errors'] / max($this->stats['total_operations'], 1) * 100;
        $memoryMB = memory_get_usage(true) / 1024 / 1024;
        $cpu = $this->getCpuUsage();
        $cpuPercent = ($cpu['total'] / $elapsed) * 100;  // CPU time / wall clock time = utilization %

        echo "\n=== Progress Report ===\n";
        echo "Elapsed: {$hours}h {$minutes}m\n";
        echo "Total Operations: " . number_format($this->stats['total_operations']) . "\n";
        echo "Operations/sec: " . number_format($opsPerSec, 2) . "\n";
        echo "Errors: {$this->stats['errors']} (" . number_format($errorRate, 2) . "%)\n";
        echo "Memory: " . number_format($memoryMB, 2) . " MB\n";
        echo "CPU: " . number_format($cpuPercent, 2) . "% (User: " . number_format($cpu['user'], 2) . "s, System: " . number_format($cpu['system'], 2) . "s)\n";

        echo "\nCommand Distribution:\n";
        arsort($this->stats['command_counts']);
        foreach ($this->stats['command_counts'] as $cmd => $count) {
            $pct = ($count / max($this->stats['total_operations'], 1)) * 100;
            echo "  {$cmd}: " . number_format($count) . " (" . number_format($pct, 2) . "%)\n";
        }

        echo "======================\n\n";
    }

    private function finalReport(): void
    {
        echo "\n\n=== FINAL REPORT ===\n";
        $this->reportProgress();

        echo "\nCommand Distribution:\n";
        arsort($this->stats['command_counts']);
        foreach ($this->stats['command_counts'] as $cmd => $count) {
            $pct = ($count / $this->stats['total_operations']) * 100;
            echo "  {$cmd}: " . number_format($count) . " (" . number_format($pct, 2) . "%)\n";
        }

        if (!empty($this->stats['error_types'])) {
            echo "\nError Types:\n";
            foreach ($this->stats['error_types'] as $type => $count) {
                echo "  {$type}: {$count}\n";
            }
        }

        echo "\n===================\n";
    }

    public function __destruct()
    {
        $this->client->close();
    }
}

// Parse command-line arguments
function parseArguments(): array
{
    $options = getopt('', [
        'duration::',
        'cluster',
        'host::',
        'port::',
    ]);

    return [
        'duration' => (float)($options['duration'] ?? TEST_DURATION_HOURS),
        'cluster' => isset($options['cluster']),
        'host' => $options['host'] ?? 'localhost',
        'port' => (int)($options['port'] ?? 6379),
    ];
}

// Main execution
$args = parseArguments();

echo "Valkey GLIDE PHP Soak Test\n";
echo "===========================\n";
echo "Duration: " . number_format($args['duration'], 2) . " hours\n";
echo "Mode: " . ($args['cluster'] ? 'Cluster' : 'Standalone') . "\n";
echo "Server: {$args['host']}:{$args['port']}\n";
echo "===========================\n\n";

$test = new SoakTest($args['cluster'], $args['host'], $args['port']);
$test->run($args['duration']);
