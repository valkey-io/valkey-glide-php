<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/TestSuite.php";
require_once __DIR__ . "/../vendor/autoload.php";

use ValkeyGlide\Search\FtAggregateBuilder;
use ValkeyGlide\Search\FtHybridBuilder;
use ValkeyGlide\Search\FtNumericField;
use ValkeyGlide\Search\FtReducer;
use ValkeyGlide\Search\FtTagField;
use ValkeyGlide\Search\FtTextField;

/**
 * Client-side validation of the FT.* builders (valkey-search 1.3.0 options).
 *
 * None of these tests talks to a server, so they run in every CI job and
 * locally without a search module. The server-backed behaviour is covered
 * by ValkeyGlideSearchTest and ValkeyGlideClusterSearchTest, which skip on
 * a module older than 1.3.0.
 */
class FtBuilderValidationTest extends TestSuite
{
    private function hybrid(): FtHybridBuilder
    {
        return (new FtHybridBuilder())->index('idx')->search('hello');
    }

    public function testVsimRequiresExactlyOneOfVectorAndParamRef(): void
    {
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->vsim('vec'), '/exactly one of/');
        $this->assertThrowsMatch(
            null,
            fn() => $this->hybrid()->vsim('vec', vector: 'abc', paramRef: 'q'),
            '/exactly one of/'
        );
        $viaVector = $this->hybrid()->vsim('vec', vector: 'abc')->toArray();
        $viaParam = $this->hybrid()->vsim('vec', paramRef: 'q')->toArray();
        $this->assertEquals(false, $viaVector['vsim']['isParamRef']);
        $this->assertEquals(true, $viaParam['vsim']['isParamRef']);
    }

    public function testVsimRejectsNonFiniteShardKRatio(): void
    {
        $this->assertThrowsMatch(
            null,
            fn() => $this->hybrid()->vsim('vec', vector: 'abc', shardKRatio: NAN),
            '/shardKRatio must be a finite/'
        );
        $this->assertThrowsMatch(
            null,
            fn() => $this->hybrid()->vsim('vec', vector: 'abc', shardKRatio: INF),
            '/shardKRatio must be a finite/'
        );
    }

    public function testCombineLinearRequiresAlphaAndBetweenTogether(): void
    {
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->combineLinear(alpha: 0.5), '/both be set/');
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->combineLinear(beta: 0.5), '/both be set/');
        $this->assertThrowsMatch(
            null,
            fn() => $this->hybrid()->combineLinear(alpha: NAN, beta: 0.5),
            '/alpha must be a finite/'
        );
        $this->assertThrowsMatch(
            null,
            fn() => $this->hybrid()->combineLinear(alpha: 0.5, beta: INF),
            '/beta must be a finite/'
        );
        $this->assertTrue($this->hybrid()->combineLinear() instanceof FtHybridBuilder);
        $this->assertTrue($this->hybrid()->combineLinear(alpha: 0.3, beta: 0.7) instanceof FtHybridBuilder);
    }

    public function testCombineRrfRejectsNonFiniteAndNegativeConstant(): void
    {
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->combineRrf(constant: NAN), '/finite/');
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->combineRrf(constant: INF), '/finite/');
        $this->assertThrowsMatch(null, fn() => $this->hybrid()->combineRrf(constant: -1.0), '/must not be negative/');
    }

    public function testVsimFilterBatchSizeRequiresBatchesPolicy(): void
    {
        $this->assertThrowsMatch(
            null,
            fn() => FtHybridBuilder::vsimFilter('@a:{x}', batchSize: 10),
            '/batchSize requires policy=BATCHES/'
        );
        $this->assertThrowsMatch(
            null,
            fn() => FtHybridBuilder::vsimFilter('@a:{x}', policy: 'ADHOC', batchSize: 10),
            '/batchSize requires policy=BATCHES/'
        );
        $filter = FtHybridBuilder::vsimFilter('@a:{x}', policy: 'BATCHES', batchSize: 10);
        $this->assertEquals('BATCHES', $filter['policy']);
        $this->assertEquals(10, $filter['batchSize']);
    }

    public function testEfRuntimeIsRejectedWithAdhocFilterPolicy(): void
    {
        $adhoc = FtHybridBuilder::vsimFilter('@a:{x}', policy: 'ADHOC');
        $builder = $this->hybrid()->vsim('vec', vector: 'abc', efRuntime: 50, filter: $adhoc);
        $this->assertThrowsMatch($builder, fn($b) => $b->toArray(), '/efRuntime is not supported/');

        $batches = FtHybridBuilder::vsimFilter('@a:{x}', policy: 'BATCHES');
        $ok = $this->hybrid()->vsim('vec', vector: 'abc', efRuntime: 50, filter: $batches)->toArray();
        $this->assertEquals(50, $ok['vsim']['efRuntime']);
    }

    public function testHybridToArrayRequiresIndexSearchAndVsim(): void
    {
        $this->assertThrowsMatch(
            null,
            fn() => (new FtHybridBuilder())->search('x')->vsim('v', vector: 'a')->toArray(),
            '/index name is required/'
        );
        $this->assertThrowsMatch(
            null,
            fn() => (new FtHybridBuilder())->index('i')->vsim('v', vector: 'a')->toArray(),
            '/search\(\) is required/'
        );
        $this->assertThrowsMatch(
            null,
            fn() => (new FtHybridBuilder())->index('i')->search('x')->toArray(),
            '/vsim\(\) is required/'
        );
    }

    public function testUnfRequiresSortable(): void
    {
        $this->assertThrowsMatch(null, fn() => (new FtTextField('t'))->unf()->toArray(), '/unf\(\) requires sortable/');
        $this->assertThrowsMatch(null, fn() => (new FtTagField('t'))->unf()->toArray(), '/unf\(\) requires sortable/');
        $this->assertThrowsMatch(null, fn() => (new FtNumericField('n'))->unf()->toArray(), '/unf\(\) requires sortable/');
        $this->assertIsArray((new FtTextField('t'))->sortable()->unf()->toArray());
    }

    public function testFirstValueOrderRequiresBy(): void
    {
        $this->assertThrowsMatch(
            null,
            fn() => FtReducer::firstValue('@x', order: 'DESC'),
            '/order requires by/'
        );
        $this->assertTrue(FtReducer::firstValue('@x', by: '@y', order: 'DESC') instanceof FtReducer);
        $this->assertTrue(FtReducer::firstValue('@x') instanceof FtReducer);
    }

    /**
     * FtAggregateBuilder and FtHybridBuilder share their pipeline clause
     * methods (FtPipelineClauses); both must emit the same tokens in the
     * same order.
     */
    public function testAggregateAndHybridEmitIdenticalPipelineTokens(): void
    {
        $apply = static function ($b) {
            return $b
                ->groupBy(['@a', '@b'], [FtReducer::count()->as('n'), FtReducer::sum('@x')])
                ->sortBy(['@n' => 'desc', '@a' => 'asc'], 5)
                ->apply('@n * 2', 'double')
                ->filter('@n > 1')
                ->limit(10, 20);
        };

        $aggregate = $apply((new FtAggregateBuilder())->index('idx')->query('*'))->toArray();
        $hybrid = $apply($this->hybrid()->vsim('vec', vector: 'abc'))->toArray();

        $this->assertEquals($aggregate['options'], $hybrid['clauseTokens']);
        $this->assertEquals(
            [
                'GROUPBY', '2', '@a', '@b',
                'REDUCE', 'COUNT', '0', 'AS', 'n',
                'REDUCE', 'SUM', '1', '@x',
                'SORTBY', '4', '@n', 'DESC', '@a', 'ASC', 'MAX', '5',
                'APPLY', '@n * 2', 'AS', 'double',
                'FILTER', '@n > 1',
                'LIMIT', '10', '20',
            ],
            $hybrid['clauseTokens']
        );
    }
}
