<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

/**
 * Integration tests for the AddressResolver feature.
 */
class AddressResolverTest extends ValkeyGlideBaseTest
{
    /**
     * Resolver maps a fake host/port to the real server — ping() succeeds.
     */
    public function testAddressResolverWithFakeAddress()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();

        $resolver = function (string $host, int $port) use ($realHost, $realPort): array {
            return ['host' => $realHost, 'port' => $realPort];
        };

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => 'fake.nonexistent.host', 'port' => 9999]],
            address_resolver: $resolver,
        );

        $this->assertConnected($client);
        $client->close();
    }

    /**
     * Resolver throws an exception — client falls back to original address, ping() succeeds.
     */
    public function testAddressResolverExceptionFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $resolver = function (string $host, int $port): array {
            throw new RuntimeException("resolver error");
        };

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertConnected($client);
        $client->close();
    }

    /**
     * Resolver returns invalid data (null) — client falls back to original address, ping() succeeds.
     */
    public function testAddressResolverReturnsInvalidFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $resolver = function (string $host, int $port): mixed {
            return null;
        };

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertConnected($client);
        $client->close();
    }

    /**
     * No resolver — normal connection works.
     */
    public function testNullAddressResolverConnectsNormally()
    {
        $this->skipIfTlsEnabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: null,
        );

        $this->assertConnected($client);
        $client->close();
    }
}
