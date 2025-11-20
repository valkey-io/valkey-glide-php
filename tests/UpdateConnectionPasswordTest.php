<?php

require_once 'TestSuite.php';

class UpdateConnectionPasswordTest extends TestSuite
{
    public function setUp()
    {
        // No-op.
    }

    protected function createClient($password = null)
    {
        try {
            $credentials = $password ? ['password' => $password] : null;
            return new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                $credentials
            );
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error creating standalone client: %s\n", $ex->getMessage());
            exit(1);
        }
    }

    protected function createClusterClient($password = null)
    {
        try {
            $credentials = $password ? ['password' => $password] : null;
            return new ValkeyGlideCluster(
                [['host' => '127.0.0.1', 'port' => 7001]],
                false, // use_tls
                $credentials,
                ValkeyGlide::READ_FROM_PRIMARY,
                null, null, null, null, null, null, null, 0
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
        
        $this->assertNotNull($client->ping(), "Client should be connected");
        $this->assertNotNull($adminClient->ping(), "Admin client should be connected");
        
        // Update client connection password
        $result = $client->updateConnectionPassword("test_password", false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        // Update server password and kill all clients using admin client
        $adminClient->config("SET", "requirepass", "test_password");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect with new password");
        
        // Clear client connection password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        // Clear server password and kill all clients using admin client
        $adminClient->config("SET", "requirepass", "");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect without password");
        
        $client->close();
        $adminClient->close();
    }

    // Test cluster password rotation with delay auth
    public function testUpdateConnectionPasswordClusterWithServerRotationDelayAuth()
    {
        $client = $this->createClusterClient();
        $adminClient = $this->createClusterClient();
        
        $this->assertNotNull($client->ping(), "Client should be connected");
        $this->assertNotNull($adminClient->ping(), "Admin client should be connected");
        
        // Update client connection password
        $result = $client->updateConnectionPassword("cluster_password", false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        // Update server password and kill all clients using admin client
        $adminClient->config("SET", "requirepass", "cluster_password");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect with new password");
        
        // Clear client connection password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        // Clear server password and kill all clients using admin client
        $adminClient->config("SET", "requirepass", "");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect without password");
        
        $client->close();
        $adminClient->close();
    }

    // Test password rotation with delay auth and automatic reconnection (standalone)
    public function testUpdateConnectionPasswordWithServerRotationDelayAuthAutoReconnect()
    {
        $client = $this->createClient();
        $adminClient = $this->createClient();
        
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Update client password (delay auth)
        $result = $client->updateConnectionPassword("test_password", false);
        $this->assertEquals("OK", $result);
        
        // Set server password and kill connections using admin client
        $adminClient->config("SET", "requirepass", "test_password");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        // Client should automatically reconnect with updated password
        $this->assertNotNull($client->ping(), "Client should reconnect with new password");
        
        // Clear client password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Clear server password and kill connections
        $adminClient->config("SET", "requirepass", "");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect without password");
        
        $client->close();
        $adminClient->close();
    }

    // Test cluster password rotation with delay auth and automatic reconnection
    public function testUpdateConnectionPasswordClusterWithServerRotationDelayAuthAutoReconnect()
    {
        $client = $this->createClusterClient();
        $adminClient = $this->createClusterClient();
        
        $this->assertNotNull($client->ping(), "Cluster client should be connected");
        
        // Update client password (delay auth)
        $result = $client->updateConnectionPassword("cluster_password", false);
        $this->assertEquals("OK", $result);
        
        // Set server password on all nodes and kill connections using admin client
        $adminClient->config("SET", "requirepass", "cluster_password");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        // Client should automatically reconnect with updated password
        $this->assertNotNull($client->ping(), "Cluster client should reconnect with new password");
        
        // Clear client password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Clear server password and kill connections
        $adminClient->config("SET", "requirepass", "");
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Cluster client should reconnect without password");
        
        $client->close();
        $adminClient->close();
    }

    // ========================================
    // 5. SERVER ROTATION - IMMEDIATE AUTH
    // ========================================

    // Test password update with immediate auth
    public function testUpdateConnectionPasswordWithServerRotationImmediateAuth()
    {
        $client = $this->createClient();
        
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Update server password
        $client->config("SET", "requirepass", "test_password");
        sleep(1);
        
        // Update client connection password with immediate auth
        $result = $client->updateConnectionPassword("test_password", true);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should work after immediate auth");
        
        // Clear server password
        $client->config("SET", "requirepass", "");
        sleep(1);
        
        // Clear client connection password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should work after clearing password");
        
        $client->close();
    }

    // Test cluster immediate auth
    public function testUpdateConnectionPasswordClusterImmediateAuth()
    {
        $client = $this->createClusterClient();
        
        $this->assertNotNull($client->ping(), "Cluster client should be connected");
        
        // Update server password on all nodes
        $client->config("SET", "requirepass", "cluster_password");
        sleep(1);
        
        // Update client connection password with immediate auth
        $result = $client->updateConnectionPassword("cluster_password", true);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Cluster client should work after immediate auth");
        
        // Clear server password
        $client->config("SET", "requirepass", "");
        sleep(1);
        
        // Clear client connection password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Cluster client should work after clearing password");
        
        $client->close();
    }

    // Test password rotation with immediate auth and automatic reconnection (standalone)
    public function testUpdateConnectionPasswordWithServerRotationImmediateAuthAutoReconnect()
    {
        $client = $this->createClient();
        $adminClient = $this->createClient();
        
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Set server password using admin client
        $adminClient->config("SET", "requirepass", "test_password");
        sleep(1);
        
        // Update client password with immediate auth
        $result = $client->updateConnectionPassword("test_password", true);
        $this->assertEquals("OK", $result);
        
        // Kill connection to force reconnection using admin client
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        // Client should automatically reconnect with password
        $this->assertNotNull($client->ping(), "Client should reconnect with password");
        
        // Clear server password using admin client
        $adminClient->config("SET", "requirepass", "");
        sleep(1);
        
        // Clear client password
        $result = $client->clearConnectionPassword(true);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Client should work after clearing password");
        
        $client->close();
        $adminClient->close();
    }

    // Test cluster password rotation with immediate auth and automatic reconnection
    public function testUpdateConnectionPasswordClusterImmediateAuthAutoReconnect()
    {
        $client = $this->createClusterClient();
        $adminClient = $this->createClusterClient();
        
        $this->assertNotNull($client->ping(), "Cluster client should be connected");
        
        // Set server password on all nodes using admin client
        $adminClient->config("SET", "requirepass", "cluster_password");
        sleep(1);
        
        // Update client password with immediate auth
        $result = $client->updateConnectionPassword("cluster_password", true);
        $this->assertEquals("OK", $result);
        
        // Kill connection to force reconnection using admin client
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        // Client should automatically reconnect with password
        $this->assertNotNull($client->ping(), "Cluster client should reconnect with password");
        
        // Clear server password using admin client
        $adminClient->config("SET", "requirepass", "");
        sleep(1);
        
        // Clear client password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        $this->assertNotNull($client->ping(), "Cluster client should work after clearing password");
        
        $client->close();
        $adminClient->close();
    }
}
