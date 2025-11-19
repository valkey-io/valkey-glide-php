<?php

require_once 'TestSuite.php';

class UpdateConnectionPasswordTest extends TestSuite
{
    public function setUp()
    {
        // No-op.
    }

    protected function createClient()
    {
        try {
            return new ValkeyGlide([[
                'host' => '127.0.0.1',
                'port' => 6379,
            ]]);
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error creating standalone client: %s\n", $ex->getMessage());
            exit(1);
        }
    }

    protected function createClusterClient()
    {
        try {
            return new ValkeyGlideCluster(
                [['host' => '127.0.0.1', 'port' => 7001]], // addresses array format
                false, // use_tls
                null, // credentials
                ValkeyGlide::READ_FROM_PRIMARY, // read_from
                null, // request_timeout
                null, // reconnect_strategy
                null, // client_name
                null, // periodic_checks
                null, // client_az
                null, // advanced_config
                null, // lazy_connect
                0     // database_id - enable multi-database support
            );
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error creating cluster client: %s\n", $ex->getMessage());
            exit(1);
        }
    }

    // ========================================
    // 1. VALIDATION TESTS (Edge Cases)
    // ========================================

    // Test that empty password throws exception
    public function testUpdateConnectionPasswordEmptyString()
    {
        $client = $this->createClient();
        
        try {
            $client->updateConnectionPassword("", false);
            $this->fail("Expected exception for empty password");
        } catch (Exception $e) {
            $this->assertStringContains("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    // Test that empty password throws exception (cluster)
    public function testUpdateConnectionPasswordEmptyStringCluster()
    {
        $client = $this->createClusterClient();
        
        try {
            $client->updateConnectionPassword("", false);
            $this->fail("Expected exception for empty password");
        } catch (Exception $e) {
            $this->assertStringContains("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    // Test null password throws TypeError or triggers deprecation
    public function testUpdateConnectionPasswordNull()
    {
        $client = $this->createClient();
        
        try {
            // Suppress deprecation warning in PHP 8.1+
            @$client->updateConnectionPassword(null, false);
            // If we get here, null was converted to empty string, which should fail
            $this->fail("Expected TypeError or exception for null password");
        } catch (TypeError $e) {
            // Expected in strict mode - PHP type system rejects null for string parameter
            $this->assertStringContains("must be of type string", $e->getMessage());
        } catch (Exception $e) {
            // Expected if null is converted to empty string
            $this->assertStringContains("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    // Test cluster null password throws TypeError or triggers deprecation
    public function testUpdateConnectionPasswordNullCluster()
    {
        $client = $this->createClusterClient();
        
        try {
            // Suppress deprecation warning in PHP 8.1+
            @$client->updateConnectionPassword(null, false);
            // If we get here, null was converted to empty string, which should fail
            $this->fail("Expected TypeError or exception for null password");
        } catch (TypeError $e) {
            // Expected in strict mode - PHP type system rejects null for string parameter
            $this->assertStringContains("must be of type string", $e->getMessage());
        } catch (Exception $e) {
            // Expected if null is converted to empty string
            $this->assertStringContains("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    // ========================================
    // 2. LONG PASSWORD TESTS
    // ========================================

    // Test long password (1000+ characters)
    public function testUpdateConnectionPasswordLongString()
    {
        $client = $this->createClient();
        
        $longPassword = str_repeat("a", 1000);
        $result = $client->updateConnectionPassword($longPassword, false);
        $this->assertEquals("OK", $result, "Update with long password should return OK");
        
        $client->close();
    }

    // Test cluster client with long password string
    public function testUpdateConnectionPasswordLongStringCluster()
    {
        $client = $this->createClusterClient();
        
        $longPassword = str_repeat("a", 1000);
        $result = $client->updateConnectionPassword($longPassword, false);
        $this->assertEquals("OK", $result, "Cluster update with long password should return OK");
        
        $client->close();
    }

    // ========================================
    // 3. INVALID PASSWORD TESTS (Auth Failures)
    // ========================================

    // Test immediate auth with invalid password fails
    public function testUpdateConnectionPasswordImmediateAuthInvalidPassword()
    {
        $client = $this->createClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Try immediate auth with wrong password (server has no password)
        try {
            $client->updateConnectionPassword("wrong_password", true);
            $this->fail("Expected exception for immediate auth with wrong password");
        } catch (Exception $e) {
            $this->assertStringContains("AUTH", $e->getMessage(), "Should fail authentication");
        }
        
        $client->close();
    }

    // Test cluster immediate auth with invalid password fails (server has no password)
    public function testUpdateConnectionPasswordClusterInvalidPassword()
    {
        $client = $this->createClusterClient();
        
        try {
            $client->updateConnectionPassword("invalid_password", true);
            $this->fail("Expected exception for immediate auth with wrong password");
        } catch (Exception $e) {
            // Server has no password, so immediate auth with any password should fail
            $this->assertStringContains("AUTH", $e->getMessage());
        }
        
        $client->close();
    }

    // ========================================
    // 4. SERVER ROTATION - DELAY AUTH
    // ========================================

    // Test password update with server password rotation (delay auth)
    public function testUpdateConnectionPasswordWithServerRotationDelayAuth()
    {
        $client = $this->createClient();
        $adminClient = $this->createClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Update client password (delay auth)
        $result = $client->updateConnectionPassword("test_password", false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works (no reconnect yet)
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        // Update server password and kill connections to force reconnect
        $adminClient->config("SET", "requirepass", "test_password");
        sleep(1);
        
        try {
            $adminClient->client("KILL", "TYPE", "NORMAL");
        } catch (Exception $e) {
            // Expected - admin client gets killed too
        }
        
        sleep(1);
        
        // Client should reconnect with new password
        $this->assertNotNull($client->ping(), "Client should reconnect with new password");
        
        // Clear password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works after clearing password
        $this->assertNotNull($client->ping(), "Client should work after clearing password");
        
        // Clear server password
        $result = $client->config("SET", "requirepass", "");
        $this->assertEquals("OK", $result, "Server password should be cleared");
        sleep(1);
        
        // Kill connections to force reconnect with cleared password
        try {
            $adminClient->client("KILL", "TYPE", "NORMAL");
        } catch (Exception $e) {
            // Expected - admin client gets killed too
        }
        
        sleep(1);
        
        // Client should reconnect with cleared password
        $this->assertNotNull($client->ping(), "Client should reconnect with cleared password");
        
        $client->close();
    }

    // Test cluster password rotation with delay auth
    public function testUpdateConnectionPasswordClusterWithServerRotationDelayAuth()
    {
        $client = $this->createClusterClient();
        $adminClient = $this->createClusterClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Cluster client should be connected");
        
        // Update client password (delay auth)
        $result = $client->updateConnectionPassword("cluster_password", false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works
        $this->assertNotNull($client->ping(), "Cluster client should still work");
        
        // Update server password on all nodes
        $adminClient->config("SET", "requirepass", "cluster_password");
        sleep(1);
        
        try {
            $adminClient->client("KILL", "TYPE", "NORMAL");
        } catch (Exception $e) {
            // Expected - admin client gets killed too
        }
        
        sleep(1);
        
        // Client should reconnect with new password
        $this->assertNotNull($client->ping(), "Cluster client should reconnect");
        
        // Clear password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works after clearing password
        $this->assertNotNull($client->ping(), "Cluster client should work after clearing password");
        
        // Clear server password
        $result = $client->config("SET", "requirepass", "");
        $this->assertEquals("OK", $result, "Server password should be cleared");
        
        $client->close();
    }

    // ========================================
    // 5. SERVER ROTATION - IMMEDIATE AUTH
    // ========================================

    // Test password update with immediate auth
    public function testUpdateConnectionPasswordWithServerRotationImmediateAuth()
    {
        $client = $this->createClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Set server password first
        $client->config("SET", "requirepass", "test_password");
        sleep(1);
        
        // Update client password with immediate auth
        $result = $client->updateConnectionPassword("test_password", true);
        $this->assertEquals("OK", $result);
        
        // Verify connection works
        $this->assertNotNull($client->ping(), "Client should work after immediate auth");
        
        // Clear server password
        $result = $client->config("SET", "requirepass", "");
        $this->assertEquals("OK", $result, "Server password should be cleared");
        sleep(1);
        
        // Clear client password with immediate auth
        $result = $client->clearConnectionPassword(true);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works after clearing password
        $this->assertNotNull($client->ping(), "Client should work after clearing password");
        
        $client->close();
    }

    // Test cluster immediate auth
    public function testUpdateConnectionPasswordClusterImmediateAuth()
    {
        $client = $this->createClusterClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Cluster client should be connected");
        
        // Set server password on all nodes
        $client->config("SET", "requirepass", "cluster_password");
        sleep(1);
        
        // Update client password with immediate auth
        $result = $client->updateConnectionPassword("cluster_password", true);
        $this->assertEquals("OK", $result);
        
        // Verify connection works
        $this->assertNotNull($client->ping(), "Cluster client should work after immediate auth");
        
        // Clear server password
        $result = $client->config("SET", "requirepass", "");
        $this->assertEquals("OK", $result, "Server password should be cleared");
        sleep(1);
        
        // Clear client password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works after clearing password
        $this->assertNotNull($client->ping(), "Cluster client should work after clearing password");
        
        $client->close();
    }
}
