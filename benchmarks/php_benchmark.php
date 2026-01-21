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

$args = parseArguments();
$benchResults = [];

function runBenchmarkOperations(
    object $client,
    int $totalCommands,
    int $dataSize,
    array &$actionLatencies,
    int &$startedOperationsCounter
): void {
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
        if ($startedOperationsCounter - $lastLoggedAt >= 100000) {
            $progress = round(($startedOperationsCounter / $totalCommands) * 100, 1);
            echo "  Progress: {$startedOperationsCounter}/{$totalCommands} ({$progress}%)\n";
            $lastLoggedAt = $startedOperationsCounter;
        }
    }
}

function runBenchmarkSingleProcess(
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
    $start = hrtime(true);
    
    runBenchmarkOperations($client, $totalCommands, $dataSize, $actionLatencies, $startedOperationsCounter);
    
    $end = hrtime(true);
    $timeSeconds = ($end - $start) / 1_000_000_000;
    
    return [
        'time' => $timeSeconds,
        'latencies' => $actionLatencies,
        'operations_completed' => $startedOperationsCounter,
    ];
}

function runBenchmarkSequential(
    int $totalCommands,
    int $iterationMultiplier,
    int $dataSize,
    string $host,
    int $port,
    bool $useTls,
    bool $isCluster,
    ClientType $clientType
): array {
    // Note: PHP benchmark runs sequentially due to ValkeyGlide's Tokio runtime limitation
    // (pcntl_fork() is incompatible with async Rust runtimes)
    echo "  Running sequential benchmark...\n";
    
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
    
    return runBenchmarkSingleProcess($client, $totalCommands, $dataSize);
}

function runClient(
    object $client,
    ClientType $clientType,
    int $totalCommands,
    int $iterationMultiplier,
    int $dataSize,
    bool $isCluster,
    string $host,
    int $port,
    bool $useTls
): array {
    $clientName = $clientType->value;
    $now = date('H:i:s');
    echo "Starting {$clientName} data size: {$dataSize} iteration level: {$iterationMultiplier} {$now}\n";
    
    // Run sequential benchmark (iteration level > 1 uses more iterations)
    if ($iterationMultiplier > 1) {
        $result = runBenchmarkSequential(
            $totalCommands,
            $iterationMultiplier,
            $dataSize,
            $host,
            $port,
            $useTls,
            $isCluster,
            $clientType
        );
    } else {
        $result = runBenchmarkSingleProcess($client, $totalCommands, $dataSize);
    }
    
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
            'num_of_tasks' => $iterationMultiplier,  // Note: iteration level, not actual concurrency (PHP runs sequentially)
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
    int $iterationMultiplier,
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
        
        $result = runClient(
            $client,
            ClientType::PHPREDIS,
            $totalCommands,
            $iterationMultiplier,
            $dataSize,
            $isCluster,
            $host,
            $port,
            $useTls
        );
        $benchResults[] = $result;
        $client->close();
    }
    
    // Run ValkeyGlide benchmark
    if ($clientsToRun === ClientType::ALL || $clientsToRun === ClientType::GLIDE) {
        // Disable certificate validation for benchmarking (self-signed certs, local testing)
        $advancedConfig = $useTls ? ['tls_config' => ['use_insecure_tls' => true]] : null;
        if ($isCluster) {
            $client = new ValkeyGlideCluster(
                addresses: [['host' => $host, 'port' => $port]],
                use_tls: $useTls,
                advanced_config: $advancedConfig
            );
        } else {
            $client = new ValkeyGlide(
                addresses: [['host' => $host, 'port' => $port]],
                use_tls: $useTls,
                advanced_config: $advancedConfig
            );
        }
        
        $result = runClient(
            $client,
            ClientType::GLIDE,
            $totalCommands,
            $iterationMultiplier,
            $dataSize,
            $isCluster,
            $host,
            $port,
            $useTls
        );
        $benchResults[] = $result;
        $client->close();
    }
}

// Main execution
$iterationLevels = $args['iterationLevel'];
$dataSize = $args['dataSize'];
$clientsToRun = ClientType::from($args['clients']);
$host = $args['host'];
$useTls = $args['tls'];
$port = $args['port'];
$isCluster = $args['clusterModeEnabled'];

$productOfArguments = [];
foreach ($iterationLevels as $iterationLevel) {
    $iterationLevel = (int)$iterationLevel;
    $productOfArguments[] = [$dataSize, $iterationLevel];
}

foreach ($productOfArguments as [$dataSize, $iterationMultiplier]) {
    $totalIterations = numberOfIterations($iterationMultiplier);
    
    main(
        $totalIterations,
        $iterationMultiplier,
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
