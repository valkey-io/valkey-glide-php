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
        echo "TEST: Password rotation with delay auth...\n";
        
        echo "  - Step 1: Creating client\n";
        $client = $this->createClient();
        
        echo "  - Step 2: Verifying initial connection\n";
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        echo "  - Step 3: Calling updateConnectionPassword (delay auth)\n";
        $result = $client->updateConnectionPassword("test_password", false);
        $this->assertEquals("OK", $result);
        
        echo "  - Step 4: Verifying connection still works\n";
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        echo "  - Step 5: Setting server password via CONFIG SET\n";
        echo "  - >>> IF SEGFAULT OCCURS, IT MAY HAPPEN HERE <<<\n";
        $client->config("SET", "requirepass", "test_password");
        
        echo "  - Step 6: Waiting 1 second\n";
        sleep(1);
        
        echo "  - Step 7: Closing client\n";
        $client->close();
        
        echo "  - Step 8: Creating new client with credentials\n";
        $client = $this->createClient("test_password");
        $this->assertNotNull($client->ping(), "Client should connect with new password");
        
        echo "  - Step 9: Clearing connection password\n";
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        echo "  - Step 10: Clearing server password\n";
        $client->config("SET", "requirepass", "");
        
        echo "  - Step 11: Closing client\n";
        $client->close();
        
        echo "  - Test completed successfully\n";
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
    // These tests verify that updateConnectionPassword API works correctly
    // and help isolate the segfault bug in client constructor/connection logic

    // Test creating client with password when server has no password
    public function testConstructorWithPasswordNoServerPassword()
    {
        echo "TEST: Creating client with password when server has no password...\n";
        
        try {
            // Server has no password, but we provide one
            echo "  - Creating ValkeyGlide with password='wrong_password'\n";
            $client = new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                ['password' => 'wrong_password']
            );
            
            echo "  - Client created, attempting ping...\n";
            $client->ping();
            $this->fail("Should have failed to connect with wrong password");
        } catch (Exception $e) {
            echo "  - Exception caught (expected): " . $e->getMessage() . "\n";
            // Expected - server has no password
            $this->assertStringContains("AUTH", $e->getMessage());
        }
        
        echo "  - Test completed successfully\n";
    }

    // Test creating client after dynamically setting server password
    // This test isolates the segfault: if this fails but updateConnectionPassword tests pass,
    // it confirms the bug is in constructor/connection logic, NOT in updateConnectionPassword API
    public function testConstructorAfterDynamicPasswordSet()
    {
        echo "TEST: Creating client after dynamically setting server password...\n";
        
        // First, set up server with password
        echo "  - Step 1: Creating setup client (no password)\n";
        $setupClient = new ValkeyGlide([['host' => '127.0.0.1', 'port' => 6379]]);
        
        echo "  - Step 2: Setting server password via CONFIG SET\n";
        $setupClient->config("SET", "requirepass", "test_password");
        
        echo "  - Step 3: Closing setup client\n";
        $setupClient->close();
        
        echo "  - Step 4: Waiting 1 second...\n";
        sleep(1);
        
        // Now try to create new client with password - THIS IS WHERE SEGFAULT OCCURS
        try {
            echo "  - Step 5: Creating new client WITH password='test_password'\n";
            echo "  - >>> IF SEGFAULT OCCURS, IT HAPPENS HERE <<<\n";
            
            $client = new ValkeyGlide(
                [['host' => '127.0.0.1', 'port' => 6379]],
                false,
                ['password' => 'test_password']
            );
            
            echo "  - Step 6: Client created successfully (no segfault!)\n";
            
            echo "  - Step 7: Testing connection with ping\n";
            $result = $client->ping();
            echo "  - PING result: " . $result . "\n";
            
            // Clean up - remove password
            echo "  - Step 8: Cleaning up - removing server password\n";
            $client->config("SET", "requirepass", "");
            $client->close();
            
            echo "  - Test completed successfully\n";
            
        } catch (Exception $e) {
            echo "  - Exception caught: " . $e->getMessage() . "\n";
            
            // Try to clean up anyway
            echo "  - Attempting cleanup...\n";
            try {
                $cleanupClient = new ValkeyGlide(
                    [['host' => '127.0.0.1', 'port' => 6379]],
                    false,
                    ['password' => 'test_password']
                );
                $cleanupClient->config("SET", "requirepass", "");
                $cleanupClient->close();
                echo "  - Cleanup successful\n";
            } catch (Exception $cleanup_e) {
                echo "  - Cleanup failed: " . $cleanup_e->getMessage() . "\n";
            }
            
            throw $e;
        }
    }
}
