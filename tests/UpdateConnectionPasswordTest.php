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
        $credentials = $password ? ['password' => $password] : null;
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => 6379]],
            use_tls: false,
            credentials: $credentials
        );
        return $client;
    }

    private function createClusterClient($password = null)
    {
        $credentials = $password ? ['password' => $password] : null;
        return new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $credentials,
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: null,
            client_az: null,
            advanced_config: null,
            lazy_connect: null,
            database_id: 0
        );
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

    // Test null password - PHP converts to empty string which throws exception
    public function testUpdateConnectionPasswordNull()
    {
        $client = $this->createClient();

        try {
            // Suppress deprecation warning - null is converted to empty string
            @$client->updateConnectionPassword(null, false);
            $this->fail("Expected exception for null/empty password");
        } catch (Exception $e) {
            // Should throw exception for empty password
            $this->assertStringContains("Password cannot be empty", $e->getMessage());
        }

        $client->close();
    }

    // Test cluster null password - PHP converts to empty string which throws exception
    public function testUpdateConnectionPasswordNullCluster()
    {
        $client = $this->createClusterClient();

        try {
            // Suppress deprecation warning - null is converted to empty string
            @$client->updateConnectionPassword(null, false);
            $this->fail("Expected exception for null/empty password");
        } catch (Exception $e) {
            // Should throw exception for empty password
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
    public function testUpdateConnectionPasswordClusterImmediateAuthInvalidPassword()
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
        // Github issue: https://github.com/valkey-io/valkey-glide-php/issues/77
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
        // Gtihub issue: https://github.com/valkey-io/valkey-glide-php/issues/77
        $this->markTestSkipped("Skipped: ValkeyGlideCluster::config() not yet implemented");
    }
}
