<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideBaseTest.php';

/**
 * TLS Certificate Validation Tests for Standalone ValkeyGlide
 */
class ValkeyGlideTlsTest extends ValkeyGlideBaseTest
{
    // Existing TLS Tests (from ValkeyGlideTest.php)
    // -----------------------------------------------

    public function testTlsSecureStream()
    {
        $this->markTestSkippedIfTlsDisabled();

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
        $this->markTestSkippedIfTlsDisabled();

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
        $this->markTestSkippedIfTlsDisabled();

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
        $this->markTestSkippedIfTlsDisabled();

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

    // Certificate Validation Tests
    // -----------------------------

    public function testTlsWithoutCertificate()
    {
        $this->markTestSkippedIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => self::HOST_ADDRESS_IPV4,
                    'port' => self::TLS_ADDRESS_STANDALONE['port']
                ]],
                use_tls: true
            );
        });
    }

    public function testTlsWithSelfSignedCertificate()
    {
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV4,
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
        $this->markTestSkippedIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithMultipleCertificates()
    {
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();
        $multipleCerts = $certData . "\n" . $certData;

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV4,
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
        $this->markTestSkippedIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => self::HOST_ADDRESS_IPV4,
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
        $this->markTestSkippedIfTlsDisabled();

        $this->assertThrows(ValkeyGlideException::class, function () {
            $client = new ValkeyGlide();
            $client->connect(
                addresses: [[
                    'host' => self::HOST_ADDRESS_IPV4,
                    'port' => self::TLS_ADDRESS_STANDALONE['port']
                ]],
                use_tls: true,
                advanced_config: [
                    'tls_config' => ['root_certs' => 'invalid-certificate-data']
                ]
            );
        });
    }

    // IP Address Tests
    // ----------------

    public function testTlsWithIPv4Address()
    {
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV4,
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
        $this->markTestSkippedIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV4,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testTlsWithIPv6Address()
    {
        $this->markTestSkippedIfTlsDisabled();

        $certData = $this->getCaCertificate();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV6,
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
        $this->markTestSkippedIfTlsDisabled();

        $client = new ValkeyGlide();
        $client->connect(
            addresses: [[
                'host' => self::HOST_ADDRESS_IPV6,
                'port' => self::TLS_ADDRESS_STANDALONE['port']
            ]],
            context: stream_context_create(['ssl' => ['cafile' => self::TLS_CERTIFICATE_PATH]])
        );

        $this->assertConnected($client);
        $client->close();
    }
}
