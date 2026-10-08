<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideClusterBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

class AddressResolverClusterTest extends ValkeyGlideClusterBaseTest
{
    public function setUp()
    {
        // Intentionally do not call parent::setUp(); each test creates its own client.
    }

    public function tearDown()
    {
        // No shared client to close.
    }

    private function newClusterClient(array $addresses, ?callable $resolver): ValkeyGlideCluster
    {
        return new ValkeyGlideCluster(
            addresses: $addresses,
            address_resolver: $resolver,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED,
        );
    }

    public function testAddressResolverWithFakeAddress()
    {
        $this->skipIfTlsEnabled();

        $primaryAddress = $this->getClusterAddresses()[0];
        $realHost = $primaryAddress['host'];
        $realPort = $primaryAddress['port'];

        $resolver = function (string $host, int $port) use ($realHost, $realPort): array {
            return ['host' => $realHost, 'port' => $realPort];
        };

        $client = $this->newClusterClient(
            addresses: [['host' => 'fake.nonexistent.host', 'port' => 9999]],
            resolver: $resolver,
        );

        try {
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    public function testAddressResolverExceptionFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): array {
            $called = true;
            throw new RuntimeException("resolver error");
        };

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: $resolver,
        );

        try {
            $this->assertTrue($called);
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    public function testAddressResolverReturnsInvalidFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): mixed {
            $called = true;
            return null;
        };

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: $resolver,
        );

        try {
            $this->assertTrue($called);
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    /**
     * Two cluster clients with different resolvers don't interfere with each other.
     */
    public function testMultiClientResolversAreIsolated()
    {
        $this->skipIfTlsEnabled();

        $primaryAddress = $this->getClusterAddresses()[0];
        $realHost = $primaryAddress['host'];
        $realPort = $primaryAddress['port'];

        $calledA = false;
        $resolverA = function (string $host, int $port) use ($realHost, $realPort, &$calledA): array {
            $calledA = true;
            return ['host' => $realHost, 'port' => $realPort];
        };

        $calledB = false;
        $resolverB = function (string $host, int $port) use ($realHost, $realPort, &$calledB): array {
            $calledB = true;
            return ['host' => $realHost, 'port' => $realPort];
        };

        $clientA = null;
        $clientB = null;
        try {
            $clientA = $this->newClusterClient(
                addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
                resolver: $resolverA,
            );

            $clientB = $this->newClusterClient(
                addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
                resolver: $resolverB,
            );

            $this->assertTrue($calledA, 'Resolver A must have been invoked for client A');
            $this->assertTrue($calledB, 'Resolver B must have been invoked for client B');
            $this->assertConnected($clientA);
            $this->assertConnected($clientB);
        } finally {
            $clientA?->close();
            $clientB?->close();
        }
    }

    public function testNullAddressResolverConnectsNormally()
    {
        $this->skipIfTlsEnabled();

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: null,
        );

        try {
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }
}
