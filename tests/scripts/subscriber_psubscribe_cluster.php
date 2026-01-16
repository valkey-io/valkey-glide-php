<?php

$host = $argv[1];
$port = (int)$argv[2];
$pattern = $argv[3];
$channel = $argv[4];
$expected_message = $argv[5];
$sync_file = $argv[6];
$result_file = $argv[7];
$error_file = $result_file . '.error';

try {
    $client = new ValkeyGlideCluster([['host' => $host, 'port' => $port]]);

    file_put_contents($sync_file, '1');

    $client->psubscribe([$pattern], function ($client, $channel, $message, $pat) use ($expected_message, $result_file) {
        if ($message === $expected_message) {
            file_put_contents($result_file, '1');
            $client->punsubscribe([$pat]);
        }
    });
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
}
