<?php

// Subscriber script for testPubSubMessageDelivery
// Args: host, port, channel, expected_message, sync_file, result_file

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$expected_msg = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];

$sub = new ValkeyGlide([['host' => $host, 'port' => $port]]);
file_put_contents($sync_file, 'ready');

$sub->subscribe([$channel], function ($client, $ch, $msg) use ($result_file, $expected_msg, $channel) {
    if ($msg === $expected_msg) {
        file_put_contents($result_file, 'SUCCESS');
    }
    $client->unsubscribe([$channel]);
});
