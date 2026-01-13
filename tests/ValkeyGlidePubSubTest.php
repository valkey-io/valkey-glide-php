<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * ValkeyGlide PubSub Test
 * Tests publish/subscribe functionality for standalone ValkeyGlide client
 */
class ValkeyGlidePubSubTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    public function testPubSubPublish()
    {
        // Test publish command works
        $channel = 'test_publish_' . uniqid();
        
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $count = $pub->publish($channel, 'test_message');
        $pub->close();
        
        $this->assertIsInt($count, 'Publish should return integer subscriber count');
        $this->assertGTE(0, $count, 'Subscriber count should be >= 0');
    }

    public function testPubSubMessageDelivery()
    {
        // Test that messages are actually delivered to subscribers
        $channel = 'test_delivery_' . uniqid();
        $message = 'hello_' . time();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        
        // Delete the temp files so we can verify they're created by the callback
        @unlink($sync_file);
        @unlink($result_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_message_delivery.php';
        
        // Start subscriber
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
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
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $count = $pub->publish($channel, $message);
        $pub->close();
        
        $this->assertGTE(1, $count, 'Should have at least 1 subscriber');
        
        // Wait for callback result
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }
        
        // Cleanup
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        
        $this->assertTrue($success, 'Message should be delivered to subscriber callback');
    }

    public function testPubSubUnsubscribe()
    {
        // Test that unsubscribe breaks the subscribe loop
        $channel = 'test_unsub_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $unsub_file = tempnam(sys_get_temp_dir(), 'unsub_');
        
        // Delete the temp files so we can verify they're created by the callback
        @unlink($sync_file);
        @unlink($unsub_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_unsubscribe.php';
        
        // Start subscriber
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
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
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel, 'trigger');
        $pub->close();
        
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
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($unsub_file);
        
        $this->assertTrue($success, 'Unsubscribe should be called and break subscribe loop');
    }

    public function testPubSubPartialUnsubscribe()
    {
        // Test that unsubscribing from one channel doesn't break the loop
        $channel1 = 'test_partial1_' . uniqid();
        $channel2 = 'test_partial2_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        
        @unlink($sync_file);
        @unlink($result_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_partial_unsubscribe.php';
        
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
            escapeshellarg($channel1),
            escapeshellarg($channel2),
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
        
        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');
        
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel1, 'msg1');
        usleep(100000);
        $pub->publish($channel2, 'msg2');
        $pub->close();
        
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }
        
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        
        $this->assertTrue($success, 'Should receive message on second channel after unsubscribing from first');
    }

    public function testPubSubModalMode()
    {
        // Test that client is in modal mode during subscribe - only unsubscribe allowed
        $channel = 'test_modal_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        
        @unlink($sync_file);
        @unlink($result_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_modal_test.php';
        
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
            escapeshellarg($channel),
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
        
        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');
        
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel, 'trigger');
        $pub->close();
        
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $content = file_get_contents($result_file);
                if ($content === 'PASS') {
                    $success = true;
                }
                break;
            }
            usleep(100000);
        }
        
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        
        $this->assertTrue($success, 'Commands should be blocked during subscribe mode');
    }

    public function testPubSubModalModeBlocksSubscribe()
    {
        $channel = 'test_modal_sub_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        
        @unlink($sync_file);
        @unlink($result_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_modal_subscribe_test.php';
        
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
            escapeshellarg($channel),
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
        
        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');
        
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel, 'trigger');
        $pub->close();
        
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $content = file_get_contents($result_file);
                if ($content === 'PASS') {
                    $success = true;
                }
                break;
            }
            usleep(100000);
        }
        
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        
        $this->assertTrue($success, 'Subscribe should be blocked during subscribe mode');
    }

    public function testPubSubPSubscribe()
    {
        $pattern = 'test_psub_*';
        $channel = 'test_psub_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        
        @unlink($sync_file);
        @unlink($result_file);
        
        $sub_script = __DIR__ . '/scripts/subscriber_psubscribe.php';
        
        $cmd = sprintf(
            '%s -n -d extension=%s/modules/valkey_glide.so %s %s %d %s %s %s 2>/dev/null',
            PHP_BINARY,
            dirname(__DIR__),
            escapeshellarg($sub_script),
            escapeshellarg($this->getHost()),
            $this->getPort(),
            escapeshellarg($pattern),
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
        
        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');
        
        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel, 'pattern_message');
        $pub->close();
        
        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $success = true;
                break;
            }
            usleep(100000);
        }
        
        foreach ($pipes as $pipe) @fclose($pipe);
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        
        $this->assertTrue($success, 'Pattern subscribe should receive matching messages');
    }
}
