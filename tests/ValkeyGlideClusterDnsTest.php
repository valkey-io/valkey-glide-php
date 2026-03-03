<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';

/**
 * DNS resolution tests for cluster client.
 * 
 * To run these tests, you need to add the following mappings to your hosts
 * file then set the environment variable VALKEY_GLIDE_DNS_TESTS_ENABLED:
 * - 127.0.0.1 valkey.glide.test.tls.com
 * - 127.0.0.1 valkey.glide.test.no_tls.com
 * - ::1 valkey.glide.test.tls.com
 * - ::1 valkey.glide.test.no_tls.com
 */
class ValkeyGlideClusterDnsTest extends ValkeyGlideBaseTest
{
    /**
     * Skips the current test if DNS tests are not enabled.
     */
    protected function skipIfNotEnabled(): void
    {
        if (!getenv('VALKEY_GLIDE_DNS_TESTS_ENABLED')) {
            $this->markTestSkipped('DNS tests are disabled. Set VALKEY_GLIDE_DNS_TESTS_ENABLED=1 to enable.');
        }
    }

    public function testDnsConnectWithValidHostname()
    {
        $this->skipIfNotEnabled();
        $this->markTestSkippedIfTlsEnabled();

        $client = new ValkeyGlideCluster(
            addresses: [['host' => self::HOSTNAME_NO_TLS, 'port' => $this->getPort()]]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testDnsConnectWithInvalidHostname()
    {
        $this->skipIfNotEnabled();
        $this->markTestSkippedIfTlsEnabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlideCluster(
                addresses: [['host' => 'nonexistent.invalid', 'port' => $this->getPort()]],
            );
        });
    }

    public function testDnsTlsWithHostnameInCertificate()
    {
        $this->skipIfNotEnabled();
        $this->markTestSkippedIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [['host' => self::HOSTNAME_TLS, 'port' => self::TLS_ADDRESS_CLUSTER['port']]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $this->getCaCertificate()]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testDnsTlsWithHostnameNotInCertificate()
    {
        $this->skipIfNotEnabled();
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $this->assertThrows(ValkeyGlideException::class, function () use ($certData) {
            $client = new ValkeyGlideCluster(
                addresses: [['host' => self::HOSTNAME_NO_TLS, 'port' => self::TLS_ADDRESS_CLUSTER['port']]],
                use_tls: true,
                advanced_config: [
                    'tls_config' => ['root_certs' => $certData]
                ]
            );
        });
    }
}
