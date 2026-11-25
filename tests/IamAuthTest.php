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

    public function setUp(): void
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

        // IAM auth requires TLS (an IAM token must never traverse plaintext), so
        // connect over the TLS-enabled server with the test CA cert.
        $connectArgs = [
            'addresses' => [self::TLS_ADDRESS_STANDALONE],
            'use_tls' => true,
            'advanced_config' => [
                'tls_config' => ['root_certs' => file_get_contents(self::TLS_CERTIFICATE_PATH)]
            ],
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
            $connectArgs['advanced_config']['connection_timeout'] = $connectionTimeout;
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
        $this->skipIfTlsDisabled();

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
        $this->skipIfTlsDisabled();

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

    /**
     * Test 3: IAM Authentication Requires TLS
     *
     * Purpose: Verify that configuring IAM authentication without TLS is
     * rejected at client creation, so an IAM token is never sent over a
     * plaintext connection.
     *
     * Needs no TLS server: the guard fails closed before any connection, so
     * this runs in plain CI without skipIfTlsDisabled().
     */
    public function testIamAuthenticationRequiresTls()
    {
        try {
            $this->setMockAwsCredentials();

            $client = new ValkeyGlide();

            try {
                $client->connect(
                    addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
                    use_tls: false,
                    credentials: [
                        'username' => 'default',
                        'iamConfig' => [
                            'clusterName' => 'test-cluster',
                            'region' => 'us-east-1',
                            'service' => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
                            'refreshIntervalSeconds' => 5
                        ]
                    ]
                );
                $this->assertTrue(false, 'Expected IAM-without-TLS to be rejected at client creation');
            } catch (ValkeyGlideException $e) {
                $this->assertStringContains('IAM authentication requires TLS', $e->getMessage());
            }
        } finally {
            $this->restoreEnvironmentVariables();
        }
    }

    /**
     * Credentials a custom credential provider returns in these tests. The local
     * server accepts any token for the default user, so the tests check how the
     * provider is called, not the signature.
     */
    private const PROVIDER_CREDENTIALS = [
        'accessKeyId' => 'provider_access_key',
        'secretAccessKey' => 'provider_secret_key',
        'sessionToken' => 'provider_session_token',
    ];

    /**
     * Connect arguments for a TLS client using $provider as its credential provider.
     */
    private function credentialProviderArgs(mixed $provider, int $refreshIntervalSeconds = 300, bool $cluster = false): array
    {
        return [
            'addresses' => [$cluster ? self::TLS_ADDRESS_CLUSTER : self::TLS_ADDRESS_STANDALONE],
            'use_tls' => true,
            'advanced_config' => [
                'tls_config' => ['root_certs' => file_get_contents(self::TLS_CERTIFICATE_PATH)]
            ],
            'credentials' => [
                'username' => 'default',
                'iamConfig' => [
                    ValkeyGlide::IAM_CONFIG_CLUSTER_NAME => 'test-cluster',
                    ValkeyGlide::IAM_CONFIG_REGION => 'us-east-1',
                    ValkeyGlide::IAM_CONFIG_SERVICE => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
                    ValkeyGlide::IAM_CONFIG_REFRESH_INTERVAL => $refreshIntervalSeconds,
                    ValkeyGlide::IAM_CONFIG_CREDENTIAL_PROVIDER => $provider,
                ]
            ]
        ];
    }

    /**
     * Connect a standalone client with $provider. Without TLS the provider is still
     * called and validated first, so rejection tests run in plain CI.
     */
    private function connectWithProvider(mixed $provider, int $refreshIntervalSeconds = 300): ValkeyGlide
    {
        $client = new ValkeyGlide();
        $client->connect(...$this->credentialProviderArgs($provider, $refreshIntervalSeconds));
        return $client;
    }

    public function testCredentialProviderUsedOnConnect()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        });

        $this->assertEquals(1, $calls);
        $this->assertConnected($client);
        $client->set('iam_provider_key', 'value');
        $this->assertEquals('value', $client->get('iam_provider_key'));
        $client->del('iam_provider_key');
        $client->close();
    }

    public function testCredentialProviderOptionalFields()
    {
        $this->skipIfTlsDisabled();

        // No session token, and an expiry far enough away not to force a refresh
        $client = $this->connectWithProvider(fn () => [
            'accessKeyId' => 'provider_access_key',
            'secretAccessKey' => 'provider_secret_key',
            'sessionToken' => null,
            'expiresAt' => (time() + 3600) * 1000,
        ]);
        $this->assertConnected($client);
        $client->close();
    }

    public function testCredentialProviderLargeSessionToken()
    {
        $this->skipIfTlsDisabled();

        // Larger than the core's first 2048-byte buffer: the core asks again with a larger one
        $credentials = self::PROVIDER_CREDENTIALS;
        $credentials['sessionToken'] = str_repeat('t', 5000);
        $client = $this->connectWithProvider(fn () => $credentials);
        $this->assertConnected($client);
        $client->close();
    }

    public function testRefreshIamTokenCallsCredentialProvider()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        });

        $client->refreshIamToken();
        $this->assertEquals(2, $calls);
        $this->assertConnected($client);
        $client->close();
    }

    public function testRefreshIamTokenCredentialProviderFailureThrows()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            if (++$calls > 1) {
                throw new RuntimeException('vault unavailable');
            }
            return self::PROVIDER_CREDENTIALS;
        });

        try {
            $client->refreshIamToken();
            $this->fail('Expected refreshIamToken() to throw');
        } catch (ValkeyGlideException $exception) {
            $this->assertStringContains('credentialProvider threw an exception', $exception->getMessage());
            $this->assertTrue($exception->getPrevious() instanceof RuntimeException);
        }
        // The client keeps working with the token it has
        $this->assertConnected($client);
        $client->close();
    }

    public function testCredentialProviderRefreshedBeforeCommand()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        }, refreshIntervalSeconds: 2);
        $this->assertEquals(1, $calls);

        // Due after half the refresh interval: the next command calls the provider first
        usleep(1200000);
        $this->assertConnected($client);
        $this->assertEquals(2, $calls);

        // Not due again right away
        $this->assertConnected($client);
        $this->assertEquals(2, $calls);
        $client->close();
    }

    public function testCredentialProviderNotCalledWhileIdle()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        }, refreshIntervalSeconds: 1);

        // The core refreshes the token every second on its own threads; PHP code never runs there
        sleep(3);
        $this->assertEquals(1, $calls);
        $this->assertConnected($client);
        $client->close();
    }

    public function testCredentialProviderFailureAfterConnectKeepsClient()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            if (++$calls > 1) {
                throw new RuntimeException('vault unavailable');
            }
            return self::PROVIDER_CREDENTIALS;
        }, refreshIntervalSeconds: 2);

        // The refresh before the command fails: the command still runs, nothing is thrown
        usleep(1200000);
        $this->assertConnected($client);
        $this->assertEquals(2, $calls);
        $client->close();
    }

    public function testCredentialProviderClosedClient()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = $this->connectWithProvider(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        }, refreshIntervalSeconds: 1);
        $client->close();

        // Refresh ticks that were already scheduled find no credentials and do not call PHP
        sleep(2);
        $this->assertEquals(1, $calls);
    }

    public function testClusterCredentialProvider()
    {
        $this->skipIfTlsDisabled();

        $calls = 0;
        $client = new ValkeyGlideCluster(...$this->credentialProviderArgs(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        }, cluster: true));
        $this->assertEquals(1, $calls);
        $this->assertConnected($client);

        $client->refreshIamToken();
        $this->assertEquals(2, $calls);
        $this->assertConnected($client);

        // OPT_SLAVE_FAILOVER recreates the core client, which must keep the provider
        $this->assertTrue($client->setOption(ValkeyGlideCluster::OPT_SLAVE_FAILOVER, ValkeyGlideCluster::FAILOVER_DISTRIBUTE));
        $this->assertConnected($client);
        $client->refreshIamToken();
        $this->assertEquals(3, $calls);
        $client->close();
    }

    /**
     * Invalid providers and results are rejected at connect, before any connection,
     * with messages that do not contain the credentials.
     */
    public function testCredentialProviderRejectsInvalidResults()
    {
        $valid = self::PROVIDER_CREDENTIALS;
        $cases = [
            'must return an array' => fn () => 'not an array',
            'accessKeyId must be a non-blank string' => fn () => ['secretAccessKey' => 'provider_secret_key'],
            'accessKeyId must be a non-blank string ' => fn () => ['accessKeyId' => '   '] + $valid,
            'accessKeyId must be a non-blank string  ' => fn () => ['accessKeyId' => 123] + $valid,
            'secretAccessKey must be a non-blank string' => fn () => ['secretAccessKey' => ''] + $valid,
            'sessionToken must be a string or null' => fn () => ['sessionToken' => 5] + $valid,
            'expiresAt must be an integer or null' => fn () => ['expiresAt' => '123'] + $valid,
            'have already expired' => fn () => ['expiresAt' => (time() - 60) * 1000] + $valid,
            'exceed 1 MiB' => fn () => ['sessionToken' => str_repeat('t', 1024 * 1024 + 1)] + $valid,
            'exceed 1 MiB ' => fn () => [
                'accessKeyId' => str_repeat('a', 400000),
                'secretAccessKey' => str_repeat('s', 400000),
                'sessionToken' => str_repeat('t', 400000),
            ],
        ];

        foreach ($cases as $expected => $provider) {
            try {
                $this->connectWithProvider($provider);
                $this->fail("Expected rejection: $expected");
            } catch (ValkeyGlideException $exception) {
                $this->assertStringContains(trim($expected), $exception->getMessage());
                $this->assertFalse(str_contains($exception->getMessage(), 'provider_secret_key'));
            }
        }
    }

    public function testCredentialProviderThrowingAtConnect()
    {
        try {
            $this->connectWithProvider(function () {
                throw new RuntimeException('vault unavailable');
            });
            $this->fail('Expected connect() to throw');
        } catch (ValkeyGlideException $exception) {
            $this->assertStringContains('credentialProvider threw an exception', $exception->getMessage());
            $previous = $exception->getPrevious();
            $this->assertTrue($previous instanceof RuntimeException);
            $this->assertEquals('vault unavailable', $previous?->getMessage());
        }
    }

    public function testCredentialProviderMustBeCallable()
    {
        $this->assertThrowsMatch(
            42,
            fn ($provider) => $this->connectWithProvider($provider),
            '/credentialProvider must be callable/'
        );
        $this->assertThrowsMatch(
            'no_such_function_for_credentials',
            fn ($provider) => $this->connectWithProvider($provider),
            '/credentialProvider must be callable/'
        );
    }

    public function testCredentialProviderMustBeInsideIamConfig()
    {
        $client = new ValkeyGlide();
        $this->assertThrowsMatch(
            null,
            fn () => $client->connect(
                addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
                credentials: [
                    'username' => 'default',
                    ValkeyGlide::IAM_CONFIG_CREDENTIAL_PROVIDER => fn () => self::PROVIDER_CREDENTIALS,
                ]
            ),
            '/credentialProvider must be set inside iamConfig/'
        );
    }

    public function testCredentialProviderWithAddressResolverRejected()
    {
        $calls = 0;
        $args = $this->credentialProviderArgs(function () use (&$calls) {
            $calls++;
            return self::PROVIDER_CREDENTIALS;
        });
        $args['address_resolver'] = fn ($host, $port) => ['host' => $host, 'port' => $port];

        $client = new ValkeyGlide();
        $this->assertThrowsMatch(
            null,
            fn () => $client->connect(...$args),
            '/cannot be combined with address_resolver/'
        );
        $this->assertEquals(0, $calls);
    }
}
