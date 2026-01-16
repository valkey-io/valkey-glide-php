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
}
