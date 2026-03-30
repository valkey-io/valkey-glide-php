<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

/**
 * DNS resolution tests for standalone client.
 * See DEVELOPER.md#dns-tests for setup instructions.
 */
class ValkeyGlideDnsTest extends ValkeyGlideBaseTest
{
    public function testDnsConnectWithValidHostname()
    {
        $this->skipIfDnsNotEnabled();
        $this->skipIfTlsEnabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOSTNAME_NO_TLS,
                'port' => $this->getPort()
            ]]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testDnsConnectWithInvalidHostname()
    {
        $this->skipIfDnsNotEnabled();
        $this->skipIfTlsEnabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => 'nonexistent.invalid',
                    'port' => $this->getPort()
                ]],
            );
        });
    }

    public function testDnsTlsWithHostnameInCertificate()
    {
        $this->skipIfDnsNotEnabled();
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOSTNAME_TLS,
                'port' => $this->getPort()
            ]],
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
        $this->skipIfDnsNotEnabled();
        $this->skipIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => TestConstants::HOSTNAME_NO_TLS,
                    'port' => $this->getPort()
                ]],
                use_tls: true,
                advanced_config: [
                    'tls_config' => ['root_certs' => $this->getCaCertificate()]
                ]
            );
        });
    }
}
