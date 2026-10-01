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
     * Build the common valkey-cli connection arguments (host, port, and — when
     * the fixture has them enabled — authentication and TLS options).
     *
     * @return string Shell-escaped option string, or null if no CLI is available.
     */
    private function shardCliBase(&$cli)
    {
        $cli = trim((string) shell_exec('command -v valkey-cli 2>/dev/null'));
        if ($cli === '') {
            $cli = trim((string) shell_exec('command -v redis-cli 2>/dev/null'));
        }
        if ($cli === '') {
            return null;
        }

        $opts = sprintf(
            '-c -h %s -p %d',
            escapeshellarg($this->getHost()),
            $this->getPort()
        );

        // Authentication (ACL username/password or password-only), if configured.
        $this->getAuthParts($user, $pass);
        if ($user !== null && $user !== '') {
            $opts .= ' --user ' . escapeshellarg($user);
        }
        if ($pass !== null && $pass !== '') {
            $opts .= ' -a ' . escapeshellarg($pass) . ' --no-auth-warning';
        }

        // TLS, if the fixture is running against a TLS endpoint.
        if ($this->getTLS()) {
            $opts .= ' --tls';
            if (defined('static::TLS_CERTIFICATE_PATH') && is_file(static::TLS_CERTIFICATE_PATH)) {
                $opts .= ' --cacert ' . escapeshellarg(static::TLS_CERTIFICATE_PATH);
            }
        }

        return $opts;
    }

    /**
     * Wait (with a bounded timeout) until the given sharded subscriber process
     * confirms its subscription.
     *
     * The confirmation is read from the subscriber's own stdout (valkey-cli
     * prints the "ssubscribe" push message with the channel name once the
     * SSUBSCRIBE is acknowledged). Using the subscriber's own connection means
     * this works regardless of authentication/TLS and does not depend on the
     * shared test client (which is constructed without TLS).
     *
     * @param resource $stdout       The subscriber's stdout pipe (non-blocking).
     * @return bool True if the subscription was confirmed within the timeout.
     */
    private function waitForShardSubscription($stdout, string $channel, float $timeoutSec = 5.0)
    {
        if (!is_resource($stdout)) {
            return false;
        }

        $seen     = '';
        $deadline = microtime(true) + $timeoutSec;
        do {
            $chunk = fread($stdout, 8192);
            if ($chunk !== false && $chunk !== '') {
                $seen .= $chunk;
                // valkey-cli prints the channel name on its own line as part of
                // the ssubscribe confirmation push.
                foreach (explode("\n", $seen) as $line) {
                    if (trim($line) === $channel) {
                        return true;
                    }
                }
            }
            usleep(100000);
        } while (microtime(true) < $deadline);

        return false;
    }

    /**
     * Start a sharded-channel subscriber in a background process using valkey-cli.
     *
     * External valkey-cli processes hold live sharded subscriptions while the
     * PHP client queries PUBSUB SHARDCHANNELS, keeping those tests independent
     * of the PHP client's own ssubscribe() implementation.
     *
     * A single valkey-cli SSUBSCRIBE can only cover channels in one slot (a
     * multi-channel subscribe across slots fails with CROSSSLOT), so callers
     * that need several channels should start one subscriber per channel.
     *
     * Authentication and TLS options from the fixture are forwarded to the CLI so
     * the subscriber can connect on secured deployments, and the method waits
     * (with a bounded timeout) for the subscription to actually register rather
     * than relying on a fixed sleep.
     *
     * @return array{proc: resource, pipes: array}|null Null if the CLI is
     *         unavailable or the subscription did not register in time.
     */
    private function startShardSubscriber(string $channel)
    {
        $baseOpts = $this->shardCliBase($cli);
        if ($baseOpts === null) {
            return null;
        }

        $cmd = sprintf(
            '%s %s ssubscribe %s',
            escapeshellarg($cli),
            $baseOpts,
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

        // Never block on the child's stdout/stderr pipes (the subscriber runs
        // indefinitely and keeps them open).
        if (is_resource($pipes[1])) {
            stream_set_blocking($pipes[1], false);
        }
        if (is_resource($pipes[2])) {
            stream_set_blocking($pipes[2], false);
        }

        $handle = ['proc' => $proc, 'pipes' => $pipes];

        // Wait for the subscriber's own SSUBSCRIBE confirmation (bounded), rather
        // than a fixed sleep. Reading the subscriber's stdout avoids depending on
        // the shared (non-TLS) test client for the readiness check.
        if (!$this->waitForShardSubscription($pipes[1], $channel)) {
            // Capture whatever child stderr is available without blocking, then
            // tear the process down.
            $stderr = is_resource($pipes[2]) ? (string) stream_get_contents($pipes[2]) : '';
            $this->stopShardSubscriber($handle);
            $this->assertTrue(
                false,
                "Sharded subscriber for '$channel' did not register in time"
                . ($stderr !== '' ? " (stderr: " . trim($stderr) . ")" : '')
            );
            return null;
        }

        return $handle;
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
     * sharded subscriber is created via valkey-cli so we can assert real
     * active-channel behaviour, matching the reference suites.
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

            // Explicit null must behave like an omitted pattern (the declared
            // default is null), not like an empty-string pattern.
            $nullArg = $this->valkey_glide->pubsub('shardchannels', null);
            $this->assertIsArray($nullArg);
            $this->assertContains($channel1, $nullArg);
            $this->assertContains($channel2, $nullArg);
            $this->assertContains($channel3, $nullArg);

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
     * Convert a flat NUMSUB/SHARDNUMSUB reply ([channel, count, ...]) into a
     * channel => count map so assertions do not depend on reply ordering.
     */
    private function numsubToMap(array $flat): array
    {
        $map = [];
        for ($i = 0; $i + 1 < count($flat); $i += 2) {
            $map[$flat[$i]] = $flat[$i + 1];
        }
        return $map;
    }

    /**
     * Start a PHP-native ssubscribe() subscriber in a background process. The
     * subscriber exits after receiving $message on $channel and unsubscribing,
     * either from the channel or, when $mode is 'all', via sunsubscribe().
     *
     * @return array{proc: resource, pipes: array, sync_file: string, result_file: string}
     */
    private function startNativeShardSubscriber(string $channel, string $message, string $mode = '')
    {
        $sync_file   = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');
        @unlink($sync_file);
        @unlink($result_file);

        $cmd = $this->buildSubscriberCommand(
            __DIR__ . '/scripts/subscriber_ssubscribe_cluster.php',
            '127.0.0.1',
            7001,
            $channel,
            $message,
            $sync_file,
            $result_file,
            $mode
        );

        $proc = proc_open($cmd, [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
        foreach ([1, 2] as $i) {
            if (isset($pipes[$i]) && is_resource($pipes[$i])) {
                stream_set_blocking($pipes[$i], false);
            }
        }

        return [
            'proc' => $proc,
            'pipes' => $pipes,
            'sync_file' => $sync_file,
            'result_file' => $result_file,
        ];
    }

    private function stopNativeShardSubscriber(array $handle)
    {
        foreach ($handle['pipes'] as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($handle['proc']);
        @proc_close($handle['proc']);
        @unlink($handle['sync_file']);
        @unlink($handle['result_file']);
        @unlink($handle['result_file'] . '.error');
    }

    /**
     * Poll PUBSUB SHARDNUMSUB until $channel reports $expected subscribers or the
     * timeout elapses. Returns the last observed count.
     */
    private function waitForShardNumSub(string $channel, int $expected, int $timeout_sec = 5): int
    {
        $count    = -1;
        $deadline = time() + $timeout_sec;
        do {
            $map   = $this->numsubToMap($this->valkey_glide->pubsub('shardnumsub', [$channel]));
            $count = $map[$channel] ?? -1;
            if ($count === $expected) {
                break;
            }
            usleep(100000);
        } while (time() < $deadline);
        return $count;
    }

    private function waitForFile(string $file, int $timeout_sec = 5): bool
    {
        $deadline = time() + $timeout_sec;
        while (!file_exists($file) && time() < $deadline) {
            usleep(100000);
        }
        return file_exists($file);
    }

    /**
     * PUBSUB SHARDNUMSUB: reports per-channel shard subscriber counts, using a
     * PHP-native ssubscribe() subscriber to produce a non-zero count.
     *
     * @see https://valkey.io/commands/pubsub-shardnumsub/
     */
    public function testPubSubShardNumSub()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $suffix   = uniqid();
        $channel  = 'test_shardnumsub_' . $suffix;
        $idle     = 'test_shardnumsub_idle_' . $suffix;
        $message  = 'shardnumsub_quit_' . $suffix;

        // No subscribers yet: every requested channel is reported with count 0.
        $map = $this->numsubToMap($this->valkey_glide->pubsub('shardnumsub', [$channel, $idle]));
        $this->assertEquals(0, $map[$channel] ?? null);
        $this->assertEquals(0, $map[$idle] ?? null);

        $handle = $this->startNativeShardSubscriber($channel, $message);
        try {
            $this->assertEquals(
                1,
                $this->waitForShardNumSub($channel, 1),
                'SHARDNUMSUB should report the PHP-native shard subscriber'
            );

            $map = $this->numsubToMap($this->valkey_glide->pubsub('shardnumsub', [$channel, $idle]));
            $this->assertEquals(1, $map[$channel] ?? null);
            $this->assertEquals(0, $map[$idle] ?? null, 'Unrelated shard channel should stay at 0');

            // Release the subscriber; its count must drop back to 0.
            $this->assertEquals(1, $this->valkey_glide->spublish($channel, $message));
            $this->assertTrue($this->waitForFile($handle['result_file']), 'Subscriber should receive the message');
            $this->assertEquals(0, $this->waitForShardNumSub($channel, 0));
        } finally {
            $this->stopNativeShardSubscriber($handle);
        }
    }

    /**
     * PUBSUB NUMSUB vs SHARDNUMSUB: a shard subscriber is counted only by
     * SHARDNUMSUB, never by the regular NUMSUB.
     */
    public function testPubSubNumSubAndShardNumSubSeparation()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $channel = 'test_numsub_separation_' . uniqid();
        $message = 'separation_quit';

        $handle = $this->startNativeShardSubscriber($channel, $message);
        try {
            $this->assertEquals(1, $this->waitForShardNumSub($channel, 1));

            $numsub = $this->numsubToMap($this->valkey_glide->pubsub('numsub', [$channel]));
            $this->assertEquals(0, $numsub[$channel] ?? null, 'NUMSUB must not count shard subscribers');

            // A regular PUBLISH does not reach the shard subscriber either.
            $this->assertEquals(0, $this->valkey_glide->publish($channel, $message));

            $this->valkey_glide->spublish($channel, $message);
            $this->assertTrue($this->waitForFile($handle['result_file']));
        } finally {
            $this->stopNativeShardSubscriber($handle);
        }
    }

    public function testPubSubShardNumSubRequiresArray()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $threw = false;
        try {
            $this->valkey_glide->pubsub('shardnumsub');
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw, 'SHARDNUMSUB without a channel array should throw');
    }

    /**
     * sunsubscribe() with no arguments unsubscribes from every shard channel and
     * ends the ssubscribe() loop.
     */
    public function testPubSubSUnsubscribeAll()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $channel = 'test_sunsubscribe_all_' . uniqid();
        $message = 'sunsubscribe_all_quit';

        $handle = $this->startNativeShardSubscriber($channel, $message, 'all');
        try {
            $this->assertEquals(1, $this->waitForShardNumSub($channel, 1));

            $this->valkey_glide->spublish($channel, $message);
            $this->assertTrue($this->waitForFile($handle['result_file']), 'Subscriber should receive the message');
            $this->assertEquals(0, $this->waitForShardNumSub($channel, 0), 'sunsubscribe() should drop all shard subscriptions');

            // The subscribe loop ends, so the subscriber process exits on its own.
            $deadline = time() + 5;
            while (proc_get_status($handle['proc'])['running'] && time() < $deadline) {
                usleep(100000);
            }
            $this->assertFalse(
                proc_get_status($handle['proc'])['running'],
                'ssubscribe() loop should exit after sunsubscribe()'
            );
        } finally {
            $this->stopNativeShardSubscriber($handle);
        }
    }

    public function testPubSubSSubscribeRequiresCallable()
    {
        $threw = false;
        try {
            $this->valkey_glide->ssubscribe(['test_ssubscribe_invalid'], 'no_such_function_' . uniqid());
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw, 'ssubscribe() with a non-callable callback should throw');
    }

    public function testPubSubSPublish()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $channel = 'test_spublish_' . uniqid();

        $count = $this->valkey_glide->spublish($channel, 'test_message');

        $this->assertIsInt($count, 'SPublish should return integer subscriber count');
        $this->assertEquals(0, $count, 'SPublish with no shard subscribers should return 0');
    }

    public function testPubSubSSubscribeMessageDelivery()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped('Sharded pub/sub requires Valkey/Redis 7.0+');
            return;
        }

        $channel = 'test_ssubscribe_' . uniqid();
        $message = 'shard_msg_' . time();
        $sync_file = tempnam(sys_get_temp_dir(), 'sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'result_');

        @unlink($sync_file);
        @unlink($result_file);

        $sub_script = __DIR__ . '/scripts/subscriber_ssubscribe_cluster.php';

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

        // The sync file is written just before ssubscribe() is issued, so retry
        // until the slot owner reports the shard subscriber.
        $count = 0;
        $timeout = time() + 5;
        while ($count < 1 && time() < $timeout) {
            $count = $this->valkey_glide->spublish($channel, $message);
            if ($count < 1) {
                usleep(100000);
            }
        }

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
        $received_channel = $success ? file_get_contents($result_file) : null;

        // Cleanup
        foreach ($pipes as $pipe) {
            @fclose($pipe);
        }
        @proc_terminate($proc);
        @proc_close($proc);
        @unlink($sync_file);
        @unlink($result_file);
        @unlink($error_file);

        $this->assertGTE(1, $count, 'SPublish should reach at least 1 shard subscriber');
        $this->assertTrue($success, 'Shard message should be delivered to ssubscribe callback');
        $this->assertEquals($channel, $received_channel, 'Callback should receive the shard channel name');
    }
}
