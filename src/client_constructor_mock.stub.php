<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

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

class ClientConstructorMock
{
    /**
     * Mock creation of a standalone connection request instance with the provided configuration.
     *
     * @param array|null $addresses              Array of server addresses [['host' => 'localhost', 'port' => 6379], ...].
     * @param bool $use_tls                      Whether to use TLS encryption.
     * @param array|null $credentials            Authentication credentials ['password' => 'xxx', 'username' => 'yyy'].
     * @param int $read_from                     Read strategy for the client.
     * @param int|null $request_timeout          Request timeout in milliseconds.
     * @param array|null $reconnect_strategy     Reconnection strategy ['num_of_retries' => 3, 'factor' => 2,
     *                                           'exponent_base' => 10, 'jitter_percent' => 15].
     * @param int|null $database_id              Database ID to select (0 or higher)
     * @param string|null $client_name           Client name identifier.
     * @param string|null $client_az             Client availability zone.
     * @param array|null $advanced_config        Advanced configuration ['connection_timeout' => 5000,
     *                                           'tls_config' => ['use_insecure_tls' => false]].
     *                                           connection_timeout is in milliseconds.
     * @param bool|null $lazy_connect            Whether to use lazy connection.
     * @param resource|null $context             Stream context for the connection.
     */
    public static function simulate_standalone_constructor(
        ?array $addresses = null,
        bool $use_tls = false,
        ?array $credentials = null,
        int $read_from = ValkeyGlide::READ_FROM_PRIMARY,
        ?int $request_timeout = null,
        ?array $reconnect_strategy = null,
        ?int $database_id = null,
        ?string $client_name = null,
        ?string $client_az = null,
        ?array $advanced_config = null,
        ?bool $lazy_connect = null,
        mixed $context = null,
    ): \Connection_request\ConnectionRequest;

    /**
     * Mock creation of a cluster connection request instance with the provided configuration.
     *
     * @param array|null $addresses                   Array of server addresses [['host' => '127.0.0.1', 'port' => 7001], ...].
     * @param bool $use_tls                           Whether to use TLS encryption.
     * @param array|null $credentials                 Authentication credentials ['password' => 'xxx', 'username' => 'yyy'].
     * @param int $read_from                          Read strategy for the client.
     * @param int|null $request_timeout               Request timeout in milliseconds.
     * @param array|null $reconnect_strategy          Reconnection strategy ['num_of_retries' => 3, 'factor' => 2,
     *                                                'exponent_base' => 10, 'jitter_percent' => 15].
     * @param string|null $client_name                Client name identifier.
     * @param int|null $periodic_checks               Periodic checks configuration.
     * @param string|null $client_az                  Client availability zone.
     * @param array|null $advanced_config             Advanced configuration ['connection_timeout' => 5000,
     *                                                'tls_config' => ['use_insecure_tls' => false]].
     *                                                connection_timeout is in milliseconds.
     * @param bool|null $lazy_connect                 Whether to use lazy connection.
     * @param int|null $database_id                   Index of the logical database to connect to. Must be non-negative
     *                                                and within the range supported by the server configuration.
     *                                                For cluster mode, requires Valkey 9.0+ with cluster-databases > 1.
     *                                                If not specified, defaults to database 0.
     * @param resource|null $context                     Stream context for the connection.
     */
    public static function simulate_cluster_constructor(
        ?array $addresses = null,
        bool $use_tls = false,
        ?array $credentials = null,
        int $read_from = ValkeyGlide::READ_FROM_PREFER_REPLICA,
        ?int $request_timeout = null,
        ?array $reconnect_strategy = null,
        ?string $client_name = null,
        ?int $periodic_checks = ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
        ?string $client_az = null,
        ?array $advanced_config = null,
        ?bool $lazy_connect = null,
        ?int $database_id = null,
        mixed $context = null,
    ): \Connection_request\ConnectionRequest;
}
