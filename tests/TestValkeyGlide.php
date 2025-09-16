<?php

define('VALKEY_GLIDE_PHP_TESTRUN', true);
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
require_once __DIR__ . "/ConnectionRequestTest.php";
require_once __DIR__ . "/ValkeyGlideBaseTest.php";
require_once __DIR__ . "/ValkeyGlideClusterBaseTest.php";
require_once __DIR__ . "/ValkeyGlideTest.php";
require_once __DIR__ . "/ValkeyGlideClusterTest.php";
require_once __DIR__ . "/ValkeyGlideFeaturesTest.php";
require_once __DIR__ . "/ValkeyGlideClusterFeaturesTest.php";
require_once __DIR__ . "/ValkeyGlideBatchTest.php";
require_once __DIR__ . "/ValkeyGlideClusterBatchTest.php";
echo "Loading ValkeyGlide tests...\n";
function getClassArray($classes)
{
    $result = [];

    if (! is_array($classes)) {
        $classes = [$classes];
    }

    foreach ($classes as $class) {
        $result = array_merge($result, explode(',', $class));
    }

    return array_unique(
        array_map(function ($v) {
            return strtolower($v);
        },
            $result)
    );
}

function getTestClass($class)
{
    $valid_classes = [
        'connectionrequest' => 'ConnectionRequestTest',
        'valkeyglide'         => 'ValkeyGlideTest',
        'valkeyglidecluster'  => 'ValkeyGlideClusterTest',
        'valkeyglideclientfeatures' => 'ValkeyGlideFeaturesTest',
        'valkeyglideclusterfeatures' => 'ValkeyGlideClusterFeaturesTest',
        'valkeyglideclientbatch' => 'ValkeyGlideBatchTest',
        'valkeyglideclusterbatch' => 'ValkeyGlideClusterBatchTest'
    ];

    /* Return early if the class is one of our built-in ones */
    if (isset($valid_classes[$class])) {
        return $valid_classes[$class];
    }

    /* Try to load it */
    return TestSuite::loadTestClass($class);
}

function raHosts($host, $ports)
{
    if (! is_array($ports)) {
        $ports = [6379, 6380, 6381, 6382];
    }

    return array_map(function ($port) use ($host) {
        return sprintf("%s:%d", $host, $port);
    }, $ports);
}
echo "Running ValkeyGlide tests...\n";
/* Make sure errors go to stdout and are shown */
error_reporting(E_ALL);
ini_set('display_errors', '1');

/* Grab options */
$opt = getopt('', ['host:', 'port:', 'class:', 'test:', 'nocolors', 'user:', 'auth:', 'tls']);

/* The test class(es) we want to run */
$classes = getClassArray($opt['class'] ?? 'connectionrequest,valkeyglide,valkeyglidecluster,valkeyglideclientfeatures,valkeyglideclusterfeatures,valkeyglideclientbatch,valkeyglideclusterbatch');

$colorize = !isset($opt['nocolors']);

/* Get our test filter if provided one */
$filter = $opt['test'] ?? null;

/* Grab override host/port if it was passed */
$host = $opt['host'] ?? '127.0.0.1';
$port = $opt['port'] ?? 6379;

/* Get optional username and auth (password) */
$user = $opt['user'] ?? null;
$auth = $opt['auth'] ?? null;

/* Check if TLS should be enabled. */
$tls = isset($opt['tls']);
if (isset($opt['tls'])) {
    echo TestSuite::makeBold("Assuming TLS connection for client constructor feature tests.\n");
}

if ($user && $auth) {
    $auth = [$user, $auth];
} elseif ($user && ! $auth) {
    echo TestSuite::makeWarning("User passed without a password!\n");
}

/* Toggle colorization in our TestSuite class */
TestSuite::flagColorization($colorize);

/* Let the user know this can take a bit of time */
echo "Note: these tests might take up to a minute. Don't worry :-)\n";
echo "Using PHP version " . PHP_VERSION . " (" . (PHP_INT_SIZE * 8) . " bits)\n";

foreach ($classes as $class) {
    $class = getTestClass($class);

    /* Depending on the classes being tested, run our tests on it */
    echo "Testing class ";

    echo TestSuite::makeBold($class) . "\n";

    if (TestSuite::run("$class", $filter, $host, $port, $auth, $tls)) {
        exit(1);
    }
}

/* Success */
exit(0);
