<?php
// Subscriber script for multi-process pubsub test
// Usage: php subscriber.php <host> <port> <channel> <message> <sync_file> <result_file>

if ($argc !== 7) {
    echo "Usage: php subscriber.php <host> <port> <channel> <message> <sync_file> <result_file>\n";
    exit(1);
}

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$expected_message = $argv[4];
$sync_file = $argv[5];
$result_file = $argv[6];

try {
    $subscriber = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    
    // Signal ready
    file_put_contents($sync_file, 'ready');
    
    $message_received = false;
    $subscriber->subscribe([$channel], function($redis, $recv_channel, $message) use (&$message_received, $channel, $expected_message, $result_file) {
        if ($recv_channel === $channel && $message === $expected_message) {
            $message_received = true;
            file_put_contents($result_file, 'SUCCESS');
        }
        $redis->unsubscribe([$channel]);
    });
    
    $subscriber->close();
    exit($message_received ? 0 : 1);
} catch (Exception $e) {
    echo "Subscriber error: " . $e->getMessage() . "\n";
    exit(1);
}
