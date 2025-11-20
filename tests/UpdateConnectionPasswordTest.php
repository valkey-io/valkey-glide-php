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
        echo "Step 1: Creating client and adminClient\n";
        $client = $this->createClient();
        $adminClient = $this->createClient();
        
        echo "Step 2: Ping both clients\n";
        $this->assertNotNull($client->ping(), "Client should be connected");
        $this->assertNotNull($adminClient->ping(), "Admin client should be connected");
        
        echo "Step 3: Update client connection password (delay auth)\n";
        $result = $client->updateConnectionPassword("test_password", false);
        $this->assertEquals("OK", $result);
        
        echo "Step 4: Ping client (no reconnect)\n";
        $this->assertNotNull($client->ping(), "Client should still work without reconnect");
        
        echo "Step 5: Admin client - CONFIG SET requirepass\n";
        $adminClient->config("SET", "requirepass", "test_password");
        
        echo "Step 6: Admin client - CLIENT KILL (kills both clients)\n";
        $adminClient->client("KILL", "TYPE", "NORMAL");
        
        echo "Step 7: Sleep 1 second\n";
        sleep(1);
        
        echo "Step 8: Ping client (should reconnect with new password)\n";
        $this->assertNotNull($client->ping(), "Client should reconnect with new password");
        
        echo "Step 9: Recreate adminClient with password (it was killed)\n";
        $adminClient->close();
        $adminClient = $this->createClient("test_password");
        
        echo "Step 10: Clear client connection password\n";
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        echo "Step 11: Admin client - CONFIG SET requirepass (empty)\n";
        $adminClient->config("SET", "requirepass", "");
        
        echo "Step 12: Admin client - CLIENT KILL\n";
        $adminClient->client("KILL", "TYPE", "NORMAL");
        
        echo "Step 13: Sleep 1 second\n";
        sleep(1);
        
        echo "Step 14: Ping client (should reconnect without password)\n";
        $this->assertNotNull($client->ping(), "Client should reconnect without password");
        
        echo "Step 15: Close both clients\n";
        $client->close();
        $adminClient->close();
        
        echo "Test completed successfully\n";
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
        
        // Recreate adminClient with password (it was killed)
        $adminClient->close();
        $adminClient = $this->createClusterClient("cluster_password");
        
        // Clear server password using new admin client
        $adminClient->config("SET", "requirepass", "");
        
        // Clear client connection password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Kill all clients to force reconnection
        $adminClient->client("KILL", "TYPE", "NORMAL");
        sleep(1);
        
        $this->assertNotNull($client->ping(), "Client should reconnect without password");
        
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
}
