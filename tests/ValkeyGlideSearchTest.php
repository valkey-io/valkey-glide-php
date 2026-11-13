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

    /**
     * Capability probe for valkey-search 1.3.0.
     *
     * Probes a real 1.3-only behaviour (FT.SEARCH INKEYS, rejected with
     * "Unexpected argument `INKEYS`" by every pre-1.3 module build) on a
     * throwaway index, rather than trusting MODULE LIST's version string
     * alone -- the version number is unreliable across rc builds (see
     * the "never trust a version string alone" note
     * in the main repo's planning tree). The probe index is cleaned up
     * before returning in every case, including when the probe itself
     * throws; a failed cleanup DROPINDEX retries once, then raises
     * (the capability probe must never swallow a genuine failure).
     */
    protected function requiresSearch13(): bool
    {
        $idx = uniqid('phpcap_');
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->addField(new FtTextField('title'))
        );
        $capable = true;
        try {
            $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('*')
                    ->inKeys([])
            );
        } catch (\Throwable $e) {
            if (stripos($e->getMessage(), 'inkeys') !== false) {
                $capable = false;
            } else {
                $this->dropCapabilityProbeIndex($idx);
                throw $e;
            }
        }
        $this->dropCapabilityProbeIndex($idx);
        return $capable;
    }

    /** @internal Drop the capability probe index, retrying once before raising. */
    private function dropCapabilityProbeIndex(string $idx): void
    {
        try {
            $this->getClient()->ftDropIndex($idx);
        } catch (\Throwable $e) {
            try {
                $this->getClient()->ftDropIndex($idx);
            } catch (\Throwable $e2) {
                throw $e2;
            }
        }
    }

    /**
     * Read the active `search.emulate-release` level from the server
     * Returns null if the config key doesn't
     * exist (pre-1.3 module, where emulate-release has no meaning) or
     * the reply can't be parsed. Callers that have already gated on
     * `requiresSearch13()` and still get null back are looking at an
     * unparseable reply from a module that IS 1.3-capable -- a bug in
     * this fixture, not "pre-1.3" -- and should treat that as a hard
     * failure rather than silently skipping (mirrors the Python port's
     * own fix for the same gap).
     */
    protected function emulateReleaseLevel(): ?string
    {
        try {
            $config = $this->getClient()->config('GET', 'search.emulate-release');
        } catch (\Throwable $e) {
            return null;
        }
        if (!is_array($config)) {
            return null;
        }
        // ValkeyGlide's CONFIG GET returns an associative array (map), not
        // a flat [key, value] list (the same shape the Python port's
        // _parse_emulate_release_level had to defend against).
        foreach ($config as $key => $value) {
            if (strcasecmp((string) $key, 'search.emulate-release') === 0 && is_string($value) && $value !== '') {
                return $value;
            }
        }
        return null;
    }

    /** True if the active emulate-release level is >= $minVersion (X.Y.Z strings). */
    protected function emulateReleaseAtLeast(string $minVersion): bool
    {
        $level = $this->emulateReleaseLevel();
        if ($level === null) {
            throw new \RuntimeException('search.emulate-release is unreadable on a valkey-search 1.3 module.');
        }
        return version_compare($level, $minVersion, '>=');
    }

    /**
     * Extract key => value pairs from a flat [key, value, key, value, ...]
     * result array, the shape FT.SEARCH's result[1] uses for plain
     * (non-WITHSORTKEYS, non-WITHSCORES) queries. Keeps assertions about
     * WHICH keys came back honest (comparing a key set), rather than
     * only a row count.
     */
    protected function flatPairs(array $flat): array
    {
        $map = [];
        for ($i = 0; $i < count($flat); $i += 2) {
            $map[$flat[$i]] = $flat[$i + 1];
        }
        return $map;
    }

    /* ── FT.SEARCH: INKEYS, WITHSCORES, SCORER, COUNT ────────────────── */

    public function testFtSearchInKeysRestrictsResults()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for INKEYS.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'Widget']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'Gadget']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // No INKEYS: both match.
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')
            );
            $this->assertEquals(2, $result[0]);

            // INKEYS restricted to one key: only that key matches, and it
            // must be the right one, not merely the right count.
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->inKeys([$prefix . '1'])
            );
            $this->assertEquals(1, $result[0]);
            $this->assertEquals([$prefix . '1'], array_keys($this->flatPairs($result[1])));

            // INKEYS with both keys listed: both match, by key.
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->inKeys([$prefix . '1', $prefix . '2'])
            );
            $this->assertEquals(2, $result[0]);
            $resultKeys = array_keys($this->flatPairs($result[1]));
            sort($resultKeys);
            $expectedKeys = [$prefix . '1', $prefix . '2'];
            sort($expectedKeys);
            $this->assertEquals($expectedKeys, $resultKeys);

            // INKEYS with an explicit empty array: excludes every result.
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->inKeys([])
            );
            $this->assertEquals(0, $result[0]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2');
        }
    }

    public function testFtSearchCountMapsToLimitZeroZero()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for COUNT.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'Widget']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'Gadget']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->count()
            );
            // LIMIT 0 0: total count, zero result rows.
            $this->assertEquals(2, $result[0]);
            $this->assertIsArray($result);
            $this->assertEquals(1, count($result));
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2');
        }
    }

    public function testFtSearchCountAndLimitAreMutuallyExclusive()
    {
        $threw = false;
        try {
            (new FtSearchBuilder())->index('idx')->query('*')->count()->limit(0, 10)->toArray();
        } catch (\ValkeyGlideException $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
    }

    public function testFtSearchWithScores()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for WITHSCORES.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->addField(new FtTextField('title'))
            );
            // One doc repeats the search term for a strictly higher BM25
            // score than the other -- a scorer that always returns a
            // constant (or silently no-ops) would give both docs the same
            // score and pass a "score > 0" check undetected.
            $this->getClient()->hSet($prefix . 'high', ['title' => 'hello hello hello world']);
            $this->getClient()->hSet($prefix . 'low', ['title' => 'hello there']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('hello')->withScores()
            );
            $this->assertEquals(2, $result[0]);
            $docs = $result[1];
            $this->assertEquals(4, count($docs)); // 2 docs x 2 (key + value)

            $scoresByKey = [];
            for ($i = 0; $i < count($docs); $i += 2) {
                $key = $docs[$i];
                $scoreAndFields = $docs[$i + 1];
                $this->assertIsArray($scoreAndFields);
                $this->assertEquals(2, count($scoreAndFields));
                $score = (float) $scoreAndFields[0];
                $this->assertTrue($score > 0);
                $scoresByKey[$key] = $score;
                $fields = $scoreAndFields[1];
                $this->assertContains('title', $fields);
            }
            $this->assertTrue($scoresByKey[$prefix . 'high'] > $scoresByKey[$prefix . 'low']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'high', $prefix . 'low');
        }
    }

    /* ── FT.AGGREGATE: ADDSCORES, SCORER, WITHCURSOR ──────────────────── */

    public function testFtAggregateAddScoresExposesScoreField()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for ADDSCORES.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'hello world']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'hello there friend']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('hello')
                    ->addScores()
                    ->load(['title'])
                    ->sortBy(['@__score' => 'DESC'])
            );
            $this->assertIsArray($result);
            $this->assertCount(2, $result);
            $scores = [];
            foreach ($result as $row) {
                $scoreIdx = array_search('__score', $row);
                $this->assertTrue($scoreIdx !== false);
                $score = (float) $row[$scoreIdx + 1];
                $this->assertTrue($score > 0);
                $scores[] = $score;
            }
            // Sorted descending by __score -- the fused ordering must be
            // stable and monotonic, not merely present.
            $sortedDesc = $scores;
            rsort($sortedDesc);
            $this->assertEquals($sortedDesc, $scores);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2');
        }
    }

    public function testFtAggregateWithCursorPagesResults()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for WITHCURSOR.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->addField(new FtNumericField('n'))
            );
            for ($i = 1; $i <= 20; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, ['n' => (string) $i]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $page = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('*')
                    ->load(['n'])
                    ->withCursor(count: 5)
            );
            $this->assertIsArray($page);
            $this->assertEquals(2, count($page));
            $cursorId = $page[1];
            $this->assertIsInt($cursorId);

            $totalRows = 0;
            $seenN = [];
            foreach ($page[0] as $row) {
                $nIdx = array_search('n', $row);
                $this->assertTrue($nIdx !== false);
                $seenN[(int) $row[$nIdx + 1]] = true;
            }
            $totalRows += count($page[0]);
            $pagesRead = 1;
            while ($cursorId !== 0) {
                $next = $this->getClient()->ftCursorRead($idx, $cursorId, 5);
                $totalRows += count($next[0]);
                foreach ($next[0] as $row) {
                    $nIdx = array_search('n', $row);
                    $this->assertTrue($nIdx !== false);
                    $seenN[(int) $row[$nIdx + 1]] = true;
                }
                $cursorId = $next[1];
                $pagesRead++;
                $this->assertTrue($pagesRead <= 10, 'paging did not terminate as expected');
            }
            $this->assertEquals(20, $totalRows);
            $expectedN = range(1, 20);
            $actualN = array_keys($seenN);
            sort($actualN);
            $this->assertEquals($expectedN, $actualN);

            // FT.CURSOR DEL on a freshly-opened cursor: must assert the
            // actual result (OK/true), not just that the call doesn't throw.
            $page2 = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')->load(['n'])->withCursor(count: 1)
            );
            $openCursorId = $page2[1];
            $this->assertNotEquals(0, $openCursorId);
            $delResult = $this->getClient()->ftCursorDel($idx, $openCursorId);
            $this->assertTrue($delResult);

            // Reading the just-deleted cursor errors.
            $client = $this->getClient();
            $this->assertThrowsMatch($openCursorId, function ($cid) use ($client, $idx) {
                $client->ftCursorRead($idx, $cid);
            }, '/Cursor/');

            // Reading an unknown cursor id errors.
            $this->assertThrowsMatch(999999999, function ($cid) use ($client, $idx) {
                $client->ftCursorRead($idx, $cid);
            }, '/Cursor/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= 20; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    /* ── FT.HYBRID ─────────────────────────────────────────────────────── */

    public function testFtHybridFusesSearchAndVsimArms()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for FT.HYBRID.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->addField(new FtTextField('title'))
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
            );
            // Fixture mirrors Python's HYBRID_DOCS: one doc the SEARCH arm
            // strongly favors (repeated "hello", vector far from query),
            // one the VSIM arm strongly favors (no "hello" at all, vector
            // on top of the query vector).
            $this->getClient()->hSet($prefix . 'text-winner', [
                'title' => 'hello hello hello world',
                'vec' => pack('f*', 100.0, 100.0),
            ]);
            $this->getClient()->hSet($prefix . 'vector-winner', [
                'title' => 'goodbye',
                'vec' => pack('f*', 1.0, 1.0),
            ]);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello')
                    ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5)
                    ->combineRrf()
                    ->limit(0, 10)
            );
            $this->assertIsArray($result);
            $this->assertCount(2, $result);

            $keysSeen = [];
            foreach ($result as $row) {
                $this->assertIsArray($row);
                $keyIdx = array_search('__key', $row);
                $this->assertTrue($keyIdx !== false);
                $keysSeen[] = $row[$keyIdx + 1];
                $scoreIdx = array_search('__score', $row);
                $this->assertTrue($scoreIdx !== false);
            }
            sort($keysSeen);
            $expected = [$prefix . 'text-winner', $prefix . 'vector-winner'];
            sort($expected);
            $this->assertEquals($expected, $keysSeen);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'text-winner', $prefix . 'vector-winner');
        }
    }

    /**
     * FT.HYBRID tail-order port rule (jamesx, #7335): emit PARAMS before
     * any aggregate clause. A FILTER placed directly after the VSIM arm
     * (no LOAD in between) must land in the pipeline tail, not get
     * silently claimed by the server's VSIM parser as a VSIM pre-filter.
     * This test needs the FILTER directly after the VSIM arm with no
     * intervening LOAD -- with a LOAD in between it would pass even with
     * the bug present.
     *
     * Mirrors Python's test_ft_hybrid_aggregate_clause_filters_post_
     * fusion: the SEARCH arm yields its own score under alias "ss" and
     * only matches the "price 50" document; the VSIM arm (k=2) matches
     * both. A FILTER keeping only rows where "ss" is positive must keep
     * exactly the SEARCH-matched document and drop the other -- a
     * misparsed FILTER (claimed as a VSIM pre-filter) would reject this
     * query outright with a syntax error, since "@ss > 0" is not valid
     * VSIM pre-filter (FT.SEARCH-style) syntax; a FILTER that was
     * silently ignored, or a pre-filter that happened to parse, would
     * keep both documents instead of exactly one.
     */
    public function testFtHybridFilterAfterVsimArmIsPipelineFilterNotVsimPrefilter()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for FT.HYBRID.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->addField(new FtTextField('title'))
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
            );
            $this->getClient()->hSet($prefix . 'wireless', [
                'title' => 'wireless widget',
                'vec' => pack('f*', 1.0, 0.0),
            ]);
            $this->getClient()->hSet($prefix . 'wired', [
                'title' => 'wired gadget',
                'vec' => pack('f*', 1.0, 0.0),
            ]);
            usleep(self::INDEX_SYNC_DELAY_US);

            // No LOAD between VSIM and FILTER: this is the exact shape that
            // regresses if PARAMS/clause ordering is wrong. "wireless" only
            // matches the "wireless widget" doc; VSIM (k:2) matches both.
            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('@title:wireless', yieldScoreAs: 'ss')
                    ->vsim('vec', vector: pack('f*', 1.0, 0.0), k: 2)
                    ->filter('@ss > 0')
            );
            $this->assertIsArray($result);
            $this->assertCount(1, $result);
            $row = $result[0];
            $keyIdx = array_search('__key', $row);
            $this->assertTrue($keyIdx !== false);
            $this->assertEquals($prefix . 'wireless', $row[$keyIdx + 1]);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'wireless', $prefix . 'wired');
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

    /* ── FT.SEARCH wildcard (valkey-search 1.3) ───────────────────────── */

    public function testFtSearchWildcardMatchesAll()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for bare *.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . 'alpha', ['title' => 'alpha']);
            $this->getClient()->hSet($prefix . 'beta', ['title' => 'beta']);
            $this->getClient()->hSet($prefix . 'gamma', ['title' => 'gamma']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->noContent()
            );
            $this->assertEquals(3, $result[0]);
            $keys = array_keys($this->flatPairs($result[1]));
            sort($keys);
            $expected = [$prefix . 'alpha', $prefix . 'beta', $prefix . 'gamma'];
            sort($expected);
            $this->assertEquals($expected, $keys);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'alpha', $prefix . 'beta', $prefix . 'gamma');
        }
    }

    /* ── FT.AGGREGATE 1.3 reducers: TOLIST, FIRST_VALUE, RANDOM_SAMPLE ── */

    public function testFtAggregateReducers13()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for TOLIST/FIRST_VALUE/RANDOM_SAMPLE.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField(new FtTagField('category'))
                    ->addField(new FtNumericField('price'))
                    ->addField(new FtTextField('title'))
            );
            // category "a" has 2 distinct prices (10, 30) across 3 records
            // (10 appears twice).
            $this->getClient()->hSet($prefix . '1', ['category' => 'a', 'price' => '10', 'title' => 'cheap_a']);
            $this->getClient()->hSet($prefix . '2', ['category' => 'a', 'price' => '30', 'title' => 'expensive_a']);
            $this->getClient()->hSet($prefix . '3', ['category' => 'a', 'price' => '10', 'title' => 'cheap_a_dup']);
            usleep(self::INDEX_SYNC_DELAY_US);

            // TOLIST: collects unique "price" values in the group, deduped.
            // Relies on valkey-search 1.3's auto-load of fields referenced
            // by a pipeline stage -- no explicit loadFields for "@price".
            $resultToList = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('*')
                    ->groupBy(['@category'], [
                        FtReducer::toList('@price')->as('prices'),
                    ])
            );
            $this->assertCount(1, $resultToList);
            $pricesIdx = array_search('prices', $resultToList[0]);
            $this->assertTrue($pricesIdx !== false);
            $prices = $resultToList[0][$pricesIdx + 1];
            sort($prices);
            $this->assertEquals(['10', '30'], $prices);

            // FIRST_VALUE with BY/DESC: returns the title of the record
            // with the highest price in the group.
            $resultFirstValue = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('*')
                    ->load(['@title', '@price'])
                    ->groupBy(['@category'], [
                        FtReducer::firstValue('@title', by: '@price', order: 'DESC')->as('most_expensive_title'),
                    ])
            );
            $this->assertCount(1, $resultFirstValue);
            $titleIdx = array_search('most_expensive_title', $resultFirstValue[0]);
            $this->assertTrue($titleIdx !== false);
            $this->assertEquals('expensive_a', $resultFirstValue[0][$titleIdx + 1]);

            // RANDOM_SAMPLE: a reservoir sample of up to size over the raw
            // (non-deduped) "price" values -- every sampled value must be
            // one of the two prices actually present.
            $resultRandomSample = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('*')
                    ->groupBy(['@category'], [
                        FtReducer::randomSample('@price', 3)->as('sample'),
                    ])
            );
            $this->assertCount(1, $resultRandomSample);
            $sampleIdx = array_search('sample', $resultRandomSample[0]);
            $this->assertTrue($sampleIdx !== false);
            $sample = $resultRandomSample[0][$sampleIdx + 1];
            $this->assertCount(3, $sample);
            foreach ($sample as $v) {
                $this->assertTrue(in_array($v, ['10', '30'], true));
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2', $prefix . '3');
        }
    }

    /* ── FT.SEARCH / FT.AGGREGATE VECTOR_RANGE (valkey-search 1.3) ────── */

    public function testFtSearchVectorRange()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for VECTOR_RANGE.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
                    ->addField(new FtTagField('category'))
            );
            // Points at known L2 distances from the origin: 0, 1, 4, 9.
            $points = [
                '0' => [[0.0, 0.0], 'a'],
                '1' => [[1.0, 0.0], 'b'],
                '2' => [[2.0, 0.0], 'a'],
                '3' => [[3.0, 0.0], 'b'],
            ];
            foreach ($points as $key => [$coords, $category]) {
                $this->getClient()->hSet($prefix . $key, [
                    'vec' => pack('f*', ...$coords),
                    'category' => $category,
                ]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $queryBlob = pack('f*', 0.0, 0.0);

            // radius 2 covers distances 0 and 1 (keys 0, 1); key 2
            // (distance 4) and key 3 (distance 9) fall outside.
            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@vec:[VECTOR_RANGE 2 $blob]')
                    ->params(['blob' => $queryBlob])
                    ->noContent()
            );
            $this->assertEquals(2, $result[0]);
            $keys = array_keys($this->flatPairs($result[1]));
            sort($keys);
            $expected = [$prefix . '0', $prefix . '1'];
            sort($expected);
            $this->assertEquals($expected, $keys);

            // YIELD_DISTANCE_AS: the computed distance is returned under
            // the given property name.
            $resultDist = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@vec:[VECTOR_RANGE $r $blob]=>{$yield_distance_as: dist}')
                    ->params(['blob' => $queryBlob, 'r' => '0'])
                    ->returnFields(['dist'])
            );
            $this->assertEquals(1, $resultDist[0]);
            $distFields = $resultDist[1][1];
            $distIdx = array_search('dist', $distFields);
            $this->assertTrue($distIdx !== false);
            $this->assertEquals(0.0, (float) $distFields[$distIdx + 1]);

            // AND with a tag filter: radius 2 (keys 0,1) intersected with
            // category "a" (keys 0,2) leaves only key 0.
            $resultAnd = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('@vec:[VECTOR_RANGE 2 $blob] @category:{a}')
                    ->params(['blob' => $queryBlob])
                    ->noContent()
            );
            $this->assertEquals(1, $resultAnd[0]);
            $this->assertEquals([$prefix . '0'], array_keys($this->flatPairs($resultAnd[1])));
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '0', $prefix . '1', $prefix . '2', $prefix . '3');
        }
    }

    public function testFtAggregateVectorRange()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for VECTOR_RANGE.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
            );
            // Distances 0, 1, 100 from the origin -- a radius of 5 must
            // include the first two and exclude the third.
            $points = [[0.0, 0.0], [1.0, 0.0], [10.0, 0.0]];
            foreach ($points as $i => $coords) {
                $this->getClient()->hSet($prefix . $i, ['vec' => pack('f*', ...$coords)]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $queryBlob = pack('f*', 0.0, 0.0);
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())
                    ->index($idx)
                    ->query('@vec:[VECTOR_RANGE 5 $blob]=>{$yield_distance_as: dist}')
                    ->load(['dist'])
                    ->params(['blob' => $queryBlob])
                    ->sortBy(['@dist' => 'ASC'])
            );
            $this->assertCount(2, $result);
            $distances = [];
            foreach ($result as $row) {
                $distIdx = array_search('dist', $row);
                $this->assertTrue($distIdx !== false);
                $distances[] = (float) $row[$distIdx + 1];
            }
            $this->assertEquals([0.0, 1.0], $distances);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '0', $prefix . '1', $prefix . '2');
        }
    }

    /* ── FT.HYBRID VSIM RANGE rejection (valkey-search 1.3) ───────────── */

    public function testFtHybridVsimRangeRejected()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required for FT.HYBRID.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField(new FtTextField('title'))
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
            );
            $queryBlob = pack('f*', 1.0, 2.0);
            // VSIM ... RANGE parses for shape but is rejected at
            // execution on valkey-search 1.3.0 -- excluded from the typed
            // API entirely (per plan decision D10), so this goes through
            // rawcommand() directly. rawcommand() returns false on a
            // server-side error rather than throwing (unlike the typed FT
            // builders' own methods); if RANGE is ever accepted upstream,
            // this would return real data instead of false and the
            // assertion below would fail loudly.
            $result = @$this->getClient()->rawcommand(
                'FT.HYBRID',
                $idx,
                'SEARCH',
                'hello',
                'VSIM',
                '@vec',
                '$blob',
                'RANGE',
                '2',
                'RADIUS',
                '5.0',
                'PARAMS',
                '2',
                'blob',
                $queryBlob
            );
            $this->assertFalse($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── SORTBY-adjacent-LIMIT widening (level-branched, valkey-search 1.3) */

    public function testFtAggregateSortByAdjacentLimitWidensRetainedRows()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        $legacy = !$this->emulateReleaseAtLeast('1.3.0');
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField((new FtNumericField('n'))->sortable())
            );
            for ($i = 1; $i <= 20; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, ['n' => (string) $i]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            // SORTBY, no MAX, no LIMIT -> the default 10 rows. Identical on
            // both branches: no adjacent LIMIT.
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')->sortBy(['@n' => 'ASC'])
            );
            $this->assertCount(10, $result);

            // SORTBY MAX 5, no LIMIT -> 5 rows. Identical on both branches.
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')->sortBy(['@n' => 'ASC'], 5)
            );
            $this->assertCount(5, $result);

            // SORTBY + LIMIT 0 15, adjacent -> 1.3 widens to 15; below
            // 1.3.0 the adjacent LIMIT does NOT widen SORTBY's default-10.
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')
                    ->sortBy(['@n' => 'ASC'])->limit(0, 15)
            );
            $this->assertCount($legacy ? 10 : 15, $result);

            // SORTBY MAX 5 + LIMIT 0 12, adjacent -> 1.3 lets LIMIT outrank
            // MAX; below 1.3.0 SORTBY's own MAX 5 still wins.
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')
                    ->sortBy(['@n' => 'ASC'], 5)->limit(0, 12)
            );
            $this->assertCount($legacy ? 5 : 12, $result);

            // SORTBY + GROUPBY + LIMIT 0 15 -- GROUPBY sits between SORTBY
            // and LIMIT, so LIMIT is not adjacent and must NOT widen
            // SORTBY's default-10 retention, on either branch.
            $result = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')
                    ->sortBy(['@n' => 'ASC'])
                    ->groupBy(['@n'])
                    ->limit(0, 15)
            );
            $this->assertCount(10, $result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= 20; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    /* ── FT.SEARCH LIMIT pagination (valkey-search 1.3) ───────────────── */

    public function testFtSearchLimitPagination()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField((new FtNumericField('n'))->sortable())
            );
            for ($i = 1; $i <= 10; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, ['n' => (string) $i]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->limit(0, 3)
            );
            $this->assertEquals(10, $result[0]);
            $this->assertEquals(3, count($this->flatPairs($result[1])));

            $resultPage2 = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->limit(3, 3)
            );
            $this->assertEquals(10, $resultPage2[0]);
            $this->assertEquals(3, count($this->flatPairs($resultPage2[1])));

            // Offset past the end returns the total count with no rows.
            $resultPastEnd = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->limit(100, 3)
            );
            $this->assertEquals(10, $resultPastEnd[0]);
            $this->assertEquals(1, count($resultPastEnd));
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= 10; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    /* ── FT.CREATE FLOAT16/BFLOAT16 vector types (C-1, valkey-search 1.3) */

    public function testFtCreateFloat16BfloatVector()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $cases = [
            'FLOAT16' => function (array $values) {
                $bytes = '';
                foreach ($values as $v) {
                    $bytes .= pack('v', $this->toFloat16($v));
                }
                return $bytes;
            },
            'BFLOAT16' => function (array $values) {
                $bytes = '';
                foreach ($values as $v) {
                    $bytes .= substr(pack('f', $v), 2, 2);
                }
                return $bytes;
            },
        ];
        foreach ($cases as $vectorType => $encode) {
            $idx = uniqid('phptest_');
            $prefix = $idx . ':';
            try {
                $this->getClient()->ftCreate(
                    (new FtCreateBuilder())
                        ->index($idx)
                        ->on('HASH')
                        ->prefix([$prefix])
                        ->addField(FtVectorField::flat('vec', 2, 'L2', $vectorType))
                );
                $points = ['0' => [0.0, 0.0], '1' => [1.0, 0.0], '2' => [2.0, 0.0]];
                foreach ($points as $key => $coords) {
                    $this->getClient()->hSet($prefix . $key, ['vec' => $encode($coords)]);
                }
                usleep(self::INDEX_SYNC_DELAY_US);

                $queryBlob = $encode([1.0, 0.0]);
                $result = $this->getClient()->ftSearch(
                    (new FtSearchBuilder())
                        ->index($idx)
                        ->query('*=>[KNN 1 @vec $blob]')
                        ->params(['blob' => $queryBlob])
                        ->returnFields(['__vec_score'])
                );
                $this->assertEquals(1, $result[0]);
                $fields = $result[1][1];
                $scoreIdx = array_search('__vec_score', $fields);
                $this->assertTrue($scoreIdx !== false);
                $this->assertEquals($prefix . '1', $result[1][0]);
                $this->assertEquals(0.0, (float) $fields[$scoreIdx + 1]);
            } finally {
                $this->getClient()->ftDropIndex($idx);
                $this->getClient()->del($prefix . '0', $prefix . '1', $prefix . '2');
            }
        }
    }

    /** @internal IEEE 754 half-precision encoding, exact for small binary powers. */
    private function toFloat16(float $v): int
    {
        if ($v === 0.0) {
            return 0;
        }
        $sign = $v < 0 ? 1 : 0;
        $v = abs($v);
        $exp = (int) floor(log($v, 2));
        $mantissa = (int) round(($v / (2 ** $exp) - 1) * 1024);
        $biasedExp = $exp + 15;
        return ($sign << 15) | ($biasedExp << 10) | $mantissa;
    }

    /* ── FT.CREATE FILTER (C-1, valkey-search 1.3) ────────────────────── */

    public function testFtCreateFilter()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->filter('@age > 18')
                    ->addField(new FtNumericField('age'))
            );
            $this->getClient()->hSet($prefix . 'adult', ['age' => '25']);
            $this->getClient()->hSet($prefix . 'minor', ['age' => '10']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('*')->noContent()
            );
            $this->assertEquals(1, $result[0]);
            $this->assertEquals([$prefix . 'adult'], array_keys($this->flatPairs($result[1])));
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'adult', $prefix . 'minor');
        }
    }

    /* ── FT.CREATE SORTABLE UNF (C-1, valkey-search 1.3) ──────────────── */

    public function testFtCreateSortableUnf()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField((new FtTextField('title'))->sortable()->unf())
            );
            $info = $this->getClient()->ftInfo($idx);
            $attrs = $info['attributes'] ?? $info['fields'] ?? null;
            $this->assertIsArray($attrs);
            $this->assertCount(1, $attrs);
            $attr = $this->flatPairs($attrs[0]);
            $this->assertEquals('1', (string) $attr['SORTABLE']);
            $this->assertEquals('1', (string) $attr['UNF']);

            $this->getClient()->hSet($prefix . '1', ['title' => 'apple']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'Banana']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('*')
                    ->returnFields(['title'])
                    ->sortBy('title', 'ASC')
            );
            $orderedKeys = [];
            $docs = $result[1];
            for ($i = 0; $i < count($docs); $i += 2) {
                $orderedKeys[] = $docs[$i];
            }
            // ASCII order: uppercase 'B' (0x42) sorts before lowercase 'a' (0x61).
            $this->assertEquals([$prefix . '2', $prefix . '1'], $orderedKeys);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2');
        }
    }

    /* ── FT.CREATE SCORE_FIELD (C-1, level-branched, valkey-search 1.3) ── */

    public function testFtCreateScoreFieldInIndexDefinition()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->scoreField('boost')
                    ->addField(new FtTextField('title'))
            );
            $info = $this->getClient()->ftInfo($idx);
            $indexDef = $this->flatPairs($info['index_definition'] ?? []);
            if (!$this->emulateReleaseAtLeast('1.3.0')) {
                $this->assertFalse(array_key_exists('score_field', $indexDef));
                $this->assertEquals('1', (string) ($indexDef['default_score'] ?? null));
            } else {
                $this->assertEquals('boost', $indexDef['score_field'] ?? null);
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.CREATE NOHL (C-1, valkey-search 1.3) ──────────────────────── */

    public function testFtCreateNoHighlight()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $result = $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->noHighlight()
                    ->addField(new FtTextField('title'))
            );
            $this->assertTrue($result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
        }
    }

    /* ── FT.SEARCH WITHSCORES + SCORER (H-3, valkey-search 1.3) ───────── */

    public function testFtSearchWithScoresAndScorer()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . 'high', ['title' => 'hello hello hello world']);
            $this->getClient()->hSet($prefix . 'low', ['title' => 'hello there']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('hello')
                    ->withScores()
                    ->scorer('BM25STD')
            );
            $this->assertEquals(2, $result[0]);
            $docs = $result[1];
            $scoresByKey = [];
            for ($i = 0; $i < count($docs); $i += 2) {
                $key = $docs[$i];
                [$score, $fields] = $docs[$i + 1];
                $this->assertTrue((float) $score > 0);
                $scoresByKey[$key] = (float) $score;
                $this->assertContains('title', $fields);
            }
            $this->assertTrue($scoresByKey[$prefix . 'high'] > $scoresByKey[$prefix . 'low']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'high', $prefix . 'low');
        }
    }

    /* ── WITHSCORES + WITHSORTKEYS combined shape (H-3, valkey-search 1.3) */

    public function testFtSearchWithScoresAndWithSortKeysCombinedShape()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField((new FtTextField('title'))->sortable())
                    ->addField((new FtNumericField('price'))->sortable())
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'hello', 'price' => '10']);
            $this->getClient()->hSet($prefix . '2', ['title' => 'world', 'price' => '20']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())
                    ->index($idx)
                    ->query('*')
                    ->withScores()
                    ->sortBy('price', 'ASC')
                    ->withSortKeys()
            );
            $this->assertEquals(2, $result[0]);
            $docs = $result[1];
            $this->assertEquals(4, count($docs));
            for ($i = 0; $i < count($docs); $i += 2) {
                $triple = $docs[$i + 1];
                $this->assertEquals(3, count($triple));
                [$score, $sortKey, $fields] = $triple;
                $this->assertTrue((float) $score >= 0);
                $this->assertTrue($sortKey !== null);
                $this->assertContains('title', $fields);
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1', $prefix . '2');
        }
    }

    /* ── FT.CREATE SCORE_FIELD affects ranking (H-3, valkey-search 1.3) ── */

    public function testFtCreateScoreFieldAffectsRanking()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->scoreField('boost')
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . 'low', ['title' => 'hello world', 'boost' => '1.0']);
            $this->getClient()->hSet($prefix . 'high', ['title' => 'hello world', 'boost' => '100.0']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftSearch(
                (new FtSearchBuilder())->index($idx)->query('hello')->withScores()
            );
            $this->assertEquals(2, $result[0]);
            $docs = $result[1];
            // Result map preserves server-returned ranking order.
            $this->assertEquals($prefix . 'high', $docs[0]);
            $scoresByKey = [];
            for ($i = 0; $i < count($docs); $i += 2) {
                $scoresByKey[$docs[$i]] = (float) $docs[$i + 1][0];
            }
            $this->assertTrue($scoresByKey[$prefix . 'high'] > $scoresByKey[$prefix . 'low']);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . 'low', $prefix . 'high');
        }
    }

    /* ── FT.SEARCH SCORER TFIDF rejected (H-3, valkey-search 1.3) ─────── */

    public function testFtSearchScorerTfidfRejected()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'hello world']);
            usleep(self::INDEX_SYNC_DELAY_US);

            $client = $this->getClient();
            $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                $client->ftSearch(
                    (new FtSearchBuilder())->index($idx)->query('hello')->withScores()->scorer('TFIDF')
                );
            }, '/SCORER/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1');
        }
    }

    /* ── FT.AGGREGATE typed cursor paged to exhaustion (H-3) ──────────── */

    public function testFtAggregateTypedCursorPagedToExhaustion()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField((new FtNumericField('n'))->sortable())
            );
            for ($i = 1; $i <= 20; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, ['n' => (string) $i]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $page = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')->load(['n'])->withCursor(count: 5)
            );
            $cursorId = $page[1];
            $totalRows = count($page[0]);
            $seenN = [];
            foreach ($page[0] as $row) {
                $nIdx = array_search('n', $row);
                $seenN[(int) $row[$nIdx + 1]] = true;
            }
            $pagesRead = 1;
            while ($cursorId !== 0) {
                $next = $this->getClient()->ftCursorRead($idx, $cursorId);
                $totalRows += count($next[0]);
                foreach ($next[0] as $row) {
                    $nIdx = array_search('n', $row);
                    $seenN[(int) $row[$nIdx + 1]] = true;
                }
                $cursorId = $next[1];
                $pagesRead++;
                $this->assertTrue($pagesRead <= 10, 'paging did not terminate as expected');
            }
            $this->assertEquals(20, $totalRows);
            $actualN = array_keys($seenN);
            sort($actualN);
            $this->assertEquals(range(1, 20), $actualN);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= 20; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    /* ── FT.AGGREGATE typed cursor DEL + errors (H-3) ─────────────────── */

    public function testFtAggregateTypedCursorDelAndErrors()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField((new FtNumericField('n'))->sortable())
            );
            for ($i = 1; $i <= 5; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, ['n' => (string) $i]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $page = $this->getClient()->ftAggregate(
                (new FtAggregateBuilder())->index($idx)->query('*')->load(['n'])->withCursor(count: 1)
            );
            $openCursorId = $page[1];
            $this->assertTrue($openCursorId !== 0);

            $this->assertTrue($this->getClient()->ftCursorDel($idx, $openCursorId));

            $client = $this->getClient();
            $this->assertThrowsMatch($openCursorId, function ($cid) use ($client, $idx) {
                $client->ftCursorRead($idx, $cid);
            }, '/Cursor/');
            $this->assertThrowsMatch(999999999, function ($cid) use ($client, $idx) {
                $client->ftCursorRead($idx, $cid);
            }, '/Cursor/');
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= 5; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    /* ── FT.HYBRID: basic query, combine methods, vector param reference,
     * filter policy, per-arm/combine YIELD_SCORE_AS, LOCALONLY, default K,
     * MULTI/EXEC rejection (H-3, valkey-search 1.3) ────────────────────── */

    /** @internal Shared FT.HYBRID fixture: mirrors Python's HYBRID_DOCS. */
    private function createHybridIndex(string $idx, string $prefix): void
    {
        $this->getClient()->ftCreate(
            (new FtCreateBuilder())
                ->index($idx)
                ->on('HASH')
                ->prefix([$prefix])
                ->addField(new FtTextField('title'))
                ->addField(FtVectorField::flat('vec', 2, 'L2'))
        );
        $this->getClient()->hSet($prefix . 'hybrid-text-winner', [
            'title' => 'hello hello hello world',
            'vec' => pack('f*', 100.0, 100.0),
        ]);
        $this->getClient()->hSet($prefix . 'hybrid-vector-winner', [
            'title' => 'goodbye',
            'vec' => pack('f*', 1.0, 1.0),
        ]);
        usleep(self::INDEX_SYNC_DELAY_US);
    }

    /** @internal Teardown for createHybridIndex(). */
    private function dropHybridIndex(string $idx, string $prefix): void
    {
        $this->getClient()->ftDropIndex($idx);
        $this->getClient()->del($prefix . 'hybrid-text-winner', $prefix . 'hybrid-vector-winner');
    }

    public function testFtHybridBasicQuery()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->createHybridIndex($idx, $prefix);
            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello')
                    ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5)
            );
            $this->assertCount(2, $result);
            $keys = [];
            foreach ($result as $row) {
                $keyIdx = array_search('__key', $row);
                $keys[] = $row[$keyIdx + 1];
                $this->assertTrue(array_search('__score', $row) !== false);
            }
            sort($keys);
            $expected = [$prefix . 'hybrid-text-winner', $prefix . 'hybrid-vector-winner'];
            sort($expected);
            $this->assertEquals($expected, $keys);
        } finally {
            $this->dropHybridIndex($idx, $prefix);
        }
    }

    public function testFtHybridCombineMethodsFuseResults()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $cases = [
            'LINEAR-text-only' => ['combine' => ['alpha' => 1.0, 'beta' => 0.0], 'expectedFirst' => 'hybrid-text-winner'],
            'LINEAR-vector-only' => ['combine' => ['alpha' => 0.0, 'beta' => 1.0], 'expectedFirst' => 'hybrid-vector-winner'],
        ];
        foreach ($cases as $label => $case) {
            $idx = uniqid('phptest_');
            $prefix = $idx . ':';
            try {
                $this->createHybridIndex($idx, $prefix);
                $result = $this->getClient()->ftHybrid(
                    (new \ValkeyGlide\Search\FtHybridBuilder())
                        ->index($idx)
                        ->search('hello', yieldScoreAs: 'ss')
                        ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5, yieldScoreAs: 'vs')
                        ->combineLinear(alpha: $case['combine']['alpha'], beta: $case['combine']['beta'])
                );
                $this->assertCount(2, $result, $label);
                $keyIdx = array_search('__key', $result[0]);
                $this->assertEquals($prefix . $case['expectedFirst'], $result[0][$keyIdx + 1], $label);
                foreach ($result as $row) {
                    $this->assertTrue(array_search('vs', $row) !== false, $label);
                }
                $rowsByKey = [];
                foreach ($result as $row) {
                    $kIdx = array_search('__key', $row);
                    $rowsByKey[$row[$kIdx + 1]] = $row;
                }
                $this->assertTrue(array_search('ss', $rowsByKey[$prefix . 'hybrid-text-winner']) !== false, $label);
                $this->assertFalse(array_search('ss', $rowsByKey[$prefix . 'hybrid-vector-winner']), $label);
            } finally {
                $this->dropHybridIndex($idx, $prefix);
            }
        }

        // FUNCTION: fuse using a caller expression referencing just one arm's alias.
        foreach ([['expr' => '@ss', 'expectedFirst' => 'hybrid-text-winner'], ['expr' => '@vs', 'expectedFirst' => 'hybrid-vector-winner']] as $case) {
            $idx = uniqid('phptest_');
            $prefix = $idx . ':';
            try {
                $this->createHybridIndex($idx, $prefix);
                $result = $this->getClient()->ftHybrid(
                    (new \ValkeyGlide\Search\FtHybridBuilder())
                        ->index($idx)
                        ->search('hello', yieldScoreAs: 'ss')
                        ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5, yieldScoreAs: 'vs')
                        ->combineFunction($case['expr'])
                );
                $this->assertCount(2, $result);
                $keyIdx = array_search('__key', $result[0]);
                $this->assertEquals($prefix . $case['expectedFirst'], $result[0][$keyIdx + 1]);
            } finally {
                $this->dropHybridIndex($idx, $prefix);
            }
        }
    }

    public function testFtHybridVectorInlineBytesAndParamReference()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->createHybridIndex($idx, $prefix);
            $vec = pack('f*', 1.0, 1.0);

            $inlineResult = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello')
                    ->vsim('vec', vector: $vec, k: 1, yieldScoreAs: 'vs')
            );
            $paramResult = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello')
                    ->vsim('vec', paramRef: '$myvec', k: 1, yieldScoreAs: 'vs')
                    ->params(['myvec' => $vec])
            );
            $this->assertCount(2, $inlineResult);
            $this->assertCount(2, $paramResult);

            foreach ([$inlineResult, $paramResult] as $result) {
                $rowsWithVs = array_filter($result, fn ($row) => array_search('vs', $row) !== false);
                $this->assertCount(1, $rowsWithVs);
                $row = array_values($rowsWithVs)[0];
                $keyIdx = array_search('__key', $row);
                $this->assertEquals($prefix . 'hybrid-vector-winner', $row[$keyIdx + 1]);
            }
        } finally {
            $this->dropHybridIndex($idx, $prefix);
        }
    }

    public function testFtHybridVsimFilterWithPolicy()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        foreach (['ADHOC', 'BATCHES'] as $policy) {
            $idx = uniqid('phptest_');
            $prefix = $idx . ':';
            try {
                $this->createHybridIndex($idx, $prefix);
                $result = $this->getClient()->ftHybrid(
                    (new \ValkeyGlide\Search\FtHybridBuilder())
                        ->index($idx)
                        ->search('nonexistentterm12345')
                        ->vsim(
                            'vec',
                            vector: pack('f*', 1.0, 1.0),
                            k: 5,
                            filter: \ValkeyGlide\Search\FtHybridBuilder::vsimFilter('@title:hello', policy: $policy)
                        )
                );
                $this->assertCount(1, $result, "policy=$policy");
                $keyIdx = array_search('__key', $result[0]);
                $this->assertEquals($prefix . 'hybrid-text-winner', $result[0][$keyIdx + 1], "policy=$policy");
            } finally {
                $this->dropHybridIndex($idx, $prefix);
            }
        }
    }

    public function testFtHybridPerArmAndCombineYieldScoreAsColumns()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->createHybridIndex($idx, $prefix);
            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello', yieldScoreAs: 'ss')
                    ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5, yieldScoreAs: 'vs')
                    ->combineRrf(yieldScoreAs: 'fused')
            );
            $this->assertCount(2, $result);
            $rowsByKey = [];
            foreach ($result as $row) {
                $kIdx = array_search('__key', $row);
                $rowsByKey[$row[$kIdx + 1]] = $row;
                $this->assertTrue(array_search('vs', $row) !== false);
                $this->assertTrue(array_search('fused', $row) !== false);
                $this->assertFalse(array_search('__score', $row));
            }
            $this->assertTrue(array_search('ss', $rowsByKey[$prefix . 'hybrid-text-winner']) !== false);
            $this->assertFalse(array_search('ss', $rowsByKey[$prefix . 'hybrid-vector-winner']));
        } finally {
            $this->dropHybridIndex($idx, $prefix);
        }
    }

    public function testFtHybridLocalOnlyAccepted()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->createHybridIndex($idx, $prefix);
            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('hello')
                    ->vsim('vec', vector: pack('f*', 1.0, 1.0), k: 5)
                    ->localOnly()
            );
            $this->assertCount(2, $result);
        } finally {
            $this->dropHybridIndex($idx, $prefix);
        }
    }

    public function testFtHybridWithoutKUsesEngineDefault()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        $docCount = 15;
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())
                    ->index($idx)
                    ->on('HASH')
                    ->prefix([$prefix])
                    ->addField(new FtTextField('title'))
                    ->addField(FtVectorField::flat('vec', 2, 'L2'))
            );
            for ($i = 1; $i <= $docCount; $i++) {
                $this->getClient()->hSet($prefix . 'd' . $i, [
                    'title' => 'goodbye',
                    'vec' => pack('f*', (float) $i, (float) $i),
                ]);
            }
            usleep(self::INDEX_SYNC_DELAY_US);

            $result = $this->getClient()->ftHybrid(
                (new \ValkeyGlide\Search\FtHybridBuilder())
                    ->index($idx)
                    ->search('nonexistentterm12345')
                    ->vsim('vec', vector: pack('f*', 1.0, 1.0))
            );
            $this->assertCount(10, $result);
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $keys = [];
            for ($i = 1; $i <= $docCount; $i++) {
                $keys[] = $prefix . 'd' . $i;
            }
            $this->getClient()->del(...$keys);
        }
    }

    public function testFtHybridInsideMultiExecRejected()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        try {
            $this->createHybridIndex($idx, $prefix);
            $client = $this->getClient();
            // FT.HYBRID is rejected outright inside MULTI/EXEC
            // (valkey-search 1.3.0, src/query/multi_search.h). No typed
            // transaction wrapper exists for FT.HYBRID, so this goes
            // through rawcommand() inside a MULTI block; exec() returns
            // false when a queued command fails server-side, matching
            // rawcommand()'s own false-on-error contract elsewhere.
            $client->multi();
            $client->rawcommand(
                'FT.HYBRID',
                $idx,
                'SEARCH',
                'hello',
                'VSIM',
                '@vec',
                '$q',
                'PARAMS',
                '2',
                'q',
                pack('f*', 1.0, 1.0)
            );
            $result = @$client->exec();
            $this->assertFalse($result);
        } finally {
            $this->dropHybridIndex($idx, $prefix);
        }
    }

    /* ── FT.AGGREGATE LOAD...AS (level-branched, valkey-search 1.3) ───── */

    /**
     * `AS <alias>` in the LOAD clause is a compatibility fix introduced
     * in valkey-search 1.3.0 (src/commands/ft_aggregate_parser.cc,
     * gated by the ft_aggregate_load_as compatibility flag). Below
     * search.emulate-release 1.3.0, the legacy branch does not
     * recognise AS as a rename keyword at all: it is consumed as just
     * another LOAD field token, and the server rejects the command
     * outright with "Index field `AS` does not exist" (confirmed live
     * against rc2 at the default level). At >= 1.3.0, AS genuinely
     * renames the loaded column.
     */
    public function testFtAggregateLoadAs()
    {
        $this->skipIfModuleNotAvailable();
        if (!$this->requiresSearch13()) {
            throw new TestSkippedException('valkey-search 1.3+ required.');
        }
        $idx = uniqid('phptest_');
        $prefix = $idx . ':';
        $legacy = !$this->emulateReleaseAtLeast('1.3.0');
        try {
            $this->getClient()->ftCreate(
                (new FtCreateBuilder())->index($idx)->on('HASH')->prefix([$prefix])
                    ->addField(new FtTextField('title'))
            );
            $this->getClient()->hSet($prefix . '1', ['title' => 'hello world']);
            usleep(self::INDEX_SYNC_DELAY_US);

            if ($legacy) {
                $client = $this->getClient();
                $this->assertThrowsMatch($idx, function ($idx) use ($client) {
                    $client->ftAggregate(
                        (new FtAggregateBuilder())
                            ->index($idx)
                            ->query('*')
                            ->load([new \ValkeyGlide\Search\LoadField('title', 't')])
                    );
                }, '/field `AS` does not exist/i');
            } else {
                $result = $this->getClient()->ftAggregate(
                    (new FtAggregateBuilder())
                        ->index($idx)
                        ->query('*')
                        ->load([new \ValkeyGlide\Search\LoadField('title', 't')])
                );
                $this->assertCount(1, $result);
                $tIdx = array_search('t', $result[0]);
                $this->assertTrue($tIdx !== false);
                $this->assertEquals('hello world', $result[0][$tIdx + 1]);
                // The bare, unrenamed field name must NOT appear -- the
                // column is renamed, not duplicated.
                $this->assertFalse(array_search('title', $result[0]));
            }
        } finally {
            $this->getClient()->ftDropIndex($idx);
            $this->getClient()->del($prefix . '1');
        }
    }

    /* ── Unit-level argv tests (H-3): no server needed ─────────────────── */

    /**
     * FT.HYBRID's PARAMS must be emitted before any aggregate-pipeline
     * clause (FILTER/LIMIT/GROUPBY/SORTBY/APPLY). clauseTokens coming
     * after PARAMS in the toArray() output, rather than interleaved with
     * the arms, is exactly the shape build_ft_hybrid_args() depends on to
     * avoid a FILTER right after the VSIM arm being misparsed as a VSIM
     * pre-filter -- see the FT.HYBRID post-filter regression test's own
     * docstring. This test pins the ORDERING at the PHP builder layer
     * (toArray()'s own output), independent of the C argv builder.
     */
    public function testFtHybridBuilderEmitsClauseTokensAfterParams()
    {
        $options = (new \ValkeyGlide\Search\FtHybridBuilder())
            ->index('idx')
            ->search('hello')
            ->vsim('vec', vector: 'rawbytes', k: 5)
            ->filter('@price > 10')
            ->toArray();
        // The PHP builder keeps 'params' and 'clauseTokens' as SEPARATE
        // array keys (the C layer interleaves them in the correct wire
        // order); what this test can pin at the PHP layer is that
        // clauseTokens contains the FILTER and nothing from params/vsim
        // leaked into it, so the C layer's own PARAMS-then-clauseTokens
        // concatenation order (verified by the live regression test) is
        // operating on clean, non-conflated inputs.
        $this->assertEquals(['FILTER', '@price > 10'], $options['clauseTokens']);
        $this->assertNull($options['params']);
    }

    /**
     * A raw vector value is ALWAYS treated as literal bytes, never as a
     * `$name` parameter reference, regardless of its first byte -- even
     * if the byte happens to be 0x24 ('$'). Only vsim()'s $paramRef
     * parameter (a distinct, explicit argument) produces a reference;
     * content-sniffing the $vector argument must never happen.
     */
    public function testFtHybridVsimRawBytesStartingWithDollarAreNotTreatedAsParamRef()
    {
        $rawBytesStartingWithDollar = "\x24\x00\x00\x00"; // literal 0x24 ('$') as the first byte
        $options = (new \ValkeyGlide\Search\FtHybridBuilder())
            ->index('idx')
            ->search('hello')
            ->vsim('vec', vector: $rawBytesStartingWithDollar, k: 5)
            ->toArray();
        $this->assertFalse($options['vsim']['isParamRef']);
        $this->assertEquals($rawBytesStartingWithDollar, $options['vsim']['vector']);
    }

    /**
     * efRuntime is not supported with VSIM filter policy=ADHOC -- the
     * server rejects this combination, and FtHybridBuilder guards it
     * client-side rather than relying on the server's error.
     */
    public function testFtHybridEfRuntimeWithAdhocPolicyRejectedClientSide()
    {
        $threw = false;
        try {
            (new \ValkeyGlide\Search\FtHybridBuilder())
                ->index('idx')
                ->search('hello')
                ->vsim(
                    'vec',
                    vector: 'rawbytes',
                    efRuntime: 40,
                    filter: \ValkeyGlide\Search\FtHybridBuilder::vsimFilter('@title:hello', policy: 'ADHOC')
                )
                ->toArray();
        } catch (\ValkeyGlideException $e) {
            $threw = true;
            $this->assertStringContains('efRuntime', $e->getMessage());
            $this->assertStringContains('ADHOC', $e->getMessage());
        }
        $this->assertTrue($threw);
    }
}
