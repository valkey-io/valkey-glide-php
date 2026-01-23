<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideClusterBaseTest.php";

use ValkeyGlide\OpenTelemetry\OpenTelemetryConfig;
use ValkeyGlide\OpenTelemetry\TracesConfig;

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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY
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

        $valkey_glide = new ValkeyGlideCluster(addresses: $addresses, use_tls: false, credentials: $this->getAuth());
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth()
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithTlsEnabled()
    {
        // Test with TLS explicitly enabled. Also enable 1 minute timeout for valgrind runs.
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 8001]],
            use_tls:true, // use_tls enabled
            credentials: $this->getAuth(),
            advanced_config: [ 'tls_config' => ['use_insecure_tls' => true], 'connection_timeout' => 60000]
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: null // no credentials
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithPasswordCredentials()
    {
        // Test with password-only credentials
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 5001]],
            use_tls: false,
            credentials: ['username' => '', 'password' => 'dummy_password'] // password credentials
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
            $valkey_glide = new ValkeyGlideCluster(addresses: $addresses, use_tls: false, credentials: $credentials);
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromPreferReplica()
    {
        // Test with READ_FROM_PREFER_REPLICA strategy
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromAzAffinity()
    {
        // Test with READ_FROM_AZ_AFFINITY strategy
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithReadFromAzAffinityReplicasAndPrimary()
    {
        // Test with READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY strategy
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: 5000 // 5 second timeout
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithRequestTimeoutExceeded()
    {
        // Test with 1 second timeout
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: 10 // 10 millisecond timeout
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: 10000 // 10 second timeout
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: $reconnectStrategy
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: $reconnectStrategy
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: "test-cluster-client-"
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithPeriodicChecksDisabled()
    {
        // Test with periodic checks disabled
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: null,
            client_az: 'us-east-1a' // client availability zone
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    public function testConstructorWithDifferentClientAz()
    {
        // Test with different client availability zone
        $valkey_glide = new ValkeyGlideCluster(
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: null,
            client_az: 'eu-west-1b' // different client availability zone
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: null,
            client_az: null,
            advanced_config: $advancedConfig
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
                addresses: [['host' => '127.0.0.1', 'port' => 7001]],
                use_tls: false,
                credentials: $this->getAuth(),
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                client_name: null,
                periodic_checks: null,
                client_az: null,
                advanced_config: null,
                lazy_connect: false
            );
            $route = ['type' => 'primarySlotKey', 'key' => 'test'];
            $clients = $valkey_glide_monitoring->client($route, 'list');
            $client_count = count($clients);

            // Test with lazy connection enabled
            $valkey_glide_lazy = new ValkeyGlideCluster(
                addresses: [['host' => '127.0.0.1', 'port' => 7001]],
                use_tls: false,
                credentials: $this->getAuth(),
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                client_name: null,
                periodic_checks: null,
                client_az: null,
                advanced_config: null,
                lazy_connect: true // lazy connect enabled
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,
            credentials: $this->getAuth(),
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: null,
            client_az: null,
            advanced_config: null,
            lazy_connect: false // lazy connect disabled
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
            addresses: $addresses,
            use_tls: false,
            credentials: $credentials,
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: 3000,
            reconnect_strategy: $reconnectStrategy,
            client_name: $clientName,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
            client_az: 'us-west-2a',
            advanced_config: $advancedConfig,
            lazy_connect: false
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
            addresses: [['host' => '127.0.0.1', 'port' => 7001]],
            use_tls: false,                                      // use_tls
            credentials: $this->getAuth(),                          // credentials
            read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA,     // read_from
            request_timeout: 5000,                                       // request_timeout
            reconnect_strategy: ['num_of_retries' => 5],                   // reconnect_strategy
            client_name: 'common-config-client'                      // client_name
        );

        $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
        $valkey_glide->close();
    }

    // ==============================================
    // DATABASE_ID PARAMETER TESTS
    // ==============================================

    /**
     * Test that cluster constructor accepts database_id parameter
     */
    public function testClusterConstructorAcceptsDatabaseId(): void
    {
        $addresses = [['host' => 'localhost', 'port' => 7001]];

        // Test that constructor accepts all 12 parameters including database_id
        try {
            $valkey_glide = new ValkeyGlideCluster(
                addresses: $addresses,
                use_tls: false,
                credentials: null,
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

            $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
            $valkey_glide->close();
        } catch (ArgumentCountError $e) {
            $this->fail('Constructor should accept database_id parameter: ' . $e->getMessage());
        } catch (Exception $e) {
            // Connection error is expected since no cluster is running
            $this->assertStringContains('Failed to create initial connections', $e->getMessage());
        }
    }

    /**
     * Test that cluster constructor works with null database_id (default)
     */
    public function testClusterConstructorWithNullDatabaseId(): void
    {
        try {
            $valkey_glide = new ValkeyGlideCluster(
                addresses: [['host' => '127.0.0.1', 'port' => 7001]],                          // addresses
                use_tls: false,                               // use_tls
                credentials: null,                                // credentials
                read_from: ValkeyGlide::READ_FROM_PRIMARY,     // read_from
                request_timeout: null,                                // request_timeout
                reconnect_strategy: null,                                // reconnect_strategy
                client_name: null,                                // client_name
                periodic_checks: null,                                // periodic_checks
                client_az: null,                                // client_az
                advanced_config: null,                                // advanced_config
                lazy_connect: null,                                // lazy_connect
                database_id: null                                 // database_id = null (default)
            );

            $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
            $valkey_glide->close();
        } catch (ArgumentCountError $e) {
            $this->fail('Constructor should accept null database_id: ' . $e->getMessage());
        } catch (Exception $e) {
            // Connection error is expected since no cluster is running
            $this->assertStringContains('Failed to create initial connections', $e->getMessage());
        }
    }

    /**
     * Test backward compatibility - constructor should work without database_id
     */
    public function testClusterConstructorBackwardCompatibility(): void
    {
        $addresses = [['host' => 'localhost', 'port' => 7001]];

        try {
            // Test with 11 parameters (without database_id) - should still work
            $valkey_glide = new ValkeyGlideCluster(
                addresses: $addresses,
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                client_name: null,
                periodic_checks: null,
                client_az: null,
                advanced_config: null,
                lazy_connect: null
                // database_id omitted - should default to null/0
            );

            $this->assertTrue($valkey_glide->ping(['type' => 'primarySlotKey', 'key' => 'test']));
            $valkey_glide->close();
        } catch (ArgumentCountError $e) {
            $this->fail('Constructor should work without database_id for backward compatibility: ' . $e->getMessage());
        } catch (Exception $e) {
            // Connection error is expected since no cluster is running
            $this->assertStringContains('Failed to create initial connections', $e->getMessage());
        }
    }

    /**
     * Test that constructor parameter count is correct
     */
    public function testClusterConstructorParameterCount(): void
    {
        $addresses = [['host' => 'localhost', 'port' => 7001]];

        try {
            $client = new ValkeyGlideCluster(
                addresses: $addresses,
                use_tls: false,
                credentials: null,
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

            $this->assertTrue($client->ping(['type' => 'primarySlotKey', 'key' => 'test']));
            $client->close();
        } catch (Exception $e) {
            $this->fail('Constructor should accept all valid parameters: ' . $e->getMessage());
        }
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

    public function testOtelClusterConfiguration()
    {
        // Wait for any pending flushes from previous tests to complete
        usleep(300000); // 300ms

        // Use same file as standalone since OTEL can only be initialized once per process
        $tracesFile = sys_get_temp_dir() . '/valkey_glide_traces_test.json';

        // Clean up any existing trace file
        if (file_exists($tracesFile)) {
            unlink($tracesFile);
        }

        // Test with 100% sampling to ensure spans are exported
        $otelConfig = OpenTelemetryConfig::builder()
            ->traces(TracesConfig::builder()
                ->endpoint('file://' . $tracesFile)
                ->samplePercentage(100)
                ->build())
            ->flushIntervalMs(100)
            ->build();

        // Create cluster client with OpenTelemetry configuration
        $client = new ValkeyGlideCluster(
            addresses: [
                ['host' => 'localhost', 'port' => 7001],
                ['host' => 'localhost', 'port' => 7002],
                ['host' => 'localhost', 'port' => 7003]
            ],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: 'otel-cluster-test',
            periodic_checks: null,
            client_az: null,
            advanced_config: [
                'otel' => $otelConfig
            ]
        );

        // Ensure 100% sampling (OTEL may already be initialized from previous tests)
        ValkeyGlide::setOtelSamplePercentage(100);


        // Execute commands that should generate spans
        $client->set('otel:cluster:test', 'value');
        $value = $client->get('otel:cluster:test');
        $this->assertEquals('value', $value, "GET should return the set value with OpenTelemetry");

        $deleteResult = $client->del('otel:cluster:test');
        $this->assertGT(0, $deleteResult, "DEL should delete the key with OpenTelemetry");

        $client->close();

        // Wait for spans to be flushed
        usleep(500000); // 500ms

        // Verify spans were exported
        $this->assertTrue(file_exists($tracesFile), "Traces file should exist");
        $this->assertGT(0, filesize($tracesFile), "Traces file should not be empty");

        // Additional delay to ensure all spans are fully written
        usleep(200000); // 200ms

        // Read and parse the traces file
        $tracesContent = file_get_contents($tracesFile);
        $this->assertGT(0, strlen($tracesContent), "Traces file should have content");

        // Parse JSON lines (each line is a separate JSON object)
        $lines = explode("\n", trim($tracesContent));
        $spanNames = [];

        foreach ($lines as $line) {
            if (empty($line)) {
                continue;
            }

            $span = json_decode($line, true);
            if ($span && isset($span['name'])) {
                $spanNames[] = $span['name'];
            }
        }

        // Verify expected span names are present
        $this->assertContains('SET', $spanNames, "Should have SET span");
        $this->assertContains('GET', $spanNames, "Should have GET span");
        $this->assertContains('DEL', $spanNames, "Should have DEL span");

        // Clean up after assertions
        if (file_exists($tracesFile)) {
            unlink($tracesFile);
        }
    }

    public function testOtelClusterSamplingPercentage()
    {
        // Use same file as standalone since OTEL can only be initialized once per process
        $tracesFile = sys_get_temp_dir() . '/valkey_glide_traces_sampling_test.json';

        // Clean up any existing trace file
        if (file_exists($tracesFile)) {
            unlink($tracesFile);
        }

        // Test with 0% sampling - no spans should be exported
        $otelConfig = OpenTelemetryConfig::builder()
            ->traces(TracesConfig::builder()
                ->endpoint('file://' . $tracesFile)
                ->samplePercentage(0)
                ->build())
            ->flushIntervalMs(100)
            ->build();

        $client = new ValkeyGlideCluster(
            addresses: [
                ['host' => 'localhost', 'port' => 7001],
                ['host' => 'localhost', 'port' => 7002],
                ['host' => 'localhost', 'port' => 7003]
            ],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: 'otel-cluster-sampling-test',
            periodic_checks: null,
            client_az: null,
            advanced_config: [
                'otel' => $otelConfig
            ]
        );

        // Execute commands
        $client->set('otel:cluster:sampling:test', 'value');
        $client->get('otel:cluster:sampling:test');
        $client->del('otel:cluster:sampling:test');
        $client->close();

        // Wait for potential flush
        usleep(500000); // 500ms

        // Verify no spans were exported (file should not exist or be empty)
        if (file_exists($tracesFile)) {
            $fileSize = filesize($tracesFile);
            unlink($tracesFile);
            $this->assertEquals(0, $fileSize, "Traces file should be empty with 0% sampling");
        }
    }

    public function testOtelClusterArrayConfigurationRejected()
    {
        // Test that array-based OpenTelemetry configuration is rejected for cluster
        $arrayConfig = [
            'traces' => [
                'endpoint' => 'file:///tmp/valkey-cluster-traces.json',
                'sample_percentage' => 10
            ],
            'metrics' => [
                'endpoint' => 'file:///tmp/valkey-cluster-metrics.json'
            ]
        ];

        try {
            $client = new ValkeyGlideCluster(
                addresses: [
                    ['host' => 'localhost', 'port' => 7001],
                    ['host' => 'localhost', 'port' => 7002],
                    ['host' => 'localhost', 'port' => 7003]
                ],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                client_name: 'array-otel-cluster-test',
                periodic_checks: null,
                client_az: null,
                advanced_config: [
                    'otel' => $arrayConfig
                ]
            );

            $this->fail("Array-based OpenTelemetry configuration should be rejected for cluster");
        } catch (Exception $e) {
            // Verify the error message indicates object is required
            $this->assertStringContains(
                "OpenTelemetryConfig object",
                $e->getMessage(),
                "Error should indicate OpenTelemetryConfig object is required"
            );
        }
    }

    public function testOtelClusterWithoutConfiguration()
    {
        // Test that cluster client works normally without OpenTelemetry configuration
        $client = new ValkeyGlideCluster(
            addresses: [
                ['host' => 'localhost', 'port' => 7001],
                ['host' => 'localhost', 'port' => 7002],
                ['host' => 'localhost', 'port' => 7003]
            ],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PRIMARY,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: 'no-otel-cluster-test'
        );

        // Operations should work normally without OpenTelemetry
        $client->set('no:otel:cluster:test', 'value');
        $value = $client->get('no:otel:cluster:test');
        $this->assertEquals('value', $value, "GET should return the set value");

        $deleteResult = $client->del('no:otel:cluster:test');
        $this->assertGT(0, $deleteResult, "DEL should delete the key");

        $client->close();
    }

    public function testOtelClusterDefaultSamplePercentage()
    {
        // Test that sample percentage defaults to 1 in PHP builder
        $tracesConfig = TracesConfig::builder()
            ->endpoint('file:///tmp/test_cluster_traces.json')
            ->build();

        // Verify default sample percentage is 1
        $this->assertEquals(
            1,
            $tracesConfig->getSamplePercentage(),
            "Sample percentage should default to 1 when not specified"
        );
    }

    public function testOtelClusterSetSamplePercentage()
    {
        // Initialize OTEL by creating a client with OTEL config
        $tracesFile = sys_get_temp_dir() . '/valkey_glide_traces_test.json';
        $otelConfig = OpenTelemetryConfig::builder()
            ->traces(
                TracesConfig::builder()
                    ->endpoint('file://' . $tracesFile)
                    ->samplePercentage(100)
                    ->build()
            )
            ->flushIntervalMs(100)
            ->build();

        $client = new ValkeyGlideCluster(
            addresses: [
                ['host' => 'localhost', 'port' => 7001],
                ['host' => 'localhost', 'port' => 7002],
                ['host' => 'localhost', 'port' => 7003]
            ],
            use_tls: false,
            advanced_config: [
                'otel' => $otelConfig
            ]
        );
        $client->close();

        // Test setting sample percentage to 100%
        ValkeyGlide::setOtelSamplePercentage(100);
        $this->assertEquals(100, ValkeyGlide::getOtelSamplePercentage(), "Sample percentage should be 100");

        // Test setting sample percentage to 0%
        ValkeyGlide::setOtelSamplePercentage(0);
        $this->assertEquals(0, ValkeyGlide::getOtelSamplePercentage(), "Sample percentage should be 0");

        // Test setting sample percentage to a random value
        $randomPercentage = rand(1, 99);
        ValkeyGlide::setOtelSamplePercentage($randomPercentage);
        $this->assertEquals($randomPercentage, ValkeyGlide::getOtelSamplePercentage(), "Sample percentage should be $randomPercentage");
    }
}
