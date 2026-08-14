<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

// Monitor multi-capture script for testing
// Usage: php monitor_multi_capture.php <host> <port> <sync_file> <result_file> <prefix> <expected_count>
//
// Captures all monitor lines containing the given prefix, up to expected_count matches,
// then writes them all (newline-separated) to result_file.
//
// NOTE: The "ready" signal is written just before monitor() because monitor()
// is blocking and create_monitor_client() completes synchronously within it,
// but the signal itself does not guarantee the MONITOR handshake has finished
// by the time the parent reads it — there is no way to observe that from this
// process without a deeper API change. Rather than relying on a fixed delay,
// the parent test resends its triggering command periodically until it
// appears in the captured output (see ValkeyGlideMonitorTest::triggerUntilCaptured),
// which bounds the race by the parent's timeout instead of guessing at a delay.

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
$max_lines = 200; // Safety limit

if (!is_numeric($argv[6]) || (int)$argv[6] != $argv[6]) {
    echo "expected_count must be an integer between 1 and {$max_lines}\n";
    exit(1);
}
$expected_count = (int)$argv[6];
if ($expected_count < 1 || $expected_count > $max_lines) {
    echo "expected_count must be an integer between 1 and {$max_lines}\n";
    exit(1);
}

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
} catch (\Throwable $e) {
    file_put_contents($result_file, 'ERROR: ' . $e->getMessage());
    exit(1);
}
