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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            '127.0.0.1',
            7001,
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
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            '127.0.0.1',
            7001,
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
        @unlink($error_file);

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

        $cmd = $this->buildSubscriberCommand(
            $sub_script,
            '127.0.0.1',
            7001,
            $pattern,
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
        @unlink($error_file);

        $this->assertTrue($success, 'Pattern subscription should work in cluster mode');
    }

    /**
     * Start a sharded-channel subscriber in a background process using valkey-cli.
     *
     * The PHP client does not yet implement SSUBSCRIBE (see follow-up issue), so
     * external valkey-cli processes are used to hold live sharded subscriptions
     * while the PHP client queries PUBSUB SHARDCHANNELS.
     *
     * A single valkey-cli SSUBSCRIBE can only cover channels in one slot (a
     * multi-channel subscribe across slots fails with CROSSSLOT), so callers
     * that need several channels should start one subscriber per channel.
     *
     * @return array{proc: resource, pipes: array}|null Null if valkey-cli is unavailable.
     */
    private function startShardSubscriber(string $channel)
    {
        $cli = trim((string) shell_exec('command -v valkey-cli 2>/dev/null'));
        if ($cli === '') {
            $cli = trim((string) shell_exec('command -v redis-cli 2>/dev/null'));
        }
        if ($cli === '') {
            return null;
        }

        $cmd = sprintf(
            '%s -c -h %s -p %d ssubscribe %s',
            escapeshellarg($cli),
            escapeshellarg($this->getHost()),
            $this->getPort(),
            escapeshellarg($channel)
        );

        $proc = proc_open(
            $cmd,
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
            $pipes
        );

        if (!is_resource($proc)) {
            return null;
        }

        // Give the subscription time to register on its owning node.
        usleep(400000);

        return ['proc' => $proc, 'pipes' => $pipes];
    }

    /**
     * Start one sharded subscriber per channel and return their handles.
     *
     * @return array<array{proc: resource, pipes: array}>|null Null if valkey-cli is unavailable.
     */
    private function startShardSubscribers(array $channels)
    {
        $handles = [];
        foreach ($channels as $channel) {
            $handle = $this->startShardSubscriber($channel);
            if ($handle === null) {
                // CLI unavailable: tear down anything already started and bail.
                foreach ($handles as $h) {
                    $this->stopShardSubscriber($h);
                }
                return null;
            }
            $handles[] = $handle;
        }
        return $handles;
    }

    private function stopShardSubscribers(array $handles)
    {
        foreach ($handles as $handle) {
            $this->stopShardSubscriber($handle);
        }
    }

    private function stopShardSubscriber($handle)
    {
        if (!is_array($handle)) {
            return;
        }
        foreach ($handle['pipes'] as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($handle['proc']);
        @proc_close($handle['proc']);
    }

    /**
     * PUBSUB SHARDCHANNELS: lists the currently active shard channels.
     *
     * Mirrors valkey-glide's cross-language pubsub_shardchannels tests. Sharded
     * pub/sub is a cluster-only feature available since Valkey/Redis 7.0. A live
     * sharded subscriber is created via valkey-cli (PHP-native SSUBSCRIBE is not
     * yet implemented) so we can assert real active-channel behaviour, matching
     * the reference suites.
     *
     * @see https://valkey.io/commands/pubsub-shardchannels/
     */
    public function testPubSubShardChannels()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('PUBSUB SHARDCHANNELS requires Valkey/Redis 7.0+');
            return;
        }

        // Empty state: no active sharded subscribers => empty list.
        $result = $this->valkey_glide->pubsub('shardchannels');
        $this->assertIsArray($result);

        // Bring up live sharded subscribers on multiple channels. Two share a
        // common prefix (so a glob pattern matches a subset) and one does not,
        // mirroring the Python/Go reference suites.
        $suffix   = uniqid();
        $channel1 = 'test_shardchannel1_' . $suffix;
        $channel2 = 'test_shardchannel2_' . $suffix;
        $channel3 = 'other_shardchannel3_' . $suffix;

        $handle = $this->startShardSubscribers([$channel1, $channel2, $channel3]);

        if ($handle === null) {
            // No valkey-cli/redis-cli available; fall back to shape-only checks.
            $this->assertIsArray($this->valkey_glide->pubsub('shardchannels', 'test_*'));
            $this->assertIsArray($this->valkey_glide->pubsub('shardchannels', 'non_matching_*'));
            return;
        }

        try {
            // Without a pattern: all three active channels must be listed.
            $channels = $this->valkey_glide->pubsub('shardchannels');
            $this->assertIsArray($channels);
            $this->assertContains($channel1, $channels);
            $this->assertContains($channel2, $channels);
            $this->assertContains($channel3, $channels);

            // With a glob pattern: only the matching subset is returned.
            $matched = $this->valkey_glide->pubsub('shardchannels', 'test_shardchannel*_' . $suffix);
            $this->assertIsArray($matched);
            $this->assertContains($channel1, $matched);
            $this->assertContains($channel2, $matched);
            $this->assertTrue(
                !in_array($channel3, $matched, true),
                'Pattern should exclude the non-matching channel'
            );

            // With a non-matching pattern: none of the channels are returned.
            $notMatched = $this->valkey_glide->pubsub('shardchannels', 'no_such_prefix_*');
            $this->assertIsArray($notMatched);
            foreach ([$channel1, $channel2, $channel3] as $ch) {
                $this->assertTrue(
                    !in_array($ch, $notMatched, true),
                    "Non-matching pattern should not include '$ch'"
                );
            }
        } finally {
            $this->stopShardSubscribers($handle);
        }
    }

    /**
     * PUBSUB CHANNELS vs SHARDCHANNELS: a sharded channel is reported by
     * SHARDCHANNELS but never by the regular CHANNELS introspection.
     */
    public function testPubSubChannelsAndShardChannelsSeparation()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $channel = 'test_separation_' . uniqid();
        $handle  = $this->startShardSubscriber($channel);

        if ($handle === null) {
            // No CLI available; assert both variants at least return arrays.
            $this->assertIsArray($this->valkey_glide->pubsub('channels'));
            $this->assertIsArray($this->valkey_glide->pubsub('shardchannels'));
            return;
        }

        try {
            $shardChannels = $this->valkey_glide->pubsub('shardchannels');
            $regularChannels = $this->valkey_glide->pubsub('channels');

            $this->assertIsArray($shardChannels);
            $this->assertIsArray($regularChannels);

            // The sharded channel appears in SHARDCHANNELS ...
            $this->assertContains($channel, $shardChannels);
            // ... but not in the regular CHANNELS listing.
            $this->assertTrue(
                !in_array($channel, $regularChannels, true),
                'Sharded channel must not appear in PUBSUB CHANNELS'
            );
        } finally {
            $this->stopShardSubscriber($handle);
        }
    }

    /**
     * PUBSUB SHARDNUMSUB is not yet supported by the PHP client and must raise
     * an exception until it is implemented (see follow-up issue).
     */
    public function testPubSubShardNumSubNotSupported()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $threw = false;
        try {
            $this->valkey_glide->pubsub('shardnumsub', ['some_channel']);
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw, 'SHARDNUMSUB should throw until it is implemented');
    }
}
