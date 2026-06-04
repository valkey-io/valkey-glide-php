<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideClusterBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

class AddressResolverClusterTest extends ValkeyGlideClusterBaseTest
{
    public function testAddressResolverWithFakeAddress()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();

        $resolver = function (string $host, int $port) use ($realHost, $realPort): array {
            return ['host' => $realHost, 'port' => $realPort];
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'fake.nonexistent.host', 'port' => 9999]],
            address_resolver: $resolver,
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testAddressResolverExceptionFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): array {
            $called = true;
            throw new RuntimeException("resolver error");
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertTrue($called);
        $this->assertConnected($client);
        $client->close();
    }

    public function testAddressResolverReturnsInvalidFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): mixed {
            $called = true;
            return null;
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertTrue($called);
        $this->assertConnected($client);
        $client->close();
    }

    /**
     * Two cluster clients with different resolvers don't interfere with each other.
     */
    public function testMultiClientResolversAreIsolated()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();

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

        $clientA = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
            address_resolver: $resolverA,
        );

        $clientB = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
            address_resolver: $resolverB,
        );

        $this->assertTrue($calledA, 'Resolver A must have been invoked for client A');
        $this->assertTrue($calledB, 'Resolver B must have been invoked for client B');
        $this->assertConnected($clientA);
        $this->assertConnected($clientB);

        $clientA->close();
        $clientB->close();
    }

    public function testNullAddressResolverConnectsNormally()
    {
        $this->skipIfTlsEnabled();

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: null,
        );

        $this->assertConnected($client);
        $client->close();
    }
}
