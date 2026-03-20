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

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * Integration tests for FT.* (Valkey Search) commands.
 * Requires a Valkey server with the Search module loaded.
 */
class ValkeyGlideValkeySearchTest extends ValkeyGlideBaseTest
{
    private ?ValkeyGlide $client = null;

    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    protected function getClient(): ValkeyGlide
    {
        if ($this->client === null) {
            $this->client = $this->newInstance();
        }
        return $this->client;
    }

    protected function skipIfModuleNotAvailable(): void
    {
        try {
            $this->getClient()->ftList();
        } catch (\Throwable $e) {
            $msg = $e->getMessage();
            if (
                stripos($msg, 'unknown command') !== false ||
                stripos($msg, 'ERR unknown') !== false ||
                stripos($msg, 'module') !== false
            ) {
                throw new TestSkippedException("Valkey Search module not available.");
            }
        }
    }

    /* ── FT.CREATE ──────────────────────────────────────────────────── */

    public function testFtCreateSimpleHnswVector()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $result = $this->getClient()->ftCreate($idx, [
            ['name' => 'vec', 'type' => 'VECTOR', 'algorithm' => 'HNSW',
             'dim' => 2, 'metric' => 'L2'],
        ]);
        $this->assertTrue($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateJsonFlatVector()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $result = $this->getClient()->ftCreate($idx, [
            ['name' => '$.vec', 'alias' => 'VEC', 'type' => 'VECTOR',
             'algorithm' => 'FLAT', 'dim' => 6, 'metric' => 'L2'],
        ], ['ON' => 'JSON', 'PREFIX' => ['json:']]);
        $this->assertTrue($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateHnswWithExtraParams()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $result = $this->getClient()->ftCreate($idx, [
            ['name' => 'doc_embedding', 'type' => 'VECTOR', 'algorithm' => 'HNSW',
             'dim' => 1536, 'metric' => 'COSINE',
             'initial_cap' => 1000, 'm' => 40,
             'ef_construction' => 250, 'ef_runtime' => 40],
        ], ['ON' => 'HASH', 'PREFIX' => ['docs:']]);
        $this->assertTrue($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateMultipleFields()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $result = $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
            ['name' => 'published_at', 'type' => 'NUMERIC'],
            ['name' => 'category', 'type' => 'TAG'],
        ], ['ON' => 'HASH', 'PREFIX' => ['blog:post:']]);
        $this->assertTrue($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateDuplicateThrows()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ]);
        $threw = false;
        try {
            $this->getClient()->ftCreate($idx, [
                ['name' => 'title', 'type' => 'TEXT'],
            ]);
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.DROPINDEX + FT._LIST ───────────────────────────────────── */

    public function testFtDropIndexAndList()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate($idx, [
            ['name' => 'vec', 'type' => 'VECTOR', 'algorithm' => 'HNSW',
             'dim' => 2, 'metric' => 'L2'],
        ]);
        $before = $this->getClient()->ftList();
        $this->assertTrue(in_array($idx, $before, true));
        $this->getClient()->ftDropIndex($idx);
        $after = $this->getClient()->ftList();
        $this->assertFalse(in_array($idx, $after, true));
    }

    public function testFtDropIndexNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $threw = false;
        try {
            $this->getClient()->ftDropIndex('nonexistent_' . uniqid());
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
    }

    /* ── FT.SEARCH ─────────────────────────────────────────────────── */

    public function testFtSearchBasic()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'vec', 'alias' => 'VEC', 'type' => 'VECTOR',
             'algorithm' => 'HNSW', 'dim' => 2, 'metric' => 'L2'],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);

        $vec0 = str_repeat("\x00", 8);
        $vec1 = "\x00\x00\x00\x00\x00\x00\x80\xBF";
        $this->getClient()->hSet($prefix . '0', 'vec', $vec0);
        $this->getClient()->hSet($prefix . '1', 'vec', $vec1);
        usleep(1500000);

        $result = $this->getClient()->ftSearch(
            $idx,
            '*=>[KNN 2 @VEC $query_vec]',
            [
                'PARAMS' => ['query_vec' => $vec0],
                'RETURN' => ['vec'],
            ]
        );
        $this->assertIsArray($result);

        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtSearchNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $threw = false;
        try {
            $this->getClient()->ftSearch('nonexistent_' . uniqid(), '*');
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
    }

    /* ── FT.INFO ───────────────────────────────────────────────────── */

    public function testFtInfoExistingIndex()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate($idx, [
            ['name' => '$.vec', 'alias' => 'VEC', 'type' => 'VECTOR',
             'algorithm' => 'HNSW', 'dim' => 42, 'metric' => 'COSINE'],
            ['name' => '$.name', 'type' => 'TEXT'],
        ], ['ON' => 'JSON', 'PREFIX' => ['123']]);
        $info = $this->getClient()->ftInfo($idx);
        $this->assertIsArray($info);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtInfoNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $threw = false;
        try {
            $this->getClient()->ftInfo('nonexistent_' . uniqid());
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
    }

    /* ── FT.ALIAS* ─────────────────────────────────────────────────── */

    public function testFtAliasOperations()
    {
        $this->skipIfModuleNotAvailable();
        $alias1 = 'alias1-' . uniqid();
        $alias2 = 'alias2-' . uniqid();
        $idx = uniqid() . '-index';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'vec', 'type' => 'VECTOR', 'algorithm' => 'FLAT',
             'dim' => 2, 'metric' => 'L2'],
        ]);

        $r = $this->getClient()->ftAliasAdd($alias1, $idx);
        $this->assertTrue($r);

        $aliases = $this->getClient()->ftAliasList();
        $this->assertIsArray($aliases);
        $this->assertEquals($idx, $aliases[$alias1] ?? null);

        // Duplicate alias -> error
        $threw = false;
        try {
            $this->getClient()->ftAliasAdd($alias1, $idx);
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);

        $r = $this->getClient()->ftAliasUpdate($alias2, $idx);
        $this->assertTrue($r);

        $aliases = $this->getClient()->ftAliasList();
        $this->assertEquals($idx, $aliases[$alias1] ?? null);
        $this->assertEquals($idx, $aliases[$alias2] ?? null);

        $this->getClient()->ftAliasDel($alias2);
        $this->getClient()->ftAliasDel($alias1);

        // Delete non-existent -> error
        $threw = false;
        try {
            $this->getClient()->ftAliasDel($alias2);
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);

        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.AGGREGATE (stays flat - pipeline ordering) ─────────────── */

    public function testFtAggregateBicycles()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{bicycles' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'model', 'type' => 'TEXT'],
            ['name' => 'price', 'type' => 'NUMERIC'],
            ['name' => 'condition', 'type' => 'TAG', 'separator' => ','],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);

        $conditions = ['new', 'used', 'used', 'used', 'used', 'new', 'new', 'new', 'new', 'refurbished'];
        for ($i = 0; $i < count($conditions); $i++) {
            $this->getClient()->hSet($prefix . $i, ['model' => 'bike' . $i, 'price' => (string)(100 + $i * 10), 'condition' => $conditions[$i]]);
        }
        usleep(1500000);

        $result = $this->getClient()->ftAggregate(
            $idx,
            '*',
            ['LOAD', '1', '__key', 'GROUPBY', '1', '@condition', 'REDUCE', 'COUNT', '0', 'AS', 'bicycles']
        );
        $this->assertIsArray($result);

        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.CREATE index-level options ─────────────────────────────── */

    public function testFtCreateWithScoreLanguageSkipInitialScan()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $result = $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], [
            'ON' => 'HASH',
            'PREFIX' => [$prefix],
            'SCORE' => 1,
            'LANGUAGE' => 'english',
            'SKIPINITIALSCAN' => true,
        ]);
        $this->assertTrue($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateNoStopWords()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], [
            'ON' => 'HASH',
            'PREFIX' => [$prefix],
            'NOSTOPWORDS' => true,
        ]);
        $this->getClient()->hSet($prefix . '1', 'title', 'the quick fox');
        usleep(1500000);
        $result = $this->getClient()->ftSearch($idx, 'the');
        $this->assertIsArray($result);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateMinStemSize()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], [
            'ON' => 'HASH',
            'PREFIX' => [$prefix],
            'MINSTEMSIZE' => 6,
        ]);
        $this->getClient()->hSet($prefix . '1', 'title', 'running');
        $this->getClient()->hSet($prefix . '2', 'title', 'plays');
        usleep(1500000);
        $r = $this->getClient()->ftSearch($idx, 'run');
        $this->assertIsArray($r);
        $r = $this->getClient()->ftSearch($idx, 'play');
        $this->assertIsArray($r);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateNoOffsets()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], [
            'ON' => 'HASH',
            'PREFIX' => [$prefix],
            'NOOFFSETS' => true,
        ]);
        $this->getClient()->hSet($prefix . '1', 'title', 'hello');
        usleep(1500000);
        $r = $this->getClient()->ftSearch($idx, 'hello');
        $this->assertIsArray($r);
        // SLOP requires offsets - should fail
        $threw = false;
        try {
            $this->getClient()->ftSearch($idx, 'hello', ['SLOP' => 1]);
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.CREATE field options ───────────────────────────────────── */

    public function testFtCreateFieldOptionsNoStemSortableWeight()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT', 'nostem' => true, 'weight' => 1, 'sortable' => true],
            ['name' => 'price', 'type' => 'NUMERIC', 'sortable' => true],
            ['name' => 'tag', 'type' => 'TAG', 'separator' => ',', 'sortable' => true],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);
        $this->getClient()->hSet($prefix . '1', ['title' => 'hello', 'price' => '10', 'tag' => 'a,b']);
        usleep(1500000);

        $r = $this->getClient()->ftSearch($idx, '@price:[1 +inf]', [
            'SORTBY' => ['price', 'ASC'],
        ]);
        $this->assertIsArray($r);

        $r = $this->getClient()->ftSearch($idx, 'hello');
        $this->assertIsArray($r);
        $r = $this->getClient()->ftSearch($idx, 'hellos');
        $this->assertIsArray($r);

        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateWithSuffixTrie()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT', 'withsuffixtrie' => true],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);
        $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
        usleep(1500000);
        $r = $this->getClient()->ftSearch($idx, '*orld');
        $this->assertIsArray($r);
        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtCreateNoSuffixTrie()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT', 'nosuffixtrie' => true],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);
        $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
        usleep(1500000);
        $threw = false;
        try {
            $this->getClient()->ftSearch($idx, '*orld');
        } catch (\Throwable $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.SEARCH cluster-mode options ────────────────────────────── */

    public function testFtSearchShardScopeAndConsistency()
    {
        // Just testing that it works with these configurations
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'tag', 'type' => 'TAG'],
            ['name' => 'score', 'type' => 'NUMERIC'],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);

        $this->getClient()->hSet($prefix . '1', 'tag', 'test', 'score', '1');
        $this->getClient()->hSet($prefix . '2', 'tag', 'test', 'score', '2');
        usleep(1500000);

        // SOMESHARDS + INCONSISTENT
        $r = $this->getClient()->ftSearch($idx, '@tag:{test}', [
            'SOMESHARDS' => true,
            'INCONSISTENT' => true,
        ]);
        $this->assertIsArray($r);

        // ALLSHARDS + CONSISTENT
        $r = $this->getClient()->ftSearch($idx, '@tag:{test}', [
            'ALLSHARDS' => true,
            'CONSISTENT' => true,
        ]);
        $this->assertIsArray($r);

        $this->getClient()->ftDropIndex($idx);
    }

    /* ── FT.INFO cluster-mode options ──────────────────────────────── */

    public function testFtInfoWithLocalScopeAndShardFlags()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);
        $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
        usleep(1500000);

        // LOCAL scope
        $info = $this->getClient()->ftInfo($idx, ['scope' => 'LOCAL']);
        $this->assertIsArray($info);

        // LOCAL + ALLSHARDS + CONSISTENT
        $info = $this->getClient()->ftInfo($idx, [
            'scope' => 'LOCAL',
            'ALLSHARDS' => true,
            'CONSISTENT' => true,
        ]);
        $this->assertIsArray($info);

        // LOCAL + SOMESHARDS + INCONSISTENT
        $info = $this->getClient()->ftInfo($idx, [
            'scope' => 'LOCAL',
            'SOMESHARDS' => true,
            'INCONSISTENT' => true,
        ]);
        $this->assertIsArray($info);

        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtInfoPrimaryAndClusterScope()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);
        $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
        usleep(1500000);

        // PRIMARY scope — works if coordinator is enabled, otherwise rejected
        try {
            $info = $this->getClient()->ftInfo($idx, ['scope' => 'PRIMARY']);
            $this->assertIsArray($info);
        } catch (\Throwable $e) {
            // Expected on servers without coordinator
            $this->assertTrue(
                stripos($e->getMessage(), 'PRIMARY') !== false ||
                stripos($e->getMessage(), 'not valid') !== false ||
                stripos($e->getMessage(), 'ERR') !== false
            );
        }

        // CLUSTER scope — works if coordinator is enabled, otherwise rejected
        try {
            $info = $this->getClient()->ftInfo($idx, ['scope' => 'CLUSTER']);
            $this->assertIsArray($info);
        } catch (\Throwable $e) {
            $this->assertTrue(
                stripos($e->getMessage(), 'CLUSTER') !== false ||
                stripos($e->getMessage(), 'not valid') !== false ||
                stripos($e->getMessage(), 'ERR') !== false
            );
        }

        $this->getClient()->ftDropIndex($idx);
    }

    public function testFtSearchReturnWithAliases()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate($idx, [
            ['name' => 'title', 'type' => 'TEXT'],
            ['name' => 'price', 'type' => 'NUMERIC'],
            ['name' => 'category', 'type' => 'TAG'],
        ], ['ON' => 'HASH', 'PREFIX' => [$prefix]]);

        $this->getClient()->hSet($prefix . '1', ['title' => 'Widget', 'price' => '9.99', 'category' => 'tools']);
        $this->getClient()->hSet($prefix . '2', ['title' => 'Gadget', 'price' => '19.99', 'category' => 'electronics']);
        usleep(1500000);

        // Simple RETURN without aliases — returned fields should use original names
        $result = $this->getClient()->ftSearch(
            $idx,
            '@category:{tools|electronics}',
            ['RETURN' => ['title', 'price'], 'SORTBY' => ['price', 'ASC']]
        );
        $this->assertIsArray($result);
        $this->assertEquals(2, $result[0]);
        $this->assertIsArray($result[1]);
        // First result's fields (index 1 in the results sub-array)
        $fields1 = $result[1][1];
        $this->assertContains('title', $fields1);
        $this->assertContains('price', $fields1);

        // RETURN with AS aliases — returned fields should use alias names
        $result = $this->getClient()->ftSearch(
            $idx,
            '@category:{tools|electronics}',
            ['RETURN' => ['title' => 't', 'price' => 'p'], 'SORTBY' => ['price', 'ASC']]
        );
        $this->assertIsArray($result);
        $this->assertEquals(2, $result[0]);
        $fields1 = $result[1][1];
        $this->assertContains('t', $fields1);
        $this->assertContains('p', $fields1);
        // Original names should NOT appear when aliased
        $this->assertFalse(in_array('title', $fields1));
        $this->assertFalse(in_array('price', $fields1));

        // Mixed: 'title' aliased to 't', 'price' unaliased
        $result = $this->getClient()->ftSearch(
            $idx,
            '@category:{tools|electronics}',
            ['RETURN' => ['title' => 't', 'price'], 'SORTBY' => ['price', 'ASC']]
        );
        $this->assertIsArray($result);
        $this->assertEquals(2, $result[0]);
        $fields1 = $result[1][1];
        $this->assertContains('t', $fields1);
        $this->assertContains('price', $fields1);

        $this->getClient()->ftDropIndex($idx);
    }
}
