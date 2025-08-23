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
        $key1 = 'batch_string_1_' . uniqid();
        $key2 = 'batch_string_2_' . uniqid();
        $key3 = 'batch_string_3_' . uniqid();
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
        $key1 = 'batch_incr_1_' . uniqid();
        $key2 = 'batch_incr_2_' . uniqid();
        $key3 = 'batch_incr_3_' . uniqid();

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
        $key1 = 'batch_decr_1_' . uniqid();
        $key2 = 'batch_decr_2_' . uniqid();
        $key3 = 'batch_decr_3_' . uniqid();

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
        $key1 = 'batch_exists_1_' . uniqid();
        $key2 = 'batch_exists_2_' . uniqid();
        $key3 = 'batch_exists_3_' . uniqid();

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
        $key1 = 'batch_expire_1_' . uniqid();
        $key2 = 'batch_expire_2_' . uniqid();
        $key3 = 'batch_expire_3_' . uniqid();

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
        $this->assertEquals(1, $results[0]); // EXPIRE result
        $this->assertEquals(1, $results[1]); // EXPIREAT result
        $this->assertEquals(1, $results[2]); // PEXPIRE result

        // Verify server-side effects
        $this->assertGT(0, $this->valkey_glide->ttl($key1)); // Has TTL
        $this->assertGT(0, $this->valkey_glide->ttl($key2)); // Has TTL
        $this->assertGT(0, $this->valkey_glide->ttl($key3)); // Has TTL

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    public function testKeyTtlOperationsBatch()
    {
        $key1 = 'batch_ttl_1_' . uniqid();
        $key2 = 'batch_ttl_2_' . uniqid();
        $key3 = 'batch_ttl_3_' . uniqid();

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
        $this->assertEquals(1, $results[2]); // PERSIST result (removed expiration)

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

    public function testHashFieldOperationsBatch()
    {
        $key1 = 'batch_hash_fields_1_' . uniqid();

        // Setup initial hash
        $this->valkey_glide->hset($key1, 'field1', 'value1', 'field2', 'value2', 'field3', 'value3');

        // Execute HEXISTS, HDEL, HLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->hexists($key1, 'field1')
            ->hdel($key1, 'field2')
            ->hlen($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // HEXISTS result (field exists)
        $this->assertEquals(1, $results[1]); // HDEL result (1 field deleted)
        $this->assertEquals(3, $results[2]); // HLEN result (before deletion in transaction)

        // Verify server-side effects
        $this->assertEquals(1, $this->valkey_glide->hexists($key1, 'field1')); // field1 still exists
        $this->assertEquals(0, $this->valkey_glide->hexists($key1, 'field2')); // field2 deleted
        $this->assertEquals(2, $this->valkey_glide->hlen($key1)); // 2 fields remaining

        // Cleanup
        $this->valkey_glide->del($key1);
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
        $this->assertEquals(4, $results[2]); // LLEN result (before pops in transaction)

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
        $this->assertEquals(['a', 'b', 'c', 'd', 'e', 'f'], $results[2]); // LRANGE (before trim in transaction)

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
        $this->assertEquals(1, $this->valkey_glide->sismember($key1, 'member1'));

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
        $this->assertEquals(1, $results[1]); // SISMEMBER result (member exists)
        $this->assertEquals(4, $results[2]); // SCARD result (before removal in transaction)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->scard($key1)); // 2 members remaining
        $this->assertEquals(1, $this->valkey_glide->sismember($key1, 'member1')); // member1 exists
        $this->assertEquals(0, $this->valkey_glide->sismember($key1, 'member2')); // member2 removed

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
        $this->assertEquals(1, $this->valkey_glide->zscore($key1, 'member1'));

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
            ->zincrby($key1, 5, 'member1')
            ->zrank($key1, 'member2')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(20, $results[0]); // ZSCORE result
        $this->assertEquals(15, $results[1]); // ZINCRBY result
        $this->assertEquals(1, $results[2]); // ZRANK result (0-based)

        // Verify server-side effects
        $this->assertEquals(15, $this->valkey_glide->zscore($key1, 'member1')); // Score updated
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
            ->zrem($key1, 'c')
            ->zremrangebyscore($key1, 4, 5)
            ->zcard($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // ZREM result (1 member removed)
        $this->assertEquals(2, $results[1]); // ZREMRANGEBYSCORE result (2 members removed)
        $this->assertEquals(5, $results[2]); // ZCARD result (before removals in transaction)

        // Verify server-side effects
        $this->assertEquals(2, $this->valkey_glide->zcard($key1)); // 2 members remaining (a, b)
        $remainingMembers = $this->valkey_glide->zrange($key1, 0, -1);
        $this->assertEquals(['a', 'b'], $remainingMembers);

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
            ->client('getname')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // INFO result (array of server info)
        $this->assertIsInt($results[1]); // CLIENT ID result (integer)
        // CLIENT GETNAME might return null if no name is set

        // Verify server info contains expected keys
        $this->assertArrayHasKey('server', $results[0]);
        $this->assertGT(0, $results[1]); // Client ID should be positive
    }

    public function testDatabaseOperationsBatch()
    {
        $key1 = 'batch_db_' . uniqid();
        $this->valkey_glide->set($key1, 'test_value');

        // Execute SELECT, DBSIZE, TYPE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->select(0) // Select database 0 (likely current)
            ->dbsize()
            ->type($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // SELECT result
        $this->assertIsInt($results[1]); // DBSIZE result
        $this->assertEquals('string', $results[2]); // TYPE result

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
        $key1 = 'batch_str_1_' . uniqid();
        $key2 = 'batch_str_2_' . uniqid();
        $key3 = 'batch_str_3_' . uniqid();

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
        $key1 = 'batch_rename_1_' . uniqid();
        $key2 = 'batch_rename_2_' . uniqid();
        $key3 = 'batch_rename_3_' . uniqid();
        $newKey1 = 'batch_rename_new_1_' . uniqid();

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
        $this->assertEquals(1, $results[2]); // RENAMENX result (success)

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
        $key1 = 'batch_mget_1_' . uniqid();
        $key2 = 'batch_mget_2_' . uniqid();
        $key3 = 'batch_mget_3_' . uniqid();

        // Execute MSET, MGET, MSETNX in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->mset([$key1 => 'value1', $key2 => 'value2'])
            ->mget($key1, $key2, $key3)
            ->msetnx([$key3 => 'value3'])
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertTrue($results[0]); // MSET result
        $this->assertEquals(['value1', 'value2', false], $results[1]); // MGET result
        $this->assertEquals(1, $results[2]); // MSETNX result (success for key3)

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
        $key1 = 'batch_bit_1_' . uniqid();
        $key2 = 'batch_bit_2_' . uniqid();
        $key3 = 'batch_bit_3_' . uniqid();

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
        $key1 = 'batch_bitadv_1_' . uniqid();
        $key2 = 'batch_bitadv_2_' . uniqid();
        $key3 = 'batch_bitadv_3_' . uniqid();

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
            ->hmget($key1, 'field1', 'field2', 'field_nonexistent')
            ->hkeys($key1)
            ->hvals($key1)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(['value1', 'value2', false], $results[0]); // HMGET result
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
        $this->assertEquals(1, $results[0]); // HSETNX result (new field set)
        $this->assertEquals(0, $results[1]); // HSETNX result (existing field not overwritten)
        $this->assertEquals(14, $results[2]); // HSTRLEN result (length of "existing_value")

        // Verify server-side effects
        $this->assertEquals('new_value', $this->valkey_glide->hget($key1, 'new_field'));
        $this->assertEquals('existing_value', $this->valkey_glide->hget($key1, 'existing_field')); // Not overwritten
        $this->assertEquals(14, $this->valkey_glide->hstrlen($key1, 'existing_field'));

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
            ->lrem($key1, 1, 'b') // Remove first occurrence of 'b'
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
        $key1 = 'batch_list_move_src_' . uniqid();
        $key2 = 'batch_list_move_dst_' . uniqid();

        // Setup initial lists
        $this->valkey_glide->rpush($key1, 'a', 'b', 'c');
        $this->valkey_glide->rpush($key2, 'x', 'y');

        // Execute LMOVE, LLEN, LLEN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->lmove($key1, $key2, 'RIGHT', 'LEFT')
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
        $this->assertEquals(0, $this->valkey_glide->sismember($key1, $results[0])); // Popped member no longer exists

        // Cleanup
        $this->valkey_glide->del($key1);
    }

    public function testSetOperationsBatch()
    {
        $key1 = 'batch_set_ops_1_' . uniqid();
        $key2 = 'batch_set_ops_2_' . uniqid();
        $key3 = 'batch_set_ops_3_' . uniqid();

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
        $key1 = 'batch_set_store_1_' . uniqid();
        $key2 = 'batch_set_store_2_' . uniqid();
        $key3 = 'batch_set_store_3_' . uniqid();
        $key4 = 'batch_set_store_4_' . uniqid();
        $key5 = 'batch_set_store_5_' . uniqid();

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
        $this->assertEquals(['a', '1'], $results[0]); // ZPOPMIN result
        $this->assertEquals(['e', '5'], $results[1]); // ZPOPMAX result
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
        $key1 = 'batch_exptime_1_' . uniqid();
        $key2 = 'batch_exptime_2_' . uniqid();
        $key3 = 'batch_exptime_3_' . uniqid();

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
        $this->assertEquals(1, $results[2]); // PEXPIREAT result (success)

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
        $key1 = 'batch_geo_adv_' . uniqid();

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
        $storeKey = 'batch_geo_store_' . uniqid();
        $results = $this->valkey_glide->multi()
            ->geohash($key1, 'Golden Gate Bridge')
            ->geosearch($key1, 'FROMLONLAT', -122.27652, 37.805186, 'BYRADIUS', 5, 'km')
            ->geosearchstore($storeKey, $key1, 'FROMLONLAT', -122.27652, 37.805186, 'BYRADIUS', 10, 'km')
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // GEOHASH result
        $this->assertIsArray($results[1]); // GEOSEARCH result
        $this->assertGTE(0, $results[2]); // GEOSEARCHSTORE result

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
        $key1 = 'batch_scan_set_' . uniqid();
        $key2 = 'batch_scan_hash_' . uniqid();
        $key3 = 'batch_scan_zset_' . uniqid();

        // Setup test data
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3');
        $this->valkey_glide->hset($key2, 'field1', 'value1', 'field2', 'value2');
        $this->valkey_glide->zadd($key3, 1, 'zmember1', 2, 'zmember2');

        // Execute SCAN, SSCAN, HSCAN in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->scan(0)
            ->sscan($key1, 0)
            ->hscan($key2, 0)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // SCAN result [cursor, keys]
        $this->assertCount(2, $results[0]);
        $this->assertIsArray($results[1]); // SSCAN result [cursor, members]
        $this->assertCount(2, $results[1]);
        $this->assertIsArray($results[2]); // HSCAN result [cursor, fields_values]
        $this->assertCount(2, $results[2]);

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
        $results = $this->valkey_glide->multi()
            ->zscan($key1, 0)
            ->zlexcount($key1, '-', '+')
            ->zrandmember($key1, 2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertIsArray($results[0]); // ZSCAN result [cursor, members_scores]
        $this->assertCount(2, $results[0]);
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
        $key1 = 'batch_sort_list_' . uniqid();
        $key2 = 'batch_sort_store_' . uniqid();

        // Setup test data
        $this->valkey_glide->lpush($key1, '3', '1', '2', '5', '4');

        // Execute SORT, SORT_RO, SORT with STORE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->sort($key1)
            ->sort_ro($key1, 'DESC')
            ->sort($key1, 'ASC', 'STORE', $key2)
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
        $key1 = 'batch_copy_src_' . uniqid();
        $key2 = 'batch_copy_dst_' . uniqid();
        $key3 = 'batch_restore_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'test_value');

        // Execute COPY, DUMP, RESTORE in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->copy($key1, $key2)
            ->dump($key1)
            ->restore($key3, 0, $results[1] ?? '') // Note: This won't work in multi, just for structure
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals(1, $results[0]); // COPY result (success)
        $this->assertIsString($results[1]); // DUMP result (serialized data)
        // RESTORE result depends on having valid dump data

        // Verify server-side effects
        $this->assertEquals('test_value', $this->valkey_glide->get($key2)); // Copied value

        // Now properly restore using the dump data
        $dumpData = $this->valkey_glide->dump($key1);
        $this->assertTrue($this->valkey_glide->restore($key3, 0, $dumpData));
        $this->assertEquals('test_value', $this->valkey_glide->get($key3));

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // MOVE OPERATIONS BATCH TESTS
    // ===================================================================

    public function testMoveBatch()
    {
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
        $this->assertEquals(1, $results[0]); // MOVE result (success)
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
        $key1 = 'batch_getdel_' . uniqid();
        $key2 = 'batch_getex_' . uniqid();
        $key3 = 'batch_getex2_' . uniqid();

        // Setup test data
        $this->valkey_glide->set($key1, 'delete_me');
        $this->valkey_glide->set($key2, 'expire_me');
        $this->valkey_glide->set($key3, 'persist_me');

        // Execute GETDEL, GETEX with expiration, GETEX with persist in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->getdel($key1)
            ->getex($key2, 'EX', 3600) // Set expiration
            ->getex($key3, 'PERSIST') // Remove expiration
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
        $key1 = 'batch_smember_' . uniqid();
        $key2 = 'batch_smove_src_' . uniqid();
        $key3 = 'batch_smove_dst_' . uniqid();

        // Setup test data
        $this->valkey_glide->sadd($key1, 'member1', 'member2', 'member3');
        $this->valkey_glide->sadd($key2, 'move_me', 'stay_here');
        $this->valkey_glide->sadd($key3, 'already_here');

        // Execute SMISMEMBER, SMOVE, SINTERCARD in multi/exec batch
        $results = $this->valkey_glide->multi()
            ->smismember($key1, 'member1', 'member2', 'nonexistent')
            ->smove($key2, $key3, 'move_me')
            ->sintercard($key1, $key2)
            ->exec();

        // Verify transaction results
        $this->assertIsArray($results);
        $this->assertCount(3, $results);
        $this->assertEquals([1, 1, 0], $results[0]); // SMISMEMBER result
        $this->assertEquals(1, $results[1]); // SMOVE result (success)
        $this->assertIsInt($results[2]); // SINTERCARD result

        // Verify server-side effects
        $this->assertEquals(0, $this->valkey_glide->sismember($key2, 'move_me')); // Moved from src
        $this->assertEquals(1, $this->valkey_glide->sismember($key3, 'move_me')); // Moved to dst

        // Cleanup
        $this->valkey_glide->del($key1, $key2, $key3);
    }

    // ===================================================================
    // CLOSING CLASS
    // ===================================================================
}
