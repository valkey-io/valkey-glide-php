<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

// Monitor script for multi-process monitor test
// Usage: php monitor_listener.php <host> <port> <sync_file> <result_file> <expected_command> [max_lines]
//
// This script:
// 1. Creates a dedicated ValkeyGlideMonitor connection.
// 2. Signals readiness via sync_file only after the first monitor record is
//    received. The parent deliberately sends a PING probe while waiting, so
//    this is a real acknowledgement that the dedicated MONITOR connection is
//    established and receiving events—not merely that this process started.
// 3. When it sees a line containing expected_command, writes it to result_file
//    and exits.

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
    $monitor_client = new ValkeyGlideMonitor(addresses: [['host' => $host, 'port' => $port]]);

    $line_count = 0;
    $found = false;
    $ready = false;

    $monitor_client->listen(function (ValkeyGlideMonitorLine $line) use (&$line_count, &$found, &$ready, $sync_file, $max_lines, $expected_command, $result_file) {
        // The parent sends PING probes while waiting for this acknowledgement.
        // A callback proves the dedicated MONITOR connection is active.
        if (!$ready) {
            file_put_contents($sync_file, 'ready');
            $ready = true;
        }

        $line_count++;

        // Render the structured line to the PHPRedis-compatible text form so
        // the substring checks below work against command + args.
        $command = (string)$line;

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
} catch (\Throwable $e) {
    file_put_contents($result_file, 'ERROR: ' . $e->getMessage());
    exit(1);
}
