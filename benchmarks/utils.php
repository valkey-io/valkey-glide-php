<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

declare(strict_types=1);

namespace ValkeyGlide\Benchmarks;

const DEFAULT_PORT = 6379;
const DEFAULT_HOST = 'localhost';
const PROB_GET_EXISTING = 0.64;
const PROB_GET_NON_EXISTING = 0.16;
const PROB_GET_TOTAL = PROB_GET_EXISTING + PROB_GET_NON_EXISTING;
const SIZE_GET_KEYSPACE = 3_750_000; // 3.75 million
const SIZE_SET_KEYSPACE = 3_000_000; // 3 million

// Iteration scaling constants
const MIN_ITERATIONS = 100_000;      // Minimum iterations for statistical significance
const MAX_ITERATIONS = 5_000_000;     // Maximum iterations cap

// Default benchmark configuration
const DEFAULT_DATA_SIZE = 100;           // Default value size in bytes
const DEFAULT_ITERATIONS = ['100_000', '1_000_000', '5_000_000'];

enum ChosenAction: int
{
    case GET_NON_EXISTING = 1;
    case GET_EXISTING = 2;
    case SET = 3;
}

function parseArguments(): array
{
    $options = getopt('', [
        'resultsFile::',
        'dataSize::',
        'clients::',
        'host::',

        'tls',
        'clusterModeEnabled',
        'port::',
        'iterations::',
    ]);

    return [
        'resultsFile' => $options['resultsFile'] ?? __DIR__ . '/../results/php-results.json',
        'dataSize' => (int)($options['dataSize'] ?? DEFAULT_DATA_SIZE),
        'clients' => $options['clients'] ?? 'all',
        'host' => $options['host'] ?? DEFAULT_HOST,
        'tls' => isset($options['tls']),
        'clusterModeEnabled' => isset($options['clusterModeEnabled']),
        'port' => (int)($options['port'] ?? DEFAULT_PORT),
        'iterations' => isset($options['iterations']) ? explode(',', $options['iterations']) : DEFAULT_ITERATIONS,
    ];
}

function generateValue(int $size): string
{
    return str_repeat('0', $size);
}

function generateKeySet(): string
{
    return (string)random_int(1, SIZE_SET_KEYSPACE);
}

function generateKeyGet(): string
{
    return (string)random_int(SIZE_SET_KEYSPACE + 1, SIZE_GET_KEYSPACE);
}

function chooseAction(): ChosenAction
{
    $random = mt_rand() / mt_getrandmax();

    if ($random > PROB_GET_TOTAL) {
        return ChosenAction::SET;
    } elseif ($random > PROB_GET_EXISTING) {
        return ChosenAction::GET_NON_EXISTING;
    } else {
        return ChosenAction::GET_EXISTING;
    }
}

function calculatePercentile(array $values, float $percentile): float
{
    if (empty($values)) {
        return 0.0;
    }
    sort($values);
    $index = (int)ceil(($percentile / 100) * count($values)) - 1;
    $index = max(0, min($index, count($values) - 1));
    return round($values[$index], 4);
}

function calculateStdDev(array $values): float
{
    if (empty($values)) {
        return 0.0;
    }
    $mean = array_sum($values) / count($values);
    $variance = array_sum(array_map(fn($x) => ($x - $mean) ** 2, $values)) / count($values);
    return round(sqrt($variance), 3);
}

function latencyResults(string $prefix, array $latencies): array
{
    if (empty($latencies)) {
        return [
            "{$prefix}_p50_latency" => 0,
            "{$prefix}_p90_latency" => 0,
            "{$prefix}_p99_latency" => 0,
            "{$prefix}_average_latency" => 0,
            "{$prefix}_std_dev" => 0,
        ];
    }

    return [
        "{$prefix}_p50_latency" => calculatePercentile($latencies, 50),
        "{$prefix}_p90_latency" => calculatePercentile($latencies, 90),
        "{$prefix}_p99_latency" => calculatePercentile($latencies, 99),
        "{$prefix}_average_latency" => round(array_sum($latencies) / count($latencies), 3),
        "{$prefix}_std_dev" => calculateStdDev($latencies),
    ];
}

function validateIterations(int $iterations): int
{
    if ($iterations < MIN_ITERATIONS) {
        echo "Error: Iterations must be at least " . number_format(MIN_ITERATIONS) . "\n";
        exit(1);
    }

    if ($iterations > MAX_ITERATIONS) {
        echo "Error: Iterations must not exceed " . number_format(MAX_ITERATIONS) . "\n";
        exit(1);
    }

    return $iterations;
}

function processResults(array $benchResults, string $resultsFile): void
{
    // Write JSON results
    file_put_contents($resultsFile, json_encode($benchResults, JSON_PRETTY_PRINT));

    // Write Markdown results
    $mdFile = str_replace('.json', '.md', $resultsFile);
    $markdown = generateMarkdownReport($benchResults);
    file_put_contents($mdFile, $markdown);
}

function generateMarkdownReport(array $benchResults): string
{
    $md = "# PHP Benchmark Results\n\n";
    $md .= "Generated: " . date('Y-m-d H:i:s') . "\n\n";

    // Group by concurrency level
    $groupedByConcurrency = [];
    foreach ($benchResults as $result) {
        $concurrency = $result['num_of_tasks'];
        if (!isset($groupedByConcurrency[$concurrency])) {
            $groupedByConcurrency[$concurrency] = [];
        }
        $groupedByConcurrency[$concurrency][] = $result;
    }

    ksort($groupedByConcurrency);

    foreach ($groupedByConcurrency as $concurrency => $results) {
        $md .= "## Concurrency: {$concurrency}\n\n";

        // Performance comparison table
        $md .= "### Performance Comparison\n\n";
        $md .= "| Client    | TPS     | GET Existing P50 | GET Non-Existing P50 | SET P50 |\n";
        $md .= "|-----------|---------|------------------|----------------------|---------|\n";

        foreach ($results as $result) {
            $md .= sprintf(
                "| %-9s | %-7s | %-16s | %-20s | %-7s |\n",
                $result['client'],
                str_pad(number_format($result['tps']), 7, ' ', STR_PAD_LEFT),
                str_pad($result['get_existing_p50_latency'] . 'ms', 16, ' ', STR_PAD_LEFT),
                str_pad($result['get_non_existing_p50_latency'] . 'ms', 20, ' ', STR_PAD_LEFT),
                str_pad($result['set_p50_latency'] . 'ms', 7, ' ', STR_PAD_LEFT)
            );
        }

        $md .= "\n### Detailed Latency Metrics\n\n";

        foreach ($results as $result) {
            $md .= "#### Client: {$result['client']}\n\n";

            // GET Existing
            $md .= "**GET (Existing Key)**\n\n";
            $md .= "| Metric  | Value (ms) |\n";
            $md .= "|---------|------------|\n";
            $md .= sprintf("| %-7s | %-10s |\n", "P50", $result['get_existing_p50_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P90", $result['get_existing_p90_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P99", $result['get_existing_p99_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "Average", $result['get_existing_average_latency']);
            $md .= sprintf("| %-7s | %-10s |\n\n", "Std Dev", $result['get_existing_std_dev']);

            // GET Non-Existing
            $md .= "**GET (Non-Existing Key)**\n\n";
            $md .= "| Metric  | Value (ms) |\n";
            $md .= "|---------|------------|\n";
            $md .= sprintf("| %-7s | %-10s |\n", "P50", $result['get_non_existing_p50_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P90", $result['get_non_existing_p90_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P99", $result['get_non_existing_p99_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "Average", $result['get_non_existing_average_latency']);
            $md .= sprintf("| %-7s | %-10s |\n\n", "Std Dev", $result['get_non_existing_std_dev']);

            // SET
            $md .= "**SET**\n\n";
            $md .= "| Metric  | Value (ms) |\n";
            $md .= "|---------|------------|\n";
            $md .= sprintf("| %-7s | %-10s |\n", "P50", $result['set_p50_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P90", $result['set_p90_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "P99", $result['set_p99_latency']);
            $md .= sprintf("| %-7s | %-10s |\n", "Average", $result['set_average_latency']);
            $md .= sprintf("| %-7s | %-10s |\n\n", "Std Dev", $result['set_std_dev']);
        }

        $md .= "---\n\n";
    }

    return $md;
}
