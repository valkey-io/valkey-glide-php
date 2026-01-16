<?php

// Subscriber script for testPubSubUnsubscribe
// Args: host, port, channel, sync_file, unsub_file

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$sync_file = $argv[4];
$unsub_file = $argv[5];
$error_file = $unsub_file . '.error';

try {
    $sub = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    file_put_contents($sync_file, 'ready');

    $sub->subscribe([$channel], function ($client, $ch, $msg) use ($unsub_file, $channel) {
        file_put_contents($unsub_file, 'unsubscribed');
        $client->unsubscribe([$channel]);
    });
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
    file_put_contents($sync_file, 'error');
}
