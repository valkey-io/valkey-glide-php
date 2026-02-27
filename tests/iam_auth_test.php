<?php

/**
 * IAM Authentication Tests for ValkeyGlide PHP
 *
 * These tests demonstrate IAM authentication functionality.
 * Note: These tests require actual AWS credentials and a configured cluster.
 * They are skipped by default in CI/CD environments.
 *
 * To run these tests:
 * 1. Set up AWS credentials (IAM role, environment variables, or credentials file)
 * 2. Create an ElastiCache or MemoryDB cluster with IAM auth enabled
 * 3. Update the configuration below with your cluster details
 * 4. Run: php tests/iam_auth_test.php
 *
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

// Skip tests in CI/CD environments
if (getenv('CI') || getenv('GITHUB_ACTIONS') || getenv('JENKINS_URL')) {
    echo "Skipping IAM auth tests in CI/CD environment\n";
    exit(0);
}

// Test configuration - update these values for your environment
const TEST_CONFIG = [
    'elasticache' => [
        'cluster_name' => 'iam-auth-cluster',
        'username' => 'iam-auth',
        'region' => 'us-east-1',
        'endpoint' => 'clustercfg.iam-auth-cluster.xxxxx.use1.cache.amazonaws.com',
        'port' => 6379
    ],
    'memorydb' => [
        'cluster_name' => 'iam-memorydb-cluster',
        'username' => 'iam-user',
        'region' => 'us-east-1',
        'endpoint' => 'clustercfg.iam-memorydb-cluster.xxxxx.memorydb.us-east-1.amazonaws.com',
        'port' => 6379
    ]
];

class IamAuthTest
{
    private $testsPassed = 0;
    private $testsFailed = 0;

    public function run()
    {
        echo "ValkeyGlide IAM Authentication Tests\n";
        echo "=====================================\n\n";

        // Run test suite
        $this->testElasticacheIamAuth();
        $this->testMemoryDBIamAuth();
        $this->testClusterModeIamAuth();
        $this->testCustomRefreshInterval();
        $this->testMissingUsername();
        $this->testInvalidRefreshInterval();

        // Print summary
        echo "\n=====================================\n";
        echo "Test Summary\n";
        echo "=====================================\n";
        echo "Passed: {$this->testsPassed}\n";
        echo "Failed: {$this->testsFailed}\n";

        return $this->testsFailed === 0;
    }

    private function testElasticacheIamAuth()
    {
        echo "Test: ElastiCache IAM Authentication\n";
        echo "-------------------------------------\n";

        try {
            $config = TEST_CONFIG['elasticache'];

            $client = new ValkeyGlide();
            $client->connect(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    'username' => $config['username'],
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'Elasticache'
                    ]
                ]
            );

            // Test basic operations
            $response = $client->ping();
            $this->assert($response === 'PONG', "PING should return PONG");

            $client->set('iam-test-key', 'test-value');
            $value = $client->get('iam-test-key');
            $this->assert($value === 'test-value', "GET should return set value");

            $client->del('iam-test-key');

            unset($client);

            echo "✓ Test passed\n\n";
            $this->testsPassed++;
        } catch (Exception $e) {
            echo "✗ Test failed: " . $e->getMessage() . "\n\n";
            $this->testsFailed++;
        }
    }

    private function testMemoryDBIamAuth()
    {
        echo "Test: MemoryDB IAM Authentication\n";
        echo "----------------------------------\n";

        try {
            $config = TEST_CONFIG['memorydb'];

            $client = new ValkeyGlide();
            $client->connect(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    'username' => $config['username'],
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'MemoryDB'
                    ]
                ]
            );

            $response = $client->ping();
            $this->assert($response === 'PONG', "PING should return PONG");

            unset($client);

            echo "✓ Test passed\n\n";
            $this->testsPassed++;
        } catch (Exception $e) {
            echo "✗ Test failed: " . $e->getMessage() . "\n\n";
            $this->testsFailed++;
        }
    }

    private function testClusterModeIamAuth()
    {
        echo "Test: Cluster Mode IAM Authentication\n";
        echo "--------------------------------------\n";

        try {
            $config = TEST_CONFIG['elasticache'];

            $client = new ValkeyGlideCluster(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    'username' => $config['username'],
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'Elasticache'
                    ]
                ]
            );

            $response = $client->ping();
            $this->assert($response === 'PONG', "PING should return PONG");

            unset($client);

            echo "✓ Test passed\n\n";
            $this->testsPassed++;
        } catch (Exception $e) {
            echo "✗ Test failed: " . $e->getMessage() . "\n\n";
            $this->testsFailed++;
        }
    }

    private function testCustomRefreshInterval()
    {
        echo "Test: Custom Refresh Interval\n";
        echo "------------------------------\n";

        try {
            $config = TEST_CONFIG['elasticache'];

            $client = new ValkeyGlide();
            $client->connect(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    'username' => $config['username'],
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'Elasticache',
                        'refreshIntervalSeconds' => 120  // 2 minutes
                    ]
                ]
            );

            $response = $client->ping();
            $this->assert($response === 'PONG', "PING should return PONG");

            unset($client);

            echo "✓ Test passed\n\n";
            $this->testsPassed++;
        } catch (Exception $e) {
            echo "✗ Test failed: " . $e->getMessage() . "\n\n";
            $this->testsFailed++;
        }
    }

    private function testMissingUsername()
    {
        echo "Test: Missing Username Error\n";
        echo "----------------------------\n";

        try {
            $config = TEST_CONFIG['elasticache'];

            // This should throw an exception
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    // Missing 'username'
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'Elasticache'
                    ]
                ]
            );

            echo "✗ Test failed: Should have thrown exception for missing username\n\n";
            $this->testsFailed++;
        } catch (Exception $e) {
            $this->assert(
                strpos($e->getMessage(), 'username') !== false,
                "Exception should mention username"
            );
            echo "✓ Test passed (expected exception caught)\n\n";
            $this->testsPassed++;
        }
    }

    private function testInvalidRefreshInterval()
    {
        echo "Test: Invalid Refresh Interval\n";
        echo "-------------------------------\n";

        try {
            $config = TEST_CONFIG['elasticache'];

            // This should throw an exception for invalid interval
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [
                    ['host' => $config['endpoint'], 'port' => $config['port']]
                ],
                use_tls: true,
                credentials: [
                    'username' => $config['username'],
                    'iamConfig' => [
                        'clusterName' => $config['cluster_name'],
                        'region' => $config['region'],
                        'service' => 'Elasticache',
                        'refreshIntervalSeconds' => 50000  // Too large (max is 43200)
                    ]
                ]
            );

            echo "✗ Test failed: Should have thrown exception for invalid refresh interval\n\n";
            $this->testsFailed++;
        } catch (Exception $e) {
            $this->assert(
                strpos($e->getMessage(), 'refresh interval') !== false ||
                strpos($e->getMessage(), 'Invalid') !== false,
                "Exception should mention refresh interval"
            );
            echo "✓ Test passed (expected exception caught)\n\n";
            $this->testsPassed++;
        }
    }

    private function assert($condition, $message)
    {
        if (!$condition) {
            throw new Exception("Assertion failed: $message");
        }
    }
}

// Run tests
$test = new IamAuthTest();
$success = $test->run();

exit($success ? 0 : 1);
