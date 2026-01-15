<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideClusterBaseTest.php";

/**
 * ValkeyGlideCluster PubSub Test
 * Tests publish/subscribe functionality for cluster ValkeyGlide client
 */
class ValkeyGlideClusterPubSubTest extends ValkeyGlideClusterBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    public function testPubSubPublish()
    {
        // Test publish command works in cluster mode
        $channel = 'test_publish_' . uniqid();

        $count = $this->valkey_glide->publish($channel, 'test_message');

        $this->assertIsInt($count, 'Publish should return integer subscriber count');
        $this->assertGTE(0, $count, 'Subscriber count should be >= 0');
    }

    public function testPubSubMessageDelivery()
    {
        // Test that messages are delivered in cluster mode
        $channel = 'test_delivery_' . uniqid();
        $message = 'hello_' . time();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_message_delivery_cluster.php';

        // Start subscriber - use cluster port 7001
        $cmd = sprintf(
            '%s %s %s %d %s %s %s %s 2>/dev/null',
            PHP_BINARY,
            escapeshellarg($sub_script),
            escapeshellarg('127.0.0.1'),
            7001,
            escapeshellarg($channel),
            escapeshellarg($message),
            escapeshellarg($sync_file),
            escapeshellarg($result_file)
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        // Wait for subscriber ready
        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }

        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');

        // Publish message
        $count = $this->valkey_glide->publish($channel, $message);

        $this->assertGTE(1, $count, 'Should have at least 1 subscriber');

        // Wait for callback result
        $success = false;
        $timeout = time() + 5;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }

        // Cleanup
        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);

        $this->assertTrue($success, 'Message should be delivered to subscriber callback in cluster mode');
    }

    public function testPubSubUnsubscribe()
    {
        // Test that unsubscribe works in cluster mode
        $channel = 'test_unsub_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $unsub_file = tempnam(sys_get_temp_dir(), 'unsub_');

        @unlink($sync_file);
        @unlink($unsub_file);

        $sub_script = __DIR__ . '/scripts/subscriber_unsubscribe_cluster.php';

        // Start subscriber - use cluster port 7001
        $cmd = sprintf(
            '%s %s %s %d %s %s %s 2>/dev/null',
            PHP_BINARY,
            escapeshellarg($sub_script),
            escapeshellarg('127.0.0.1'),
            7001,
            escapeshellarg($channel),
            escapeshellarg($sync_file),
            escapeshellarg($unsub_file)
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        // Wait for subscriber ready
        $timeout = time() + 5;
        while (!file_exists($sync_file) && time() < $timeout) {
            usleep(100000);
        }

        // Publish to trigger callback
        $this->valkey_glide->publish($channel, 'trigger');

        // Wait for unsubscribe signal
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($unsub_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }

        // Cleanup
        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($unsub_file);

        $this->assertTrue($success, 'Unsubscribe should work in cluster mode');
    }

    public function testPubSubPSubscribe()
    {
        $pattern = 'test_pattern_*';
        $channel = 'test_pattern_' . uniqid();
        $message = 'pattern_msg_' . time();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_psubscribe_cluster.php';

        $cmd = sprintf(
            '%s %s %s %d %s %s %s %s %s 2>/dev/null',
            PHP_BINARY,
            escapeshellarg($sub_script),
            escapeshellarg('127.0.0.1'),
            7001,
            escapeshellarg($pattern),
            escapeshellarg($channel),
            escapeshellarg($message),
            escapeshellarg($sync_file),
            escapeshellarg($result_file)
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

        $this->assertTrue(file_exists($sync_file), 'PSubscriber should signal ready');

        $this->valkey_glide->publish($channel, $message);

        $success = false;
        $timeout = time() + 5;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);

        $this->assertTrue($success, 'Pattern subscription should work in cluster mode');
    }
}
