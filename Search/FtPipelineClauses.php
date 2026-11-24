<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * The aggregate pipeline clause methods (FILTER, LIMIT, GROUPBY, SORTBY,
 * APPLY) shared by FtAggregateBuilder and FtHybridBuilder. FT.HYBRID
 * reuses the FT.AGGREGATE pipeline grammar, so both builders emit these
 * clauses identically; keeping them in one place stops the two from
 * drifting when one gets a fix.
 *
 * The using class decides where the tokens go by implementing
 * appendPipelineTokens().
 */
trait FtPipelineClauses
{
    /**
     * Append raw tokens to the builder's pipeline, in call order.
     *
     * @param string ...$tokens
     */
    abstract protected function appendPipelineTokens(string ...$tokens): void;

    /**
     * Add a GROUPBY clause with optional reducers.
     *
     * @param array       $fields   Fields to group by (e.g. ['@condition']).
     * @param FtReducer[] $reducers Reducer definitions built with FtReducer.
     */
    public function groupBy(array $fields, array $reducers = []): self
    {
        $this->appendPipelineTokens('GROUPBY', (string) count($fields));
        foreach ($fields as $field) {
            $this->appendPipelineTokens((string) $field);
        }
        foreach ($reducers as $reducer) {
            $this->appendPipelineTokens('REDUCE', ...$reducer->toTokens());
        }
        return $this;
    }

    /**
     * Add a SORTBY clause.
     *
     * @param array    $fieldOrders Associative array: ['@field' => 'ASC|DESC', ...].
     * @param int|null $max         Optional MAX limit for partial sorting.
     */
    public function sortBy(array $fieldOrders, ?int $max = null): self
    {
        $this->appendPipelineTokens('SORTBY', (string) (count($fieldOrders) * 2));
        foreach ($fieldOrders as $field => $order) {
            $this->appendPipelineTokens((string) $field, strtoupper($order));
        }
        if ($max !== null) {
            $this->appendPipelineTokens('MAX', (string) $max);
        }
        return $this;
    }

    /**
     * Add an APPLY clause.
     *
     * @param string $expression The expression to compute.
     * @param string $alias      The output property name.
     */
    public function apply(string $expression, string $alias): self
    {
        $this->appendPipelineTokens('APPLY', $expression, 'AS', $alias);
        return $this;
    }

    /**
     * Add a FILTER clause.
     *
     * @param string $expression The filter expression.
     */
    public function filter(string $expression): self
    {
        $this->appendPipelineTokens('FILTER', $expression);
        return $this;
    }

    /**
     * Add a LIMIT clause.
     *
     * @param int $offset Starting offset.
     * @param int $count  Number of results.
     */
    public function limit(int $offset, int $count): self
    {
        $this->appendPipelineTokens('LIMIT', (string) $offset, (string) $count);
        return $this;
    }
}
