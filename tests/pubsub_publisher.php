<?php
// Publisher script for multi-process pubsub test
// Usage: php publisher.php <host> <port> <channel> <message> <sync_file>

// Load ValkeyGlide extension if not already loaded
if (!extension_loaded('valkey_glide')) {
    echo "ValkeyGlide extension not loaded\n";
    exit(1);
}

if ($argc !== 6) {
    echo "Usage: php publisher.php <host> <port> <channel> <message> <sync_file>\n";
    exit(1);
}

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$message = $argv[4];
$sync_file = $argv[5];

// Wait for subscriber to be ready
$timeout = time() + 10;
while (!file_exists($sync_file)) {
    usleep(10000); // 10ms
    if (time() > $timeout) {
        echo "Timeout waiting for subscriber\n";
        exit(1);
    }
}

try {
    $publisher = new ValkeyGlide([['host' => $host, 'port' => $port]]);
    $count = $publisher->publish($channel, $message);
    $publisher->close();
    exit(0);
} catch (Exception $e) {
    echo "Publisher error: " . $e->getMessage() . "\n";
    exit(1);
}
