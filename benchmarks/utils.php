<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

const PORT = 6379;
const PROB_GET = 0.8;
const PROB_GET_EXISTING_KEY = 0.8;
const SIZE_GET_KEYSPACE = 3750000; // 3.75 million
const SIZE_SET_KEYSPACE = 3000000; // 3 million

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
        'clientCount::',
        'tls',
        'clusterModeEnabled',
        'port::',
        'minimal',
        'concurrentTasks::',
    ]);

    return [
        'resultsFile' => $options['resultsFile'] ?? __DIR__ . '/../results/php-results.json',
        'dataSize' => (int)($options['dataSize'] ?? 100),
        'clients' => $options['clients'] ?? 'all',
        'host' => $options['host'] ?? 'localhost',
        'clientCount' => isset($options['clientCount']) ? explode(',', $options['clientCount']) : ['1'],
        'tls' => isset($options['tls']),
        'clusterModeEnabled' => isset($options['clusterModeEnabled']),
        'port' => (int)($options['port'] ?? PORT),
        'minimal' => isset($options['minimal']),
        'concurrentTasks' => isset($options['concurrentTasks']) ? explode(',', $options['concurrentTasks']) : ['1', '10', '100', '1000'],
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
    if ((mt_rand() / mt_getrandmax()) > PROB_GET) {
        return ChosenAction::SET;
    }
    if ((mt_rand() / mt_getrandmax()) > PROB_GET_EXISTING_KEY) {
        return ChosenAction::GET_NON_EXISTING;
    }
    return ChosenAction::GET_EXISTING;
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

function numberOfIterations(int $numOfConcurrentTasks): int
{
    return min(max(100000, $numOfConcurrentTasks * 10000), 5000000);
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
    
    // Group by client
    $groupedResults = [];
    foreach ($benchResults as $result) {
        $client = $result['client'];
        if (!isset($groupedResults[$client])) {
            $groupedResults[$client] = [];
        }
        $groupedResults[$client][] = $result;
    }
    
    foreach ($groupedResults as $client => $results) {
        $md .= "## Client: {$client}\n\n";
        
        // Summary table
        $md .= "### Performance Summary\n\n";
        $md .= "| Concurrency | Data Size | TPS     | Client Count | Cluster |\n";
        $md .= "|-------------|-----------|---------|--------------|----------|\n";
        
        foreach ($results as $result) {
            $cluster = $result['is_cluster'] ? 'Yes' : 'No';
            $md .= sprintf(
                "| %-11d | %-9d | %-7s | %-12d | %-8s |\n",
                $result['num_of_tasks'],
                $result['data_size'],
                number_format($result['tps']),
                $result['client_count'],
                $cluster
            );
        }
        
        $md .= "\n### Latency Details\n\n";
        
        foreach ($results as $result) {
            $md .= "#### Concurrency: {$result['num_of_tasks']}, Data Size: {$result['data_size']} bytes\n\n";
            
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
    }
    
    return $md;
}
