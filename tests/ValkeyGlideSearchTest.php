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
foreach (glob(__DIR__ . '/../Search/*.php') as $file) {
    require_once $file;
}

use ValkeyGlide\Search\{FtCreateBuilder, FtTextField, FtNumericField, FtTagField, FtVectorField, FtSearchBuilder, FtAggregateBuilder, FtReducer};

/**
 * Integration tests for FT.* (Valkey Search) commands.
 * Requires a Valkey server with the Search module loaded.
 */
class ValkeyGlideSearchTest extends ValkeyGlideBaseTest
{
    /**
     * Time in microseconds to wait for the search index to sync after
     * writing documents. Valkey Search indexes asynchronously, so a
     * short delay is needed between writes and reads in integration tests.
     */
    private const INDEX_SYNC_DELAY_US = 1500000;

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
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->addField(FtVectorField::hnsw('vec', 2, 'L2'))
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateJsonFlatVector()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('JSON')
                    ->prefix(['json:'])
                    ->addField(FtVectorField::flat('$.vec', 6, 'L2')->alias('VEC'))
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateHnswWithExtraParams()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix(['docs:'])
                    ->addField(
                        FtVectorField::hnsw('doc_embedding', 1536, 'COSINE')
                            ->initialCap(1000)->m(40)->efConstruction(250)->efRuntime(40)
                    )
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateMultipleFields()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix(['blog:post:'])
                    ->addField(new FtTextField('title'))
                    ->addField(new FtNumericField('published_at'))
                    ->addField(new FtTagField('category'))
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateDuplicateThrows()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())->index($idx)->addField(new FtTextField('title'))
        );
        try {
            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftCreate(
                    (new FtCreateBuilder())->index($idx)->addField(new FtTextField('title'))
                );
            }, '/already exists/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateDuplicateFieldThrows()
    {
        $this->skipIfModuleNotAvailable();
        $client = $this->getClient();
        $this->assertThrowsMatch(null, function () use ($client) {
            $client->ftCreate(
                (new FtCreateBuilder())
                    ->index(uniqid('phptest_'))
                    ->on('HASH')
                    ->prefix(['dup:'])
                    ->addField(new FtTextField('name'))
                    ->addField(new FtTextField('name'))
            );
        }, '/Duplicate/');
    }

    /* ── FT.DROPINDEX + FT._LIST ───────────────────────────────────── */

    public function testFtDropIndexAndList()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())->index($idx)->addField(FtVectorField::hnsw('vec', 2, 'L2'))
        );
        try {
            $before = $this->getClient()->ftList();
            $this->assertTrue(in_array($idx, $before, true));
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
        $after = $this->getClient()->ftList();
        $this->assertFalse(in_array($idx, $after, true));
    }

    public function testFtDropIndexNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $client = $this->getClient();
        $nonExistent = 'nonexistent_' . uniqid();
        $this->assertThrowsMatch($nonExistent, function ($idx) use ($client) {
            $client->ftDropIndex($idx);
        }, '/not found/');
    }

    /* ── FT.SEARCH ─────────────────────────────────────────────────── */

    public function testFtSearchBasic()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(FtVectorField::hnsw('vec', 2, 'L2')->alias('VEC'))
        );

        try {
            $vec0 = str_repeat("\x00", 8);
            $vec1 = "\x00\x00\x00\x00\x00\x00\x80\xBF";
            $this->getClient()->hSet($prefix . '0', 'vec', $vec0);
            $this->getClient()->hSet($prefix . '1', 'vec', $vec1);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('*=>[KNN 2 @VEC $query_vec]')
                    ->params(['query_vec' => $vec0])
                    ->returnFields(['vec'])
            );
            $this->assertIsArray($result);
            $this->assertEquals(2, $result[0]);
            $this->assertIsArray($result[1]);
            $this->assertEquals($prefix . '0', $result[1][0]);
            $this->assertEquals($prefix . '1', $result[1][2]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $client = $this->getClient();
        $nonExistent = 'nonexistent_' . uniqid();
        $this->assertThrowsMatch($nonExistent, function ($idx) use ($client) {
            $client->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')
            );
        }, '/not found/');
    }

    public function testFtSearchNoContent()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $key = $prefix . 'doc';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );

        try {
            $this->getClient()->hSet($key, 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);

            // NOCONTENT: only keys returned, no field content
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello')
                    ->noContent()
            );
            $this->assertIsArray($result);
            $this->assertEquals(1, $result[0]);
            // Result should contain the key but no field data
            $this->assertIsArray($result[1]);
            $this->assertEquals($key, $result[1][0]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchDialect()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $key = $prefix . 'doc';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );

        try {
            $this->getClient()->hSet($key, 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);

            // dialect 2 is accepted; assert the document is returned
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello')
                    ->dialect(2)
            );
            $this->assertIsArray($result);
            $this->assertEquals(1, $result[0]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchDialectInvalid()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );

        try {
            // dialect < 2 is not supported; expect an error
            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftSearch(
                    (new FtSearchBuilder())
                        ->index($idx)
                        ->query('hello')
                        ->dialect(1)
                );
            }, '/DIALECT/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchSortBy()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField((new FtNumericField('price'))->sortable())
                ->addField(new FtTextField('name'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', ['price' => '10', 'name' => 'Aardvark']);
            $this->getClient()->hSet($prefix . '2', ['price' => '20', 'name' => 'Mango']);
            $this->getClient()->hSet($prefix . '3', ['price' => '30', 'name' => 'Zebra']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // SORTBY price ASC
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@price:[1 +inf]')
                    ->sortBy('price', 'ASC')
            );
            $this->assertIsArray($result);
            $this->assertEquals(3, $result[0]);
            // First result should have lowest price
            $fields = $result[1][1];
            $priceIdx = array_search('price', $fields);
            $this->assertTrue($priceIdx !== false);
            $this->assertEquals('10', $fields[$priceIdx + 1]);

            // SORTBY price DESC
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@price:[1 +inf]')
                    ->sortBy('price', 'DESC')
            );
            $this->assertIsArray($result);
            $this->assertEquals(3, $result[0]);
            // First result should have highest price
            $fields = $result[1][1];
            $priceIdx = array_search('price', $fields);
            $this->assertTrue($priceIdx !== false);
            $this->assertEquals('30', $fields[$priceIdx + 1]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchWithSortKeys()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField((new FtNumericField('price'))->sortable())
                ->addField(new FtTextField('name'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', ['price' => '10', 'name' => 'Aardvark']);
            $this->getClient()->hSet($prefix . '2', ['price' => '20', 'name' => 'Mango']);
            $this->getClient()->hSet($prefix . '3', ['price' => '30', 'name' => 'Zebra']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // WITHSORTKEYS — each doc value becomes [sortKey, fieldMap]
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@price:[1 +inf]')
                    ->sortBy('price', 'ASC')
                    ->withSortKeys()
            );
            $this->assertIsArray($result);
            $this->assertEquals(3, $result[0]);
            $this->assertIsArray($result[1]);

            // Result[1] is a flat map: [key, [sortkey, [fields...]], key, [sortkey, [fields...]], ...]
            $docs = $result[1];
            $this->assertEquals(6, count($docs)); // 3 docs × 2 (key + value)

            $expectedPrices = ['10', '20', '30'];
            for ($i = 0; $i < 3; $i++) {
                $docKey = $docs[$i * 2];
                $docValue = $docs[$i * 2 + 1];

                // docKey should be a string (the hash key)
                $this->assertIsString($docKey);

                // docValue should be [sortKey, fieldMap]
                $this->assertIsArray($docValue);
                $this->assertEquals(2, count($docValue));

                // Sort key for numeric fields is prefixed with #
                $sortKey = $docValue[0];
                $this->assertIsString($sortKey);
                $this->assertStringContains($expectedPrices[$i], $sortKey);

                // Field map is a flat array: [field, val, field, val, ...]
                $fieldArr = $docValue[1];
                $this->assertIsArray($fieldArr);
                $fieldMap = [];
                for ($j = 0; $j < count($fieldArr); $j += 2) {
                    $fieldMap[$fieldArr[$j]] = $fieldArr[$j + 1];
                }
                $this->assertEquals($expectedPrices[$i], $fieldMap['price']);
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchTextQueryFlags()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
            $this->getClient()->hSet($prefix . '2', 'title', 'hello there');
            $this->getClient()->hSet($prefix . '3', 'title', 'goodbye world');
            $this->getClient()->hSet($prefix . '4', 'title', 'world hello');
            usleep(self::INDEX_SYNC_DELAY_US);

            // VERBATIM — no stemming
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello')
                    ->verbatim()
            );
            $this->assertIsArray($result);
            $this->assertEquals(3, $result[0]); // hello world, hello there, world hello

            // SLOP without INORDER — proximity match in any order
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello world')
                    ->slop(1)
            );
            $this->assertIsArray($result);
            $this->assertEquals(2, $result[0]); // hello world, world hello

            // SLOP with INORDER — proximity match in order only
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello world')
                    ->inOrder()
                    ->slop(1)
            );
            $this->assertIsArray($result);
            $this->assertEquals(1, $result[0]); // hello world only
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.INFO ───────────────────────────────────────────────────── */

    public function testFtInfoExistingIndex()
    {
        $this->skipIfModuleNotAvailable();
        $idx = uniqid('phptest_');
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('JSON')
                ->prefix(['123'])
                ->addField(FtVectorField::hnsw('$.vec', 42, 'COSINE')->alias('VEC'))
                ->addField((new FtTextField('$.name'))->alias('name'))
        );
        try {
            $info = $this->getClient()->ftInfo($idx);
            $this->assertIsArray($info);
            $this->assertStringContains($idx, $info['index_name'] ?? '');
            // Verify the schema fields are present in the info response
            $this->assertTrue(isset($info['attributes']) || isset($info['fields']));
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtInfoNonExistentThrows()
    {
        $this->skipIfModuleNotAvailable();
        $client = $this->getClient();
        $nonExistent = 'nonexistent_' . uniqid();
        $this->assertThrowsMatch($nonExistent, function ($idx) use ($client) {
            $client->ftInfo($idx);
        }, '/not found/');
    }

    /* ── FT.ALIAS* ─────────────────────────────────────────────────── */

    // FT.ALIAS* commands not yet supported by valkey-search — re-enable once available
    public function testFtAliasOperations()
    {
        $this->skipIfModuleNotAvailable();
        $this->markTestSkipped('FT.ALIAS* commands not yet supported by valkey-search');

        $alias1 = 'alias1-' . uniqid();
        $alias2 = 'alias2-' . uniqid();
        $idx = uniqid() . '-index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())->index($idx)->addField(FtVectorField::flat('vec', 2, 'L2'))
        );

        try {
            $r = $this->getClient()->ftAliasAdd($alias1, $idx);
            $this->assertTrue($r);

            $aliases = $this->getClient()->ftAliasList();
            $this->assertIsArray($aliases);
            $this->assertEquals($idx, $aliases[$alias1] ?? null);

            // Duplicate alias -> error
            $client = $this->getClient();
            $this->assertThrowsMatch($alias1, function ($alias) use ($client, $idx) {
                $client->ftAliasAdd($alias, $idx);
            });

            $r = $this->getClient()->ftAliasUpdate($alias2, $idx);
            $this->assertTrue($r);

            $aliases = $this->getClient()->ftAliasList();
            $this->assertEquals($idx, $aliases[$alias1] ?? null);
            $this->assertEquals($idx, $aliases[$alias2] ?? null);

            $this->getClient()->ftAliasDel($alias2);
            $this->getClient()->ftAliasDel($alias1);

            // Delete non-existent -> error
            $this->assertThrowsMatch($alias2, function ($alias) use ($client) {
                $client->ftAliasDel($alias);
            });
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.AGGREGATE (stays flat - pipeline ordering) ─────────────── */

    public function testFtAggregateBicycles()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{bicycles' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('model'))
                ->addField(new FtNumericField('price'))
                ->addField((new FtTagField('condition'))->separator(','))
        );

        try {
            $conditions = ['new', 'used', 'used', 'used', 'used', 'new', 'new', 'new', 'new', 'refurbished'];
            for ($i = 0; $i < count($conditions); $i++) {
                $this->getClient()->hSet($prefix . $i, ['model' => 'bike' . $i, 'price' => (string)(100 + $i * 10), 'condition' => $conditions[$i]]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@condition:{new|used|refurbished}')
                    ->load(['__key', '@condition'])
                    ->groupBy(['@condition'], [
                        FtReducer::count()->as('bicycles'),
                    ])
            );
            $this->assertIsArray($result);
            $this->assertCount(3, $result);

            // Build a condition => count map from the flat result rows
            $groups = [];
            foreach ($result as $row) {
                $this->assertIsArray($row);
                // Each row is a flat array: ["condition", <value>, "bicycles", <count>]
                $condIdx = array_search('condition', $row);
                $bikeIdx = array_search('bicycles', $row);
                $this->assertTrue($condIdx !== false);
                $this->assertTrue($bikeIdx !== false);
                $groups[$row[$condIdx + 1]] = (int) $row[$bikeIdx + 1];
            }

            $this->assertArrayHasKey('new', $groups);
            $this->assertArrayHasKey('used', $groups);
            $this->assertArrayHasKey('refurbished', $groups);
            $this->assertEquals(5, $groups['new']);
            $this->assertEquals(4, $groups['used']);
            $this->assertEquals(1, $groups['refurbished']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtAggregateMovies()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{movies' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
                ->addField(new FtNumericField('release_year'))
                ->addField(new FtNumericField('rating'))
                ->addField(new FtTagField('genre'))
                ->addField(new FtNumericField('votes'))
        );

        try {
            $this->getClient()->hSet($prefix . '11002', [
                'title' => 'Star Wars V', 'release_year' => '1980',
                'genre' => 'Action', 'rating' => '8.7', 'votes' => '1127635',
            ]);
            $this->getClient()->hSet($prefix . '11003', [
                'title' => 'The Godfather', 'release_year' => '1972',
                'genre' => 'Drama', 'rating' => '9.2', 'votes' => '1563839',
            ]);
            $this->getClient()->hSet($prefix . '11004', [
                'title' => 'Heat', 'release_year' => '1995',
                'genre' => 'Thriller', 'rating' => '8.2', 'votes' => '559490',
            ]);
            $this->getClient()->hSet($prefix . '11005', [
                'title' => 'Star Wars VI', 'release_year' => '1983',
                'genre' => 'Action', 'rating' => '8.3', 'votes' => '906260',
            ]);
            usleep(self::INDEX_SYNC_DELAY_US);

            // APPLY ceil(@rating) + GROUPBY genre with COUNT, SUM, AVG + SORTBY
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@genre:{Action|Drama|Thriller}')
                    ->load(['@genre', '@rating', '@votes'])
                    ->apply('ceil(@rating)', 'r_rating')
                    ->groupBy(['@genre'], [
                        FtReducer::count()->as('nb_of_movies'),
                        FtReducer::sum('@votes')->as('nb_of_votes'),
                        FtReducer::avg('@r_rating')->as('avg_rating'),
                    ])
                    ->sortBy(['@avg_rating' => 'DESC', '@nb_of_votes' => 'DESC'])
            );
            $this->assertIsArray($result);
            $this->assertCount(3, $result);

            // Build genre => fields map from the flat result rows
            $genres = [];
            foreach ($result as $row) {
                $this->assertIsArray($row);
                $genreIdx = array_search('genre', $row);
                $this->assertTrue($genreIdx !== false);
                $genre = $row[$genreIdx + 1];
                $fields = [];
                for ($i = 0; $i < count($row); $i += 2) {
                    $fields[$row[$i]] = $row[$i + 1];
                }
                $genres[$genre] = $fields;
            }

            // Verify sorted order: Drama (avg 10), Action (avg 9), Thriller (avg 9)
            $genreKeys = array_keys($genres);
            $this->assertEquals('Drama', $genreKeys[0]);
            $this->assertEquals('Action', $genreKeys[1]);
            $this->assertEquals('Thriller', $genreKeys[2]);

            // Drama: 1 movie, ceil(9.2)=10
            $this->assertEquals('1', $genres['Drama']['nb_of_movies']);
            $this->assertEquals('10', $genres['Drama']['avg_rating']);

            // Action: 2 movies, avg(ceil(8.7), ceil(8.3)) = avg(9, 9) = 9
            $this->assertEquals('2', $genres['Action']['nb_of_movies']);
            $this->assertEquals('9', $genres['Action']['avg_rating']);

            // Thriller: 1 movie, ceil(8.2)=9
            $this->assertEquals('1', $genres['Thriller']['nb_of_movies']);
            $this->assertEquals('9', $genres['Thriller']['avg_rating']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtAggregateQueryFlags()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{aggflags' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtNumericField('score'))
                ->addField(new FtTextField('title'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', ['score' => '10', 'title' => 'hello world']);
            $this->getClient()->hSet($prefix . '2', ['score' => '20', 'title' => 'hello there']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // VERBATIM — disables stemming; both docs match, no LOAD so empty rows
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@score:[1 +inf]')
                    ->verbatim()
            );
            $this->assertIsArray($result);
            $this->assertCount(2, $result);
            foreach ($result as $row) {
                $this->assertIsArray($row);
                $this->assertCount(0, $row);
            }

            // INORDER + SLOP — proximity matching flags accepted
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@score:[1 +inf]')
                    ->inOrder()
                    ->slop(1)
            );
            $this->assertIsArray($result);
            $this->assertCount(2, $result);
            foreach ($result as $row) {
                $this->assertIsArray($row);
                $this->assertCount(0, $row);
            }

            // DIALECT
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@score:[1 +inf]')
                    ->dialect(2)
            );
            $this->assertIsArray($result);
            $this->assertCount(2, $result);
            foreach ($result as $row) {
                $this->assertIsArray($row);
                $this->assertCount(0, $row);
            }

            // LOAD with explicit fields — single matching doc returns field values
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@score:[20 +inf]')
                    ->load(['@score', '@title'])
            );
            $this->assertIsArray($result);
            $this->assertCount(1, $result);
            $row = $result[0];
            $this->assertIsArray($row);
            $this->assertTrue(count($row) > 0);
            // Convert flat row to map
            $fields = [];
            for ($i = 0; $i < count($row); $i += 2) {
                $fields[$row[$i]] = $row[$i + 1];
            }
            $this->assertEquals('hello there', $fields['title']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.CREATE index-level options ─────────────────────────────── */

    public function testFtCreateWithScoreLanguageSkipInitialScan()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->score(1)
                    ->language('english')
                    ->skipInitialScan()
                    ->addField(new FtTextField('title'))
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateNoStopWords()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)->on('HASH')->prefix([$prefix])
                ->noStopWords()
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'the quick fox');
            usleep(self::INDEX_SYNC_DELAY_US);
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('the')
            );
            $this->assertIsArray($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateStopWords()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)->on('HASH')->prefix([$prefix])
                ->stopWords(['fox', 'an'])
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'the quick fox');
            usleep(self::INDEX_SYNC_DELAY_US);

            // "the" and "quick" are not stop words, so they should be searchable
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('the')
            );
            $this->assertIsArray($result);
            $this->assertEquals(1, $result[0]);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('quick')
            );
            $this->assertIsArray($result);
            $this->assertEquals(1, $result[0]);

            // "fox" is a custom stop word, so searching for it should fail
            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftSearch(
                    (new FtSearchBuilder())->index($idx)->query('fox')
                );
            }, '/Invalid.*Query Syntax/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateMinStemSize()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->minStemSize(6)
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'running');
            $this->getClient()->hSet($prefix . '2', 'title', 'plays');
            usleep(self::INDEX_SYNC_DELAY_US);
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('run')
            );
            $this->assertIsArray($r);
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('play')
            );
            $this->assertIsArray($r);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateNoOffsets()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->noOffsets()
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello');
            usleep(self::INDEX_SYNC_DELAY_US);
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('hello')
            );
            $this->assertIsArray($r);
            // SLOP requires offsets - should fail
            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftSearch(
                    (new FtSearchBuilder())->index($idx)->query('hello')->slop(1)
                );
            }, '/does not support offsets/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.CREATE field options ───────────────────────────────────── */

    public function testFtCreateFieldOptionsNoStemSortableWeight()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField((new FtTextField('title'))->noStem()->weight(1)->sortable())
                ->addField((new FtNumericField('price'))->sortable())
                ->addField((new FtTagField('tag'))->separator(',')->sortable())
        );
        try {
            $this->getClient()->hSet($prefix . '1', ['title' => 'hello', 'price' => '10', 'tag' => 'a,b']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('@price:[1 +inf]')->sortBy('price', 'ASC')
            );
            $this->assertIsArray($r);

            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('hello')
            );
            $this->assertIsArray($r);
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('hellos')
            );
            $this->assertIsArray($r);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateWithSuffixTrie()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField((new FtTextField('title'))->withSuffixTrie())
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*orld')
            );
            $this->assertIsArray($r);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtCreateNoSuffixTrie()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)->on('HASH')->prefix([$prefix])
                ->addField((new FtTextField('title'))->noSuffixTrie())
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);
            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftSearch(
                    (new FtSearchBuilder())->index($idx)->query('*orld')
                );
            }, '/suffix/i');
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.SEARCH cluster-mode options ────────────────────────────── */

    public function testFtSearchShardScopeAndConsistency()
    {
        // Just testing that it works with these configurations
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTagField('tag'))
                ->addField(new FtNumericField('score'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', 'tag', 'test', 'score', '1');
            $this->getClient()->hSet($prefix . '2', 'tag', 'test', 'score', '2');
            usleep(self::INDEX_SYNC_DELAY_US);

            // SOMESHARDS + INCONSISTENT
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('@tag:{test}')
                    ->someShards()->inconsistent()
            );
            $this->assertIsArray($r);

            // ALLSHARDS + CONSISTENT
            $r = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('@tag:{test}')
                    ->allShards()->consistent()
            );
            $this->assertIsArray($r);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.INFO cluster-mode options ──────────────────────────────── */

    public function testFtInfoWithLocalScopeAndShardFlags()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);

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
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtInfoPrimaryAndClusterScope()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'idx';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                ->addField(new FtTextField('title'))
        );
        try {
            $this->getClient()->hSet($prefix . '1', 'title', 'hello world');
            usleep(self::INDEX_SYNC_DELAY_US);

            // PRIMARY scope — works if coordinator is enabled, otherwise rejected
            try {
                $info = $this->getClient()->ftInfo($idx, ['scope' => 'PRIMARY']);
                $this->assertIsArray($info);
            } catch (\Throwable $e) {
                // Expected on servers without coordinator
                $this->assertStringContains('PRIMARY option is not valid in this configuration', $e->getMessage());
            }

            // CLUSTER scope — works if coordinator is enabled, otherwise rejected
            try {
                $info = $this->getClient()->ftInfo($idx, ['scope' => 'CLUSTER']);
                $this->assertIsArray($info);
            } catch (\Throwable $e) {
                $this->assertStringContains('CLUSTER option is not valid in this configuration', $e->getMessage());
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    public function testFtSearchReturnWithAliases()
    {
        $this->skipIfModuleNotAvailable();
        $prefix = '{' . uniqid() . '}:';
        $idx = $prefix . 'index';
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
                ->addField(new FtNumericField('price'))
                ->addField(new FtTagField('category'))
        );

        try {
            $this->getClient()->hSet($prefix . '1', ['title' => 'Widget', 'price' => '9.99', 'category' => 'tools']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'Gadget', 'price' => '19.99', 'category' => 'electronics']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // Simple RETURN without aliases — returned fields should use original names
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@category:{tools|electronics}')
                    ->returnFields(['title', 'price'])
                    ->sortBy('price', 'ASC')
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
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@category:{tools|electronics}')
                    ->returnFields(['title' => 't', 'price' => 'p'])
                    ->sortBy('price', 'ASC')
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
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@category:{tools|electronics}')
                    ->returnFields(['title' => 't', 'price'])
                    ->sortBy('price', 'ASC')
            );
            $this->assertIsArray($result);
            $this->assertEquals(2, $result[0]);
            $fields1 = $result[1][1];
            $this->assertContains('t', $fields1);
            $this->assertContains('price', $fields1);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }
}
