<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

require_once __DIR__ . '/utils.php';

$args = parseArguments();
$benchResults = [];
$startedTasksCounter = 0;

function executeCommands(
    object $client,
    int $totalCommands,
    int $dataSize,
    array &$actionLatencies,
    int &$startedTasksCounter
): void {
    $value = generateValue($dataSize);
    $lastLoggedAt = 0;
    
    while ($startedTasksCounter < $totalCommands) {
        $startedTasksCounter++;
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
        $latencyMs = ($end - $start) / 1_000_000;
        $actionLatencies[$action->value][] = round($latencyMs, 3);
        
        // Log progress every 100,000 iterations
        if ($startedTasksCounter - $lastLoggedAt >= 100000) {
            $progress = round(($startedTasksCounter / $totalCommands) * 100, 1);
            echo "  Progress: {$startedTasksCounter}/{$totalCommands} ({$progress}%)\n";
            $lastLoggedAt = $startedTasksCounter;
        }
    }
}

function runBenchmark(
    object $client,
    int $totalCommands,
    int $dataSize,
    int &$startedTasksCounter
): array {
    $actionLatencies = [
        ChosenAction::GET_NON_EXISTING->value => [],
        ChosenAction::GET_EXISTING->value => [],
        ChosenAction::SET->value => [],
    ];
    
    $startedTasksCounter = 0;
    $start = hrtime(true);
    
    executeCommands($client, $totalCommands, $dataSize, $actionLatencies, $startedTasksCounter);
    
    $end = hrtime(true);
    $timeSeconds = ($end - $start) / 1_000_000_000;
    
    return [
        'time' => $timeSeconds,
        'latencies' => $actionLatencies,
    ];
}

function runClients(
    array $clients,
    string $clientName,
    int $totalCommands,
    int $numOfConcurrentTasks,
    int $dataSize,
    bool $isCluster,
    int &$startedTasksCounter
): array {
    $now = date('H:i:s');
    echo "Starting {$clientName} data size: {$dataSize} concurrency: {$numOfConcurrentTasks} " .
         "client count: " . count($clients) . " {$now}\n";
    
    $result = runBenchmark($clients[0], $totalCommands, $dataSize, $startedTasksCounter);
    $time = $result['time'];
    $actionLatencies = $result['latencies'];
    
    $tps = (int)($startedTasksCounter / $time);
    
    $getNonExistingResults = latencyResults('get_non_existing', $actionLatencies[ChosenAction::GET_NON_EXISTING->value]);
    $getExistingResults = latencyResults('get_existing', $actionLatencies[ChosenAction::GET_EXISTING->value]);
    $setResults = latencyResults('set', $actionLatencies[ChosenAction::SET->value]);
    
    return array_merge(
        [
            'client' => $clientName,
            'num_of_tasks' => $numOfConcurrentTasks,
            'data_size' => $dataSize,
            'tps' => $tps,
            'client_count' => count($clients),
            'is_cluster' => $isCluster,
        ],
        $getExistingResults,
        $getNonExistingResults,
        $setResults
    );
}

function createClients(int $clientCount, callable $createAction): array
{
    $clients = [];
    for ($i = 0; $i < $clientCount; $i++) {
        $clients[] = $createAction();
    }
    return $clients;
}

function main(
    int $totalCommands,
    int $numOfConcurrentTasks,
    int $dataSize,
    string $clientsToRun,
    string $host,
    int $clientCount,
    bool $useTls,
    bool $isCluster,
    int $port,
    array &$benchResults,
    int &$startedTasksCounter
): void {
    // Run phpredis benchmark
    if ($clientsToRun === 'all') {
        if ($isCluster) {
            $clients = createClients($clientCount, function () use ($host, $port, $useTls) {
                $client = new RedisCluster(null, ["{$host}:{$port}"]);
                if ($useTls) {
                    $client->setOption(Redis::OPT_SSL_CONTEXT, ['verify_peer' => false]);
                }
                return $client;
            });
        } else {
            $clients = createClients($clientCount, function () use ($host, $port, $useTls) {
                $client = new Redis();
                if ($useTls) {
                    $client->connect("tls://{$host}", $port);
                } else {
                    $client->connect($host, $port);
                }
                return $client;
            });
        }
        
        $result = runClients(
            $clients,
            'phpredis',
            $totalCommands,
            $numOfConcurrentTasks,
            $dataSize,
            $isCluster,
            $startedTasksCounter
        );
        $benchResults[] = $result;
        
        foreach ($clients as $client) {
            $client->close();
        }
    }
    
    // Run ValkeyGlide benchmark
    if ($clientsToRun === 'all' || $clientsToRun === 'glide') {
        if ($isCluster) {
            $clients = createClients($clientCount, function () use ($host, $port, $useTls) {
                return new ValkeyGlideCluster(
                    addresses: [['host' => $host, 'port' => $port]],
                    use_tls: $useTls
                );
            });
        } else {
            $clients = createClients($clientCount, function () use ($host, $port, $useTls) {
                return new ValkeyGlide(
                    addresses: [['host' => $host, 'port' => $port]],
                    use_tls: $useTls
                );
            });
        }
        
        $result = runClients(
            $clients,
            'glide',
            $totalCommands,
            $numOfConcurrentTasks,
            $dataSize,
            $isCluster,
            $startedTasksCounter
        );
        $benchResults[] = $result;
        
        foreach ($clients as $client) {
            $client->close();
        }
    }
}

// Main execution
$concurrentTasks = $args['concurrentTasks'];
$dataSize = $args['dataSize'];
$clientsToRun = $args['clients'];
$clientCount = $args['clientCount'];
$host = $args['host'];
$useTls = $args['tls'];
$port = $args['port'];
$isCluster = $args['clusterModeEnabled'];

$productOfArguments = [];
foreach ($concurrentTasks as $numTasks) {
    foreach ($clientCount as $numClients) {
        $numTasks = (int)$numTasks;
        $numClients = (int)$numClients;
        if ($numClients <= $numTasks) {
            $productOfArguments[] = [$dataSize, $numTasks, $numClients];
        }
    }
}

foreach ($productOfArguments as [$dataSize, $numOfConcurrentTasks, $numberOfClients]) {
    $iterations = $args['minimal'] ? 1000 : numberOfIterations($numOfConcurrentTasks);
    
    main(
        $iterations,
        $numOfConcurrentTasks,
        $dataSize,
        $clientsToRun,
        $host,
        $numberOfClients,
        $useTls,
        $isCluster,
        $port,
        $benchResults,
        $startedTasksCounter
    );
}

processResults($benchResults, $args['resultsFile']);
$mdFile = str_replace('.json', '.md', $args['resultsFile']);
echo "Results written to {$args['resultsFile']}\n";
echo "Markdown report written to {$mdFile}\n";
