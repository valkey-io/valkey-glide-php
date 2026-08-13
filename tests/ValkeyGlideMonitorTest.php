<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * ValkeyGlide Monitor Test
 * Tests MONITOR command functionality for standalone ValkeyGlide client.
 *
 * Test coverage mirrors the C# implementation test cases:
 * - Basic monitor captures commands from other clients
 * - Monitor captures SET/GET commands
 * - Monitor callback receives correct format (timestamp, db, client addr, command)
 * - Monitor exits when callback returns non-null
 * - Monitor throws on invalid callback
 * - Monitor not supported on cluster (standalone only)
 */
class ValkeyGlideMonitorTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    private function buildMonitorCommand($script, ...$args)
    {
        $extension_path = __DIR__ . '/../modules/valkey_glide.so';

        if (file_exists($extension_path)) {
            $cmd_parts = [
                PHP_BINARY,
                '-n',
                '-d',
                'extension=' . escapeshellarg($extension_path),
                escapeshellarg($script)
            ];
        } else {
            $cmd_parts = [
                PHP_BINARY,
                '-n',
                '-d',
                'extension=valkey_glide',
                escapeshellarg($script)
            ];
        }

        foreach ($args as $arg) {
            $cmd_parts[] = is_int($arg) ? $arg : escapeshellarg($arg);
        }

        return implode(' ', $cmd_parts);
    }

    /**
     * Test that monitor() requires a callable argument.
     */
    public function testMonitorRequiresCallable()
    {
        $client = new ValkeyGlide();
        $client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);

        $threw = false;
        try {
            // Passing a non-callable should throw TypeError due to callable type hint
            $client->monitor('not_a_function_that_exists');
        } catch (\TypeError $e) {
            $threw = true;
        }

        $this->assertTrue($threw, 'monitor() should throw TypeError when callback is not callable');
        $client->close();
    }

    /**
     * Test that monitor captures SET commands from another client.
     * Uses subprocess approach (same pattern as pubsub tests).
     */
    public function testMonitorCapturesSetCommand()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        $test_key = 'monitor_test_key_' . uniqid();
        $test_value = 'monitor_test_value_' . time();

        // Start monitor listener in subprocess
        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $test_key  // expected_command: look for our key name in monitor output
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $this->assertTrue(is_resource($proc), 'Monitor subprocess should start');

        // Wait for monitor to be ready
        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }

        $this->assertTrue(file_exists($sync_file), 'Monitor should signal ready');

        // Small delay to ensure monitor is actively listening
        usleep(200000);

        // Execute a SET command on a separate client
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($test_key, $test_value);
        $action_client->del($test_key);
        $action_client->close();

        // Wait for monitor to capture and write result
        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }

        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        // Cleanup subprocess
        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        proc_close($proc);

        // Verify result
        $this->assertTrue(file_exists($result_file), 'Monitor should write result file');
        $result = file_get_contents($result_file);

        $this->assertNotEquals('', $result, 'Result should not be empty');
        $this->assertStringNotContains('ERROR:', $result, 'Monitor should not error: ' . $result);
        $this->assertStringNotContains('TIMEOUT:', $result, 'Monitor should find command before timeout');

        // Verify the captured line contains our key
        $this->assertStringContains(
            $test_key,
            $result,
            'Monitor output should contain the key name'
        );

        // Verify the captured line contains the SET command (not just the key
        // which could also appear in the DEL that follows)
        $this->assertTrue(
            stripos($result, 'SET') !== false,
            'Monitor output should contain SET command: ' . $result
        );

        // Verify format: monitor lines look like "1339877440.333333 [0 127.0.0.1:6379] \"SET\" \"key\" \"value\""
        $this->assertRegex(
            '/^\d+\.\d+/',
            $result,
            'Monitor output should start with a timestamp'
        );

        // Cleanup temp files
        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that monitor captures GET commands.
     */
    public function testMonitorCapturesGetCommand()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        $test_key = 'monitor_get_key_' . uniqid();

        // Start monitor listener
        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $test_key
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        // Wait for ready
        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        // Execute a GET command
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->get($test_key);
        $action_client->close();

        // Wait for result
        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        proc_close($proc);

        $this->assertTrue(file_exists($result_file), 'Monitor should write result');
        $result = file_get_contents($result_file);

        $this->assertStringContains(
            $test_key,
            $result,
            'Monitor should capture GET command with key'
        );
        // The output should contain "GET" (case insensitive since Valkey may uppercase it)
        $this->assertTrue(
            stripos($result, 'GET') !== false,
            'Monitor output should contain GET command: ' . $result
        );

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that monitor output format includes timestamp, db, and client address.
     * Format: "1339877440.333333 [0 127.0.0.1:6379] \"command\" \"args\""
     */
    public function testMonitorOutputFormat()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        $unique_marker = 'FMTTEST_' . uniqid();

        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $unique_marker
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        // Execute a PING with our marker as key lookup
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($unique_marker, 'value');
        $action_client->close();

        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        proc_close($proc);

        $result = file_get_contents($result_file);

        // Validate format: timestamp [db client_addr] command args
        // Pattern: float_timestamp [int ip:port] "COMMAND" "arg1" "arg2"
        $this->assertRegex(
            '/^\d+\.\d+\s+\[\d+\s+[\d\.:]+\]/',
            $result,
            'Monitor output should match format: timestamp [db client_addr]: ' . $result
        );

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that monitor callback returning non-null exits monitor mode.
     */
    public function testMonitorCallbackReturnExits()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        // Use a unique marker that will definitely appear (PING from monitor setup)
        $marker = 'EXIT_MARKER_' . uniqid();

        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $marker,
            10  // max_lines = 10 (short to ensure it exits quickly)
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        // Send the marker command
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($marker, 'test');
        $action_client->close();

        // The monitor should exit after finding the marker
        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        $exit_code = proc_close($proc);

        $this->assertTrue(
            file_exists($result_file),
            'Monitor should have written result (exited via callback return)'
        );
        $this->assertEquals(0, $exit_code, 'Monitor process should exit cleanly');

        $result = file_get_contents($result_file);
        $this->assertStringContains(
            $marker,
            $result,
            'Result should contain our marker command'
        );

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that monitor captures multiple command types (SET, GET, DEL).
     */
    public function testMonitorCapturesMultipleCommands()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        // Use a script that captures multiple lines
        $monitor_script = __DIR__ . '/scripts/monitor_multi_capture.php';

        // Only run this test if the multi-capture script exists
        if (!file_exists($monitor_script)) {
            $this->markTestSkipped('monitor_multi_capture.php script not available');
            return;
        }

        $unique_prefix = 'MULTI_' . uniqid();

        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $unique_prefix,
            3  // expect 3 commands
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        // Execute SET, GET, DEL
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($unique_prefix . '_key', 'value');
        $action_client->get($unique_prefix . '_key');
        $action_client->del($unique_prefix . '_key');
        $action_client->close();

        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        proc_close($proc);

        $this->assertTrue(file_exists($result_file), 'Monitor multi-capture should write result file');

        $result = file_get_contents($result_file);
        $lines = explode("\n", trim($result));

        $this->assertGreaterThanOrEqual(
            3,
            count($lines),
            'Monitor should capture at least 3 commands'
        );

        // Verify SET, GET, and DEL are all captured
        $has_set = false;
        $has_get = false;
        $has_del = false;
        foreach ($lines as $line) {
            if (stripos($line, 'SET') !== false) {
                $has_set = true;
            }
            if (stripos($line, 'GET') !== false) {
                $has_get = true;
            }
            if (stripos($line, 'DEL') !== false) {
                $has_del = true;
            }
        }

        $this->assertTrue($has_set, 'Monitor should capture SET command');
        $this->assertTrue($has_get, 'Monitor should capture GET command');
        $this->assertTrue($has_del, 'Monitor should capture DEL command');

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that calling close() after monitor() completes is safe (idempotent cleanup).
     * Mirrors Python test_monitor_stop_idempotent and Go TestMonitorCloseIdempotent.
     */
    public function testMonitorCloseAfterMonitorIsSafe()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        $marker = 'CLOSE_SAFE_' . uniqid();

        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $marker,
            5  // max_lines
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        // Trigger the monitor to exit
        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($marker, 'test');
        $action_client->close();

        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        $exit_code = proc_close($proc);

        // The monitor subprocess calls close() after monitor() returns — it should not crash
        $this->assertEquals(
            0,
            $exit_code,
            'Process should exit cleanly after monitor() and close()'
        );

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Test that monitor is NOT available on cluster client.
     * MONITOR is only supported for standalone connections.
     * Mirrors Python test_monitor_rejects_cluster_config.
     */
    public function testMonitorNotAvailableOnCluster()
    {
        // ValkeyGlideCluster should not have a monitor() method
        $clusterReflection = new \ReflectionClass('ValkeyGlideCluster');
        $hasMonitor = $clusterReflection->hasMethod('monitor');
        $this->assertFalse($hasMonitor);
    }

    /**
     * Test that monitor output contains the correct command arguments.
     * Mirrors Go TestMonitorFields which checks len(line.Args)==2 for SET key value.
     */
    public function testMonitorOutputContainsArgs()
    {
        $sync_file = tempnam(sys_get_temp_dir(), 'mon_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'mon_result_');

        @unlink($sync_file);
        @unlink($result_file);

        $monitor_script = __DIR__ . '/scripts/monitor_listener.php';
        $test_key = 'ARGS_KEY_' . uniqid();
        $test_value = 'ARGS_VALUE_' . uniqid();

        $cmd = $this->buildMonitorCommand(
            $monitor_script,
            $this->getHost(),
            $this->getPort(),
            $sync_file,
            $result_file,
            $test_key
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }
        usleep(200000);

        $action_client = new ValkeyGlide();
        $action_client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $action_client->set($test_key, $test_value);
        $action_client->close();

        $timeout = time() + 5;
        while (!file_exists($result_file) && time() < $timeout) {
            usleep(100000);
        }


        // Terminate subprocess if it hasn't exited
        if (!file_exists($result_file)) {
            proc_terminate($proc);
        }

        foreach ($pipes as $pipe) {
            fclose($pipe);
        }
        proc_close($proc);

        $result = file_get_contents($result_file);

        // Verify the output contains both the key AND the value as arguments
        $this->assertStringContains(
            $test_key,
            $result,
            'Monitor output should contain the key argument'
        );
        $this->assertStringContains(
            $test_value,
            $result,
            'Monitor output should contain the value argument'
        );

        // Verify the SET command is present
        $this->assertTrue(
            stripos($result, 'SET') !== false,
            'Monitor output should contain SET command'
        );

        @unlink($sync_file);
        @unlink($result_file);
    }

    /**
     * Helper assertions
     */
    private function assertStringContains($needle, $haystack, $message = '')
    {
        $this->assertTrue(
            strpos($haystack, $needle) !== false,
            $message ?: "Failed asserting that '$haystack' contains '$needle'"
        );
    }

    private function assertStringNotContains($needle, $haystack, $message = '')
    {
        $this->assertTrue(
            strpos($haystack, $needle) === false,
            $message ?: "Failed asserting that '$haystack' does not contain '$needle'"
        );
    }

    private function assertRegex($pattern, $subject, $message = '')
    {
        $this->assertTrue(
            preg_match($pattern, $subject) === 1,
            $message ?: "Failed asserting that '$subject' matches pattern '$pattern'"
        );
    }

    private function assertGreaterThanOrEqual($expected, $actual, $message = '')
    {
        $this->assertTrue(
            $actual >= $expected,
            $message ?: "Failed asserting that $actual >= $expected"
        );
    }

    private function markTestSkipped($reason)
    {
        echo "    SKIPPED: $reason\n";
    }
}
