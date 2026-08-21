<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

// Monitor multi-capture script for testing
// Usage: php monitor_multi_capture.php <host> <port> <sync_file> <result_file> <prefix> <expected_count>
//
// Captures all monitor lines containing the given prefix, up to expected_count matches,
// then writes them all (newline-separated) to result_file.
//
// The parent sends PING probes while waiting for a ready acknowledgement.
// This script writes sync_file only from its first MONITOR callback, proving
// the dedicated monitor connection is established and receiving events before
// the parent sends the commands being captured.

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

/**
 * Render a monitor line to the Valkey MONITOR text form
 * ("<ts> [db addr] \"CMD\" \"arg\"...") for substring matching and output.
 * Kept as a local test helper rather than relying on a client-side method.
 */
function format_monitor_line(ValkeyGlideMonitorLine $line): string
{
    $out = sprintf('%.6f [%d %s] "%s"', $line->timestamp, $line->db, $line->clientAddr, $line->command);
    foreach ($line->args as $arg) {
        $out .= ' "' . str_replace(['\\', '"'], ['\\\\', '\\"'], $arg) . '"';
    }
    return $out;
}

try {
    $monitor_client = new ValkeyGlideMonitor(addresses: [['host' => $host, 'port' => $port]]);

    $captured = [];
    $line_count = 0;
    $ready = false;

    $monitor_client->listen(function (ValkeyGlideMonitorLine $line) use (&$captured, &$line_count, &$ready, $sync_file, $max_lines, $prefix, $expected_count, $result_file) {
        // The parent sends PING probes while waiting for this acknowledgement.
        // A callback proves the dedicated MONITOR connection is active.
        if (!$ready) {
            file_put_contents($sync_file, 'ready');
            $ready = true;
        }

        $line_count++;

        // Render the structured line to text for substring matching.
        $command = format_monitor_line($line);

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
