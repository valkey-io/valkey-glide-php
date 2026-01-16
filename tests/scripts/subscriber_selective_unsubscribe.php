<?php

// Subscriber script for testPubSubSelectiveUnsubscribe
// Args: host, port, channel1, channel2, sync_file, result_file

$host = $argv[1];
$port = (int)$argv[2];
$channel1 = $argv[3];
$channel2 = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];
$error_file = $result_file . '.error';

try {
    $client = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    file_put_contents($sync_file, 'ready');

    $received = [];
    $client->subscribe([$channel1, $channel2], function ($c, $ch, $msg) use (&$received, $channel1, $channel2, $result_file) {
        $received[] = "$ch:$msg";

        if ($msg === 'bing' && $ch === $channel1) {
            $c->unsubscribe([$channel1]);
        }

        if ($msg === 'bong' && $ch === $channel2) {
            file_put_contents($result_file, implode(',', $received));
            $c->unsubscribe([$channel2]);
        }
    });
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
}
