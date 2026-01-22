#!/usr/bin/env php
<?php
// Script to test unsubscribing from a channel we never subscribed to
// Args: host port real_channel fake_channel sync_file result_file

$host = $argv[1];
$port = $argv[2];
$real_channel = $argv[3];
$fake_channel = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];

try {
    $client = new ValkeyGlide();
    $client->connect(addresses: [['host' => $host, 'port' => (int)$port]]);
    file_put_contents($sync_file, 'ready');

    $client->subscribe([$real_channel], function ($client, $channel, $message) use ($fake_channel, $real_channel, $result_file, $sync_file) {
        // Unsubscribe from fake_channel (never subscribed)
        $client->unsubscribe([$fake_channel]);

        // Signal message received
        file_put_contents($result_file, 'RECEIVED');

        // Unsubscribe from real_channel to exit
        $client->unsubscribe([$real_channel]);
    });
} catch (Exception $e) {
    file_put_contents($result_file . '.error', $e->getMessage() . "\n" . $e->getTraceAsString());
    file_put_contents($sync_file, 'error');
    exit(1);
}
