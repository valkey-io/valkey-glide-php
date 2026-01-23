<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

declare(strict_types=1);

namespace ValkeyGlide\Benchmarks;

// phpcs:disable PSR1.Files.SideEffects
require_once __DIR__ . '/utils.php';

use ValkeyGlide;
use ValkeyGlideCluster;
use Redis;
use RedisCluster;

// Progress reporting constants
const PROGRESS_REPORT_INTERVAL = 100_000;  // Report progress every N operations

// Time conversion constants
const NANOSECONDS_TO_MILLISECONDS = 1_000_000;
const NANOSECONDS_TO_SECONDS = 1_000_000_000;

function convertNanosecsToSecs(int $nanoseconds): float
{
    return $nanoseconds / NANOSECONDS_TO_SECONDS;
}

function convertNanosecsToMillisecs(int $nanoseconds): float
{
    return $nanoseconds / NANOSECONDS_TO_MILLISECONDS;
}

enum ClientType: string
{
    case GLIDE = 'glide';
    case PHPREDIS = 'phpredis';
    case ALL = 'all';
}

function prePopulateDatabase(object $client, int $dataSize): void
{
    echo "Pre-populating database with " . number_format(SIZE_SET_KEYSPACE) . " keys...\n";
    echo "  (This may take several minutes on first run)\n";
    $value = generateValue($dataSize);
    $start = hrtime(true);

    for ($i = 1; $i <= SIZE_SET_KEYSPACE; $i++) {
        $client->set((string)$i, $value);

        if ($i % PROGRESS_REPORT_INTERVAL === 0) {
            $elapsed = convertNanosecsToSecs(hrtime(true) - $start);
            $rate = (int)($i / $elapsed);
            echo "  Progress: " . number_format($i) . "/" . number_format(SIZE_SET_KEYSPACE) .
                 " (" . number_format($rate) . " keys/sec)\n";
        }
    }

    $elapsed = convertNanosecsToSecs(hrtime(true) - $start);
    echo "Pre-population completed in " . round($elapsed, 2) . " seconds\n\n";
}

function createClient(
    ClientType $clientType,
    string $host,
    int $port,
    bool $useTls,
    bool $isCluster
): object {
    if ($clientType === ClientType::GLIDE) {
        $advancedConfig = $useTls ? ['tls_config' => ['use_insecure_tls' => true]] : null;
        if ($isCluster) {
            return new ValkeyGlideCluster(
                addresses: [['host' => $host, 'port' => $port]],
                use_tls: $useTls,
                advanced_config: $advancedConfig
            );
        } else {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [['host' => $host, 'port' => $port]],
                use_tls: $useTls,
                advanced_config: $advancedConfig
            );
            return $client;
        }
    } else {
        if ($isCluster) {
            $client = new RedisCluster(null, ["{$host}:{$port}"]);
            if ($useTls) {
                $client->setOption(Redis::OPT_SSL_CONTEXT, ['verify_peer' => false]);
            }
            return $client;
        } else {
            $client = new Redis();
            if ($useTls) {
                $client->connect($host, $port);
                $client->setOption(Redis::OPT_SSL_CONTEXT, ['verify_peer' => false]);
            } else {
                $client->connect($host, $port);
            }
            return $client;
        }
    }
}

function runBenchmarkOperations(
    object $client,
    int $totalCommands,
    int $dataSize
): array {
    $actionLatencies = [
        ChosenAction::GET_NON_EXISTING->value => [],
        ChosenAction::GET_EXISTING->value => [],
        ChosenAction::SET->value => [],
    ];
    $startedOperationsCounter = 0;
    $value = generateValue($dataSize);
    $lastLoggedAt = 0;

    while ($startedOperationsCounter < $totalCommands) {
        $startedOperationsCounter++;
        $action = chooseAction();

        $start = hrtime(true);

        switch ($action) {
            case ChosenAction::GET_EXISTING:
                $client->get(generateKeySet());
                break;
            case ChosenAction::GET_NON_EXISTING:
                $client->get(generateKeyGet());
                break;
            case ChosenAction::SET:
                $client->set(generateKeySet(), $value);
                break;
        }

        $end = hrtime(true);
        $latencyMs = convertNanosecsToMillisecs($end - $start);  // Convert nanoseconds to milliseconds
        $actionLatencies[$action->value][] = $latencyMs;  // Store full precision for accurate statistics

        // Log progress every 100,000 iterations
        if ($startedOperationsCounter - $lastLoggedAt >= PROGRESS_REPORT_INTERVAL) {
            $progress = round(($startedOperationsCounter / $totalCommands) * 100, 1);
            echo "  Progress: {$startedOperationsCounter}/{$totalCommands} ({$progress}%)\n";
            $lastLoggedAt = $startedOperationsCounter;
        }
    }

    return [
        'latencies' => $actionLatencies,
        'operations_completed' => $startedOperationsCounter,
    ];
}

function runBenchmarkProcess(
    object $client,
    int $totalCommands,
    int $dataSize
): array {
    $start = hrtime(true);

    $result = runBenchmarkOperations($client, $totalCommands, $dataSize);

    $end = hrtime(true);
    $timeSeconds = convertNanosecsToSecs($end - $start);

    return [
        'time' => $timeSeconds,
        'latencies' => $result['latencies'],
        'operations_completed' => $result['operations_completed'],
    ];
}

function runBenchmarkSequential(
    int $totalCommands,
    int $dataSize,
    string $host,
    int $port,
    bool $useTls,
    bool $isCluster,
    ClientType $clientType
): array {
    $client = createClient($clientType, $host, $port, $useTls, $isCluster);
    $result = runBenchmarkProcess($client, $totalCommands, $dataSize);
    $client->close();

    return $result;
}

function runClient(
    ClientType $clientType,
    int $totalCommands,
    int $dataSize,
    bool $isCluster,
    string $host,
    int $port,
    bool $useTls,
    bool $skipPrePopulation = false
): array {
    $clientName = $clientType->value;
    $now = date('H:i:s');
    echo "Starting {$clientName} | data size: {$dataSize} bytes | iterations: {$totalCommands} | {$now}\n";

    if (!$skipPrePopulation) {
        $tempClient = createClient($clientType, $host, $port, $useTls, $isCluster);
        prePopulateDatabase($tempClient, $dataSize);
        $tempClient->close();
    }

    $result = runBenchmarkSequential(
        $totalCommands,
        $dataSize,
        $host,
        $port,
        $useTls,
        $isCluster,
        $clientType
    );

    $time = $result['time'];
    $actionLatencies = $result['latencies'];
    $operationsCompleted = $result['operations_completed'];

    $tps = (int)($operationsCompleted / $time);

    $getNonExistingResults = latencyResults('get_non_existing', $actionLatencies[ChosenAction::GET_NON_EXISTING->value]);
    $getExistingResults = latencyResults('get_existing', $actionLatencies[ChosenAction::GET_EXISTING->value]);
    $setResults = latencyResults('set', $actionLatencies[ChosenAction::SET->value]);

    return array_merge(
        [
            'client' => $clientName,
            'num_of_tasks' => 1,  // PHP runs sequentially (no concurrency)
            'data_size' => $dataSize,
            'tps' => $tps,
            'client_count' => 1,
            'is_cluster' => $isCluster,
        ],
        $getExistingResults,
        $getNonExistingResults,
        $setResults
    );
}

function main(
    int $totalCommands,
    int $dataSize,
    ClientType $clientsToRun,
    string $host,
    bool $useTls,
    bool $isCluster,
    int $port,
    array &$benchResults
): void {
    // Run phpredis benchmark
    if ($clientsToRun === ClientType::ALL || $clientsToRun === ClientType::PHPREDIS) {
        $result = runClient(
            ClientType::PHPREDIS,
            $totalCommands,
            $dataSize,
            $isCluster,
            $host,
            $port,
            $useTls,
            false  // skipPrePopulation = false (DO pre-populate)
        );
        $benchResults[] = $result;
    }

    // Run ValkeyGlide benchmark
    if ($clientsToRun === ClientType::ALL || $clientsToRun === ClientType::GLIDE) {
        $skipPrePopulation = $clientsToRun === ClientType::ALL;  // Skip if phpredis already pre-populated
        $result = runClient(
            ClientType::GLIDE,
            $totalCommands,
            $dataSize,
            $isCluster,
            $host,
            $port,
            $useTls,
            $skipPrePopulation
        );
        $benchResults[] = $result;
    }
}

// Main execution
$args = parseArguments();
$benchResults = [];

$iterationsList = $args['iterations'];
$dataSize = $args['dataSize'];
$clientsToRun = ClientType::tryFrom($args['clients']);
if ($clientsToRun === null) {
    echo "Error: Invalid client type '{$args['clients']}'. Valid options: all, glide, phpredis\n";
    exit(1);
}
$host = $args['host'];
$useTls = $args['tls'];
$port = $args['port'];
$isCluster = $args['clusterModeEnabled'];

$productOfArguments = [];
foreach ($iterationsList as $iterations) {
    $iterations = validateIterations((int)$iterations);
    $productOfArguments[] = [$dataSize, $iterations];
}

foreach ($productOfArguments as [$dataSize, $totalIterations]) {
    main(
        $totalIterations,
        $dataSize,
        $clientsToRun,
        $host,
        $useTls,
        $isCluster,
        $port,
        $benchResults
    );
}

processResults($benchResults, $args['resultsFile']);
$mdFile = str_replace('.json', '.md', $args['resultsFile']);
echo "Results written to {$args['resultsFile']}\n";
echo "Markdown report written to {$mdFile}\n";
