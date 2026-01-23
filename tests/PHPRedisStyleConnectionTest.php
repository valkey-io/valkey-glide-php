<?php

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
            $this->assertFalse($result, 'connect() should return false for invalid host');
        } catch (Exception $e) {
            // Exception is also acceptable for connection failure
            $this->assertTrue(true, 'Exception thrown for connection failure');
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
        if (!class_exists('Redis')) {
            $this->markTestSkipped('Redis alias not available');
            return;
        }

        $redis = new Redis();
        $result = $redis->connect($this->getHost(), $this->getPort());

        $this->assertTrue($result, 'Redis alias connect() should work');

        $redis->set('alias_test', 'value');
        $value = $redis->get('alias_test');
        $this->assertEquals('value', $value);

        $redis->del(['alias_test']);
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
     * Test default port is used when not specified
     */
    public function testDefaultPort()
    {
        if ($this->getPort() != 6379) {
            $this->markTestSkipped('Test requires default port 6379');
            return;
        }

        $client = new ValkeyGlide();
        $result = $client->connect($this->getHost());

        $this->assertTrue($result, 'connect() with default port should work');

        $client->ping();
        $client->close();
    }
}
