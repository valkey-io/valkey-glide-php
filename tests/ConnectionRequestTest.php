<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");
/*
* --------------------------------------------------------------------
*                   The PHP License, version 3.01
* Copyright (c) 1999 - 2010 The PHP Group. All rights reserved.
* --------------------------------------------------------------------
*
* Redistribution and use in source and binary forms, with or without
* modification, is permitted provided that the following conditions
* are met:
*
*   1. Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in
*      the documentation and/or other materials provided with the
*      distribution.
*
*   3. The name "PHP" must not be used to endorse or promote products
*      derived from this software without prior written permission. For
*      written permission, please contact group@php.net.
*
*   4. Products derived from this software may not be called "PHP", nor
*      may "PHP" appear in their name, without prior written permission
*      from group@php.net.  You may indicate that your software works in
*      conjunction with PHP by saying "Foo for PHP" instead of calling
*      it "PHP Foo" or "phpfoo"
*
*   5. The PHP Group may publish revised and/or new versions of the
*      license from time to time. Each version will be given a
*      distinguishing version number.
*      Once covered code has been published under a particular version
*      of the license, you may always continue to use it under the terms
*      of that version. You may also choose to use such covered code
*      under the terms of any subsequent version of the license
*      published by the PHP Group. No one other than the PHP Group has
*      the right to modify the terms applicable to covered code created
*      under this License.
*
*   6. Redistributions of any form whatsoever must retain the following
*      acknowledgment:
*      "This product includes PHP software, freely available from
*      <http://www.php.net/software/>".
*
* THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
* ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
* DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* --------------------------------------------------------------------
*
* This software consists of voluntary contributions made by many
* individuals on behalf of the PHP Group.
*
* The PHP Group can be contacted via Email at group@php.net.
*
* For more information on the PHP Group and the PHP project,
* please see <http://www.php.net>.
*
* PHP includes the Zend Engine, freely available at
* <http://www.zend.com>.
*/

require_once __DIR__ . "/TestSuite.php";
require_once __DIR__ . "/../vendor/autoload.php";
require_once __DIR__ . "/Connection_request/AuthenticationInfo.php";
require_once __DIR__ . "/Connection_request/ConnectionRequest.php";
require_once __DIR__ . "/Connection_request/ConnectionRetryStrategy.php";
require_once __DIR__ . "/Connection_request/NodeAddress.php";
require_once __DIR__ . "/Connection_request/PeriodicChecksDisabled.php";
require_once __DIR__ . "/Connection_request/PeriodicChecksManualInterval.php";
require_once __DIR__ . "/Connection_request/ProtocolVersion.php";
require_once __DIR__ . "/Connection_request/PubSubChannelsOrPatterns.php";
require_once __DIR__ . "/Connection_request/PubSubChannelType.php";
require_once __DIR__ . "/Connection_request/PubSubSubscriptions.php";
require_once __DIR__ . "/Connection_request/ReadFrom.php";
require_once __DIR__ . "/Connection_request/TlsMode.php";
require_once __DIR__ . "/GPBMetadata/ConnectionRequest.php";

/**
 * ValkeyGlide Base Test Class
 * Abstract base class providing infrastructure methods for ValkeyGlide tests
 * Contains no actual test methods - only setup and helper functionality
 */
class ConnectionRequestTest extends \TestSuite
{
    /** Internal helper function to call from C to deserialize the message to a ConnectionRequest object */
    public static function deserialize($data): \Connection_request\ConnectionRequest
    {
        $connection_request = new \Connection_request\ConnectionRequest();
        $connection_request->mergeFromString($data);
        return $connection_request;
    }

    public function setUp()
    {
        // No-op.
    }

    public function testStandaloneBasicConstructor()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor([
            ['host' => 'localhost', 'port' => 8080]
        ]);

        $this->assertFalse($request->getClusterModeEnabled());

        $addresses = $request->getAddresses();
        $this->assertEquals(1, count($addresses));

        $address = $addresses[0];
        $this->assertEquals('localhost', $address->getHost());
        $this->assertEquals(8080, $address->getPort());
    }

    public function testStandaloneMultipleAddresses()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor([
            ['host' => 'localhost', 'port' => 8080],
            ['host' => '172.0.1.24', 'port' => 9000]]);

        $addresses = $request->getAddresses();
        $this->assertEquals(2, count($addresses));

        $address = $addresses[0];
        $this->assertEquals('localhost', $address->getHost());
        $this->assertEquals(8080, $address->getPort());

        $address = $addresses[1];
        $this->assertEquals('172.0.1.24', $address->getHost());
        $this->assertEquals(9000, $address->getPort());
    }

    public function testClusterBasicConstructor()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor([
            ['host' => 'localhost', 'port' => 8080]
        ]);
        $this->assertTrue($request->getClusterModeEnabled());

        $addresses = $request->getAddresses();
        $this->assertEquals(1, count($addresses));

        $address = $addresses[0];
        $this->assertEquals('localhost', $address->getHost());
        $this->assertEquals(8080, $address->getPort());
    }

    public function testClusterMultipleAddresses()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor([
            ['host' => 'localhost', 'port' => 8080],
            ['host' => '172.0.1.24', 'port' => 9000]]);

        $addresses = $request->getAddresses();
        $this->assertEquals(2, count($addresses));

        $address = $addresses[0];
        $this->assertEquals('localhost', $address->getHost());
        $this->assertEquals(8080, $address->getPort());

        $address = $addresses[1];
        $this->assertEquals('172.0.1.24', $address->getHost());
        $this->assertEquals(9000, $address->getPort());
    }

    public function testStandaloneCredentials()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            credentials: ['username' => 'user', 'password' => 'pass']
        );

        $authentication_info = $request->getAuthenticationInfo();
        $this->assertEquals('user', $authentication_info->getUsername());
        $this->assertEquals('pass', $authentication_info->getPassword());
    }

    public function testClusterCredentials()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            credentials: ['username' => 'user', 'password' => 'pass']
        );

        $authentication_info = $request->getAuthenticationInfo();
        $this->assertEquals('user', $authentication_info->getUsername());
        $this->assertEquals('pass', $authentication_info->getPassword());
    }

    public function testStandaloneReadFrom()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY);
        $this->assertEquals(\Connection_request\ReadFrom::AZAffinity, $request->getReadFrom());
    }

    public function testClusterReadFrom()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY);
        $this->assertEquals(\Connection_request\ReadFrom::AZAffinityReplicasAndPrimary, $request->getReadFrom());
    }

    public function testStandaloneRequestTimeout()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(request_timeout: 999);
        $this->assertEquals(999, $request->getRequestTimeout());
    }

    public function testClusterRequestTimeout()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(request_timeout: 999);
        $this->assertEquals(999, $request->getRequestTimeout());
    }

    public function testStandaloneReconnectStrategy()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            reconnect_strategy: ['num_of_retries' => 2, 'factor' => 3, 'exponent_base' => 7, 'jitter_percent' => 15]
        );

        $connection_retry_strategy = $request->getConnectionRetryStrategy();
        $this->assertEquals(2, $connection_retry_strategy->getNumberOfRetries());
        $this->assertEquals(3, $connection_retry_strategy->getFactor());
        $this->assertEquals(7, $connection_retry_strategy->getExponentBase());
        $this->assertEquals(15, $connection_retry_strategy->getJitterPercent());
    }

    public function testClusterReconnectStrategy()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            reconnect_strategy: ['num_of_retries' => 2, 'factor' => 3, 'exponent_base' => 7, 'jitter_percent' => 15]
        );

        $connection_retry_strategy = $request->getConnectionRetryStrategy();
        $this->assertEquals(2, $connection_retry_strategy->getNumberOfRetries());
        $this->assertEquals(3, $connection_retry_strategy->getFactor());
        $this->assertEquals(7, $connection_retry_strategy->getExponentBase());
        $this->assertEquals(15, $connection_retry_strategy->getJitterPercent());
    }

    public function testStandaloneClientName()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(client_name: 'foobar');
        $this->assertEquals('foobar', $request->getClientName());
    }

    public function testClusterClientName()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(client_name: 'foobar');
        $this->assertEquals('foobar', $request->getClientName());
    }

    public function testStandaloneClientAz()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(client_az: 'us-east-1');
        $this->assertEquals('us-east-1', $request->getClientAz());
    }

    public function testClusterClientAz()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(client_az: 'us-east-1');
        $this->assertEquals('us-east-1', $request->getClientAz());
    }

    public function testStandaloneAdvancedConfig()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(advanced_config: ['connection_timeout' => 999]);
        $this->assertEquals(999, $request->getConnectionTimeout());
    }

    public function testClusterAdvancedConfig()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(advanced_config: ['connection_timeout' => 999]);
        $this->assertEquals(999, $request->getConnectionTimeout());
    }

    public function testStandaloneLazyConnect()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(lazy_connect: true);
        $this->assertEquals(true, $request->getLazyConnect());
    }

    public function testClusterLazyConnect()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(lazy_connect: true);
        $this->assertEquals(true, $request->getLazyConnect());
    }

    public function testStandaloneDatabaseId()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(database_id: 7);
        $this->assertEquals(7, $request->getDatabaseId());
    }

    public function testClusterDatabaseId()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(database_id: 5);
        $this->assertEquals(5, $request->getDatabaseId());
    }

    public function testClusterDatabaseIdDefault()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor();
        $this->assertEquals(0, $request->getDatabaseId());
    }

    public function testStandaloneDatabaseIdNegative()
    {
        try {
            ClientConstructorMock::simulate_standalone_constructor(database_id: -1);
            $this->assertTrue(false, 'Expected ValkeyGlideException was not thrown');
        } catch (ValkeyGlideException $e) {
            $this->assertStringContains('Database ID must be non-negative', $e->getMessage());
        }
    }

    public function testClusterDatabaseIdNegative()
    {
        try {
            ClientConstructorMock::simulate_cluster_constructor(database_id: -1);
            $this->assertTrue(false, 'Expected ValkeyGlideException was not thrown');
        } catch (ValkeyGlideException $e) {
            $this->assertStringContains('Database ID must be non-negative', $e->getMessage());
        }
    }

    public function testClusterPeriodicChecksDisabled()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED
        );

        $this->assertTrue($request->hasPeriodicChecksDisabled());
        $this->assertFalse($request->hasPeriodicChecksManualInterval());
    }

    public function testClusterPeriodicChecksDefault()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS
        );

        $this->assertFalse($request->hasPeriodicChecksDisabled());
        $this->assertTrue($request->hasPeriodicChecksManualInterval());
    }

    public function testClusterRefreshTopologyFromInitialNodesDefault()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor();
        $this->assertFalse($request->getRefreshTopologyFromInitialNodes());
    }

    public function testClusterRefreshTopologyFromInitialNodesEnabled()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            advanced_config: ['refresh_topology_from_initial_nodes' => true]
        );
        $this->assertTrue($request->getRefreshTopologyFromInitialNodes());
    }

    public function testClusterRefreshTopologyFromInitialNodesDisabled()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            advanced_config: ['refresh_topology_from_initial_nodes' => false]
        );
        $this->assertFalse($request->getRefreshTopologyFromInitialNodes());
    }

    public function testStandaloneRefreshTopologyFromInitialNodesIgnored()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            advanced_config: ['refresh_topology_from_initial_nodes' => true]
        );
        $this->assertFalse($request->getRefreshTopologyFromInitialNodes());
    }

    // ================================================================
    // Tls Tests
    // ================================================================

    // TlsMode Tests
    // -------------

    public function testTlsModeDefault()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor();
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor();
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());
    }

    public function testTlsModeUseTlsEnabled()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: true);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: true);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testTlsModeUseTlsDisabled()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: false);
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: false);
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());
    }

    public function testTlsModeStreamContext()
    {
        $stream_context = stream_context_create(['ssl' => ['option' => 'value']]);

        $request = ClientConstructorMock::simulate_standalone_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testTlsModeStreamContextVerifyPeerEnabled()
    {
        $stream_context = stream_context_create(['ssl' => ['verify_peer' => true]]);

        $request = ClientConstructorMock::simulate_cluster_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testTlsModeStreamContextVerifyPeerDisabled()
    {
        $stream_context = stream_context_create(['ssl' => ['verify_peer' => false]]);

        $request = ClientConstructorMock::simulate_standalone_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_standalone_constructor(context: $stream_context);
        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());
    }

    public function testTlsModeTlsPrefix()
    {
        $host = 'localhost';
        $address = ['host' => 'tls://' . $host];

        $request = ClientConstructorMock::simulate_standalone_constructor([$address]);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
        $this->assertEquals([$host], $this->get_addresses($request));

        $request = ClientConstructorMock::simulate_cluster_constructor([$address]);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
        $this->assertEquals([$host], $this->get_addresses($request));
    }

    public function testTlsModeSslPrefix()
    {
        $host = 'localhost';
        $address = ['host' => 'ssl://' . $host];

        $request = ClientConstructorMock::simulate_standalone_constructor([$address]);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
        $this->assertEquals([$host], $this->get_addresses($request));

        $request = ClientConstructorMock::simulate_cluster_constructor([$address]);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
        $this->assertEquals([$host], $this->get_addresses($request));
    }

    public function testTlsModeAdvancedConfigUseInsecureTlsEnabled()
    {
        $advanced_config = ['tls_config' => ['use_insecure_tls' => true]];

        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: true, advanced_config: $advanced_config);
        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: true, advanced_config: $advanced_config);
        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());
    }

    public function testTlsModeAdvancedConfigUseInsecureTlsDisabled()
    {
        $advanced_config = ['tls_config' => ['use_insecure_tls' => false]];

        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: true, advanced_config: $advanced_config);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: true, advanced_config: $advanced_config);
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testTlsModeAdvancedConfigUseInsecureTlsEnabledNoTls()
    {
        $advanced_config = ['tls_config' => ['use_insecure_tls' => true]];
        $expected_msg = 'Cannot configure insecure TLS when TLS is disabled.';

        try {
            ClientConstructorMock::simulate_standalone_constructor(advanced_config: $advanced_config);
            $this->assertTrue(false, 'Expected ValkeyGlideException was not thrown');
        } catch (ValkeyGlideException $e) {
            $this->assertEquals($expected_msg, $e->getMessage());
        }

        try {
            ClientConstructorMock::simulate_cluster_constructor(advanced_config: $advanced_config);
            $this->assertTrue(false, 'Expected ValkeyGlideClusterException was not thrown');
        } catch (ValkeyGlideClusterException $e) {
            $this->assertEquals($expected_msg, $e->getMessage());
        }
    }

    // RootCerts Tests
    // ---------------

    private const CERTIFICATE_DATA = "CERTIFICATE_DATA";

    public function testRootCertsDefault()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor();
        $this->assertEmpty($request->getRootCerts());

        $request = ClientConstructorMock::simulate_cluster_constructor();
        $this->assertEmpty($request->getRootCerts());
    }

    public function testRootCertsAdvancedConfig()
    {
        $advanced_config = ['tls_config' => ['root_certs' => self::CERTIFICATE_DATA]];

        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: true, advanced_config: $advanced_config);

        $root_certs = $request->getRootCerts();
        $this->assertEquals(1, $root_certs->count());
        $this->assertEquals(self::CERTIFICATE_DATA, $root_certs[0]);

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: true, advanced_config: $advanced_config);

        $root_certs = $request->getRootCerts();
        $this->assertEquals(1, $root_certs->count());
        $this->assertEquals(self::CERTIFICATE_DATA, $root_certs[0]);
    }

    public function testRootCertsStreamContext()
    {
        $file_handle = tmpfile();
        fwrite($file_handle, self::CERTIFICATE_DATA);

        $file_path = stream_get_meta_data($file_handle)['uri'];
        $stream_context = stream_context_create(['ssl' => ['cafile' => $file_path]]);

        $request = ClientConstructorMock::simulate_standalone_constructor(use_tls: true, context: $stream_context);

        $root_certs = $request->getRootCerts();
        $this->assertEquals(1, $root_certs->count());
        $this->assertEquals(self::CERTIFICATE_DATA, $root_certs[0]);

        $request = ClientConstructorMock::simulate_cluster_constructor(use_tls: true, context: $stream_context);

        $root_certs = $request->getRootCerts();
        $this->assertEquals(1, $root_certs->count());
        $this->assertEquals(self::CERTIFICATE_DATA, $root_certs[0]);

        fclose($file_handle);
    }

    public function testRootCertStreamContextInvalidPath()
    {
        $stream_context = stream_context_create(['ssl' => ['cafile' => '/invalid/cert.pem']]);
        $expected_msg = 'Failed to load root certificate from file';

        try {
            ClientConstructorMock::simulate_standalone_constructor(use_tls: true, context: $stream_context);
            $this->assertTrue(false, 'Expected ValkeyGlideException was not thrown');
        } catch (ValkeyGlideException $e) {
            $this->assertStringContains($expected_msg, $e->getMessage());
        }

        try {
            ClientConstructorMock::simulate_cluster_constructor(use_tls: true, context: $stream_context);
            $this->assertTrue(false, 'Expected ValkeyGlideClusterException was not thrown');
        } catch (ValkeyGlideClusterException $e) {
            $this->assertStringContains($expected_msg, $e->getMessage());
        }
    }

    public function testRootCertsAdvancedConfigAndStreamContext()
    {
        $advanced_config = ['tls_config' => []];
        $stream_context = stream_context_create(['ssl' => ['option' => 'value']]);
        $expected_msg = 'At most one of stream context or advanced TLS config can be specified.';

        try {
            ClientConstructorMock::simulate_standalone_constructor(advanced_config: $advanced_config, context: $stream_context);
            $this->assertTrue(false, 'Expected ValkeyGlideException was not thrown');
        } catch (ValkeyGlideException $e) {
            $this->assertStringContains($expected_msg, $e->getMessage());
        }

        try {
            ClientConstructorMock::simulate_cluster_constructor(advanced_config: $advanced_config, context: $stream_context);
            $this->assertTrue(false, 'Expected ValkeyGlideClusterException was not thrown');
        } catch (ValkeyGlideClusterException $e) {
            $this->assertStringContains($expected_msg, $e->getMessage());
        }
    }

    // Helper methods
    // --------------

    /**
     * Returns host names from a connection request's addresses.
     *
     * @param \Connection_request\ConnectionRequest $request The connection request containing addresses.
     * @return string[] Host names extracted from the request's addresses.
     */
    private function get_addresses(\Connection_request\ConnectionRequest $request): array
    {
        return array_map(fn($a) => $a->getHost(), iterator_to_array($request->getAddresses()));
    }
}
