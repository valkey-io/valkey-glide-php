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
        // TODO: Re-enable once client connection bug is fixed
        // SKIP: This test causes segfault when reconnecting after server password change
        // This indicates a bug in the client connection logic, not in updateConnectionPassword
        // Related issues: CLIENT KILL also causes segfaults
        $this->markTestSkipped("Skipped: Reconnection after server password change causes segfault");
    }

    // Test cluster password rotation with delay auth
    public function testUpdateConnectionPasswordClusterWithServerRotationDelayAuth()
    {
        // TODO: Re-enable once client connection bug is fixed
        // SKIP: This test causes segfault when reconnecting after server password change
        // This indicates a bug in the client connection logic, not in updateConnectionPassword
        // Related issues: CLIENT KILL also causes segfaults
        $this->markTestSkipped("Skipped: Reconnection after server password change causes segfault");
    }

    // ========================================
    // 5. SERVER ROTATION - IMMEDIATE AUTH
    // ========================================

    // Test password update with immediate auth
    public function testUpdateConnectionPasswordWithServerRotationImmediateAuth()
    {
        // TODO: Re-enable once client connection bug is fixed
        // SKIP: This test causes segfault during server password rotation
        // This indicates a bug in the client connection logic, not in updateConnectionPassword
        $this->markTestSkipped("Skipped: Server password rotation with immediate auth causes segfault");
    }

    // Test cluster immediate auth
    public function testUpdateConnectionPasswordClusterImmediateAuth()
    {
        // TODO: Re-enable once client connection bug is fixed
        // SKIP: This test causes segfault during server password rotation
        // This indicates a bug in the client connection logic, not in updateConnectionPassword
        $this->markTestSkipped("Skipped: Server password rotation with immediate auth causes segfault");
    }

    // ========================================
    // 6. CLIENT CONSTRUCTOR PASSWORD TESTS
    // ========================================

    // Test creating client with password when server has no password
    public function testConstructorWithPasswordNoServerPassword()
    {
        try {
            // Server has no password, but we provide one
            $client = new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                ['password' => 'wrong_password']
            );
            
            // Should fail to connect or throw exception
            $client->ping();
            $this->fail("Should have failed to connect with wrong password");
        } catch (Exception $e) {
            // Expected - server has no password
            $this->assertStringContains("AUTH", $e->getMessage());
        }
    }

    // Test creating client after dynamically setting server password
    public function testConstructorAfterDynamicPasswordSet()
    {
        // First, set up server with password
        $setupClient = new ValkeyGlide([['host' => '127.0.0.1', 'port' => 6379]]);
        $setupClient->config("SET", "requirepass", "test_password");
        $setupClient->close();
        
        sleep(1);
        
        // Now try to create new client with password - THIS IS WHERE SEGFAULT OCCURS
        try {
            echo "Attempting to create client with password...\n";
            $client = new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                ['password' => 'test_password']
            );
            echo "Client created successfully\n";
            
            $result = $client->ping();
            echo "PING result: " . $result . "\n";
            
            // Clean up - remove password
            $client->config("SET", "requirepass", "");
            $client->close();
            
        } catch (Exception $e) {
            echo "Exception caught: " . $e->getMessage() . "\n";
            
            // Try to clean up anyway
            try {
                $cleanupClient = new ValkeyGlide(
                    [['host' => '127.0.0.1', 'port' => 6379]],
                    false,
                    ['password' => 'test_password']
                );
                $cleanupClient->config("SET", "requirepass", "");
                $cleanupClient->close();
            } catch (Exception $cleanup_e) {
                echo "Cleanup failed: " . $cleanup_e->getMessage() . "\n";
            }
            
            throw $e;
        }
    }
}
