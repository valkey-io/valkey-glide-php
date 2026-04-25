<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideTest.php";

/**
 * Most ValkeyGlideBatchCluster tests should work the same as the standard ValkeyGlide object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class ValkeyGlideClusterBatchTest extends ValkeyGlideBatchTest
{
    private static string $seed_source = '';

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

        // Log server type and version for debugging
        $server_type = $this->is_valkey ? 'Valkey' : 'Redis';
        echo "Connected to $server_type cluster batch server version: {$this->version}\n";
    }

    /* Override newInstance as we want a ValkeyGlideCluster object */
    protected function newInstance()
    {
        try {
            return new ValkeyGlideCluster(
                addresses: [['host' => '127.0.0.1', 'port' => 7001]],
                use_tls: false,
                credentials: $this->getAuth(),
                read_from: ValkeyGlide::READ_FROM_PRIMARY,
            );
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error: %s\n", $ex->getMessage());
            //TestSuite::errorMessage("Seeds: %s\n", implode(' ', self::$seeds));
            TestSuite::errorMessage("Seed source: %s\n", self::$seed_source);
            exit(1);
        }
    }

    public function testServerOperationsBatch()
    {
    }

    public function testInfoOperationsBatch()
    {

        // Execute INFO, CLIENT ID, CLIENT GETNAME in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->client('id')
          //  ->client('setname', 'phpredis_unit_tests')//TODO return once setname is supported
            ->client('getname')
            ->client('list')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsInt($results[0]); // CLIENT ID result (integer)
        // CLIENT GETNAME might return null if no name is set
        $this->assertEquals('valkey-glide-php', $results[1]); // CLIENT SETNAME result

        $this->assertGT(0, $results[0]); // Client ID should be positive
    }

    public function testDatabaseOperationsBatch()
    {
        $key1 = '{xxx}batch_db_' . uniqid();

        // Execute SELECT, DBSIZE, TYPE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->set('{xxx}x', 'y')
            ->set($key1, 'test_value')
            ->type($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_STRING, $results[2]); // TYPE result


        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testAdvancedKeyOperationsBatch()
    {

        $key1 = '{xyz}batch_adv_1_' . uniqid();
        $key2 = '{xyz}batch_adv_2_' . uniqid();
        $key3 = '{xyz}batch_adv_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'test_value');

        // Execute UNLINK, TOUCH, RANDOMKEY in multi/exec batch
        $results = $this->valkey_glide->multi()
          ->unlink($key1)
          ->touch($key2, $key3) // Touch non-existing keys
          ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(2, $results);
        $this->assertEquals(1, $results[0]); // UNLINK result (1 key removed)
        $this->assertEquals(0, $results[1]); // TOUCH result (0 keys touched - they don't exist)
        // RANDOMKEY result can be any existing key or null

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Key unlinked

        // No cleanup needed as keys were deleted/don't exist
    }

    public function testConfigOperationsBatch()
    {
       // Config operations are not supported in batch and cluster mode
    }

    public function testFlushOperationsBatch()
    {
        // Flush operations are not supported in batch and cluster mode
    }

    public function testWaitBatch()
    {
        // WAIT is not supported in batch and cluster mode
    }


    public function testScanOperationsBatch()
    {
        $key1 = '{scantest}batch_scan_set_' . uniqid();
        $key2 = '{scantest}batch_scan_hash_' . uniqid();

        // Setup test data
        $this->valkey_glide->del($key1, $key2);
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3');
        $this->valkey_glide->hset($key2, 'field1', 'value1', 'field2', 'value2');

        // Execute SSCAN, HSCAN in multi/exec batch
        $sscan_it = null;
        $hscan_it = null;

        $results = $this->valkey_glide->multi()
            ->sscan($key1, $sscan_it)
            ->hscan($key2, $hscan_it)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(2, $results);
        $this->assertIsArray($results[0]); // SSCAN result [cursor, members]
        $sscan_it = null;
        $this->assertEquals($results[0], $this->valkey_glide->sscan($key1, $sscan_it));
        $this->assertIsArray($results[1]); // HSCAN result [cursor, fields_values]
        $hscan_it = null;
        $this->assertEquals($results[1], $this->valkey_glide->hscan($key2, $hscan_it));
        // Verify server-side effects (scan operations don't modify data)
        $this->assertEquals(3, $this->valkey_glide->scard($key1));
        $this->assertEquals(2, $this->valkey_glide->hlen($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    public function testMoveBatch()
    {
        // MOVE is not supported in cluster mode
    }

    public function testFunctionManagementBatch()
    {

        // FUNCTION management is not supported in batch and cluster mode
    }

    public function testFunctionDumpRestoreBatch()
    {
        // FUNCTION DUMP and RESTORE are not supported in batch and cluster mode
    }

    // ===================================================================
    // COMPRESSION BATCH TESTS
    // ===================================================================

    /**
     * Helper to create a cluster client with compression enabled
     */
    private function createCompressedClusterBatchClient(): ValkeyGlideCluster
    {
        $client = new ValkeyGlideCluster();
        $client->connect(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            compression: [
                'enabled' => true,
                'backend' => ValkeyGlideCluster::COMPRESSION_BACKEND_ZSTD,
                'min_compression_size' => 64
            ]
        );
        return $client;
    }

    /**
     * Test compression with cluster batch SET/GET operations
     */
    public function testCompressionClusterBatchSetGet()
    {
        $client = $this->createCompressedClusterBatchClient();

        $key1 = '{compress}cluster_batch_set_1_' . uniqid();
        $key2 = '{compress}cluster_batch_set_2_' . uniqid();
        $key3 = '{compress}cluster_batch_set_3_' . uniqid();

        // Create values large enough to trigger compression (> 64 bytes)
        $value1 = str_repeat('A', 100);
        $value2 = str_repeat('B', 150);
        $value3 = str_repeat('C', 200);

        // Execute SET operations in batch with compression
        $results = $client->multi()
            ->set($key1, $value1)
            ->set($key2, $value2)
            ->set($key3, $value3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]);
        $this->assertTrue($results[1]);
        $this->assertTrue($results[2]);

        // Execute GET operations in batch with compression
        $getResults = $client->multi()
            ->get($key1)
            ->get($key2)
            ->get($key3)
            ->exec();

        // Verify decompressed values match original
        $this->assertIsArray($getResults);
        $this->assertCount(3, $getResults);
        $this->assertEquals($value1, $getResults[0]);
        $this->assertEquals($value2, $getResults[1]);
        $this->assertEquals($value3, $getResults[2]);

        // Cleanup
        $client->del($key1, $key2, $key3);
        $client->close();
    }

    /**
     * Test compression with cluster batch MSET/MGET operations
     */
    public function testCompressionClusterBatchMsetMget()
    {
        $client = $this->createCompressedClusterBatchClient();

        $key1 = '{compress}cluster_batch_mset_1_' . uniqid();
        $key2 = '{compress}cluster_batch_mset_2_' . uniqid();
        $key3 = '{compress}cluster_batch_mset_3_' . uniqid();

        // Create values large enough to trigger compression
        $value1 = str_repeat('X', 100);
        $value2 = str_repeat('Y', 150);
        $value3 = str_repeat('Z', 200);

        // Execute MSET and MGET in batch with compression
        $results = $client->multi()
            ->mset([$key1 => $value1, $key2 => $value2])
            ->mget([$key1, $key2])
            ->set($key3, $value3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // MSET result
        $this->assertEquals([$value1, $value2], $results[1]); // MGET result with decompressed values
        $this->assertTrue($results[2]); // SET result

        // Verify values can be retrieved correctly
        $finalGet = $client->mget([$key1, $key2, $key3]);
        $this->assertEquals([$value1, $value2, $value3], $finalGet);

        // Cleanup
        $client->del($key1, $key2, $key3);
        $client->close();
    }

    /**
     * Test that blocked commands return false in cluster batch mode with compression
     */
    public function testCompressionClusterBatchBlockedCommands()
    {
        $client = $this->createCompressedClusterBatchClient();

        $key1 = '{compress}cluster_batch_blocked_' . uniqid();

        // Set up a key first
        $client->set($key1, 'test_value');

        // Test APPEND - batch should fail entirely when blocked command is used with compression
        $results = $client->multi()
            ->append($key1, '_suffix')
            ->exec();

        $this->assertFalse($results); // Entire batch fails when blocked command is used

        // Test INCR - batch should fail entirely when blocked command is used with compression
        $client->set($key1, '10');
        $results = $client->multi()
            ->incr($key1)
            ->exec();

        $this->assertFalse($results); // Entire batch fails when blocked command is used

        // Test STRLEN - batch should fail entirely when blocked command is used with compression
        $results = $client->multi()
            ->strlen($key1)
            ->exec();

        $this->assertFalse($results); // Entire batch fails when blocked command is used

        // Cleanup
        $client->del($key1);
        $client->close();
    }
}
