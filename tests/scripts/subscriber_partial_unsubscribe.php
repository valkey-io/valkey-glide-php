<?php

// Subscriber script for testPubSubPartialUnsubscribe
// Args: host, port, channel1, channel2, sync_file, result_file

$host = $argv[1];
$port = (int)$argv[2];
$channel1 = $argv[3];
$channel2 = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];
$error_file = $result_file . '.error';

try {
    $sub = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    file_put_contents($sync_file, 'ready');

    $sub->subscribe([$channel1, $channel2], function ($client, $ch, $msg) use ($result_file, $channel1, $channel2) {
        if ($ch === $channel1) {
            // Unsubscribe from channel1 only - should NOT break loop
            $client->unsubscribe([$channel1]);
        } elseif ($ch === $channel2) {
            // Got message on channel2 after unsubscribing from channel1 - success!
            file_put_contents($result_file, 'SUCCESS');
            $client->unsubscribe();
        }
    });
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
}
