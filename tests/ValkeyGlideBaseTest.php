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

/**
 * ValkeyGlide Base Test Class
 * Abstract base class providing infrastructure methods for ValkeyGlide tests
 * Contains no actual test methods - only setup and helper functionality
 */
abstract class ValkeyGlideBaseTest extends TestSuite
{
    private static $debug_logged = false;
    private static $version_logged = false;
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    /**
     * @var ValkeyGlide
     */
    public $valkey_glide;

    /* City lat/long */
    protected $cities = [
        'Chico'         => [-121.837478, 39.728494],
        'Sacramento'    => [-121.494400, 38.581572],
        'Gridley'       => [-121.693583, 39.363777],
        'Marysville'    => [-121.591355, 39.145725],
        'Cupertino'     => [-122.032182, 37.322998]
    ];

    protected const TLS_ADDRESS_STANDALONE = ['host' => 'localhost', 'port' => 6400];
    protected const TLS_ADDRESS_CLUSTER    = ['host' => 'localhost', 'port' => 8001];
    protected const TLS_CERTIFICATE_PATH   = __DIR__ . '/../valkey-glide/utils/tls_crts/ca.crt';

    protected function getNilValue()
    {
        return false;
    }

    /* Overridable left/right constants */
    protected function getLeftConstant()
    {
        return ValkeyGlide::LEFT;
    }

    protected function getRightConstant()
    {
        return ValkeyGlide::RIGHT;
    }

    protected function detectValkey(array $info)
    {
        return isset($info['server_name']) && $info['server_name'] === 'valkey';
    }

    protected function compare_major_version_number($minMajorVersion)
    {
        $currentMajor = intval(explode('.', $this->version)[0]);
        return $currentMajor >= $minMajorVersion;
    }

    public function setUp()
    {
        $this->valkey_glide = $this->newInstance();
        $info = $this->valkey_glide->info();

        // Handle case where info() returns false (connection failed)
        if ($info === false) {
            throw new Exception("Failed to connect to Valkey/Redis server. Please ensure server is running and accessible.");
        }

        $this->version  = $info['valkey_version'] ?? $info['redis_version'] ?? '0.0.0';
        $this->is_valkey = $this->detectValkey($info);

        // Debug: Show what keys are available in INFO response (only once per test suite)
        if (!self::$debug_logged) {
            echo "DEBUG: Available INFO keys: " . implode(', ', array_keys($info)) . "\n";
            if (isset($info['valkey_version'])) {
                echo "DEBUG: valkey_version found: " . $info['valkey_version'] . "\n";
            }
            if (isset($info['redis_version'])) {
                echo "DEBUG: redis_version found: " . $info['redis_version'] . "\n";
            }
            self::$debug_logged = true;
        }

        // Log server type and version for debugging (only once per test suite)
        if (!self::$version_logged) {
            $server_type = $this->is_valkey ? 'Valkey' : 'Redis';
            echo "Connected to $server_type server version: {$this->version}\n";
            self::$version_logged = true;
        }
    }

    protected function minVersionCheck($version)
    {
        return version_compare($this->version, $version) >= 0;
    }

    protected function mstime()
    {
        return round(microtime(true) * 1000);
    }

    protected function getAuthParts(&$user, &$pass)
    {
        $user = $pass = null;

        $auth = $this->getAuth();
        if (! $auth) {
            return;
        }

        if (is_array($auth)) {
            if (count($auth) > 1) {
                list($user, $pass) = $auth;
            } else {
                $pass = $auth[0];
            }
        } else {
            $pass = $auth;
        }
    }

    protected function getAuthFragment()
    {
        $this->getAuthParts($user, $pass);

        if ($user && $pass) {
            return sprintf('auth[user]=%s&auth[pass]=%s', $user, $pass);
        } elseif ($pass) {
            return sprintf('auth[pass]=%s', $pass);
        } else {
            return '';
        }
    }

    protected function newInstance()
    {
        $args = ['addresses' => [[
            'host' => $this->getHost(),
            'port' => $this->getPort()
        ]]];

        // Add TLS arguments
        if ($this->getTLS()) {
            $args['use_tls'] = true;
            $args['advanced_config'] = ['tls_config' => ['use_insecure_tls' => true]];
        }

        $r = new ValkeyGlide(...$args);

        if ($this->getAuth()) {
            $this->assertTrue($r->auth($this->getAuth()));
        }

        return $r;
    }

    public function tearDown()
    {
        if ($this->valkey_glide) {
            $this->valkey_glide->close();
        }
    }

    public function reset()
    {
        $this->setUp();
        $this->tearDown();
    }

    /* Helper function to determine if the class has pipeline support */
    protected function havePipeline()
    {
        return defined(get_class($this->valkey_glide) . '::PIPELINE');
    }

    protected function haveMulti()
    {
        return defined(get_class($this->valkey_glide) . '::MULTI');
    }

    protected function assertConnected(ValkeyGlide|ValkeyGlideCluster $client)
    {
        $result =
            $client instanceof ValkeyGlideCluster
            ? $client->ping('allPrimaries')
            : $client->ping();

        $this->assertTrue($result, "Client should be connected");
    }
}
