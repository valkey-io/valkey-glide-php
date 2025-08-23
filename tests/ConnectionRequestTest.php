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
        $request = ClientConstructorMock::simulate_standalone_constructor([['host' => 'localhost', 'port' => 8080]]);
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
        $request = ClientConstructorMock::simulate_cluster_constructor([['host' => 'localhost', 'port' => 8080]]);
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

    public function testStandaloneUseTlsOn()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: true
        );
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testClusterUseTlsOn()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: true
        );
        $this->assertEquals(\Connection_request\TlsMode::SecureTls, $request->getTlsMode());
    }

    public function testStandaloneUseTlsOff()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: false
        );
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());
    }

    public function testClusterUseTlsOff()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: false
        );
        $this->assertEquals(\Connection_request\TlsMode::NoTls, $request->getTlsMode());
    }

    public function testStandaloneCredentials()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            credentials: ['username' => 'user', 'password' => 'pass']
        );
        $this->assertEquals('user', $request->getAuthenticationInfo()->getUsername());
        $this->assertEquals('pass', $request->getAuthenticationInfo()->getPassword());
    }

    public function testClusterCredentials()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            credentials: ['username' => 'user', 'password' => 'pass']
        );
        $this->assertEquals('user', $request->getAuthenticationInfo()->getUsername());
        $this->assertEquals('pass', $request->getAuthenticationInfo()->getPassword());
    }

    public function testStandaloneReadFrom()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY
        );

        $this->assertEquals(\Connection_request\ReadFrom::AZAffinity, $request->getReadFrom());
    }

    public function testClusterReadFrom()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY
        );

        $this->assertEquals(\Connection_request\ReadFrom::AZAffinityReplicasAndPrimary, $request->getReadFrom());
    }

    public function testStandaloneRequestTimeout()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            request_timeout: 999
        );

        $this->assertEquals(999, $request->getRequestTimeout());
    }

    public function testClusterRequestTimeout()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            request_timeout: 999
        );

        $this->assertEquals(999, $request->getRequestTimeout());
    }

    public function testStandaloneReconnectStrategy()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            reconnect_strategy: ['num_of_retries' => 2, 'factor' => 3, 'exponent_base' => 7, 'jitter_percent' => 15]
        );

        $this->assertEquals(2, $request->getConnectionRetryStrategy()->getNumberOfRetries());
        $this->assertEquals(3, $request->getConnectionRetryStrategy()->getFactor());
        $this->assertEquals(7, $request->getConnectionRetryStrategy()->getExponentBase());
        $this->assertEquals(15, $request->getConnectionRetryStrategy()->getJitterPercent());
    }

    public function testClusterReconnectStrategy()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            reconnect_strategy: ['num_of_retries' => 2, 'factor' => 3, 'exponent_base' => 7, 'jitter_percent' => 15]
        );

        $this->assertEquals(2, $request->getConnectionRetryStrategy()->getNumberOfRetries());
        $this->assertEquals(3, $request->getConnectionRetryStrategy()->getFactor());
        $this->assertEquals(7, $request->getConnectionRetryStrategy()->getExponentBase());
        $this->assertEquals(15, $request->getConnectionRetryStrategy()->getJitterPercent());
    }

    public function testStandaloneClientName()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            client_name: 'foobar'
        );

        $this->assertEquals('foobar', $request->getClientName());
    }

    public function testClusterClientName()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            client_name: 'foobar'
        );

        $this->assertEquals('foobar', $request->getClientName());
    }

    public function testStandaloneClientAz()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            client_az: 'us-east-1'
        );

        $this->assertEquals('us-east-1', $request->getClientAz());
    }

    public function testClusterClientAz()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            client_az: 'us-east-1'
        );

        $this->assertEquals('us-east-1', $request->getClientAz());
    }

    public function testStandaloneAdvancedConfig()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            advanced_config: ['connection_timeout' => 999]
        );

        $this->assertEquals(999, $request->getConnectionTimeout());
    }

    public function testClusterAdvancedConfig()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            advanced_config: ['connection_timeout' => 999]
        );

        $this->assertEquals(999, $request->getConnectionTimeout());
    }

    public function testStandaloneInsecureTls()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: true,
            advanced_config: ['tls_config' => ['use_insecure_tls' => true]]
        );

        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());
    }

    public function testClusterInsecureTls()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            use_tls: true,
            advanced_config: ['tls_config' => ['use_insecure_tls' => true]]
        );

        $this->assertEquals(\Connection_request\TlsMode::InsecureTls, $request->getTlsMode());
    }

    public function testStandaloneLazyConnect()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            lazy_connect: true
        );

        $this->assertEquals(true, $request->getLazyConnect());
    }

    public function testClusterLazyConnect()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            lazy_connect: true
        );

        $this->assertEquals(true, $request->getLazyConnect());
    }

    public function testStandaloneDatabaseId()
    {
        $request = ClientConstructorMock::simulate_standalone_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            database_id: 7
        );

        $this->assertEquals(7, $request->getDatabaseId());
    }

    public function testClusterPeriodicChecksDisabled()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED
        );

        $this->assertTrue($request->hasPeriodicChecksDisabled());
        $this->assertFalse($request->hasPeriodicChecksManualInterval());
    }

    public function testClusterPeriodicChecksDefault()
    {
        $request = ClientConstructorMock::simulate_cluster_constructor(
            addresses: [['host' => 'localhost', 'port' => 8080]],
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS
        );

        $this->assertFalse($request->hasPeriodicChecksDisabled());
        $this->assertTrue($request->hasPeriodicChecksManualInterval());
    }
}
