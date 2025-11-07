<?php

require_once 'TestSuite.php';

class ValkeyGlideOtelTest extends TestSuite
{
    public function testOtelConfigurationParsing()
    {
        // Test that OTEL configuration is accepted without errors
        $otelConfig = [
            'traces' => [
                'endpoint' => 'grpc://localhost:4317',
                'sample_percentage' => 50
            ],
            'metrics' => [
                'endpoint' => 'grpc://localhost:4317'
            ],
            'flush_interval_ms' => 2000
        ];

        try {
            $client = new ValkeyGlide(
                addresses: [['host' => 'localhost', 'port' => 6379]],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                database_id: 0,
                client_name: 'otel-test-client',
                client_az: null,
                advanced_config: [
                    'connection_timeout' => 5000,
                    'otel' => $otelConfig
                ]
            );

            // If we get here, OTEL config was accepted
            $this->assertTrue(true);
            $client->close();
        } catch (Exception $e) {
            // OTEL config should not cause construction to fail
            $this->fail("OTEL configuration caused client construction to fail: " . $e->getMessage());
        }
    }

    public function testOtelWithTracesOnly()
    {
        $otelConfig = [
            'traces' => [
                'endpoint' => 'file:///tmp/valkey-traces.json',
                'sample_percentage' => 100
            ]
        ];

        try {
            $client = new ValkeyGlide(
                addresses: [['host' => 'localhost', 'port' => 6379]],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                database_id: 0,
                client_name: 'otel-traces-test',
                client_az: null,
                advanced_config: [
                    'otel' => $otelConfig
                ]
            );

            // Perform some operations to generate traces
            $client->set('otel:trace:test', 'value');
            $value = $client->get('otel:trace:test');
            $this->assertEquals('value', $value);
            $client->del('otel:trace:test');

            $client->close();
            $this->assertTrue(true);
        } catch (Exception $e) {
            $this->fail("Traces-only OTEL config failed: " . $e->getMessage());
        }
    }

    public function testOtelWithMetricsOnly()
    {
        $otelConfig = [
            'metrics' => [
                'endpoint' => 'file:///tmp/valkey-metrics.json'
            ]
        ];

        try {
            $client = new ValkeyGlide(
                addresses: [['host' => 'localhost', 'port' => 6379]],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                database_id: 0,
                client_name: 'otel-metrics-test',
                client_az: null,
                advanced_config: [
                    'otel' => $otelConfig
                ]
            );

            // Perform some operations to generate metrics
            $client->set('otel:metric:test', 'value');
            $client->get('otel:metric:test');
            $client->del('otel:metric:test');

            $client->close();
            $this->assertTrue(true);
        } catch (Exception $e) {
            $this->fail("Metrics-only OTEL config failed: " . $e->getMessage());
        }
    }

    public function testOtelClusterSupport()
    {
        $otelConfig = [
            'traces' => [
                'endpoint' => 'file:///tmp/valkey-cluster-traces.json',
                'sample_percentage' => 100
            ]
        ];

        try {
            $client = new ValkeyGlideCluster(
                addresses: [['host' => 'localhost', 'port' => 7001]],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                client_name: 'otel-cluster-test',
                periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
                client_az: null,
                advanced_config: [
                    'otel' => $otelConfig
                ]
            );

            // Perform cluster operations to generate traces
            $client->set('otel:cluster:test', 'value');
            $value = $client->get('otel:cluster:test');
            $this->assertEquals('value', $value);
            $client->del('otel:cluster:test');

            $client->close();
            $this->assertTrue(true);
        } catch (Exception $e) {
            $this->fail("Cluster OTEL config failed: " . $e->getMessage());
        }
    }

    public function testOtelWithoutConfiguration()
    {
        // Test that client works normally without OTEL config
        try {
            $client = new ValkeyGlide(
                addresses: [['host' => 'localhost', 'port' => 6379]],
                use_tls: false,
                credentials: null,
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
                request_timeout: null,
                reconnect_strategy: null,
                database_id: 0,
                client_name: 'no-otel-test',
                client_az: null,
                advanced_config: [
                    'connection_timeout' => 5000
                    // No OTEL config
                ]
            );

            // Operations should work normally
            $client->set('no:otel:test', 'value');
            $value = $client->get('no:otel:test');
            $this->assertEquals('value', $value);
            $client->del('no:otel:test');

            $client->close();
            $this->assertTrue(true);
        } catch (Exception $e) {
            $this->fail("Client without OTEL should work normally: " . $e->getMessage());
        }
    }
}
