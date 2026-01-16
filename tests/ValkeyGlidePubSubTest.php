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

    private function buildSubscriberCommand($script, ...$args)
    {
        $extension_path = __DIR__ . '/../modules/valkey_glide.so';

        if (file_exists($extension_path)) {
            // Regular tests: load from modules directory
            $cmd_parts = [
                PHP_BINARY,
                '-n',
                '-d',
                'extension=' . escapeshellarg($extension_path),
                escapeshellarg($script)
            ];
        } else {
            // PECL tests: extension installed system-wide
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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $message,
            $sync_file,
            $result_file
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

        // Check for error file immediately after sync file appears
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            if (isset($pipes[0]) && is_resource($pipes[0])) {
                @fclose($pipes[0]);
            }
            if (isset($pipes[1]) && is_resource($pipes[1])) {
                @fclose($pipes[1]);
            }
            if (isset($pipes[2]) && is_resource($pipes[2])) {
                @fclose($pipes[2]);
            }
            if (is_resource($proc)) {
                @proc_terminate($proc);
            }
            if (is_resource($proc)) {
                @proc_close($proc);
            }
            $this->fail('Subscriber script error: ' . $error);
        }

        // If sync file doesn't exist, check stderr
        if (!file_exists($sync_file)) {
            $stderr_content = '';
            if (isset($pipes[2]) && is_resource($pipes[2])) {
                stream_set_blocking($pipes[2], false);
                $stderr_content = stream_get_contents($pipes[2]);
            }
            if (isset($pipes[0]) && is_resource($pipes[0])) {
                @fclose($pipes[0]);
            }
            if (isset($pipes[1]) && is_resource($pipes[1])) {
                @fclose($pipes[1]);
            }
            if (isset($pipes[2]) && is_resource($pipes[2])) {
                @fclose($pipes[2]);
            }
            if (is_resource($proc)) {
                @proc_terminate($proc);
            }
            if (is_resource($proc)) {
                @proc_close($proc);
            }
            $this->fail('Subscriber did not start. Stderr: ' . ($stderr_content ?: 'No stderr output'));
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
        if (isset($pipes[0]) && is_resource($pipes[0])) {
            @fclose($pipes[0]);
        }
        if (isset($pipes[1]) && is_resource($pipes[1])) {
            @fclose($pipes[1]);
        }
        if (isset($pipes[2]) && is_resource($pipes[2])) {
            @fclose($pipes[2]);
        }
        if (is_resource($proc)) {
            @proc_terminate($proc);
        }
        if (is_resource($proc)) {
            @proc_close($proc);
        }
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $sync_file,
            $unsub_file
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

        // Check for error file immediately
        $error_file = $unsub_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
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
        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($unsub_file);
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel1,
            $channel2,
            $sync_file,
            $result_file
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

        // Check for error file immediately
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
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

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $sync_file,
            $result_file
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

        // Check for error file immediately
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
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

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $sync_file,
            $result_file
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

        // Check for error file immediately
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
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

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $pattern,
            $sync_file,
            $result_file
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

        // Check for error file immediately
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
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

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

        $this->assertTrue($success, 'Pattern subscribe should receive matching messages');
    }

    public function testPubSubChannels()
    {
        $result = $this->valkey_glide->pubsub("channels");
        $this->assertIsArray($result);
    }

    public function testPubSubChannelsWithPattern()
    {
        $result = $this->valkey_glide->pubsub("channels", "test*");
        $this->assertIsArray($result);
    }

    public function testPubSubNumSub()
    {
        $result = $this->valkey_glide->pubsub("numsub", ["test_channel", "another_channel"]);
        $this->assertIsArray($result);
    }

    public function testPubSubNumPat()
    {
        $result = $this->valkey_glide->pubsub("numpat");
        $this->assertIsInt($result);
        $this->assertGTE(0, $result);
    }

    public function testPubSubSelectiveUnsubscribe()
    {
        // Test unsubscribing from one channel while remaining subscribed to another
        $channel1 = 'test_selective1_' . uniqid();
        $channel2 = 'test_selective2_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_selective_unsubscribe.php';

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel1,
            $channel2,
            $sync_file,
            $result_file
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

        // Check for error file immediately
        $error_file = $result_file . '.error';
        if (file_exists($error_file)) {
            $error = file_get_contents($error_file);
            @unlink($error_file);
            @unlink($sync_file);
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            @proc_terminate($proc);
            @proc_close($proc);
            $this->fail('Subscriber script error: ' . $error);
        }

        $this->assertTrue(file_exists($sync_file));

        $pub = new ValkeyGlide([['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel1, 'bing');
        usleep(200000);
        $pub->publish($channel1, 'should_not_receive');
        usleep(200000);
        $pub->publish($channel2, 'bong');
        $pub->close();

        $success = false;
        $timeout = time() + 3;
        while (!$success && time() < $timeout) {
            if (file_exists($result_file)) {
                $content = file_get_contents($result_file);
                $messages = explode(',', $content);
                // Should have received bing on channel1 and bong on channel2, but NOT should_not_receive
                $this->assertContains($channel1 . ':bing', $messages);
                $this->assertContains($channel2 . ':bong', $messages);
                // Verify we didn't receive message after unsubscribing from channel1
                $has_should_not_receive = in_array($channel1 . ':should_not_receive', $messages);
                $this->assertFalse($has_should_not_receive);
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
        @unlink($error_file);

        $this->assertTrue($success);
    }
}
