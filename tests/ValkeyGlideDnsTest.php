<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';

/**
 * DNS Resolution Tests for Standalone ValkeyGlide
 */
class ValkeyGlideDnsTest extends ValkeyGlideBaseTest
{
    /**
     * Marks the current test as skipped if DNS tests are disabled.
     */
    protected function markTestSkippedIfDnsTestsDisabled(): void
    {
        if (!getenv('VALKEY_GLIDE_DNS_TESTS_ENABLED')) {
            $this->markTestSkipped('DNS tests are disabled. Set VALKEY_GLIDE_DNS_TESTS_ENABLED=1 to enable.');
        }
    }

    // Non-TLS DNS Tests
    // -----------------

    public function testDnsConnectWithValidHostname()
    {
        $this->markTestSkippedIfDnsTestsDisabled();
        $this->markTestSkippedIfTlsEnabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => self::HOSTNAME_NO_TLS, 'port' => $this->getPort()]]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testDnsConnectWithInvalidHostname()
    {
        $this->markTestSkippedIfDnsTestsDisabled();
        $this->markTestSkippedIfTlsEnabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [['host' => 'nonexistent.invalid', 'port' => $this->getPort()]],
                advanced_config: ['connection_timeout' => 5000]
            );
        });
    }

    // TLS DNS Tests
    // -------------

    public function testDnsTlsWithHostnameInCertificate()
    {
        $this->markTestSkippedIfDnsTestsDisabled();
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [['host' => self::HOSTNAME_TLS, 'port' => self::TLS_ADDRESS_STANDALONE['port']]],
            use_tls: true,
            advanced_config: [
                'connection_timeout' => 5000,
                'tls_config' => ['root_certs' => $certData]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testDnsTlsWithHostnameNotInCertificate()
    {
        $this->markTestSkippedIfDnsTestsDisabled();
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $this->assertThrows(ValkeyGlideException::class, function () use ($certData) {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [['host' => self::HOSTNAME_NO_TLS, 'port' => self::TLS_ADDRESS_STANDALONE['port']]],
                use_tls: true,
                advanced_config: [
                    'connection_timeout' => 5000,
                    'tls_config' => ['root_certs' => $certData]
                ]
            );
        });
    }
}
