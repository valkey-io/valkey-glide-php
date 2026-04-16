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
            $value2 = $this->valkey_glide->jsonGet($key, '$', $opts);
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
}
