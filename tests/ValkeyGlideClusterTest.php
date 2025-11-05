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

require_once __DIR__ . "/ValkeyGlideTest.php";

/**
 * Most ValkeyGlideCluster tests should work the same as the standard ValkeyGlide object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class ValkeyGlideClusterTest extends ValkeyGlideTest
{
    private $valkey_glide_types = [
        ValkeyGlide::VALKEY_GLIDE_STRING,
        ValkeyGlide::VALKEY_GLIDE_SET,
        ValkeyGlide::VALKEY_GLIDE_LIST,
        ValkeyGlide::VALKEY_GLIDE_ZSET,
        ValkeyGlide::VALKEY_GLIDE_HASH
    ];


    private static $debug_logged = false;
    private static $version_logged = false;

    protected static array $seeds = [];
    private static string $seed_source = '';

    /* Tests we'll skip all together in the context of ValkeyGlideCluster.  The
     * ValkeyGlideCluster class doesn't implement specialized (non-redis) commands
     * such as sortAsc, or sortDesc and other commands such as SELECT are
     * simply invalid in ValkeyGlide Cluster */
    public function testPipelinePublish()
    {
        $this->markTestSkipped();
    }
    public function testSortAsc()
    {
        $this->markTestSkipped();
    }
    public function testSortDesc()
    {
        $this->markTestSkipped();
    }
    public function testWait()
    {
        $this->markTestSkipped();
    }

    public function testSelect()
    {
        $this->assertFalse(@$this->valkey_glide->select(-1));
        $this->assertTrue($this->valkey_glide->select(0));
    }

    public function testMove()
    {
        // Basic MOVE method availability test
        $key = '{key}test_move_' . uniqid();
        $this->valkey_glide->set($key, 'test_value');
        
        // MOVE should return boolean (may be false if multi-database not supported)
        $result = $this->valkey_glide->move($key, 1);
        $this->assertIsBool($result);
        
        // Clean up
        $this->valkey_glide->del($key);
    }

    public function testReconnectSelect()
    {
        $this->markTestSkipped();
    }

    /* These 'directed node' commands work differently in ValkeyGlideCluster */
    public function testConfig()
    {
        $this->markTestSkipped();
    }
    public function testFlushDB()
    {
        $key = "key:0";
        $this->assertTrue($this->valkey_glide->flushdb($key));
        $this->assertTrue($this->valkey_glide->flushdb($key, null));
        $this->assertTrue($this->valkey_glide->flushdb($key, false));
        $this->assertTrue($this->valkey_glide->flushdb($key, true));
    }

    public function testFunction()
    {
        $this->markTestSkipped();
    }







    /* Load our seeds on construction */
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    /* Override setUp to get info from a specific node */
    public function setUp()
    {
        $this->valkey_glide    = $this->newInstance();
        $info           = $this->valkey_glide->info("randomNode");
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

    /* Override newInstance as we want a ValkeyGlideCluster object */
    protected function newInstance()
    {
        try {
            return new ValkeyGlideCluster(
                [['host' => '127.0.0.1', 'port' => 7001]], // addresses array format
                false, // use_tls
                $this->getAuth(), // credentials
                ValkeyGlide::READ_FROM_PRIMARY, // read_from
                null, // request_timeout
                null, // reconnect_strategy
                null, // client_name
                null, // periodic_checks
                null, // client_az
                null, // advanced_config
                null, // lazy_connect
                0     // database_id - enable multi-database support
            );
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error: %s\n", $ex->getMessage());
            //TestSuite::errorMessage("Seeds: %s\n", implode(' ', self::$seeds));
            TestSuite::errorMessage("Seed source: %s\n", self::$seed_source);
            exit(1);
        }
    }

    /* Overrides for ValkeyGlideTest where the function signature is different.  This
     * is only true for a few commands, which by definition have to be directed
     * at a specific node */

    public function testPing()
    {
        for ($i = 0; $i < 20; $i++) {
            $this->assertTrue($this->valkey_glide->ping(['type' => 'primarySlotKey', 'key' => "key:$i"]));
            $this->assertEquals('BEEP', $this->valkey_glide->ping(['type' => 'primarySlotKey', 'key' => "key:$i"], 'BEEP'));
        }
    }

    public function testRandomKey()
    {
        /* Ensure some keys are present to test */
        for ($i = 0; $i < 1000; $i++) {
            if (rand(1, 2) == 1) {
                $this->valkey_glide->set("key:$i", "val:$i");
            }
        }

        for ($i = 0; $i < 1000; $i++) {
            $k = $this->valkey_glide->randomKey("key:$i");
            $this->assertEquals(1, $this->valkey_glide->exists($k));
        }
    }

    public function testEcho()
    {
        $this->assertEquals('hello', $this->valkey_glide->echo('echo1', 'hello'));
        $this->assertEquals('world', $this->valkey_glide->echo('echo2', 'world'));
        $this->assertEquals(' 0123 ', $this->valkey_glide->echo('echo3', " 0123 "));
    }

    public function testSortPrefix()
    {
        $this->valkey_glide->del('some-prefix:some-item');
        $this->valkey_glide->sadd('some-prefix:some-item', 1);
        $this->valkey_glide->sadd('some-prefix:some-item', 2);
        $this->valkey_glide->sadd('some-prefix:some-item', 3);
        $this->assertEquals(['1', '2', '3'], $this->valkey_glide->sort('some-prefix:some-item'));

        // Kill our set/prefix
        $this->valkey_glide->del('some-prefix:some-item');
    }

    public function testDBSize()
    {
        for ($i = 0; $i < 10; $i++) {
            $key = "key:$i";
            $this->assertTrue($this->valkey_glide->flushdb($key));
            $this->valkey_glide->set($key, "val:$i");
            $this->assertEquals(1, $this->valkey_glide->dbsize($key));
        }
    }

    public function testFlushAll()
    {

        for ($i = 0; $i < 10; $i++) {
            $key = "key:$i";
            $this->assertTrue($this->valkey_glide->flushAll($key, true));
            $this->assertTrue($this->valkey_glide->flushAll($key));
            $this->assertEquals(0, $this->valkey_glide->dbsize($key));
            $this->valkey_glide->set($key, "val:$i");
            $this->assertEquals(1, $this->valkey_glide->dbsize($key));
        }
    }

    public function testInfo()
    {
        $fields = [
            "redis_version", "arch_bits", "uptime_in_seconds", "uptime_in_days",
            "connected_clients", "connected_slaves", "used_memory",
            "total_connections_received", "total_commands_processed",
            "role"
        ];

        // Test 1: primarySlotKey routing (array format)
        for ($i = 0; $i < 3; $i++) {
            $info = $this->valkey_glide->info(['type' => 'primarySlotKey', 'key' => "test-key-$i"]);
            $this->assertIsArray($info);
            foreach ($fields as $field) {
                $this->assertArrayKey($info, $field);
            }
        }

        // Test 2: randomNode routing (string format)
        $randomNodeInfo = $this->valkey_glide->info("randomNode");
        $this->assertIsArray($randomNodeInfo);
        foreach ($fields as $field) {
            $this->assertArrayKey($randomNodeInfo, $field);
        }

        // Test 3: randomNode with specific section
        $randomNodeServerInfo = $this->valkey_glide->info("randomNode", "server");
        $this->assertIsArray($randomNodeServerInfo);
        $this->assertArrayKey($randomNodeServerInfo, "redis_version");

        // Test 4: allPrimaries routing
        $allPrimariesInfo = $this->valkey_glide->info("allPrimaries");
        $this->assertIsArray($allPrimariesInfo);
        $this->assertGT(0, count($allPrimariesInfo), "allPrimaries should return data from multiple nodes");

        // Test 5: allPrimaries with specific section
        $allPrimariesMemoryInfo = $this->valkey_glide->info("allPrimaries", "memory");
        $this->assertIsArray($allPrimariesMemoryInfo);
        $this->assertEquals(6, count($allPrimariesMemoryInfo), "Should have 12 entries (6 nodes * 2 entries each)");

        // Test 6: allNodes routing
        $allNodesInfo = $this->valkey_glide->info("allNodes", "cpu");
        $this->assertIsArray($allNodesInfo);
        $this->assertEquals(12, count($allNodesInfo), "Should have 12 entries (6 nodes * 2 entries each)");

        $nodesSeen = [];
        // Test each node entry
        foreach ($allNodesInfo as $index => $nodeInfo) {
            if ($index % 2 == 0) {
                $this->assertIsInt($nodeInfo['127.0.0.1'], "Port field should be an integer");
                $nodePort = $nodeInfo['127.0.0.1'];
                $this->assertFalse(array_key_exists($nodePort, $nodesSeen));
                $this->assertIsArray($nodeInfo, 1);
                $nodesSeen[$nodePort] = true;
            } else {
                // Should contain used_cpu_sys field (since we requested cpu section)
                $this->assertArrayKey($nodeInfo, 'used_cpu_sys');
            }
        }

        // Test 7: Simple key string routing (slot-based routing)
        $keyBasedInfo = $this->valkey_glide->info("simple-test-key");
        $this->assertIsArray($keyBasedInfo);
        foreach ($fields as $field) {
            $this->assertArrayKey($keyBasedInfo, $field);
        }

        // Test 8: Simple key string routing with section
        $keyBasedServerInfo = $this->valkey_glide->info("simple-test-key", "server");
        $this->assertIsArray($keyBasedServerInfo);
        $this->assertArrayKey($keyBasedServerInfo, "redis_version");

        // Test 9: routeByAddress routing (specific node)
        $routeByAddressInfo = $this->valkey_glide->info(['type' => 'routeByAddress', 'host' => '127.0.0.1', 'port' => 7001]);
        $this->assertIsArray($routeByAddressInfo);
        foreach ($fields as $field) {
            $this->assertArrayKey($routeByAddressInfo, $field);
        }

        // Test 10: routeByAddress with specific section
        $routeByAddressMemoryInfo = $this->valkey_glide->info(['type' => 'routeByAddress', 'host' => '127.0.0.1', 'port' => 7001], "memory");
        $this->assertIsArray($routeByAddressMemoryInfo);
        $this->assertArrayKey($routeByAddressMemoryInfo, "used_memory");

        // Test 11: Multiple sections with different routing types
        $multiSectionInfo = $this->valkey_glide->info("randomNode", "server", "memory");
        $this->assertIsArray($multiSectionInfo);
        $this->assertArrayKey($multiSectionInfo, "redis_version");
        $this->assertArrayKey($multiSectionInfo, "used_memory");

        // Test 12: All sections (no section parameter)
        $allSectionsInfo = $this->valkey_glide->info("randomNode");
        $this->assertIsArray($allSectionsInfo);
        $this->assertGT(10, count($allSectionsInfo), "All sections should return many fields");
    }

    public function testClient()
    {
        $key = 'key-' . rand(1, 100);

//        $this->assertTrue($this->valkey_glide->client($key, 'setname', 'cluster_tests'));

        $clients = $this->valkey_glide->client($key, 'list');
        $this->assertIsArray($clients);

        /* Find us in the list */
        $addr = null;
        foreach ($clients as $client) {
            if ($client['name'] == 'cluster_tests') {
                $addr = $client['addr'];
                break;
            }
        }

        /* We should be in there */
//        $this->assertIsString($addr);

        /* Kill our own client! */
//        $this->assertTrue($this->valkey_glide->client($key, 'kill', $addr));
    }

    public function testTime()
    {
        [$sec, $usec] = $this->valkey_glide->time(uniqid());
        $this->assertEquals(strval(intval($sec)), strval($sec));
        $this->assertEquals(strval(intval($usec)), strval($usec));
    }

    public function testScan()
    {
        set_time_limit(10); // Enforce a 10-second limit on this test
        $key_count = 0;
        $scan_count = 0;

        /* Iterate over our masters, scanning each one */
        $key_count = $this->valkey_glide->dbsize("allPrimaries");
        /* Scan the keys here using ClusterScanCursor - create new cursor each iteration */
        $cursor = new ClusterScanCursor(); // Create fresh cursor each time
        while (true) {
            $keys = $this->valkey_glide->scan($cursor);
            if ($keys) {
                $scan_count += count($keys);
            }
            $new_cursor = new ClusterScanCursor($cursor->getNextCursor()); // Create a new cursor with the updated cursor ID
            $cursor = $new_cursor; // Update the cursor reference
            if ($cursor->isFinished()) {
                break;
            }
            // Cursor goes out of scope here, destructor should be called
        }


        /* Our total key count should match */
        $this->assertEquals($scan_count, $key_count);
        set_time_limit(0);  // Reset to unlimited (or default) at the end
    }

    public function testScanPattern()
    {
         return;//TODO
        $id = uniqid();

            $keys = [];
            // Create some simple keys and lists
        for ($i = 0; $i < 3; $i++) {
            $simple = "simple:{$id}:$i";
            $list = "list:{$id}:$i";

            $this->valkey_glide->set($simple, $i);
            $this->valkey_glide->del($list);
            $this->valkey_glide->rpush($list, ['foo']);

            $keys['STRING'][] = $simple;
            $keys['LIST'][] = $list;
        }

        // Make sure we can scan for specific types
        $cursor = new ClusterScanCursor(); // Create fresh cursor each time

        foreach ($keys as $type => $vals) {
            foreach ([0, 13] as $count) {
                $resp = [];

                while (true) {
                    $scan = $this->valkey_glide->scan($cursor, "*$id*", $count, $type);
                    if ($scan) {
                        $resp = array_merge($resp, $scan);
                    }
                    $new_cursor = new ClusterScanCursor($cursor->getNextCursor()); // Create a new cursor with the updated cursor ID
                    $cursor = $new_cursor; // Update the cursor reference
                    if ($cursor->isFinished()) {
                        break;
                    }
                }

                $this->assertEqualsCanonicalizing($vals, $resp);
            }
        }
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub()
    {
        $this->markTestSkipped();

        // PUBSUB CHANNELS ...
        $result = $this->valkey_glide->pubsub("somekey", "channels", "*");
        $this->assertIsArray($result);
        $result = $this->valkey_glide->pubsub("somekey", "channels");
        $this->assertIsArray($result);

        // PUBSUB NUMSUB

        $c1 = '{pubsub}-' . rand(1, 100);
        $c2 = '{pubsub}-' . rand(1, 100);

        $result = $this->valkey_glide->pubsub("{pubsub}", "numsub", $c1, $c2);

        // Should get an array back, with two elements
        $this->assertIsArray($result);
        $this->assertEquals(4, count($result));

        $zipped = [];
        for ($i = 0; $i <= count($result) / 2; $i += 2) {
            $zipped[$result[$i]] = $result[$i + 1];
        }
        $result = $zipped;

        // Make sure the elements are correct, and have zero counts
        foreach ([$c1,$c2] as $channel) {
            $this->assertArrayKey($result, $channel);
            $this->assertEquals(0, $result[$channel]);
        }

        // PUBSUB NUMPAT
        $result = $this->valkey_glide->pubsub("somekey", "numpat");
        $this->assertIsInt($result);

        // Invalid call
        $this->assertFalse($this->valkey_glide->pubsub("somekey", "notacommand"));
    }

    /* Unlike ValkeyGlide proper, MsetNX won't always totally fail if all keys can't
     * be set, but rather will only fail per-node when that is the case */
    public function testMSetNX()
    {
        $this->markTestSkipped();//TODO understand how to do it in GLIDE

        /* All of these keys should get set */
        $this->valkey_glide->del('x', 'y', 'z');
        $ret = $this->valkey_glide->msetnx(['x' => 'a', 'y' => 'b', 'z' => 'c']);
        $this->assertIsArray($ret);
        $this->assertEquals(array_sum($ret), count($ret));

        /* Delete one key */
        $this->valkey_glide->del('x');
        $ret = $this->valkey_glide->msetnx(['x' => 'a', 'y' => 'b', 'z' => 'c']);
        $this->assertIsArray($ret);
        $this->assertEquals(1, array_sum($ret));

        $this->assertFalse($this->valkey_glide->msetnx([])); // set ø → FALSE
    }

    /* Slowlog needs to take a key or [ip, port], to direct it to a node */
    public function testSlowlog()
    {
        $this->markTestSkipped();
        $key = uniqid() . '-' . rand(1, 1000);

        $this->assertIsArray($this->valkey_glide->slowlog($key, 'get'));
        $this->assertIsArray($this->valkey_glide->slowlog($key, 'get', 10));
        $this->assertIsInt($this->valkey_glide->slowlog($key, 'len'));
        $this->assertTrue($this->valkey_glide->slowlog($key, 'reset'));
        $this->assertFalse(@$this->valkey_glide->slowlog($key, 'notvalid'));
    }

    /* INFO COMMANDSTATS requires a key or ip:port for node direction */
    public function testInfoCommandStats()
    {
        $info = $this->valkey_glide->info("3", "COMMANDSTATS");

        $this->assertIsArray($info);
        if (is_array($info)) {
            foreach ($info as $k => $value) {
                $this->assertStringContains('cmdstat_', $k);
            }
        }
    }

    /* ValkeyGlideCluster will always respond with an array, even if transactions
     * failed, because the commands could be coming from multiple nodes */
    public function testFailedTransactions()
    {
        $this->valkey_glide->set('x', 42);

        // failed transaction
        $this->valkey_glide->watch('x');

        $r = $this->newInstance(); // new instance, modifying `x'.
        $r->incr('x');

        // This transaction should fail because the other client changed 'x'
        $ret = $this->valkey_glide->multi()->get('x')->exec();
        $this->assertEquals(false, $ret);
        // watch and unwatch
        $this->valkey_glide->watch(['x']);
        $r->incr('x'); // other instance
        $this->valkey_glide->unwatch(); // cancel transaction watch

        // This should succeed as the watch has been cancelled
        $ret = $this->valkey_glide->multi()->get('x')->exec();
        $this->assertEquals(['44'], $ret);
    }

    public function testDiscard()
    {
        $this->markTestSkipped();
        $this->valkey_glide->multi();
        $this->valkey_glide->set('pipecount', 'over9000');
        $this->valkey_glide->get('pipecount');

        $this->assertTrue($this->valkey_glide->discard());
    }

    /* ValkeyGlideCluster::script() is a 'raw' command, which requires a key such that
     * we can direct it to a given node */
    public function testScript()
    {
        $this->markTestSkipped();
        $key = uniqid() . '-' . rand(1, 1000);

        // Flush any scripts we have
        $this->assertTrue($this->valkey_glide->script($key, 'flush'));

        // Silly scripts to test against
        $s1_src = 'return 1';
        $s1_sha = sha1($s1_src);
        $s2_src = 'return 2';
        $s2_sha = sha1($s2_src);
        $s3_src = 'return 3';
        $s3_sha = sha1($s3_src);

        // None should exist
        $result = $this->valkey_glide->script($key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertIsArray($result, 3);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

        // Load them up
        $this->assertEquals($s1_sha, $this->valkey_glide->script($key, 'load', $s1_src));
        $this->assertEquals($s2_sha, $this->valkey_glide->script($key, 'load', $s2_src));
        $this->assertEquals($s3_sha, $this->valkey_glide->script($key, 'load', $s3_src));

        // They should all exist
        $result = $this->valkey_glide->script($key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    /* ValkeyGlideCluster::EVALSHA needs a 'key' to let us know which node we want to
     * direct the command at */
    public function testEvalSHA()
    {
        $this->markTestSkipped();
        $key = uniqid() . '-' . rand(1, 1000);

        // Flush any loaded scripts
        $this->valkey_glide->script($key, 'flush');

        // Non existent script (but proper sha1), and a random (not) sha1 string
        $this->assertFalse($this->valkey_glide->evalsha(sha1(uniqid()), [$key], 1));
        $this->assertFalse($this->valkey_glide->evalsha('some-random-data'), [$key], 1);

        // Load a script
        $cb  = uniqid(); // To ensure the script is new
        $scr = "local cb='$cb' return 1";
        $sha = sha1($scr);

        // Run it when it doesn't exist, run it with eval, and then run it with sha1
        $this->assertFalse($this->valkey_glide->evalsha($scr, [$key], 1));
        $this->assertEquals(1, $this->valkey_glide->eval($scr, [$key], 1));
        $this->assertEquals(1, $this->valkey_glide->evalsha($sha, [$key], 1));
    }

    public function testEvalBulkResponse()
    {
        $this->markTestSkipped();
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->valkey_glide->script($key1, 'flush');
        $this->valkey_glide->script($key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $result = $this->valkey_glide->eval($scr, [$key1, $key2], 2);

        $this->assertEquals($key1, $result[0]);
        $this->assertEquals($key2, $result[1]);
    }

    public function testEvalBulkResponseMulti()
    {
        $this->markTestSkipped();
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->valkey_glide->script($key1, 'flush');
        $this->valkey_glide->script($key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $this->valkey_glide->multi();
        $this->valkey_glide->eval($scr, [$key1, $key2], 2);

        $result = $this->valkey_glide->exec();

        $this->assertEquals($key1, $result[0][0]);
        $this->assertEquals($key2, $result[0][1]);
    }

    public function testEvalBulkEmptyResponse()
    {
        $this->markTestSkipped();
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->valkey_glide->script($key1, 'flush');
        $this->valkey_glide->script($key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $result = $this->valkey_glide->eval($scr, [$key1, $key2], 2);

        $this->assertNull($result);
    }

    public function testEvalBulkEmptyResponseMulti()
    {
        $this->markTestSkipped();
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->valkey_glide->script($key1, 'flush');
        $this->valkey_glide->script($key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $this->valkey_glide->multi();
        $this->valkey_glide->eval($scr, [$key1, $key2], 2);
        $result = $this->valkey_glide->exec();

        $this->assertNull($result[0]);
    }

    /* Cluster specific introspection stuff */
    public function testIntrospection()
    {
        $this->markTestSkipped();
        $primaries = $this->valkey_glide->_masters();
        $this->assertIsArray($primaries);

        foreach ($primaries as [$host, $port]) {
            $this->assertIsString($host);
            $this->assertIsInt($port);
        }
    }

    protected function keyTypeToString($key_type)
    {
        switch ($key_type) {
            case ValkeyGlide::VALKEY_GLIDE_STRING:
                return "string";
            case ValkeyGlide::VALKEY_GLIDE_SET:
                return "set";
            case ValkeyGlide::VALKEY_GLIDE_LIST:
                return "list";
            case ValkeyGlide::VALKEY_GLIDE_ZSET:
                return "zset";
            case ValkeyGlide::VALKEY_GLIDE_HASH:
                return "hash";
            case ValkeyGlide::VALKEY_GLIDE_STREAM:
                return "stream";
            default:
                return "unknown($key_type)";
        }
    }

    protected function genKeyName($key_index, $key_type)
    {
        return sprintf('%s-%s', $this->keyTypeToString($key_type), $key_index);
    }

    protected function setKeyVals($key_index, $key_type, &$arr_ref)
    {
        $key = $this->genKeyName($key_index, $key_type);

        $this->valkey_glide->del($key);

        switch ($key_type) {
            case ValkeyGlide::VALKEY_GLIDE_STRING:
                $value = "$key-value";
                $this->valkey_glide->set($key, $value);
                break;
            case ValkeyGlide::VALKEY_GLIDE_SET:
                $value = [
                    "$key-mem1", "$key-mem2", "$key-mem3",
                    "$key-mem4", "$key-mem5", "$key-mem6"
                ];
                $args = $value;
                array_unshift($args, $key);
                call_user_func_array([$this->valkey_glide, 'sadd'], $args);
                break;
            case ValkeyGlide::VALKEY_GLIDE_HASH:
                $value = [
                    "$key-mem1" => "$key-val1",
                    "$key-mem2" => "$key-val2",
                    "$key-mem3" => "$key-val3"
                ];
                $this->valkey_glide->hmset($key, $value);
                break;
            case ValkeyGlide::VALKEY_GLIDE_LIST:
                $value = [
                    "$key-ele1", "$key-ele2", "$key-ele3",
                    "$key-ele4", "$key-ele5", "$key-ele6"
                ];
                $args = $value;
                array_unshift($args, $key);
                call_user_func_array([$this->valkey_glide, 'rpush'], $args);
                break;
            case ValkeyGlide::VALKEY_GLIDE_ZSET:
                $score = 1;
                $value = [
                    "$key-mem1" => 1, "$key-mem2" => 2,
                    "$key-mem3" => 3, "$key-mem3" => 3
                ];
                foreach ($value as $mem => $score) {
                    $this->valkey_glide->zadd($key, $score, $mem);
                }
                break;
        }

        /* Update our reference array so we can verify values */
        $arr_ref[$key] = $value;

        return $key;
    }

    /* Verify that our ZSET values are identical */
    protected function checkZSetEquality($a, $b)
    {
        /* If the count is off, the array keys are different or the sums are
         * different, we know there is something off */
        $boo_diff = count($a) != count($b) ||
            count(array_diff(array_keys($a), array_keys($b))) != 0 ||
            array_sum($a) != array_sum($b);

        if ($boo_diff) {
            $this->assertEquals($a, $b);
            return;
        }
    }

    protected function checkKeyValue($key, $key_type, $value)
    {
        switch ($key_type) {
            case ValkeyGlide::VALKEY_GLIDE_STRING:
                $this->assertEquals($value, $this->valkey_glide->get($key));
                break;
            case ValkeyGlide::VALKEY_GLIDE_SET:
                $arr_r_values = $this->valkey_glide->sMembers($key);
                $arr_l_values = $value;
                sort($arr_r_values);
                sort($arr_l_values);
                $this->assertEquals($arr_r_values, $arr_l_values);
                break;
            case ValkeyGlide::VALKEY_GLIDE_LIST:
                $this->assertEquals($value, $this->valkey_glide->lrange($key, 0, -1));
                break;
            case ValkeyGlide::VALKEY_GLIDE_HASH:
                $this->assertEquals($value, $this->valkey_glide->hgetall($key));
                break;
            case ValkeyGlide::VALKEY_GLIDE_ZSET:
                $this->checkZSetEquality($value, $this->valkey_glide->zrange($key, 0, -1, true));
                break;
            default:
                throw new Exception("Unknown type " . $key_type);
        }
    }

    /* Test a 'raw' command */
    public function testRawCommand()
    {
        $this->valkey_glide->rawCommand('mykey', 'set', 'mykey', 'my-value');
        $this->assertEquals('my-value', $this->valkey_glide->get('mykey'));

        $this->valkey_glide->del('mylist');
        $this->valkey_glide->rpush('mylist', 'A', 'B', 'C', 'D');
        $this->assertEquals(['A', 'B', 'C', 'D'], $this->valkey_glide->lrange('mylist', 0, -1));
    }

    protected function rawCommandArray($key, $args)
    {
        array_unshift($args, $key);
        return call_user_func_array([$this->valkey_glide, 'rawCommand'], $args);
    }

    /* Test that rawCommand and EVAL can be configured to return simple string values */
    public function testReplyLiteral()
    {
        $this->markTestSkipped();

        $this->valkey_glide->setOption(ValkeyGlide::OPT_REPLY_LITERAL, false);
        $this->assertTrue($this->valkey_glide->rawCommand('foo', 'set', 'foo', 'bar'));
        $this->assertTrue($this->valkey_glide->eval("return redis.call('set', KEYS[1], 'bar')", ['foo'], 1));

        $rv = $this->valkey_glide->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals([true, true], $rv);

        $this->valkey_glide->setOption(ValkeyGlide::OPT_REPLY_LITERAL, true);
        $this->assertEquals('OK', $this->valkey_glide->rawCommand('foo', 'set', 'foo', 'bar'));
        $this->assertEquals('OK', $this->valkey_glide->eval("return redis.call('set', KEYS[1], 'bar')", ['foo'], 1));

        $rv = $this->valkey_glide->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals(['OK', 'PONG'], $rv);

        // Reset
        $this->valkey_glide->setOption(ValkeyGlide::OPT_REPLY_LITERAL, false);
    }

    public function testCopyCluster()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped('COPY command requires Valkey 6.2.0+');
        }

        $this->valkey_glide->del('{key}dst');
        $this->valkey_glide->set('{key}src', 'foo');
        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->valkey_glide->set('{key}src', 'bar');
        $this->assertFalse($this->valkey_glide->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst', ['REPLACE' => true]));
        $this->assertKeyEquals('bar', '{key}dst');
    }

    public function testCopyClusterWithDatabase()
    {
        if (version_compare($this->version, '9.0.0') < 0) {
            $this->markTestSkipped('COPY with database ID in cluster mode requires Valkey 9.0.0+');
        }

        // Test copy to different database in cluster mode
        $this->valkey_glide->del('{key}src', '{key}dst');
        $this->valkey_glide->set('{key}src', 'cluster_test_value');
        
        // Test with string key
        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst', ['DB' => 1]));
        
        // Test with constant
        $this->valkey_glide->set('{key}src2', 'cluster_constant_test');
        $this->assertTrue($this->valkey_glide->copy('{key}src2', '{key}dst2', [ValkeyGlide::COPY_DB => 1]));
        
        // Test combined options
        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst', [
            ValkeyGlide::COPY_DB => 1,
            ValkeyGlide::COPY_REPLACE => true
        ]));
    }

    public function testSelectMultipleDatabase()
    {
        if (version_compare($this->version, '9.0.0') < 0) {
            $this->markTestSkipped('Multi-database operations in cluster mode require Valkey 9.0.0+');
        }

        // SELECT should work in Valkey 9.0+ clusters
        $this->assertTrue($this->valkey_glide->select(0));
        $this->assertTrue($this->valkey_glide->select(1));
        $this->assertTrue($this->valkey_glide->select(2));
        $this->assertTrue($this->valkey_glide->select(15));
        $this->assertFalse(@$this->valkey_glide->select(-1));
        $this->assertTrue($this->valkey_glide->select(0));
    }

    public function testDatabaseIsolation()
    {
        if (version_compare($this->version, '9.0.0') < 0) {
            $this->markTestSkipped('Multi-database operations in cluster mode require Valkey 9.0.0+');
        }

        $key = '{key}isolation_test_' . uniqid();
        
        $this->valkey_glide->select(0);
        $this->valkey_glide->set($key, 'value_db0');
        $this->valkey_glide->select(1);
        $this->valkey_glide->set($key, 'value_db1');
        
        $this->valkey_glide->select(0);
        $this->assertEquals('value_db0', $this->valkey_glide->get($key));
        $this->valkey_glide->select(1);
        $this->assertEquals('value_db1', $this->valkey_glide->get($key));
        
        // Clean up
        $this->valkey_glide->del($key);
        $this->valkey_glide->select(0);
        $this->valkey_glide->del($key);
    }

    public function testMoveMultiDatabase()
    {
        if (version_compare($this->version, '9.0.0') < 0) {
            $this->markTestSkipped('Multi-database MOVE in cluster mode requires Valkey 9.0.0+');
        }

        $key = '{key}move_test_' . uniqid();
        
        $this->valkey_glide->select(0);
        $this->valkey_glide->set($key, 'move_test_value');
        
        // In Valkey 9.0+, MOVE should succeed - failure indicates missing cluster-databases config
        $result = $this->valkey_glide->move($key, 1);
        $this->assertTrue($result, 'MOVE should succeed in Valkey 9.0+ cluster (ensure cluster-databases > 1 is configured)');
        
        // Verify MOVE worked correctly
        $this->assertEquals(0, $this->valkey_glide->exists($key)); // Should not exist in DB 0
        
        $this->valkey_glide->select(1);
        $this->assertEquals(1, $this->valkey_glide->exists($key)); // Should exist in DB 1
        $this->assertEquals('move_test_value', $this->valkey_glide->get($key));
        
        // Clean up
        $this->valkey_glide->del($key);
        $this->valkey_glide->select(0);
    }

    public function testCopyMultiDatabase()
    {
        if (version_compare($this->version, '9.0.0') < 0) {
            $this->markTestSkipped('Multi-database COPY in cluster mode requires Valkey 9.0.0+');
        }

        $srcKey = '{key}copy_src_' . uniqid();
        $dstKey = '{key}copy_dst_' . uniqid();
        
        $this->valkey_glide->select(0);
        $this->valkey_glide->set($srcKey, 'copy_test_value');
        
        // COPY with DB parameter should work in Valkey 9.0+ clusters
        $result = $this->valkey_glide->copy($srcKey, $dstKey, ['DB' => 1]);
        $this->assertTrue($result, 'COPY should succeed in Valkey 9.0+ cluster');
        
        // Verify COPY worked correctly
        $this->assertEquals('copy_test_value', $this->valkey_glide->get($srcKey)); // Original still exists
        
        $this->valkey_glide->select(1);
        $this->assertEquals('copy_test_value', $this->valkey_glide->get($dstKey)); // Copy exists
        
        // Clean up
        $this->valkey_glide->del($dstKey);
        $this->valkey_glide->select(0);
        $this->valkey_glide->del($srcKey);
    }

    public function testRefreshTopologyFromInitialNodesDefault()
    {
        // Test that refresh_topology_from_initial_nodes defaults to false when not specified
        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'localhost', 'port' => 7001]],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
            client_az: null,
            advanced_config: ['connection_timeout' => 5000]
        );
        
        // Perform basic operation to verify client works
        $result = $client->ping(['type' => 'primarySlotKey', 'key' => 'test']);
        $this->assertTrue($result);
        
        $client->close();
    }

    public function testRefreshTopologyFromInitialNodesEnabled()
    {
        // Test that refresh_topology_from_initial_nodes can be set to true
        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'localhost', 'port' => 7001]],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
            client_az: null,
            advanced_config: [
                'connection_timeout' => 5000,
                'refresh_topology_from_initial_nodes' => true
            ]
        );
        
        // Perform basic operation to verify client works
        $result = $client->ping(['type' => 'primarySlotKey', 'key' => 'test']);
        $this->assertTrue($result);
        
        $client->close();
    }

    public function testRefreshTopologyFromInitialNodesDisabled()
    {
        // Test that refresh_topology_from_initial_nodes can be explicitly set to false
        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'localhost', 'port' => 7001]],
            use_tls: false,
            credentials: null,
            read_from: ValkeyGlide::READ_FROM_PREFER_REPLICA,
            request_timeout: null,
            reconnect_strategy: null,
            client_name: null,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_ENABLED_DEFAULT_CONFIGS,
            client_az: null,
            advanced_config: [
                'connection_timeout' => 5000,
                'refresh_topology_from_initial_nodes' => false
            ]
        );
        
        // Perform basic operation to verify client works
        $result = $client->ping(['type' => 'primarySlotKey', 'key' => 'test']);
        $this->assertTrue($result);
        
        $client->close();
    }
}
