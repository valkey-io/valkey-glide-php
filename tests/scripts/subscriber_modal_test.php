<?php
// Subscriber script for modal mode test

$host = $argv[1];
$port = (int)$argv[2];
$channel = $argv[3];
$sync_file = $argv[4];
$result_file = $argv[5];

$client = new ValkeyGlide([['host' => $host, 'port' => $port]]);

// Test command before subscribe - should work
try {
    $client->set('test_before', 'value');
} catch (Exception $e) {
    file_put_contents($result_file, 'FAIL_BEFORE');
    exit(1);
}

// Signal ready
file_put_contents($sync_file, '1');

$client->subscribe([$channel], function($client, $ch, $msg) use ($result_file) {
    try {
        // Try to call GET - should fail
        $client->get('some_key');
        // If we get here, test failed
        file_put_contents($result_file, 'FAIL');
    } catch (Exception $e) {
        if (strpos($e->getMessage(), 'subscribe mode') !== false) {
            // Correct exception - test passed
            file_put_contents($result_file, 'PASS');
        } else {
            // Wrong exception
            file_put_contents($result_file, 'FAIL');
        }
    }
    
    // Unsubscribe to exit
    $client->unsubscribe();
});

$client->close();
