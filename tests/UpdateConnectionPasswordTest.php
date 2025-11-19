<?php

require_once 'TestSuite.php';

class UpdateConnectionPasswordTest extends TestSuite
{
    // Test basic password update without immediate auth
    public function testUpdateConnectionPasswordStandalone()
    {
        $client = $this->createClient();
        
        $result = $client->updateConnectionPassword("new_password", false);
        $this->assertEquals("OK", $result, "Update password should return OK");
        
        $client->close();
    }

    // Test password update with immediate authentication
    public function testUpdateConnectionPasswordWithImmediateAuth()
    {
        $client = $this->createClient();
        
        $result = $client->updateConnectionPassword("new_password", true);
        $this->assertEquals("OK", $result, "Update password with immediate auth should return OK");
        
        $client->close();
    }

    // Test clearing password without immediate auth
    public function testClearConnectionPasswordStandalone()
    {
        $client = $this->createClient();
        
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result, "Clear password should return OK");
        
        $client->close();
    }

    // Test clearing password with immediate authentication
    public function testClearConnectionPasswordWithImmediateAuth()
    {
        $client = $this->createClient();
        
        $result = $client->clearConnectionPassword(true);
        $this->assertEquals("OK", $result, "Clear password with immediate auth should return OK");
        
        $client->close();
    }

    // Test that empty password throws exception
    public function testUpdateConnectionPasswordEmptyString()
    {
        $client = $this->createClient();
        
        try {
            $client->updateConnectionPassword("", false);
            $this->fail("Expected exception for empty password");
        } catch (Exception $e) {
            $this->assertStringContainsString("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    // Test long password (1000+ characters)
    public function testUpdateConnectionPasswordLongString()
    {
        $client = $this->createClient();
        
        $longPassword = str_repeat("a", 1000);
        $result = $client->updateConnectionPassword($longPassword, false);
        $this->assertEquals("OK", $result, "Update with long password should return OK");
        
        $client->close();
    }

    // Test cluster client password update
    public function testUpdateConnectionPasswordCluster()
    {
        $client = $this->createClusterClient();
        
        $result = $client->updateConnectionPassword("cluster_password", false);
        $this->assertEquals("OK", $result, "Cluster password update should return OK");
        
        $client->close();
    }

    // Test cluster client password clear
    public function testClearConnectionPasswordCluster()
    {
        $client = $this->createClusterClient();
        
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result, "Cluster password clear should return OK");
        
        $client->close();
    }

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
            $adminClient->customCommand(["CLIENT", "KILL", "TYPE", "NORMAL"]);
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
        
        $client->close();
    }

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
        
        // Clear client password
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result);
        
        // Verify connection still works after clearing password
        $this->assertNotNull($client->ping(), "Client should work after clearing password");
        
        $client->close();
    }

    // Test immediate auth with wrong password fails
    public function testUpdateConnectionPasswordImmediateAuthWrongPassword()
    {
        $client = $this->createClient();
        
        // Verify initial connection
        $this->assertNotNull($client->ping(), "Client should be connected");
        
        // Try immediate auth with wrong password (server has no password)
        try {
            $client->updateConnectionPassword("wrong_password", true);
            $this->fail("Expected exception for immediate auth with wrong password");
        } catch (Exception $e) {
            $this->assertStringContainsString("AUTH", $e->getMessage(), "Should fail authentication");
        }
        
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
            $adminClient->customCommand(["CLIENT", "KILL", "TYPE", "NORMAL"]);
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

    // Test null password throws TypeError
    public function testUpdateConnectionPasswordNull()
    {
        $client = $this->createClient();
        
        try {
            $client->updateConnectionPassword(null, false);
            $this->fail("Expected TypeError for null password");
        } catch (TypeError $e) {
            // Expected - PHP type system rejects null for string parameter
            $this->assertStringContainsString("must be of type string", $e->getMessage());
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
            $this->assertStringContainsString("AUTH", $e->getMessage());
        }
        
        $client->close();
    }

    // Test cluster null password throws TypeError
    public function testUpdateConnectionPasswordNullCluster()
    {
        $client = $this->createClusterClient();
        
        try {
            $client->updateConnectionPassword(null, false);
            $this->fail("Expected TypeError for null password");
        } catch (TypeError $e) {
            // Expected - PHP type system rejects null for string parameter
            $this->assertStringContainsString("must be of type string", $e->getMessage());
        }
        
        $client->close();
    }
}
