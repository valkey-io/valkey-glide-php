<?php

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$sync_file = $argv[4];
$unsub_file = $argv[5];
$error_file = $unsub_file . '.error';

try {
    $client = new ValkeyGlideCluster([['host' => $host, 'port' => $port]]);

    file_put_contents($sync_file, '1');

    $client->subscribe([$channel], function ($client, $ch, $msg) use ($unsub_file) {
        $client->unsubscribe([$ch]);
        file_put_contents($unsub_file, '1');
    });
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
    file_put_contents($sync_file, 'error');
}
