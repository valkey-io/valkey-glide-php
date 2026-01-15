<?php

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$expected_message = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];

$client = new ValkeyGlideCluster([['host' => $host, 'port' => $port]]);

file_put_contents($sync_file, '1');

$client->subscribe([$channel], function ($client, $ch, $msg) use ($expected_message, $result_file) {
    if ($msg === $expected_message) {
        file_put_contents($result_file, '1');
        $client->unsubscribe([$ch]);
    }
});
