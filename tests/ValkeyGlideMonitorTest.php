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
     * Test that listen() requires a callable argument.
     */
    public function testMonitorRequiresCallable()
    {
        $monitor = new ValkeyGlideMonitor(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]
        );

        $threw = false;
        try {
            // A non-callable string must be rejected by the callable type check.
            $monitor->listen('not_a_function_that_exists');
        } catch (\Throwable $e) {
            $threw = true;
        }

        $this->assertTrue(
            $threw,
            'listen() should reject a non-callable argument'
        );
        $monitor->close();
    }

    /**
     * Test the pull API: getMonitorMessage() returns a structured
     * ValkeyGlideMonitorLine with decoded fields.
     */
    public function testMonitorPullReturnsStructuredLine()
    {
        $monitor = new ValkeyGlideMonitor(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]
        );

        $key = 'PULL_KEY_' . uniqid();
        $val = 'PULL_VAL_' . uniqid();

        // Give the dedicated monitor connection a moment to activate.
        $action = new ValkeyGlide();
        $action->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        // Capture a reference time just before issuing the command so we can
        // assert the monitor line's timestamp is at/after it.
        $before = microtime(true);
        $action->set($key, $val);
        $action->close();

        // MONITOR streams every command the (shared) server processes, including
        // traffic from other clients/tests, so scan until we find our own SET or
        // a deadline elapses — rather than after a fixed number of reads.
        $found = null;
        $deadline = microtime(true) + 5.0;
        while (microtime(true) < $deadline) {
            $line = $monitor->getMonitorMessage(timeout: 1.0);
            if ($line === null) {
                continue;
            }
            $this->assertTrue(
                $line instanceof ValkeyGlideMonitorLine,
                'getMonitorMessage should return a ValkeyGlideMonitorLine'
            );
            if (strcasecmp($line->command, 'SET') === 0 && in_array($key, $line->args, true)) {
                $found = $line;
                break;
            }
        }

        $this->assertTrue($found !== null, 'Pull API should capture the SET command');
        if ($found !== null) {
            // We issued this command ourselves, so assert exact values.
            $this->assertGreaterThanOrEqual(
                $before,
                $found->timestamp,
                'timestamp should be at or after the command was issued'
            );
            $this->assertTrue(
                $found->timestamp <= microtime(true) + 1.0,
                'timestamp should not be in the future'
            );
            // Connected without a database_id, so the command ran against db 0.
            $this->assertEquals(0, $found->db, 'db should be 0 (default database)');
            // clientAddr is the action client's address; the port is OS-assigned
            // (ephemeral), so verify the host:port shape rather than an exact port.
            $this->assertRegex('/^[\d.]+:\d+$/', $found->clientAddr, 'clientAddr should be host:port');
            // MONITOR echoes the command name in lowercase.
            $this->assertEquals('set', strtolower($found->command), 'command should be SET');
            // A plain SET carries exactly [key, value] as its arguments, in order.
            $this->assertEquals([$key, $val], $found->args, 'args should be exactly [key, value]');
            // __toString renders the Valkey/Redis MONITOR line:
            //   "<timestamp> [<db> <addr>] \"SET\" \"key\" \"value\"".
            $this->assertRegex(
                '/^\d+\.\d+ \[0 [\d.]+:\d+\] "set"/i',
                (string)$found,
                'toString should render the MONITOR line prefix (db 0 + quoted command)'
            );
            $this->assertTrue(strpos((string)$found, $key) !== false, 'toString should include the key');
            $this->assertTrue(strpos((string)$found, $val) !== false, 'toString should include the value');
        }

        $this->assertTrue($monitor->getDroppedCount() >= 0, 'getDroppedCount should be non-negative');
        $monitor->close();
    }

    /**
     * Test that tryGetMonitorMessage() is non-blocking (returns null when idle).
     */
    public function testMonitorTryGetIsNonBlocking()
    {
        $monitor = new ValkeyGlideMonitor(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]
        );

        // Drain any startup/background traffic until the queue is empty,
        // bounded by a deadline so a busy server can't make this loop run
        // forever. Then confirm a single call returns promptly (non-blocking).
        $drain_deadline = microtime(true) + 2.0;
        while ($monitor->tryGetMonitorMessage() !== null && microtime(true) < $drain_deadline) {
            // keep draining queued lines
        }

        $start = microtime(true);
        $monitor->tryGetMonitorMessage();
        $elapsed = microtime(true) - $start;

        $this->assertTrue(
            $elapsed < 1.0,
            'tryGetMonitorMessage should not block waiting for messages'
        );
        $monitor->close();
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

        // Probe until the listener's first MONITOR callback acknowledges the
        // dedicated connection is active. Only then send the command under test.
        $this->waitForMonitorActivation($sync_file);

        // Execute a SET command on a separate client.
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

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

        // Execute a GET command.
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

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

        // Execute a SET with our unique marker.
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

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

        // Send the marker command.
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
            $this->markMonitorTestSkipped('monitor_multi_capture.php script not available');
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

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

        // Execute SET, GET, DEL.
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
            100  // Allow monitor/probe connection setup traffic before the marker command.
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

        // Trigger the monitor to exit.
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
     * Test that MONITOR is exposed only via the dedicated ValkeyGlideMonitor
     * class, and is not a method on ValkeyGlide or ValkeyGlideCluster.
     * MONITOR is standalone-only. Mirrors Python test_monitor_rejects_cluster_config.
     */
    public function testMonitorNotAvailableOnCluster()
    {
        // The dedicated monitor class must exist.
        $this->assertTrue(class_exists('ValkeyGlideMonitor'), 'ValkeyGlideMonitor should exist');
        $this->assertTrue(
            class_exists('ValkeyGlideMonitorLine'),
            'ValkeyGlideMonitorLine should exist'
        );

        // monitor() must NOT be a method on the standalone or cluster clients.
        $standaloneReflection = new \ReflectionClass('ValkeyGlide');
        $this->assertFalse(
            $standaloneReflection->hasMethod('monitor'),
            'ValkeyGlide should not expose a monitor() method'
        );

        $clusterReflection = new \ReflectionClass('ValkeyGlideCluster');
        $this->assertFalse(
            $clusterReflection->hasMethod('monitor'),
            'ValkeyGlideCluster should not expose a monitor() method'
        );
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

        // Probe until the listener acknowledges that MONITOR is active.
        $this->waitForMonitorActivation($sync_file);

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
     * Wait for a listener to acknowledge that its dedicated MONITOR connection
     * is established and receiving events.
     *
     * The listener writes $syncFile from its first monitor callback, not before
     * entering monitor(). Sending PING probes until that file appears therefore
     * creates an explicit activation handshake: the acknowledgement cannot
     * exist until Valkey has delivered at least one record through MONITOR.
     */
    protected function waitForMonitorActivation(string $syncFile, int $timeoutSeconds = 5)
    {
        $probeClient = new ValkeyGlide();
        $probeClient->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);

        $deadline = microtime(true) + $timeoutSeconds;
        while (!file_exists($syncFile) && microtime(true) < $deadline) {
            $probeClient->ping();
            usleep(100000);
        }

        $probeClient->close();
        $this->assertTrue(
            file_exists($syncFile),
            'Monitor should acknowledge that its dedicated connection is active'
        );
    }

    /**
     * Helper assertions
     */
    protected function assertStringContains(string $needle, $haystack, $message = ''): bool
    {
        return $this->assertTrue(
            strpos($haystack, $needle) !== false,
            $message ?: "Failed asserting that '$haystack' contains '$needle'"
        );
    }

    protected function assertStringNotContains($needle, $haystack, $message = '')
    {
        $this->assertTrue(
            strpos($haystack, $needle) === false,
            $message ?: "Failed asserting that '$haystack' does not contain '$needle'"
        );
    }

    protected function assertRegex($pattern, $subject, $message = '')
    {
        $this->assertTrue(
            preg_match($pattern, $subject) === 1,
            $message ?: "Failed asserting that '$subject' matches pattern '$pattern'"
        );
    }

    protected function assertGreaterThanOrEqual($expected, $actual, $message = '')
    {
        $this->assertTrue(
            $actual >= $expected,
            $message ?: "Failed asserting that $actual >= $expected"
        );
    }

    protected function markMonitorTestSkipped($reason)
    {
        echo "    SKIPPED: $reason\n";
    }
}
