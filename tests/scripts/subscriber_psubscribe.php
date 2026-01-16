<?php

$host = $argv[1];
$port = (int)$argv[2];
$pattern = $argv[3];
$sync_file = $argv[4];
$result_file = $argv[5];
$error_file = $result_file . '.error';

try {
    $client = new ValkeyGlide([['host' => $host, 'port' => $port]]);

    file_put_contents($sync_file, '1');

    $client->psubscribe([$pattern], function ($client, $channel, $message, $pattern) use ($result_file) {
        file_put_contents($result_file, '1');
        $client->punsubscribe();
    });

    $client->close();
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
}
