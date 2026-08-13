<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

// Monitor multi-capture script for testing
// Usage: php monitor_multi_capture.php <host> <port> <sync_file> <result_file> <prefix> <expected_count>
//
// Captures all monitor lines containing the given prefix, up to expected_count matches,
// then writes them all (newline-separated) to result_file.
//
// NOTE: The "ready" signal is written just before monitor() because monitor() is
// blocking and create_monitor_client() completes synchronously within it. The parent
// test uses a small delay after receiving the signal to ensure the monitor loop has
// started receiving events.

if (!extension_loaded('valkey_glide')) {
    echo "ValkeyGlide extension not loaded\n";
    exit(1);
}

if ($argc < 7) {
    echo "Usage: php monitor_multi_capture.php <host> <port> <sync_file> <result_file> <prefix> <expected_count>\n";
    exit(1);
}

$host = $argv[1];
$port = (int)$argv[2];
$sync_file = $argv[3];
$result_file = $argv[4];
$prefix = $argv[5];
$expected_count = (int)$argv[6];
$max_lines = 200; // Safety limit

try {
    $monitor_client = new ValkeyGlide();
    $monitor_client->connect(addresses: [['host' => $host, 'port' => $port]]);

    // Signal ready — monitor() is about to be called. create_monitor_client() inside
    // monitor() is synchronous, so the connection will be fully active by the time
    // the blocking loop starts.
    file_put_contents($sync_file, 'ready');

    $captured = [];
    $line_count = 0;

    $monitor_client->monitor(function ($client, $command) use (&$captured, &$line_count, $max_lines, $prefix, $expected_count, $result_file) {
        $line_count++;

        // Capture lines that contain our prefix
        if (stripos($command, $prefix) !== false) {
            $captured[] = $command;

            if (count($captured) >= $expected_count) {
                file_put_contents($result_file, implode("\n", $captured));
                return true; // Exit monitor mode
            }
        }

        // Safety: exit after max_lines
        if ($line_count >= $max_lines) {
            file_put_contents($result_file, implode("\n", $captured));
            return true;
        }

        return null; // Continue monitoring
    });

    $monitor_client->close();
    exit(count($captured) >= $expected_count ? 0 : 1);
} catch (Exception $e) {
    file_put_contents($result_file, 'ERROR: ' . $e->getMessage());
    exit(1);
}
