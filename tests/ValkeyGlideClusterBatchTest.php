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
}
