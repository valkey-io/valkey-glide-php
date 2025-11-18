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
}


    public function testUpdateConnectionPasswordEmptyString()
    {
        $client = $this->createClient();
        
        try {
            $client->updateConnectionPassword("", false);
            $this->fail("Should throw exception for empty password");
        } catch (Exception $e) {
            $this->assertStringContainsString("Password cannot be empty", $e->getMessage());
        }
        
        $client->close();
    }

    public function testUpdateConnectionPasswordCluster()
    {
        $client = $this->createClusterClient();
        
        // Test updating password in cluster mode
        $result = $client->updateConnectionPassword("new_password", false);
        $this->assertEquals("OK", $result, "Update password in cluster should return OK");
        
        $client->close();
    }

    public function testClearConnectionPasswordCluster()
    {
        $client = $this->createClusterClient();
        
        // Test clearing password in cluster mode
        $result = $client->clearConnectionPassword(false);
        $this->assertEquals("OK", $result, "Clear password in cluster should return OK");
        
        $client->close();
    }

    public function testUpdateConnectionPasswordWithIAMShouldFail()
    {
        // This test would require IAM configuration
        // Skipping for now as it requires AWS setup
        $this->markTestSkipped("IAM test requires AWS configuration");
    }
}

// Run tests
$test = new UpdateConnectionPasswordTest();
$test->run();
