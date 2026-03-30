<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

/**
 * TLS tests for Valkey GLIDE standalone client.
 */
class ValkeyGlideTlsTest extends ValkeyGlideBaseTest
{
    public function testTlsSecureStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [self::TLS_ADDRESS_STANDALONE],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsSecureConfig()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [self::TLS_ADDRESS_STANDALONE],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => file_get_contents(self::TLS_CERTIFICATE_PATH)]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsInsecureStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [self::TLS_ADDRESS_STANDALONE],
            context: stream_context_create(['ssl' => ['verify_peer' => false]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsInsecureConfig()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [self::TLS_ADDRESS_STANDALONE],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['use_insecure_tls' => true]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithoutCertificate()
    {
        $this->skipIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => TestConstants::HOST_ADDRESS_IPV4,
                    'port' => self::TLS_ADDRESS_STANDALONE['port']
                ]],
                use_tls: true
            );
        });
    }

    public function testTlsWithSelfSignedCertificate()
    {
        $this->skipIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $certData]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithSelfSignedCertificateStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithMultipleCertificates()
    {
        $this->skipIfTlsDisabled();

        $certData = $this->getCaCertificate();
        $multipleCerts = $certData . "\n" . $certData;

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
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
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => TestConstants::HOST_ADDRESS_IPV4,
                    'port' => self::TLS_ADDRESS_STANDALONE['port']
                ]],
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
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => TestConstants::HOST_ADDRESS_IPV4,
                    'port' => self::TLS_ADDRESS_STANDALONE['port']
                ]],
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

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $certData]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithIPv4AddressStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithIPv6Address()
    {
        $this->skipIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV6,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            use_tls: true,
            advanced_config: [
                'tls_config' => ['root_certs' => $certData]
            ]
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithIPv6AddressStream()
    {
        $this->skipIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => TestConstants::HOST_ADDRESS_IPV6,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }
}
