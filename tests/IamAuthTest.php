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

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * IAM Authentication Tests for ValkeyGlide PHP
 *
 * These tests verify IAM authentication functionality using mock AWS credentials.
 * They test both basic IAM authentication automatic token refresh at configured
 * intervals.
 *
 * Based on IAM_AUTH_TEST_PORTING_GUIDE.md
 *
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */
class IamAuthTest extends ValkeyGlideBaseTest
{
    private $originalEnvVars = [];

    public function setUp()
    {
        // Save original AWS environment variables
        $this->originalEnvVars = [
            'AWS_ACCESS_KEY_ID' => getenv('AWS_ACCESS_KEY_ID'),
            'AWS_SECRET_ACCESS_KEY' => getenv('AWS_SECRET_ACCESS_KEY'),
            'AWS_SESSION_TOKEN' => getenv('AWS_SESSION_TOKEN')
        ];
    }

    private function restoreEnvironmentVariables()
    {
        foreach ($this->originalEnvVars as $key => $value) {
            if ($value !== false) {
                putenv("$key=$value");
            } else {
                // Unset the environment variable if it didn't exist originally
                putenv($key);
            }
        }
    }

    /**
     * Set mock AWS credentials in the environment.
     */
    private function setMockAwsCredentials()
    {
        putenv('AWS_ACCESS_KEY_ID=test_access_key');
        putenv('AWS_SECRET_ACCESS_KEY=test_secret_key');
        putenv('AWS_SESSION_TOKEN=test_session_token');
    }

    /**
     * Create a ValkeyGlide client configured with IAM authentication.
     *
     * @param int $refreshIntervalSeconds IAM token refresh interval in seconds
     * @param int|null $connectionTimeout Connection timeout in milliseconds (optional)
     * @return ValkeyGlide
     */
    private function createIamClient(int $refreshIntervalSeconds, ?int $connectionTimeout = null): ValkeyGlide
    {
        $client = new ValkeyGlide();

        $connectArgs = [
            'addresses' => [
                ['host' => $this->getHost(), 'port' => $this->getPort()]
            ],
            'use_tls' => false,
            'credentials' => [
                'username' => 'default',
                'iamConfig' => [
                    'clusterName' => 'test-cluster',
                    'region' => 'us-east-1',
                    'service' => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
                    'refreshIntervalSeconds' => $refreshIntervalSeconds
                ]
            ]
        ];

        if ($connectionTimeout !== null) {
            $connectArgs['advanced_config'] = ['connection_timeout' => $connectionTimeout];
        }

        $client->connect(...$connectArgs);

        return $client;
    }

    /**
     * Test 1: Basic IAM Authentication with Mock Credentials
     *
     * Purpose: Verify client can connect and operate using IAM authentication
     * with mock AWS credentials.
     *
     * Test Steps:
     * 1. Set mock AWS credentials
     * 2. Create IAM configuration with 5 second refresh interval
     * 3. Create client with IAM authentication (use_tls=false for local testing)
     * 4. Verify connection with PING command
     * 5. Test basic operations (SET/GET)
     * 6. Verify operations still work after token refresh
     */
    public function testIamAuthenticationWithMockCredentials()
    {
        try {
            $this->setMockAwsCredentials();

            $client = $this->createIamClient(
                refreshIntervalSeconds: 5,
                connectionTimeout: 5000
            );

            // Verify connection with PING
            $this->assertConnected($client);

            // Test basic operations (SET/GET)
            $client->set('iam_test_key', 'iam_test_value');
            $value = $client->get('iam_test_key');
            $this->assertEquals('iam_test_value', $value);

            // Manually refresh IAM token
            $client->refreshIamToken();

            // Verify operations still work after token refresh
            $this->assertConnected($client);
            $client->set('iam_test_key2', 'iam_test_value2');
            $value2 = $client->get('iam_test_key2');
            $this->assertEquals('iam_test_value2', $value2);

            // Cleanup
            $client->del('iam_test_key', 'iam_test_key2');
            $client->close();
        } finally {
            $this->restoreEnvironmentVariables();
        }
    }

    /**
     * Test 2: Automatic IAM Token Refresh
     *
     * Purpose: Verify client automatically refreshes IAM tokens at configured intervals.
     *
     * Test Steps:
     * 1. Set mock AWS credentials
     * 2. Create IAM configuration with very short refresh interval (2 seconds)
     * 3. Create client with IAM authentication
     * 4. Verify initial connection with PING
     * 5. Wait for automatic token refresh (sleep 3 seconds)
     * 6. Verify client still works after automatic refresh
     */
    public function testAutomaticIamTokenRefresh()
    {
        try {
            $this->setMockAwsCredentials();

            $client = $this->createIamClient(refreshIntervalSeconds: 2);

            // Verify initial connection with PING
            $this->assertConnected($client);

            // Test initial operation
            $client->set('auto_refresh_test', 'initial_value');
            $value = $client->get('auto_refresh_test');
            $this->assertEquals('initial_value', $value);

            // Wait for automatic token refresh
            sleep(3);

            // Verify client still works after automatic refresh
            $this->assertConnected($client);

            $client->set('auto_refresh_test', 'refreshed_value');
            $value = $client->get('auto_refresh_test');
            $this->assertEquals('refreshed_value', $value);

            // Cleanup
            $client->del('auto_refresh_test');
            $client->close();
        } finally {
            $this->restoreEnvironmentVariables();
        }
    }
}
