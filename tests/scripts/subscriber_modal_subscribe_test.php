<?php

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$sync_file = $argv[4];
$result_file = $argv[5];
$error_file = $result_file . '.error';

try {
    $client = new ValkeyGlide([['host' => $host, 'port' => $port]]);

    file_put_contents($sync_file, '1');

    try {
        $client->subscribe([$channel], function ($client, $ch, $msg) use ($result_file) {
            try {
                $client->subscribe(['another_channel'], function () {
                });
                file_put_contents($result_file, 'FAIL');
            } catch (Exception $e) {
                if (strpos($e->getMessage(), 'subscribe mode') !== false) {
                    file_put_contents($result_file, 'PASS');
                } else {
                    file_put_contents($result_file, 'FAIL');
                }
            }
            $client->unsubscribe();
        });
    } catch (Exception $e) {
        file_put_contents($result_file, 'FAIL');
    }

    $client->close();
} catch (Exception $e) {
    file_put_contents($error_file, $e->getMessage() . "\n" . $e->getTraceAsString());
    file_put_contents($sync_file, 'error');
}
