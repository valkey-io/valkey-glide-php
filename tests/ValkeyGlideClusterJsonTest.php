<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideClusterBaseTest.php";

/**
 * Integration tests for JSON.SET and JSON.GET commands on cluster.
 * Requires a Valkey cluster with the JSON module loaded.
 */
class ValkeyGlideClusterJsonTest extends ValkeyGlideClusterBaseTest
{
    public function setUp()
    {
        parent::setUp();

        try {
            $this->valkey_glide->jsonSet('__json_probe__', '$', '1');
            $this->valkey_glide->del('__json_probe__');
        } catch (\Throwable $e) {
            $msg = $e->getMessage();
            if (
                stripos($msg, 'unknown command') !== false
                || stripos($msg, 'ERR unknown') !== false
                || stripos($msg, 'module') !== false
            ) {
                throw new TestSkippedException("JSON module not available on this server");
            }
        }
    }

    public function testJsonSetAndGet()
    {
        $key = '{json}:' . uniqid();
        try {
            $result = $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": "hello"}');
            $this->assertTrue($result);

            $value = $this->valkey_glide->jsonGet($key);
            $this->assertNotEquals(false, $value);

            $decoded = json_decode($value, true);
            $this->assertEquals(1, $decoded[0]['a']);
            $this->assertEquals('hello', $decoded[0]['b']);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetWithPath()
    {

        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": 2}');
            $this->valkey_glide->jsonSet($key, '$.a', '42');

            $value = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[42]', $value);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetNX()
    {

        $key = '{json}:' . uniqid();
        try {
            $result = $this->valkey_glide->jsonSet($key, '$', '{"a": 1}', 'NX');
            $this->assertTrue($result);

            $result = $this->valkey_glide->jsonSet($key, '$', '{"a": 2}', 'NX');
            $this->assertEquals(null, $result);

            $value = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[1]', $value);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetXX()
    {

        $key = '{json}:' . uniqid();
        try {
            $result = $this->valkey_glide->jsonSet($key, '$', '{"a": 1}', 'XX');
            $this->assertEquals(null, $result);

            $this->valkey_glide->jsonSet($key, '$', '{"a": 1}');

            $result = $this->valkey_glide->jsonSet($key, '$', '{"a": 2}', 'XX');
            $this->assertTrue($result);

            $value = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[2]', $value);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonGetNonExistingKey()
    {

        $key = '{json}:' . uniqid();
        $this->valkey_glide->del($key);

        $value = $this->valkey_glide->jsonGet($key);
        $this->assertEquals(null, $value);
    }

    public function testJsonGetMultiplePaths()
    {

        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": "two", "c": [3, 4]}');

            $value = $this->valkey_glide->jsonGet($key, ['$.a', '$.b']);
            $this->assertNotEquals(false, $value);

            $decoded = json_decode($value, true);
            $this->assertNotEquals(null, $decoded);
            $this->assertTrue(isset($decoded['$.a']));
            $this->assertTrue(isset($decoded['$.b']));
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonGetSinglePath()
    {

        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"name": "test", "value": 42}');

            $value = $this->valkey_glide->jsonGet($key, '$.name');
            $this->assertEquals('["test"]', $value);

            $value = $this->valkey_glide->jsonGet($key, '$.value');
            $this->assertEquals('[42]', $value);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetNestedObject()
    {

        $key = '{json}:' . uniqid();
        try {
            $json = '{"user": {"name": "Alice", "age": 30}, "scores": [10, 20, 30]}';
            $this->valkey_glide->jsonSet($key, '$', $json);

            $value = $this->valkey_glide->jsonGet($key, '$.user.name');
            $this->assertEquals('["Alice"]', $value);

            $value = $this->valkey_glide->jsonGet($key, '$.scores');
            $this->assertEquals('[[10,20,30]]', $value);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetInvalidJson()
    {
        $key = '{json}:' . uniqid();
        $threw = false;
        try {
            $this->valkey_glide->jsonSet($key, '$', 'not valid json');
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
        $this->valkey_glide->del($key);
    }

    public function testJsonGetWithOptions()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": 2}');

            // Test with array
            $value = $this->valkey_glide->jsonGet($key, '$', [
                'indent' => '  ',
                'newline' => "\n",
                'space' => ' ',
            ]);
            $this->assertNotEquals(false, $value);
            $this->assertTrue(strpos($value, "\n") !== false);

            // Test with builder
            $opts = \ValkeyGlide\Json\JsonGetOptions::builder()
                ->indent('  ')
                ->newline("\n")
                ->space(' ');
            $value2 = $this->valkey_glide->jsonGet($key, '$', $opts->toArray());
            $this->assertNotEquals(false, $value2);
            $this->assertTrue(strpos($value2, "\n") !== false);
            $this->assertEquals($value, $value2);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonGetInvalidPath()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1}');
            $threw = false;
            try {
                $this->valkey_glide->jsonGet($key, '.invalid[path');
            } catch (\Throwable $e) {
                $threw = true;
            }
            $this->assertTrue($threw);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonSetOnWrongKeyType()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->set($key, 'plain_string');
            $threw = false;
            try {
                $this->valkey_glide->jsonSet($key, '$.a', '1');
            } catch (\Throwable $e) {
                $threw = true;
            }
            $this->assertTrue($threw);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonDel()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "nested": {"a": 2, "b": 3}}');
            $result = $this->valkey_glide->jsonDel($key, '$..a');
            $this->assertEquals(2, $result);

            $this->valkey_glide->jsonSet($key, '$', '{"x": 1}');
            $result = $this->valkey_glide->jsonDel($key);
            $this->assertEquals(1, $result);

            $result = $this->valkey_glide->jsonDel('non_existing_key_' . uniqid());
            $this->assertEquals(0, $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonForget()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": 2}');
            $result = $this->valkey_glide->jsonForget($key, '$.a');
            $this->assertEquals(1, $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonClear()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": [1, 2, 3]}');
            $result = $this->valkey_glide->jsonClear($key, '$.*');
            $this->assertEquals(2, $result);

            $result = $this->valkey_glide->jsonClear($key, '$.*');
            $this->assertEquals(0, $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonMGet()
    {
        $key1 = '{json}:mget1:' . uniqid();
        $key2 = '{json}:mget2:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key1, '$', '{"a": 1}');
            $this->valkey_glide->jsonSet($key2, '$', '{"a": 2}');

            $result = $this->valkey_glide->jsonMGet([$key1, $key2, 'non_existing_' . uniqid()], '$.a');
            $this->assertIsArray($result);
            $this->assertCount(3, $result);
            $this->assertEquals('[1]', $result[0]);
            $this->assertEquals('[2]', $result[1]);
            $this->assertEquals(null, $result[2]);
        } finally {
            $this->valkey_glide->del($key1);
            $this->valkey_glide->del($key2);
        }
    }

    public function testJsonType()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": "hello", "c": [1, 2]}');

            $result = $this->valkey_glide->jsonType($key, '$.a');
            $this->assertEquals(['integer'], $result);

            $result = $this->valkey_glide->jsonType($key, '$.b');
            $this->assertEquals(['string'], $result);

            $result = $this->valkey_glide->jsonType($key, '$.c');
            $this->assertEquals(['array'], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonNumIncrBy()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": 2.5}');

            $result = $this->valkey_glide->jsonNumIncrBy($key, '$.a', 10);
            $this->assertEquals('[11]', $result);

            $result = $this->valkey_glide->jsonNumIncrBy($key, '$.b', 0.5);
            $this->assertEquals('[3]', $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonNumMultBy()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 2, "b": 3}');

            $result = $this->valkey_glide->jsonNumMultBy($key, '$.a', 5);
            $this->assertEquals('[10]', $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonToggle()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"bool": true, "nested": {"bool": false}}');

            $result = $this->valkey_glide->jsonToggle($key, '$..bool');
            $this->assertIsArray($result);
            $this->assertEquals(false, $result[0]);
            $this->assertEquals(true, $result[1]);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonStrAppend()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": "foo"}');

            $result = $this->valkey_glide->jsonStrAppend($key, '$.a', '"bar"');
            $this->assertEquals([6], $result);

            $getResult = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertTrue(strpos($getResult, 'foobar') !== false);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonStrLen()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": "hello"}');

            $result = $this->valkey_glide->jsonStrLen($key, '$.a');
            $this->assertEquals([5], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonObjLen()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": {"x": 1, "y": 2}}');

            $result = $this->valkey_glide->jsonObjLen($key);
            $this->assertEquals(2, $result);

            $result = $this->valkey_glide->jsonObjLen($key, '$.b');
            $this->assertEquals([2], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonObjKeys()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": 2}');

            $result = $this->valkey_glide->jsonObjKeys($key);
            $this->assertEquals(['a', 'b'], $result);

            $result = $this->valkey_glide->jsonObjKeys('non_existing_key_' . uniqid());
            $this->assertNull($result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrAppend()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [1, 2]}');

            $result = $this->valkey_glide->jsonArrAppend($key, '$.a', '3', '4');
            $this->assertEquals([4], $result);

            $getResult = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[[1,2,3,4]]', $getResult);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrInsert()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [1, 2, 3]}');

            $result = $this->valkey_glide->jsonArrInsert($key, '$.a', 1, '"x"');
            $this->assertEquals([4], $result);

            $getResult = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[[1,"x",2,3]]', $getResult);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrIndex()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [1, 2, 3, 2]}');

            $result = $this->valkey_glide->jsonArrIndex($key, '$.a', '2');
            $this->assertEquals([1], $result);

            $result = $this->valkey_glide->jsonArrIndex($key, '$.a', '99');
            $this->assertEquals([-1], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrLen()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [1, 2, 3]}');

            $result = $this->valkey_glide->jsonArrLen($key, '$.a');
            $this->assertEquals([3], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrPop()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [10, 20, 30]}');

            $result = $this->valkey_glide->jsonArrPop($key, '$.a');
            $this->assertEquals(['30'], $result);

            $this->valkey_glide->jsonSet($key, '$', '{"a": [10, 20, 30]}');
            $result = $this->valkey_glide->jsonArrPop($key, '$.a', 0);
            $this->assertEquals(['10'], $result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonArrTrim()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": [1, 2, 3, 4, 5]}');

            $result = $this->valkey_glide->jsonArrTrim($key, '$.a', 1, 3);
            $this->assertEquals([3], $result);

            $getResult = $this->valkey_glide->jsonGet($key, '$.a');
            $this->assertEquals('[[2,3,4]]', $getResult);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonResp()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1}');

            $result = $this->valkey_glide->jsonResp($key);
            $this->assertIsArray($result);
            $this->assertEquals('{', $result[0]);
            $this->assertEquals('a', $result[1][0]);
            $this->assertEquals(1, $result[1][1]);

            $result = $this->valkey_glide->jsonResp('non_existing_key_' . uniqid());
            $this->assertNull($result);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonDebugMemory()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": "hello"}');

            $result = $this->valkey_glide->jsonDebugMemory($key);
            $this->assertIsInt($result);
            $this->assertTrue($result > 0);

            $result = $this->valkey_glide->jsonDebugMemory($key, '$.a');
            $this->assertIsArray($result);
            $this->assertTrue($result[0] > 0);
        } finally {
            $this->valkey_glide->del($key);
        }
    }

    public function testJsonDebugFields()
    {
        $key = '{json}:' . uniqid();
        try {
            $this->valkey_glide->jsonSet($key, '$', '{"a": 1, "b": [1, 2, 3]}');

            $result = $this->valkey_glide->jsonDebugFields($key);
            $this->assertIsInt($result);
            $this->assertEquals(5, $result);

            $result = $this->valkey_glide->jsonDebugFields($key, '$.b');
            $this->assertIsArray($result);
            $this->assertEquals(3, $result[0]);
        } finally {
            $this->valkey_glide->del($key);
        }
    }
}
