<?php

/*
* --------------------------------------------------------------------
*                   The PHP License, version 3.01
* Copyright (c) 1999 - 2010 The PHP Group. All rights reserved.
* --------------------------------------------------------------------
*
* Redistribution and use in source and binary forms, with or without
* modification, is permitted provided that the following conditions
* are met:
*
*   1. Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in
*      the documentation and/or other materials provided with the
*      distribution.
*
*   3. The name "PHP" must not be used to endorse or promote products
*      derived from this software without prior written permission. For
*      written permission, please contact group@php.net.
*
*   4. Products derived from this software may not be called "PHP", nor
*      may "PHP" appear in their name, without prior written permission
*      from group@php.net.  You may indicate that your software works in
*      conjunction with PHP by saying "Foo for PHP" instead of calling
*      it "PHP Foo" or "phpfoo"
*
*   5. The PHP Group may publish revised and/or new versions of the
*      license from time to time. Each version will be given a
*      distinguishing version number.
*      Once covered code has been published under a particular version
*      of the license, you may always continue to use it under the terms
*      of that version. You may also choose to use such covered code
*      under the terms of any subsequent version of the license
*      published by the PHP Group. No one other than the PHP Group has
*      the right to modify the terms applicable to covered code created
*      under this License.
*
*   6. Redistributions of any form whatsoever must retain the following
*      acknowledgment:
*      "This product includes PHP software, freely available from
*      <http://www.php.net/software/>".
*
* THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
* ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
* DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* --------------------------------------------------------------------
*
* This software consists of voluntary contributions made by many
* individuals on behalf of the PHP Group.
*
* The PHP Group can be contacted via Email at group@php.net.
*
* For more information on the PHP Group and the PHP project,
* please see <http://www.php.net>.
*
* PHP includes the Zend Engine, freely available at
* <http://www.zend.com>.
*/

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
        // Test publish command works with 0 subscribers
        $channel = 'test_publish_' . uniqid();

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $count = $pub->publish($channel, 'test_message');
        $pub->close();

        $this->assertIsInt($count, 'Publish should return integer subscriber count');
        $this->assertEquals(0, $count, 'Subscriber count should be 0 when no subscribers');
    }

    public function testPubSubPublishWithTwoSubscribers()
    {
        // Test publish returns correct count with 2 subscribers
        $channel = 'test_publish_multi_' . uniqid();
        $sync_file1 = tempnam(sys_get_temp_dir(), 'sync1_');
        $sync_file2 = tempnam(sys_get_temp_dir(), 'sync2_');
        $result_file1 = tempnam(sys_get_temp_dir(), 'result1_');
        $result_file2 = tempnam(sys_get_temp_dir(), 'result2_');

        @unlink($sync_file1);
        @unlink($sync_file2);
        @unlink($result_file1);
        @unlink($result_file2);

        $sub_script = __DIR__ . '/scripts/subscriber_message_delivery.php';
        $message = 'multi_test_' . time();

        // Start first subscriber
        $cmd1 = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $message,
            $sync_file1,
            $result_file1
        );

        $proc1 = proc_open(
            $cmd1,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes1
        );

        // Start second subscriber
        $cmd2 = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $channel,
            $message,
            $sync_file2,
            $result_file2
        );

        $proc2 = proc_open(
            $cmd2,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes2
        );

        // Wait for both subscribers ready
        $timeout = time() + 5;
        while ((!file_exists($sync_file1) || !file_exists($sync_file2)) && time() < $timeout) {
            usleep(100000);
        }

        $this->assertTrue(file_exists($sync_file1), 'First subscriber should signal ready');
        $this->assertTrue(file_exists($sync_file2), 'Second subscriber should signal ready');

        // Publish message
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $count = $pub->publish($channel, $message);
        $pub->close();

        $this->assertIsInt($count, 'Publish should return integer subscriber count');
        $this->assertEquals(2, $count, 'Subscriber count should be 2');

        // Wait for both callbacks
        $success1 = false;
        $success2 = false;
        $timeout = time() + 3;
        while ((!$success1 || !$success2) && time() < $timeout) {
            if (file_exists($result_file1)) {
                $success1 = true;
            }
            if (file_exists($result_file2)) {
                $success2 = true;
            }
            if ($success1 && $success2) {
                break;
            }
            usleep(100000);
        }

        // Cleanup
        foreach ($pipes1 as $pipe) {
            @fclose($pipe);
        }
        foreach ($pipes2 as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc1);
        @proc_terminate($proc2);
        @proc_close($proc1);
        @proc_close($proc2);
        @unlink($sync_file1);
        @unlink($sync_file2);
        @unlink($result_file1);
        @unlink($result_file2);
        @unlink($result_file1 . '.error');
        @unlink($result_file2 . '.error');

        $this->assertTrue($success1 || $success2, 'At least one subscriber should receive message');
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
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
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

    public function testPubSubUnsubscribeNonExistentChannel()
    {
        // Test edge case: unsubscribe from channel we never subscribed to
        // This should not break the subscription loop for other channels
        $channel1 = 'test_real_' . uniqid();
        $channel2 = 'test_fake_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_unsubscribe_nonexistent.php';

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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->publish($channel1, 'test_message');
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

        $this->assertTrue($success, 'Should still receive messages after unsubscribing from non-existent channel');
    }
}
