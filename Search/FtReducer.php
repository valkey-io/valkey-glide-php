<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * Builder for REDUCE clauses inside FT.AGGREGATE GROUPBY.
 *
 * Usage:
 *   FtReducer::count()->as('total')
 *   FtReducer::sum('@price')->as('total_price')
 *   FtReducer::avg('@rating')->as('avg_rating')
 *   FtReducer::min('@price')->as('cheapest')
 *   FtReducer::max('@price')->as('priciest')
 *   FtReducer::countDistinct('@category')->as('num_categories')
 *
 * @see FtAggregateBuilder::groupBy()
 */
class FtReducer
{
    private string $function;
    private array $args;
    private ?string $alias = null;

    private function __construct(string $function, array $args = [])
    {
        $this->function = $function;
        $this->args = $args;
    }

    /** COUNT — count rows in the group. */
    public static function count(): self
    {
        return new self('COUNT');
    }

    /** COUNT_DISTINCT — count distinct values of a property. */
    public static function countDistinct(string $property): self
    {
        return new self('COUNT_DISTINCT', [$property]);
    }

    /** SUM — sum values of a property. */
    public static function sum(string $property): self
    {
        return new self('SUM', [$property]);
    }

    /** MIN — minimum value of a property. */
    public static function min(string $property): self
    {
        return new self('MIN', [$property]);
    }

    /** MAX — maximum value of a property. */
    public static function max(string $property): self
    {
        return new self('MAX', [$property]);
    }

    /** AVG — average value of a property. */
    public static function avg(string $property): self
    {
        return new self('AVG', [$property]);
    }

    /** STDDEV — standard deviation of a property. */
    public static function stddev(string $property): self
    {
        return new self('STDDEV', [$property]);
    }

    /** Set the output alias (AS name). */
    public function as(string $alias): self
    {
        $this->alias = $alias;
        return $this;
    }

    /**
     * Convert to flat token array for the aggregate command.
     *
     * @internal
     * @return array<string>
     */
    public function toTokens(): array
    {
        $tokens = [$this->function, (string) count($this->args)];
        foreach ($this->args as $arg) {
            $tokens[] = $arg;
        }
        if ($this->alias !== null) {
            $tokens[] = 'AS';
            $tokens[] = $this->alias;
        }
        return $tokens;
    }
}
