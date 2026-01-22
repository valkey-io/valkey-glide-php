<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

require_once __DIR__ . '/utils.php';

enum ClientType: string
{
    case GLIDE = 'glide';
    case PHPREDIS = 'phpredis';
    case ALL = 'all';
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
        $latencyMs = ($end - $start) / 1_000_000;  // Convert nanoseconds to milliseconds
        $actionLatencies[$action->value][] = $latencyMs;  // Store full precision for accurate statistics
        
        // Log progress every 100,000 iterations
        if ($startedOperationsCounter - $lastLoggedAt >= 100_000) {
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
    $timeSeconds = ($end - $start) / 1_000_000_000;
    
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
    if ($clientType === ClientType::GLIDE) {
        // Disable certificate validation for benchmarking (self-signed certs, local testing)
        $advancedConfig = $useTls ? ['tls_config' => ['use_insecure_tls' => true]] : null;
        $client = $isCluster
            ? new ValkeyGlideCluster(
                addresses: [['host' => $host, 'port' => $port]], 
                use_tls: $useTls,
                advanced_config: $advancedConfig
            )
            : new ValkeyGlide(
                addresses: [['host' => $host, 'port' => $port]], 
                use_tls: $useTls,
                advanced_config: $advancedConfig
            );
    } else {
        if ($isCluster) {
            $client = new RedisCluster(null, ["{$host}:{$port}"]);
            if ($useTls) {
                // Disable certificate validation for benchmarking (self-signed certs, local testing)
                $client->setOption(Redis::OPT_SSL_CONTEXT, ['verify_peer' => false]);
            }
        } else {
            $client = new Redis();
            if ($useTls) {
                $client->connect($host, $port);
                // Disable certificate validation for benchmarking (self-signed certs, local testing)
                $client->setOption(Redis::OPT_SSL_CONTEXT, ['verify_peer' => false]);
            } else {
                $client->connect($host, $port);
            }
        }
    }
    
    return runBenchmarkProcess($client, $totalCommands, $dataSize);
}

function runClient(
    ClientType $clientType,
    int $totalCommands,
    int $dataSize,
    bool $isCluster,
    string $host,
    int $port,
    bool $useTls
): array {
    $clientName = $clientType->value;
    $now = date('H:i:s');
    echo "Starting {$clientName} | data size: {$dataSize} bytes | iterations: {$totalCommands} | {$now}\n";
    
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
            $useTls
        );
        $benchResults[] = $result;
    }
    
    // Run ValkeyGlide benchmark
    if ($clientsToRun === ClientType::ALL || $clientsToRun === ClientType::GLIDE) {
        $result = runClient(
            ClientType::GLIDE,
            $totalCommands,
            $dataSize,
            $isCluster,
            $host,
            $port,
            $useTls
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
