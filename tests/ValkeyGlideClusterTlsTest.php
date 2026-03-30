<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideClusterBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

/**
 * TLS tests for Valkey GLIDE cluster client.
 */
class ValkeyGlideClusterTlsTest extends ValkeyGlideClusterBaseTest
{
    public function testTlsWithoutCertificate()
    {
        $this->skipIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            new ValkeyGlideCluster(
                addresses: [self::TLS_ADDRESS_CLUSTER],
                use_tls: true
            );
        });
    }

    public function testTlsWithCertificateStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [self::TLS_ADDRESS_CLUSTER],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithCertificateConfig()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [self::TLS_ADDRESS_CLUSTER],
            use_tls: true,
            advanced_config: ['tls_config' => ['root_certs' => $this->getCaCertificate()]]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithMultipleCertificates()
    {
        $this->skipIfTlsDisabled();

        $certData = $this->getCaCertificate();
        $multipleCerts = $certData . "\n" . $certData;

        $client = new ValkeyGlideCluster(
            addresses: [self::TLS_ADDRESS_CLUSTER],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $multipleCerts]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithEmptyCertificate()
    {
        $this->skipIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlideCluster(
                addresses: [self::TLS_ADDRESS_CLUSTER],
                use_tls: true,
                advanced_config: [
                    'tls_config' => ['root_certs' => '']
                ]
            );
        });
    }

    public function testTlsWithInvalidCertificate()
    {
        $this->skipIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlideCluster(
                addresses: [self::TLS_ADDRESS_CLUSTER],
                use_tls: true,
                advanced_config: [
                    'tls_config' => ['root_certs' => 'invalid-certificate-data']
                ]
            );
        });
    }

    public function testTlsWithIPv4Address()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_CLUSTER['port']
            ]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $this->getCaCertificate()]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithIPv6Address()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV6,
                'port' => self::TLS_ADDRESS_CLUSTER['port']
            ]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $this->getCaCertificate()]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithInsecureStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [self::TLS_ADDRESS_CLUSTER],
            context: stream_context_create(['ssl' => ['verify_peer' => false]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithInsecureConfig()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlideCluster(
            addresses: [self::TLS_ADDRESS_CLUSTER],
            use_tls: true,
            advanced_config: ['tls_config' => ['use_insecure_tls' => true]]
        );

        $this->assertConnected($client);
        $client->close();
    }
}
