<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die('Use TestValkeyGlide.php to run tests!\n');
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

require_once __DIR__ . '/ValkeyGlideBaseTest.php';


class ValkeyGlideTest extends ValkeyGlideBaseTest
{
    public function testMinimumVersion()
    {
        $this->assertTrue(version_compare($this->version, '2.4.0') >= 0);
    }

    public function testPing()
    {
        /* Reply literal off */
        $this->assertTrue($this->valkey_glide->ping());

        //$this->assertTrue($this->valkey_glide->ping(NULL));
        $this->assertEquals('BEEP', $this->valkey_glide->ping('BEEP'));
        return;
        /* Make sure we're good in MULTI mode */
        if ($this->haveMulti()) {
            $this->assertEquals(
                [true, 'BEEP'],
                $this->valkey_glide->multi()
                    ->ping()
                    ->ping('BEEP')
                    ->exec()
            );
        }
    }

/*    public function testPipelinePublish() {
        $ret = $this->valkey_glide->pipeline()
            ->publish('chan', 'msg')
            ->exec();

        $this->assertIsArray($ret, 1);
        $this->assertGT(-1, $ret[0] ?? -1);
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub() {
        // Only available since 2.8.0
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        // PUBSUB CHANNELS ...
        $result = $this->valkey_glide->pubsub('channels', '*');
        $this->assertIsArray($result);
        $result = $this->valkey_glide->pubsub('channels');
        $this->assertIsArray($result);

        // PUBSUB NUMSUB

        $c1 = uniqid();
        $c2 = uniqid();

        $result = $this->valkey_glide->pubsub('numsub', [$c1, $c2]);

        // Should get an array back, with two elements
        $this->assertIsArray($result);
        $this->assertEquals(2, count($result));

        // Make sure the elements are correct, and have zero counts
        foreach ([$c1,$c2] as $channel) {
            $this->assertArrayKeyEquals($result, $channel, 0);
        }

        // PUBSUB NUMPAT
        $result = $this->valkey_glide->pubsub('numpat');
        $this->assertIsInt($result);

        // Invalid calls
        $this->assertFalse(@$this->valkey_glide->pubsub('notacommand'));
        $this->assertFalse(@$this->valkey_glide->pubsub('numsub', 'not-an-array'));
    }*/

    /* These test cases were generated randomly.  We're just trying to test
       that PhpValkeyGlide handles all combination of arguments correctly. */
    public function testBitcount()
    {
        /* key */
        $this->valkey_glide->set('bitcountkey', hex2bin('bd906b854ca76cae'));
        $this->assertEquals(33, $this->valkey_glide->bitcount('bitcountkey'));

        /* key, start */
        $this->valkey_glide->set('bitcountkey', hex2bin('400aac171382a29bebaab554f178'));
        $this->assertEquals(4, $this->valkey_glide->bitcount('bitcountkey', 13));

        /* key, start, end */
        $this->valkey_glide->set('bitcountkey', hex2bin('b1f32405'));
        $this->assertEquals(2, $this->valkey_glide->bitcount('bitcountkey', 3, 3));

        /* key, start, end BYTE */
        $this->valkey_glide->set('bitcountkey', hex2bin('10eb8939e68bfdb640260f0629f3'));
        $this->assertEquals(1, $this->valkey_glide->bitcount('bitcountkey', 8, 8, false));

        if (!  $this->minVersionCheck('7.0')) {
            /* key, start, end, BIT */
            $this->valkey_glide->set('bitcountkey', hex2bin('cd0e4c80f9e4590d888a10'));
            $this->assertEquals(5, $this->valkey_glide->bitcount('bitcountkey', 0, 9, true));
        }
    }

    public function testBitop()
    {

        if (! $this->minVersionCheck('2.6.0')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->set('{key}1', 'foobar');
        $this->valkey_glide->set('{key}2', 'abcdef');

        // Regression test for GitHub issue #2210
        $this->assertEquals(6, $this->valkey_glide->bitop('AND', '{key}1', '{key}2'));

        // Make sure ValkeyGlideCluster doesn't even send the command.  We don't care
        // about what ValkeyGlide returns
        @$this->valkey_glide->bitop('AND', 'key1', 'key2', 'key3');

        $this->valkey_glide->del('{key}1', '{key}2');
    }

    public function testBitsets()
    {
        $this->valkey_glide->del('key');
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 0));
        $this->assertFalse($this->valkey_glide->getBit('key', -1));
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 100000));

        $this->valkey_glide->set('key', "\xff");
        for ($i = 0; $i < 8; $i++) {
            $this->assertEquals(1, $this->valkey_glide->getBit('key', $i));
        }
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 8));

        // change bit 0
        $this->assertEquals(1, $this->valkey_glide->setBit('key', 0, 0));
        $this->assertEquals(0, $this->valkey_glide->setBit('key', 0, 0));
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 0));
        $this->assertKeyEquals("\x7f", 'key');

        // change bit 1
        $this->assertEquals(1, $this->valkey_glide->setBit('key', 1, 0));
        $this->assertEquals(0, $this->valkey_glide->setBit('key', 1, 0));
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 1));
        $this->assertKeyEquals("\x3f", 'key');

        // change bit > 1
        $this->assertEquals(1, $this->valkey_glide->setBit('key', 2, 0));
        $this->assertEquals(0, $this->valkey_glide->setBit('key', 2, 0));
        $this->assertEquals(0, $this->valkey_glide->getBit('key', 2));
        $this->assertKeyEquals("\x1f", 'key');

        // values above 1 are changed to 1 but don't overflow on bits to the right.
        $this->assertEquals(0, $this->valkey_glide->setBit('key', 0, 0xff));
        $this->assertKeyEquals("\x9f", 'key');

        // Verify valid offset ranges
        $this->assertFalse($this->valkey_glide->getBit('key', -1));

        $this->valkey_glide->setBit('key', 0x7fffffff, 1);
        $this->assertEquals(1, $this->valkey_glide->getBit('key', 0x7fffffff));
    }

    public function testLcs()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped();
        }

        $key1 = '{lcs}1';
        $key2 = '{lcs}2';
        $this->assertTrue($this->valkey_glide->set($key1, '12244447777777'));
        $this->assertTrue($this->valkey_glide->set($key2, '6666662244441'));

        $this->assertEquals('224444', $this->valkey_glide->lcs($key1, $key2));

        $this->assertEquals(
            ['matches', [[[1, 6], [6, 11]]], 'len', 6],
            $this->valkey_glide->lcs($key1, $key2, ['idx'])
        );
        $this->assertEquals(
            ['matches', [[[1, 6], [6, 11], 6]], 'len', 6],
            $this->valkey_glide->lcs($key1, $key2, ['idx', 'withmatchlen'])
        );

        $this->assertEquals(6, $this->valkey_glide->lcs($key1, $key2, ['len']));

        $this->valkey_glide->del([$key1, $key2]);
    }

    public function testLmpop()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $key1 = '{l}1';
        $key2 = '{l}2';

        $this->valkey_glide->del($key1, $key2);

        $this->assertEquals(6, $this->valkey_glide->rpush($key1, 'A', 'B', 'C', 'D', 'E', 'F'));

        $this->assertEquals(6, $this->valkey_glide->rpush($key2, 'F', 'E', 'D', 'C', 'B', 'A'));

        $this->assertEquals([$key1, ['A']], $this->valkey_glide->lmpop([$key1, $key2], 'LEFT'));
        $this->assertEquals([$key1, ['F']], $this->valkey_glide->lmpop([$key1, $key2], 'RIGHT'));
        $this->assertEquals([$key1, ['B', 'C', 'D']], $this->valkey_glide->lmpop([$key1, $key2], 'LEFT', 3));

        $this->assertEquals(2, $this->valkey_glide->del($key1, $key2));
    }

    public function testBLmpop()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $key1 = '{bl}1';
        $key2 = '{bl}2';

        $this->valkey_glide->del($key1, $key2);

        $this->assertEquals(2, $this->valkey_glide->rpush($key1, 'A', 'B'));
        $this->assertEquals(2, $this->valkey_glide->rpush($key2, 'C', 'D'));

        $this->assertEquals([$key1, ['B', 'A']], $this->valkey_glide->blmpop(.2, [$key1, $key2], 'RIGHT', 2));
        $this->assertEquals([$key2, ['C']], $this->valkey_glide->blmpop(.2, [$key1, $key2], 'LEFT'));
        $this->assertEquals([$key2, ['D']], $this->valkey_glide->blmpop(.2, [$key1, $key2], 'LEFT'));

        $st = microtime(true);
        $this->assertFalse($this->valkey_glide->blmpop(.2, [$key1, $key2], 'LEFT'));
        $et = microtime(true);

        // Very loose tolerance because CI is run on a potato
        $this->assertBetween($et - $st, .05, .75);
    }

    public function testZmpop()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $key1 = '{z}1';
        $key2 = '{z}2';

        $this->valkey_glide->del($key1, $key2);

        $this->assertEquals(4, $this->valkey_glide->zadd($key1, 0, 'zero', 2, 'two', 4, 'four', 6, 'six'));
        $this->assertEquals(4, $this->valkey_glide->zadd($key2, 1, 'one', 3, 'three', 5, 'five', 7, 'seven'));

        $this->assertEquals([$key1, ['zero' => 0.0]], $this->valkey_glide->zmpop([$key1, $key2], 'MIN'));
        $this->assertEquals([$key1, ['six' => 6.0]], $this->valkey_glide->zmpop([$key1, $key2], 'MAX'));
        $this->assertEquals([$key1, ['two' => 2.0, 'four' => 4.0]], $this->valkey_glide->zmpop([$key1, $key2], 'MIN', 3));

        $this->assertEquals(
            [$key2, ['one' => 1.0, 'three' => 3.0, 'five' => 5.0, 'seven' => 7.0]],
            $this->valkey_glide->zmpop([$key1, $key2], 'MIN', 128)
        );

        $this->assertFalse($this->valkey_glide->zmpop([$key1, $key2], 'MIN'));

        return; // Set the option to return NULL for empty MULTIBULK
        $this->valkey_glide->setOption(ValkeyGlide::OPT_NULL_MULTIBULK_AS_NULL, true);
        $this->assertNull($this->valkey_glide->zmpop([$key1, $key2], 'MIN'));
        $this->valkey_glide->setOption(ValkeyGlide::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    public function testBZmpop()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $key1 = '{z}1';
        $key2 = '{z}2';

        $this->valkey_glide->del($key1, $key2);

        $this->assertEquals(2, $this->valkey_glide->zadd($key1, 0, 'zero', 2, 'two'));
        $this->assertEquals(2, $this->valkey_glide->zadd($key2, 1, 'one', 3, 'three'));

        $this->assertEquals(
            [$key1, ['zero' => 0.0, 'two' => 2.0]],
            $this->valkey_glide->bzmpop(.1, [$key1, $key2], 'MIN', 2)
        );

        $this->assertEquals([$key2, ['three' => 3.0]], $this->valkey_glide->bzmpop(.1, [$key1, $key2], 'MAX'));
        $this->assertEquals([$key2, ['one' => 1.0]], $this->valkey_glide->bzmpop(.1, [$key1, $key2], 'MAX'));

        $st = microtime(true);
        $this->assertFalse($this->valkey_glide->bzmpop(.2, [$key1, $key2], 'MIN'));
        $et = microtime(true);

        $this->assertBetween($et - $st, .05, .75);
    }

    public function testBitPos()
    {
        if (version_compare($this->version, '2.8.7') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->valkey_glide->del('bpkey');

        $this->valkey_glide->set('bpkey', "\xff\xf0\x00");
        $this->assertEquals(12, $this->valkey_glide->bitpos('bpkey', 0));

        $this->valkey_glide->set('bpkey', "\x00\xff\xf0");
        $this->assertEquals(8, $this->valkey_glide->bitpos('bpkey', 1, 0));
        $this->assertEquals(8, $this->valkey_glide->bitpos('bpkey', 1, 1));

        $this->valkey_glide->set('bpkey', "\x00\x00\x00");
        $this->assertEquals(-1, $this->valkey_glide->bitpos('bpkey', 1));

        if (! $this->minVersionCheck('7.0.0')) {
            return;
        }

        $this->valkey_glide->set('bpkey', "\xF");
        $this->assertEquals(4, $this->valkey_glide->bitpos('bpkey', 1, 0, -1, true));
                $this->assertEquals(-1, $this->valkey_glide->bitpos('bpkey', 1, 1, -1));
        $this->assertEquals(-1, $this->valkey_glide->bitpos('bpkey', 1, 1, -1, false));
    }

    public function testSetLargeKeys()
    {
        foreach ([1000, 100000, 1000000] as $size) {
            $value = str_repeat('A', $size);
            $this->assertTrue($this->valkey_glide->set('x', $value));
            $this->assertKeyEquals($value, 'x');
        }
    }

    public function testEcho()
    {
        $this->assertEquals('hello', $this->valkey_glide->echo('hello'));
        $this->assertEquals('', $this->valkey_glide->echo(''));
        $this->assertEquals(' 0123 ', $this->valkey_glide->echo(' 0123 '));
    }

    public function testErr()
    {
        $this->valkey_glide->set('x', '-ERR');
        $this->assertKeyEquals('-ERR', 'x');
    }

    public function testSet()
    {
        $this->assertTrue($this->valkey_glide->set('key', 'nil'));
        $this->assertKeyEquals('nil', 'key');

        $this->assertTrue($this->valkey_glide->set('key', 'val'));

        $this->assertKeyEquals('val', 'key');
        $this->assertKeyEquals('val', 'key');
        $this->valkey_glide->del('keyNotExist');
        $this->assertKeyMissing('keyNotExist');

        $this->valkey_glide->set('key2', 'val');
        $this->assertKeyEquals('val', 'key2');

        $value1 = bin2hex(random_bytes(rand(64, 128)));
        $value2 = random_bytes(rand(65536, 65536 * 2));
        ;

        $this->valkey_glide->set('key2', $value1);
        $this->assertKeyEquals($value1, 'key2');
        $this->assertKeyEquals($value1, 'key2');

        $this->valkey_glide->del('key');
        $this->valkey_glide->del('key2');


        $this->valkey_glide->set('key', $value2);
        $this->assertKeyEquals($value2, 'key');
        $this->valkey_glide->del('key');
        $this->assertKeyMissing('key');



        $data = gzcompress('42');
        $this->assertTrue($this->valkey_glide->set('key', $data));
        $this->assertEquals('42', gzuncompress($this->valkey_glide->get('key')));



        $this->valkey_glide->del('key');
        $data = gzcompress('value1');
        $this->assertTrue($this->valkey_glide->set('key', $data));
        $this->assertEquals('value1', gzuncompress($this->valkey_glide->get('key')));


        $this->valkey_glide->del('key');


        $this->assertTrue($this->valkey_glide->set('key', 0));


        $this->assertKeyEquals('0', 'key');
        $this->assertTrue($this->valkey_glide->set('key', 1));
        $this->assertKeyEquals('1', 'key');
        $this->assertTrue($this->valkey_glide->set('key', 0.1));
        $this->assertKeyEquals('0.1', 'key');


        $this->assertTrue($this->valkey_glide->set('key', '0.1'));
        $this->assertKeyEquals('0.1', 'key');
        $this->assertTrue($this->valkey_glide->set('key', true));
        $this->assertKeyEquals('1', 'key');


        $this->assertTrue($this->valkey_glide->set('key', ''));
        $this->assertKeyEquals('', 'key');
        $this->assertTrue($this->valkey_glide->set('key', null));
        $this->assertKeyEquals('', 'key');


        $this->assertTrue($this->valkey_glide->set('key', gzcompress('42')));
        $this->assertEquals('42', gzuncompress($this->valkey_glide->get('key')));
    }

    /* Extended SET options for ValkeyGlide >= 2.6.12 */
    public function testExtendedSet()
    {
        // Skip the test if we don't have a new enough version of ValkeyGlide
        if (version_compare($this->version, '2.6.12') < 0) {
            $this->markTestSkipped();
        }

        /* Legacy SETEX redirection */
        $this->valkey_glide->del('foo');
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', 20));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertEquals(20, $this->valkey_glide->ttl('foo'));

        /* Should coerce doubles into long */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar-20.5', 20.5));
        $this->assertEquals(20, $this->valkey_glide->ttl('foo'));
        $this->assertKeyEquals('bar-20.5', 'foo');

        /* Invalid third arguments */
        $this->assertFalse(@$this->valkey_glide->set('foo', 'bar', 'baz'));
        $this->assertFalse(@$this->valkey_glide->set('foo', 'bar', new StdClass()));

        /* Set if not exist */
        $this->valkey_glide->del('foo');
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['nx']));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertFalse($this->valkey_glide->set('foo', 'bar', ['nx']));

        /* Set if exists */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['xx']));
        $this->assertKeyEquals('bar', 'foo');
        $this->valkey_glide->del('foo');
        $this->assertFalse($this->valkey_glide->set('foo', 'bar', ['xx']));

        /* Set with a TTL */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['ex' => 100]));
        $this->assertEquals(100, $this->valkey_glide->ttl('foo'));

        /* Set with a PTTL */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['px' => 100000]));
        $this->assertBetween($this->valkey_glide->pttl('foo'), 99000, 100001);

        /* Set if exists, with a TTL */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['xx', 'ex' => 105]));
        $this->assertEquals(105, $this->valkey_glide->ttl('foo'));
        $this->assertKeyEquals('bar', 'foo');

        /* Set if not exists, with a TTL */
        $this->valkey_glide->del('foo');
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['nx', 'ex' => 110]));
        $this->assertEquals(110, $this->valkey_glide->ttl('foo'));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertFalse($this->valkey_glide->set('foo', 'bar', ['nx', 'ex' => 110]));

        /* Throw some nonsense into the array, and check that the TTL came through */
        $this->valkey_glide->del('foo');
        $this->assertTrue($this->valkey_glide->set('foo', 'barbaz', ['not-valid', 'nx', 'invalid', 'ex' => 200]));
        $this->assertEquals(200, $this->valkey_glide->ttl('foo'));
        $this->assertKeyEquals('barbaz', 'foo');

        /* Pass NULL as the optional arguments which should be ignored */
        $this->valkey_glide->del('foo');
        $this->valkey_glide->set('foo', 'bar', null);
        $this->assertKeyEquals('bar', 'foo');
        $this->assertLT(0, $this->valkey_glide->ttl('foo'));

        /* Make sure we ignore bad/non-string options (regression test for #1835) */
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', [null, 'EX' => 60]));
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', [null, new stdClass(), 'EX' => 60]));
        $this->assertFalse(@$this->valkey_glide->set('foo', 'bar', [null, 'EX' => []]));

        if (version_compare($this->version, '6.0.0') < 0) {
            return;
        }

        /* KEEPTTL works by itself */
        $this->valkey_glide->set('foo', 'bar', ['EX' => 100]);
        $this->valkey_glide->set('foo', 'bar', ['KEEPTTL']);
        $this->assertBetween($this->valkey_glide->ttl('foo'), 90, 100);

        /* Works with other options */
        $this->valkey_glide->set('foo', 'bar', ['XX', 'KEEPTTL']);
        $this->assertBetween($this->valkey_glide->ttl('foo'), 90, 100);
        $this->valkey_glide->set('foo', 'bar', ['XX']);
        $this->assertEquals(-1, $this->valkey_glide->ttl('foo'));

        if (version_compare($this->version, '6.2.0') < 0) {
            return;
        }

        $this->assertEquals('bar', $this->valkey_glide->set('foo', 'baz', ['GET']));
    }

    /* Test Valkey >= 8.1 IFEQ SET option */
    public function testValkeyIfEq()
    {
        if (! $this->is_valkey || ! $this->minVersionCheck('8.1.0')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('foo');
        $this->assertTrue($this->valkey_glide->set('foo', 'bar'));
        $this->assertTrue($this->valkey_glide->set('foo', 'bar2', ['IFEQ' => 'bar']));
        $this->assertFalse($this->valkey_glide->set('foo', 'bar4', ['IFEQ' => 'bar3']));

        $this->assertEquals('bar2', $this->valkey_glide->set('foo', 'bar3', ['IFEQ' => 'bar2', 'GET']));
    }

    public function testGetSet()
    {
        $this->valkey_glide->del('key');
        $this->assertFalse($this->valkey_glide->getSet('key', '42'));
        $this->assertEquals('42', $this->valkey_glide->getSet('key', '123'));
        $this->assertEquals('123', $this->valkey_glide->getSet('key', '123'));
    }

    public function testGetDel()
    {
        $this->valkey_glide->del('key');
        $this->assertTrue($this->valkey_glide->set('key', 'iexist'));
        $this->assertEquals('iexist', $this->valkey_glide->getDel('key'));
        $this->assertEquals(0, $this->valkey_glide->exists('key'));
    }

    public function testSetEXAT()
    {
        // EXAT option was introduced in Redis/Valkey 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        // Test EXAT with absolute Unix timestamp (2 minutes from now)
        $this->valkey_glide->del('foo');
        $future_timestamp = time() + 120;
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['EXAT' => $future_timestamp]));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertBetween($this->valkey_glide->ttl('foo'), 115, 120);

        // Test EXAT with NX option
        $this->valkey_glide->del('foo');
        $future_timestamp = time() + 180;
        $this->assertTrue($this->valkey_glide->set('foo', 'bar', ['NX', 'EXAT' => $future_timestamp]));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertBetween($this->valkey_glide->ttl('foo'), 175, 180);

        // Should fail with NX when key exists
        $this->assertFalse($this->valkey_glide->set('foo', 'baz', ['NX', 'EXAT' => time() + 200]));
        $this->assertKeyEquals('bar', 'foo'); // Value should remain unchanged

        // Test EXAT with XX option
        $future_timestamp = time() + 240;
        $this->assertTrue($this->valkey_glide->set('foo', 'updated', ['XX', 'EXAT' => $future_timestamp]));
        $this->assertKeyEquals('updated', 'foo');
        $this->assertBetween($this->valkey_glide->ttl('foo'), 235, 240);

        // Test EXAT with GET option (should return previous value)
        $future_timestamp = time() + 300;
        $this->assertEquals('updated', $this->valkey_glide->set('foo', 'final', ['GET', 'EXAT' => $future_timestamp]));
        $this->assertKeyEquals('final', 'foo');
        $this->assertBetween($this->valkey_glide->ttl('foo'), 295, 300);

        // Test EXAT with past timestamp (key should expire immediately)
        $past_timestamp = time() - 10;
        $this->assertTrue($this->valkey_glide->set('foo', 'expired', ['EXAT' => $past_timestamp]));
        // Key should be expired/not exist
        $this->assertFalse($this->valkey_glide->get('foo'));

        // Test invalid EXAT value (should fail)
        $this->assertFalse(@$this->valkey_glide->set('foo', 'bar', ['EXAT' => 'invalid']));
        $this->assertFalse(@$this->valkey_glide->set('foo', 'bar', ['EXAT' => -1]));

        // Clean up
        $this->valkey_glide->del('foo');
    }

    public function testRandomKey()
    {
        for ($i = 0; $i < 1000; $i++) {
            $k = $this->valkey_glide->randomKey();
            $this->assertKeyExists($k);
        }
    }

    public function testRename()
    {
        // strings
        $this->valkey_glide->del('{key}0');
        $this->valkey_glide->set('{key}0', 'val0');
        $this->valkey_glide->rename('{key}0', '{key}1');
        $this->assertKeyMissing('{key}0');
        $this->assertKeyEquals('val0', '{key}1');
    }

    public function testRenameNx()
    {
        // strings
        $this->valkey_glide->del('{key}0', '{key}1');
        $this->valkey_glide->set('{key}0', 'val0');
        $this->valkey_glide->set('{key}1', 'val1');
        $this->assertFalse($this->valkey_glide->renameNx('{key}0', '{key}1'));
        $this->assertKeyEquals('val0', '{key}0');
        $this->assertKeyEquals('val1', '{key}1');

        // lists
        $this->valkey_glide->del('{key}0');
        $this->valkey_glide->del('{key}1');
        $this->valkey_glide->lPush('{key}0', 'val0');
        $this->valkey_glide->lPush('{key}0', 'val1');
        $this->valkey_glide->lPush('{key}1', 'val1-0');
        $this->valkey_glide->lPush('{key}1', 'val1-1');
        $this->assertFalse($this->valkey_glide->renameNx('{key}0', '{key}1'));
        $this->assertEquals(['val1', 'val0'], $this->valkey_glide->lRange('{key}0', 0, -1));

        $this->assertEquals(['val1-1', 'val1-0'], $this->valkey_glide->lRange('{key}1', 0, -1));

        $this->valkey_glide->del('{key}2');
        $this->assertTrue($this->valkey_glide->renameNx('{key}0', '{key}2'));
        $this->assertEquals([], $this->valkey_glide->lRange('{key}0', 0, -1));
        $this->assertEquals(['val1', 'val0'], $this->valkey_glide->lRange('{key}2', 0, -1));
    }

    public function testMultiple1()
    {
        $kvals = [
            'mget1' => 'v1',
            'mget2' => 'v2',
            'mget3' => 'v3'
        ];

        $this->valkey_glide->mset($kvals);

        $this->valkey_glide->set(1, 'test');

        $this->assertEquals([$kvals['mget1']], $this->valkey_glide->mget(['mget1']));

        $this->assertEquals(['v1', 'v2', false], $this->valkey_glide->mget(['mget1', 'mget2', 'NoKey']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->valkey_glide->mget(['mget1', 'mget2', 'mget3']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->valkey_glide->mget(['mget1', 'mget2', 'mget3']));

        $this->valkey_glide->set('k5', '$1111111111');
        $this->assertEquals(['$1111111111'], $this->valkey_glide->mget(['k5']));

        $this->assertEquals(['test'], $this->valkey_glide->mget([1])); // non-string
    }

    public function testMultipleBin()
    {
        $kvals = [
            'binkey-1' => random_bytes(16),
            'binkey-2' => random_bytes(16),
            'binkey-3' => random_bytes(16),
        ];

        foreach ($kvals as $k => $v) {
            $this->valkey_glide->set($k, $v);
        }

        $this->assertEquals(
            array_values($kvals),
            $this->valkey_glide->mget(array_keys($kvals))
        );
    }



    public function testExpire()
    {
        $this->valkey_glide->del('key');
        $this->valkey_glide->set('key', 'value');

        $this->assertKeyEquals('value', 'key');
        $this->valkey_glide->expire('key', 1);
        $this->assertKeyEquals('value', 'key');
        sleep(2);
        $this->assertKeyMissing('key');
    }

    /* This test is prone to failure in the Travis container, so attempt to
       mitigate this by running more than once */
    public function testExpireAt()
    {
        $success = false;

        for ($i = 0; !$success && $i < 3; $i++) {
            $this->valkey_glide->del('key');
            $this->valkey_glide->set('key', 'value');
            $this->valkey_glide->expireAt('key', time() + 1);
            usleep(1500000);

            $success = false === $this->valkey_glide->get('key');
        }

        $this->assertTrue($success);
    }

    public function testExpireOptions()
    {
        if (! $this->minVersionCheck('7.0.0')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->set('eopts', 'value');

        /* NX -- Only if expiry isn't set so success, then failure */
        $this->assertTrue($this->valkey_glide->expire('eopts', 1000, 'NX'));

        $this->assertFalse($this->valkey_glide->expire('eopts', 1000, 'NX'));

        /* XX -- Only set if the key has an existing expiry */
        $this->assertTrue($this->valkey_glide->expire('eopts', 1000, 'XX'));

        $this->assertTrue($this->valkey_glide->persist('eopts'));

        $this->assertFalse($this->valkey_glide->expire('eopts', 1000, 'XX'));

        /* GT -- Only set when new expiry > current expiry */
        $this->assertTrue($this->valkey_glide->expire('eopts', 200));
        $this->assertTrue($this->valkey_glide->expire('eopts', 300, 'GT'));
        $this->assertFalse($this->valkey_glide->expire('eopts', 100, 'GT'));

        /* LT -- Only set when expiry < current expiry */
        $this->assertTrue($this->valkey_glide->expire('eopts', 200));
        $this->assertTrue($this->valkey_glide->expire('eopts', 100, 'LT'));
        $this->assertFalse($this->valkey_glide->expire('eopts', 300, 'LT'));

        /* Sending a nonsensical mode fails without sending a command */
        $this->assertFalse(@$this->valkey_glide->expire('eopts', 999, 'nonsense'));

        $this->valkey_glide->del('eopts');
    }

    public function testExpiretime()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $now = time();

        // Test expireat (existing test)
        $this->assertTrue($this->valkey_glide->set('key1', 'value'));
        $this->assertTrue($this->valkey_glide->expireat('key1', $now + 10));
        $this->assertEquals($now + 10, $this->valkey_glide->expiretime('key1'));
        $this->assertEquals(1000 * ($now + 10), $this->valkey_glide->pexpiretime('key1'));

        // Test pexpire (new)
        $this->assertTrue($this->valkey_glide->set('key2', 'value'));
        $this->assertTrue($this->valkey_glide->pexpire('key2', 15000)); // 15 seconds in ms
        $this->assertBetween($this->valkey_glide->expiretime('key2'), $now + 14, $now + 16);
        $this->assertBetween($this->valkey_glide->pexpiretime('key2'), ($now + 14) * 1000, ($now + 16) * 1000);

        // Test pexpireat (new)
        $future_ms = ($now + 20) * 1000;
        $this->assertTrue($this->valkey_glide->set('key3', 'value'));
        $this->assertTrue($this->valkey_glide->pexpireat('key3', $future_ms));
        $this->assertEquals($now + 20, $this->valkey_glide->expiretime('key3'));
        $this->assertEquals($future_ms, $this->valkey_glide->pexpiretime('key3'));

        $this->valkey_glide->del('key1', 'key2', 'key3');
    }

    public function testGetEx()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->assertTrue($this->valkey_glide->set('key', 'value'));

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['EX' => 100]));

        $this->assertBetween($this->valkey_glide->ttl('key'), 95, 100);

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['PX' => 100000]));
        $this->assertBetween($this->valkey_glide->pttl('key'), 95000, 100000);

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['EXAT' => time() + 200]));
        $this->assertBetween($this->valkey_glide->ttl('key'), 195, 200);

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['PXAT' => (time() * 1000) + 25000]));
        $this->assertBetween($this->valkey_glide->pttl('key'), 24000, 25000);

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['PERSIST' => true]));
        $this->assertEquals(-1, $this->valkey_glide->ttl('key'));

        $this->assertTrue($this->valkey_glide->expire('key', 100));
        $this->assertBetween($this->valkey_glide->ttl('key'), 95, 100);

        $this->assertEquals('value', $this->valkey_glide->getEx('key', ['PERSIST']));
        $this->assertEquals(-1, $this->valkey_glide->ttl('key'));
    }

    public function testSetEx()
    {
        $this->valkey_glide->del('key');
        $this->assertTrue($this->valkey_glide->setex('key', 7, 'val'));
        $this->assertEquals(7, $this->valkey_glide->ttl('key'));
        $this->assertKeyEquals('val', 'key');
    }

    public function testPSetEx()
    {
        $this->valkey_glide->del('key');
        $this->assertTrue($this->valkey_glide->psetex('key', 7 * 1000, 'val'));
        $this->assertEquals(7, $this->valkey_glide->ttl('key'));
        $this->assertKeyEquals('val', 'key');
    }

    public function testSetNX()
    {

        $this->valkey_glide->set('key', 42);
        $this->assertFalse($this->valkey_glide->setnx('key', 'err'));
        $this->assertKeyEquals('42', 'key');

        $this->valkey_glide->del('key');
        $this->assertTrue($this->valkey_glide->setnx('key', '42'));
        $this->assertKeyEquals('42', 'key');
    }

    public function testExpireAtWithLong()
    {
        if (PHP_INT_SIZE != 8) {
            $this->markTestSkipped('64 bits only');
        }

        $large_expiry = 3153600000;
        $this->valkey_glide->del('key');
        $this->assertTrue($this->valkey_glide->setex('key', $large_expiry, 'val'));
        $this->assertEquals($large_expiry, $this->valkey_glide->ttl('key'));
    }

    public function testIncr()
    {
        $this->valkey_glide->set('key', 0);

        $this->valkey_glide->incr('key');
        $this->assertKeyEqualsWeak(1, 'key');

        $this->valkey_glide->incr('key');
        $this->assertKeyEqualsWeak(2, 'key');

        $this->valkey_glide->incrBy('key', 3);
        $this->assertKeyEqualsWeak(5, 'key');

        $this->valkey_glide->incrBy('key', 1);
        $this->assertKeyEqualsWeak(6, 'key');

        $this->valkey_glide->incrBy('key', -1);
        $this->assertKeyEqualsWeak(5, 'key');

        $this->valkey_glide->incr('key', 5);
        $this->assertKeyEqualsWeak(10, 'key');

        $this->valkey_glide->del('key');

        $this->valkey_glide->set('key', 'abc');

        $this->valkey_glide->incr('key');
        $this->assertKeyEquals('abc', 'key');

        $this->valkey_glide->incr('key');
        $this->assertKeyEquals('abc', 'key');

        $this->valkey_glide->set('key', 0);
        $this->assertEquals(PHP_INT_MAX, $this->valkey_glide->incrby('key', PHP_INT_MAX));
    }

    public function testIncrByFloat()
    {
        // incrbyfloat is new in 2.6.0
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('key');

        $this->valkey_glide->set('key', 0);

        $this->valkey_glide->incrbyfloat('key', 1.5);
        $this->assertKeyEquals('1.5', 'key');

        $this->valkey_glide->incrbyfloat('key', 2.25);
        $this->assertKeyEquals('3.75', 'key');

        $this->valkey_glide->incrbyfloat('key', -2.25);
        $this->assertKeyEquals('1.5', 'key');

        $this->valkey_glide->set('key', 'abc');

        $this->valkey_glide->incrbyfloat('key', 1.5);
        $this->assertKeyEquals('abc', 'key');

        $this->valkey_glide->incrbyfloat('key', -1.5);
        $this->assertKeyEquals('abc', 'key');

        // Test with prefixing
       // $this->valkey_glide->setOption(ValkeyGlide::OPT_PREFIX, 'someprefix:');
        // TODO ADD this option to the test
        $this->valkey_glide->del('someprefix:key');
        $this->valkey_glide->incrbyfloat('someprefix:key', 1.8);
        $this->assertKeyEqualsWeak(1.8, 'someprefix:key');
        //$this->valkey_glide->setOption(ValkeyGlide::OPT_PREFIX, '');
        $this->assertKeyExists('someprefix:key');
        $this->valkey_glide->del('someprefix:key');
    }

    public function testDecr()
    {
        $this->valkey_glide->set('key', 5);

        $this->valkey_glide->decr('key');
        $this->assertKeyEqualsWeak(4, 'key');


        $this->valkey_glide->decr('key');
        $this->assertKeyEqualsWeak(3, 'key');

        $this->valkey_glide->decrBy('key', 2);
        $this->assertKeyEqualsWeak(1, 'key');

        $this->valkey_glide->decrBy('key', 1);
        $this->assertKeyEqualsWeak(0, 'key');

        $this->valkey_glide->decrBy('key', -10);
        $this->assertKeyEqualsWeak(10, 'key');

        $this->valkey_glide->decr('key', 10);
        $this->assertKeyEqualsWeak(0, 'key');
    }


    public function testExists()
    {
        /* Single key */
        $this->valkey_glide->del('key');
        $this->assertKeyMissing('key');
        $this->valkey_glide->set('key', 'val');
        $this->assertKeyExists('key');
        return;

        /* Add multiple keys */
        $mkeys = [];
        for ($i = 0; $i < 10; $i++) {
            if (rand(1, 2) == 1) {
                $mkey = "{exists}key:$i";
                $this->valkey_glide->set($mkey, $i);
                $mkeys[] = $mkey;
            }
        }

        /* Test passing an array as well as the keys variadic */
        $this->assertEquals(count($mkeys), $this->valkey_glide->exists($mkeys));
        if (count($mkeys)) {
            $this->assertEquals(count($mkeys), $this->valkey_glide->exists(...$mkeys));
        }
    }

    public function testTouch()
    {
        if (! $this->minVersionCheck('3.2.1')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('notakey');

        $this->assertTrue($this->valkey_glide->mset(['{idle}1' => 'beep', '{idle}2' => 'boop']));
        usleep(1100000);
        $this->assertGT(0, $this->valkey_glide->object('idletime', '{idle}1'));
        $this->assertGT(0, $this->valkey_glide->object('idletime', '{idle}2'));

        $this->assertEquals(2, $this->valkey_glide->touch('{idle}1', '{idle}2', '{idle}notakey'));

        $idle1 = $this->valkey_glide->object('idletime', '{idle}1');
        $idle2 = $this->valkey_glide->object('idletime', '{idle}2');

        /* We're not testing if idle is 0 because CPU scheduling on GitHub CI
         * potatoes can cause that to erroneously fail. */
        $this->assertLT(2, $idle1);
        $this->assertLT(2, $idle2);
    }



    protected function genericDelUnlink($cmd)
    {
        $key = uniqid('key:');
        $this->valkey_glide->set($key, 'val');
        $this->assertKeyEquals('val', $key);
        $this->assertEquals(1, $this->valkey_glide->$cmd($key));
        
        $this->assertFalse($this->valkey_glide->get($key));

        // multiple, all existing
        $this->valkey_glide->set('x', 0);
        $this->valkey_glide->set('y', 1);
        $this->valkey_glide->set('z', 2);
        $this->assertEquals(3, $this->valkey_glide->$cmd('x', 'y', 'z'));

        $this->assertFalse($this->valkey_glide->get('x'));
        $this->assertFalse($this->valkey_glide->get('y'));
        $this->assertFalse($this->valkey_glide->get('z'));

        // multiple, none existing
        $this->assertEquals(0, $this->valkey_glide->$cmd('x', 'y', 'z'));
        $this->assertFalse($this->valkey_glide->get('x'));
        $this->assertFalse($this->valkey_glide->get('y'));
        $this->assertFalse($this->valkey_glide->get('z'));

        // multiple, some existing
        $this->valkey_glide->set('y', 1);
        $this->assertEquals(1, $this->valkey_glide->$cmd('x', 'y', 'z'));
        $this->assertFalse($this->valkey_glide->get('y'));

        $this->valkey_glide->set('x', 0);
        $this->valkey_glide->set('y', 1);
        $this->assertEquals(2, $this->valkey_glide->$cmd(['x', 'y']));
    }

    public function testDelete()
    {
        $this->genericDelUnlink('DEL');
    }

    public function testUnlink()
    {
        if (version_compare($this->version, '4.0.0') < 0) {
            $this->markTestSkipped();
        }

        $this->genericDelUnlink('UNLINK');
    }

    public function testType()
    {
        // string
        $this->valkey_glide->set('key', 'val');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_STRING, $this->valkey_glide->type('key'));

        // list
        $this->valkey_glide->lPush('keyList', 'val0');
        $this->valkey_glide->lPush('keyList', 'val1');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_LIST, $this->valkey_glide->type('keyList'));

        // set
        $this->valkey_glide->del('keySet');
        $this->valkey_glide->sAdd('keySet', 'val0');
        $this->valkey_glide->sAdd('keySet', 'val1');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_SET, $this->valkey_glide->type('keySet'));

        // zset
        $this->valkey_glide->del('keyZSet');
        $this->valkey_glide->zAdd('keyZSet', 0, 'val0');
        $this->valkey_glide->zAdd('keyZSet', 1, 'val1');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_ZSET, $this->valkey_glide->type('keyZSet'));

        // hash
        $this->valkey_glide->del('keyHash');
        $this->valkey_glide->hSet('keyHash', 'key0', 'val0');
        $this->valkey_glide->hSet('keyHash', 'key1', 'val1');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_HASH, $this->valkey_glide->type('keyHash'));

        // stream
        if ($this->minVersionCheck('5.0')) {
            $this->valkey_glide->del('stream');
            $this->valkey_glide->xAdd('stream', '*', ['foo' => 'bar']);

            $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_STREAM, $this->valkey_glide->type('stream'));
        }

        // None
        $this->valkey_glide->del('keyNotExists');
        $this->assertEquals(ValkeyGlide::VALKEY_GLIDE_NOT_FOUND, $this->valkey_glide->type('keyNotExists'));
    }

    public function testStr()
    {
        $this->valkey_glide->set('key', 'val1');
        $this->assertEquals(8, $this->valkey_glide->append('key', 'val2'));
        $this->assertKeyEquals('val1val2', 'key');

        $this->valkey_glide->del('keyNotExist');
        $this->assertEquals(5, $this->valkey_glide->append('keyNotExist', 'value'));
        $this->assertKeyEquals('value', 'keyNotExist');

        $this->valkey_glide->set('key', 'This is a string') ;
        $this->assertEquals('This', $this->valkey_glide->getRange('key', 0, 3));
        $this->assertEquals('string', $this->valkey_glide->getRange('key', -6, -1));
        $this->assertEquals('string', $this->valkey_glide->getRange('key', -6, 100000));
        $this->assertKeyEquals('This is a string', 'key');

        $this->valkey_glide->set('key', 'This is a string') ;
        $this->assertEquals(16, $this->valkey_glide->strlen('key'));

        $this->valkey_glide->set('key', 10) ;
        $this->assertEquals(2, $this->valkey_glide->strlen('key'));
        $this->valkey_glide->set('key', '') ;
        $this->assertEquals(0, $this->valkey_glide->strlen('key'));
        $this->valkey_glide->set('key', '000') ;
        $this->assertEquals(3, $this->valkey_glide->strlen('key'));
    }

    public function testlPop()
    {
        $this->valkey_glide->del('list');

        $this->valkey_glide->lPush('list', 'val');
        $this->valkey_glide->lPush('list', 'val2');
        $this->valkey_glide->rPush('list', 'val3');

        $this->assertEquals('val2', $this->valkey_glide->lPop('list'));
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->assertEquals('val', $this->valkey_glide->lPop('list'));
            $this->assertEquals('val3', $this->valkey_glide->lPop('list'));
        } else {
            $this->assertEquals(['val', 'val3'], $this->valkey_glide->lPop('list', 2));
        }

        $this->assertFalse($this->valkey_glide->lPop('list'));

        $this->valkey_glide->del('list');
        $this->assertEquals(1, $this->valkey_glide->lPush('list', gzcompress('val1')));
        $this->assertEquals(2, $this->valkey_glide->lPush('list', gzcompress('val2')));
        $this->assertEquals(3, $this->valkey_glide->lPush('list', gzcompress('val3')));

        $this->assertEquals('val3', gzuncompress($this->valkey_glide->lPop('list')));
        $this->assertEquals('val2', gzuncompress($this->valkey_glide->lPop('list')));
        $this->assertEquals('val1', gzuncompress($this->valkey_glide->lPop('list')));
    }

    public function testrPop()
    {
        $this->valkey_glide->del('list');

        $this->valkey_glide->rPush('list', 'val');
        $this->valkey_glide->rPush('list', 'val2');
        $this->valkey_glide->lPush('list', 'val3');

        $this->assertEquals('val2', $this->valkey_glide->rPop('list'));
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->assertEquals('val', $this->valkey_glide->rPop('list'));
            $this->assertEquals('val3', $this->valkey_glide->rPop('list'));
        } else {
            $this->assertEquals(['val', 'val3'], $this->valkey_glide->rPop('list', 2));
        }

        $this->assertFalse($this->valkey_glide->rPop('list'));

        $this->valkey_glide->del('list');
        $this->assertEquals(1, $this->valkey_glide->rPush('list', gzcompress('val1')));
        $this->assertEquals(2, $this->valkey_glide->rPush('list', gzcompress('val2')));
        $this->assertEquals(3, $this->valkey_glide->rPush('list', gzcompress('val3')));

        $this->assertEquals('val3', gzuncompress($this->valkey_glide->rPop('list')));
        $this->assertEquals('val2', gzuncompress($this->valkey_glide->rPop('list')));
        $this->assertEquals('val1', gzuncompress($this->valkey_glide->rPop('list')));
    }



    public function testblockingPop()
    {
        /* Test with a double timeout in ValkeyGlide >= 6.0.0 */
        if (version_compare($this->version, '6.0.0') >= 0) {
            $this->valkey_glide->del('list');
            $this->valkey_glide->lpush('list', 'val1', 'val2');
            $this->assertEquals(['list', 'val2'], $this->valkey_glide->blpop(['list'], .1));
            $this->assertEquals(['list', 'val1'], $this->valkey_glide->blpop(['list'], .1));
        }

        // non blocking blPop, brPop
        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'val1', 'val2');
        $this->assertEquals(['list', 'val2'], $this->valkey_glide->blPop(['list'], 2));
        $this->assertEquals(['list', 'val1'], $this->valkey_glide->blPop(['list'], 2));

        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'val1', 'val2');
        $this->assertEquals(['list', 'val1'], $this->valkey_glide->brPop(['list'], 1));
        $this->assertEquals(['list', 'val2'], $this->valkey_glide->brPop(['list'], 1));

        // blocking blpop, brpop
        $this->valkey_glide->del('list');
        return; //TODO: fix this test
        /* Also test our option that we want *-1 to be returned as NULL */
        foreach ([false => [], true => null] as $opt => $val) {
            $this->valkey_glide->setOption(ValkeyGlide::OPT_NULL_MULTIBULK_AS_NULL, $opt);
            $this->assertEquals($val, $this->valkey_glide->blPop(['list'], 1));
            $this->assertEquals($val, $this->valkey_glide->brPop(['list'], 1));
        }

        $this->valkey_glide->setOption(ValkeyGlide::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    public function testLLen()
    {
        $this->valkey_glide->del('list');

        $this->valkey_glide->lPush('list', 'val');
        $this->assertEquals(1, $this->valkey_glide->llen('list'));

        $this->valkey_glide->lPush('list', 'val2');
        $this->assertEquals(2, $this->valkey_glide->llen('list'));

        $this->assertEquals('val2', $this->valkey_glide->lPop('list'));
        $this->assertEquals(1, $this->valkey_glide->llen('list'));

        $this->assertEquals('val', $this->valkey_glide->lPop('list'));
        $this->assertEquals(0, $this->valkey_glide->llen('list'));

        $this->assertFalse($this->valkey_glide->lPop('list'));
        $this->assertEquals(0, $this->valkey_glide->llen('list'));    // empty returns 0

        $this->valkey_glide->del('list');
        $this->assertEquals(0, $this->valkey_glide->llen('list'));    // non-existent returns 0

        $this->valkey_glide->set('list', 'actually not a list');
        $this->assertFalse($this->valkey_glide->llen('list'));// not a list returns FALSE
    }

    public function testlPopx()
    {
        $this->valkey_glide->del('keyNotExists');
        $this->assertEquals(0, $this->valkey_glide->lPushx('keyNotExists', 'value'));
        $this->assertEquals(0, $this->valkey_glide->rPushx('keyNotExists', 'value'));

        $this->valkey_glide->del('key');
        $this->valkey_glide->lPush('key', 'val0');
        $this->assertEquals(2, $this->valkey_glide->lPushx('key', 'val1'));
        $this->assertEquals(3, $this->valkey_glide->rPushx('key', 'val2'));
        $this->assertEquals(['val1', 'val0', 'val2'], $this->valkey_glide->lrange('key', 0, -1));

        //test linsert
        $this->valkey_glide->del('key');
        $this->valkey_glide->lPush('key', 'val0');
        $this->assertEquals(0, $this->valkey_glide->lInsert('keyNotExists', ValkeyGlide::AFTER, 'val1', 'val2'));
        $this->assertEquals(-1, $this->valkey_glide->lInsert('key', ValkeyGlide::BEFORE, 'valX', 'val2'));

        $this->assertEquals(2, $this->valkey_glide->lInsert('key', ValkeyGlide::AFTER, 'val0', 'val1'));
        $this->assertEquals(3, $this->valkey_glide->lInsert('key', ValkeyGlide::BEFORE, 'val0', 'val2'));
        $this->assertEquals(['val2', 'val0', 'val1'], $this->valkey_glide->lrange('key', 0, -1));
    }

    public function testlPos()
    {
        $this->valkey_glide->del('key');
        $this->valkey_glide->lPush('key', 'val0', 'val1', 'val1');
        $this->assertEquals(2, $this->valkey_glide->lPos('key', 'val0'));
        $this->assertEquals(0, $this->valkey_glide->lPos('key', 'val1'));
        $this->assertEquals(1, $this->valkey_glide->lPos('key', 'val1', ['rank' => 2]));
        $this->assertEquals([0, 1], $this->valkey_glide->lPos('key', 'val1', ['count' => 2]));
        $this->assertEquals([0], $this->valkey_glide->lPos('key', 'val1', ['count' => 2, 'maxlen' => 1]));
        $this->assertEquals([], $this->valkey_glide->lPos('key', 'val2', ['count' => 1]));

        return; //TODO: fix this test
        foreach ([[true, null], [false, false]] as $optpack) {
            list ($setting, $expected) = $optpack;
            $this->valkey_glide->setOption(ValkeyGlide::OPT_NULL_MULTIBULK_AS_NULL, $setting);
            $this->assertEquals($expected, $this->valkey_glide->lPos('key', 'val2'));
        }
    }

    // ltrim, lLen, lpop
    public function testltrim()
    {
        $this->valkey_glide->del('list');

        $this->valkey_glide->lPush('list', 'val');
        $this->valkey_glide->lPush('list', 'val2');
        $this->valkey_glide->lPush('list', 'val3');
        $this->valkey_glide->lPush('list', 'val4');

        $this->assertTrue($this->valkey_glide->ltrim('list', 0, 2));
        $this->assertEquals(3, $this->valkey_glide->llen('list'));

        $this->valkey_glide->ltrim('list', 0, 0);
        $this->assertEquals(1, $this->valkey_glide->llen('list'));
        $this->assertEquals('val4', $this->valkey_glide->lPop('list'));

        $this->assertTrue($this->valkey_glide->ltrim('list', 10, 10000));
        $this->assertTrue($this->valkey_glide->ltrim('list', 10000, 10));

        // test invalid type
        $this->valkey_glide->set('list', 'not a list...');
        $this->assertFalse($this->valkey_glide->ltrim('list', 0, 2));
    }

    public function setupSort()
    {
        // people with name, age, salary
        $this->valkey_glide->set('person:name_1', 'Alice');
        $this->valkey_glide->set('person:age_1', 27);
        $this->valkey_glide->set('person:salary_1', 2500);

        $this->valkey_glide->set('person:name_2', 'Bob');
        $this->valkey_glide->set('person:age_2', 34);
        $this->valkey_glide->set('person:salary_2', 2000);

        $this->valkey_glide->set('person:name_3', 'Carol');
        $this->valkey_glide->set('person:age_3', 25);
        $this->valkey_glide->set('person:salary_3', 2800);

        $this->valkey_glide->set('person:name_4', 'Dave');
        $this->valkey_glide->set('person:age_4', 41);
        $this->valkey_glide->set('person:salary_4', 3100);

        // set-up
        $this->valkey_glide->del('person:id');
        foreach ([1, 2, 3, 4] as $id) {
            $this->valkey_glide->lPush('person:id', $id);
        }
    }

    public function testSortPrefix()
    {

        // Make sure that sorting works with a prefix
        $this->valkey_glide->del('some-item');
        $this->valkey_glide->sadd('some-item', 1);
        $this->valkey_glide->sadd('some-item', 2);
        $this->valkey_glide->sadd('some-item', 3);

        $this->assertEquals(['1', '2', '3'], $this->valkey_glide->sort('some-item', ['sort' => 'asc']));
        $this->assertEquals(['3', '2', '1'], $this->valkey_glide->sort('some-item', ['sort' => 'desc']));
        $this->assertEquals(['1', '2', '3'], $this->valkey_glide->sort('some-item'));

        // Kill our set/prefix
        $this->valkey_glide->del('some-item');
    }

    public function testSortAsc()
    {
        $this->setupSort();
        // sort by age and get IDs
        $byAgeAsc = ['3', '1', '2', '4'];
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*']));
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'sort' => 'asc']));
        $this->assertEquals(['1', '2', '3', '4'], $this->valkey_glide->sort('person:id', ['by' => null]));   // check that NULL works.
        $this->assertEquals(['1', '2', '3', '4'], $this->valkey_glide->sort('person:id', ['by' => null, 'get' => null])); // for all fields.
        $this->assertEquals(['1', '2', '3', '4'], $this->valkey_glide->sort('person:id', ['sort' => 'asc']));

        // sort by age and get names
        $byAgeAsc = ['Carol', 'Alice', 'Bob', 'Dave'];
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*']));
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2], 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2], 'sort' => 'asc']));
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 4]]));
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, '4']])); // with strings
        $this->assertEquals($byAgeAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => ['0', 4]]));

        // sort by salary and get ages
        $agesBySalaryAsc = ['34', '27', '25', '41'];
        $this->assertEquals($agesBySalaryAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*']));
        $this->assertEquals($agesBySalaryAsc, $this->valkey_glide->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*', 'sort' => 'asc']));

        $agesAndSalaries = $this->valkey_glide->sort('person:id', ['by' => 'person:salary_*', 'get' => ['person:age_*', 'person:salary_*'], 'sort' => 'asc']);
        $this->assertEquals(['34', '2000', '27', '2500', '25', '2800', '41', '3100'], $agesAndSalaries);

        // sort non-alpha doesn't change all-string lists
        // list → [ghi, def, abc]
        $list = ['abc', 'def', 'ghi'];
        $this->valkey_glide->del('list');
        foreach ($list as $i) {
            $this->valkey_glide->lPush('list', $i);
        }

        // SORT list → [ghi, def, abc]
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->assertEquals(array_reverse($list), $this->valkey_glide->sort('list'));
            $this->assertEquals(array_reverse($list), $this->valkey_glide->sort('list', ['sort' => 'asc']));
        } else {
            // TODO rewrite, from 2.6.0 release notes:
            // SORT now will refuse to sort in numerical mode elements that can't be parsed
            // as numbers
        }

        // SORT list ALPHA → [abc, def, ghi]
        $this->assertEquals($list, $this->valkey_glide->sort('list', ['alpha' => true]));
        $this->assertEquals($list, $this->valkey_glide->sort('list', ['sort' => 'asc', 'alpha' => true]));
    }

    public function testSortDesc()
    {
        $this->setupSort();

        // sort by age and get IDs
        $byAgeDesc = ['4', '2', '1', '3'];
        $this->assertEquals($byAgeDesc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'sort' => 'desc']));

        // sort by age and get names
        $byAgeDesc = ['Dave', 'Bob', 'Alice', 'Carol'];
        $this->assertEquals($byAgeDesc, $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'desc']));

        $this->assertEquals(array_slice($byAgeDesc, 0, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2], 'sort' => 'desc']));
        $this->assertEquals(array_slice($byAgeDesc, 1, 2), $this->valkey_glide->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2], 'sort' => 'desc']));

        // sort by salary and get ages
        $agesBySalaryDesc = ['41', '25', '27', '34'];
        $this->assertEquals($agesBySalaryDesc, $this->valkey_glide->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*', 'sort' => 'desc']));

        // sort non-alpha doesn't change all-string lists
        $list = ['def', 'abc', 'ghi'];
        $this->valkey_glide->del('list');
        foreach ($list as $i) {
            $this->valkey_glide->lPush('list', $i);
        }

        // SORT list ALPHA → [abc, def, ghi]
        $this->assertEquals(['ghi', 'def', 'abc'], $this->valkey_glide->sort('list', ['sort' => 'desc', 'alpha' => true]));
    }

    /* This test is just to make sure SORT and SORT_RO are both callable */
    public function testSortHandler()
    {
        $this->valkey_glide->del('list');

        $this->valkey_glide->rpush('list', 'c', 'b', 'a');

        $methods = ['sort'];
        if ($this->minVersionCheck('7.0.0')) {
            $methods[] = 'sort_ro';
        }

        foreach ($methods as $method) {
            $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->$method('list', ['sort' => 'asc', 'alpha' => true]));
        }
    }

    public function testLindex()
    {

        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'val');

        $this->valkey_glide->lPush('list', 'val2');

        $this->valkey_glide->lPush('list', 'val3');

        $this->assertEquals('val3', $this->valkey_glide->lIndex('list', 0));
        $this->assertEquals('val2', $this->valkey_glide->lIndex('list', 1));
        $this->assertEquals('val', $this->valkey_glide->lIndex('list', 2));
        $this->assertEquals('val', $this->valkey_glide->lIndex('list', -1));
        $this->assertEquals('val2', $this->valkey_glide->lIndex('list', -2));
        $this->assertEquals('val3', $this->valkey_glide->lIndex('list', -3));
        $this->assertFalse($this->valkey_glide->lIndex('list', -4));

        $this->valkey_glide->rPush('list', 'val4');
        $this->assertEquals('val4', $this->valkey_glide->lIndex('list', 3));
        $this->assertEquals('val4', $this->valkey_glide->lIndex('list', -1));
    }

    public function testlMove()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        [$list1, $list2] = ['{l}0', '{l}1'];
        $left  = $this->getLeftConstant();
        $right = $this->getRightConstant();

        $this->valkey_glide->del($list1, $list2);
        $this->valkey_glide->lPush($list1, 'a');
        $this->valkey_glide->lPush($list1, 'b');
        $this->valkey_glide->lPush($list1, 'c');

        $return = $this->valkey_glide->lMove($list1, $list2, $left, $right);
        $this->assertEquals('c', $return);

        $return = $this->valkey_glide->lMove($list1, $list2, $right, $left);
        $this->assertEquals('a', $return);

        $this->assertEquals(['b'], $this->valkey_glide->lRange($list1, 0, -1));
        $this->assertEquals(['a', 'c'], $this->valkey_glide->lRange($list2, 0, -1));
    }

    public function testMove()
    {
        // Version check if needed (move has been available since early Redis versions)

        $key1 = 'move_test_key1';
        $key2 = 'move_test_key2';
        $value1 = 'test_value1';
        $value2 = 'test_value2';

        // Ensure we're in database 0
        $this->valkey_glide->select(0);

        // Clean up any existing keys
        $this->valkey_glide->del($key1, $key2);
        $this->valkey_glide->select(1);
        $this->valkey_glide->del($key1, $key2);
        $this->valkey_glide->select(0);

        // Test successful move
        $this->valkey_glide->set($key1, $value1);
        $this->assertTrue($this->valkey_glide->move($key1, 1));

        // Verify key moved
        $this->assertEquals(0, $this->valkey_glide->exists($key1)); // Gone from db 0
        $this->valkey_glide->select(1);
        $this->assertKeyEquals($value1, $key1); // Present in db 1
    }

    public function testBlmove()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        [$list1, $list2] = ['{l}0', '{l}1'];
        $left = $this->getLeftConstant();

        $this->valkey_glide->del($list1, $list2);
        $this->valkey_glide->rpush($list1, 'a');


        $this->assertEquals('a', $this->valkey_glide->blmove($list1, $list2, $left, $left, 1.));

        $st = microtime(true);
        $ret = $this->valkey_glide->blmove($list1, $list2, $left, $left, .1);
        $et = microtime(true);

        $this->assertFalse($ret);
        $this->assertGT(.09, $et - $st);
    }

    // lRem testing
    public function testLRem()
    {
        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'a');
        $this->valkey_glide->lPush('list', 'b');
        $this->valkey_glide->lPush('list', 'c');
        $this->valkey_glide->lPush('list', 'c');
        $this->valkey_glide->lPush('list', 'b');
        $this->valkey_glide->lPush('list', 'c');

        // ['c', 'b', 'c', 'c', 'b', 'a']
        $return = $this->valkey_glide->lrem('list', 'b', 2);
        // ['c', 'c', 'c', 'a']
        $this->assertEquals(2, $return);
        $this->assertEquals('c', $this->valkey_glide->lIndex('list', 0));
        $this->assertEquals('c', $this->valkey_glide->lIndex('list', 1));
        $this->assertEquals('c', $this->valkey_glide->lIndex('list', 2));
        $this->assertEquals('a', $this->valkey_glide->lIndex('list', 3));

        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'a');
        $this->valkey_glide->lPush('list', 'b');
        $this->valkey_glide->lPush('list', 'c');
        $this->valkey_glide->lPush('list', 'c');
        $this->valkey_glide->lPush('list', 'b');
        $this->valkey_glide->lPush('list', 'c');

        // ['c', 'b', 'c', 'c', 'b', 'a']
        $this->valkey_glide->lrem('list', 'c', -2);
        // ['c', 'b', 'b', 'a']
        $this->assertEquals(2, $return);
        $this->assertEquals('c', $this->valkey_glide->lIndex('list', 0));
        $this->assertEquals('b', $this->valkey_glide->lIndex('list', 1));
        $this->assertEquals('b', $this->valkey_glide->lIndex('list', 2));
        $this->assertEquals('a', $this->valkey_glide->lIndex('list', 3));

        // remove each element
        $this->assertEquals(1, $this->valkey_glide->lrem('list', 'a', 0));
        $this->assertEquals(0, $this->valkey_glide->lrem('list', 'x', 0));
        $this->assertEquals(2, $this->valkey_glide->lrem('list', 'b', 0));
        $this->assertEquals(1, $this->valkey_glide->lrem('list', 'c', 0));
        $this->assertFalse($this->valkey_glide->get('list'));

        $this->valkey_glide->set('list', 'actually not a list');
        $this->assertFalse($this->valkey_glide->lrem('list', 'x'));
    }

    public function testSAdd()
    {
        $this->valkey_glide->del('set');

        $this->assertEquals(1, $this->valkey_glide->sAdd('set', 'val'));
        $this->assertEquals(0, $this->valkey_glide->sAdd('set', 'val'));

        $this->assertTrue($this->valkey_glide->sismember('set', 'val'));
        $this->assertFalse($this->valkey_glide->sismember('set', 'val2'));

        $this->assertEquals(1, $this->valkey_glide->sAdd('set', 'val2'));

        $this->assertTrue($this->valkey_glide->sismember('set', 'val2'));
    }

    public function testSCard()
    {
        $this->valkey_glide->del('set');
        $this->assertEquals(1, $this->valkey_glide->sAdd('set', 'val'));
        $this->assertEquals(1, $this->valkey_glide->scard('set'));
        $this->assertEquals(1, $this->valkey_glide->sAdd('set', 'val2'));
        $this->assertEquals(2, $this->valkey_glide->scard('set'));
    }

    public function testSRem()
    {
        $this->valkey_glide->del('set');
        $this->valkey_glide->sAdd('set', 'val');
        $this->valkey_glide->sAdd('set', 'val2');
        $this->valkey_glide->srem('set', 'val');
        $this->assertEquals(1, $this->valkey_glide->scard('set'));
        $this->valkey_glide->srem('set', 'val2');
        $this->assertEquals(0, $this->valkey_glide->scard('set'));
    }

    public function testsMove()
    {
        $this->valkey_glide->del('{set}0');
        $this->valkey_glide->del('{set}1');

        $this->valkey_glide->sAdd('{set}0', 'val');
        $this->valkey_glide->sAdd('{set}0', 'val2');

        $this->assertTrue($this->valkey_glide->sMove('{set}0', '{set}1', 'val'));
        $this->assertFalse($this->valkey_glide->sMove('{set}0', '{set}1', 'val'));
        $this->assertFalse($this->valkey_glide->sMove('{set}0', '{set}1', 'val-what'));

        $this->assertEquals(1, $this->valkey_glide->scard('{set}0'));
        $this->assertEquals(1, $this->valkey_glide->scard('{set}1'));

        $this->assertEquals(['val2'], $this->valkey_glide->smembers('{set}0'));
        $this->assertEquals(['val'], $this->valkey_glide->smembers('{set}1'));
    }

    public function testsPop()
    {
        $this->valkey_glide->del('set0');
        $this->assertFalse($this->valkey_glide->sPop('set0'));

        $this->valkey_glide->sAdd('set0', 'val');
        $this->valkey_glide->sAdd('set0', 'val2');

        $v0 = $this->valkey_glide->sPop('set0');
        $this->assertEquals(1, $this->valkey_glide->scard('set0'));
        $this->assertInArray($v0, ['val', 'val2']);
        $v1 = $this->valkey_glide->sPop('set0');
        $this->assertEquals(0, $this->valkey_glide->scard('set0'));
        $this->assertEqualsCanonicalizing(['val', 'val2'], [$v0, $v1]);

        $this->assertFalse($this->valkey_glide->sPop('set0'));
    }

    public function testsPopWithCount()
    {
        if (! $this->minVersionCheck('3.2')) {
            $this->markTestSkipped();
        }

        $set = 'set0';
        $prefix = 'member';
        $count = 5;

        /* Add a few members */
        $this->valkey_glide->del($set);
        for ($i = 0; $i < $count; $i++) {
            $this->valkey_glide->sadd($set, $prefix . $i);
        }

        /* Pop them all */
        $ret = $this->valkey_glide->sPop($set, $i);

        /* Make sure we got an arary and the count is right */
        if ($this->assertIsArray($ret, $count)) {
            /* Probably overkill but validate the actual returned members */
            for ($i = 0; $i < $count; $i++) {
                $this->assertInArray($prefix . $i, $ret);
            }
        }
    }

    public function testsRandMember()
    {
        $this->valkey_glide->del('set0');
        $this->assertFalse($this->valkey_glide->sRandMember('set0'));

        $this->valkey_glide->sAdd('set0', 'val');
        $this->valkey_glide->sAdd('set0', 'val2');

        $got = [];
        while (true) {
            $v = $this->valkey_glide->sRandMember('set0');
            $this->assertEquals(2, $this->valkey_glide->scard('set0')); // no change.
            $this->assertInArray($v, ['val', 'val2']);

            $got[$v] = $v;
            if (count($got) == 2) {
                break;
            }
        }

        //
        // With and without count, while serializing
        //

        $this->valkey_glide->del('set0');

        for ($i = 0; $i < 5; $i++) {
            $member = "member:$i";
            $this->valkey_glide->sAdd('set0', $member);
            $mems[] = $member;
        }

        $member = $this->valkey_glide->srandmember('set0');
        $this->assertInArray($member, $mems);

        $rmembers = $this->valkey_glide->srandmember('set0', $i);
        foreach ($rmembers as $reply_mem) {
            $this->assertInArray($reply_mem, $mems);
        }

        /* Ensure we can handle basically any return type */
        // TODO support new stdClass(),
        foreach ([3.1415, 42, 'hello', null] as $val) {
            $this->assertEquals(1, $this->valkey_glide->del('set0'));
            $this->assertEquals(1, $this->valkey_glide->sadd('set0', $val));
            //$this->assertSameType($val, $this->valkey_glide->srandmember('set0'));  TODO
        }
    }

    public function testSRandMemberWithCount()
    {
        // Make sure the set is nuked
        $this->valkey_glide->del('set0');

        // Run with a count (positive and negative) on an empty set
        $ret_pos = $this->valkey_glide->sRandMember('set0', 10);
        $ret_neg = $this->valkey_glide->sRandMember('set0', -10);

        // Should both be empty arrays
        $this->assertEquals([], $ret_pos);
        $this->assertEquals([], $ret_neg);

        // Add a few items to the set
        for ($i = 0; $i < 100; $i++) {
            $this->valkey_glide->sadd('set0', "member$i");
        }

        // Get less than the size of the list
        $ret_slice = $this->valkey_glide->srandmember('set0', 20);

        // Should be an array with 20 items
        $this->assertIsArray($ret_slice, 20);

        // Ask for more items than are in the list (but with a positive count)
        $ret_slice = $this->valkey_glide->srandmember('set0', 200);

        // Should be an array, should be however big the set is, exactly
        $this->assertIsArray($ret_slice, $i);

        // Now ask for too many items but negative
        $ret_slice = $this->valkey_glide->srandmember('set0', -200);

        // Should be an array, should have exactly the # of items we asked for (will be dups)
        $this->assertIsArray($ret_slice, 200);

        //
        // Test in a pipeline
        //

        if ($this->havePipeline()) {
            $pipe = $this->valkey_glide->pipeline();

            $pipe->srandmember('set0', 20);
            $pipe->srandmember('set0', 200);
            $pipe->srandmember('set0', -200);

            $ret = $this->valkey_glide->exec();

            $this->assertIsArray($ret[0], 20);
            $this->assertIsArray($ret[1], $i);
            $this->assertIsArray($ret[2], 200);

            // Kill the set
            $this->valkey_glide->del('set0');
        }
    }

    public function testSIsMember()
    {
        $this->valkey_glide->del('set');

        $this->valkey_glide->sAdd('set', 'val');

        $this->assertTrue($this->valkey_glide->sismember('set', 'val'));
        $this->assertFalse($this->valkey_glide->sismember('set', 'val2'));
    }

    public function testSMembers()
    {
        $this->valkey_glide->del('set');

        $data = ['val', 'val2', 'val3'];
        foreach ($data as $member) {
            $this->valkey_glide->sAdd('set', $member);
        }

        $this->assertEqualsCanonicalizing($data, $this->valkey_glide->smembers('set'));
    }

    public function testsMisMember()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('set');

        $this->valkey_glide->sAdd('set', 'val');
        $this->valkey_glide->sAdd('set', 'val2');
        $this->valkey_glide->sAdd('set', 'val3');

        $misMembers = $this->valkey_glide->sMisMember('set', 'val', 'notamember', 'val3');
        $this->assertEquals([true, false, true], $misMembers);

        $misMembers = $this->valkey_glide->sMisMember('wrongkey', 'val', 'val2', 'val3');
        $this->assertEquals([false, false, false], $misMembers);
    }

    public function testlSet()
    {
        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'val');
        $this->valkey_glide->lPush('list', 'val2');
        $this->valkey_glide->lPush('list', 'val3');

        $this->assertEquals('val3', $this->valkey_glide->lIndex('list', 0));
        $this->assertEquals('val2', $this->valkey_glide->lIndex('list', 1));
        $this->assertEquals('val', $this->valkey_glide->lIndex('list', 2));

        $this->assertTrue($this->valkey_glide->lSet('list', 1, 'valx'));

        $this->assertEquals('val3', $this->valkey_glide->lIndex('list', 0));
        $this->assertEquals('valx', $this->valkey_glide->lIndex('list', 1));
        $this->assertEquals('val', $this->valkey_glide->lIndex('list', 2));
    }

    public function testsInter()
    {
        $this->valkey_glide->del('{set}odd');    // set of odd numbers
        $this->valkey_glide->del('{set}prime');  // set of prime numbers
        $this->valkey_glide->del('{set}square'); // set of squares
        $this->valkey_glide->del('{set}seq');    // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}odd', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}prime', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}square', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}seq', $i);
        }

        $xy = $this->valkey_glide->sInter('{set}odd', '{set}prime');   // odd prime numbers
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($x, $y));
        }

        $xy = $this->valkey_glide->sInter(['{set}odd', '{set}prime']);    // odd prime numbers, as array.
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($x, $y));
        }

        $yz = $this->valkey_glide->sInter('{set}prime', '{set}square');   // set of prime squares
        foreach ($yz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($y, $z));
        }

        $yz = $this->valkey_glide->sInter(['{set}prime', '{set}square']);    // set of odd squares, as array
        foreach ($yz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($y, $z));
        }

        $zt = $this->valkey_glide->sInter('{set}square', '{set}seq');   // prime squares
        $this->assertEquals([], $zt);
        $zt = $this->valkey_glide->sInter(['{set}square', '{set}seq']);    // prime squares, as array
        $this->assertEquals([], $zt);

        $xyz = $this->valkey_glide->sInter('{set}odd', '{set}prime', '{set}square');// odd prime squares
        $this->assertEquals(['1'], $xyz);

        $xyz = $this->valkey_glide->sInter(['{set}odd', '{set}prime', '{set}square']);// odd prime squares, with an array as a parameter
        $this->assertEquals(['1'], $xyz);

        $nil = $this->valkey_glide->sInter([]);
        $this->assertFalse($nil);
    }

    public function testsInterStore()
    {
        $this->valkey_glide->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}t', $i);
        }

        /* Regression test for passing a single array */
        $this->assertEquals(
            count(array_intersect($x, $y)),
            $this->valkey_glide->sInterStore(['{set}k', '{set}x', '{set}y'])
        );

        $count = $this->valkey_glide->sInterStore('{set}k', '{set}x', '{set}y');  // odd prime numbers
        $this->assertEquals($count, $this->valkey_glide->scard('{set}k'));
        foreach (array_intersect($x, $y) as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sInterStore('{set}k', '{set}y', '{set}z');  // set of odd squares
        $this->assertEquals($count, $this->valkey_glide->scard('{set}k'));
        foreach (array_intersect($y, $z) as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sInterStore('{set}k', '{set}z', '{set}t');  // squares of the form n^2 + 1
        $this->assertEquals($count, 0);
        $this->assertEquals($count, $this->valkey_glide->scard('{set}k'));

        $this->valkey_glide->del('{set}z');
        $xyz = $this->valkey_glide->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // only z missing, expect 0.
        $this->assertEquals(0, $xyz);

        $this->valkey_glide->del('{set}y');
        $xyz = $this->valkey_glide->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // y and z missing, expect 0.
        $this->assertEquals(0, $xyz);

        $this->valkey_glide->del('{set}x');
        $xyz = $this->valkey_glide->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // x y and z ALL missing, expect 0.
        $this->assertEquals(0, $xyz);
    }

    public function testsUnion()
    {
        $this->valkey_glide->del('{set}x');  // set of odd numbers
        $this->valkey_glide->del('{set}y');  // set of prime numbers
        $this->valkey_glide->del('{set}z');  // set of squares
        $this->valkey_glide->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}t', $i);
        }

        $xy = $this->valkey_glide->sUnion('{set}x', '{set}y');   // x U y
        foreach ($xy as $i) {
            $this->assertInArray($i, array_merge($x, $y));
        }

        $yz = $this->valkey_glide->sUnion('{set}y', '{set}z');   // y U Z
        foreach ($yz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_merge($y, $z));
        }

        $zt = $this->valkey_glide->sUnion('{set}z', '{set}t');   // z U t
        foreach ($zt as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_merge($z, $t));
        }

        $xyz = $this->valkey_glide->sUnion('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach ($xyz as $i) {
            $this->assertInArray($i, array_merge($x, $y, $z));
        }
    }

    public function testsUnionStore()
    {
        $this->valkey_glide->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}t', $i);
        }

        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}x', '{set}y');  // x U y
        $xy = array_unique(array_merge($x, $y));
        $this->assertEquals($count, count($xy));
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}y', '{set}z');  // y U z
        $yz = array_unique(array_merge($y, $z));
        $this->assertEquals($count, count($yz));
        foreach ($yz as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}z', '{set}t');  // z U t
        $zt = array_unique(array_merge($z, $t));
        $this->assertEquals($count, count($zt));
        foreach ($zt as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $xyz = array_unique(array_merge($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach ($xyz as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $this->valkey_glide->del('{set}x');  // x missing now
        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals($count, count(array_unique(array_merge($y, $z))));

        $this->valkey_glide->del('{set}y');  // x and y missing
        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals($count, count(array_unique($z)));

        $this->valkey_glide->del('{set}z');  // x, y, and z ALL missing
        $count = $this->valkey_glide->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals(0, $count);
    }

    public function testsDiff()
    {
        $this->valkey_glide->del('{set}x');  // set of odd numbers
        $this->valkey_glide->del('{set}y');  // set of prime numbers
        $this->valkey_glide->del('{set}z');  // set of squares
        $this->valkey_glide->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}t', $i);
        }

        $xy = $this->valkey_glide->sDiff('{set}x', '{set}y');    // x U y
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_diff($x, $y));
        }

        $yz = $this->valkey_glide->sDiff('{set}y', '{set}z');    // y U Z
        foreach ($yz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_diff($y, $z));
        }

        $zt = $this->valkey_glide->sDiff('{set}z', '{set}t');    // z U t
        foreach ($zt as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_diff($z, $t));
        }

        $xyz = $this->valkey_glide->sDiff('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach ($xyz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_diff($x, $y, $z));
        }
    }

    public function testsDiffStore()
    {
        $this->valkey_glide->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->valkey_glide->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->valkey_glide->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->valkey_glide->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->valkey_glide->sAdd('{set}t', $i);
        }

        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}x', '{set}y');   // x - y
        $xy = array_unique(array_diff($x, $y));
        $this->assertEquals($count, count($xy));
        foreach ($xy as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}y', '{set}z');   // y - z
        $yz = array_unique(array_diff($y, $z));
        $this->assertEquals($count, count($yz));
        foreach ($yz as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}z', '{set}t');   // z - t
        $zt = array_unique(array_diff($z, $t));
        $this->assertEquals($count, count($zt));
        foreach ($zt as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $xyz = array_unique(array_diff($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach ($xyz as $i) {
            $this->assertTrue($this->valkey_glide->sismember('{set}k', $i));
        }

        $this->valkey_glide->del('{set}x');  // x missing now
        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);

        $this->valkey_glide->del('{set}y');  // x and y missing
        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);

        $this->valkey_glide->del('{set}z');  // x, y, and z ALL missing
        $count = $this->valkey_glide->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);
    }

    public function testInterCard()
    {
        if (version_compare($this->version, '7.0.0') < 0) {
            $this->markTestSkipped();
        }

        $set_data = [
            ['aardvark', 'dog', 'fish', 'squirrel', 'tiger'],
            ['bear', 'coyote', 'fish', 'gorilla', 'dog']
        ];

        $ssets = $zsets = [];

        foreach ($set_data as $n => $values) {
            $sset = "s{set}:$n";
            $zset = "z{set}:$n";

            $this->valkey_glide->del([$sset, $zset]);

            $ssets[] = $sset;
            $zsets[] = $zset;

            foreach ($values as $score => $value) {
                $this->assertEquals(1, $this->valkey_glide->sAdd("s{set}:$n", $value));
                $this->assertEquals(1, $this->valkey_glide->zAdd("z{set}:$n", $score, $value));
            }
        }

        $exp = count(array_intersect(...$set_data));

        $act = $this->valkey_glide->sintercard($ssets);
        $this->assertEquals($exp, $act);
        $act = $this->valkey_glide->zintercard($zsets);
        $this->assertEquals($exp, $act);

        $this->assertEquals(1, $this->valkey_glide->sintercard($ssets, 1));
        $this->assertEquals(2, $this->valkey_glide->sintercard($ssets, 2));


        $this->assertEquals(1, $this->valkey_glide->zintercard($zsets, 1));
        $this->assertEquals(2, $this->valkey_glide->zintercard($zsets, 2));

        $this->assertFalse(@$this->valkey_glide->sintercard($ssets, -1));

        $this->assertFalse(@$this->valkey_glide->zintercard($ssets, -1));

        $this->assertFalse(@$this->valkey_glide->sintercard([]));
        $this->assertFalse(@$this->valkey_glide->zintercard([]));

        $this->valkey_glide->del(array_merge($ssets, $zsets));
    }

    public function testLRange()
    {
        $this->valkey_glide->del('list');
        $this->valkey_glide->lPush('list', 'val');
        $this->valkey_glide->lPush('list', 'val2');
        $this->valkey_glide->lPush('list', 'val3');

        $this->assertEquals(['val3'], $this->valkey_glide->lrange('list', 0, 0));
        $this->assertEquals(['val3', 'val2'], $this->valkey_glide->lrange('list', 0, 1));
        $this->assertEquals(['val3', 'val2', 'val'], $this->valkey_glide->lrange('list', 0, 2));
        $this->assertEquals(['val3', 'val2', 'val'], $this->valkey_glide->lrange('list', 0, 3));

        $this->assertEquals(['val3', 'val2', 'val'], $this->valkey_glide->lrange('list', 0, -1));
        $this->assertEquals(['val3', 'val2'], $this->valkey_glide->lrange('list', 0, -2));
        $this->assertEquals(['val2', 'val'], $this->valkey_glide->lrange('list', -2, -1));

        $this->valkey_glide->del('list');
        $this->assertEquals([], $this->valkey_glide->lrange('list', 0, -1));
    }

    public function testdbSize()
    {
        $this->assertTrue($this->valkey_glide->flushDB());
        $this->valkey_glide->set('x', 'y');
        $this->assertEquals(1, $this->valkey_glide->dbSize());
    }

    public function testFlushDB()
    {
        $this->assertTrue($this->valkey_glide->flushdb());
        $this->assertTrue($this->valkey_glide->flushdb(null));
        $this->assertTrue($this->valkey_glide->flushdb(false));
        $this->assertTrue($this->valkey_glide->flushdb(true));
    }

    public function testFlushAll()
    {
        $this->valkey_glide->set('x', 'y');
        $this->assertTrue($this->valkey_glide->flushAll());
        $this->assertEquals(0, $this->valkey_glide->dbSize());

        $this->valkey_glide->set('x', 'y');
        $this->assertTrue($this->valkey_glide->flushAll(null));
        $this->assertEquals(0, $this->valkey_glide->dbSize());

        $this->valkey_glide->set('x', 'y');
        $this->assertTrue($this->valkey_glide->flushAll(false));
        $this->assertEquals(0, $this->valkey_glide->dbSize());

        $this->valkey_glide->set('x', 'y');
        $this->assertTrue($this->valkey_glide->flushAll(true));
        $this->assertEquals(0, $this->valkey_glide->dbSize());
    }

    public function testTTL()
    {
        $this->valkey_glide->set('x', 'y');
        $this->valkey_glide->expire('x', 5);
        $ttl = $this->valkey_glide->ttl('x');
        $this->assertBetween($ttl, 1, 5);

        // A key with no TTL
        $this->valkey_glide->del('x');
        $this->valkey_glide->set('x', 'bar');
        $this->assertEquals(-1, $this->valkey_glide->ttl('x'));

        // A key that doesn't exist (> 2.8 will return -2)
        if (version_compare($this->version, '2.8.0') >= 0) {
            $this->valkey_glide->del('x');
            $this->assertEquals(-2, $this->valkey_glide->ttl('x'));
        }
    }

    public function testPersist()
    {
        $this->valkey_glide->set('x', 'y');
        $this->valkey_glide->expire('x', 100);
        $this->assertTrue($this->valkey_glide->persist('x'));     // true if there is a timeout
        $this->assertEquals(-1, $this->valkey_glide->ttl('x'));       // -1: timeout has been removed.
        $this->assertFalse($this->valkey_glide->persist('x'));    // false if there is no timeout
        $this->valkey_glide->del('x');
        $this->assertFalse($this->valkey_glide->persist('x'));    // false if the key doesn’t exist.
    }





    public function testWait()
    {
        // Closest we can check based on redis commit history
        if (version_compare($this->version, '2.9.11') < 0) {
            $this->markTestSkipped();
        }

        // We could have slaves here, so determine that
        $info     = $this->valkey_glide->info();
        $replicas = $info['connected_slaves'];

        // Send a couple commands
        $this->valkey_glide->set('wait-foo', 'over9000');
        $this->valkey_glide->set('wait-bar', 'revo9000');

        // Make sure we get the right replication count
        $this->assertEquals($replicas, $this->valkey_glide->wait($replicas, 100));

        // Pass more slaves than are connected
        $this->valkey_glide->set('wait-foo', 'over9000');
        $this->valkey_glide->set('wait-bar', 'revo9000');
        $this->assertLT($replicas + 1, $this->valkey_glide->wait($replicas + 1, 100));

        // Make sure when we pass with bad arguments we just get back false
        $this->assertFalse($this->valkey_glide->wait(-1, -1));
        $this->assertFalse($this->valkey_glide->wait(-1, 20));
    }

    public function testInfo()
    {
        $sequence = [false];
        if ($this->haveMulti()) {
            $sequence[] = true;
        }

        foreach ($sequence as $boo_multi) {
            if ($boo_multi) {
                $this->valkey_glide->multi();
                $this->valkey_glide->info();
                $info = $this->valkey_glide->exec();
                $info = $info[0];
            } else {
                $info = $this->valkey_glide->info();
            }
            $keys = [
                'redis_version',
                'arch_bits',
                'uptime_in_seconds',
                'uptime_in_days',
                'connected_clients',
                'connected_slaves',
                'used_memory',
                'total_connections_received',
                'total_commands_processed',
                'role'
            ];
            if (version_compare($this->version, '2.5.0') < 0) {
                array_push(
                    $keys,
                    'changes_since_last_save',
                    'bgsave_in_progress',
                    'last_save_time'
                );
            } else {
                array_push(
                    $keys,
                    'rdb_changes_since_last_save',
                    'rdb_bgsave_in_progress',
                    'rdb_last_save_time'
                );
            }

            foreach ($keys as $k) {
                $this->assertInArray($k, array_keys($info));
            }
        }

        if (! $this->minVersionCheck('7.0.0')) {
            return;
        }

        $res = $this->valkey_glide->info('server', 'memory');
        $this->assertTrue(is_array($res) && isset($res['redis_version']) && isset($res['used_memory']));
    }

    public function testInfoCommandStats()
    {
        // INFO COMMANDSTATS is new in 2.6.0
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        $info = $this->valkey_glide->info('COMMANDSTATS');


        if (! $this->assertIsArray($info)) {
            return;
        }

        foreach ($info as $k => $value) {
            if (! is_string($k)) {
                self::$errors [] = $this->assertionTrace("'%s' is not a string", $this->printArg($haystack));
                return false;
            }
            $this->assertStringContains('cmdstat_', $k);
        }
    }

    public function testSelect()
    {
        $this->assertFalse(@$this->valkey_glide->select(-1));
        $this->assertTrue($this->valkey_glide->select(0));
    }



    public function testMset()
    {
        $this->valkey_glide->del('x', 'y', 'z');    // remove x y z
        $this->assertTrue($this->valkey_glide->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->mget(['x', 'y', 'z']));    // check x y z

        $this->valkey_glide->del('x');  // delete just x
        $this->assertTrue($this->valkey_glide->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z
        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->mget(['x', 'y', 'z']));    // check x y z

        $this->assertFalse($this->valkey_glide->mset([])); // set ø → FALSE

        /*
         * Integer keys
         */

        // No prefix
        $set_array = [-1 => 'neg1', -2 => 'neg2', -3 => 'neg3', 1 => 'one', 2 => 'two', '3' => 'three'];
        $this->valkey_glide->del(array_keys($set_array));
        $this->assertTrue($this->valkey_glide->mset($set_array));
        $this->assertEquals(array_values($set_array), $this->valkey_glide->mget(array_keys($set_array)));
        $this->valkey_glide->del(array_keys($set_array));
    }

    public function testMsetNX()
    {
        $this->valkey_glide->del('x', 'y', 'z');    // remove x y z
        $this->assertTrue($this->valkey_glide->msetnx(['x' => 'a', 'y' => 'b', 'z' => 'c']));    // set x y z

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->mget(['x', 'y', 'z']));    // check x y z

        $this->valkey_glide->del('x');  // delete just x
        $this->assertFalse($this->valkey_glide->msetnx(['x' => 'A', 'y' => 'B', 'z' => 'C']));   // set x y z
        $this->assertEquals([false, 'b', 'c'], $this->valkey_glide->mget(['x', 'y', 'z']));  // check x y z

        $this->assertFalse($this->valkey_glide->msetnx([])); // set ø → FALSE
    }



    public function testZAddFirstArg()
    {
        $this->valkey_glide->del('100');

        $zsetName = 100; // not a string!
        $this->assertEquals(1, $this->valkey_glide->zAdd($zsetName, 0, 'val0'));
        $this->assertEquals(1, $this->valkey_glide->zAdd($zsetName, 1, 'val1'));

        $this->assertEquals(['val0', 'val1'], $this->valkey_glide->zRange($zsetName, 0, -1));
    }

    public function testZaddIncr()
    {
        $this->valkey_glide->del('zset');

        $this->assertEquals(10.0, $this->valkey_glide->zAdd('zset', ['incr'], 10, 'value'));
        $this->assertEquals(20.0, $this->valkey_glide->zAdd('zset', ['incr'], 10, 'value'));

        $this->assertFalse($this->valkey_glide->zAdd('zset', ['incr'], 10, 'value', 20, 'value2'));
    }

    /**
     * Test ZADD with CH option to cover line 178
     */
    public function testZaddWithChOption()
    {
        $key = 'test_zadd_ch_' . uniqid();

        // First add some members
        $this->assertEquals(2, $this->valkey_glide->zadd($key, 1, 'member1', 2, 'member2'));

        // Test with CH option using string array format
        $result = $this->valkey_glide->zadd($key, ['CH'], 1, 'member1', 3, 'member3');
        $this->assertTrue(is_numeric($result));

        // Clean up
        $this->valkey_glide->del($key);
    }

    /**
     * Test ZADD with associative array options format (lines 184-198)
     */
    public function testZaddWithAssociativeArrayOptions()
    {
        $key = 'test_zadd_assoc_' . uniqid();

        // Test XX option with associative array
        $this->assertEquals(0, $this->valkey_glide->zadd($key, ['XX' => true], 1, 'member1'));

        // Add a member first
        $this->assertEquals(1, $this->valkey_glide->zadd($key, 1, 'member1'));

        // Test NX option with associative array (should fail since member exists)
        $this->assertEquals(0, $this->valkey_glide->zadd($key, ['NX' => true], 2, 'member1'));

        // Test LT option with associative array
        $this->assertEquals(0, $this->valkey_glide->zadd($key, ['LT' => true], 0.5, 'member1'));

        // Test GT option with associative array
        $this->assertEquals(0, $this->valkey_glide->zadd($key, ['GT' => true], 2, 'member1'));

        // Test CH option with associative array
        $result = $this->valkey_glide->zadd($key, ['CH' => true], 3, 'member1');
        $this->assertTrue(is_numeric($result));

        // Test INCR option with associative array
        $result = $this->valkey_glide->zadd($key, ['INCR' => true], 1, 'member1');
        $this->assertTrue(is_numeric($result));

        // Clean up
        $this->valkey_glide->del($key);
    }

    /**
     * Test ZADD with false values in associative array
     */
    public function testZaddWithFalseAssociativeValues()
    {
        $key = 'test_zadd_false_' . uniqid();

        // Test with false values (should be ignored)
        $this->assertEquals(1, $this->valkey_glide->zadd($key, ['XX' => false], 1, 'member1'));
        $this->assertEquals(1, $this->valkey_glide->zadd($key, ['NX' => false], 2, 'member2'));

        // Clean up
        $this->valkey_glide->del($key);
    }

    /**
     * Test ZADD with mixed associative options
     */
    public function testZaddWithMixedAssociativeOptions()
    {
        $key = 'test_zadd_mixed_' . uniqid();

        // Test multiple options in associative format
        $this->assertEquals(1, $this->valkey_glide->zadd($key, ['CH' => true, 'NX' => true], 1, 'member1'));

        // Test combination that should work
        $result = $this->valkey_glide->zadd($key, ['CH' => true, 'XX' => true], 2, 'member1');
        $this->assertTrue(is_numeric($result));

        // Clean up
        $this->valkey_glide->del($key);
    }

    public function testZX()
    {
        $this->valkey_glide->del('key');

        $this->assertEquals([], $this->valkey_glide->zRange('key', 0, -1));
        $this->assertEquals([], $this->valkey_glide->zRange('key', 0, -1, true));

        $this->assertEquals(1, $this->valkey_glide->zAdd('key', 0, 'val0'));
        $this->assertEquals(1, $this->valkey_glide->zAdd('key', 2, 'val2'));
        $this->assertEquals(2, $this->valkey_glide->zAdd('key', 4, 'val4', 5, 'val5')); // multiple parameters
        if (version_compare($this->version, '3.0.2') < 0) {
            $this->assertEquals(1, $this->valkey_glide->zAdd('key', 1, 'val1'));
            $this->assertEquals(1, $this->valkey_glide->zAdd('key', 3, 'val3'));
        } else {
            $this->assertEquals(1, $this->valkey_glide->zAdd('key', [], 1, 'val1')); // empty options
            $this->assertEquals(1, $this->valkey_glide->zAdd('key', ['nx'], 3, 'val3')); // nx option
            $this->assertEquals(0, $this->valkey_glide->zAdd('key', ['xx'], 3, 'val3')); // xx option

            if (version_compare($this->version, '6.2.0') >= 0) {
                $this->assertEquals(0, $this->valkey_glide->zAdd('key', ['lt'], 4, 'val3')); // lt option
                $this->assertEquals(0, $this->valkey_glide->zAdd('key', ['gt'], 2, 'val3')); // gt option
            }
        }
        $this->assertEquals(['val0', 'val1', 'val2', 'val3', 'val4', 'val5'], $this->valkey_glide->zRange('key', 0, -1));
        // withscores
        $ret = $this->valkey_glide->zRange('key', 0, -1, true);

        $this->assertEquals(6, count($ret));

        $this->assertEquals(0.0, $ret['val0']);
        $this->assertEquals(1.0, $ret['val1']);
        $this->assertEquals(2.0, $ret['val2']);
        $this->assertEquals(3.0, $ret['val3']);
        $this->assertEquals(4.0, $ret['val4']);
        $this->assertEquals(5.0, $ret['val5']);

        $this->assertEquals(0, $this->valkey_glide->zRem('key', 'valX'));

        $this->assertEquals(1, $this->valkey_glide->zRem('key', 'val3'));
        $this->assertEquals(1, $this->valkey_glide->zRem('key', 'val4'));
        $this->assertEquals(1, $this->valkey_glide->zRem('key', 'val5'));

        $this->assertEquals(['val0', 'val1', 'val2'], $this->valkey_glide->zRange('key', 0, -1));

        // zGetReverseRange

        $this->assertEquals(1, $this->valkey_glide->zAdd('key', 3, 'val3'));
        $this->assertEquals(1, $this->valkey_glide->zAdd('key', 3, 'aal3'));

        $zero_to_three = $this->valkey_glide->zRangeByScore('key', 0, 3);

        $this->assertEquals(['val0', 'val1', 'val2', 'aal3', 'val3'], $zero_to_three);

        $three_to_zero = $this->valkey_glide->zRevRangeByScore('key', 3, 0);

        $this->assertEquals(array_reverse(['val0', 'val1', 'val2', 'aal3', 'val3']), $three_to_zero);

        $this->assertEquals(5, $this->valkey_glide->zCount('key', 0, 3));

        // withscores

        $this->valkey_glide->zRem('key', 'aal3');
        $zero_to_three = $this->valkey_glide->zRangeByScore('key', 0, 3, ['withscores' => true]);



        $this->assertEquals(['val0' => 0.0, 'val1' => 1.0, 'val2' => 2.0, 'val3' => 3.0], $zero_to_three);

        $this->assertEquals(4, $this->valkey_glide->zCount('key', 0, 3));


        // limit
        $this->assertEquals(['val0'], $this->valkey_glide->zRangeByScore('key', 0, 3, ['limit' => [0, 1]]));

        $this->assertEquals(
            ['val0', 'val1'],
            $this->valkey_glide->zRangeByScore('key', 0, 3, ['limit' => [0, 2]])
        );
        $this->assertEquals(
            ['val1', 'val2'],
            $this->valkey_glide->zRangeByScore('key', 0, 3, ['limit' => [1, 2]])
        );
        $this->assertEquals(
            ['val0', 'val1'],
            $this->valkey_glide->zRangeByScore('key', 0, 1, ['limit' => [0, 100]])
        );

        if ($this->minVersionCheck('6.2.0')) {
            $this->assertEquals(['val0', 'val1'], $this->valkey_glide->zrange('key', 0, 1, ['byscore', 'limit' => [0, 100]]));
        }

        // limits as references
        $limit = [0, 100];
        foreach ($limit as &$val) {
        }
        $this->assertEquals(['val0', 'val1'], $this->valkey_glide->zRangeByScore('key', 0, 1, ['limit' => $limit]));

        $this->assertEquals(
            ['val3'],
            $this->valkey_glide->zRevRangeByScore('key', 3, 0, ['limit' => [0, 1]])
        );
        $this->assertEquals(
            ['val3', 'val2'],
            $this->valkey_glide->zRevRangeByScore('key', 3, 0, ['limit' => [0, 2]])
        );
        $this->assertEquals(
            ['val2', 'val1'],
            $this->valkey_glide->zRevRangeByScore('key', 3, 0, ['limit' => [1, 2]])
        );
        $this->assertEquals(
            ['val1', 'val0'],
            $this->valkey_glide->zRevRangeByScore('key', 1, 0, ['limit' => [0, 100]])
        );

        if ($this->minVersionCheck('6.2.0')) {
            $this->assertEquals(
                ['val1', 'val0'],
                $this->valkey_glide->zrange('key', 1, 0, ['byscore', 'rev', 'limit' => [0, 100]])
            );

            $this->assertEquals(2, $this->valkey_glide->zrangestore(
                'dst{key}',
                'key',
                1,
                0,
                ['byscore', 'rev', 'limit' => [0, 100]]
            ));

            $this->assertEquals(['val0', 'val1'], $this->valkey_glide->zRange('dst{key}', 0, -1));

            $this->assertEquals(1, $this->valkey_glide->zrangestore(
                'dst{key}',
                'key',
                1,
                0,
                ['byscore', 'rev', 'limit' => [0, 1]]
            ));
            $this->assertEquals(['val1'], $this->valkey_glide->zrange('dst{key}', 0, -1));
        }
        $this->assertEquals(4, $this->valkey_glide->zCard('key'));

        $this->assertEquals(1.0, $this->valkey_glide->zScore('key', 'val1'));        
        $this->assertFalse($this->valkey_glide->zScore('key', 'val'));
        $this->assertFalse($this->valkey_glide->zScore(3, 2));

        // with () and +inf, -inf
        $this->valkey_glide->del('zset');
        $this->valkey_glide->zAdd('zset', 1, 'foo');
        $this->valkey_glide->zAdd('zset', 2, 'bar');
        $this->valkey_glide->zAdd('zset', 3, 'biz');
        $this->valkey_glide->zAdd('zset', 4, 'foz');
        $this->assertEquals(
            ['foo' => 1.0, 'bar' => 2.0, 'biz' => 3.0, 'foz' => 4.0],
            $this->valkey_glide->zRangeByScore('zset', '-inf', '+inf', ['withscores' => true])
        );

        $this->assertEquals(
            ['foo' => 1.0, 'bar' => 2.0],
            $this->valkey_glide->zRangeByScore('zset', 1, 2, ['withscores' => true])
        );
        $this->assertEquals(
            ['bar' => 2.0],
            $this->valkey_glide->zRangeByScore('zset', '(1', 2, ['withscores' => true])
        );
        $this->assertEquals([], $this->valkey_glide->zRangeByScore('zset', '(1', '(2', ['withscores' => true]));

        $this->assertEquals(4, $this->valkey_glide->zCount('zset', '-inf', '+inf'));
        $this->assertEquals(2, $this->valkey_glide->zCount('zset', 1, 2));
        $this->assertEquals(1, $this->valkey_glide->zCount('zset', '(1', 2));
        $this->assertEquals(0, $this->valkey_glide->zCount('zset', '(1', '(2'));

        // zincrby
        $this->valkey_glide->del('key');
        $this->assertEquals(1.0, $this->valkey_glide->zIncrBy('key', 1, 'val1'));
        $this->assertEquals(1.0, $this->valkey_glide->zScore('key', 'val1'));
        $this->assertEquals(2.5, $this->valkey_glide->zIncrBy('key', 1.5, 'val1'));
        $this->assertEquals(2.5, $this->valkey_glide->zScore('key', 'val1'));

        // zUnionStore
        $this->valkey_glide->del('{zset}1');
        $this->valkey_glide->del('{zset}2');
        $this->valkey_glide->del('{zset}3');
        $this->valkey_glide->del('{zset}U');

        $this->valkey_glide->zAdd('{zset}1', 0, 'val0');
        $this->valkey_glide->zAdd('{zset}1', 1, 'val1');

        $this->valkey_glide->zAdd('{zset}2', 2, 'val2');
        $this->valkey_glide->zAdd('{zset}2', 3, 'val3');

        $this->valkey_glide->zAdd('{zset}3', 4, 'val4');
        $this->valkey_glide->zAdd('{zset}3', 5, 'val5');

        $this->assertEquals(4, $this->valkey_glide->zUnionStore('{zset}U', ['{zset}1', '{zset}3']));

        $this->assertEquals(['val0', 'val1', 'val4', 'val5'], $this->valkey_glide->zRange('{zset}U', 0, -1));

        // Union on non existing keys
        $this->valkey_glide->del('{zset}U');
        $this->assertEquals(0, $this->valkey_glide->zUnionStore('{zset}U', ['{zset}X', '{zset}Y']));
        $this->assertEquals([], $this->valkey_glide->zRange('{zset}U', 0, -1));

        // !Exist U Exist → copy of existing zset.
        $this->valkey_glide->del('{zset}U', 'X');
        $this->assertEquals(2, $this->valkey_glide->zUnionStore('{zset}U', ['{zset}1', '{zset}X']));

        // test weighted zUnion
        $this->valkey_glide->del('{zset}Z');
        $this->assertEquals(4, $this->valkey_glide->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertEquals(['val0', 'val1', 'val2', 'val3'], $this->valkey_glide->zRange('{zset}Z', 0, -1));

        $this->valkey_glide->zRemRangeByScore('{zset}Z', 0, 10);
        $this->assertEquals(4, $this->valkey_glide->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [5, 1]));
        $this->assertEquals(['val0', 'val2', 'val3', 'val1'], $this->valkey_glide->zRange('{zset}Z', 0, -1));

        $this->valkey_glide->del('{zset}1');
        $this->valkey_glide->del('{zset}2');
        $this->valkey_glide->del('{zset}3');

        //test zUnion with weights and aggegration function
        $this->valkey_glide->zadd('{zset}1', 1, 'duplicate');
        $this->valkey_glide->zadd('{zset}2', 2, 'duplicate');
        $this->valkey_glide->zUnionStore('{zset}U', ['{zset}1', '{zset}2'], [1, 1], 'MIN');

        $this->assertEquals(1.0, $this->valkey_glide->zScore('{zset}U', 'duplicate'));
        $this->valkey_glide->del('{zset}U');

        //now test zUnion *without* weights but with aggregate function
        $this->valkey_glide->zUnionStore('{zset}U', ['{zset}1', '{zset}2'], null, 'MIN');

        $this->assertEquals(1.0, $this->valkey_glide->zScore('{zset}U', 'duplicate'));
        $this->valkey_glide->del('{zset}U', '{zset}1', '{zset}2');

        // test integer and float weights (GitHub issue #109).
        $this->valkey_glide->del('{zset}1', '{zset}2', '{zset}3');

        $this->valkey_glide->zadd('{zset}1', 1, 'one');
        $this->valkey_glide->zadd('{zset}1', 2, 'two');
        $this->valkey_glide->zadd('{zset}2', 1, 'one');
        $this->valkey_glide->zadd('{zset}2', 2, 'two');
        $this->valkey_glide->zadd('{zset}2', 3, 'three');

        $this->assertEquals(3, $this->valkey_glide->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [2, 3.0]));

        $this->valkey_glide->del('{zset}1');
        $this->valkey_glide->del('{zset}2');
        $this->valkey_glide->del('{zset}3');

        // Test 'inf', '-inf', and '+inf' weights (GitHub issue #336)
        $this->valkey_glide->zadd('{zset}1', 1, 'one', 2, 'two', 3, 'three');
        $this->valkey_glide->zadd('{zset}2', 3, 'three', 4, 'four', 5, 'five');

        // Make sure phpredis handles these weights
        $this->assertEquals(5, $this->valkey_glide->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, 'inf']));
        $this->assertEquals(5, $this->valkey_glide->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, '-inf']));
        $this->assertEquals(5, $this->valkey_glide->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, '+inf']));

        // Now, confirm that they're being sent, and that it works
        $weights = ['inf', '-inf', '+inf'];

        foreach ($weights as $weight) {
            $r = $this->valkey_glide->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, $weight]);
            $this->assertEquals(5, $r);
            $r = $this->valkey_glide->zrangebyscore('{zset}3', '(-inf', '(inf', ['withscores' => true]);
            $this->assertEquals(2, count($r));
            $this->assertArrayKey($r, 'one');
            $this->assertArrayKey($r, 'two');
        }

        $this->valkey_glide->del('{zset}1', '{zset}2', '{zset}3');

        $this->valkey_glide->zadd('{zset}1', 2000.1, 'one');
        $this->valkey_glide->zadd('{zset}1', 3000.1, 'two');
        $this->valkey_glide->zadd('{zset}1', 4000.1, 'three');

        $ret = $this->valkey_glide->zRange('{zset}1', 0, -1, true);
        $this->assertEquals(3, count($ret));
        $retValues = array_keys($ret);

        $this->assertEquals(['one', 'two', 'three'], $retValues);

        // + 0 converts from string to float OR integer
        $this->assertArrayKeyEquals($ret, 'one', 2000.1);
        $this->assertArrayKeyEquals($ret, 'two', 3000.1);
        $this->assertArrayKeyEquals($ret, 'three', 4000.1);

        $this->valkey_glide->del('{zset}1');

        // ZREMRANGEBYRANK
        $this->valkey_glide->zAdd('{zset}1', 1, 'one');
        $this->valkey_glide->zAdd('{zset}1', 2, 'two');
        $this->valkey_glide->zAdd('{zset}1', 3, 'three');
        $this->assertEquals(2, $this->valkey_glide->zremrangebyrank('{zset}1', 0, 1));
        $this->assertEquals(['three' => 3.], $this->valkey_glide->zRange('{zset}1', 0, -1, true));

        $this->valkey_glide->del('{zset}1');

        // zInterStore

        $this->valkey_glide->zAdd('{zset}1', 0, 'val0');
        $this->valkey_glide->zAdd('{zset}1', 1, 'val1');
        $this->valkey_glide->zAdd('{zset}1', 3, 'val3');

        $this->valkey_glide->zAdd('{zset}2', 2, 'val1');
        $this->valkey_glide->zAdd('{zset}2', 3, 'val3');

        $this->valkey_glide->zAdd('{zset}3', 4, 'val3');
        $this->valkey_glide->zAdd('{zset}3', 5, 'val5');

        $this->valkey_glide->del('{zset}I');
        $this->assertEquals(2, $this->valkey_glide->zInterStore('{zset}I', ['{zset}1', '{zset}2']));
        $this->assertEquals(['val1', 'val3'], $this->valkey_glide->zRange('{zset}I', 0, -1));

        // Union on non existing keys
        $this->assertEquals(0, $this->valkey_glide->zInterStore('{zset}X', ['{zset}X', '{zset}Y']));
        $this->assertEquals([], $this->valkey_glide->zRange('{zset}X', 0, -1));

        // !Exist U Exist
        $this->assertEquals(0, $this->valkey_glide->zInterStore('{zset}Y', ['{zset}1', '{zset}X']));
        $this->assertEquals([], $this->valkey_glide->zRange('keyY', 0, -1));


        // test weighted zInterStore
        $this->valkey_glide->del('{zset}1');
        $this->valkey_glide->del('{zset}2');
        $this->valkey_glide->del('{zset}3');

        $this->valkey_glide->zAdd('{zset}1', 0, 'val0');
        $this->valkey_glide->zAdd('{zset}1', 1, 'val1');
        $this->valkey_glide->zAdd('{zset}1', 3, 'val3');


        $this->valkey_glide->zAdd('{zset}2', 2, 'val1');
        $this->valkey_glide->zAdd('{zset}2', 1, 'val3');

        $this->valkey_glide->zAdd('{zset}3', 7, 'val1');
        $this->valkey_glide->zAdd('{zset}3', 3, 'val3');

        $this->valkey_glide->del('{zset}I');
        $this->assertEquals(2, $this->valkey_glide->zInterStore('{zset}I', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertEquals(['val1', 'val3'], $this->valkey_glide->zRange('{zset}I', 0, -1));

        $this->valkey_glide->del('{zset}I');
        $this->assertEquals(2, $this->valkey_glide->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'min'));
        $this->assertEquals(['val1', 'val3'], $this->valkey_glide->zRange('{zset}I', 0, -1));
        $this->valkey_glide->del('{zset}I');
        $this->assertEquals(2, $this->valkey_glide->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'max'));
        $this->assertEquals(['val3', 'val1'], $this->valkey_glide->zRange('{zset}I', 0, -1));

        $this->valkey_glide->del('{zset}I');
        $this->assertEquals(2, $this->valkey_glide->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], null, 'max'));
        $this->assertEquals(7., $this->valkey_glide->zScore('{zset}I', 'val1'));

        // zrank, zrevrank
        $this->valkey_glide->del('z');
        $this->valkey_glide->zadd('z', 1, 'one');
        $this->valkey_glide->zadd('z', 2, 'two');
        $this->valkey_glide->zadd('z', 5, 'five');

        $this->assertEquals(0, $this->valkey_glide->zRank('z', 'one'));
        $this->assertEquals(1, $this->valkey_glide->zRank('z', 'two'));
        $this->assertEquals(2, $this->valkey_glide->zRank('z', 'five'));

        $this->assertEquals(2, $this->valkey_glide->zRevRank('z', 'one'));
        $this->assertEquals(1, $this->valkey_glide->zRevRank('z', 'two'));
        $this->assertEquals(0, $this->valkey_glide->zRevRank('z', 'five'));
    }

    public function testZRangeScoreArg()
    {
        $this->valkey_glide->del('{z}');

        $mems = ['one' => 1.0, 'two' => 2.0, 'three' => 3.0];
        foreach ($mems as $mem => $score) {
            $this->valkey_glide->zAdd('{z}', $score, $mem);
        }

        /* Verify we can pass true and ['withscores' => true] */
        $this->assertEquals($mems, $this->valkey_glide->zRange('{z}', 0, -1, true));
        $this->assertEquals($mems, $this->valkey_glide->zRange('{z}', 0, -1, ['withscores' => true]));
    }

    public function testZRangeByLex()
    {
        /* ZRANGEBYLEX available on versions >= 2.8.9 */
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'g') as $c) {
            $this->valkey_glide->zAdd('key', 0, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zRangeByLex('key', '-', '[c'));
        $this->assertEquals(['f', 'g'], $this->valkey_glide->zRangeByLex('key', '(e', '+'));


        // with limit offset
        $this->assertEquals(['b', 'c'], $this->valkey_glide->zRangeByLex('key', '-', '[c', 1, 2));
        $this->assertEquals(['b'], $this->valkey_glide->zRangeByLex('key', '-', '(c', 1, 2));

        /* Test getting the same functionality via ZRANGE and options */
        if ($this->minVersionCheck('6.2.0')) {
            $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zRange('key', '-', '[c', ['BYLEX']));

            $this->assertEquals(['b', 'c'], $this->valkey_glide->zRange('key', '-', '[c', ['BYLEX', 'LIMIT' => [1, 2]]));
            $this->assertEquals(['b'], $this->valkey_glide->zRange('key', '-', '(c', ['BYLEX', 'LIMIT' => [1, 2]]));

            $this->assertEquals(['b', 'a'], $this->valkey_glide->zRange('key', '[c', '-', ['BYLEX', 'REV', 'LIMIT' => [1, 2]]));
        }
    }

    public function testZLexCount()
    {
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'g') as $c) {
            $entries[] = $c;
            $this->valkey_glide->zAdd('key', 0, $c);
        }

        /* Special -/+ values */
        $this->assertEquals(0, $this->valkey_glide->zLexCount('key', '-', '-'));
        $this->assertEquals(count($entries), $this->valkey_glide->zLexCount('key', '-', '+'));

        /* Verify invalid arguments return FALSE */
        $this->assertFalse(@$this->valkey_glide->zLexCount('key', '[a', 'bad'));
        $this->assertFalse(@$this->valkey_glide->zLexCount('key', 'bad', '[a'));

        /* Now iterate through */
        $start = $entries[0];
        for ($i = 1; $i < count($entries); $i++) {
            $end = $entries[$i];
            $this->assertEquals($i + 1, $this->valkey_glide->zLexCount('key', "[$start", "[$end"));
            $this->assertEquals($i, $this->valkey_glide->zLexCount('key', "[$start", "($end"));
            $this->assertEquals($i - 1, $this->valkey_glide->zLexCount('key', "($start", "($end"));
        }
    }

    public function testzDiff()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'c') as $c) {
            $this->valkey_glide->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zDiff(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->valkey_glide->zDiff(['key'], ['withscores' => true]));
    }

    public function testzInter()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'c') as $c) {
            $this->valkey_glide->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zInter(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->valkey_glide->zInter(['key'], null, ['withscores' => true]));
    }

    public function testzUnion()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'c') as $c) {
            $this->valkey_glide->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zUnion(['key']));

        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->valkey_glide->zUnion(['key'], null, ['withscores' => true]));
    }


   /**
     * Test ZUNION with WEIGHTS option - verify specific weighted scores
     */
    public function testZunionWithWeights()
    {
        $key1 = '{test}test_zunion_weights_1_' . uniqid();
        $key2 = '{test}test_zunion_weights_2_' . uniqid();

        // Set up test data with known scores
        $this->valkey_glide->zadd($key1, 1, 'member1', 2, 'member2');
        $this->valkey_glide->zadd($key2, 3, 'member1', 4, 'member3');

        $keys = [$key1, $key2];
        $weights = [2, 3]; // key1 scores * 2, key2 scores * 3

        // Test ZUNION with weights and WITHSCORES
        $result = $this->valkey_glide->zUnion($keys, $weights, ['WITHSCORES' => true]);

        // Verify specific results:
        // member1: (1*2) + (3*3) = 2 + 9 = 11
        // member2: (2*2) + (0*3) = 4 + 0 = 4 (member2 only in key1)
        // member3: (0*2) + (4*3) = 0 + 12 = 12 (member3 only in key2)

        $this->assertIsArray($result);
        $this->assertEquals(3, count($result)); // 3 members * 2 (member + score)

        // Verify members and scores are present
        $this->assertEquals($result['member2'], 4.0);
        $this->assertEquals($result['member1'], 11.0);
        $this->assertEquals($result['member3'], 12.0);

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }
    /**
     * Test ZUNION with AGGREGATE option - verify specific aggregation results
     */
    public function testZunionWithAggregate()
    {
        $key1 = '{test}test_zunion_agg_1_' . uniqid();
        $key2 = '{test}test_zunion_agg_2_' . uniqid();

        // Set up test data with overlapping member 'common'
        $this->valkey_glide->zadd($key1, 5, 'common', 2, 'unique1');
        $this->valkey_glide->zadd($key2, 3, 'common', 4, 'unique2');

        $keys = [$key1, $key2];

        // Test SUM aggregation (default behavior)
        $result = $this->valkey_glide->zUnion($keys, null, ['AGGREGATE' => 'SUM', 'WITHSCORES' => true]);
        $this->assertIsArray($result);
        $this->assertEquals(3, count($result)); // 3 unique members
        $this->assertEquals(8.0, $result['common']); // 5 + 3 = 8
        $this->assertEquals(2.0, $result['unique1']);
        $this->assertEquals(4.0, $result['unique2']);

        // Test MIN aggregation
        $result = $this->valkey_glide->zUnion($keys, null, ['AGGREGATE' => 'MIN', 'WITHSCORES' => true]);
        $this->assertEquals(3.0, $result['common']); // min(5, 3) = 3

        // Test MAX aggregation
        $result = $this->valkey_glide->zUnion($keys, null, ['AGGREGATE' => 'MAX', 'WITHSCORES' => true]);
        $this->assertEquals(5.0, $result['common']); // max(5, 3) = 5

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }

    /**
     * Test ZUNION with both WEIGHTS and AGGREGATE - verify precise calculations
     */
    public function testZunionWithWeightsAndAggregate()
    {
        $key1 = '{test}test_zunion_both_1_' . uniqid();
        $key2 = '{test}test_zunion_both_2_' . uniqid();

        // Set up data where 'common' appears in both sets
        $this->valkey_glide->zadd($key1, 2, 'common', 1, 'unique1');
        $this->valkey_glide->zadd($key2, 4, 'common', 3, 'unique2');

        $keys = [$key1, $key2];
        $weights = [3, 2]; // key1 * 3, key2 * 2

        // Test with MIN aggregation and weights
        // For 'common': min(2*3, 4*2) = min(6, 8) = 6
        $result = $this->valkey_glide->zUnion($keys, $weights, ['AGGREGATE' => 'MIN', 'WITHSCORES' => true]);
        $this->assertIsArray($result);
        $this->assertEquals(3, count($result)); // 3 unique members
        $this->assertEquals(6.0, $result['common']);
        $this->assertEquals(3.0, $result['unique1']); // 1 * 3 = 3
        $this->assertEquals(6.0, $result['unique2']); // 3 * 2 = 6

        // Test with MAX aggregation and weights
        // For 'common': max(2*3, 4*2) = max(6, 8) = 8
        $result = $this->valkey_glide->zUnion($keys, $weights, ['AGGREGATE' => 'MAX', 'WITHSCORES' => true]);
        $this->assertEquals(8.0, $result['common']);
        $this->assertEquals(3.0, $result['unique1']); // 1 * 3 = 3
        $this->assertEquals(6.0, $result['unique2']); // 3 * 2 = 6

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }

    /**
     * Test ZUNION with float weights - verify decimal precision
     */
    public function testZunionWithFloatWeights()
    {
        $key1 = '{test}test_zunion_float_1_' . uniqid();
        $key2 = '{test}test_zunion_float_2_' . uniqid();

        // Set up test data
        $this->valkey_glide->zadd($key1, 2, 'member1');
        $this->valkey_glide->zadd($key2, 3, 'member2');

        $keys = [$key1, $key2];
        $weights = [1.5, 2.5]; // Decimal weights

        $result = $this->valkey_glide->zUnion($keys, $weights, ['WITHSCORES' => true]);

        // Verify calculated scores:
        // member1: 2 * 1.5 = 3.0
        // member2: 3 * 2.5 = 7.5
        $this->assertIsArray($result);
        $this->assertEquals(2, count($result)); // 2 unique members
        $this->assertEquals(3.0, $result['member1']);
        $this->assertEquals(7.5, $result['member2']);

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }

    /**
     * Test ZUNION with and without WITHSCORES - verify format differences
     */
    public function testZunionWithScoresComparison()
    {
        $key1 = '{test}test_zunion_scores_1_' . uniqid();
        $key2 = '{test}test_zunion_scores_2_' . uniqid();

        // Set up test data
        $this->valkey_glide->zadd($key1, 1, 'alpha', 3, 'charlie');
        $this->valkey_glide->zadd($key2, 2, 'beta', 4, 'delta');

        $keys = [$key1, $key2];

        // Test without WITHSCORES (members only)
        $result_no_scores = $this->valkey_glide->zUnion($keys);
        $this->assertIsArray($result_no_scores);
        $this->assertEquals(4, count($result_no_scores)); // Just 4 members as indexed array
        $this->assertEquals('alpha', $result_no_scores[0]);
        $this->assertEquals('beta', $result_no_scores[1]);
        $this->assertEquals('charlie', $result_no_scores[2]);
        $this->assertEquals('delta', $result_no_scores[3]);

        // Test with WITHSCORES (members + scores as associative array)
        $result_with_scores = $this->valkey_glide->zUnion($keys, null, ['WITHSCORES' => true]);
        $this->assertIsArray($result_with_scores);
        $this->assertEquals(4, count($result_with_scores)); // 4 members in associative array
        $this->assertEquals(1.0, $result_with_scores['alpha']);
        $this->assertEquals(2.0, $result_with_scores['beta']);
        $this->assertEquals(3.0, $result_with_scores['charlie']);
        $this->assertEquals(4.0, $result_with_scores['delta']);

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }

/**
 * Test ZUNION with non-string keys (numeric keys)
 */
    public function testZunionWithNumericKeys()
    {
        $key1 = '{test}123'; // Numeric-like key
        $key2 = '{test}456'; // Numeric-like key

        // Set up test data
        $this->valkey_glide->zadd($key1, 1, 'member1');
        $this->valkey_glide->zadd($key2, 2, 'member2');

        // Test ZUNION with numeric-like keys in array
        $keys = [$key1, $key2];

        $result = $this->valkey_glide->zUnion($keys, null, ['WITHSCORES' => true]);
        $this->assertIsArray($result);
        $this->assertEquals(2, count($result)); // 2 members
        $this->assertEquals(1.0, $result['member1']);
        $this->assertEquals(2.0, $result['member2']);

        // Clean up
        $this->valkey_glide->del($key1, $key2);
    }

    public function testzDiffStore()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('{zkey}src');
        foreach (range('a', 'c') as $c) {
            $this->valkey_glide->zAdd('{zkey}src', 1, $c);
        }
        $this->assertEquals(3, $this->valkey_glide->zDiffStore('{zkey}dst', ['{zkey}src']));
        $this->assertEquals(['a', 'b', 'c'], $this->valkey_glide->zRange('{zkey}dst', 0, -1));
    }

    public function testzMscore()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('key');
        foreach (range('a', 'c') as $c) {
            $this->valkey_glide->zAdd('key', 1, $c);
        }

        $scores = $this->valkey_glide->zMscore('key', 'a', 'notamember', 'c');

        $this->assertEquals([1.0, false, 1.0], $scores);

        $scores = $this->valkey_glide->zMscore('wrongkey', 'a', 'b', 'c');
        $this->assertEquals([false, false, false], $scores);
    }

    public function testZRemRangeByLex()
    {
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->valkey_glide->del('key');
        $this->valkey_glide->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(3, $this->valkey_glide->zRemRangeByLex('key', '-', '+'));

        $this->valkey_glide->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(3, $this->valkey_glide->zRemRangeByLex('key', '[a', '[c'));

        $this->valkey_glide->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(0, $this->valkey_glide->zRemRangeByLex('key', '[a', '(a'));
        $this->assertEquals(1, $this->valkey_glide->zRemRangeByLex('key', '(a', '(c'));
        $this->assertEquals(2, $this->valkey_glide->zRemRangeByLex('key', '[a', '[c'));
    }

    public function testBZPop()
    {
        if (version_compare($this->version, '5.0.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->valkey_glide->del('{zs}1', '{zs}2');
        $this->valkey_glide->zAdd('{zs}1', 0, 'a', 1, 'b', 2, 'c');
        $this->valkey_glide->zAdd('{zs}2', 3, 'A', 4, 'B', 5, 'D');

        $this->assertEquals(['{zs}1', 'a', '0'], $this->valkey_glide->bzPopMin('{zs}1', '{zs}2', 0));

        $this->assertEquals(['{zs}1', 'c', '2'], $this->valkey_glide->bzPopMax(['{zs}1', '{zs}2'], 0));
        $this->assertEquals(['{zs}2', 'A', '3'], $this->valkey_glide->bzPopMin(['{zs}2', '{zs}1'], 0));


        /* Verify timeout is being sent */
        $this->valkey_glide->del('{zs}1', '{zs}2');
        $st = microtime(true) * 1000;
        $this->valkey_glide->bzPopMin('{zs}1', '{zs}2', 1);
        $et = microtime(true) * 1000;
        $this->assertGT(100, $et - $st);
    }

    public function testZPop()
    {
        if (version_compare($this->version, '5.0.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        // zPopMax and zPopMin without a COUNT argument
        $this->valkey_glide->del('key');
        $this->valkey_glide->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['e' => 4.0], $this->valkey_glide->zPopMax('key'));
        $this->assertEquals(['a' => 0.0], $this->valkey_glide->zPopMin('key'));

        // zPopMax with a COUNT argument
        $this->valkey_glide->del('key');
        $this->valkey_glide->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['e' => 4.0, 'd' => 3.0, 'c' => 2.0], $this->valkey_glide->zPopMax('key', 3));

        // zPopMin with a COUNT argument
        $this->valkey_glide->del('key');
        $this->valkey_glide->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['a' => 0.0, 'b' => 1.0, 'c' => 2.0], $this->valkey_glide->zPopMin('key', 3));
    }

    public function testZRandMember()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }
        $this->valkey_glide->del('key');
        $this->valkey_glide->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');

        $result = $this->valkey_glide->zRandMember('key');

        $this->assertEquals(array_intersect($result, ['a', 'b', 'c', 'd', 'e']), $result);


        $result = $this->valkey_glide->zRandMember('key', ['count' => 3]);

        $this->assertEquals(3, count($result));
        $this->assertEquals(array_intersect($result, ['a', 'b', 'c', 'd', 'e']), $result);

        $result = $this->valkey_glide->zRandMember('key', ['count' => 2, 'withscores' => true]);
        $this->assertEquals(2, count($result));


        $this->assertEquals(array_intersect_key($result, ['a' => 0, 'b' => 1, 'c' => 2, 'd' => 3, 'e' => 4]), $result);
    }

    public function testHashes()
    {
        $this->valkey_glide->del('h', 'key');

        $this->assertEquals(0, $this->valkey_glide->hLen('h'));
        

        $this->assertEquals(1, $this->valkey_glide->hSet('h', 'a', 'a-value'));

        $this->assertEquals(1, $this->valkey_glide->hLen('h'));
        $this->assertEquals(1, $this->valkey_glide->hSet('h', 'b', 'b-value'));
        $this->assertEquals(2, $this->valkey_glide->hLen('h'));
        
        $this->assertEquals('a-value', $this->valkey_glide->hGet('h', 'a'));  // simple get
        $this->assertEquals('b-value', $this->valkey_glide->hGet('h', 'b'));  // simple get

        $this->assertEquals(0, $this->valkey_glide->hSet('h', 'a', 'another-value')); // replacement
        
        $this->assertEquals('another-value', $this->valkey_glide->hGet('h', 'a'));    // get the new value

        $this->assertEquals('b-value', $this->valkey_glide->hGet('h', 'b'));  // simple get

        $this->assertFalse($this->valkey_glide->hGet('h', 'c'));  // unknown hash member

        $this->assertFalse($this->valkey_glide->hGet('key', 'c'));    // unknownkey
        // hDel
        $this->assertEquals(1, $this->valkey_glide->hDel('h', 'a')); // 1 on success

        $this->assertEquals(0, $this->valkey_glide->hDel('h', 'a')); // 0 on failure

        $this->valkey_glide->del('h');
        $this->valkey_glide->hSet('h', 'x', 'a');
        $this->valkey_glide->hSet('h', 'y', 'b');
        $this->assertEquals(2, $this->valkey_glide->hDel('h', 'x', 'y')); // variadic


        // hsetnx
        $this->valkey_glide->del('h');

        $this->assertTrue($this->valkey_glide->hSetNx('h', 'x', 'a'));

        $this->assertTrue($this->valkey_glide->hSetNx('h', 'y', 'b'));
        $this->assertFalse($this->valkey_glide->hSetNx('h', 'x', '?'));
        $this->assertFalse($this->valkey_glide->hSetNx('h', 'y', '?'));
        $this->assertEquals('a', $this->valkey_glide->hGet('h', 'x'));
        $this->assertEquals('b', $this->valkey_glide->hGet('h', 'y'));

        // keys
        $keys = $this->valkey_glide->hKeys('h');
        $this->assertEqualsCanonicalizing(['x', 'y'], $keys);

        // values
        $values = $this->valkey_glide->hVals('h');
        $this->assertEqualsCanonicalizing(['a', 'b'], $values);

        // keys + values
        $all = $this->valkey_glide->hGetAll('h');

        $this->assertEqualsCanonicalizing(['x' => 'a', 'y' => 'b'], $all, true);

        // hExists
        $this->assertTrue($this->valkey_glide->hExists('h', 'x'));
        
        $this->assertTrue($this->valkey_glide->hExists('h', 'y'));
        $this->assertFalse($this->valkey_glide->hExists('h', 'w'));
        $this->valkey_glide->del('h');
        $this->assertFalse($this->valkey_glide->hExists('h', 'x'));

        // hIncrBy
        $this->valkey_glide->del('h');
        $this->assertEquals(2, $this->valkey_glide->hIncrBy('h', 'x', 2));

        $this->assertEquals(3, $this->valkey_glide->hIncrBy('h', 'x', 1));
        $this->assertEquals(2, $this->valkey_glide->hIncrBy('h', 'x', -1));
        $this->assertEquals('2', $this->valkey_glide->hGet('h', 'x'));
        $this->assertEquals(PHP_INT_MAX, $this->valkey_glide->hIncrBy('h', 'x', PHP_INT_MAX - 2));
        $this->assertEquals('' . PHP_INT_MAX, $this->valkey_glide->hGet('h', 'x'));

        $this->valkey_glide->hSet('h', 'y', 'not-a-number');
        $this->assertFalse($this->valkey_glide->hIncrBy('h', 'y', 1));

        if (version_compare($this->version, '2.5.0') >= 0) {
            // hIncrByFloat
            $this->valkey_glide->del('h');
            $this->assertEquals(1.5, $this->valkey_glide->hIncrByFloat('h', 'x', 1.5));

            $this->assertEquals(3.0, $this->valkey_glide->hincrByFloat('h', 'x', 1.5));
            $this->assertEquals(1.5, $this->valkey_glide->hincrByFloat('h', 'x', -1.5));
            $this->assertEquals(1000000000001.5, $this->valkey_glide->hincrByFloat('h', 'x', 1000000000000));

            $this->valkey_glide->hset('h', 'y', 'not-a-number');
            $this->assertFalse($this->valkey_glide->hIncrByFloat('h', 'y', 1.5));
        }

        // hmset
        $this->valkey_glide->del('h');
        $this->assertTrue($this->valkey_glide->hMset('h', ['x' => 123, 'y' => 456, 'z' => 'abc']));

        $this->assertEquals('123', $this->valkey_glide->hGet('h', 'x'));
        $this->assertEquals('456', $this->valkey_glide->hGet('h', 'y'));
        $this->assertEquals('abc', $this->valkey_glide->hGet('h', 'z'));
        $this->assertFalse($this->valkey_glide->hGet('h', 't'));

        // hmget
        $this->assertEquals(['x' => '123', 'y' => '456'], $this->valkey_glide->hMget('h', ['x', 'y']));

        $this->assertEquals(['z' => 'abc'], $this->valkey_glide->hMget('h', ['z']));
        $this->assertEquals(['x' => '123', 't' => false, 'y' => '456'], $this->valkey_glide->hMget('h', ['x', 't', 'y']));
        $this->assertEquals(['x' => '123', 't' => false, 'y' => '456'], $this->valkey_glide->hMget('h', ['x', 't', 'y']));
        $this->assertNotEquals([123 => 'x'], $this->valkey_glide->hMget('h', [123]));
        $this->assertEquals([123 => false], $this->valkey_glide->hMget('h', [123]));

        // Test with an array populated with things we can't use as keys
        $this->assertFalse($this->valkey_glide->hmget('h', [false,null,false]));

        // Test with some invalid keys mixed in (which should just be ignored)
        $this->assertEquals(
            ['x' => '123', 'y' => '456', 'z' => 'abc'],
            $this->valkey_glide->hMget('h', ['x', null, 'y', '', 'z', false])
        );

        // hmget/hmset with numeric fields
        $this->valkey_glide->del('h');
        $this->assertTrue($this->valkey_glide->hMset('h', [123 => 'x', 'y' => 456]));
        $this->assertEquals('x', $this->valkey_glide->hGet('h', 123));
        $this->assertEquals('x', $this->valkey_glide->hGet('h', '123'));
        $this->assertEquals('456', $this->valkey_glide->hGet('h', 'y'));
        $this->assertEquals([123 => 'x', 'y' => '456'], $this->valkey_glide->hMget('h', ['123', 'y']));

        // references
        $keys = [123, 'y'];
        foreach ($keys as &$key) {
        }

        $this->assertEquals([123 => 'x', 'y' => '456'], $this->valkey_glide->hMget('h', $keys));

        // check non-string types.

        $this->valkey_glide->del('h1');
        $this->assertTrue($this->valkey_glide->hMSet('h1', ['x' => 0, 'y' => [], 'z' => new stdclass(), 't' => null]));

        $h1 = $this->valkey_glide->hGetAll('h1');        
        $this->assertEquals('0', $h1['x']);
        
        $this->assertEquals('Array', $h1['y']);

       // $this->assertEquals('Object', $h1['z']); //TODO
        $this->assertEquals('', $h1['t']);

        // hset with fields + values as an associative array
        if (version_compare($this->version, '4.0.0') >= 0) {
            $this->valkey_glide->del('h');
            $this->assertEquals(3, $this->valkey_glide->hSet('h', ['x' => 123, 'y' => 456, 'z' => 'abc']));

            $this->assertEquals(['x' => '123', 'y' => '456', 'z' => 'abc'], $this->valkey_glide->hGetAll('h'));
            $this->assertEquals(0, $this->valkey_glide->hSet('h', ['x' => 789]));
            $this->assertEquals(['x' => '789', 'y' => '456', 'z' => 'abc'], $this->valkey_glide->hGetAll('h'));
        }


        // hset with variadic fields + values
        if (version_compare($this->version, '4.0.0') >= 0) {
            $this->valkey_glide->del('h');
            $this->assertEquals(3, $this->valkey_glide->hSet('h', 'x', 123, 'y', 456, 'z', 'abc'));
            $this->assertEquals(['x' => '123', 'y' => '456', 'z' => 'abc'], $this->valkey_glide->hGetAll('h'));
            $this->assertEquals(0, $this->valkey_glide->hSet('h', 'x', 789));
            $this->assertEquals(['x' => '789', 'y' => '456', 'z' => 'abc'], $this->valkey_glide->hGetAll('h'));
        }

        // hstrlen
        if (version_compare($this->version, '3.2.0') >= 0) {
            $this->valkey_glide->del('h');
            $this->assertEquals(0, $this->valkey_glide->hStrLen('h', 'x')); // key doesn't exist
            $this->valkey_glide->hSet('h', 'foo', 'bar');
            $this->assertEquals(0, $this->valkey_glide->hStrLen('h', 'x')); // field is not present in the hash
            $this->assertEquals(3, $this->valkey_glide->hStrLen('h', 'foo'));
        }
    }

    public function testHRandField()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->MarkTestSkipped();
        }

        $this->valkey_glide->del('key');
        $this->valkey_glide->hMSet('key', ['a' => 0, 'b' => 1, 'c' => 'foo', 'd' => 'bar', 'e' => null]);

        $this->assertInArray($this->valkey_glide->hRandField('key'), ['a', 'b', 'c', 'd', 'e']);

        $result = $this->valkey_glide->hRandField('key', ['count' => 3]);
        $this->assertEquals(3, count($result));
        $this->assertEquals(array_intersect($result, ['a', 'b', 'c', 'd', 'e']), $result);

        $result = $this->valkey_glide->hRandField('key', ['count' => 2, 'withvalues' => true]);

        $this->assertEquals(2, count($result));
        $xx = ['a' => 0, 'b' => 1, 'c' => 'foo', 'd' => 'bar', 'e' => null];
        $this->assertEquals(array_intersect_key($result, $xx), $result);

        /* Make sure PhpValkeyGlide sends COUNt (1) when `WITHVALUES` is set */
        $result = $this->valkey_glide->hRandField('key', ['withvalues' => true]);

        $this->assertIsArray($result);

        $this->assertEquals(1, count($result));

        /* We can return false if the key doesn't exist */
        $this->assertIsInt($this->valkey_glide->del('notahash'));
        $this->assertFalse($this->valkey_glide->hRandField('notahash'));
        $this->assertFalse($this->valkey_glide->hRandField('notahash', ['count' => 2, 'withvalues' => true]));
        $this->assertFalse($this->valkey_glide->hRandField('notahash', ['withvalues' => true]));
    }

    public function testSetRange()
    {

        $this->valkey_glide->del('key');
        $this->valkey_glide->set('key', 'hello world');
        $this->valkey_glide->setRange('key', 6, 'redis');
        $this->assertKeyEquals('hello redis', 'key');
        $this->valkey_glide->setRange('key', 6, 'you'); // don't cut off the end
        $this->assertKeyEquals('hello youis', 'key');

        $this->valkey_glide->set('key', 'hello world');

        // fill with zeros if needed
        $this->valkey_glide->del('key');
        $this->valkey_glide->setRange('key', 6, 'foo');
        $this->assertKeyEquals("\x00\x00\x00\x00\x00\x00foo", 'key');
    }

    public function testObject()
    {
        /* Version 3.0.0 (represented as >= 2.9.0 in redis info)  and moving
         * forward uses 'embstr' instead of 'raw' for small string values */
        if (version_compare($this->version, '2.9.0') < 0) {
            $small_encoding = 'raw';
        } else {
            $small_encoding = 'embstr';
        }

        $this->valkey_glide->del('key');
        $this->assertFalse($this->valkey_glide->object('encoding', 'key'));
        $this->assertFalse($this->valkey_glide->object('refcount', 'key'));
        $this->assertFalse($this->valkey_glide->object('idletime', 'key'));
        $this->assertFalse($this->valkey_glide->object('freq', 'key'));

        $this->valkey_glide->set('key', 'value');
        $this->assertEquals($small_encoding, $this->valkey_glide->object('encoding', 'key'));
        $this->assertEquals(1, $this->valkey_glide->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->valkey_glide->object('idletime', 'key')));


        $this->valkey_glide->del('key');
        $this->valkey_glide->lpush('key', 'value');

        /* ValkeyGlide has improved the encoding here throughout the various versions.  The value
           can either be 'ziplist', 'quicklist', or 'listpack' */
        $encoding = $this->valkey_glide->object('encoding', 'key');
        $this->assertInArray($encoding, ['ziplist', 'quicklist', 'listpack']);

        $this->assertEquals(1, $this->valkey_glide->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->valkey_glide->object('idletime', 'key')));

        $this->valkey_glide->del('key');
        $this->valkey_glide->sadd('key', 'value');

        /* ValkeyGlide 7.2.0 switched to 'listpack' for small sets */
        $encoding = $this->valkey_glide->object('encoding', 'key');
        $this->assertInArray($encoding, ['hashtable', 'listpack']);
        $this->assertEquals(1, $this->valkey_glide->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->valkey_glide->object('idletime', 'key')));

        $this->valkey_glide->del('key');
        $this->valkey_glide->sadd('key', 42);
        $this->valkey_glide->sadd('key', 1729);
        $this->assertEquals('intset', $this->valkey_glide->object('encoding', 'key'));
        $this->assertEquals(1, $this->valkey_glide->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->valkey_glide->object('idletime', 'key')));

        $this->valkey_glide->del('key');
        $this->valkey_glide->lpush('key', str_repeat('A', pow(10, 6))); // 1M elements, too big for a ziplist.

        $encoding = $this->valkey_glide->object('encoding', 'key');
        $this->assertInArray($encoding, ['linkedlist', 'quicklist']);

        $this->assertEquals(1, $this->valkey_glide->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->valkey_glide->object('idletime', 'key')));
    }

    public function testMultiExec()
    {
        $this->MarkTestSkipped();
        $this->sequence(ValkeyGlide::MULTI);

        $this->differentType(ValkeyGlide::MULTI);

        // with prefix as well

        $this->sequence(ValkeyGlide::MULTI);
        $this->differentType(ValkeyGlide::MULTI);

        $this->valkey_glide->set('x', '42');

        $this->assertTrue($this->valkey_glide->watch('x'));
        $ret = $this->valkey_glide->multi()->get('x')->exec();

        // successful transaction
        $this->assertEquals(['42'], $ret);
    }

    public function testFailedTransactions()
    {
         $this->markTestSkipped();//TODO
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
        if (! $this->havePipeline()) {
            $this->markTestSkipped();
        }

        $this->sequence(ValkeyGlide::PIPELINE);
        $this->differentType(ValkeyGlide::PIPELINE);

        // with prefix as well
        $this->valkey_glide->setOption(ValkeyGlide::OPT_PREFIX, 'test:');
        $this->sequence(ValkeyGlide::PIPELINE);
        $this->differentType(ValkeyGlide::PIPELINE);
        $this->valkey_glide->setOption(ValkeyGlide::OPT_PREFIX, '');
    }

    public function testPipelineMultiExec()
    {
        if (! $this->havePipeline()) {
            $this->markTestSkipped();
        }

        $ret = $this->valkey_glide->pipeline()->multi()->exec()->exec();
        $this->assertIsArray($ret);
        $this->assertEquals(1, count($ret)); // empty transaction

        $ret = $this->valkey_glide->pipeline()
            ->ping()
            ->multi()->set('x', 42)->incr('x')->exec()
            ->ping()
            ->multi()->get('x')->del('x')->exec()
            ->ping()
            ->exec();
        $this->assertIsArray($ret);
        $this->assertEquals(5, count($ret)); // should be 5 atomic operations
    }

    public function testMultiEmpty()
    {
         $this->markTestSkipped();//TODO
        $ret = $this->valkey_glide->multi()->exec();
        $this->assertEquals([], $ret);
    }

    public function testPipelineEmpty()
    {
        if (!$this->havePipeline()) {
            $this->markTestSkipped();
        }

        $ret = $this->valkey_glide->pipeline()->exec();
        $this->assertEquals([], $ret);
    }

    /* GitHub issue #1211 (ignore redundant calls to pipeline or multi) */
    public function testDoublePipeNoOp()
    {
         $this->markTestSkipped();//TODO
        /* Only the first pipeline should be honored */
        for ($i = 0; $i < 6; $i++) {
            $this->valkey_glide->pipeline();
        }

        /* Set and get in our pipeline */
        $this->valkey_glide->set('pipecount', 'over9000')->get('pipecount');

        $data = $this->valkey_glide->exec();
        $this->assertEquals([true,'over9000'], $data);

        /* Only the first MULTI should be honored */
        for ($i = 0; $i < 6; $i++) {
            $this->valkey_glide->multi();
        }

        /* Set and get in our MULTI block */
        $this->valkey_glide->set('multicount', 'over9000')->get('multicount');

        $data = $this->valkey_glide->exec();
        $this->assertEquals([true, 'over9000'], $data);
    }

    public function testDiscard()
    {
         $this->markTestSkipped();//TODO
        foreach ([ValkeyGlide::PIPELINE, ValkeyGlide::MULTI] as $mode) {
            /* start transaction */
            $this->valkey_glide->multi($mode);

            /* Set and get in our transaction */
            $this->valkey_glide->set('pipecount', 'over9000')->get('pipecount');

            /* first call closes transaction and clears commands queue */
            $this->assertTrue($this->valkey_glide->discard());

            /* next call fails because mode is ATOMIC */
            $this->assertFalse($this->valkey_glide->discard());
        }
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
        return;
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

        $this->valkey_glide->setOption(ValkeyGlide::OPT_SERIALIZER, $serializer);

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
            ->rpoplpush('{list}lkey', '{list}lDest')
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
        $this->assertEquals('lvalue', $ret[$i++]); // rpoplpush returns the element: 'lvalue'
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
            ->rpoplpush('{list}lkey', '{list}lDest')
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


        $serializer = $this->valkey_glide->getOption(ValkeyGlide::OPT_SERIALIZER);
        $this->valkey_glide->setOption(ValkeyGlide::OPT_SERIALIZER, ValkeyGlide::SERIALIZER_NONE); // testing incr, which doesn't work with the serializer
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
        $this->valkey_glide->setOption(ValkeyGlide::OPT_SERIALIZER, $serializer);

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
            ->rpoplpush('{l}key', '{l}Dest')
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
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // adding zValue11
        $this->assertEquals(1, $ret[$i++]); // adding zValue12
        $this->assertEquals(1, $ret[$i++]); // adding zValue13
        $this->assertEquals(1, $ret[$i++]); // adding zValue14
        $this->assertEquals(1, $ret[$i++]); // adding zValue15
        $this->assertEquals(3, $ret[$i++]); // deleted zValue11, zValue12, zValue13
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]);
        $this->assertEquals(['zValue15', 'zValue14', 'zValue5', 'zValue1'], $ret[$i++]);
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
            ->rPoplPush($key, $dkey . 'lkey1')

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
            ->rPoplPush($key, $dkey . 'lkey1')

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
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
            ->rPoplPush($key, $dkey . 'lkey1')

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
            ->rPoplPush($key, $dkey . 'lkey1')

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

    public function testDifferentTypeString()
    {
        $this->markTestSkipped();

        $key = '{hash}string';
        $dkey = '{hash}' . __FUNCTION__;

        $this->valkey_glide->del($key);
        $this->assertTrue($this->valkey_glide->set($key, 'value'));

        // lists I/F
        $this->assertFalse($this->valkey_glide->rPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lLen($key));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->lrange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->lTrim($key, 0, 1));
        $this->assertFalse($this->valkey_glide->lIndex($key, 0));
        $this->assertFalse($this->valkey_glide->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->valkey_glide->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->rPop($key));        

        // sets I/F
        $this->assertFalse($this->valkey_glide->sAdd($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->srem($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sPop($key));
        $this->assertFalse($this->valkey_glide->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->valkey_glide->scard($key));
        $this->assertFalse($this->valkey_glide->sismember($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->valkey_glide->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->valkey_glide->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->valkey_glide->sMembers($key));
        $this->assertFalse($this->valkey_glide->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->valkey_glide->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRem($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zCount($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zCard($key));
        $this->assertFalse($this->valkey_glide->zScore($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->valkey_glide->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->valkey_glide->hGet($key, 'key1'));
        $this->assertFalse($this->valkey_glide->hMGet($key, ['key1']));
        $this->assertFalse($this->valkey_glide->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->valkey_glide->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->valkey_glide->hExists($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hDel($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hLen($key));
        $this->assertFalse($this->valkey_glide->hKeys($key));
        $this->assertFalse($this->valkey_glide->hVals($key));
        $this->assertFalse($this->valkey_glide->hGetAll($key));
    }

    public function testDifferentTypeList()
    {
         $this->markTestSkipped();
        $key = '{hash}list';
        $dkey = '{hash}' . __FUNCTION__;

        $this->valkey_glide->del($key);
        $this->assertEquals(1, $this->valkey_glide->lPush($key, 'value'));

        // string I/F
        $this->assertFalse($this->valkey_glide->get($key));
        $this->assertFalse($this->valkey_glide->getset($key, 'value2'));
        $this->assertFalse($this->valkey_glide->append($key, 'append'));
        $this->assertFalse($this->valkey_glide->getRange($key, 0, 8));
        $this->assertEquals([false], $this->valkey_glide->mget([$key]));
        $this->assertFalse($this->valkey_glide->incr($key));
        $this->assertFalse($this->valkey_glide->incrBy($key, 1));
        $this->assertFalse($this->valkey_glide->decr($key));
        $this->assertFalse($this->valkey_glide->decrBy($key, 1));

        // sets I/F
        $this->assertFalse($this->valkey_glide->sAdd($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->srem($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sPop($key));
        $this->assertFalse($this->valkey_glide->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->valkey_glide->scard($key));
        $this->assertFalse($this->valkey_glide->sismember($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->valkey_glide->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->valkey_glide->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->valkey_glide->sMembers($key));
        $this->assertFalse($this->valkey_glide->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->valkey_glide->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRem($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zCount($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zCard($key));
        $this->assertFalse($this->valkey_glide->zScore($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->valkey_glide->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->valkey_glide->hGet($key, 'key1'));
        $this->assertFalse($this->valkey_glide->hMGet($key, ['key1']));
        $this->assertFalse($this->valkey_glide->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->valkey_glide->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->valkey_glide->hExists($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hDel($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hLen($key));
        $this->assertFalse($this->valkey_glide->hKeys($key));
        $this->assertFalse($this->valkey_glide->hVals($key));
        $this->assertFalse($this->valkey_glide->hGetAll($key));
    }

    public function testDifferentTypeSet()
    {
         $this->markTestSkipped();
        $key = '{hash}set';
        $dkey = '{hash}' . __FUNCTION__;
        $this->valkey_glide->del($key);
        $this->assertEquals(1, $this->valkey_glide->sAdd($key, 'value'));

        // string I/F
        $this->assertFalse($this->valkey_glide->get($key));
        $this->assertFalse($this->valkey_glide->getset($key, 'value2'));
        $this->assertFalse($this->valkey_glide->append($key, 'append'));
        $this->assertFalse($this->valkey_glide->getRange($key, 0, 8));
        $this->assertEquals([false], $this->valkey_glide->mget([$key]));
        $this->assertFalse($this->valkey_glide->incr($key));
        $this->assertFalse($this->valkey_glide->incrBy($key, 1));
        $this->assertFalse($this->valkey_glide->decr($key));
        $this->assertFalse($this->valkey_glide->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->valkey_glide->rPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lLen($key));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->lrange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->lTrim($key, 0, 1));
        $this->assertFalse($this->valkey_glide->lIndex($key, 0));
        $this->assertFalse($this->valkey_glide->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->valkey_glide->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->rPop($key));
        $this->assertFalse($this->valkey_glide->rPoplPush($key, $dkey  . 'lkey1'));

        // sorted sets I/F
        $this->assertFalse($this->valkey_glide->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRem($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zCount($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zCard($key));
        $this->assertFalse($this->valkey_glide->zScore($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->valkey_glide->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->valkey_glide->hGet($key, 'key1'));
        $this->assertFalse($this->valkey_glide->hMGet($key, ['key1']));
        $this->assertFalse($this->valkey_glide->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->valkey_glide->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->valkey_glide->hExists($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hDel($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hLen($key));
        $this->assertFalse($this->valkey_glide->hKeys($key));
        $this->assertFalse($this->valkey_glide->hVals($key));
        $this->assertFalse($this->valkey_glide->hGetAll($key));
    }

    public function testDifferentTypeSortedSet()
    {
         $this->markTestSkipped();
        $key = '{hash}sortedset';
        $dkey = '{hash}' . __FUNCTION__;

        $this->valkey_glide->del($key);
        $this->assertEquals(1, $this->valkey_glide->zAdd($key, 0, 'value'));

        // string I/F
        $this->assertFalse($this->valkey_glide->get($key));
        $this->assertFalse($this->valkey_glide->getset($key, 'value2'));
        $this->assertFalse($this->valkey_glide->append($key, 'append'));
        $this->assertFalse($this->valkey_glide->getRange($key, 0, 8));
        $this->assertEquals([false], $this->valkey_glide->mget([$key]));
        $this->assertFalse($this->valkey_glide->incr($key));
        $this->assertFalse($this->valkey_glide->incrBy($key, 1));
        $this->assertFalse($this->valkey_glide->decr($key));
        $this->assertFalse($this->valkey_glide->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->valkey_glide->rPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lLen($key));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->lrange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->lTrim($key, 0, 1));
        $this->assertFalse($this->valkey_glide->lIndex($key, 0));
        $this->assertFalse($this->valkey_glide->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->valkey_glide->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->rPop($key));
 

        // sets I/F
        $this->assertFalse($this->valkey_glide->sAdd($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->srem($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sPop($key));
        $this->assertFalse($this->valkey_glide->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->valkey_glide->scard($key));
        $this->assertFalse($this->valkey_glide->sismember($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->valkey_glide->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->valkey_glide->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->valkey_glide->sMembers($key));
        $this->assertFalse($this->valkey_glide->sRandMember($key));

        // hash I/F
        $this->assertFalse($this->valkey_glide->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->valkey_glide->hGet($key, 'key1'));
        $this->assertFalse($this->valkey_glide->hMGet($key, ['key1']));
        $this->assertFalse($this->valkey_glide->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->valkey_glide->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->valkey_glide->hExists($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hDel($key, 'key2'));
        $this->assertFalse($this->valkey_glide->hLen($key));
        $this->assertFalse($this->valkey_glide->hKeys($key));
        $this->assertFalse($this->valkey_glide->hVals($key));
        $this->assertFalse($this->valkey_glide->hGetAll($key));
    }

    public function testDifferentTypeHash()
    {
         $this->markTestSkipped();
        $key = '{hash}hash';
        $dkey = '{hash}hash';

        $this->valkey_glide->del($key);
        $this->assertEquals(1, $this->valkey_glide->hSet($key, 'key', 'value'));

        // string I/F
        $this->assertFalse($this->valkey_glide->get($key));
        $this->assertFalse($this->valkey_glide->getset($key, 'value2'));
        $this->assertFalse($this->valkey_glide->append($key, 'append'));
        $this->assertFalse($this->valkey_glide->getRange($key, 0, 8));
        $this->assertEquals([false], $this->valkey_glide->mget([$key]));
        $this->assertFalse($this->valkey_glide->incr($key));
        $this->assertFalse($this->valkey_glide->incrBy($key, 1));
        $this->assertFalse($this->valkey_glide->decr($key));
        $this->assertFalse($this->valkey_glide->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->valkey_glide->rPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lPush($key, 'lvalue'));
        $this->assertFalse($this->valkey_glide->lLen($key));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->lrange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->lTrim($key, 0, 1));
        $this->assertFalse($this->valkey_glide->lIndex($key, 0));
        $this->assertFalse($this->valkey_glide->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->valkey_glide->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->valkey_glide->lPop($key));
        $this->assertFalse($this->valkey_glide->rPop($key));
        $this->assertFalse($this->valkey_glide->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertFalse($this->valkey_glide->sAdd($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->srem($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sPop($key));
        $this->assertFalse($this->valkey_glide->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->valkey_glide->scard($key));
        $this->assertFalse($this->valkey_glide->sismember($key, 'sValue1'));
        $this->assertFalse($this->valkey_glide->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->valkey_glide->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->valkey_glide->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->valkey_glide->sMembers($key));
        $this->assertFalse($this->valkey_glide->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->valkey_glide->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRem($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRange($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zCount($key, 0, -1));
        $this->assertFalse($this->valkey_glide->zCard($key));
        $this->assertFalse($this->valkey_glide->zScore($key, 'zValue1'));
        $this->assertFalse($this->valkey_glide->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->valkey_glide->zRemRangeByScore($key, 1, 2));
    }



    private function cartesianProduct(array $arrays)
    {
        $result = [[]];

        foreach ($arrays as $array) {
            $append = [];
            foreach ($result as $product) {
                foreach ($array as $item) {
                    $newProduct = $product;
                    $newProduct[] = $item;
                    $append[] = $newProduct;
                }
            }

            $result = $append;
        }

        return $result;
    }


    public function testDumpRestore()
    {

        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('foo');
        $this->valkey_glide->del('bar');

        $this->valkey_glide->set('foo', 'this-is-foo');
        $this->valkey_glide->set('bar', 'this-is-bar');

        $d_foo = $this->valkey_glide->dump('foo');

        $d_bar = $this->valkey_glide->dump('bar');

        $this->valkey_glide->del('foo');
        $this->valkey_glide->del('bar');

        // Assert returns from restore
        $this->assertTrue($this->valkey_glide->restore('foo', 0, $d_bar));
        $this->assertTrue($this->valkey_glide->restore('bar', 0, $d_foo));


        // Now check that the keys have switched
        $this->assertKeyEquals('this-is-bar', 'foo');
        $this->assertKeyEquals('this-is-foo', 'bar');

        /* Test that we can REPLACE a key */
        $this->assertTrue($this->valkey_glide->set('foo', 'some-value'));
        $this->assertTrue($this->valkey_glide->restore('foo', 0, $d_bar, ['REPLACE']));


        /* Ensure we can set an absolute TTL */
        $this->assertTrue($this->valkey_glide->restore('foo', time() + 10, $d_bar, ['REPLACE', 'ABSTTL']));
        $this->assertLTE(10, $this->valkey_glide->ttl('foo'));

        /* Ensure we can set an IDLETIME */
        $this->assertTrue($this->valkey_glide->restore('foo', 0, $d_bar, ['REPLACE', 'IDLETIME' => 200]));
        $this->assertGT(100, $this->valkey_glide->object('idletime', 'foo'));

        /* We can't neccissarily check this depending on LRU policy, but at least attempt to use
           the FREQ option */
        $this->assertTrue($this->valkey_glide->restore('foo', 0, $d_bar, ['REPLACE', 'FREQ' => 200]));

        $this->valkey_glide->del('foo');
        $this->valkey_glide->del('bar');
    }



    // Helper function to compare nested results -- from the php.net array_diff page, I believe
    private function arrayDiffRecursive($aArray1, $aArray2)
    {
        $aReturn = [];

        foreach ($aArray1 as $mKey => $mValue) {
            if (array_key_exists($mKey, $aArray2)) {
                if (is_array($mValue)) {
                    $aRecursiveDiff = $this->arrayDiffRecursive($mValue, $aArray2[$mKey]);
                    if (count($aRecursiveDiff)) {
                        $aReturn[$mKey] = $aRecursiveDiff;
                    }
                } else {
                    if ($mValue != $aArray2[$mKey]) {
                        $aReturn[$mKey] = $mValue;
                    }
                }
            } else {
                $aReturn[$mKey] = $mValue;
            }
        }

        return $aReturn;
    }

    public function testScript()
    {
        //TODO
        $this->markTestSkipped();
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        // Flush any scripts we have
        $this->assertTrue($this->valkey_glide->script('flush'));

        // Silly scripts to test against
        $s1_src = 'return 1';
        $s1_sha = sha1($s1_src);
        $s2_src = 'return 2';
        $s2_sha = sha1($s2_src);
        $s3_src = 'return 3';
        $s3_sha = sha1($s3_src);

        // None should exist
        $result = $this->valkey_glide->script('exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertIsArray($result, 3);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

        // Load them up
        $this->assertEquals($s1_sha, $this->valkey_glide->script('load', $s1_src));
        $this->assertEquals($s2_sha, $this->valkey_glide->script('load', $s2_src));
        $this->assertEquals($s3_sha, $this->valkey_glide->script('load', $s3_src));

        // They should all exist
        $result = $this->valkey_glide->script('exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    public function testEval()
    {
        //TODO
        $this->markTestSkipped();
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        /* The eval_ro method uses the same underlying handlers as eval so we
           only need to verify we can call it. */
        if ($this->minVersionCheck('7.0.0')) {
            $this->assertEquals('1.55', $this->valkey_glide->eval_ro("return '1.55'"));
        }

        // Basic single line response tests
        $this->assertEquals(1, $this->valkey_glide->eval('return 1'));
        $this->assertEqualsWeak(1.55, $this->valkey_glide->eval("return '1.55'"));
        $this->assertEquals('hello, world', $this->valkey_glide->eval("return 'hello, world'"));

        /*
         * Keys to be incorporated into lua results
         */
        // Make a list
        $this->valkey_glide->del('{eval-key}-list');
        $this->valkey_glide->rpush('{eval-key}-list', 'a');
        $this->valkey_glide->rpush('{eval-key}-list', 'b');
        $this->valkey_glide->rpush('{eval-key}-list', 'c');

        // Make a set
        $this->valkey_glide->del('{eval-key}-zset');
        $this->valkey_glide->zadd('{eval-key}-zset', 0, 'd');
        $this->valkey_glide->zadd('{eval-key}-zset', 1, 'e');
        $this->valkey_glide->zadd('{eval-key}-zset', 2, 'f');

        // Basic keys
        $this->valkey_glide->set('{eval-key}-str1', 'hello, world');
        $this->valkey_glide->set('{eval-key}-str2', 'hello again!');

        // Use a script to return our list, and verify its response
        $list = $this->valkey_glide->eval("return redis.call('lrange', KEYS[1], 0, -1)", ['{eval-key}-list'], 1);
        $this->assertEquals(['a', 'b', 'c'], $list);

        // Use a script to return our zset
        $zset = $this->valkey_glide->eval("return redis.call('zrange', KEYS[1], 0, -1)", ['{eval-key}-zset'], 1);
        $this->assertEquals(['d', 'e', 'f'], $zset);

        // Test an empty MULTI BULK response
        $this->valkey_glide->del('{eval-key}-nolist');
        $empty_resp = $this->valkey_glide->eval(
            "return redis.call('lrange', '{eval-key}-nolist', 0, -1)",
            ['{eval-key}-nolist'],
            1
        );
        $this->assertEquals([], $empty_resp);

        // Now test a nested reply
        $nested_script = "
            return {
                1,2,3, {
                    redis.call('get', '{eval-key}-str1'),
                    redis.call('get', '{eval-key}-str2'),
                    redis.call('lrange', 'not-any-kind-of-list', 0, -1),
                    {
                        redis.call('zrange', '{eval-key}-zset', 0, -1),
                        redis.call('lrange', '{eval-key}-list', 0, -1)
                    }
                }
            }
        ";

        $expected = [
            1, 2, 3, [
                'hello, world',
                'hello again!',
                [],
                [
                    ['d', 'e', 'f'],
                    ['a', 'b', 'c']
                ]
            ]
        ];

        // Now run our script, and check our values against each other
        $eval_result = $this->valkey_glide->eval($nested_script, ['{eval-key}-str1', '{eval-key}-str2', '{eval-key}-zset', '{eval-key}-list'], 4);
        $this->assertTrue(
            is_array($eval_result) &&
            count($this->arrayDiffRecursive($eval_result, $expected)) == 0
        );

        /*
         * Nested reply wihin a multi/pipeline block
         */

        $num_scripts = 10;

        $modes = [ValkeyGlide::MULTI];
        if ($this->havePipeline()) {
            $modes[] = ValkeyGlide::PIPELINE;
        }

        foreach ($modes as $mode) {
            $this->valkey_glide->multi($mode);
            for ($i = 0; $i < $num_scripts; $i++) {
                $this->valkey_glide->eval($nested_script, ['{eval-key}-dummy'], 1);
            }
            $replies = $this->valkey_glide->exec();

            foreach ($replies as $reply) {
                $this->assertTrue(
                    is_array($reply) &&
                    count($this->arrayDiffRecursive($reply, $expected)) == 0
                );
            }
        }

        /*
         * KEYS/ARGV
         */

        $args_script = 'return {KEYS[1],KEYS[2],KEYS[3],ARGV[1],ARGV[2],ARGV[3]}';
        $args_args   = ['{k}1', '{k}2', '{k}3', 'v1', 'v2', 'v3'];
        $args_result = $this->valkey_glide->eval($args_script, $args_args, 3);
        $this->assertEquals($args_args, $args_result);

        // turn on key prefixing
        $this->valkey_glide->setOption(ValkeyGlide::OPT_PREFIX, 'prefix:');
        $args_result = $this->valkey_glide->eval($args_script, $args_args, 3);

        // Make sure our first three are prefixed
        for ($i = 0; $i < count($args_result); $i++) {
            if ($i < 3) {
                $this->assertEquals('prefix:' . $args_args[$i], $args_result[$i]);
            } else {
                $this->assertEquals($args_args[$i], $args_result[$i]);
            }
        }
    }

    public function testEvalSHA()
    {
        //TODO
        $this->markTestSkipped();
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        // Flush any loaded scripts
        $this->valkey_glide->script('flush');

        // Non existent script (but proper sha1), and a random (not) sha1 string
        $this->assertFalse($this->valkey_glide->evalsha(sha1(uniqid())));
        $this->assertFalse($this->valkey_glide->evalsha('some-random-data'));

        // Load a script
        $cb  = uniqid(); // To ensure the script is new
        $scr = "local cb='$cb' return 1";
        $sha = sha1($scr);

        // Run it when it doesn't exist, run it with eval, and then run it with sha1
        $this->assertFalse($this->valkey_glide->evalsha($scr));
        $this->assertEquals(1, $this->valkey_glide->eval($scr));
        $this->assertEquals(1, $this->valkey_glide->evalsha($sha));

        /* Our evalsha_ro handler is the same as evalsha so just make sure
           we can invoke the command */
        if ($this->minVersionCheck('7.0.0')) {
            $this->assertEquals(1, $this->valkey_glide->evalsha_ro($sha));
        }
    }

    public function testClient()
    {
        /* CLIENT SETNAME */
        $this->assertTrue($this->valkey_glide->client('setname', 'phpredis_unit_tests'));

        /* CLIENT LIST */
        $clients = $this->valkey_glide->client('list');
        $this->assertIsArray($clients);

        // Figure out which ip:port is us!
        $address = null;
        foreach ($clients as $client) {
            if ($client['name'] == 'phpredis_unit_tests') {
                $address = $client['addr'];
            }
        }

        // We should have found our connection
        $this->assertIsString($address);

        /* CLIENT GETNAME */
        $this->assertEquals('phpredis_unit_tests', $this->valkey_glide->client('getname'));

        if (version_compare($this->version, '5.0.0') >= 0) {
            $this->assertGT(0, $this->valkey_glide->client('id'));

            if (version_compare($this->version, '6.0.0') >= 0) {
                if (version_compare($this->version, '6.2.0') >= 0) {
                    $this->assertFalse(empty($this->valkey_glide->client('info')));

                    if (version_compare($this->version, '7.0.0') >= 0) {
                        $this->assertTrue($this->valkey_glide->client('no-evict', 'on'));
                    }
                }
            }
        }

        /* CLIENT KILL -- phpredis will reconnect, so we can do this */
        $this->assertTrue($this->valkey_glide->client('kill', $address));
    }


    public function testConfig()
    {
        /* GET */
        $cfg = $this->valkey_glide->config('GET', 'timeout');

        $this->assertArrayKey($cfg, 'timeout');
        $sec = $cfg['timeout'];

        /* SET */
        foreach ([$sec + 30, $sec] as $val) {
            $this->assertTrue($this->valkey_glide->config('SET', 'timeout', $val));
            $cfg = $this->valkey_glide->config('GET', 'timeout');
            $this->assertArrayKey($cfg, 'timeout', function ($v) use ($val) {
                return $v == $val;
            });
        }

        /* RESETSTAT */
        $c1 = count($this->valkey_glide->info('commandstats'));
        $this->assertTrue($this->valkey_glide->config('resetstat'));
        $this->assertLT($c1, count($this->valkey_glide->info('commandstats')));


        /* Ensure invalid calls are handled by PhpValkeyGlide */
        foreach (['notacommand', 'get', 'set'] as $cmd) {
            $this->assertFalse(@$this->valkey_glide->config($cmd));
        }
        $this->assertFalse(@$this->valkey_glide->config('set', 'foo'));

        /* REWRITE.  We don't care if it actually works, just that the
           command be attempted */
        $res = $this->valkey_glide->config('rewrite');
        $this->assertIsBool($res);

        if (! $this->minVersionCheck('7.0.0')) {
            return;
        }

        /* Test getting multiple values */
        $settings = $this->valkey_glide->config('get', ['timeout', 'databases', 'set-max-intset-entries']);
        $this->assertTrue(is_array($settings) && isset($settings['timeout']) &&
                          isset($settings['databases']) && isset($settings['set-max-intset-entries']));

        /* Short circuit if the above assertion would have failed */
        if (! is_array($settings) || ! isset($settings['timeout']) || ! isset($settings['set-max-intset-entries'])) {
            return;
        }

        list($timeout, $max_intset) = [$settings['timeout'], $settings['set-max-intset-entries']];

        $updates = [
            ['timeout' => $timeout + 30, 'set-max-intset-entries' => $max_intset + 128],
            ['timeout' => $timeout,      'set-max-intset-entries' => $max_intset],
        ];

        foreach ($updates as $update) {
            $this->assertTrue($this->valkey_glide->config('set', $update));

            $vals = $this->valkey_glide->config('get', array_keys($update));
            $this->assertEqualsWeak($vals, $update, true);
        }

        /* Make sure PhpValkeyGlide catches malformed multiple get/set calls */
        $this->assertFalse(@$this->valkey_glide->config('get', []));
        $this->assertFalse(@$this->valkey_glide->config('set', []));
        $this->assertFalse(@$this->valkey_glide->config('set', [0, 1, 2]));
    }

    public function testReconnectSelect()
    {
        $key = 'reconnect-select';
        $value = 'Has been set!';

        $original_cfg = $this->valkey_glide->config('GET', 'timeout');

        // Make sure the default DB doesn't have the key.
        $this->valkey_glide->select(0);
        $this->valkey_glide->del($key);

        // Set the key on a different DB.
        $this->valkey_glide->select(5);
        $this->valkey_glide->set($key, $value);

        // Time out after 1 second.
        $this->valkey_glide->config('SET', 'timeout', '1');

        // Wait for the connection to time out.  On very old versions
        // of ValkeyGlide we need to wait much longer (TODO:  Investigate
        // which version exactly)
        sleep(5);
        $this->assertFalse($this->valkey_glide->get($key));

        $this->valkey_glide->select(5);
        // Make sure we're still using the same DB.
        $this->assertKeyEquals($value, $key);

        // Revert the setting.
        $this->valkey_glide->config('SET', 'timeout', $original_cfg['timeout']);
    }

    public function testTime()
    {
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->markTestSkipped();
        }

        $time_arr = $this->valkey_glide->time();
        $this->assertTrue(is_array($time_arr) && count($time_arr) == 2 &&
                          strval(intval($time_arr[0])) === strval($time_arr[0]) &&
                          strval(intval($time_arr[1])) === strval($time_arr[1]));
    }






    /**
     * Scan and variants
     */

    protected function getKeyspaceCount($db)
    {
        $info = $this->valkey_glide->info();
        if (isset($info[$db])) {
            $info = $info[$db];
            $info = explode(',', $info);
            $info = explode('=', $info[0]);
            return $info[1];
        } else {
            return 0;
        }
    }

    public function testScan()
    {
        set_time_limit(10); // Enforce a 10-second limit on this test
        if (version_compare($this->version, '2.8.0') < 0) {
            $this->markTestSkipped();
        }
        $ttl = 0;
        // Key count
        $key_count = $this->getKeyspaceCount('db0');

        // Scan them all
        $it = null;
        while (true) {
            $ttl += 1;
            $keys = $this->valkey_glide->scan($it);
            if ($keys) {
                $key_count -= count($keys);
            }
            if ($it == "0") {
                break;
            }
            $this->assertNotEquals(1000, $ttl);
            if ($ttl > 1000) {
                break;
            }
        }
        // Should have iterated all keys
        $this->assertEquals(0, $key_count);

        // Unique keys, for pattern matching
        $uniq = uniqid();
        for ($i = 0; $i < 10; $i++) {
            $this->valkey_glide->set($uniq . "::$i", "bar::$i");
        }

        // Scan just these keys using a pattern match
        $it = null;

        while (true) {
            $ttl += 1;
            $keys = $this->valkey_glide->scan($it, "*$uniq*");

            if ($keys) {
                $i -= count($keys);
            }

            if ($it == "0") {
                break;
            }
            $this->assertNotEquals(1000, $ttl);
            if ($ttl > 1000) {
                break;
            }
        }

        $this->assertEquals(0, $i);

        // SCAN with type is scheduled for release in ValkeyGlide 6.
        if (version_compare($this->version, '6.0.0') >= 0) {
            // Use a unique ID so we can find our type keys
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
            foreach ($keys as $type => $vals) {
                foreach ([0, 10] as $count) {
                    $resp = [];

                    $it = null;
                    while (true) {
                        $ttl += 1;
                        $scan = $this->valkey_glide->scan($it, "*$id*", $count, $type);
                        if ($scan) {
                            $resp = array_merge($resp, $scan);
                        }
                        if ($it == "0") {
                            break;
                        }
                        $this->assertNotEquals(1000, $ttl);
                        if ($ttl > 1000) {
                            break;
                        }
                    }

                    $this->assertEqualsCanonicalizing($vals, $resp);
                }
            }
        }

        $this->assertLT(1000, $ttl);
        set_time_limit(0);  // Reset to unlimited (or default) at the end
    }

    public function testHScan()
    {
        set_time_limit(10); // Enforce a 10-second limit on this test
        if (version_compare($this->version, '2.8.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('hash');
        $foo_mems = 0;

        for ($i = 0; $i < 1000; $i++) {
            if ($i > 3) {
                $this->valkey_glide->hset('hash', "member:$i", "value:$i");
            } else {
                $this->valkey_glide->hset('hash', "foomember:$i", "value:$i");
                $foo_mems++;
            }
        }

        // Scan all of them
        $it = null;
        while (true) {
            $keys = $this->valkey_glide->hscan('hash', $it);
            if ($keys) {
                $i -= count($keys);
            }
            if ($it == "0") {
                break;
            }
        }

        $this->assertEquals(0, $i);

        // Scan just *foomem* (should be 4)
        $it = null;
        while (true) {
            $keys = $this->valkey_glide->hscan('hash', $it, '*foomember*', 1000);
            if ($keys) {
                $foo_mems -= count($keys);
                foreach ($keys as $mem => $val) {
                    $this->assertStringContains('member', $mem);
                    $this->assertStringContains('value', $val);
                }
            }
            if ($it == 0) {
                break;
            }
        }

        $this->assertEquals(0, $foo_mems);
        set_time_limit(0);  // Reset to unlimited (or default) at the end
    }

    public function testSScan()
    {
        set_time_limit(10); // Enforce a 10-second limit on this test
        if (version_compare($this->version, '2.8.0') < 0) {
            $this->markTestSkipped();
        }



        $this->valkey_glide->del('set');
        for ($i = 0; $i < 100; $i++) {
            $this->valkey_glide->sadd('set', "member:$i");
        }

        // Scan all of them
        $it = null;
        while (true) {
            $keys = $this->valkey_glide->sscan('set', $it);
            $i -= count($keys);
            foreach ($keys as $mem) {
                $this->assertStringContains('member', $mem);
            }
            if ($it == "0") {
                break;
            }
        }
        $this->assertEquals(0, $i);

        // Scan just ones with zero in them (0, 10, 20, 30, 40, 50, 60, 70, 80, 90)
        $it = null;
        $w_zero = 0;
        while (true) {
            $keys = $this->valkey_glide->sscan('set', $it, '*0*');
            $w_zero += count($keys);
            if ($it == "0") {
                break;
            }
        }
        $this->assertEquals(10, $w_zero);
        set_time_limit(0);  // Reset to unlimited (or default) at the end
    }

    public function testZScan()
    {
        set_time_limit(10); // Enforce a 10-second limit on this test
        if (version_compare($this->version, '2.8.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('zset');

        [$t_score, $p_score, $p_count] = [0, 0, 0];
        for ($i = 0; $i < 2000; $i++) {
            if ($i < 10) {
                $this->valkey_glide->zadd('zset', $i, "pmem:$i");
                $p_score += $i;
                $p_count++;
            } else {
                $this->valkey_glide->zadd('zset', $i, "mem:$i");
            }

            $t_score += $i;
        }

        // Scan them all
        $it = null;
        while (true) {
            $keys = $this->valkey_glide->zscan('zset', $it);

            foreach ($keys as $mem => $f_score) {
                $t_score -= $f_score;
                $i--;
            }

            if ($it == "0") {
                break;
            }
        }

        $this->assertEquals(0, $i);
        $this->assertEquals(0, $t_score);

        // Just scan 'pmem' members
        $it = null;
        $p_score_old = $p_score;
        $p_count_old = $p_count;
        while (true) {
            $keys = $this->valkey_glide->zscan('zset', $it, '*pmem*');
            if ($keys) {
                foreach ($keys as $mem => $f_score) {
                    $p_score -= $f_score;
                    $p_count -= 1;
                }
            }
            if ($it == "0") {
                break;
            }
        }
        $this->assertEquals(0, $p_score);
        $this->assertEquals(0, $p_count);

        // Turn off retrying and we should get some empty results
        [$skips, $p_score, $p_count] = [0, $p_score_old, $p_count_old];

        $it = null;
        while (true) {
            $keys = $this->valkey_glide->zscan('zset', $it, '*pmem*');

            if ($keys !== false) {
                if (count($keys) == 0) {
                    $skips++;
                }
                foreach ($keys as $mem => $f_score) {
                    $p_score -= $f_score;
                    $p_count -= 1;
                }
            }
            if ($it == "0") {
                break;
            }
        }
        // We should still get all the keys, just with several empty results
        $this->assertGT(0, $skips);
        $this->assertEquals(0, $p_score);
        $this->assertEquals(0, $p_count);
        set_time_limit(0);  // Reset to unlimited (or default) at the end
    }

    /* Make sure we capture errors when scanning */
    public function testScanErrors()
    {

        $this->valkey_glide->set('scankey', 'simplekey');

        foreach (['sScan', 'hScan', 'zScan'] as $method) {
            $it = null;
            $this->valkey_glide->$method('scankey', $it);
        }
    }

    //
    // HyperLogLog (PF) commands
    //

    protected function createPFKey($key, $count)
    {
        $mems = [];
        for ($i = 0; $i < $count; $i++) {
            $mems[] = uniqid('pfmem:');
        }

        // Estimation by ValkeyGlide
        $this->valkey_glide->pfAdd($key, $count);
    }

    public function testPFCommands()
    {
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->markTestSkipped();
        }

        $mems = [];

        for ($i = 0; $i < 1000; $i++) {
            if ($i % 2 == 0) {
                $mems[] = uniqid();
            } else {
                $mems[] = $i;
            }
        }

        // How many keys to create
        $key_count = 10;

        // Iterate prefixing/serialization options

                $keys = [];

                // Now add for each key
        for ($i = 0; $i < $key_count; $i++) {
            $key    = "{key}:$i";
            $keys[] = $key;

            // Clean up this key
            $this->valkey_glide->del($key);

            // Add to our cardinality set, and confirm we got a valid response
            $this->assertGT(0, $this->valkey_glide->pfadd($key, $mems));

            // Grab estimated cardinality
            $card = $this->valkey_glide->pfcount($key);

            $this->assertIsInt($card);

            // Count should be close
            $this->assertBetween($card, count($mems) * .9, count($mems) * 1.1);

            // The PFCOUNT on this key should be the same as the above returned response
            $this->assertEquals($card, $this->valkey_glide->pfcount($key));
        }

                // Clean up merge key
                $this->valkey_glide->del('pf-merge-{key}');

                // Merge the counters
                $this->assertTrue($this->valkey_glide->pfmerge('pf-merge-{key}', $keys));

                // Validate our merged count
                $valkey_glide_card = $this->valkey_glide->pfcount('pf-merge-{key}');

                // Merged cardinality should still be roughly 1000
                $this->assertBetween(
                    $valkey_glide_card,
                    count($mems) * .9,
                    count($mems) * 1.1
                );

                // Clean up merge key
                $this->valkey_glide->del('pf-merge-{key}');
    }

    //
    // GEO* command tests
    //

    protected function rawCommandArray($key, $args)
    {
        return call_user_func_array([$this->valkey_glide, 'rawCommand'], $args);
    }

    protected function addCities($key)
    {
        $this->valkey_glide->del($key);
        foreach ($this->cities as $city => $longlat) {
            $this->valkey_glide->geoadd($key, $longlat[0], $longlat[1], $city);
        }
    }

    /* GEOADD */
    public function testGeoAdd()
    {
        if (! $this->minVersionCheck('3.2')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('geokey');

        /* Add them one at a time */
        foreach ($this->cities as $city => $longlat) {
            $this->assertEquals(1, $this->valkey_glide->geoadd('geokey', $longlat[0], $longlat[1], $city));
        }

        /* Add them again, all at once */
        $args = ['geokey'];
        foreach ($this->cities as $city => $longlat) {
            $args = array_merge($args, [$longlat[0], $longlat[1], $city]);
        }

        /* They all exist, should be nothing added */
        $this->assertEquals(call_user_func_array([$this->valkey_glide, 'geoadd'], $args), 0);
    }



    public function testGeoPos()
    {
        if (! $this->minVersionCheck('3.2.0')) {
            $this->markTestSkipped();
        }

        $this->addCities('gk');
        $this->assertEquals($this->rawCommandArray('gk', ['geopos', 'gk', 'Chico', 'Sacramento']), $this->valkey_glide->geopos('gk', 'Chico', 'Sacramento'));
        $this->assertEquals($this->rawCommandArray('gk', ['geopos', 'gk', 'Cupertino']), $this->valkey_glide->geopos('gk', 'Cupertino'));
    }

    public function testGeoHash()
    {
        if (! $this->minVersionCheck('3.2.0')) {
            $this->markTestSkipped();
        }

        $this->addCities('gk');
        $this->assertEquals($this->rawCommandArray('gk', ['geohash', 'gk', 'Chico', 'Sacramento']), $this->valkey_glide->geohash('gk', 'Chico', 'Sacramento'));
        $this->assertEquals($this->rawCommandArray('gk', ['geohash', 'gk', 'Chico']), $this->valkey_glide->geohash('gk', 'Chico'));
    }

    public function testGeoDist()
    {
        if (! $this->minVersionCheck('3.2.0')) {
            $this->markTestSkipped();
        }

        $this->addCities('gk');

        $r1 = $this->valkey_glide->geodist('gk', 'Chico', 'Cupertino');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino']);
        $this->assertEquals(round($r1, 8), round($r2, 8));

        $r1 = $this->valkey_glide->geodist('gk', 'Chico', 'Cupertino', 'km');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino', 'km']);
        $this->assertEquals(round($r1, 8), round($r2, 8));
    }

    public function testGeoSearch()
    {
        if (! $this->minVersionCheck('6.2.0')) {
            $this->markTestSkipped();
        }

        $this->addCities('gk');

        $this->assertEquals(['Chico'], $this->valkey_glide->geosearch('gk', 'Chico', 1, 'm'));
        $res = $this->valkey_glide->geosearch('gk', 'Chico', 1, 'm', ['withcoord', 'withdist', 'withhash']);

        $this->assertValidate($this->valkey_glide->geosearch('gk', 'Chico', 1, 'm', ['withcoord', 'withdist', 'withhash']), function ($v) {

            $this->assertArrayKey($v, 'Chico', 'is_array');

            $this->assertEquals(count($v['Chico']), 3);
            $this->assertArrayKey($v['Chico'], 0, 'is_float');
            $this->assertArrayKey($v['Chico'], 1, 'is_int');
            $this->assertArrayKey($v['Chico'], 2, 'is_array');
            return true;
        });
    }

    public function testGeoSearchStore()
    {
        if (! $this->minVersionCheck('6.2.0')) {
            $this->markTestSkipped();
        }

        $this->addCities('{gk}src');
        $this->assertEquals(3, $this->valkey_glide->geosearchstore('{gk}dst', '{gk}src', 'Chico', 100, 'km'));
        $this->assertEquals(['Chico'], $this->valkey_glide->geosearch('{gk}dst', 'Chico', 1, 'm'));
    }

    /* Test a 'raw' command */
    public function testRawCommand()
    {
        $key = uniqid();

        $this->valkey_glide->set($key, 'some-value');
        $result = $this->valkey_glide->rawCommand('get', $key);
        $this->assertEquals($result, 'some-value');

        $this->valkey_glide->del('mylist');
        $this->valkey_glide->rpush('mylist', 'A', 'B', 'C', 'D');
        $this->assertEquals(['A', 'B', 'C', 'D'], $this->valkey_glide->lrange('mylist', 0, -1));
    }

    /* STREAMS */

    protected function addStreamEntries($key, $count)
    {
        $ids = [];

        $this->valkey_glide->del($key);

        for ($i = 0; $i < $count; $i++) {
            $ids[] = $this->valkey_glide->xAdd($key, '*', ['field' => "value:$i"]);
        }

        return $ids;
    }

    protected function addStreamsAndGroups($streams, $count, $groups)
    {
        $ids = [];

        foreach ($streams as $stream) {
            $ids[$stream] = $this->addStreamEntries($stream, $count);
            foreach ($groups as $group => $id) {
                $this->valkey_glide->xGroup('CREATE', $stream, $group, $id);
            }
        }

        return $ids;
    }

    public function testXAdd()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('stream');
        for ($i = 0; $i < 5; $i++) {
            $id = $this->valkey_glide->xAdd('stream', '*', ['k1' => 'v1', 'k2' => 'v2']);
            $this->assertEquals($i + 1, $this->valkey_glide->xLen('stream'));

            /* ValkeyGlide should return <timestamp>-<sequence> */
            $bits = explode('-', $id);
            $this->assertEquals(count($bits), 2);
            $this->assertTrue(is_numeric($bits[0]));
            $this->assertTrue(is_numeric($bits[1]));
        }

        /* Test an absolute maximum length */
        for ($i = 0; $i < 100; $i++) {
            $this->valkey_glide->xAdd('stream', '*', ['k' => 'v'], 10);
        }
        $this->assertEquals(10, $this->valkey_glide->xLen('stream'));

        /* Not the greatest test but I'm unsure if approximate trimming is
         * totally deterministic, so just make sure we are able to add with
         * an approximate maxlen argument structure */
        $id = $this->valkey_glide->xAdd('stream', '*', ['k' => 'v'], 10, true);
        $this->assertEquals(count(explode('-', $id)), 2);

        /* Empty message should fail */
        @$this->valkey_glide->xAdd('stream', '*', []);
    }

    protected function doXRangeTest($reverse)
    {
        $key = '{stream}';

        if ($reverse) {
            list($cmd,$a1,$a2) = ['xRevRange', '+', 0];
        } else {
            list($cmd,$a1,$a2) = ['xRange', 0, '+'];
        }

        $this->valkey_glide->del($key);
        for ($i = 0; $i < 3; $i++) {
            $msg = ['field' => "value:$i"];
            $id = $this->valkey_glide->xAdd($key, '*', $msg);
            $rows[$id] = $msg;
        }

        $messages = $this->valkey_glide->$cmd($key, $a1, $a2);

        $this->assertEquals(count($messages), 3);


        $i = $reverse ? 2 : 0;
        foreach ($messages as $seq => $v) {
            $this->assertEquals(count(explode('-', $seq)), 2);
            $this->assertEquals($v, ['field' => "value:$i"]);
            $i += $reverse ? -1 : 1;
        }

        /* Test COUNT option */
        for ($count = 1; $count <= 3; $count++) {
            $messages = $this->valkey_glide->$cmd($key, $a1, $a2, $count);
            $this->assertEquals(count($messages), $count);
        }
    }

    public function testXRange()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        foreach ([false, true] as $reverse) {
            foreach ([null] as $prefix) {
                $this->doXRangeTest($reverse);
            }
        }
    }

    protected function testXLen()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('{stream}');
        for ($i = 0; $i < 5; $i++) {
            $this->valkey_glide->xadd('{stream}', '*', ['foo' => 'bar']);
            $this->assertEquals($i + 1, $this->valkey_glide->xLen('{stream}'));
        }
    }

    public function testXGroup()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        /* CREATE MKSTREAM */
        $key = 's:' . uniqid();
        $this->assertFalse($this->valkey_glide->xGroup('CREATE', $key, 'g0', 0));


        $this->assertTrue($this->valkey_glide->xGroup('CREATE', $key, 'g1', 0, true));

        /* XGROUP DESTROY */
        $this->assertTrue($this->valkey_glide->xGroup('DESTROY', $key, 'g1'));

        /* Populate some entries in stream 's' */
        $this->addStreamEntries('s', 2);

        /* CREATE */
        $this->assertTrue($this->valkey_glide->xGroup('CREATE', 's', 'mygroup', '$'));
        $this->assertFalse($this->valkey_glide->xGroup('CREATE', 's', 'mygroup', 'BAD_ID'));

        /* BUSYGROUP */
        $this->assertFalse($this->valkey_glide->xGroup('CREATE', 's', 'mygroup', '$'));


        /* SETID */
        $this->assertTrue($this->valkey_glide->xGroup('SETID', 's', 'mygroup', '$'));
        $this->assertFalse($this->valkey_glide->xGroup('SETID', 's', 'mygroup', 'BAD_ID'));

        $this->assertEquals(0, $this->valkey_glide->xGroup('DELCONSUMER', 's', 'mygroup', 'myconsumer'));

        if (! $this->minVersionCheck('6.2.0')) {
            return;
        }

        /* CREATECONSUMER */
        $this->assertEquals(1, $this->valkey_glide->del('s'));
        $this->assertTrue($this->valkey_glide->xgroup('create', 's', 'mygroup', '$', true));
        for ($i = 0; $i < 3; $i++) {
            $this->assertTrue($this->valkey_glide->xgroup('createconsumer', 's', 'mygroup', "c:$i"));
            $info = $this->valkey_glide->xinfo('consumers', 's', 'mygroup');
            $this->assertIsArray($info, $i + 1);
            for ($j = 0; $j <= $i; $j++) {
                $this->assertTrue(isset($info[$j]) && isset($info[$j]['name']) && $info[$j]['name'] == "c:$j");
            }
        }

        /* Make sure we don't erroneously send options that don't belong to the operation */
        $this->assertFalse(
            $this->valkey_glide->xGroup('CREATECONSUMER', 's', 'mygroup', 'fake-consumer', true, 1337)
        );

        /* Make sure we handle the case where the user doesn't send enough arguments */
        $this->assertFalse(@$this->valkey_glide->xGroup('CREATECONSUMER'));

        $this->assertFalse(@$this->valkey_glide->xGroup('create'));
        return;
        if (! $this->minVersionCheck('7.0.0')) {
            return;
        }

        /* ENTRIESREAD */
        $this->assertEquals(1, $this->valkey_glide->del('s'));
        $this->assertTrue($this->valkey_glide->xGroup('create', 's', 'mygroup', '$', true, 1337));
        $info = $this->valkey_glide->xinfo('groups', 's');
        $this->assertTrue(isset($info[0]['entries-read']) && 1337 == (int)$info[0]['entries-read']);
    }

    public function testXAck()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        for ($n = 1; $n <= 3; $n++) {
            $this->addStreamsAndGroups(['{s}'], 3, ['g1' => 0]);

            $msg = $this->valkey_glide->xReadGroup('g1', 'c1', ['{s}' => '>']);
            /* Extract IDs */
            $smsg = array_shift($msg);
            $ids = array_keys($smsg);

            /* Now ACK $n messages */
            $ids = array_slice($ids, 0, $n);

            $this->assertEquals($n, $this->valkey_glide->xAck('{s}', 'g1', $ids));
        }

        /* Verify sending no IDs is a failure */
        $this->assertFalse($this->valkey_glide->xAck('{s}', 'g1', []));
    }

    protected function doXReadTest()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        $row = ['f1' => 'v1', 'f2' => 'v2'];
        $msgdata = [
            '{stream}-1' => $row,
            '{stream}-2' => $row,
        ];

        /* Append a bit of data and populate STREAM queries */
        $this->valkey_glide->del(array_keys($msgdata));
        foreach ($msgdata as $key => $message) {
            for ($r = 0; $r < 2; $r++) {
                $id = $this->valkey_glide->xAdd($key, '*', $message);
                $qresult[$key][$id] = $message;
            }
            $qzero[$key] = 0;
            $qnew[$key] = '$';
            $keys[] = $key;
        }

        /* Everything from both streams */
        $rmsg = $this->valkey_glide->xRead($qzero);


        $this->assertEquals($rmsg, $qresult);

        /* Test COUNT option */
        for ($count = 1; $count <= 2; $count++) {
            $rmsg = $this->valkey_glide->xRead($qzero, $count);


            foreach ($keys as $key) {
                $this->assertEquals(count($rmsg[$key]), $count);
            }
        }
        $out = $this->valkey_glide->xRead($qnew);

        /* Should be empty (no new entries) */
        $this->assertEquals(count($out), 0);

        /* Test against a specific ID */
        $id = $this->valkey_glide->xAdd('{stream}-1', '*', $row);
        $new_id = $this->valkey_glide->xAdd('{stream}-1', '*', ['final' => 'row']);

        $rmsg = $this->valkey_glide->xRead(['{stream}-1' => $id]);



        /* Empty query should fail */
        $this->assertFalse(@$this->valkey_glide->xRead([]));
    }

    public function testXRead()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        $this->doXReadTest();

        /* Don't need to test BLOCK multiple times */
        $m1 = round(microtime(true) * 1000);
        $this->valkey_glide->xRead(['somestream' => '$'], -1, 100);
        $m2 = round(microtime(true) * 1000);
        $this->assertGT(99, $m2 - $m1);
    }

    protected function compareStreamIds($valkey_glide, $control)
    {
        foreach ($control as $stream => $ids) {
            $rcount = count($valkey_glide[$stream]);
            $lcount = count($control[$stream]);

            /* We should have the same number of messages */
            $this->assertEquals($rcount, $lcount);

            /* We should have the exact same IDs */
            foreach ($ids as $k => $id) {
                $this->assertTrue(isset($valkey_glide[$stream][$id]));
            }
        }
    }

    public function testXReadGroup()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        /* Create some streams and groups */
        $streams = ['{s}-1', '{s}-2'];
        $groups = ['group1' => 0, 'group2' => 0];

        /* I'm not totally sure why ValkeyGlide behaves this way, but we have to
         * send '>' first and then send ID '0' for subsequent xReadGroup calls
         * or ValkeyGlide will not return any messages.  This behavior changed from
         * redis 5.0.1 and 5.0.2 but doing it this way works for both versions. */
        $qcount = 0;
        $query1 = ['{s}-1' => '>', '{s}-2' => '>'];
        $query2 = ['{s}-1' => '0', '{s}-2' => '0'];

        $ids = $this->addStreamsAndGroups($streams, 1, $groups);

        /* Test that we get get the IDs we should */
        foreach (['group1', 'group2'] as $group) {
            foreach ($ids as $stream => $messages) {
                while ($ids[$stream]) {
                    /* Read more messages */
                    $query = !$qcount++ ? $query1 : $query2;
                    $resp = $this->valkey_glide->xReadGroup($group, 'consumer', $query);

                    /* They should match with our local control array */
                    $this->compareStreamIds($resp, $ids);

                    /* Remove a message from our control *and* XACK it in ValkeyGlide */
                    $id = array_shift($ids[$stream]);
                    $this->valkey_glide->xAck($stream, $group, [$id]);
                }
            }
        }

        /* Test COUNT option */
        for ($c = 1; $c <= 3; $c++) {
            $this->addStreamsAndGroups($streams, 3, $groups);
            $resp = $this->valkey_glide->xReadGroup('group1', 'consumer', $query1, $c);

            foreach ($resp as $stream => $smsg) {
                $this->assertEquals(count($smsg), $c);
            }
        }

        /* Test COUNT option with NULL (should be ignored) */
        $this->addStreamsAndGroups($streams, 3, $groups, null);
        $resp = $this->valkey_glide->xReadGroup('group1', 'consumer', $query1, 0);
        foreach ($resp as $stream => $smsg) {
            $this->assertEquals(count($smsg), 3);
        }

        /* Finally test BLOCK with a sloppy timing test */
        $tm1 = $this->mstime();
        $qnew = ['{s}-1' => '>', '{s}-2' => '>'];

        $this->valkey_glide->xReadGroup('group1', 'c1', $qnew, 0, 100);

        $this->assertGTE(100, $this->mstime() - $tm1);

        /* Make sure passing NULL to block doesn't block */
        $tm1 = $this->mstime();
        $this->valkey_glide->xReadGroup('group1', 'c1', $qnew, 0);
        $this->assertLT(100, $this->mstime() - $tm1);

        /* Make sure passing bad values to BLOCK or COUNT immediately fails */
        $this->assertFalse(@$this->valkey_glide->xReadGroup('group1', 'c1', $qnew, -1));
        $this->assertFalse(@$this->valkey_glide->xReadGroup('group1', 'c1', $qnew, null, -1));
    }

    public function testXPending()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        $rows = 5;
        $this->addStreamsAndGroups(['s'], $rows, ['group' => 0]);

        $msg = $this->valkey_glide->xReadGroup('group', 'consumer', ['s' => 0]);

        $ids = array_keys($msg['s']);

        for ($n = count($ids); $n >= 0; $n--) {
            $xp = $this->valkey_glide->xPending('s', 'group');

            $this->assertEquals(count($ids), $xp[0]);

            /* Verify we're seeing the IDs themselves */
            for ($idx = 1; $idx <= 2; $idx++) {
                if ($xp[$idx]) {
                    $this->assertPatternMatch('/^[0-9].*-[0-9].*/', $xp[$idx]);
                }
            }

            if ($ids) {
                $id = array_shift($ids);
                $this->valkey_glide->xAck('s', 'group', [$id]);
            }
        }
    }

    public function testXDel()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        for ($n = 5; $n > 0; $n--) {
            $ids = $this->addStreamEntries('s', 5);
            $todel = array_slice($ids, 0, $n);
            $this->assertEquals(count($todel), $this->valkey_glide->xDel('s', $todel));
        }

        /* Empty array should fail */
        $this->assertFalse(@$this->valkey_glide->xDel('s', []));
    }

    public function testXTrim()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        for ($maxlen = 0; $maxlen <= 50; $maxlen += 10) {
            $this->addStreamEntries('stream', 100);
            $trimmed = $this->valkey_glide->xTrim('stream', $maxlen);
            $this->assertEquals(100 - $maxlen, $trimmed);
        }

        /* APPROX trimming isn't easily deterministic, so just make sure we
           can call it with the flag */
        $this->addStreamEntries('stream', 100);
        $this->assertEquals(0, $this->valkey_glide->xTrim('stream', 1, true));

        /* We need ValkeyGlide >= 6.2.0 for MINID and LIMIT options */
        if (! $this->minVersionCheck('6.2.0')) {
            return;
        }

        $this->assertEquals(1, $this->valkey_glide->del('stream'));

        /* Test minid by generating a stream with more than one */
        for ($i = 1; $i < 3; $i++) {
            for ($j = 0; $j < 3; $j++) {
                $this->valkey_glide->xadd('stream', "$i-$j", ['foo' => 'bar']);
            }
        }

        /* MINID of 2-0 */
        $this->assertEquals(3, $this->valkey_glide->xtrim('stream', 2, false, true));
        $this->assertEquals(['2-0', '2-1', '2-2'], array_keys($this->valkey_glide->xrange('stream', '0', '+')));

        /* TODO:  Figure oiut how to test LIMIT deterministically.  For now just
                  send a LIMIT and verify we don't get a failure from ValkeyGlide. */
        $this->assertIsInt(@$this->valkey_glide->xtrim('stream', 2, true, false, 3));
    }

    /* XCLAIM is one of the most complicated commands, with a great deal of different options
     * The following test attempts to verify every combination of every possible option. */
    public function testXClaim()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        foreach ([0, 100] as $min_idle_time) {
            foreach ([false, true] as $justid) {
                foreach ([0, 10] as $retrycount) {
                    /* We need to test not passing TIME/IDLE as well as passing either */
                    if ($min_idle_time == 0) {
                        $topts = [[], ['IDLE', 1000000], ['TIME', time() * 1000]];
                    } else {
                        $topts = [null];
                    }

                    foreach ($topts as $tinfo) {
                        if ($tinfo) {
                            list($ttype, $tvalue) = $tinfo;
                        } else {
                            $ttype = null;
                            $tvalue = null;
                        }

                        /* Add some messages and create a group */
                        $this->addStreamsAndGroups(['s'], 10, ['group1' => 0]);

                        /* Create a second stream we can FORCE ownership on */
                        $fids = $this->addStreamsAndGroups(['f'], 10, ['group1' => 0]);
                        $fids = $fids['f'];

                        /* Have consumer 'Mike' read the messages */
                        $oids = $this->valkey_glide->xReadGroup('group1', 'Mike', ['s' => '>']);
                        $oids = array_keys($oids['s']); /* We're only dealing with stream 's' */

                        /* Construct our options array */
                        $opts = [];
                        if ($justid) {
                            $opts[] = 'JUSTID';
                        }
                        if ($retrycount) {
                            $opts['RETRYCOUNT'] = $retrycount;
                        }
                        if ($tvalue !== null) {
                            $opts[$ttype] = $tvalue;
                        }

                        /* Now have pavlo XCLAIM them */
                        $cids = $this->valkey_glide->xClaim('s', 'group1', 'Pavlo', $min_idle_time, $oids, $opts);

                        if (! $justid) {
                            $cids = array_keys($cids);
                        }

                        if ($min_idle_time == 0) {
                            $this->assertEquals($cids, $oids);

                            /* Append the FORCE option to our second stream where we have not already
                             * assigned to a PEL group */
                            $opts[] = 'FORCE';
                            $freturn = $this->valkey_glide->xClaim('f', 'group1', 'Test', 0, $fids, $opts);

                            if (! $justid) {
                                $freturn = array_keys($freturn);
                            }

                            $this->assertEquals($freturn, $fids);

                            if ($retrycount || $tvalue !== null) {
                                $pending = $this->valkey_glide->xPending('s', 'group1', 0, '+', 1, 'Pavlo');

                                if ($retrycount) {
                                    $this->assertEquals($pending[0][3], $retrycount);
                                }
                                if ($tvalue !== null) {
                                    if ($ttype == 'IDLE') {
                                        /* If testing IDLE the value must be >= what we set */
                                        $this->assertGTE($tvalue, $pending[0][2]);
                                    } else {
                                        /* Timing tests are notoriously irritating but I don't see
                                         * how we'll get >= 20,000 ms between XCLAIM and XPENDING no
                                         * matter how slow the machine/VM running the tests is */
                                        $this->assertLT(20000, $pending[0][2]);
                                    }
                                }
                            }
                        } else {
                            /* We're verifying that we get no messages when we've set 100 seconds
                             * as our idle time, which should match nothing */
                            $this->assertEquals([], $cids);
                        }
                    }
                }
            }
        }
    }

    /* Make sure our XAUTOCLAIM handler works */
    public function testXAutoClaim()
    {
        $this->valkey_glide->del('ships');
        $this->valkey_glide->xGroup('CREATE', 'ships', 'combatants', '0-0', true);

        // Test an empty xautoclaim reply
        $res = $this->valkey_glide->xAutoClaim('ships', 'combatants', 'Sisko', 0, '0-0');

        $this->assertTrue(is_array($res) && (count($res) == 2 || count($res) == 3));
        if (count($res) == 2) {
            $this->assertEquals(['0-0', []], $res);
        } else {
            $this->assertEquals(['0-0', [], []], $res);
        }

        $this->valkey_glide->xAdd('ships', '1424-74205', ['name' => 'Defiant']);

        // Consume the ['name' => 'Defiant'] message
        $this->valkey_glide->xReadGroup('combatants', "Jem'Hadar", ['ships' => '>'], 1);

        // The "Jem'Hadar" consumer has the message presently
        $pending = $this->valkey_glide->xPending('ships', 'combatants');
        $this->assertTrue($pending && isset($pending[3][0][0]) && $pending[3][0][0] == "Jem'Hadar");

        // Assume control of the pending message with a different consumer.
        $res = $this->valkey_glide->xAutoClaim('ships', 'combatants', 'Sisko', 0, '0-0');

        $this->assertTrue($res && (count($res) == 2 || count($res) == 3));

        $this->assertTrue(isset($res[1]['1424-74205']['name']) &&
                          $res[1]['1424-74205']['name'] == 'Defiant');

        // Now the 'Sisko' consumer should own the message
        $pending = $this->valkey_glide->xPending('ships', 'combatants');
        $this->assertTrue(isset($pending[3][0][0]) && $pending[3][0][0] == 'Sisko');
    }

    public function testXInfo()
    {
        if (! $this->minVersionCheck('5.0')) {
            $this->markTestSkipped();
        }

        /* Create some streams and groups */
        $stream = 's';
        $groups = ['g1' => 0, 'g2' => 0];
        $this->addStreamsAndGroups([$stream], 1, $groups);


        $info = $this->valkey_glide->xInfo('GROUPS', $stream);

        $this->assertIsArray($info);
        $this->assertEquals(count($info), count($groups));
        foreach ($info as $group) {
            $this->assertArrayKey($group, 'name');
            $this->assertArrayKey($groups, $group['name']);
        }

        $info = $this->valkey_glide->xInfo('STREAM', $stream);
        $this->assertIsArray($info);
        $this->assertArrayKey($info, 'groups', function ($v) use ($groups) {
            return count($groups) == $v;
        });

        foreach (['first-entry', 'last-entry'] as $key) {
            $this->assertArrayKey($info, $key, 'is_array');
        }

        /* Ensure that default/NULL arguments are ignored */
        $info = $this->valkey_glide->xInfo('STREAM', $stream, null);


        $this->assertIsArray($info);

        $info = $this->valkey_glide->xInfo('STREAM', $stream, null, -1);

        $this->assertIsArray($info);

        /* XINFO STREAM FULL [COUNT N] Requires >= 6.0.0 */
        if (! $this->minVersionCheck('6.0')) {
            return;
        }

        /* Add some items to the stream so we can test COUNT */
        for ($i = 0; $i < 5; $i++) {
            $this->valkey_glide->xAdd($stream, '*', ['foo' => 'bar']);
        }

        $info = $this->valkey_glide->xInfo('STREAM', $stream, 'full');
        $this->assertArrayKey($info, 'length', 'is_numeric');

        for ($count = 1; $count < 5; $count++) {
            $info = $this->valkey_glide->xInfo('STREAM', $stream, 'full', $count);

            $n = isset($info['entries']) ? count($info['entries']) : 0;
            $this->assertEquals($n, $count);
        }

        /* Count <= 0 should be ignored */
        foreach ([-1, 0] as $count) {
            $info = $this->valkey_glide->xInfo('STREAM', $stream, 'full', 0);
            $n = isset($info['entries']) ? count($info['entries']) : 0;
            $this->assertEquals($n, $this->valkey_glide->xLen($stream));
        }

        /* Make sure we can't erroneously send non-null args after null ones */
        $this->assertFalse(@$this->valkey_glide->xInfo('FOO', null, 'fail', 25));
        $this->assertFalse(@$this->valkey_glide->xInfo('FOO', null, null, -2));
    }

    /* Regression test for issue-1831 (XINFO STREAM on an empty stream) */
    public function testXInfoEmptyStream()
    {
        /* Configure an empty stream */
        $this->valkey_glide->del('s');
        $this->valkey_glide->xAdd('s', '*', ['foo' => 'bar']);
        $this->valkey_glide->xTrim('s', 0);

        $info = $this->valkey_glide->xInfo('STREAM', 's');

        $this->assertIsArray($info);
        $this->assertEquals(0, $info['length']);
        $this->assertNull($info['first-entry']);
        $this->assertNull($info['last-entry']);
    }


    protected function testRequiresMode(string $mode)
    {
        if (php_sapi_name() != $mode) {
            $this->markTestSkipped("Test requires PHP running in '$mode' mode");
        }
    }

    public function testCopy()
    {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
        }

        $this->valkey_glide->del('{key}dst');
        $this->valkey_glide->set('{key}src', 'foo');
        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->valkey_glide->set('{key}src', 'bar');
        $this->assertFalse($this->valkey_glide->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->assertTrue($this->valkey_glide->copy('{key}src', '{key}dst', ['replace' => true]));
        $this->assertKeyEquals('bar', '{key}dst');
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

    public function testFunction()
    {
        if (version_compare($this->version, '7.0') < 0) {
            $this->markTestSkipped();
        }

        $this->assertTrue($this->valkey_glide->function('flush', 'sync'));
        $this->assertEquals('mylib', $this->valkey_glide->function('load', "#!lua name=mylib\nredis.register_function('myfunc', function(keys, args) return args[1] end)"));
        $this->assertEquals('foo', $this->valkey_glide->fcall('myfunc', [], ['foo']));
        $payload = $this->valkey_glide->function('dump');
        $this->assertEquals('mylib', $this->valkey_glide->function('load', 'replace', "#!lua name=mylib\nredis.register_function{function_name='myfunc', callback=function(keys, args) return args[1] end, flags={'no-writes'}}"));
        $this->assertEquals('foo', $this->valkey_glide->fcall_ro('myfunc', [], ['foo']));
        $this->assertEquals(['running_script' => false, 'engines' => ['LUA' => ['libraries_count' => 1, 'functions_count' => 1]]], $this->valkey_glide->function('stats'));
        $this->assertTrue($this->valkey_glide->function('delete', 'mylib'));
        $this->assertTrue($this->valkey_glide->function('restore', $payload));
        $this->assertEquals([['library_name' => 'mylib', 'engine' => 'LUA', 'functions' => [['name' => 'myfunc', 'description' => false,'flags' => []]]]], $this->valkey_glide->function('list'));
        $this->assertTrue($this->valkey_glide->function('delete', 'mylib'));
    }
}
