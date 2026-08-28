<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';

/**
 * Integration tests for the standalone NodeDiscoveryMode configuration option.
 *
 * These tests rely on the primary/replica topology started by
 * tests/start_valkey_with_replicas.sh:
 *   - Primary:  127.0.0.1:6379
 *   - Replica:  127.0.0.1:6380 (REPLICAOF 6379)
 *   - Replica:  127.0.0.1:6381 (REPLICAOF 6379)
 *
 * See the reference implementations in the core repository under
 * python/tests/.../test_node_discovery_mode and node/tests/NodeDiscoveryMode.test.ts.
 */
class ValkeyGlideNodeDiscoveryModeTest extends ValkeyGlideBaseTest
{
    private const PRIMARY_PORT  = 6379;
    private const REPLICA0_PORT = 6380;
    private const REPLICA1_PORT = 6381;

    /**
     * These tests require a real primary/replica topology on the well-known
     * ports and are not applicable to TLS-only runs.
     */
    public function setUp()
    {
        // Intentionally do not call parent::setUp(); each test creates its own client.
        $this->skipIfTlsEnabled();
        $this->skipIfReplicasUnavailable();
    }

    public function tearDown()
    {
        // No shared client to close.
    }

    /**
     * Skips the test if the replica topology is not reachable.
     */
    private function skipIfReplicasUnavailable(): void
    {
        foreach ([self::PRIMARY_PORT, self::REPLICA0_PORT, self::REPLICA1_PORT] as $port) {
            $conn = @fsockopen('127.0.0.1', $port, $errno, $errstr, 0.5);
            if ($conn === false) {
                $this->markTestSkipped(
                    "Replica topology not available on port {$port}. " .
                    "Run tests/start_valkey_with_replicas.sh to enable NodeDiscoveryMode tests."
                );
            }
            fclose($conn);
        }
    }

    /**
     * Connects a probe client directly to a single node (typically a replica).
     *
     * STATIC mode is used so the client trusts the provided address as-is and
     * skips role detection (a plain STANDARD connection to a replica-only address
     * would fail with "No primary node found").
     *
     * @param int $port The node port to connect the probe to.
     */
    private function connectProbe(int $port): ValkeyGlide
    {
        $probe = new ValkeyGlide();
        $probe->connect(
            addresses: [['host' => '127.0.0.1', 'port' => $port]],
            request_timeout: 2000,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_STATIC
        );
        return $probe;
    }

    /* ----------------------------------------------------------------------
     * Type validation for the node_discovery_mode parameter
     * ------------------------------------------------------------------- */

    /**
     * The public API declares node_discovery_mode as ?int. Non-null, non-integer
     * values must be rejected rather than silently coerced (e.g. false -> 0/STANDARD)
     * or misread through the wrong zval union member.
     */
    public function testNodeDiscoveryModeRejectsNonIntegerTypes()
    {
        $invalidValues = [
            'boolean' => false,
            'string'  => 'static',
            'float'   => 1.5,
            'array'   => [1],
            'object'  => new stdClass(),
        ];

        foreach ($invalidValues as $type => $value) {
            $this->assertThrows(ValkeyGlideException::class, function () use ($value) {
                $client = new ValkeyGlide();
                $client->connect(
                    addresses: [['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT]],
                    request_timeout: 2000,
                    node_discovery_mode: $value
                );
            });
        }
    }

    /* ----------------------------------------------------------------------
     * STANDARD mode (default)
     * ------------------------------------------------------------------- */

    public function testStandardModeConnectsAndReads()
    {
        // STANDARD is the default mode: it verifies node roles via INFO REPLICATION
        // and uses only the provided addresses. Connecting to the primary should
        // succeed and allow reads and writes.
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT]],
            request_timeout: 2000,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_STANDARD
        );

        $this->assertConnected($client);

        $key = 'ndm_standard_' . uniqid();
        try {
            $this->assertTrue($client->set($key, 'value'));
            $this->assertEquals('value', $client->get($key));
        } finally {
            $client->del($key);
            $client->close();
        }
    }

    /* ----------------------------------------------------------------------
     * STATIC mode
     * ------------------------------------------------------------------- */

    public function testStaticModeConnectsAndReads()
    {
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT]],
            request_timeout: 2000,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_STATIC
        );

        // STATIC mode skips INFO REPLICATION role detection; verify the client can
        // still connect and execute a read command against the trusted address.
        $this->assertConnected($client);

        // A read round-trips successfully; a missing key returns the client's nil value.
        $this->assertEquals($this->getNilValue(), $client->get('nonexistent_' . uniqid()));

        $client->close();
    }

    public function testStaticModeAllowsWrites()
    {
        // Use only the primary address with STATIC mode; the first address is
        // treated as the primary, so writes must succeed.
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT]],
            request_timeout: 2000,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_STATIC
        );

        $key = 'ndm_static_write_' . uniqid();
        try {
            $this->assertTrue($client->set($key, 'value'));
            $this->assertEquals('value', $client->get($key));
        } finally {
            $client->del($key);
            $client->close();
        }
    }

    /* ----------------------------------------------------------------------
     * DISCOVER_ALL mode
     * ------------------------------------------------------------------- */

    public function testDiscoverAllFromPrimary()
    {
        $uniqueName = 'ndm_discover_primary_' . uniqid();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT]],
            request_timeout: 2000,
            client_name: $uniqueName,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_DISCOVER_ALL
        );

        // A separate probe connected directly to a replica should observe the
        // discovery client's connection, proving the topology was discovered.
        $probe = $this->connectProbe(self::REPLICA0_PORT);

        try {
            $this->waitFor(function () use ($probe, $uniqueName) {
                $clientList = (string) $probe->rawcommand('CLIENT', 'LIST');
                return str_contains($clientList, $uniqueName);
            }, 10, "Discovery client '{$uniqueName}' was not found on the replica");
        } finally {
            $probe->close();
            $client->close();
        }
    }

    public function testDiscoverAllFromReplica()
    {
        $uniqueName = 'ndm_discover_replica_' . uniqid();

        // Start discovery from a replica address; the client should discover the
        // primary and connect to the full topology.
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => '127.0.0.1', 'port' => self::REPLICA0_PORT]],
            request_timeout: 2000,
            client_name: $uniqueName,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_DISCOVER_ALL
        );

        $key = 'ndm_discover_replica_key_' . uniqid();

        $probe = $this->connectProbe(self::REPLICA0_PORT);

        try {
            // Writes must be routed to the discovered primary.
            $this->assertTrue($client->set($key, 'value'));
            $this->assertEquals('value', $client->get($key));

            $this->waitFor(function () use ($probe, $uniqueName) {
                $clientList = (string) $probe->rawcommand('CLIENT', 'LIST');
                return str_contains($clientList, $uniqueName);
            }, 10, "Discovery client '{$uniqueName}' was not found on the replica");
        } finally {
            $client->del($key);
            $probe->close();
            $client->close();
        }
    }

    public function testDiscoverAllFromPartialAddresses()
    {
        $uniqueName = 'ndm_discover_partial_' . uniqid();

        // Provide a subset of addresses (primary + one replica); the client must
        // still discover and connect to the remaining replica.
        $client = new ValkeyGlide();
        $client->connect(
            addresses: [
                ['host' => '127.0.0.1', 'port' => self::PRIMARY_PORT],
                ['host' => '127.0.0.1', 'port' => self::REPLICA0_PORT],
            ],
            request_timeout: 2000,
            client_name: $uniqueName,
            node_discovery_mode: ValkeyGlide::NODE_DISCOVERY_MODE_DISCOVER_ALL
        );

        $probe = $this->connectProbe(self::REPLICA1_PORT);

        try {
            $this->waitFor(function () use ($probe, $uniqueName) {
                $clientList = (string) $probe->rawcommand('CLIENT', 'LIST');
                return str_contains($clientList, $uniqueName);
            }, 10, "Discovery client '{$uniqueName}' was not found on the second replica");
        } finally {
            $probe->close();
            $client->close();
        }
    }
}
