<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

// Monitor script for multi-process monitor test
// Usage: php monitor_listener.php <host> <port> <sync_file> <result_file> <expected_command> [max_lines]
//
// This script:
// 1. Connects to Valkey
// 2. Signals readiness via sync_file
// 3. Enters monitor mode (create_monitor_client is synchronous, so by the time
//    the blocking loop starts the monitor connection is fully active)
// 4. When it sees a line containing expected_command, writes it to result_file and exits
//
// NOTE: The "ready" signal is written just before monitor() because monitor() is
// blocking and create_monitor_client() completes synchronously within it. The parent
// test uses a small delay after receiving the signal to ensure the monitor loop has
// started receiving events.

if (!extension_loaded('valkey_glide')) {
    echo "ValkeyGlide extension not loaded\n";
    exit(1);
}

if ($argc < 6) {
    echo "Usage: php monitor_listener.php <host> <port> <sync_file> <result_file> <expected_command> [max_lines]\n";
    exit(1);
}

$host = $argv[1];
$port = (int)$argv[2];
$sync_file = $argv[3];
$result_file = $argv[4];
$expected_command = $argv[5];
$max_lines = $argc > 6 ? (int)$argv[6] : 100;

try {
    $monitor_client = new ValkeyGlide();
    $monitor_client->connect(addresses: [['host' => $host, 'port' => $port]]);

    // Signal ready — monitor() is about to be called. create_monitor_client() inside
    // monitor() is synchronous, so the connection will be fully active by the time
    // the blocking loop starts.
    file_put_contents($sync_file, 'ready');

    $line_count = 0;
    $found = false;

    $monitor_client->monitor(function ($client, $command) use (&$line_count, &$found, $max_lines, $expected_command, $result_file) {
        $line_count++;

        // Check if this line contains our expected command
        if (stripos($command, $expected_command) !== false) {
            $found = true;
            file_put_contents($result_file, $command);
            return true; // Exit monitor mode
        }

        // Safety: exit after max_lines to prevent infinite loop
        if ($line_count >= $max_lines) {
            file_put_contents($result_file, 'TIMEOUT: exceeded max lines without finding command');
            return true; // Exit monitor mode
        }

        return null; // Continue monitoring
    });

    $monitor_client->close();
    exit($found ? 0 : 1);
} catch (Exception $e) {
    file_put_contents($result_file, 'ERROR: ' . $e->getMessage());
    exit(1);
}
