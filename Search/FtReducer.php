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

    /**
     * TOLIST — collect all values for `$property` across the records in
     * a group into a single array, removing duplicates and nil values.
     * Values are returned in first-encountered order.
     */
    public static function toList(string $property): self
    {
        return new self('TOLIST', [$property]);
    }

    /**
     * FIRST_VALUE — return the value of `$property` from a single record
     * in the group. Without `$by`, the first record encountered wins.
     * With `$by`, the record whose `$by` property is smallest (`'ASC'`,
     * the server's own default) or largest (`'DESC'`) wins; ties keep
     * the first-encountered record. `$order` requires `$by` to be set;
     * omitting it relies on the server's own ASC default rather than
     * emitting a redundant explicit `ASC` token.
     *
     * @throws \ValkeyGlideException if `$order` is given without `$by`.
     */
    public static function firstValue(string $property, ?string $by = null, ?string $order = null): self
    {
        if ($order !== null && $by === null) {
            throw new \ValkeyGlideException('FtReducer::firstValue: order requires by to be set');
        }
        $args = [$property];
        if ($by !== null) {
            $args[] = 'BY';
            $args[] = $by;
            if ($order !== null) {
                $args[] = strtoupper($order);
            }
        }
        return new self('FIRST_VALUE', $args);
    }

    /**
     * RANDOM_SAMPLE — return a uniform random sample of up to `$size`
     * values of `$property` from the records in a group, using
     * reservoir sampling. `$size` must be a non-negative integer no
     * greater than 1000; the server rejects values outside this range.
     */
    public static function randomSample(string $property, int $size): self
    {
        return new self('RANDOM_SAMPLE', [$property, (string) $size]);
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
