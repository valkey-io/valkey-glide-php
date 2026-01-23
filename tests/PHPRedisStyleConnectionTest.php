<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die('Use TestValkeyGlide.php to run tests!\n');
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

/**
 * Tests for PHPRedis-style connection patterns
 */

require_once __DIR__ . '/TestSuite.php';

class PHPRedisStyleConnectionTest extends TestSuite
{
    /**
     * Test PHPRedis-style connection with host and port
     */
    public function testConnectWithHostPort()
    {
        $client = new ValkeyGlide();
        $result = $client->connect($this->getHost(), $this->getPort());

        $this->assertTrue($result, 'connect() should return true on success');

        // Verify connection works
        $pingResult = $client->ping();
        $this->assertEquals('PONG', $pingResult, 'PING should return PONG');

        $client->close();
    }

    /**
     * Test PHPRedis-style connection with timeout
     */
    public function testConnectWithTimeout()
    {
        $client = new ValkeyGlide();
        $result = $client->connect($this->getHost(), $this->getPort(), 2.5);

        $this->assertTrue($result, 'connect() with timeout should return true');

        // Test basic operation
        $client->set('timeout_test', 'value');
        $value = $client->get('timeout_test');
        $this->assertEquals('value', $value, 'GET should return set value');

        $client->del(['timeout_test']);
        $client->close();
    }

    /**
     * Test connection failure returns false
     */
    public function testConnectFailureReturnsFalse()
    {
        $client = new ValkeyGlide();

        try {
            $result = $client->connect('nonexistent-host', 9999, 1.0);
            $this->fail('connect() should throw ValkeyGlideException for invalid host');
        } catch (ValkeyGlideException $e) {
            // Verify exception message contains the error details
            $this->assertStringContainsString(
                'nonexistent-host:9999',
                $e->getMessage(),
                'Exception should mention the failed address'
            );
            $this->assertStringContainsString(
                'timed out',
                $e->getMessage(),
                'Exception should mention timeout error'
            );
        }
    }

    /**
     * Test mixing PHPRedis and ValkeyGlide style parameters fails
     */
    public function testMixedParametersFails()
    {
        $client = new ValkeyGlide();

        try {
            // This should fail - mixing positional host with named addresses
            $client->connect($this->getHost(), addresses: [['host' => 'localhost', 'port' => 6379]]);
            $this->fail('Should throw exception for conflicting parameters');
        } catch (Exception $e) {
            $this->assertStringContainsString('Cannot specify both', $e->getMessage());
        }
    }

    /**
     * Test ValkeyGlide-style connection with addresses array
     */
    public function testConnectWithAddressesArray()
    {
        $client = new ValkeyGlide();
        $result = $client->connect(addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]]);

        $this->assertTrue($result, 'connect() with addresses should return true');

        $client->set('addresses_test', 'works');
        $value = $client->get('addresses_test');
        $this->assertEquals('works', $value);

        $client->del(['addresses_test']);
        $client->close();
    }

    /**
     * Test Redis alias works with PHPRedis-style connection
     */
    public function testRedisAliasConnection()
    {
        if (PHP_VERSION_ID < 80300) {
            $this->markTestSkipped('PHPRedis aliases require PHP 8.3+');
            return;
        }

        require_once __DIR__ . '/../phpredis_aliases.php';

        $this->assertTrue(class_exists('Redis'), 'Redis class alias should exist');
        $this->assertTrue(class_exists('RedisException'), 'RedisException class alias should exist');

        $redis = new Redis();
        $result = $redis->connect($this->getHost(), $this->getPort());

        $this->assertTrue($result, 'Redis alias connect() should work');
        $this->assertTrue($redis instanceof Redis, 'Instance should be Redis');
        $this->assertTrue($redis instanceof ValkeyGlide, 'Instance should be ValkeyGlide');

        $redis->set('alias_test', 'value');
        $value = $redis->get('alias_test');
        $this->assertEquals('value', $value);

        $redis->del(['alias_test']);

        try {
            $badRedis = new Redis();
            $badRedis->connect('localhost', 9999, 1.0);
            $badRedis->ping();
            $this->fail('Expected RedisException to be thrown');
        } catch (RedisException $e) {
            $this->assertTrue($e instanceof RedisException, 'Exception should be RedisException');
            $this->assertTrue($e instanceof ValkeyGlideException, 'Exception should be ValkeyGlideException');
        }

        $redis->close();
    }

    /**
     * Test double connect throws exception
     */
    public function testDoubleConnectThrowsException()
    {
        $client = new ValkeyGlide();
        $client->connect($this->getHost(), $this->getPort());

        try {
            $client->connect($this->getHost(), $this->getPort());
            $this->fail('Second connect() should throw exception');
        } catch (Exception $e) {
            $this->assertStringContainsString('already connected', $e->getMessage());
        }

        $client->close();
    }

    /**
     * Test ValkeyGlideCluster with PHPRedis RedisCluster-style seeds parameter
     */
    public function testClusterConnectWithSeeds()
    {
        $seeds = [
            ['host' => '127.0.0.1', 'port' => 7001],
            ['host' => '127.0.0.1', 'port' => 7002],
        ];

        $cluster = new ValkeyGlideCluster(name: null, seeds: $seeds);

        $pingResult = $cluster->ping();
        $this->assertEquals('PONG', $pingResult, 'Cluster PING should return PONG');

        $cluster->close();
    }

    /**
     * Test ValkeyGlideCluster with timeout parameter
     */
    public function testClusterConnectWithTimeout()
    {
        $seeds = [['host' => '127.0.0.1', 'port' => 7001]];

        $cluster = new ValkeyGlideCluster(name: null, seeds: $seeds, timeout: 2.5);

        $cluster->set('cluster_timeout_test', 'value');
        $value = $cluster->get('cluster_timeout_test');
        $this->assertEquals('value', $value);

        $cluster->del(['cluster_timeout_test']);
        $cluster->close();
    }

    /**
     * Test ValkeyGlideCluster with auth parameter (string password)
     */
    public function testClusterConnectWithStringAuth()
    {
        if (!$this->getAuth()) {
            $this->markTestSkipped('Test requires authentication');
            return;
        }

        $seeds = [['host' => '127.0.0.1', 'port' => 7001]];
        $password = is_array($this->getAuth()) ? $this->getAuth()['password'] : $this->getAuth();

        $cluster = new ValkeyGlideCluster(name: null, seeds: $seeds, auth: $password);

        $pingResult = $cluster->ping();
        $this->assertEquals('PONG', $pingResult);

        $cluster->close();
    }

    /**
     * Test ValkeyGlideCluster with auth parameter (array with username and password)
     */
    public function testClusterConnectWithArrayAuth()
    {
        if (!$this->getAuth() || !is_array($this->getAuth())) {
            $this->markTestSkipped('Test requires array authentication');
            return;
        }

        $seeds = [['host' => '127.0.0.1', 'port' => 7001]];
        $auth = [$this->getAuth()['username'], $this->getAuth()['password']];

        $cluster = new ValkeyGlideCluster(name: null, seeds: $seeds, auth: $auth);

        $pingResult = $cluster->ping();
        $this->assertEquals('PONG', $pingResult);

        $cluster->close();
    }

    /**
     * Test ValkeyGlideCluster with ValkeyGlide-style addresses parameter
     */
    public function testClusterConnectWithAddresses()
    {
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 7001],
            ['host' => '127.0.0.1', 'port' => 7002],
        ];

        $cluster = new ValkeyGlideCluster(addresses: $addresses);

        $cluster->set('cluster_addresses_test', 'works');
        $value = $cluster->get('cluster_addresses_test');
        $this->assertEquals('works', $value);

        $cluster->del(['cluster_addresses_test']);
        $cluster->close();
    }

    /**
     * Test ValkeyGlideCluster mixing PHPRedis and ValkeyGlide parameters fails
     */
    public function testClusterMixedParametersFails()
    {
        try {
            $cluster = new ValkeyGlideCluster(
                seeds: [['host' => '127.0.0.1', 'port' => 7001]],
                addresses: [['host' => '127.0.0.1', 'port' => 7002]]
            );
            $this->fail('Should throw exception for conflicting parameters');
        } catch (Exception $e) {
            $this->assertStringContainsString('Cannot specify both', $e->getMessage());
        }
    }

    /**
     * Test ValkeyGlideCluster with ValkeyGlide-style credentials parameter
     */
    public function testClusterConnectWithCredentials()
    {
        if (!$this->getAuth()) {
            $this->markTestSkipped('Test requires authentication');
            return;
        }

        $addresses = [['host' => '127.0.0.1', 'port' => 7001]];
        $credentials = is_array($this->getAuth()) ? $this->getAuth() : ['password' => $this->getAuth()];

        $cluster = new ValkeyGlideCluster(addresses: $addresses, credentials: $credentials);

        $pingResult = $cluster->ping();
        $this->assertEquals('PONG', $pingResult);

        $cluster->close();
    }

    /**
     * Test RedisCluster alias works with PHPRedis-style connection
     */
    public function testRedisClusterAliasConnection()
    {
        if (PHP_VERSION_ID < 80300) {
            $this->markTestSkipped('PHPRedis aliases require PHP 8.3+');
            return;
        }

        require_once __DIR__ . '/../phpredis_aliases.php';

        $this->assertTrue(class_exists('RedisCluster'), 'RedisCluster class alias should exist');
        $this->assertTrue(class_exists('RedisException'), 'RedisException class alias should exist');

        $seeds = [['host' => '127.0.0.1', 'port' => 7001]];
        $cluster = new RedisCluster(name: null, seeds: $seeds);

        $this->assertTrue($cluster instanceof RedisCluster, 'Instance should be RedisCluster');
        $this->assertTrue($cluster instanceof ValkeyGlideCluster, 'Instance should be ValkeyGlideCluster');

        $pingResult = $cluster->ping();
        $this->assertEquals('PONG', $pingResult);

        $cluster->set('alias_cluster_test', 'value');
        $value = $cluster->get('alias_cluster_test');
        $this->assertEquals('value', $value);

        $cluster->del(['alias_cluster_test']);

        try {
            $badCluster = new RedisCluster(name: null, seeds: [['host' => 'localhost', 'port' => 9999]]);
            $badCluster->ping();
            $this->fail('Expected RedisException to be thrown');
        } catch (RedisException $e) {
            $this->assertTrue($e instanceof RedisException, 'Exception should be RedisException');
            $this->assertTrue($e instanceof ValkeyGlideException, 'Exception should be ValkeyGlideException');
        }

        $cluster->close();
    }
}
