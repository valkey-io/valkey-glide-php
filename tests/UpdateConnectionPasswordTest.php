<?php

require_once 'TestSuite.php';

class UpdateConnectionPasswordTest extends TestSuite
{
    public function setUp()
    {
        // No-op.
    }

    private function createClient($password = null)
    {
        try {
            $credentials = $password ? ['password' => $password] : null;
            return new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                $credentials
            );
        } catch (Exception $ex) {
            $this->fail("Fatal error creating standalone client: " . $ex->getMessage());
        }
    }

    private function createClusterClient($password = null)
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
            $this->fail("Fatal error creating cluster client: " . $ex->getMessage());
        }
    }

    // ========================================
    // 1. VALIDATION TESTS (Edge Cases)
    // ========================================

    // Test that empty password clears the password (same as clearConnectionPassword)
    public function testUpdateConnectionPasswordEmptyString()
    {
        $client = $this->createClient();
        
        // Empty string should clear the password
        $result = $client->updateConnectionPassword("", false);
        $this->assertEquals("OK", $result, "Empty password should clear password");
        
        $client->close();
    }

    // Test that empty password clears the password (cluster)
    public function testUpdateConnectionPasswordEmptyStringCluster()
    {
        $client = $this->createClusterClient();
        
        // Empty string should clear the password
        $result = $client->updateConnectionPassword("", false);
        $this->assertEquals("OK", $result, "Empty password should clear password");
        
        $client->close();
    }

    // Test null password - PHP converts to empty string which clears password
    public function testUpdateConnectionPasswordNull()
    {
        $client = $this->createClient();
        
        // Suppress deprecation warning - null is converted to empty string
        // Empty string clears the password (same as clearConnectionPassword)
        $result = @$client->updateConnectionPassword(null, false);
        $this->assertEquals("OK", $result, "Null (converted to empty) should clear password");
        
        $client->close();
    }

    // Test cluster null password - PHP converts to empty string which clears password
    public function testUpdateConnectionPasswordNullCluster()
    {
        $client = $this->createClusterClient();
        
        // Suppress deprecation warning - null is converted to empty string
        // Empty string clears the password (same as clearConnectionPassword)
        $result = @$client->updateConnectionPassword(null, false);
        $this->assertEquals("OK", $result, "Null (converted to empty) should clear password");
        
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
        
        try {
            $this->assertNotNull($client->ping(), "Client should be connected");
            $this->assertNotNull($adminClient->ping(), "Admin client should be connected");
            
            // Update client connection password
            $result = $client->updateConnectionPassword("test_password", false);
            $this->assertEquals("OK", $result);
            
            $this->assertNotNull($client->ping(), "Client should still work without reconnect");
            
            // Update server password using admin client
            $adminClient->config("SET", "requirepass", "test_password");
            
            // Get client ID and kill only the test client
            $clientId = $client->client("ID");
            $adminClient->client("KILL", "ID", $clientId);
            sleep(1);
            
            $this->assertNotNull($client->ping(), "Client should reconnect with new password");
            
            // Clear client connection password
            $result = $client->clearConnectionPassword(false);
            $this->assertEquals("OK", $result);
            
            $this->assertNotNull($client->ping(), "Client should still work without reconnect");
            
            // Clear server password using admin client
            $adminClient->config("SET", "requirepass", "");
            
            // Kill test client again
            $clientId = $client->client("ID");
            $adminClient->client("KILL", "ID", $clientId);
            sleep(1);
            
            $this->assertNotNull($client->ping(), "Client should reconnect without password");
        } finally {
            // Ensure server password is cleared even if test fails
            try {
                if (isset($adminClient)) {
                    $adminClient->config("SET", "requirepass", "");
                }
            } catch (Exception $e) {
                // Ignore cleanup errors
            }
            
            if (isset($client)) {
                $client->close();
            }
            if (isset($adminClient)) {
                $adminClient->close();
            }
        }
    }

    // Test cluster password rotation with delay auth
    public function testUpdateConnectionPasswordClusterWithServerRotationDelayAuth()
    {
        // TODO: Re-enable once ValkeyGlideCluster::config() is implemented
        // SKIP: Cluster config() method is not yet implemented (marked as TODO in stub)
        // Cannot set server password on cluster nodes without config() method
        $this->markTestSkipped("Skipped: ValkeyGlideCluster::config() not yet implemented");
    }

    // ========================================
    // 5. SERVER ROTATION - IMMEDIATE AUTH
    // ========================================

    // Test password update with immediate auth
    public function testUpdateConnectionPasswordWithServerRotationImmediateAuth()
    {
        $client = $this->createClient();
        
        try {
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
        } finally {
            // Ensure server password is cleared even if test fails
            try {
                if (isset($client)) {
                    $client->config("SET", "requirepass", "");
                }
            } catch (Exception $e) {
                // Ignore cleanup errors
            }
            
            if (isset($client)) {
                $client->close();
            }
        }
    }

    // Test cluster immediate auth
    public function testUpdateConnectionPasswordClusterImmediateAuth()
    {
        // TODO: Re-enable once ValkeyGlideCluster::config() is implemented
        // SKIP: Cluster config() method is not yet implemented (marked as TODO in stub)
        // Cannot set server password on cluster nodes without config() method
        $this->markTestSkipped("Skipped: ValkeyGlideCluster::config() not yet implemented");
    }
}
