<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideClusterBaseTest.php";

/**
 * ValkeyGlide Cluster Features Test
 * Tests various constructor options and features for ValkeyGlideCluster client
 */
class ValkeyGlideClusterFeaturesTest extends ValkeyGlideClusterBaseTest
{
    public function testBasicClusterConstructor()
    {
        // Test creating ValkeyGlideCluster with basic configuration
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]], // addresses array format
            false, // use_tls
            $this->getAuth(), // credentials
            ValkeyGlide::READ_FROM_PRIMARY // read_from
        );

        // Verify the connection works with a simple ping
        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));

        // Clean up
        $valkey_glide->close();
    }

    // ==============================================
    // ADDRESSES PARAMETER TESTS
    // ==============================================

    public function testConstructorWithMultipleAddresses()
    {
        // Test with multiple cluster addresses
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 7001],
            ['host' => '127.0.0.1', 'port' => 7002],
            ['host' => '127.0.0.1', 'port' => 7003]
        ];

        $valkey_glide = new ValkeyGlideCluster($addresses, false, $this->getAuth());
        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // TLS PARAMETER TESTS
    // ==============================================

    public function testConstructorWithTlsDisabled()
    {
        // Test with TLS explicitly disabled
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false, // use_tls disabled
            $this->getAuth()
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithTlsEnabled()
    {
        // Test with TLS explicitly disabled
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 8001]],
            use_tls:true, // use_tls disabled
            credentials: $this->getAuth(),
            advanced_config: [ 'tls_config' => ['use_insecure_tls' => true]]
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // CREDENTIALS PARAMETER TESTS
    // ==============================================

    public function testConstructorWithNullCredentials()
    {
        // Test with no credentials (null)
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            null // no credentials
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithPasswordCredentials()
    {
        // Test with password-only credentials
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 5001]],
            false,
            ['username' => '', 'password' => 'dummy_password'] // password credentials
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorInvalidAuth()
    {
        // Test constructor with credentials (if auth is configured)
        $addresses = [
            [['host' => '127.0.0.1', 'port' => 5001]],
        ];

        // Try to connect with incorrect auth
        $valkey_glide = null;
        try {
            $credentials = ['username' => 'invalid_user', 'password' => 'invalid_password'];
            $valkey_glide = new ValkeyGlideCluster($addresses, false, $credentials);
            $this->fail("Should throw an exception when running commands with invalid authentication");
        } catch (Exception $e) {
            $this->assertStringContains("WRONGPASS", $e->getMessage(), "Exception should indicate authentication failure");
        } finally {
            // Clean up
            $valkey_glide?->close();
        }
    }

    // ==============================================
    // READ STRATEGY PARAMETER TESTS
    // ==============================================

    public function testConstructorWithReadFromPrimary()
    {
        // Test with READ_FROM_PRIMARY strategy
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromPreferReplica()
    {
        // Test with READ_FROM_PREFER_REPLICA strategy
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PREFER_REPLICA
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromAzAffinity()
    {
        // Test with READ_FROM_AZ_AFFINITY strategy
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_AZ_AFFINITY
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromAzAffinityReplicasAndPrimary()
    {
        // Test with READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY strategy
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // REQUEST TIMEOUT PARAMETER TESTS
    // ==============================================

    public function testConstructorWithRequestTimeout()
    {
        // Test with 5 second timeout
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            5000 // 5 second timeout
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithRequestTimeoutExceeded()
    {
        // Test with 1 second timeout
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            10 // 10 millisecond timeout
        );

        $res = $valkey_glide->rawcommand(['type' => 'primarySlotKey', 'key' => 'test'], "DEBUG", "SLEEP", "2");

        // Sleep the test runner so that the server can finish the sleep command.
        sleep(2);
        // Clean up
        $valkey_glide?->close();

        // Now evaluate the result of the DEBUG command (should have failed due to timeout).
        $this->assertFalse($res);
    }

    public function testConstructorWithLongTimeout()
    {
        // Test with 10 second timeout
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            10000 // 10 second timeout
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // RECONNECTION STRATEGY PARAMETER TESTS
    // ==============================================

    public function testConstructorWithSimpleReconnectStrategy()
    {
        // Test with simple reconnection strategy
        $reconnectStrategy = ['num_of_retries' => 5];

        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            $reconnectStrategy
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithComplexReconnectStrategy()
    {
        // Test with complex reconnection strategy
        $reconnectStrategy = [
            'num_of_retries' => 3,
            'factor' => 2,
            'exponent_base' => 1.5,
            'max_delay' => 5000
        ];

        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            $reconnectStrategy
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // CLIENT NAME PARAMETER TESTS
    // ==============================================

    public function testConstructorWithClientName()
    {
        // Test with custom client name

        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            "test-cluster-client-"
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $this->assertStringContains("name=test-cluster-client-", $valkey_glide->client(['type' => 'primarySlotKey', 'key' => 'test'], "info"));
        $valkey_glide->close();
    }

    // ==============================================
    // PERIODIC CHECKS PARAMETER TESTS
    // ==============================================

    public function testConstructorWithPeriodicChecksEnabled()
    {
        // Test with periodic checks enabled (default)
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithPeriodicChecksDisabled()
    {
        // Test with periodic checks disabled
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            ValkeyGlideCluster::PERIODIC_CHECK_DISABLED
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // CLIENT AVAILABILITY ZONE PARAMETER TESTS
    // ==============================================

    public function testConstructorWithClientAz()
    {
        // Test with client availability zone
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            null,
            'us-east-1a' // client availability zone
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithDifferentClientAz()
    {
        // Test with different client availability zone
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            null,
            'eu-west-1b' // different client availability zone
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // ADVANCED CONFIGURATION PARAMETER TESTS
    // ==============================================

    public function testConstructorWithAdvancedConfig()
    {
        // Test with advanced configuration
        $advancedConfig = [
            'connection_timeout' => 5000,
            'socket_timeout' => 3000
        ];

        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            null,
            null,
            $advancedConfig
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // LAZY CONNECT PARAMETER TESTS
    // ==============================================

    public function testConstructorWithLazyConnectEnabled()
    {
        $valkey_glide_lazy = null;
        $valkey_glide_monitoring = null;

        try {
            $key =
            // Create monitoring client and get the initial count.
            $valkey_glide_monitoring = new ValkeyGlideCluster(
                [['host' => '127.0.0.1', 'port' => 7001]],
                false,
                $this->getAuth(),
                ValkeyGlide::READ_FROM_PRIMARY,
                null,
                null,
                null,
                null,
                null,
                null,
                false
            );
            $route = ['type' => 'primarySlotKey', 'key' => 'test'];
            $clients = $valkey_glide_monitoring->client($route, 'list');
            $client_count = count($clients);

            // Test with lazy connection enabled
            $valkey_glide_lazy = new ValkeyGlideCluster(
                [['host' => '127.0.0.1', 'port' => 7001]],
                false,
                $this->getAuth(),
                ValkeyGlide::READ_FROM_PRIMARY,
                null,
                null,
                null,
                null,
                null,
                null,
                true // lazy connect enabled
            );
            // Lazy connection should retain the same client count.
            $clients = $valkey_glide_monitoring->client($route, 'list');
            $this->assertTrue(count($clients) == $client_count);

            // Trigger activity on the lazy connection. Should increment the client count.
            $this->assertTrue($valkey_glide_lazy->ping(['type' => 'primarySlotKey', 'key' => 'test']));
            $clients = $valkey_glide_monitoring->client($route, 'list');
            $this->assertTrue(count($clients) > $client_count);
        } finally {
            $valkey_glide_lazy?->close();
            $valkey_glide_monitoring?->close();
        }
    }

    public function testConstructorWithLazyConnectDisabled()
    {
        // Test with lazy connection disabled
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,
            $this->getAuth(),
            ValkeyGlide::READ_FROM_PRIMARY,
            null,
            null,
            null,
            null,
            null,
            null,
            false // lazy connect disabled
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // COMBINED PARAMETER TESTS
    // ==============================================

    public function testConstructorWithAllParameters()
    {
        // Test with all parameters specified
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 7001],
            ['host' => '127.0.0.1', 'port' => 7002]
        ];
        $credentials = $this->getAuth();
        $reconnectStrategy = ['num_of_retries' => 3, 'factor' => 2];
        $clientName = 'comprehensive-test-client';
        $advancedConfig = ['connection_timeout' => 5000];

        $valkey_glide = new ValkeyGlideCluster(
            $addresses,                                     // addresses
            false,                                          // use_tls
            $credentials,                                   // credentials
            ValkeyGlide::READ_FROM_PRIMARY,                // read_from
            3000,                                           // request_timeout
            $reconnectStrategy,                             // reconnect_strategy
            $clientName,                                    // client_name
            ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS, // periodic_checks
            'us-west-2a',                                   // client_az
            $advancedConfig,                                // advanced_config
            false                                           // lazy_connect
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));

        // Perform a basic operation to ensure everything works
        $key = 'test:comprehensive:' . uniqid();
        $this->assertTrue($valkey_glide->set($key, 'test-value'));
        $this->assertEquals('test-value', $valkey_glide->get($key));
        $valkey_glide->del($key);

        $valkey_glide->close();
    }

    public function testConstructorWithCommonConfiguration()
    {
        // Test with commonly used parameter combination
        $valkey_glide = new ValkeyGlideCluster(
            [['host' => '127.0.0.1', 'port' => 7001]],
            false,                                      // use_tls
            $this->getAuth(),                          // credentials
            ValkeyGlide::READ_FROM_PREFER_REPLICA,     // read_from
            5000,                                       // request_timeout
            ['num_of_retries' => 5],                   // reconnect_strategy
            'common-config-client'                      // client_name
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testClusterClientCreateDeleteLoop()
    {
        // Simple test that creates and deletes ValkeyGlideCluster clients in a loop
        $loopCount = 500; // Fewer iterations for cluster due to overhead
        $successCount = 0;
        $errorCount = 0;
        $startMemory = memory_get_usage(true);

        echo "Testing cluster client create/delete loop with {$loopCount} iterations...\n";

        for ($i = 1; $i <= $loopCount; $i++) {
            try {
                // Create a new cluster client using the base class method
                $client = $this->newInstance();

                // Verify the client works with a cluster-specific ping
                $route = ['type' => 'primarySlotKey', 'key' => 'test'];
                $this->assertTrue($client->ping($route), "Cluster client ping failed on iteration {$i}");

                // Close the client
                $client->close();

                // Explicitly unset to help with cleanup
                unset($client);

                $successCount++;

                // Log progress every 5 iterations (fewer than standalone due to lower count)
                if ($i % 100 == 0) {
                    echo "Completed {$i}/{$loopCount} iterations...\n";
                }
            } catch (Exception $e) {
                $errorCount++;
                echo "Error on iteration {$i}: " . $e->getMessage() . "\n";

                // Continue with the test even if some iterations fail
                continue;
            }
        }

        $endMemory = memory_get_usage(true);
        $memoryGrowth = $endMemory - $startMemory;

        // Log final results
        echo "Cluster Create/Delete Loop Test Results:\n";
        echo "- Total iterations: {$loopCount}\n";
        echo "- Successful iterations: {$successCount}\n";
        echo "- Failed iterations: {$errorCount}\n";
        echo "- Memory growth: " . round($memoryGrowth / 1024, 2) . " KB\n";

        // Assert that most iterations were successful
        $successRate = $successCount / $loopCount;
        $this->assertTrue($successRate > 0.9, "Success rate should be > 90%, got " . round($successRate * 100, 1) . "%");

        // Warn if memory growth is significant
        if ($memoryGrowth > 5 * 1024 * 1024) { // More than 5MB
            echo "WARNING: Significant memory growth detected: " . round($memoryGrowth / 1024 / 1024, 2) . " MB\n";
        }
    }
}
