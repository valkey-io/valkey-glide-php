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

    /**
     * Wait until each channel has at least the expected number of subscribers.
     * If $isUnsubscribe is true, wait until each channel has at most the expected number of subscribers.
     *
     * @param ValkeyGlide $client Client used to query PUBSUB NUMSUB.
     * @param array<string, int> $expectedCounts Minimum/Maximum subscriber count for each channel.
     * @param float $timeoutSeconds Maximum time to poll, in seconds.
     * @param bool $isUnsubscribe Whether to check for at most the expected count (true) or at least the expected count (false).
     * @return bool Whether all minimum/maximum counts were reached before polling stopped.
     * @throws UnexpectedValueException If PUBSUB NUMSUB returns a non-array response.
     */
    private function waitForSubscribers(ValkeyGlide $client, array $expectedCounts, float $timeoutSeconds = 5, bool $isUnsubscribe = false): bool
    {
        $timeout = microtime(true) + $timeoutSeconds;
        $channels = array_keys($expectedCounts);

        do {
            $counts = $client->pubsub('numsub', $channels);
            if (!is_array($counts)) {
                throw new UnexpectedValueException('PUBSUB NUMSUB should return an array');
            }

            if (array_is_list($counts)) {
                $counts_by_channel = [];
                for ($i = 0; $i + 1 < count($counts); $i += 2) {
                    $counts_by_channel[$counts[$i]] = $counts[$i + 1];
                }
                $counts = $counts_by_channel;
            }

            $ready = true;
            foreach ($expectedCounts as $channel => $expectedCount) {
                if (!$isUnsubscribe && ($counts[$channel] ?? 0) < $expectedCount) {
                    $ready = false;
                    break;
                } elseif ($isUnsubscribe && ($counts[$channel] ?? 0) > $expectedCount) {
                    $ready = false;
                    break;
                }
            }

            if ($ready) {
                return true;
            }

            usleep(50000);
        } while (microtime(true) < $timeout);

        return false;
    }

    /**
     * Wait until the server reports at least the expected number of pattern subscriptions.
     *
     * PUBSUB NUMPAT reports a server-wide count, not the subscribed patterns themselves.
     *
     * @param ValkeyGlide $client Client used to query PUBSUB NUMPAT.
     * @param int $minimumCount Minimum number of active pattern subscriptions.
     * @param float $timeoutSeconds Maximum time to poll, in seconds.
     * @return bool Whether the minimum count was reached before polling stopped.
     * @throws UnexpectedValueException If PUBSUB NUMPAT returns a non-integer response.
     */
    private function waitForPatternSubscribers(ValkeyGlide $client, int $minimumCount, float $timeoutSeconds = 5): bool
    {
        $timeout = microtime(true) + $timeoutSeconds;

        do {
            $count = $client->pubsub('numpat');
            if (!is_int($count)) {
                throw new UnexpectedValueException('PUBSUB NUMPAT should return an integer');
            }

            if ($count >= $minimumCount) {
                return true;
            }

            usleep(50000);
        } while (microtime(true) < $timeout);

        return false;
    }

    /**
     * Wait for a subprocess signal or result file to contain data.
     *
     * @param string $path File to poll.
     * @param string $expectedContent Content to expect in the file.
     * @param float $timeoutSeconds Maximum time to poll, in seconds.
     * @return bool Whether the expected content was found.
     */
    private function waitForFileContent(string $path, string $expectedContent = '', float $timeoutSeconds = 5): bool
    {
        $timeout = microtime(true) + $timeoutSeconds;
        do {
            if (file_exists($path)) {
                $content = file_get_contents($path);
                if ($content !== false && $content === $expectedContent) {
                    return true;
                }
            }

            usleep(50000);
        } while (microtime(true) < $timeout);

        return false;
    }

    /**
     * Wait for non-empty content in a file.
     *
     * @param string $path File to poll.
     * @param float $timeoutSeconds Maximum time to poll, in seconds.
     * @return string|null Content read from file, or null if timeout reached.
     */
    private function waitForFileToHaveContent(string $path, float $timeoutSeconds = 5): ?string
    {
        $timeout = microtime(true) + $timeoutSeconds;
        do {
            if (file_exists($path)) {
                $content = file_get_contents($path);
                if ($content !== false && $content !== '') {
                    return $content;
                }
            }

            usleep(50000);
        } while (microtime(true) < $timeout);

        return null;
    }

    /* Wait until a file's content starts with $prefix */
    private function waitForFileToMatch(string $path, string $prefix, float $timeoutSeconds = 5): ?string
    {
        $timeout = microtime(true) + $timeoutSeconds;
        do {
            $content = @file_get_contents($path);
            if ($content !== false && str_starts_with($content, $prefix)) {
                return $content;
            }
            usleep(50000);
        } while (microtime(true) < $timeout);

        return null;
    }

    /* Wait until the server holds no unsent output for any subscribed client */
    private function waitForSubscriberOutputFlushed(ValkeyGlide $client, float $timeoutSeconds = 5): bool
    {
        $timeout = microtime(true) + $timeoutSeconds;
        do {
            $flushed = true;
            foreach (explode("\n", trim($client->rawcommand('CLIENT', 'LIST'))) as $line) {
                if (preg_match('/ sub=([1-9]\d*) /', $line) && !preg_match('/ omem=0 /', $line)) {
                    $flushed = false;
                }
            }
            if ($flushed) {
                return true;
            }
            usleep(50000);
        } while (microtime(true) < $timeout);

        return false;
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

    /**
     * Verify publishing reports two subscribers and delivers the message to both.
     */
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);

        // Wait until both subscriptions have been registered by the server.
        $timeout = time() + 5;
        while ((!file_exists($sync_file1) || !file_exists($sync_file2)) && time() < $timeout) {
            usleep(100000);
        }

        $subscribers_ready = file_exists($sync_file1)
            && file_exists($sync_file2)
            && $this->waitForSubscribers($pub, [$channel => 2]);
        if (!$subscribers_ready) {
            $pub->close();
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

            $this->assertTrue(
                false,
                'Both subscribers should be ready and registered before publishing'
            );
            return;
        }

        $count = $pub->publish($channel, $message);
        $pub->close();

        $this->assertIsInt($count, 'Publish should return integer subscriber count');
        $this->assertEquals(2, $count, 'Subscriber count should be 2');

        // Wait for both callbacks
        $success1 = false;
        $success2 = false;
        $timeout = microtime(true) + 5;
        while ((!$success1 || !$success2) && microtime(true) < $timeout) {
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

        $this->assertTrue($success1 && $success2, 'Both subscribers should receive message');
    }

    /**
     * Verify a registered subscriber receives the published channel and message.
     */
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
            return;
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
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel => 1]),
            'Subscriber should be registered before publishing'
        );
        $count = $pub->publish($channel, $message);
        $pub->close();

        $this->assertGTE(1, $count, 'Should have at least 1 subscriber');

        // Wait for callback result
        $success = $this->waitForFileContent($result_file, 'SUCCESS');

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

    /**
     * Verify a message larger than the subscriber queue's 64 MiB byte cap is
     * still delivered when the queue is empty, followed by a small one.
     */
    public function testPubSubOversizedMessage()
    {
        if ($this->getTLS() || $this->getAuth()) {
            $this->markTestSkipped('The subscriber script connects without TLS or auth');
        }

        $channel = 'test_oversized_' . uniqid();
        $big_len = 64 * 1024 * 1024 + 1;
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        $error_file = $result_file . '.error';
        @unlink($sync_file);
        @unlink($result_file);

        // The default pub/sub output buffer hard limit (32 MiB) would make the
        // server disconnect the subscriber instead of sending the message
        $admin = new ValkeyGlide();
        $admin->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $limits = $admin->config('GET', 'client-output-buffer-limit')['client-output-buffer-limit'];
        $admin->config('SET', 'client-output-buffer-limit', 'pubsub 0 0 0');

        $cmd = $this->buildSubscriberCommand(
            __DIR__ . '/scripts/subscriber_oversized_message.php',
            $this->getHost(),
            $this->getPort(),
            $channel,
            $sync_file,
            $result_file
        );
        $proc = proc_open($cmd, [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);

        try {
            $this->assertTrue($this->waitForFileToHaveContent($sync_file) !== null, 'Subscriber should signal ready');
            if (file_exists($error_file)) {
                $this->fail('Subscriber script error: ' . file_get_contents($error_file));
            }
            $this->assertTrue(
                $this->waitForSubscribers($admin, [$channel => 1]),
                'Subscriber should be registered before publishing'
            );

            // While the callback is blocked, the oversized message enters the
            // empty queue and pushes it over the byte cap, so the small
            // messages after it are dropped
            $this->assertEquals(1, $admin->publish($channel, 'block'));
            $this->assertNotNull(
                $this->waitForFileToHaveContent($result_file . '.blocked', 10),
                'The callback should block on the first message'
            );
            $this->assertEquals(1, $admin->publish($channel, str_repeat('x', $big_len)));
            for ($i = 0; $i < 3; $i++) {
                $this->assertEquals(1, $admin->publish($channel, 'x'));
            }
            $this->assertTrue(
                $this->waitForSubscriberOutputFlushed($admin, 30),
                'The server should finish sending the messages to the subscriber'
            );
            // The server has written everything to the socket; give the client
            // a moment to read it into the queue before releasing the callback
            usleep(500000);
            touch($result_file . '.release');

            // Publish the sentinel once the oversized message has left the queue
            $this->assertNotNull($this->waitForFileToMatch(
                $result_file . '.progress',
                strlen('block') . ',' . $big_len,
                30
            ));
            $this->assertEquals(1, $admin->publish($channel, 'sentinel'));

            $this->assertEquals(
                strlen('block') . ',' . $big_len . ',' . strlen('sentinel'),
                $this->waitForFileToHaveContent($result_file, 30),
                'The oversized message and the sentinel should be delivered, the messages queued behind it dropped'
            );
        } finally {
            $admin->config('SET', 'client-output-buffer-limit', $limits);
            $admin->close();
            foreach ($pipes as $pipe) {
                @fclose($pipe);
            }
            if (is_resource($proc)) {
                @proc_terminate($proc);
                @proc_close($proc);
            }
            @unlink($sync_file);
            @unlink($result_file);
            @unlink($error_file);
            foreach (['.blocked', '.release', '.progress'] as $suffix) {
                @unlink($result_file . $suffix);
            }
        }
    }

    /**
     * Verify unsubscribing from a callback ends the subscription loop.
     */
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
            return;
        }

        // Publish to trigger callback
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel => 1]),
            'Subscriber should be registered before publishing'
        );
        $pub->publish($channel, 'trigger');
        $pub->close();

        // Wait for unsubscribe signal
        $success = $this->waitForFileContent($unsub_file, 'unsubscribed') === true;

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

    /**
     * Wait for a subscriber process to exit by itself, draining its output so
     * it cannot block on a full pipe. Returns its exit code, null on timeout.
     */
    private function waitForExit($proc, array $pipes, float $timeoutSeconds = 5): ?int
    {
        // Pipe 0 is the subscriber's stdin, which the parent can only write to
        $output = [$pipes[1], $pipes[2]];
        foreach ($output as $pipe) {
            stream_set_blocking($pipe, false);
        }
        $timeout = microtime(true) + $timeoutSeconds;
        do {
            foreach ($output as $pipe) {
                stream_get_contents($pipe);
            }
            $status = proc_get_status($proc);
            if (!$status['running']) {
                return $status['exitcode'];
            }
            usleep(50000);
        } while (microtime(true) < $timeout);
        return null;
    }

    /**
     * Verify closing the client from a callback ends the subscription loop.
     */
    public function testPubSubCloseInCallback()
    {
        $channel = 'test_close_cb_' . uniqid();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_close_in_callback.php';

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

        // Wait for subscriber ready
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
            return;
        }

        // Publish to trigger the callback that closes the client
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel => 1]),
            'Subscriber should be registered before publishing'
        );
        $pub->publish($channel, 'trigger');
        $pub->close();

        // subscribe() hung here before close() ended the loop
        $success = $this->waitForFileContent($result_file, 'returned') === true;
        $result = @file_get_contents($result_file);
        // A crash while the subscriber shuts down shows only in its exit code
        $exit_code = $success ? $this->waitForExit($proc, $pipes) : null;

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

        $this->assertTrue($success, 'close() in the callback should end the subscribe loop');
        $this->assertEquals('returned', $result, 'A client reconnected in the callback should work');
        $this->assertEquals(0, $exit_code, 'The subscriber should exit cleanly');
    }

    /**
     * Verify OPT_PREFIX applies to SUBSCRIBE, PUBLISH and PUBSUB NUMSUB channels, as in PHPRedis.
     */
    public function testPubSubPrefix()
    {
        $prefix = 'pfx_' . uniqid() . ':';
        $channel = 'test_prefix';
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_prefix.php';

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            $this->getHost(),
            $this->getPort(),
            $prefix,
            $channel,
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

        // The subscriber is on the prefixed channel
        $raw = new ValkeyGlide();
        $raw->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($raw, [$prefix . $channel => 1]),
            'Subscriber should be registered on the prefixed channel'
        );
        $raw->close();

        // A prefixed client counts and publishes on the prefixed channel too
        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $pub->setOption(ValkeyGlide::OPT_PREFIX, $prefix);
        $this->assertEquals([$prefix . $channel, 1], $pub->pubsub('numsub', [$channel]));
        $this->assertEquals(1, $pub->publish($channel, 'hello'));
        $pub->close();

        $success = $this->waitForFileContent($result_file, $prefix . $channel . '|hello') === true;

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

        $this->assertTrue($success, 'The callback should receive the prefixed channel and message');
    }

    /**
     * Verify the second channel still delivers messages after unsubscribing from the first.
     */
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
            $result_file,
            $result_file . '.unsubscribed'
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
            return;
        }

        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel1 => 1, $channel2 => 1]),
            'Both channel subscriptions should be registered before publishing'
        );
        $pub->publish($channel1, 'msg1');
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel1 => 0, $channel2 => 1], 5, true),
            'First channel should be unsubscribed before publishing to second channel'
        );
        $unsubscribed_file = $result_file . '.unsubscribed';
        $timeout = microtime(true) + 5;
        while (!file_exists($unsubscribed_file) && microtime(true) < $timeout) {
            usleep(100000);
        }
        $this->assertTrue(file_exists($unsubscribed_file), 'First channel should be unsubscribed');
        $pub->publish($channel2, 'msg2');
        $pub->close();

        $success = $this->waitForFileContent($result_file, 'SUCCESS');

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($unsubscribed_file);
        @unlink($error_file);

        $this->assertTrue($success, 'Should receive message on second channel after unsubscribing from first');
    }

    /**
     * Verify regular commands are rejected while the client is in subscribe mode.
     */
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
            return;
        }

        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel => 1]),
            'Subscriber should be registered before publishing'
        );
        $pub->publish($channel, 'trigger');
        $pub->close();

        $success = $this->waitForFileContent($result_file, 'PASS');

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

    /**
     * Verify another subscribe call is rejected while the client is in subscribe mode.
     */
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
            return;
        }

        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel => 1]),
            'Subscriber should be registered before publishing'
        );

        $result = null;
        $timeout = microtime(true) + 5;
        while ($result === null && microtime(true) < $timeout) {
            $result = $this->waitForFileToHaveContent($result_file, 0.1);
            if ($result !== null) {
                break;
            }
            $pub->publish($channel, 'trigger');
        }
        $pub->close();

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

        $this->assertTrue($result === 'PASS', 'Subscribe should be blocked during subscribe mode');
    }

    /**
     * Verify a pattern subscription receives a message on a matching channel.
     */
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

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $initial_pattern_count = $pub->pubsub('numpat');
        if (!is_int($initial_pattern_count)) {
            throw new UnexpectedValueException('PUBSUB NUMPAT should return an integer');
        }

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
            return;
        }

        $this->assertTrue(file_exists($sync_file), 'Subscriber should signal ready');

        $this->assertTrue(
            $this->waitForPatternSubscribers($pub, $initial_pattern_count + 1),
            'Pattern subscription should be registered on the server before publishing'
        );

        $pub->publish($channel, 'pattern_message');

        $this->assertTrue(
            $this->waitForFileContent($result_file, 'pattern_message', 5),
            'Pattern subscribe should receive matching messages'
        );
        $pub->close();

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);
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

    /**
     * Verify unsubscribed channels stop delivering while remaining channels still deliver.
     */
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
            $result_file,
            $result_file . '.unsubscribed'
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
            return;
        }

        $this->assertTrue(file_exists($sync_file));

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel1 => 1, $channel2 => 1]),
            'Both channel subscriptions should be registered before publishing'
        );
        $pub->publish($channel1, 'bing');
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel1 => 0, $channel2 => 1], 5, true),
            'First channel should be unsubscribed before publishing to second channel'
        );
        $unsubscribed_file = $result_file . '.unsubscribed';
        $timeout = microtime(true) + 5;
        while (!file_exists($unsubscribed_file) && microtime(true) < $timeout) {
            usleep(100000);
        }
        $this->assertTrue(file_exists($unsubscribed_file), 'First channel should be unsubscribed');
        $pub->publish($channel1, 'should_not_receive');
        $pub->publish($channel2, 'bong');
        $pub->close();

        $content = $this->waitForFileToHaveContent($result_file);

        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($unsubscribed_file);
        @unlink($error_file);

        $this->assertNotNull($content);
        $messages = $content === null ? [] : explode(',', $content);
        // Should have received bing on channel1 and bong on channel2, but NOT should_not_receive
        $this->assertContains($channel1 . ':bing', $messages);
        $this->assertContains($channel2 . ':bong', $messages);
        // Verify we didn't receive message after unsubscribing from channel1
        $has_should_not_receive = in_array($channel1 . ':should_not_receive', $messages);
        $this->assertFalse($has_should_not_receive);
    }

    /**
     * Verify unsubscribing from an unknown channel preserves the active subscription.
     */
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
            return;
        }

        $this->assertTrue(file_exists($sync_file));

        $pub = new ValkeyGlide();
        $pub->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);
        $this->assertTrue(
            $this->waitForSubscribers($pub, [$channel1 => 1]),
            'Subscriber should be registered before publishing'
        );
        $pub->publish($channel1, 'test_message');
        $pub->close();

        $success = $this->waitForFileContent($result_file, 'RECEIVED') === true;

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

    /* The pub/sub callback registry is request memory: a PHP-FPM style worker must
     * be able to subscribe again in later requests after it was released */
    public function testSubscribeInConsecutiveRequests()
    {
        if ($this->getTLS() || $this->getAuth()) {
            $this->markTestSkipped('The web server helper connects without TLS or auth');
        }
        if (!function_exists('proc_open')) {
            $this->markTestSkipped('proc_open is not available');
        }

        $probe = stream_socket_server('tcp://127.0.0.1:0');
        $web_port = (int) substr(strrchr(stream_socket_get_name($probe, false), ':'), 1);
        fclose($probe);

        $extension_path = __DIR__ . '/../modules/valkey_glide.so';
        $extension = file_exists($extension_path) ? $extension_path : 'valkey_glide';
        /* One server process, so every request runs in the same worker */
        $env = getenv();
        unset($env['PHP_CLI_SERVER_WORKERS']);
        $server = proc_open(
            [PHP_BINARY, '-n', '-d', "extension=$extension", '-S', "127.0.0.1:$web_port", __DIR__ . '/scripts/pubsub_web.php'],
            [1 => ['file', '/dev/null', 'w'], 2 => ['file', '/dev/null', 'w']],
            $pipes,
            null,
            $env
        );
        if (!$this->assertTrue(is_resource($server), 'Failed to start the web server')) {
            return;
        }

        $publisher = null;
        try {
            $publisher = new ValkeyGlide();
            $publisher->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);

            for ($request = 1; $request <= 3; $request++) {
                $channel = 'pubsub_requests_' . uniqid();
                $query = http_build_query(['host' => $this->getHost(), 'port' => $this->getPort(), 'channel' => $channel]);

                /* Wait for the server to start */
                $socket = false;
                for ($attempt = 0; $attempt < 50 && !$socket; $attempt++) {
                    $socket = @stream_socket_client("tcp://127.0.0.1:$web_port", $errno, $errstr, 1);
                    if (!$socket) {
                        usleep(100000);
                    }
                }
                if (!$this->assertTrue((bool) $socket, 'The web server did not answer')) {
                    return;
                }
                fwrite($socket, "GET /?$query HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n");
                stream_set_blocking($socket, false);

                /* Publish until the request (subscribed, then unsubscribed on the first message) returns */
                $response = '';
                $deadline = microtime(true) + 5;
                while (!feof($socket) && microtime(true) < $deadline) {
                    $publisher->publish($channel, "message $request");
                    $read = [$socket];
                    $write = $except = null;
                    if (stream_select($read, $write, $except, 0, 100000)) {
                        $response .= fread($socket, 8192);
                    }
                }
                fclose($socket);

                $body = substr($response, (int) strpos($response, "\r\n\r\n") + 4);
                $this->assertEquals(['received' => "message $request"], json_decode($body, true));
            }
        } finally {
            $publisher?->close();
            proc_terminate($server);
            proc_close($server);
        }
    }
}
