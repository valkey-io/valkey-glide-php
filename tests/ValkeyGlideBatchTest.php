<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * ValkeyGlide Batch Test
 * Tests multi/exec batch operations for all supported commands
 * Each test contains 3 commands in a single multi/exec transaction and verifies server-side effects
 */
class ValkeyGlideBatchTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }
    
    // ===================================================================
    // CORE STRING OPERATIONS BATCH TESTS
    // ===================================================================

    public function testStringOperationsBatch()
    {
        $key1 = '{prefix}batch_string_1_' . uniqid();
        $key2 = '{prefix}batch_string_2_' . uniqid();
        $key3 = '{prefix}batch_string_3_' . uniqid();
        $value1 = 'test_value_1';
        $value2 = 'test_value_2';
        $value3 = 'test_value_3';

        // Execute SET, GET, MSET in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->set($key1, $value1)
            ->get($key1)
            ->mset([$key2 => $value2, $key3 => $value3])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results, 3);
        $this->assertTrue($results[0]); // SET result
        $this->assertEquals($value1, $results[1]); // GET result
        $this->assertTrue($results[2]); // MSET result

        // Verify server-side effects
        $this->assertEquals($value1, $this->valkey_glide->get($key1));
        $this->assertEquals($value2, $this->valkey_glide->get($key2));
        $this->assertEquals($value3, $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testIncrementOperationsBatch()
    {
        $key1 = '{prefix}batch_incr_1_' . uniqid();
        $key2 = '{prefix}batch_incr_2_' . uniqid();
        $key3 = '{prefix}batch_incr_3_' . uniqid();

        // Setup initial values
        $this->valkey_glide->set($key1, '10');
        $this->valkey_glide->set($key2, '20');
        $this->valkey_glide->set($key3, '3.14');

        // Execute INCR, INCRBY, INCRBYFLOAT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->incr($key1)
            ->incrby($key2, 5)
            ->incrbyfloat($key3, 2.86)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results, 3);
        $this->assertEquals(11, $results[0]); // INCR result
        $this->assertEquals(25, $results[1]); // INCRBY result
        $this->assertEquals(6.0, (float)$results[2]); // INCRBYFLOAT result

        // Verify server-side effects
        $this->assertEquals('11', $this->valkey_glide->get($key1));
        $this->assertEquals('25', $this->valkey_glide->get($key2));
        $this->assertEquals('6', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testDecrementOperationsBatch()
    {
        $key1 = '{prefix}batch_decr_1_' . uniqid();
        $key2 = '{prefix}batch_decr_2_' . uniqid();
        $key3 = '{prefix}batch_decr_3_' . uniqid();

        // Setup initial values
        $this->valkey_glide->set($key1, '10');
        $this->valkey_glide->set($key2, '20');
        $this->valkey_glide->set($key3, 'test_append');

        // Execute DECR, DECRBY, APPEND in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->decr($key1)
            ->decrby($key2, 3)
            ->append($key3, '_suffix')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(9, $results[0]); // DECR result
        $this->assertEquals(17, $results[1]); // DECRBY result
        $this->assertEquals(18, $results[2]); // APPEND result (length)

        // Verify server-side effects
        $this->assertEquals('9', $this->valkey_glide->get($key1));
        $this->assertEquals('17', $this->valkey_glide->get($key2));
        $this->assertEquals('test_append_suffix', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // KEY MANAGEMENT OPERATIONS BATCH TESTS
    // ===================================================================

    public function testKeyExistenceOperationsBatch()
    {
        $key1 = '{temp}batch_exists_1_' . uniqid();
        $key2 = '{temp}batch_exists_2_' . uniqid();
        $key3 = '{temp}batch_exists_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->set($key2, 'value2');

        // Execute EXISTS, DEL, EXISTS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->exists($key1, $key2, $key3)
            ->del($key1)
            ->exists($key1, $key2, $key3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // EXISTS result (2 keys exist)
        $this->assertEquals(1, $results[1]); // DEL result (1 key deleted)
        $this->assertEquals(1, $results[2]); // EXISTS result (1 key exists)

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // key1 deleted
        $this->assertEquals(1, $this->valkey_glide->exists($key2)); // key2 still exists
        $this->assertEquals(0, $this->valkey_glide->exists($key3)); // key3 never existed

        // Cleanup
        $this->valkey_glide->del($key2);
    }

    public function testKeyExpirationOperationsBatch()
    {
        $key1 = '{temp}batch_expire_1_' . uniqid();
        $key2 = '{temp}batch_expire_2_' . uniqid();
        $key3 = '{temp}batch_expire_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->set($key2, 'value2');
        $this->valkey_glide->set($key3, 'value3');

        $expireTime = time() + 3600; // 1 hour from now
        $pexpireTime = (time() + 3600) * 1000; // 1 hour from now in milliseconds

        // Execute EXPIRE, EXPIREAT, PEXPIRE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->expire($key1, 3600)
            ->expireat($key2, $expireTime)
            ->pexpire($key3, 3600000)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // EXPIRE result
        $this->assertTrue($results[1]); // EXPIREAT result
        $this->assertTrue($results[2]); // PEXPIRE result

        // Verify server-side effects
        $this->assertGT(0, $this->valkey_glide->ttl($key1)); // Has TTL
        $this->assertGT(0, $this->valkey_glide->ttl($key2)); // Has TTL
        $this->assertGT(0, $this->valkey_glide->ttl($key3)); // Has TTL

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testKeyTtlOperationsBatch()
    {
        $key1 = '{temp}batch_ttl_1_' . uniqid();
        $key2 = '{temp}batch_ttl_2_' . uniqid();
        $key3 = '{temp}batch_ttl_3_' . uniqid();

        // Setup test data with different expiration states
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->setex($key2, 3600, 'value2'); // Set with expiration
        $this->valkey_glide->set($key3, 'value3');
        $this->valkey_glide->expire($key3, 3600);

        // Execute TTL, PTTL, PERSIST in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->ttl($key1)
            ->pttl($key2)
            ->persist($key3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(-1, $results[0]); // TTL result (no expiration)
        $this->assertGT(0, $results[1]); // PTTL result (has expiration in ms)
        $this->assertTrue($results[2]); // PERSIST result (removed expiration)

        // Verify server-side effects
        $this->assertEquals(-1, $this->valkey_glide->ttl($key1)); // No expiration
        $this->assertGT(0, $this->valkey_glide->ttl($key2)); // Still has expiration
        $this->assertEquals(-1, $this->valkey_glide->ttl($key3)); // Expiration removed

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // HASH OPERATIONS BATCH TESTS
    // ===================================================================

    public function testHashSetOperationsBatch()
    {
        $key1 = 'batch_hash_1_' . uniqid();
        $key2 = 'batch_hash_2_' . uniqid();
        $key3 = 'batch_hash_3_' . uniqid();

        // Execute HSET, HGET, HGETALL in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hset($key1, 'field1', 'value1')
            ->hget($key1, 'field1')
            ->hgetall($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // HSET result (new field)
        $this->assertEquals('value1', $results[1]); // HGET result
        $this->assertEquals(['field1' => 'value1'], $results[2]); // HGETALL result

        // Verify server-side effects
        $this->assertEquals('value1', $this->valkey_glide->hget($key1, 'field1'));
        $this->assertEquals(1, $this->valkey_glide->hlen($key1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }


    public function testHashIncrementOperationsBatch()
    {
        $key1 = 'batch_hash_incr_' . uniqid();

        // Setup initial hash with numeric values
        $this->valkey_glide->hset($key1, 'counter1', '10', 'counter2', '20', 'float_counter', '3.14');

        // Execute HINCRBY, HINCRBY, HINCRBYFLOAT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hincrby($key1, 'counter1', 5)
            ->hincrby($key1, 'counter2', -3)
            ->hincrbyfloat($key1, 'float_counter', 2.86)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(15, $results[0]); // HINCRBY result
        $this->assertEquals(17, $results[1]); // HINCRBY result
        $this->assertEquals(6.0, (float)$results[2]); // HINCRBYFLOAT result

        // Verify server-side effects
        $this->assertEquals('15', $this->valkey_glide->hget($key1, 'counter1'));
        $this->assertEquals('17', $this->valkey_glide->hget($key1, 'counter2'));
        $this->assertEquals('6', $this->valkey_glide->hget($key1, 'float_counter'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

   

    // ===================================================================
    // SERVER & CONFIG OPERATIONS BATCH TESTS
    // ===================================================================

    public function testServerOperationsBatch()
    {
        $key1 = 'batch_server_' . uniqid();

        // Execute PING, ECHO, TIME in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->ping()
            ->echo('test_message')
            ->time()
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // PING result
        $this->assertEquals('test_message', $results[1]); // ECHO result
        $this->assertIsArray($results[2]); // TIME result (array with seconds and microseconds)
        $this->assertCount(2, $results[2]);

        // No cleanup needed for server operations
    }

    public function testInfoOperationsBatch()
    {
        
        // Execute INFO, CLIENT ID, CLIENT GETNAME in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->info()
            ->client('id')            
            //->client('setname', 'phpredis_unit_tests') //TODO return once setname is supported
            ->client('getname')
            ->client('list')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(4, $results);
        $this->assertIsArray($results[0]); // INFO result (array of server info)
        $this->assertIsInt($results[1]); // CLIENT ID result (integer)
        // CLIENT GETNAME might return null if no name is set
        $this->assertEquals('valkey-glide-php', $results[2]); // CLIENT SETNAME result
        // Verify server info contains expected keys
        $this->assertArrayHasKey('server_name', $results[0]);
        $this->assertGT(0, $results[1]); // Client ID should be positive
    }

    public function testDatabaseOperationsBatch()
    {
        $key1 = 'batch_db_' . uniqid();

        // Execute SELECT, DBSIZE, TYPE in multi/exec batch
        $results = $this->valkey_glide->multi()
           // ->select(0) // Select database 0 (likely current) //TODO return once select is supported
            ->flushDB()
            ->set('x', 'y')
            ->set($key1, 'test_value')
            ->dbsize()
            ->type($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(5, $results);
        //$this->assertTrue($results[0]); // SELECT result
        $this->assertEquals(2, $results[3]);
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_STRING, $results[4]); // TYPE result

        // Verify server-side effects
        $this->assertGTE(1, $this->valkey_glide->dbsize()); // At least 1 key (our test key)

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // STRING MANIPULATION OPERATIONS BATCH TESTS
    // ===================================================================

    public function testStringManipulationBatch()
    {
        $key1 = '{xyz}batch_str_1_' . uniqid();
        $key2 = '{xyz}batch_str_2_' . uniqid();
        $key3 = '{xyz}batch_str_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'Hello World');
        $this->valkey_glide->set($key2, '12345');

        // Execute STRLEN, GETRANGE, SETRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->strlen($key1)
            ->getrange($key1, 0, 4)
            ->setrange($key2, 2, 'ABC')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(11, $results[0]); // STRLEN result
        $this->assertEquals('Hello', $results[1]); // GETRANGE result
        $this->assertEquals(5, $results[2]); // SETRANGE result (new length)

        // Verify server-side effects
        $this->assertEquals(11, $this->valkey_glide->strlen($key1));
        $this->assertEquals('Hello', $this->valkey_glide->getrange($key1, 0, 4));
        $this->assertEquals('12ABC', $this->valkey_glide->get($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testKeyRenameBatch()
    {
        $key1 = '{xyz}batch_rename_1_' . uniqid();
        $key2 = '{xyz}batch_rename_2_' . uniqid();
        $key3 = '{xyz}batch_rename_3_' . uniqid();
        $newKey1 = '{xyz}batch_rename_new_1_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->set($key2, 'value2');

        // Execute RENAME, EXISTS, RENAMENX in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->rename($key1, $newKey1)
            ->exists($key1, $newKey1)
            ->renamenx($key2, $key3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // RENAME result
        $this->assertEquals(1, $results[1]); // EXISTS result (only newKey1 exists now)
        $this->assertTrue($results[2]); // RENAMENX result (success)

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Original key gone
        $this->assertEquals(1, $this->valkey_glide->exists($newKey1)); // New key exists
        $this->assertEquals('value1', $this->valkey_glide->get($newKey1));
        $this->assertEquals(1, $this->valkey_glide->exists($key3)); // Renamed key exists
        $this->assertEquals('value2', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($newKey1, $key3);
    }

    // ===================================================================
    // ADVANCED KEY OPERATIONS BATCH TESTS
    // ===================================================================

    public function testAdvancedKeyOperationsBatch()
    {
        
        $key1 = 'batch_adv_1_' . uniqid();
        $key2 = 'batch_adv_2_' . uniqid();
        $key3 = 'batch_adv_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'test_value');

        // Execute UNLINK, TOUCH, RANDOMKEY in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->unlink($key1)
            ->touch($key2, $key3) // Touch non-existing keys
            ->randomkey()
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // UNLINK result (1 key removed)
        $this->assertEquals(0, $results[1]); // TOUCH result (0 keys touched - they don't exist)
        // RANDOMKEY result can be any existing key or null

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Key unlinked

        // No cleanup needed as keys were deleted/don't exist
    }

    public function testMgetMsetOperationsBatch()
    {
        $key1 = '{xyz}batch_mget_1_' . uniqid();
        $key2 = '{xyz}batch_mget_2_' . uniqid();
        $key3 = '{xyz}batch_mget_3_' . uniqid();

        // Execute MSET, MGET, MSETNX in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->mset([$key1 => 'value1', $key2 => 'value2'])
            ->mget([$key1, $key2, $key3])
            ->msetnx([$key3 => 'value3'])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // MSET result
        $this->assertEquals(['value1', 'value2', false], $results[1]); // MGET result
        $this->assertTrue($results[2]); // MSETNX result (success for key3)

        // Verify server-side effects
        $this->assertEquals('value1', $this->valkey_glide->get($key1));
        $this->assertEquals('value2', $this->valkey_glide->get($key2));
        $this->assertEquals('value3', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // BIT OPERATIONS BATCH TESTS
    // ===================================================================

    public function testBitOperationsBatch()
    {
        $key1 = '{ttt}batch_bit_1_' . uniqid();
        $key2 = '{ttt}batch_bit_2_' . uniqid();
        $key3 = '{ttt}batch_bit_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'hello'); // Binary: 01101000 01100101 01101100 01101100 01101111

        // Execute SETBIT, GETBIT, BITCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->setbit($key2, 7, 1)
            ->getbit($key1, 0)
            ->bitcount($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(0, $results[0]); // SETBIT result (previous bit value)
        $this->assertEquals(0, $results[1]); // GETBIT result
        $this->assertGT(0, $results[2]); // BITCOUNT result

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->getbit($key2, 7));
        $this->assertGT(0, $this->valkey_glide->bitcount($key1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testBitAdvancedOperationsBatch()
    {
        $key1 = '{ttt2}batch_bitadv_1_' . uniqid();
        $key2 = '{ttt2}batch_bitadv_2_' . uniqid();
        $key3 = '{ttt2}batch_bitadv_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, "\x0F\x0F"); // Binary with some bits set
        $this->valkey_glide->set($key2, "\xF0\xF0"); // Binary with different bits set

        // Execute BITPOS, BITOP AND, BITCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
             ->bitpos($key1, 1)
             ->bitop('AND', $key3, $key1, $key2)
             ->bitcount($key3)
            ->exec();
        
        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertGTE(0, $results[0]); // BITPOS result (position of first set bit)
        $this->assertEquals(2, $results[1]); // BITOP result (length of result)
        $this->assertGTE(0, $results[2]); // BITCOUNT result

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->exists($key3)); // AND result key exists
        $this->assertGTE(0, $this->valkey_glide->bitcount($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // HASH ADVANCED OPERATIONS BATCH TESTS
    // ===================================================================

    public function testHashAdvancedOperationsBatch()
    {
        $key1 = 'batch_hash_adv_' . uniqid();

        // Setup initial hash
        $this->valkey_glide->hset($key1, 'field1', 'value1', 'field2', 'value2', 'field3', 'value3');

        // Execute HMGET, HKEYS, HVALS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hmget($key1, ['field1', 'field2', 'field_nonexistent'])
            ->hkeys($key1)
            ->hvals($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['field1' =>'value1', 'field2' =>'value2', 'field_nonexistent' =>false], $results[0]); // HMGET result
        $this->assertCount(3, $results[1]); // HKEYS result
        $this->assertContains('field1', $results[1]);
        $this->assertCount(3, $results[2]); // HVALS result
        $this->assertContains('value1', $results[2]);

        // Verify server-side effects
        $keys = $this->valkey_glide->hkeys($key1);
        $this->assertCount(3, $keys);
        $vals = $this->valkey_glide->hvals($key1);
        $this->assertCount(3, $vals);

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testHashSetNxBatch()
    {
        $key1 = 'batch_hash_setnx_' . uniqid();

        // Setup initial hash
        $this->valkey_glide->hset($key1, 'existing_field', 'existing_value');

        // Execute HSETNX, HSETNX, HSTRLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hsetnx($key1, 'new_field', 'new_value')
            ->hsetnx($key1, 'existing_field', 'should_not_overwrite')
            ->hstrlen($key1, 'existing_field')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // HSETNX result (new field set)
        $this->assertFalse($results[1]); // HSETNX result (existing field not overwritten)
        $this->assertEquals(14, $results[2]); // HSTRLEN result (length of "existing_value")

        // Verify server-side effects
        $this->assertEquals('new_value', $this->valkey_glide->hget($key1, 'new_field'));
        $this->assertEquals('existing_value', $this->valkey_glide->hget($key1, 'existing_field')); // Not overwritten
        $this->assertEquals(14, $this->valkey_glide->hstrlen($key1, 'existing_field'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testHashMsetBatch()
    {
        $key1 = 'batch_hash_mset_' . uniqid();

        // Execute HMSET, HGETALL, HLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hmset($key1, ['field1' => 'value1', 'field2' => 'value2', 'field3' => 'value3'])
            ->hgetall($key1)
            ->hlen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // HMSET result (success)
        $this->assertEquals(['field1' => 'value1', 'field2' => 'value2', 'field3' => 'value3'], $results[1]); // HGETALL result
        $this->assertEquals(3, $results[2]); // HLEN result

        // Verify server-side effects
        $this->assertEquals('value1', $this->valkey_glide->hget($key1, 'field1'));
        $this->assertEquals('value2', $this->valkey_glide->hget($key1, 'field2'));
        $this->assertEquals('value3', $this->valkey_glide->hget($key1, 'field3'));
        $this->assertEquals(3, $this->valkey_glide->hlen($key1));
        $allFields = $this->valkey_glide->hgetall($key1);
        $this->assertEquals(['field1' => 'value1', 'field2' => 'value2', 'field3' => 'value3'], $allFields);

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testHashRandfieldBatch()
    {
        $key1 = 'batch_hash_randfield_' . uniqid();

        // Setup initial hash with multiple fields
        $this->valkey_glide->hset($key1, 'alpha', 'value_a', 'beta', 'value_b', 'gamma', 'value_c', 'delta', 'value_d');

        // Execute HRANDFIELD (single), HRANDFIELD (multiple), HRANDFIELD (with values) in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hrandfield($key1) // Single random field
            ->hrandfield($key1, ['count' => 2]) // Multiple random fields
            ->hrandfield($key1, ['count' => 2, 'withvalues' => true]) // Random fields with values
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        
        // Single field result - should be a string (field name)
        $this->assertIsString($results[0]); // HRANDFIELD single result
        $this->assertContains($results[0], ['alpha', 'beta', 'gamma', 'delta']);
        
        // Multiple fields result - should be array of field names
        $this->assertIsArray($results[1]); // HRANDFIELD multiple result
        $this->assertCount(2, $results[1]);
        foreach ($results[1] as $field) {
            $this->assertContains($field, ['alpha', 'beta', 'gamma', 'delta']);
        }
        
        // Fields with values result - should be associative array
        $this->assertIsArray($results[2]); // HRANDFIELD with values result
        $this->assertCount(2, $results[2]);
        foreach ($results[2] as $field => $value) {
            $this->assertContains($field, ['alpha', 'beta', 'gamma', 'delta']);
            $expectedValue = $field === 'alpha' ? 'value_a' : 
                           ($field === 'beta' ? 'value_b' : 
                           ($field === 'gamma' ? 'value_c' : 'value_d'));
            $this->assertEquals($expectedValue, $value);
        }

        // Verify server-side effects (HRANDFIELD is read-only, so hash should be unchanged)
        $this->assertEquals(4, $this->valkey_glide->hlen($key1)); // Hash length unchanged
        $this->assertEquals('value_a', $this->valkey_glide->hget($key1, 'alpha')); // Fields unchanged
        $this->assertEquals('value_b', $this->valkey_glide->hget($key1, 'beta'));
        $this->assertEquals('value_c', $this->valkey_glide->hget($key1, 'gamma'));
        $this->assertEquals('value_d', $this->valkey_glide->hget($key1, 'delta'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // LIST ADVANCED OPERATIONS BATCH TESTS
    // ===================================================================

    public function testListAdvancedOperationsBatch()
    {        
        $key1 = 'batch_list_adv_' . uniqid();
        $key2 = 'batch_list_adv_2_' . uniqid();

        // Setup initial list
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c', 'b', 'a');

        // Execute LINDEX, LREM, LINSERT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lindex($key1, 2)
            ->lrem($key1,'b', 1) // Remove first occurrence of 'b'
            ->linsert($key1, 'BEFORE', 'c', 'inserted')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals('c', $results[0]); // LINDEX result
        $this->assertEquals(1, $results[1]); // LREM result (1 element removed)
        $this->assertEquals(5, $results[2]); // LINSERT result (new length)

        // Verify server-side effects
        $finalList = $this->valkey_glide->lrange($key1, 0, -1);
        $this->assertContains('inserted', $finalList);
        $this->assertContains('c', $finalList);

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    public function testListPositionBatch()
    {
     
        $key1 = 'batch_list_pos_' . uniqid();

        // Setup initial list
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c', 'b', 'a');

        // Execute LPOS, LSET, LPOS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lpos($key1, 'b')
            ->lset($key1, 1, 'modified')
            ->lpos($key1, 'modified')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // LPOS result (first position of 'b')
        $this->assertTrue($results[1]); // LSET result
        $this->assertEquals(1, $results[2]); // LPOS result (position of 'modified')

        // Verify server-side effects
        $this->assertEquals('modified', $this->valkey_glide->lindex($key1, 1));
        $this->assertEquals(1, $this->valkey_glide->lpos($key1, 'modified'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testListMoveBatch()
    {
        
        $key1 = '{momtom}batch_list_move_src_' . uniqid();
        $key2 = '{momtom}batch_list_move_dst_' . uniqid();

        // Setup initial lists
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c');
        $this->valkey_glide->rpush($key2, 'x', 'y');

        // Execute LMOVE, LLEN, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lmove($key1, $key2, $this->getRightConstant(), $this->getLeftConstant())
            ->llen($key1)
            ->llen($key2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals('c', $results[0]); // LMOVE result (moved element)
        $this->assertEquals(2, $results[1]); // LLEN result (source list)
        $this->assertEquals(3, $results[2]); // LLEN result (destination list)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->llen($key1)); // Source list shortened
        $this->assertEquals(3, $this->valkey_glide->llen($key2)); // Destination list lengthened
        $destList = $this->valkey_glide->lrange($key2, 0, -1);
        $this->assertEquals('c', $destList[0]); // Moved element at the front

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    // ===================================================================
    // SET ADVANCED OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSetRandomOperationsBatch()
    {        
        $key1 = 'batch_set_rand_' . uniqid();

        // Setup initial set
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3', 'member4', 'member5');

        // Execute SPOP, SRANDMEMBER, SCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->spop($key1)
            ->srandmember($key1, 2) // Get 2 random members
            ->scard($key1)
            ->exec();
       
        // Verify transaction results
        $this->assertIsArray($results);
         
        $this->assertCount(3, $results);
        $this->assertNotNull($results[0]); // SPOP result (random removed member)
        $this->assertIsArray($results[1]); // SRANDMEMBER result
        $this->assertCount(2, $results[1]);
        $this->assertEquals(4, $results[2]); // SCARD result (one less after SPOP)

        // Verify server-side effects
        $this->assertEquals(4, $this->valkey_glide->scard($key1)); // One member removed
        $this->assertFalse($this->valkey_glide->sismember($key1, $results[0])); // Popped member no longer exists

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSetOperationsBatch()
    {
        
        $key1 = '{ppp}batch_set_ops_1_' . uniqid();
        $key2 = '{ppp}batch_set_ops_2_' . uniqid();
        $key3 = '{ppp}batch_set_ops_3_' . uniqid();

        // Setup initial sets
        $this->valkey_glide->sadd($key1, 'a', 'b', 'c');
        $this->valkey_glide->sadd($key2, 'b', 'c', 'd');

        // Execute SINTER, SUNION, SDIFF in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->sinter($key1, $key2)
            ->sunion($key1, $key2)
            ->sdiff($key1, $key2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertCount(2, $results[0]); // SINTER result (b, c)
        $this->assertContains('b', $results[0]);
        $this->assertContains('c', $results[0]);
        $this->assertCount(4, $results[1]); // SUNION result (a, b, c, d)
        $this->assertCount(1, $results[2]); // SDIFF result (a)
        $this->assertContains('a', $results[2]);

        // Verify server-side effects (original sets unchanged)
        $this->assertEquals(3, $this->valkey_glide->scard($key1));
        $this->assertEquals(3, $this->valkey_glide->scard($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testSetStoreBatch()
    {        
        
        $key1 = '{prefix}batch_set_store_1_' . uniqid();
        $key2 = '{prefix}batch_set_store_2_' . uniqid();
        $key3 = '{prefix}batch_set_store_3_' . uniqid();
        $key4 = '{prefix}batch_set_store_4_' . uniqid();
        $key5 = '{prefix}batch_set_store_5_' . uniqid();

        // Setup initial sets
        $this->valkey_glide->sadd($key1, 'a', 'b', 'c');
        $this->valkey_glide->sadd($key2, 'b', 'c', 'd');

        // Execute SINTERSTORE, SUNIONSTORE, SDIFFSTORE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->sinterstore($key3, $key1, $key2)
            ->sunionstore($key4, $key1, $key2)
            ->sdiffstore($key5, $key1, $key2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // SINTERSTORE result (size of intersection)
        $this->assertEquals(4, $results[1]); // SUNIONSTORE result (size of union)
        $this->assertEquals(1, $results[2]); // SDIFFSTORE result (size of difference)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->scard($key3)); // Intersection stored
        $this->assertEquals(4, $this->valkey_glide->scard($key4)); // Union stored
        $this->assertEquals(1, $this->valkey_glide->scard($key5)); // Difference stored

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3, $key4, $key5);
    }

    // ===================================================================
    // SORTED SET ADVANCED OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSortedSetRankBatch()
    {
        $key1 = 'batch_zset_rank_' . uniqid();

        // Setup initial sorted set
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c', 4, 'd');

        // Execute ZRANK, ZREVRANK, ZCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zrank($key1, 'c')
            ->zrevrank($key1, 'c')
            ->zcount($key1, 1, 3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // ZRANK result (0-based rank)
        $this->assertEquals(1, $results[1]); // ZREVRANK result (reverse rank)
        $this->assertEquals(3, $results[2]); // ZCOUNT result (elements in score range)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->zrank($key1, 'c'));
        $this->assertEquals(1, $this->valkey_glide->zrevrank($key1, 'c'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSortedSetPopBatch()
    {
        
        $key1 = 'batch_zset_pop_' . uniqid();

        // Setup initial sorted set
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e');

        // Execute ZPOPMIN, ZPOPMAX, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zpopmin($key1)
            ->zpopmax($key1)
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);        
        $this->assertEquals(['a' => 1.0], $results[0]); // ZPOPMIN result        
        $this->assertEquals(['e' => 5.0], $results[1]); // ZPOPMAX result
        $this->assertEquals(3, $results[2]); // ZCARD result (after pops)

        // Verify server-side effects
        $this->assertEquals(3, $this->valkey_glide->zcard($key1)); // 3 elements remaining
        $this->assertEquals(false, $this->valkey_glide->zscore($key1, 'a')); // Min popped
        $this->assertEquals(false, $this->valkey_glide->zscore($key1, 'e')); // Max popped

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSortedSetRemoveRankBatch()
    {
        $key1 = 'batch_zset_remrank_' . uniqid();

        // Setup initial sorted set
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e');

        // Execute ZREMRANGEBYRANK, ZREMRANGEBYLEX, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zremrangebyrank($key1, 0, 1) // Remove first 2 elements (by rank)
            ->zremrangebylex($key1, '[d', '[e') // Remove elements d-e (by lex)
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // ZREMRANGEBYRANK result
        $this->assertEquals(2, $results[1]); // ZREMRANGEBYLEX result
        $this->assertEquals(1, $results[2]); // ZCARD result (only 'c' remains)

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->zcard($key1)); // Only 1 element remains
        $remaining = $this->valkey_glide->zrange($key1, 0, -1);
        $this->assertEquals(['c'], $remaining);

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // EXPIRATION TIME OPERATIONS BATCH TESTS
    // ===================================================================

    public function testExpirationTimeBatch()
    {
        
        $key1 = '{fox}batch_exptime_1_' . uniqid();
        $key2 = '{fox}batch_exptime_2_' . uniqid();
        $key3 = '{fox}batch_exptime_3_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->setex($key2, 3600, 'value2');
        $this->valkey_glide->set($key3, 'value3');
        $this->valkey_glide->pexpire($key3, 3600000);

        // Execute EXPIRETIME, PEXPIRETIME, PEXPIREAT in multi/exec batch
        $expireTimestamp = time() + 7200; // 2 hours from now
        $results = $this->valkey_glide->multi()
            ->expiretime($key1) // Should be -1 (no expiration)
            ->pexpiretime($key2) // Should be positive (has expiration)
            ->pexpireat($key3, $expireTimestamp * 1000) // Set expiration in ms
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(-1, $results[0]); // EXPIRETIME result (no expiration)
        $this->assertGT(0, $results[1]); // PEXPIRETIME result (has expiration)
        $this->assertTrue($results[2]); // PEXPIREAT result (success)

        // Verify server-side effects
        $this->assertEquals(-1, $this->valkey_glide->expiretime($key1)); // No expiration
        $this->assertGT(0, $this->valkey_glide->pexpiretime($key2)); // Has expiration
        $this->assertGT(0, $this->valkey_glide->expiretime($key3)); // New expiration set

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // ADVANCED SERVER OPERATIONS BATCH TESTS
    // ===================================================================

    public function testConfigOperationsBatch()
    {
        // Execute CONFIG GET, CONFIG SET, CONFIG RESETSTAT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->config('get', 'maxmemory')
            ->config('set', 'timeout', '300')
            ->config('resetstat')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // CONFIG GET result
        $this->assertTrue($results[1]); // CONFIG SET result
        $this->assertTrue($results[2]); // CONFIG RESETSTAT result

        // No cleanup needed for config operations
    }

    public function testFlushOperationsBatch()
    {
        $key1 = 'batch_flush_1_' . uniqid();
        $key2 = 'batch_flush_2_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'value1');
        $this->valkey_glide->set($key2, 'value2');

        // Execute FLUSHDB, DBSIZE, FLUSHALL in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->flushdb()
            ->dbsize()
            ->flushall()
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // FLUSHDB result
        $this->assertEquals(0, $results[1]); // DBSIZE result (after flush)
        $this->assertTrue($results[2]); // FLUSHALL result

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->dbsize()); // Database is empty

        // No cleanup needed as database was flushed
    }

    public function testWaitBatch()
    {
        $key1 = 'batch_wait_' . uniqid();

        // Execute SET, WAIT, EXISTS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->set($key1, 'test_value')
            ->wait(1, 1000) // Wait for 1 replica, max 1000ms
            ->exists($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // SET result
        $this->assertIsInt($results[1]); // WAIT result (number of replicas)
        $this->assertEquals(1, $results[2]); // EXISTS result

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->exists($key1));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // GEOSPATIAL OPERATIONS BATCH TESTS
    // ===================================================================

    public function testGeospatialOperationsBatch()
    {
        
        $key1 = 'batch_geo_' . uniqid();

        // Execute GEOADD, GEOPOS, GEODIST in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->geoadd($key1, -122.27652, 37.805186, 'Golden Gate Bridge', -122.2674626, 37.8062344, 'Crissy Field')
            ->geopos($key1, 'Golden Gate Bridge')
            ->geodist($key1, 'Golden Gate Bridge', 'Crissy Field', 'km')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // GEOADD result (2 locations added)
        $this->assertIsArray($results[1]); // GEOPOS result
        $this->assertIsArray($results[1][0]); // Position coordinates
        $this->assertGT(0, (float)$results[2]); // GEODIST result (distance in km)

        // Verify server-side effects
        $pos = $this->valkey_glide->geopos($key1, 'Golden Gate Bridge');
        $this->assertIsArray($pos[0]);
        $this->assertCount(2, $pos[0]); // Longitude and latitude

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testGeospatialAdvancedBatch()
    {
        
        $key1 = '{geotest}batch_geo_adv_' . uniqid();

        // Setup initial geo data
        $this->valkey_glide->geoadd(
            $key1,
            -122.27652,
            37.805186,
            'Golden Gate Bridge',
            -122.2674626,
            37.8062344,
            'Crissy Field',
            -122.258814,
            37.827429,
            'Lombard Street'
        );

        // Execute GEOHASH, GEOSEARCH, GEOSEARCHSTORE in multi/exec batch
        $storeKey = '{geotest}batch_geo_store_' . uniqid();
        $results = $this->valkey_glide->multi()
            ->geohash($key1, 'Golden Gate Bridge')
            ->geosearch($key1, [-122.27652, 37.805186], 5, 'km')
            ->geosearchstore($storeKey, $key1, [-122.27652, 37.805186], 10, 'km')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // GEOHASH result        
        $this->assertEquals(["Golden Gate Bridge", "Crissy Field", "Lombard Street"], $results[1]); // GEOSEARCH result
        $this->assertEquals(3, $results[2]); // GEOSEARCHSTORE result        
        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->exists($storeKey)); // Store key created

        // Cleanup
        $this->valkey_glide->del($key1, $storeKey);
    }

    // ===================================================================
    // SCAN OPERATIONS BATCH TESTS
    // ===================================================================

    public function testScanOperationsBatch()
    {        
        $key1 = '{scantest}batch_scan_set_' . uniqid();
        $key2 = '{scantest}batch_scan_hash_' . uniqid();
        $key3 = '{scantest}batch_scan_zset_' . uniqid();

        // Setup test data
        $this->valkey_glide->del($key1, $key2, $key3);
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3');
        $this->valkey_glide->hset($key2, 'field1', 'value1', 'field2', 'value2');
        $this->valkey_glide->zadd($key3, 1, 'zmember1', 2, 'zmember2');

        // Execute SCAN, SSCAN, HSCAN in multi/exec batch
        $it = null;
        $sscan_it = null;
        $hscan_it = null;

        $results = $this->valkey_glide->multi()
            ->scan($it)
            ->sscan($key1, $sscan_it)
            ->hscan($key2, $hscan_it)
            ->exec();
        
        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // SCAN result [cursor, keys]      
        $temp_it = null;    
        $this->assertEquals($this->valkey_glide->scan($temp_it), $results[0]);
        $this->assertIsArray($results[1]); // SSCAN result [cursor, members]
        $sscan_it = null;
        $this->assertEquals($results[1],$this->valkey_glide->sscan($key1, $sscan_it));        
        $this->assertIsArray($results[2]); // HSCAN result [cursor, fields_values]        
        $hscan_it = null;
        $this->assertEquals($results[2],$this->valkey_glide->hscan($key2, $hscan_it));
        // Verify server-side effects (scan operations don't modify data)
        $this->assertEquals(3, $this->valkey_glide->scard($key1));
        $this->assertEquals(2, $this->valkey_glide->hlen($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testZscanBatch()
    {
        
        $key1 = 'batch_zscan_' . uniqid();

        // Setup test data
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e');

        // Execute ZSCAN, ZLEXCOUNT, ZRANDMEMBER in multi/exec batch
        $it = null;
        $results = $this->valkey_glide->multi()
            ->zscan($key1, $it)
            ->zlexcount($key1, '-', '+')
            ->zrandmember($key1, ['count' => 2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // ZSCAN result [cursor, members_scores]
        $this->assertCount(5, $results[0]);
        $this->assertEquals(5, $results[1]); // ZLEXCOUNT result
        $this->assertIsArray($results[2]); // ZRANDMEMBER result
        $this->assertCount(2, $results[2]);

        // Verify server-side effects (no modifications)
        $this->assertEquals(5, $this->valkey_glide->zcard($key1));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // SORT OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSortOperationsBatch()
    {
        $key1 = '{sorttest}batch_sort_list_' . uniqid();
        $key2 = '{sorttest}batch_sort_store_' . uniqid();

        // Setup test data
        $this->valkey_glide->lpush($key1, '3', '1', '2', '5', '4');

        // Execute SORT, SORT_RO, SORT with STORE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->sort($key1)
            ->sort_ro($key1, ['sort' => 'DESC'])
            ->sort($key1, ['sort' => 'ASC', 'STORE'=> $key2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['1', '2', '3', '4', '5'], $results[0]); // SORT result
        $this->assertEquals(['5', '4', '3', '2', '1'], $results[1]); // SORT_RO result   
        $this->assertEquals(5, $results[2]); // SORT with STORE result (count of stored elements)
        
        // Verify server-side effects
        $this->assertEquals(5, $this->valkey_glide->llen($key2)); // Sorted list stored
        $storedList = $this->valkey_glide->lrange($key2, 0, -1);
        $this->assertEquals(['1', '2', '3', '4', '5'], $storedList);

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    // ===================================================================
    // COPY & DUMP/RESTORE OPERATIONS BATCH TESTS
    // ===================================================================

    public function testCopyDumpRestoreBatch()
    {
        
        $key1 = '{test}batch_copy_src_' . uniqid();
        $key2 = '{test}batch_copy_dst_' . uniqid();
        $key3 = '{test}batch_restore_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'test_value');
        $res_key1 = $this->valkey_glide->dump($key1);
        // Execute COPY, DUMP, RESTORE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->copy($key1, $key2)
            ->dump($key1)
            ->restore($key3, 0, $res_key1) 
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // COPY result (success)
        $this->assertIsString($results[1]); // DUMP result (serialized data)
        // RESTORE result depends on having valid dump data
        
        // Verify server-side effects
        $this->assertEquals('test_value', $this->valkey_glide->get($key2)); // Copied value

        // Now properly restore using the dump data
        $this->assertEquals('test_value', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // MOVE OPERATIONS BATCH TESTS
    // ===================================================================

    public function testMoveBatch()
    {        
        //TODO return once select is supported
        $this->markTestSkipped();
        $key1 = 'batch_move_' . uniqid();

        // Setup test data in database 0
        $this->valkey_glide->select(0);
        $this->valkey_glide->set($key1, 'move_test_value');

        // Execute MOVE, SELECT, EXISTS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->move($key1, 1) // Move to database 1
            ->select(1)
            ->exists($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // MOVE result (success)
        $this->assertTrue($results[1]); // SELECT result
        $this->assertEquals(1, $results[2]); // EXISTS result (key exists in db 1)

        // Verify server-side effects
        $this->valkey_glide->select(0);
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Key no longer in db 0
        $this->valkey_glide->select(1);
        $this->assertEquals('move_test_value', $this->valkey_glide->get($key1)); // Key in db 1

        // Cleanup
        $this->valkey_glide->del($key1);
        $this->valkey_glide->select(0); // Return to db 0
    }

    // ===================================================================
    // GETDEL/GETEX OPERATIONS BATCH TESTS
    // ===================================================================

    public function testGetDelExBatch()
    {
        
        $key1 = '{GD}batch_getdel_' . uniqid();
        $key2 = '{GD}batch_getex_' . uniqid();
        $key3 = '{GD}batch_getex2_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'delete_me');
        $this->valkey_glide->set($key2, 'expire_me');
        $this->valkey_glide->set($key3, 'persist_me');

        // Execute GETDEL, GETEX with expiration, GETEX with persist in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->getdel($key1)
            ->getex($key2, ['EX' => 3600]) // Set expiration
            ->getex($key3, ['PERSIST' => true]) // Remove expiration
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals('delete_me', $results[0]); // GETDEL result
        $this->assertEquals('expire_me', $results[1]); // GETEX result
        $this->assertEquals('persist_me', $results[2]); // GETEX result

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Key deleted
        $this->assertGT(0, $this->valkey_glide->ttl($key2)); // Key has expiration
        $this->assertEquals(-1, $this->valkey_glide->ttl($key3)); // Key has no expiration

        // Cleanup
        $this->valkey_glide->del($key2, $key3);
    }

    // ===================================================================
    // OBJECT OPERATIONS BATCH TESTS
    // ===================================================================

    public function testObjectOperationsBatch()
    {
        
        $key1 = 'batch_object_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'test_object_value');

        // Execute OBJECT ENCODING, OBJECT IDLETIME, OBJECT REFCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->object('encoding', $key1)
            ->object('idletime', $key1)
            ->object('refcount', $key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsString($results[0]); // OBJECT ENCODING result
        $this->assertIsInt($results[1]); // OBJECT IDLETIME result
        $this->assertIsInt($results[2]); // OBJECT REFCOUNT result
        $this->assertGTE(0, $results[1]); // Idle time >= 0
        $this->assertGTE(1, $results[2]); // Ref count >= 1

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // SET MEMBERSHIP OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSetMembershipBatch()
    {
        
        $key1 = '{bb}batch_smember_' . uniqid();
        $key2 = '{bb}batch_smove_src_' . uniqid();
        $key3 = '{bb}batch_smove_dst_' . uniqid();

        // Setup test data
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3');
        $this->valkey_glide->sadd($key2, 'move_me', 'stay_here');
        $this->valkey_glide->sadd($key3, 'already_here');

        // Execute SMISMEMBER, SMOVE, SINTERCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->smismember($key1, 'member1', 'member2', 'nonexistent')
            ->smove($key2, $key3, 'move_me')
            ->sintercard([$key1, $key2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([true, true, false], $results[0]); // SMISMEMBER result
        $this->assertTrue($results[1]); // SMOVE result (success)
        $this->assertIsInt($results[2]); // SINTERCARD result

        // Verify server-side effects
        $this->assertFalse($this->valkey_glide->sismember($key2, 'move_me')); // Moved from src
        $this->assertTrue($this->valkey_glide->sismember($key3, 'move_me')); // Moved to dst

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    protected function sequence($mode)
    {

        $ret = $this->valkey_glide->multi($mode)
            ->set('x', 42)
            ->type('x')
            ->get('x')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertTrue($ret[$i++]);
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_STRING, $ret[$i++]);
        $this->assertEqualsWeak('42', $ret[$i]);

        $ret = $this->valkey_glide->multi($mode)
            ->del('{key}1')
            ->set('{key}1', 'value1')
            ->get('{key}1')
            ->getSet('{key}1', 'value2')
            ->get('{key}1')
            ->set('{key}2', 4)
            ->incr('{key}2')
            ->get('{key}2')
            ->decr('{key}2')
            ->get('{key}2')
            ->rename('{key}2', '{key}3')
            ->get('{key}3')
            ->renameNx('{key}3', '{key}1')
            ->rename('{key}3', '{key}2')
            ->incrby('{key}2', 5)
            ->get('{key}2')
            ->decrby('{key}2', 5)
            ->get('{key}2')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++]));
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value2', $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(false, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(9, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEquals($i, count($ret));


        $ret = $this->valkey_glide->multi($mode)
            ->del('{key}1')
            ->del('{key}2')
            ->set('{key}1', 'val1')
            ->setnx('{key}1', 'valX')
            ->setnx('{key}2', 'valX')
            ->exists('{key}1')
            ->exists('{key}3')
            ->exec();

        $this->assertIsArray($ret);
        $this->assertEqualsWeak(true, $ret[0]);
        $this->assertEqualsWeak(true, $ret[1]);
        $this->assertEqualsWeak(true, $ret[2]);
        $this->assertEqualsWeak(false, $ret[3]);
        $this->assertEqualsWeak(true, $ret[4]);
        $this->assertEqualsWeak(true, $ret[5]);
        $this->assertEqualsWeak(false, $ret[6]);

        // ttl, mget, mset, msetnx, expire, expireAt
        $this->valkey_glide->del('key');
        $ret = $this->valkey_glide->multi($mode)
            ->ttl('key')
            ->mget(['{key}1', '{key}2', '{key}3'])
            ->mset(['{key}3' => 'value3', '{key}4' => 'value4'])
            ->set('key', 'value')
            ->expire('key', 5)
            ->ttl('key')
            ->expireAt('key', '0000')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $ttl = $ret[$i++];
        $this->assertBetween($ttl, -2, -1);
        $this->assertEquals(['val1', 'valX', false], $ret[$i++]); // mget
        $this->assertTrue($ret[$i++]); // mset
        $this->assertTrue($ret[$i++]); // set
        $this->assertTrue($ret[$i++]); // expire
        $this->assertEquals(5, $ret[$i++]);    // ttl
        $this->assertTrue($ret[$i++]); // expireAt
        $this->assertEquals($i, count($ret));
        $ret = $this->valkey_glide->multi($mode)
            ->set('{list}lkey', 'x')
            ->set('{list}lDest', 'y')
            ->del('{list}lkey', '{list}lDest')
            ->rpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lmove('{list}lkey', '{list}lDest', $this->getRightConstant(), $this->getLeftConstant())   //->rpoplpush('{list}lkey', '{list}lDest')
            ->lrange('{list}lDest', 0, -1)
            ->lpop('{list}lkey')
            ->llen('{list}lkey')
            ->lrem('{list}lkey', 'lvalue', 3)
            ->llen('{list}lkey')
            ->lIndex('{list}lkey', 0)
            ->lrange('{list}lkey', 0, -1)
            ->lSet('{list}lkey', 1, 'newValue')    // check errors on key not exists
            ->lrange('{list}lkey', 0, -1)
            ->llen('{list}lkey')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertTrue($ret[$i++]); // SET
        $this->assertTrue($ret[$i++]); // SET
        $this->assertEquals(2, $ret[$i++]); // deleting 2 keys
        $this->assertEquals(1, $ret[$i++]); // rpush, now 1 element
        $this->assertEquals(2, $ret[$i++]); // lpush, now 2 elements
        $this->assertEquals(3, $ret[$i++]); // lpush, now 3 elements
        $this->assertEquals(4, $ret[$i++]); // lpush, now 4 elements
        $this->assertEquals(5, $ret[$i++]); // lpush, now 5 elements
        $this->assertEquals(6, $ret[$i++]); // lpush, now 6 elements
        $this->assertEquals('lvalue', $ret[$i++]); // lmove returns the element: 'lvalue'
        $this->assertEquals(['lvalue'], $ret[$i++]); // lDest contains only that one element.
        $this->assertEquals('lvalue', $ret[$i++]); // removing a second element from lkey, now 4 elements left ↓
        $this->assertEquals(4, $ret[$i++]); // 4 elements left, after 2 pops.
        $this->assertEquals(3, $ret[$i++]); // removing 3 elements, now 1 left.
        $this->assertEquals(1, $ret[$i++]); // 1 element left
        $this->assertEquals('lvalue', $ret[$i++]); // this is the current head.
        $this->assertEquals(['lvalue'], $ret[$i++]); // this is the current list.
        $this->assertFalse($ret[$i++]); // updating a non-existent element fails.
        $this->assertEquals(['lvalue'], $ret[$i++]); // this is the current list.
        $this->assertEquals(1, $ret[$i++]); // 1 element left
        $this->assertEquals($i, count($ret));

        $ret = $this->valkey_glide->multi($mode)
            ->del('{list}lkey', '{list}lDest')
            ->rpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lmove('{list}lkey', '{list}lDest', $this->getRightConstant(), $this->getLeftConstant())//->rpoplpush('{list}lkey', '{list}lDest')
            ->lrange('{list}lDest', 0, -1)
            ->lpop('{list}lkey')
            ->exec();
        $this->assertIsArray($ret);

        $i = 0;

        $this->assertLTE(2, $ret[$i++]);      // deleting 2 keys
        $this->assertEquals(1, $ret[$i++]); // 1 element in the list
        $this->assertEquals(2, $ret[$i++]); // 2 elements in the list
        $this->assertEquals(3, $ret[$i++]); // 3 elements in the list
        $this->assertEquals('lvalue', $ret[$i++]); // rpoplpush returns the element: 'lvalue'
        $this->assertEquals(['lvalue'], $ret[$i++]); // rpoplpush returns the element: 'lvalue'
        $this->assertEquals('lvalue', $ret[$i++]); // pop returns the front element: 'lvalue'
        $this->assertEquals($i, count($ret));       
        
        $ret = $this->valkey_glide->multi($mode)
            ->del('{key}1')
            ->set('{key}1', 'value1')
            ->get('{key}1')
            ->getSet('{key}1', 'value2')
            ->get('{key}1')
            ->set('{key}2', 4)
            ->incr('{key}2')
            ->get('{key}2')
            ->decr('{key}2')
            ->get('{key}2')
            ->rename('{key}2', '{key}3')
            ->get('{key}3')
            ->renameNx('{key}3', '{key}1')
            ->rename('{key}3', '{key}2')
            ->incrby('{key}2', 5)
            ->get('{key}2')
            ->decrby('{key}2', 5)
            ->get('{key}2')
            ->set('{key}3', 'value3')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertLTE(1, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEquals('value1', $ret[$i++]);
        $this->assertEquals('value1', $ret[$i++]);
        $this->assertEquals('value2', $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertTrue($ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertFalse($ret[$i++]);
        $this->assertTrue($ret[$i++]);
        $this->assertEquals(9, $ret[$i++]);          // incrby('{key}2', 5)
        $this->assertEqualsWeak(9, $ret[$i++]);      // get('{key}2')
        $this->assertEquals(4, $ret[$i++]);          // decrby('{key}2', 5)
        $this->assertEqualsWeak(4, $ret[$i++]);      // get('{key}2')
        $this->assertTrue($ret[$i++]);
        

        $ret = $this->valkey_glide->multi($mode)
            ->del('{key}1')
            ->del('{key}2')
            ->del('{key}3')
            ->set('{key}1', 'val1')
            ->setnx('{key}1', 'valX')
            ->setnx('{key}2', 'valX')
            ->exists('{key}1')
            ->exists('{key}3')
            ->exec();

        $this->assertIsArray($ret);
        $this->assertEquals(1, $ret[0]); // del('{key}1')
        $this->assertEquals(1, $ret[1]); // del('{key}2')
        $this->assertEquals(1, $ret[2]); // del('{key}3')
        $this->assertTrue($ret[3]);      // set('{key}1', 'val1')
        $this->assertFalse($ret[4]);     // setnx('{key}1', 'valX')
        $this->assertTrue($ret[5]);      // setnx('{key}2', 'valX')
        $this->assertEquals(1, $ret[6]); // exists('{key}1')
        $this->assertEquals(0, $ret[7]); // exists('{key}3')

        // ttl, mget, mset, msetnx, expire, expireAt
        $ret = $this->valkey_glide->multi($mode)
            ->ttl('key')
            ->mget(['{key}1', '{key}2', '{key}3'])
            ->mset(['{key}3' => 'value3', '{key}4' => 'value4'])
            ->set('key', 'value')
            ->expire('key', 5)
            ->ttl('key')
            ->expireAt('key', '0000')
            ->exec();
        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++]));
        $this->assertIsArray($ret[$i++], 3);
//        $i++;
        $this->assertTrue($ret[$i++]); // mset always returns true
        $this->assertTrue($ret[$i++]); // set always returns true
        $this->assertTrue($ret[$i++]); // expire always returns true
        $this->assertEquals(5, $ret[$i++]); // TTL was just set.
        $this->assertTrue($ret[$i++]); // expireAt returns true for an existing key
        $this->assertEquals($i, count($ret));

        // lists
        $ret = $this->valkey_glide->multi($mode)
            ->del('{l}key', '{l}Dest')
            ->rpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lmove('{l}key', '{l}Dest', $this->getRightConstant(), $this->getLeftConstant())//->rpoplpush('{l}key', '{l}Dest')
            ->lrange('{l}Dest', 0, -1)
            ->lpop('{l}key')
            ->llen('{l}key')
            ->lrem('{l}key', 'lvalue', 3)
            ->llen('{l}key')
            ->lIndex('{l}key', 0)
            ->lrange('{l}key', 0, -1)
            ->lSet('{l}key', 1, 'newValue')    // check errors on missing key
            ->lrange('{l}key', 0, -1)
            ->llen('{l}key')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertBetween($ret[$i++], 0, 2); // del
        $this->assertEquals(1, $ret[$i++]); // 1 value
        $this->assertEquals(2, $ret[$i++]); // 2 values
        $this->assertEquals(3, $ret[$i++]); // 3 values
        $this->assertEquals(4, $ret[$i++]); // 4 values
        $this->assertEquals(5, $ret[$i++]); // 5 values
        $this->assertEquals(6, $ret[$i++]); // 6 values
        $this->assertEquals('lvalue', $ret[$i++]);
        $this->assertEquals(['lvalue'], $ret[$i++]); // 1 value only in lDest
        $this->assertEquals('lvalue', $ret[$i++]); // now 4 values left
        $this->assertEquals(4, $ret[$i++]);
        $this->assertEquals(3, $ret[$i++]); // removing 3 elements.
        $this->assertEquals(1, $ret[$i++]); // length is now 1
        $this->assertEquals('lvalue', $ret[$i++]); // this is the head
        $this->assertEquals(['lvalue'], $ret[$i++]); // 1 value only in lkey
        $this->assertFalse($ret[$i++]); // can't set list[1] if we only have a single value in it.
        $this->assertEquals(['lvalue'], $ret[$i++]); // the previous error didn't touch anything.
        $this->assertEquals(1, $ret[$i++]); // the previous error didn't change the length
        $this->assertEquals($i, count($ret));        

        // sets
        $ret = $this->valkey_glide->multi($mode)
            ->del('{s}key1', '{s}key2', '{s}keydest', '{s}keyUnion', '{s}DiffDest')
            ->sadd('{s}key1', 'sValue1')
            ->sadd('{s}key1', 'sValue2')
            ->sadd('{s}key1', 'sValue3')
            ->sadd('{s}key1', 'sValue4')
            ->sadd('{s}key2', 'sValue1')
            ->sadd('{s}key2', 'sValue2')
            ->scard('{s}key1')
            ->srem('{s}key1', 'sValue2')
            ->scard('{s}key1')
            ->sMove('{s}key1', '{s}key2', 'sValue4')
            ->scard('{s}key2')
            ->sismember('{s}key2', 'sValue4')
            ->sMembers('{s}key1')
            ->sMembers('{s}key2')
            ->sInter('{s}key1', '{s}key2')
            ->sInterStore('{s}keydest', '{s}key1', '{s}key2')
            ->sMembers('{s}keydest')
            ->sUnion('{s}key2', '{s}keydest')
            ->sUnionStore('{s}keyUnion', '{s}key2', '{s}keydest')
            ->sMembers('{s}keyUnion')
            ->sDiff('{s}key1', '{s}key2')
            ->sDiffStore('{s}DiffDest', '{s}key1', '{s}key2')
            ->sMembers('{s}DiffDest')
            ->sPop('{s}key2')
            ->scard('{s}key2')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertBetween($ret[$i++], 0, 5); // we deleted at most 5 values.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 1 element.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 2 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 3 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 4 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey2 now has 1 element.
        $this->assertEquals(1, $ret[$i++]);     // skey2 now has 2 elements.
        $this->assertEquals(4, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);     // we did remove that value.
        $this->assertEquals(3, $ret[$i++]);     // now 3 values only.

        $this->assertTrue($ret[$i++]); // the move did succeed.
        $this->assertEquals(3, $ret[$i++]); // sKey2 now has 3 values.
        $this->assertTrue($ret[$i++]); // sKey2 does contain sValue4.
        foreach (['sValue1', 'sValue3'] as $k) { // sKey1 contains sValue1 and sValue3.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(2, count($ret[$i++]));
        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // sKey2 contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++]));
        $this->assertEquals(['sValue1'], $ret[$i++]); // intersection
        $this->assertEquals(1, $ret[$i++]); // intersection + store → 1 value in the destination set.
        $this->assertEquals(['sValue1'], $ret[$i++]); // sinterstore destination contents

        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeydest U sKey2) contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++])); // union size

        $this->assertEquals(3, $ret[$i++]); // unionstore size
        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeyUnion) contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++])); // skeyUnion size

        $this->assertEquals(['sValue3'], $ret[$i++]); // diff skey1, skey2 : only sValue3 is not shared.
        $this->assertEquals(1, $ret[$i++]); // sdiffstore size == 1
        $this->assertEquals(['sValue3'], $ret[$i++]); // contents of sDiffDest

        $this->assertInArray($ret[$i++], ['sValue1', 'sValue2', 'sValue4']); // we removed an element from sKey2
        $this->assertEquals(2, $ret[$i++]); // sKey2 now has 2 elements only.

        $this->assertEquals($i, count($ret));

        // sorted sets
        $ret = $this->valkey_glide->multi($mode)
            ->del('{z}key1', '{z}key2', '{z}key5', '{z}Inter', '{z}Union')
            ->zadd('{z}key1', 1, 'zValue1')
            ->zadd('{z}key1', 5, 'zValue5')
            ->zadd('{z}key1', 2, 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zRem('{z}key1', 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zadd('{z}key1', 11, 'zValue11')
            ->zadd('{z}key1', 12, 'zValue12')
            ->zadd('{z}key1', 13, 'zValue13')
            ->zadd('{z}key1', 14, 'zValue14')
            ->zadd('{z}key1', 15, 'zValue15')
            ->zRemRangeByScore('{z}key1', 11, 13)
            ->zrange('{z}key1', 0, -1)
            ->zRange('{z}key1', 0, -1,['rev'])
            ->zRangeByScore('{z}key1', 1, 6)
            ->zCard('{z}key1')
            ->zScore('{z}key1', 'zValue15')
            ->zadd('{z}key2', 5, 'zValue5')
            ->zadd('{z}key2', 2, 'zValue2')
            ->zInterStore('{z}Inter', ['{z}key1', '{z}key2'])
            ->zRange('{z}key1', 0, -1)
            ->zRange('{z}key2', 0, -1)
            ->zRange('{z}Inter', 0, -1)
            ->zUnionStore('{z}Union', ['{z}key1', '{z}key2'])
            ->zRange('{z}Union', 0, -1)
            ->zadd('{z}key5', 5, 'zValue5')
            ->zIncrBy('{z}key5', 3, 'zValue5') // fix this
            ->zScore('{z}key5', 'zValue5')
            ->zScore('{z}key5', 'unknown')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertBetween($ret[$i++], 0, 5); // we deleted at most 5 values.
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);//zadd
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5'], $ret[$i++]);//zrange
        $this->assertEquals(1, $ret[$i++]);//zrem
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);//zrange
        $this->assertEquals(1, $ret[$i++]); // adding zValue11
        $this->assertEquals(1, $ret[$i++]); // adding zValue12
        $this->assertEquals(1, $ret[$i++]); // adding zValue13
        $this->assertEquals(1, $ret[$i++]); // adding zValue14
        $this->assertEquals(1, $ret[$i++]); // adding zValue15
        $this->assertEquals(3, $ret[$i++]); // deleted zValue11, zValue12, zValue13 //zRemRangeByScore
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]);
        $this->assertEquals(['zValue15', 'zValue14', 'zValue5', 'zValue1'], $ret[$i++]);//zRangeByScore
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);//zcard
        $this->assertEquals(4, $ret[$i++]); // 4 elements 
        $this->assertEquals(15.0, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // zinter only has 1 value
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}key1 contents
        $this->assertEquals(['zValue2', 'zValue5'], $ret[$i++]); // {z}key2 contents
        $this->assertEquals(['zValue5'], $ret[$i++]); // {z}inter contents
        $this->assertEquals(5, $ret[$i++]); // {z}Union has 5 values (1, 2, 5, 14, 15)
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}Union contents
        $this->assertEquals(1, $ret[$i++]); // added value to {z}key5, with score 5
        $this->assertEquals(8.0, $ret[$i++]); // incremented score by 3 → it is now 8.
        $this->assertEquals(8.0, $ret[$i++]); // current score is 8.
        $this->assertFalse($ret[$i++]); // score for unknown element.

        $this->assertEquals($i, count($ret));       

        // hash
        $ret = $this->valkey_glide->multi($mode)
            ->del('hkey1')
            ->hset('hkey1', 'key1', 'value1')
            ->hset('hkey1', 'key2', 'value2')
            ->hset('hkey1', 'key3', 'value3')
            ->hmget('hkey1', ['key1', 'key2', 'key3'])
            ->hget('hkey1', 'key1')
            ->hlen('hkey1')
            ->hdel('hkey1', 'key2')
            ->hdel('hkey1', 'key2')
            ->hexists('hkey1', 'key2')
            ->hkeys('hkey1')
            ->hvals('hkey1')
            ->hgetall('hkey1')
            ->hset('hkey1', 'valn', 1)
            ->hset('hkey1', 'val-fail', 'non-string')
            ->hget('hkey1', 'val-fail')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertLT(2, $ret[$i++]); // delete
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(['key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3'], $ret[$i++]); // hmget, 3 elements
        $this->assertEquals('value1', $ret[$i++]); // hget
        $this->assertEquals(3, $ret[$i++]); // hlen
        $this->assertEquals(1, $ret[$i++]); // hdel succeeded
        $this->assertEquals(0, $ret[$i++]); // hdel failed
        $this->assertFalse($ret[$i++]); // hexists didn't find the deleted key
        $this->assertEqualsCanonicalizing(['key1', 'key3'], $ret[$i++]); // hkeys
        $this->assertEqualsCanonicalizing(['value1', 'value3'], $ret[$i++]); // hvals
        $this->assertEqualsCanonicalizing(['key1' => 'value1', 'key3' => 'value3'], $ret[$i++]); // hgetall
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added the element, so 1.
        $this->assertEquals('non-string', $ret[$i++]); // hset succeeded
        $this->assertEquals($i, count($ret));

        $ret = $this->valkey_glide->multi($mode) // default to MULTI, not PIPELINE.
            ->del('test')
            ->set('test', 'xyz')
            ->get('test')
            ->exec();
        $i = 0;
        $this->assertIsArray($ret);
        $this->assertLTE(1, $ret[$i++]); // delete
        $this->assertTrue($ret[$i++]); // added 1 element
        $this->assertEquals('xyz', $ret[$i++]);
        $this->assertEquals($i, count($ret));

        // GitHub issue 78
        $this->valkey_glide->del('test');
        for ($i = 1; $i <= 5; $i++) {
            $this->valkey_glide->zadd('test', $i, (string)$i);
        }

        $result = $this->valkey_glide->multi($mode)
            ->zscore('test', '1')
            ->zscore('test', '6')
            ->zscore('test', '8')
            ->zscore('test', '2')
            ->exec();

        $this->assertEquals([1.0, false, false, 2.0], $result);
    }

    protected function differentType($mode)
    {
        // string
        $key = '{hash}string';
        $dkey = '{hash}' . __FUNCTION__;

        $ret = $this->valkey_glide->multi($mode)
            ->del($key)
            ->set($key, 'value')

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, 'newValue')
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->lMove($key, $dkey . 'lkey1', $this->getRightConstant(), $this->getLeftConstant())//  ->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')

            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')

            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRange($key, 0, -1, ['rev'])
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++]); // set
        
        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange         
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

        $this->assertEquals($i, count($ret));

        // list
        $key = '{hash}list';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->valkey_glide->multi($mode)
            ->del($key)
            ->lpush($key, 'lvalue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRange($key, 0, -1, ['rev'])
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // lpush

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

        $this->assertEquals($i, count($ret));

        // set
        $key = '{hash}set';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->valkey_glide->multi($mode)
            ->del($key)
            ->sAdd($key, 'sValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, 'newValue')
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->lMove($key, $dkey . 'lkey1', $this->getRightConstant(), $this->getLeftConstant())//->rPoplPush($key, $dkey . 'lkey1')

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRange($key, 0, -1, ['rev'])
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // zadd

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

        $this->assertEquals($i, count($ret));

        // sorted set
        $key = '{hash}sortedset';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->valkey_glide->multi($mode)
            ->del($key)
            ->zAdd($key, 0, 'zValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, 'newValue')
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->lMove($key, $dkey . 'lkey1', $this->getRightConstant(), $this->getLeftConstant())//->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // zadd

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

        $this->assertEquals($i, count($ret));

        // hash
        $key = '{hash}hash';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->valkey_glide->multi($mode)
            ->del($key)
            ->hset($key, 'key1', 'hValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, 'newValue')
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->lMove($key, $dkey . 'lkey1', $this->getRightConstant(), $this->getLeftConstant())//->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRange($key, 0, -1, ['rev'])
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // hset

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertEquals($i, count($ret));
    }

    public function testDifferentType()
    {        
        $this->differentType(ValkeyGlide::MULTI);        
        $this->differentType(ValkeyGlide::PIPELINE);
    }
    public function testMultiExec()
    {        
        $this->sequence(ValkeyGlide::MULTI);        
       
        $this->valkey_glide->set('x', '42');

        $this->assertTrue($this->valkey_glide->watch('x'));
        $ret = $this->valkey_glide->multi()->get('x')->exec();

        // successful transaction
        $this->assertEquals(['42'], $ret);
    }

    public function testFailedTransactions()
    {
        $this->valkey_glide->set('x', 42);

        // failed transaction
        $this->valkey_glide->watch('x');      
         
        $r = $this->newInstance(); // new instance, modifying `x'.
        $r->incr('x');

        $ret = $this->valkey_glide->multi()->get('x')->exec();
        $this->assertFalse($ret); // failed because another client changed our watched key between WATCH and EXEC.
               
        // watch and unwatch
        $this->valkey_glide->watch('x');
        $r->incr('x'); // other instance
        $this->valkey_glide->unwatch(); // cancel transaction watch

        $ret = $this->valkey_glide->multi()->get('x')->exec();

        // succeeded since we've cancel the WATCH command.
        $this->assertEquals(['44'], $ret);
    }

    public function testPipeline()
    {
        $this->sequence(ValkeyGlide::PIPELINE);
        $this->differentType(ValkeyGlide::PIPELINE);
    }


    public function testMultiZ()
    {
        $this->valkey_glide->del('{z}key1', '{z}key2', '{z}key5', '{z}Inter', '{z}Union');
        // sorted sets
        $ret = $this->valkey_glide->multi(ValkeyGlide::MULTI)

            ->zadd('{z}key1', 1, 'zValue1')
            ->zadd('{z}key1', 5, 'zValue5')
            ->zadd('{z}key1', 2, 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zRem('{z}key1', 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zadd('{z}key1', 11, 'zValue11')
            ->zadd('{z}key1', 12, 'zValue12')
            ->zadd('{z}key1', 13, 'zValue13')
            ->zadd('{z}key1', 14, 'zValue14')
            ->zadd('{z}key1', 15, 'zValue15')
            ->zRemRangeByScore('{z}key1', 11, 13)
            ->zrange('{z}key1', 0, -1)
            ->zRangeByScore('{z}key1', 1, 6)
            ->zCard('{z}key1')
            ->zScore('{z}key1', 'zValue15')
            ->zadd('{z}key2', 5, 'zValue5')
            ->zadd('{z}key2', 2, 'zValue2')
            ->zInterStore('{z}Inter', ['{z}key1', '{z}key2'])
            ->zRange('{z}key1', 0, -1)
            ->zRange('{z}key2', 0, -1)
            ->zRange('{z}Inter', 0, -1)
            ->zUnionStore('{z}Union', ['{z}key1', '{z}key2'])
            ->zRange('{z}Union', 0, -1)
            ->zadd('{z}key5', 5, 'zValue5')
            ->zIncrBy('{z}key5', 3, 'zValue5') // fix this
            ->zScore('{z}key5', 'zValue5')
            ->zScore('{z}key5', 'unknown')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // adding zValue11
        $this->assertEquals(1, $ret[$i++]); // adding zValue12
        $this->assertEquals(1, $ret[$i++]); // adding zValue13
        $this->assertEquals(1, $ret[$i++]); // adding zValue14
        $this->assertEquals(1, $ret[$i++]); // adding zValue15  ->zadd('{z}key1', 15, 'zValue15')
        $this->assertEquals(3, $ret[$i++]); // deleted zValue11, zValue12, zValue13
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);
        $this->assertEquals(4, $ret[$i++]); // 4 elements
        $this->assertEquals(15.0, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // zinter only has 1 value
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}key1 contents
        $this->assertEquals(['zValue2', 'zValue5'], $ret[$i++]); // {z}key2 contents
        $this->assertEquals(['zValue5'], $ret[$i++]); // {z}inter contents
        $this->assertEquals(5, $ret[$i++]); // {z}Union has 5 values (1, 2, 5, 14, 15)
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}Union contents
        $this->assertEquals(1, $ret[$i++]); // added value to {z}key5, with score 5
        $this->assertEquals(8.0, $ret[$i++]); // incremented score by 3 → it is now 8.
        $this->assertEquals(8.0, $ret[$i++]); // current score is 8.
        $this->assertFalse($ret[$i++]); // score for unknown element.

        $this->assertEquals($i, count($ret));
    }

    public function testMultiHash()
    {
        // hash
        $this->valkey_glide->del('hkey1');
        $ret = $this->valkey_glide->multi(ValkeyGlide::MULTI)
            ->hset('hkey1', 'key1', 'value1')
            ->hset('hkey1', 'key2', 'value2')
            ->hset('hkey1', 'key3', 'value3')
            ->hmget('hkey1', ['key1', 'key2', 'key3'])
            ->hget('hkey1', 'key1')
            ->hlen('hkey1')
            ->hdel('hkey1', 'key2')
            ->hdel('hkey1', 'key2')
            ->hexists('hkey1', 'key2')
            ->hkeys('hkey1')
            ->hvals('hkey1')
            ->hgetall('hkey1')
            ->hset('hkey1', 'valn', 1)
            ->hset('hkey1', 'val-fail', 'non-string')
            ->hget('hkey1', 'val-fail')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(['key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3'], $ret[$i++]); // hmget, 3 elements
        $this->assertEquals('value1', $ret[$i++]); // hget
        $this->assertEquals(3, $ret[$i++]); // hlen
        $this->assertEquals(1, $ret[$i++]); // hdel succeeded
        $this->assertEquals(0, $ret[$i++]); // hdel failed
        $this->assertFalse($ret[$i++]); // hexists didn't find the deleted key
        $this->assertEqualsCanonicalizing(['key1', 'key3'], $ret[$i++]); // hkeys
        $this->assertEqualsCanonicalizing(['value1', 'value3'], $ret[$i++]); // hvals
        $this->assertEqualsCanonicalizing(['key1' => 'value1', 'key3' => 'value3'], $ret[$i++]); // hgetall
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added the element, so 1.
        $this->assertEquals('non-string', $ret[$i++]); // hset succeeded
        $this->assertEquals($i, count($ret));
    }

    public function testMultiString()
    {
        $ret = $this->valkey_glide->multi(ValkeyGlide::MULTI)
            ->del('{key}1')
            ->set('{key}1', 'value1')
            ->get('{key}1')
            ->getSet('{key}1', 'value2')
            ->get('{key}1')
            ->set('{key}2', 4)
            ->incr('{key}2')
            ->get('{key}2')
            ->decr('{key}2')
            ->get('{key}2')
            ->rename('{key}2', '{key}3')
            ->get('{key}3')
            ->renameNx('{key}3', '{key}1')
            ->rename('{key}3', '{key}2')
            ->incrby('{key}2', 5)
            ->get('{key}2')
            ->decrby('{key}2', 5)
            ->get('{key}2')
            ->exec();

        $i = 0;
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++]));
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value2', $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(false, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(9, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEquals($i, count($ret));
    }

     // ===================================================================
    // LIST OPERATIONS BATCH TESTS
    // ===================================================================

    public function testListPushOperationsBatch()
    {
        
        $key1 = 'batch_list_1_' . uniqid();
        $key2 = 'batch_list_2_' . uniqid();
        $key3 = 'batch_list_3_' . uniqid();

        // Execute LPUSH, RPUSH, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lpush($key1, 'left1', 'left2')
            ->rpush($key1, 'right1', 'right2')
            ->llen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // LPUSH result (list length after push)
        $this->assertEquals(4, $results[1]); // RPUSH result (list length after push)
        $this->assertEquals(4, $results[2]); // LLEN result

        // Verify server-side effects
        $this->assertEquals(4, $this->valkey_glide->llen($key1));
        $listContents = $this->valkey_glide->lrange($key1, 0, -1);
        $this->assertEquals(['left2', 'left1', 'right1', 'right2'], $listContents);

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testListPopOperationsBatch()
    {
        
        $key1 = 'batch_list_pop_' . uniqid();

        // Setup initial list
        $this->valkey_glide->rpush($key1, 'item1', 'item2', 'item3', 'item4');

        // Execute LPOP, RPOP, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lpop($key1)
            ->rpop($key1)
            ->llen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals('item1', $results[0]); // LPOP result
        $this->assertEquals('item4', $results[1]); // RPOP result
        $this->assertEquals(2, $results[2]); 
        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->llen($key1)); // 2 items remaining
        $listContents = $this->valkey_glide->lrange($key1, 0, -1);
        $this->assertEquals(['item2', 'item3'], $listContents);

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testListRangeOperationsBatch()
    {
        
        $key1 = 'batch_list_range_' . uniqid();

        // Setup initial list
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c', 'd', 'e', 'f');

        // Execute LRANGE, LTRIM, LRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lrange($key1, 0, 2)
            ->ltrim($key1, 1, 4)
            ->lrange($key1, 0, -1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['a', 'b', 'c'], $results[0]); // LRANGE result (first 3)
        $this->assertTrue($results[1]); // LTRIM result
        $this->assertEquals(['b', 'c', 'd', 'e'], $results[2]); 

        // Verify server-side effects after transaction
        $finalContents = $this->valkey_glide->lrange($key1, 0, -1);
        $this->assertEquals(['b', 'c', 'd', 'e'], $finalContents); // After trim

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // SET OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSetAddOperationsBatch()
    {
        
        $key1 = 'batch_set_1_' . uniqid();
        $key2 = 'batch_set_2_' . uniqid();

        // Execute SADD, SCARD, SMEMBERS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->sadd($key1, 'member1', 'member2', 'member3')
            ->scard($key1)
            ->smembers($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(3, $results[0]); // SADD result (3 new members)
        $this->assertEquals(3, $results[1]); // SCARD result
        $this->assertCount(3, $results[2]); // SMEMBERS result
        $this->assertContains('member1', $results[2]);
        $this->assertContains('member2', $results[2]);
        $this->assertContains('member3', $results[2]);

        // Verify server-side effects
        $this->assertEquals(3, $this->valkey_glide->scard($key1));
        $this->assertTrue($this->valkey_glide->sismember($key1, 'member1'));

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    public function testSetRemoveOperationsBatch()
    {
        
        $key1 = 'batch_set_rem_' . uniqid();

        // Setup initial set
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3', 'member4');

        // Execute SREM, SISMEMBER, SCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->srem($key1, 'member2', 'member3')
            ->sismember($key1, 'member1')
            ->scard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // SREM result (2 members removed)
        $this->assertTrue($results[1]); // SISMEMBER result (member exists)
        $this->assertEquals(2, $results[2]); // SCARD result (after removal in transaction)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->scard($key1)); // 2 members remaining
        $this->assertTrue($this->valkey_glide->sismember($key1, 'member1')); // member1 exists
        $this->assertFalse($this->valkey_glide->sismember($key1, 'member2')); // member2 removed

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    // ===================================================================
    // SORTED SET OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSortedSetAddOperationsBatch()
    {
        
        $key1 = 'batch_zset_1_' . uniqid();

        // Execute ZADD, ZCARD, ZRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zadd($key1, 1, 'member1', 2, 'member2', 3, 'member3')
            ->zcard($key1)
            ->zrange($key1, 0, -1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(3, $results[0]); // ZADD result (3 new members)
        $this->assertEquals(3, $results[1]); // ZCARD result
        $this->assertEquals(['member1', 'member2', 'member3'], $results[2]); // ZRANGE result

        // Verify server-side effects
        $this->assertEquals(3, $this->valkey_glide->zcard($key1));
        $this->assertEquals(1.0, $this->valkey_glide->zscore($key1, 'member1'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSortedSetScoreOperationsBatch()
    {
        
        $key1 = 'batch_zset_score_' . uniqid();

        // Setup initial sorted set
        $this->valkey_glide->zadd($key1, 10, 'member1', 20, 'member2', 30, 'member3');

        // Execute ZSCORE, ZINCRBY, ZRANK in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zscore($key1, 'member2')
            ->zincrby($key1, 15, 'member1')
            ->zrank($key1, 'member2')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(20.0, $results[0]); // ZSCORE result
        $this->assertEquals(25.0, $results[1]); // ZINCRBY result
        $this->assertEquals(0, $results[2]); // ZRANK result (0-based)

        // Verify server-side effects
        $this->assertEquals(25.0, $this->valkey_glide->zscore($key1, 'member1')); // Score updated
        $this->assertEquals(1, $this->valkey_glide->zrank($key1, 'member1')); // Rank updated

        // Cleanup        
        $this->valkey_glide->del($key1);
    }

    public function testSortedSetRemoveOperationsBatch()
    {        
        $key1 = 'batch_zset_rem_' . uniqid();

        // Setup initial sorted set
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e');

        // Execute ZREM, ZREMRANGEBYSCORE, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zcard($key1)
            ->zrem($key1, 'c')            
            ->zremrangebyscore($key1, 4, 5)            
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(5, $results[0]); // ZCARD result (before removals in transaction)
        $this->assertEquals(1, $results[1]); // ZREM result (1 member removed)                
        $this->assertEquals(2, $results[2]); // ZREMRANGEBYSCORE result (2 members removed)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->zcard($key1)); // 2 members remaining (a, b)
        $remainingMembers = $this->valkey_glide->zrange($key1, 0, -1);
        $this->assertEquals(['a', 'b'], $remainingMembers);

        // Cleanup
        $this->valkey_glide->del($key1);
    }
    // ===================================================================
    // BLOCKING LIST OPERATIONS BATCH TESTS
    // ===================================================================

    public function testBlockingListOperationsBatch()
    {
        $key1 = '{test_list}batch_blocking_list_1_' . uniqid();
        $key2 = '{test_list}batch_blocking_list_2_' . uniqid();
        $key3 = '{test_list}batch_blocking_list_3_' . uniqid();

        // Setup initial lists
        $this->valkey_glide->rpush($key1, 'item1', 'item2', 'item3');
        $this->valkey_glide->rpush($key2, 'item4', 'item5');

        // Execute BRPOP, BLPOP, LLEN in multi/exec batch (with timeout 0 for non-blocking)
        $results = $this->valkey_glide->multi()
            ->brpop($key1, 0)
            ->blpop($key2, 0)
            ->llen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([$key1, 'item3'], $results[0]); // BRPOP result
        $this->assertEquals([$key2, 'item4'], $results[1]); // BLPOP result
        $this->assertEquals(2, $results[2]); // LLEN result (after BRPOP)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->llen($key1)); // One item removed by BRPOP
        $this->assertEquals(1, $this->valkey_glide->llen($key2)); // One item removed by BLPOP
        $this->assertEquals(['item1', 'item2'], $this->valkey_glide->lrange($key1, 0, -1));
        $this->assertEquals(['item5'], $this->valkey_glide->lrange($key2, 0, -1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testAdvancedListMoveOperationsBatch()
    {
        
        $key1 = '{test_list}batch_list_move_1_' . uniqid();
        $key2 = '{test_list}batch_list_move_2_' . uniqid();
        $key3 = '{test_list}batch_list_move_3_' . uniqid();

        // Setup initial lists
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c');
        $this->valkey_glide->rpush($key2, 'x', 'y');

        // Execute BLMOVE, LMPOP, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->blmove($key1, $key2, $this->getLeftConstant(), $this->getRightConstant(), 0)
            ->lmpop([$key1], $this->getRightConstant(), 1)
            ->llen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals('a', $results[0]); // BLMOVE result (moved element)        
        $this->assertEquals([$key1, ['c']], $results[1]); // LMPOP result        
        $this->assertEquals(1, $results[2]); // LLEN result (after moves)

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->llen($key1)); // Only 'b' remains
        $this->assertEquals(['b'], $this->valkey_glide->lrange($key1, 0, -1));
        $this->assertEquals(['x', 'y', 'a'], $this->valkey_glide->lrange($key2, 0, -1)); // 'a' moved to right

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testConditionalListPushBatch()
    {
        $key1 = '{test_list}batch_pushx_1_' . uniqid();
        $key2 = '{test_list}batch_pushx_2_' . uniqid();
        $key3 = '{test_list}batch_pushx_3_' . uniqid();

        // Setup: key1 exists, key2 doesn't exist
        $this->valkey_glide->rpush($key1, 'existing');

        // Execute RPUSHX, LPUSHX, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->rpushx($key1, 'right_added')
            ->lpushx($key2, 'should_not_add') // key2 doesn't exist
            ->llen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // RPUSHX result (new length)
        $this->assertEquals(0, $results[1]); // LPUSHX result (key doesn't exist)
        $this->assertEquals(2, $results[2]); // LLEN result

        // Verify server-side effects
        $this->assertEquals(['existing', 'right_added'], $this->valkey_glide->lrange($key1, 0, -1));
        $this->assertEquals(0, $this->valkey_glide->exists($key2)); // key2 was not created

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testBlockingListMultiPopBatch()
    {
        
        $key1 = '{test_list}batch_blmpop_1_' . uniqid();
        $key2 = '{test_list}batch_blmpop_2_' . uniqid();
        $key3 = '{test_list}batch_blmpop_3_' . uniqid();

        // Setup initial lists
        $this->valkey_glide->rpush($key1, 'a1', 'a2', 'a3');
        $this->valkey_glide->rpush($key2, 'b1', 'b2');

        // Execute BLMPOP, LLEN, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->blmpop(0, [$key1, $key2], $this->getLeftConstant(), 2)
            ->llen($key1)
            ->llen($key2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([$key1, ['a1', 'a2']], $results[0]); // BLMPOP result
        $this->assertEquals(1, $results[1]); // LLEN result (key1 after pop)
        $this->assertEquals(2, $results[2]); // LLEN result (key2 unchanged)

        // Verify server-side effects
        $this->assertEquals(['a3'], $this->valkey_glide->lrange($key1, 0, -1));
        $this->assertEquals(['b1', 'b2'], $this->valkey_glide->lrange($key2, 0, -1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // STREAM OPERATIONS BATCH TESTS
    // ===================================================================

    public function testStreamBasicOperationsBatch()
    {        
        $stream1 = '{test_stream}batch_stream_1_' . uniqid();
        $stream2 = '{test_stream}batch_stream_2_' . uniqid();

        // Execute XADD, XADD, XLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xadd($stream1, '*', ['field1' => 'value1'])
            ->xadd($stream1, '*', ['field2' => 'value2'])
            ->xlen($stream1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsString($results[0]); // First XADD result (entry ID)
        $this->assertIsString($results[1]); // Second XADD result (entry ID)
        $this->assertEquals(2, $results[2]); // XLEN result (2 entries)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->xlen($stream1));

        // Cleanup
        $this->valkey_glide->del($stream1, $stream2);
    }

    public function testStreamReadOperationsBatch()
    {
        $stream1 = 'batch_stream_read_' . uniqid();

        // Setup stream with entries
        $id1 = $this->valkey_glide->xadd($stream1, '*', ['msg' => 'hello']);
        $id2 = $this->valkey_glide->xadd($stream1, '*', ['msg' => 'world']);

        // Execute XREAD, XRANGE, XREVRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xread([$stream1 => '0-0'], 1)
            ->xrange($stream1, '-', '+', 1)
            ->xrevrange($stream1, '+', '-', 1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // XREAD result
        $this->assertArrayHasKey($stream1, $results[0]);
        $this->assertIsArray($results[1]); // XRANGE result
        $this->assertCount(1, $results[1]); // Limited to 1 entry
        $this->assertIsArray($results[2]); // XREVRANGE result
        $this->assertCount(1, $results[2]); // Limited to 1 entry

        // Verify server-side effects
        $allEntries = $this->valkey_glide->xrange($stream1, '-', '+');
        $this->assertCount(2, $allEntries);

        // Cleanup
        $this->valkey_glide->del($stream1);
    }

    public function testStreamGroupOperationsBatch()
    {
        $stream1 = 'batch_stream_group_' . uniqid();
        $group1 = 'batch_group_1';
        $consumer1 = 'batch_consumer_1';

        // Setup stream
        $this->valkey_glide->xadd($stream1, '*', ['data' => 'test']);

        // Execute XGROUP CREATE, XGROUP CREATECONSUMER, XGROUP DELCONSUMER in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xgroup('CREATE', $stream1, $group1, '0-0', true)
            ->xgroup('CREATECONSUMER', $stream1, $group1, $consumer1)
            ->xgroup('DELCONSUMER', $stream1, $group1, $consumer1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // XGROUP CREATE result
        $this->assertTrue($results[1]); // XGROUP CREATECONSUMER result (consumer created)
        $this->assertEquals(0, $results[2]); // XGROUP DELCONSUMER result (0 pending messages)

        // Verify server-side effects by checking group info
        $groupInfo = $this->valkey_glide->xinfo('GROUPS', $stream1);
        $this->assertIsArray($groupInfo);
        $this->assertCount(1, $groupInfo); // One group exists

        // Cleanup
        $this->valkey_glide->xgroup('DESTROY', $stream1, $group1);
        $this->valkey_glide->del($stream1);
    }

    public function testStreamGroupManagementBatch()
    {
        
        $stream1 = 'batch_stream_mgmt_' . uniqid();
        $group1 = 'batch_group_mgmt';

        // Setup stream and group
        $this->valkey_glide->xadd($stream1, '1-0', ['data' => 'first']);
        $this->valkey_glide->xadd($stream1, '2-0', ['data' => 'second']);
        $this->valkey_glide->xgroup('CREATE', $stream1, $group1, '0-0');

        // Execute XGROUP SETID, XGROUP DESTROY, XLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xgroup('SETID', $stream1, $group1, '1-0')
            ->xgroup('DESTROY', $stream1, $group1)
            ->xlen($stream1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // XGROUP SETID result
        $this->assertTrue($results[1]); // XGROUP DESTROY result (1 group destroyed)
        $this->assertEquals(2, $results[2]); // XLEN result (stream still has entries)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->xlen($stream1));
        $groupInfo = $this->valkey_glide->xinfo('GROUPS', $stream1);
        $this->assertCount(0, $groupInfo); // No groups remain

        // Cleanup
        $this->valkey_glide->del($stream1);
    }

    public function testStreamInfoOperationsBatch()
    {
        $stream1 = 'batch_stream_info_' . uniqid();
        $group1 = 'batch_info_group';
        $consumer1 = 'batch_info_consumer';

        // Setup stream, group, and consumer
        $this->valkey_glide->xadd($stream1, '*', ['data' => 'test']);
        $this->valkey_glide->xgroup('CREATE', $stream1, $group1, '0-0');
        $this->valkey_glide->xgroup('CREATECONSUMER', $stream1, $group1, $consumer1);

        // Execute XINFO STREAM, XINFO GROUPS, XINFO CONSUMERS in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xinfo('STREAM', $stream1)
            ->xinfo('GROUPS', $stream1)
            ->xinfo('CONSUMERS', $stream1, $group1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // XINFO STREAM result
        $this->assertArrayHasKey('length', $results[0]);
        $this->assertEquals(1, $results[0]['length']);
        $this->assertIsArray($results[1]); // XINFO GROUPS result
        $this->assertCount(1, $results[1]);
        $this->assertIsArray($results[2]); // XINFO CONSUMERS result
        $this->assertCount(1, $results[2]);

        // Verify server-side effects
        $streamInfo = $this->valkey_glide->xinfo('STREAM', $stream1);
        $this->assertEquals(1, $streamInfo['length']);

        // Cleanup
        $this->valkey_glide->xgroup('DESTROY', $stream1, $group1);
        $this->valkey_glide->del($stream1);
    }

    public function testStreamConsumerOperationsBatch()
    {
        $stream1 = 'batch_stream_consumer_' . uniqid();
        $group1 = 'batch_consumer_group';
        $consumer1 = 'batch_consumer_1';

        // Setup stream and group
        $entryId = $this->valkey_glide->xadd($stream1, '*', ['msg' => 'test']);
        $this->valkey_glide->xgroup('CREATE', $stream1, $group1, '0-0');

        // Execute XREADGROUP, XACK, XPENDING in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xreadgroup($group1, $consumer1, [$stream1 => '>'], 1)
            ->xack($stream1, $group1, [$entryId])
            ->xpending($stream1, $group1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // XREADGROUP result
        $this->assertArrayHasKey($stream1, $results[0]);
        $this->assertEquals(1, $results[1]); // XACK result (1 message acknowledged)
        $this->assertIsArray($results[2]); // XPENDING result
        $this->assertEquals(0, $results[2][0]); // No pending messages after ACK

        // Verify server-side effects
        $pending = $this->valkey_glide->xpending($stream1, $group1);
        $this->assertEquals(0, $pending[0]); // No pending messages

        // Cleanup
        $this->valkey_glide->xgroup('DESTROY', $stream1, $group1);
        $this->valkey_glide->del($stream1);
    }

    public function testStreamAdvancedOperationsBatch()
    {
        $stream1 = 'batch_stream_advanced_' . uniqid();
        $group1 = 'batch_advanced_group';
        $consumer1 = 'batch_advanced_consumer';

        // Setup stream and group
        $id1 = $this->valkey_glide->xadd($stream1, '*', ['msg' => 'first']);
        $id2 = $this->valkey_glide->xadd($stream1, '*', ['msg' => 'second']);
        $this->valkey_glide->xgroup('CREATE', $stream1, $group1, '0-0');

        // Read messages to make them pending
        $this->valkey_glide->xreadgroup($group1, $consumer1, [$stream1 => '>'], 2);

        // Execute XCLAIM, XAUTOCLAIM, XDEL in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xclaim($stream1, $group1, $consumer1, 0, [$id1], ['JUSTID'])
            ->xautoclaim($stream1, $group1, $consumer1, 0, '0-0', 1, true)
            ->xdel($stream1, [$id2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // XCLAIM result
        $this->assertContains($id1, $results[0]);
        $this->assertIsArray($results[1]); // XAUTOCLAIM result
        $this->assertEquals(1, $results[2]); // XDEL result (1 message deleted)

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->xlen($stream1)); // One message deleted

        // Cleanup
        $this->valkey_glide->xgroup('DESTROY', $stream1, $group1);
        $this->valkey_glide->del($stream1);
    }

    public function testStreamTrimOperationsBatch()
    {
        $stream1 = 'batch_stream_trim_' . uniqid();

        // Setup stream with multiple entries
        $this->valkey_glide->xadd($stream1, '*', ['msg' => '1']);
        $this->valkey_glide->xadd($stream1, '*', ['msg' => '2']);
        $this->valkey_glide->xadd($stream1, '*', ['msg' => '3']);
        $this->valkey_glide->xadd($stream1, '*', ['msg' => '4']);
        $this->valkey_glide->xadd($stream1, '*', ['msg' => '5']);

        // Execute XTRIM, XLEN, XRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->xtrim($stream1, '3') // Keep only 3 entries
            ->xlen($stream1)
            ->xrange($stream1, '-', '+')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(2, $results[0]); // XTRIM result (2 entries removed)
        $this->assertEquals(3, $results[1]); // XLEN result (3 entries remain)
        $this->assertCount(3, $results[2]); // XRANGE result (3 entries)

        // Verify server-side effects
        $this->assertEquals(3, $this->valkey_glide->xlen($stream1));

        // Cleanup
        $this->valkey_glide->del($stream1);
    }

    // ===================================================================
    // HYPERLOGLOG OPERATIONS BATCH TESTS
    // ===================================================================

    public function testHyperLogLogOperationsBatch()
    {        
        $key1 = '{test_hll}batch_hll_1_' . uniqid();
        $key2 = '{test_hll}batch_hll_2_' . uniqid();
        $key3 = '{test_hll}batch_hll_3_' . uniqid();

        // Execute PFADD, PFADD, PFCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->pfadd($key1, ['element1', 'element2', 'element3'])
            ->pfadd($key2, ['element3', 'element4', 'element5'])
            ->pfcount([$key1, $key2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // PFADD result (HLL was altered)
        $this->assertEquals(1, $results[1]); // PFADD result (HLL was altered)
        $this->assertGTE(4, $results[2]); // PFCOUNT result (approximate count >= 4)
        $this->assertLTE(6, $results[2]); // PFCOUNT result (approximate count <= 6)
        
        // Verify server-side effects
        $count1 = $this->valkey_glide->pfcount($key1);
        $count2 = $this->valkey_glide->pfcount($key2);
        $this->assertGTE(3, $count1); // At least 3 elements
        $this->assertGTE(3, $count2); // At least 3 elements

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testHyperLogLogMergeBatch()
    {
        $key1 = '{test_hll}batch_hll_merge_1_' . uniqid();
        $key2 = '{test_hll}batch_hll_merge_2_' . uniqid();
        $key3 = '{test_hll}batch_hll_merge_3_' . uniqid();

        // Setup HLLs
        $this->valkey_glide->pfadd($key1, ['a', 'b', 'c']);
        $this->valkey_glide->pfadd($key2, ['c', 'd', 'e']);

        // Execute PFMERGE, PFCOUNT, PFCOUNT in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->pfmerge($key3, [$key1, $key2])
            ->pfcount($key3)
            ->pfcount([$key1, $key2, $key3])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // PFMERGE result
        $this->assertGTE(4, $results[1]); // PFCOUNT result (merged HLL)
        $this->assertGTE(4, $results[2]); // PFCOUNT result (union of all)

        // Verify server-side effects
        $mergedCount = $this->valkey_glide->pfcount($key3);
        $this->assertGTE(4, $mergedCount); // Merged HLL should have at least 4 unique elements

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // ADVANCED SORTED SET OPERATIONS BATCH TESTS
    // ===================================================================

    public function testSortedSetBlockingPopBatch()
    {
        $key1 = '{test_hll}batch_zset_blocking_1_' . uniqid();
        $key2 = '{test_hll}batch_zset_blocking_2_' . uniqid();

        // Setup sorted sets
        $this->valkey_glide->zadd($key1, 1, 'one', 2, 'two', 3, 'three');
        $this->valkey_glide->zadd($key2, 10, 'ten', 20, 'twenty');

        // Execute BZPOPMIN, BZPOPMAX, ZCARD in multi/exec batch (timeout 0 for non-blocking)
        $results = $this->valkey_glide->multi()
            ->bzpopmin($key1, 0)
            ->bzpopmax($key2, 0)
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([$key1, 'one', '1'], $results[0]); // BZPOPMIN result
        $this->assertEquals([$key2, 'twenty', '20'], $results[1]); // BZPOPMAX result
        $this->assertEquals(2, $results[2]); // ZCARD result (after BZPOPMIN)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->zcard($key1)); // One element removed
        $this->assertEquals(1, $this->valkey_glide->zcard($key2)); // One element removed
        $this->assertEquals(['two', 'three'], $this->valkey_glide->zrange($key1, 0, -1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    public function testSortedSetMultiScoreBatch()
    {
        $key1 = 'batch_zset_mscore_' . uniqid();

        // Setup sorted set
        $this->valkey_glide->zadd($key1, 1, 'one', 2, 'two', 3, 'three', 4, 'four');

        // Execute ZMSCORE, ZMSCORE, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zmscore($key1, 'one', 'three')
            ->zmscore($key1, 'two', 'nonexistent')
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([1.0, 3.0], $results[0]); // ZMSCORE result
        $this->assertEquals([2.0, false], $results[1]); // ZMSCORE result (with non-existent)
        $this->assertEquals(4, $results[2]); // ZCARD result

        // Verify server-side effects
        $this->assertEquals(1.0, $this->valkey_glide->zscore($key1, 'one'));
        $this->assertEquals(3.0, $this->valkey_glide->zscore($key1, 'three'));

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSortedSetDiffOperationsBatch()
    {
        $key1 = '{test_zset}batch_zset_diff_1_' . uniqid();
        $key2 = '{test_zset}batch_zset_diff_2_' . uniqid();
        $key3 = '{test_zset}batch_zset_diff_3_' . uniqid();

        // Setup sorted sets
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c');
        $this->valkey_glide->zadd($key2, 2, 'b', 4, 'd');

        // Execute ZDIFF, ZDIFFSTORE, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zdiff([$key1, $key2])
            ->zdiffstore($key3, [$key1, $key2])
            ->zcard($key3)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['a', 'c'], $results[0]); // ZDIFF result
        $this->assertEquals(2, $results[1]); // ZDIFFSTORE result (2 elements stored)
        $this->assertEquals(2, $results[2]); // ZCARD result

        // Verify server-side effects
        $this->assertEquals(['a', 'c'], $this->valkey_glide->zrange($key3, 0, -1));
        $this->assertEquals(2, $this->valkey_glide->zcard($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testSortedSetRangeStoreBatch()
    {
        $key1 = '{test_zset}batch_zset_rangestore_1_' . uniqid();
        $key2 = '{test_zset}batch_zset_rangestore_2_' . uniqid();

        // Setup sorted set
        $this->valkey_glide->zadd($key1, 1, 'one', 2, 'two', 3, 'three', 4, 'four', 5, 'five');

        // Execute ZRANGESTORE, ZCARD, ZRANGE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zrangestore($key2, $key1, '1', '3') // Store elements at ranks 1-3
            ->zcard($key2)
            ->zrange($key2, 0, -1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(3, $results[0]); // ZRANGESTORE result (3 elements stored)
        $this->assertEquals(3, $results[1]); // ZCARD result
        $this->assertEquals(['two', 'three', 'four'], $results[2]); // ZRANGE result

        // Verify server-side effects
        $this->assertEquals(3, $this->valkey_glide->zcard($key2));
        $this->assertEquals(['two', 'three', 'four'], $this->valkey_glide->zrange($key2, 0, -1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    public function testSortedSetInterUnionBatch()
    {
        
        $key1 = '{test_zset}batch_zset_inter_1_' . uniqid();
        $key2 = '{test_zset}batch_zset_inter_2_' . uniqid();
        $key3 = '{test_zset}batch_zset_inter_3_' . uniqid();

        // Setup sorted sets
        $this->valkey_glide->zadd($key1, 1, 'a', 2, 'b', 3, 'c');
        $this->valkey_glide->zadd($key2, 2, 'b', 3, 'c', 4, 'd');

        // Execute ZINTER, ZUNION, ZINTERCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zinter([$key1, $key2])
            ->zunion([$key1, $key2])
            ->zintercard([$key1, $key2])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['b', 'c'], $results[0]); // ZINTER result
        $this->assertEquals(['a', 'b', 'd', 'c'], $results[1]); // ZUNION result        
        $this->assertEquals(2, $results[2]); // ZINTERCARD result

        // Verify server-side effects (original sets unchanged)
        $this->assertEquals(3, $this->valkey_glide->zcard($key1));
        $this->assertEquals(3, $this->valkey_glide->zcard($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testSortedSetMultiPopBatch()
    {
        
        $key1 = '{test_zset}batch_zset_mpop_1_' . uniqid();
        $key2 = '{test_zset}batch_zset_mpop_2_' . uniqid();

        // Setup sorted sets
        $this->valkey_glide->zadd($key1, 1, 'one', 2, 'two', 3, 'three');
        $this->valkey_glide->zadd($key2, 10, 'ten', 20, 'twenty');

        // Execute ZMPOP, BZMPOP, ZCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->zmpop([$key1], 'MIN', 2)
            ->bzmpop(0, [$key2], 'MAX', 1)
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([$key1, ['one' => 1.0, 'two' => 2.0]], $results[0]); // ZMPOP result
        $this->assertEquals([$key2, ['twenty' => 20.0]], $results[1]); // BZMPOP result
        $this->assertEquals(1, $results[2]); // ZCARD result (after ZMPOP)

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->zcard($key1)); // 2 elements removed
        $this->assertEquals(1, $this->valkey_glide->zcard($key2)); // 1 element removed
        $this->assertEquals(['three'], $this->valkey_glide->zrange($key1, 0, -1));

        // Cleanup
        $this->valkey_glide->del($key1, $key2);
    }

    // ===================================================================
    // LONGEST COMMON SUBSEQUENCE BATCH TESTS
    // ===================================================================

    public function testLongestCommonSubsequenceBatch()
    {
        
        $key1 = '{test_lcs}batch_lcs_1_' . uniqid();
        $key2 = '{test_lcs}batch_lcs_2_' . uniqid();
        $key3 = '{test_lcs}batch_lcs_3_' . uniqid();

        // Setup test strings
        $this->valkey_glide->set($key1, 'abcdefg');
        $this->valkey_glide->set($key2, 'aceg');

        // Execute LCS, LCS with LEN, LCS with IDX in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lcs($key1, $key2)
            ->lcs($key1, $key2, ['len'])
            ->lcs($key1, $key2, ['idx'])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsString($results[0]); // LCS result (common subsequence)        
        $this->assertIsInt($results[1]); // LCS LEN result (length)
        $this->assertIsArray($results[2]); // LCS IDX result (with indexes)

        // Verify server-side effects (LCS is read-only)
        $this->assertEquals('abcdefg', $this->valkey_glide->get($key1));
        $this->assertEquals('aceg', $this->valkey_glide->get($key2));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // FUNCTION MANAGEMENT BATCH TESTS
    // ===================================================================

    public function testFunctionManagementBatch()
    {
        
        $functionCode = "#!lua name=mylib\nredis.register_function('myfunc', function(keys, args) return args[1] end)";

        // Execute FUNCTION LOAD, FUNCTION LIST, FUNCTION DELETE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->function('LOAD', $functionCode)
            ->function('LIST')
            ->fcall('myfunc', [], ['foo'])  
            ->function('load', 'replace',"#!lua name=mylib\nredis.register_function{function_name='myfunc_ro', callback=function(keys, args) return args[1] end, flags={'no-writes'}}")
            ->fcall_ro('myfunc_ro', [], ['foo'])                      
            ->function('DELETE', 'mylib')
            ->exec();
        
        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(6, $results);
        $this->assertEquals('mylib', $results[0]); // FUNCTION LOAD result        
        $this->assertIsArray($results[1]); // FUNCTION LIST result        
        $this->assertEquals('foo',$results[2]); // fcall result
        $this->assertEquals('mylib', $results[3]); // FUNCTION LOAD result                
        $this->assertEquals('foo',$results[4]); // fcall_ro result
        $this->assertTrue($results[5]); // FUNCTION DELETE result

        // Verify server-side effects
        $functions = $this->valkey_glide->function('LIST');
        $this->assertIsArray($functions);
        // Library should be deleted, so it shouldn't appear in the list
    }

    public function testFunctionDumpRestoreBatch()
    {
        
        $functionCode = "#!lua name=mylib\nredis.register_function('myfunc', function(keys, args) return args[1] end)";

        // Setup function
        $this->valkey_glide->function('LOAD', $functionCode);
        $dump = $this->valkey_glide->function('DUMP');

        // Execute FUNCTION FLUSH, FUNCTION RESTORE, FUNCTION LIST in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->function('FLUSH')
            ->function('RESTORE', $dump)
            ->function('LIST')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // FUNCTION FLUSH result
        $this->assertTrue($results[1]); // FUNCTION RESTORE result
        $this->assertIsArray($results[2]); // FUNCTION LIST result
        $this->assertCount(1, $results[2]); // One library restored

        // Cleanup
        $this->valkey_glide->function('FLUSH');
    }

    // ===================================================================
    // CLOSING CLASS
    // ===================================================================
}
