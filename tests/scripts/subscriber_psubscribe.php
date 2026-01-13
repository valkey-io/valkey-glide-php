<?php
$host = $argv[1];
$port = (int)$argv[2];
$pattern = $argv[3];
$sync_file = $argv[4];
$result_file = $argv[5];

$client = new ValkeyGlide([['host' => $host, 'port' => $port]]);

file_put_contents($sync_file, '1');

$client->psubscribe([$pattern], function($client, $channel, $message, $pattern) use ($result_file) {
    file_put_contents($result_file, '1');
    $client->punsubscribe();
});

$client->close();
