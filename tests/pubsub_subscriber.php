<?php
// Subscriber script for multi-process pubsub test
// Usage: php subscriber.php <host> <port> <channel> <message> <sync_file> <result_file> [done_file]

if (!extension_loaded('valkey_glide')) {
    echo "ValkeyGlide extension not loaded\n";
    exit(1);
}

if ($argc < 7) {
    echo "Usage: php subscriber.php <host> <port> <channel> <message> <sync_file> <result_file> [done_file]\n";
    exit(1);
}

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$expected_message = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];
$done_file = $argc > 7 ? $argv[7] : null;

try {
    $subscriber = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    
    // Signal ready
    file_put_contents($sync_file, 'ready');
    
    $message_received = false;
    
    $subscriber->subscribe([$channel], function($redis, $recv_channel, $message) use (&$message_received, $channel, $expected_message, $result_file, $done_file) {
        if ($recv_channel === $channel && $message === $expected_message) {
            $message_received = true;
            file_put_contents($result_file, 'SUCCESS');
        }
        $redis->unsubscribe([$channel]);
        
        // Signal completion if done_file provided
        if ($done_file) {
            file_put_contents($done_file, 'done');
        }
    });
    
    $subscriber->close();
    exit($message_received ? 0 : 1);
} catch (Exception $e) {
    file_put_contents($result_file, 'ERROR: ' . $e->getMessage());
    exit(1);
}
