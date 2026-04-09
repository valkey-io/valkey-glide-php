<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * Builder for FT.AGGREGATE commands.
 *
 * Pipeline clauses are appended in the order you call them,
 * which matches the server's execution order.
 *
 * Usage:
 *   $client->ftAggregate(
 *       (new FtAggregateBuilder())
 *           ->index('idx')
 *           ->query('*')
 *           ->load(['__key'])
 *           ->groupBy(['@condition'], [
 *               FtReducer::count()->as('total'),
 *           ])
 *           ->sortBy(['@total' => 'DESC'])
 *           ->limit(0, 10)
 *   );
 *
 * @see \ValkeyGlide::ftAggregate()
 * @see https://valkey.io/commands/ft.aggregate/
 */
class FtAggregateBuilder
{
    private ?string $indexName = null;
    private ?string $queryStr = null;

    /**
     * Flat token list built incrementally.
     *
     * FtCreateBuilder and FtSearchBuilder return associative arrays for their
     * options because each option key (LIMIT, SORTBY, etc.) appears at most
     * once. That doesn't work here: an aggregate pipeline can have multiple
     * GROUPBY, SORTBY, APPLY, FILTER, and LIMIT clauses, and order matters.
     * A flat list avoids the duplicate-key problem and preserves clause order.
     *
     * @var array<string>
     */
    private array $tokens = [];

    /** Set the index name. */
    public function index(string $name): self
    {
        $this->indexName = $name;
        return $this;
    }

    /** Set the filter query string. */
    public function query(string $query): self
    {
        $this->queryStr = $query;
        return $this;
    }

    /** Disable stemming. */
    public function verbatim(): self
    {
        $this->tokens[] = 'VERBATIM';
        return $this;
    }

    /** Require proximity terms in order. */
    public function inOrder(): self
    {
        $this->tokens[] = 'INORDER';
        return $this;
    }

    /** Set max distance between proximity terms. */
    public function slop(int $slop): self
    {
        $this->tokens[] = 'SLOP';
        $this->tokens[] = (string) $slop;
        return $this;
    }

    /** Override module timeout in milliseconds. */
    public function timeout(int $ms): self
    {
        $this->tokens[] = 'TIMEOUT';
        $this->tokens[] = (string) $ms;
        return $this;
    }

    /** Set query dialect version. */
    public function dialect(int $dialect): self
    {
        $this->tokens[] = 'DIALECT';
        $this->tokens[] = (string) $dialect;
        return $this;
    }

    /**
     * Set query parameters.
     *
     * @param array $params Associative array of parameter names to values.
     */
    public function params(array $params): self
    {
        $this->tokens[] = 'PARAMS';
        $this->tokens[] = (string) (count($params) * 2);
        foreach ($params as $key => $value) {
            $this->tokens[] = (string) $key;
            $this->tokens[] = (string) $value;
        }
        return $this;
    }

    /**
     * Load fields from the document.
     *
     * Pass ['*'] to load all fields, or a list of field names.
     *
     * @param array $fields Field names (prefixed with @ or not).
     */
    public function load(array $fields): self
    {
        $this->tokens[] = 'LOAD';
        if ($fields === ['*']) {
            $this->tokens[] = '*';
        } else {
            $this->tokens[] = (string) count($fields);
            foreach ($fields as $field) {
                $this->tokens[] = $field;
            }
        }
        return $this;
    }

    /**
     * Add a GROUPBY clause with optional reducers.
     *
     * @param array       $fields   Fields to group by (e.g. ['@condition']).
     * @param FtReducer[] $reducers Reducer definitions built with FtReducer.
     */
    public function groupBy(array $fields, array $reducers = []): self
    {
        $this->tokens[] = 'GROUPBY';
        $this->tokens[] = (string) count($fields);
        foreach ($fields as $field) {
            $this->tokens[] = $field;
        }
        foreach ($reducers as $reducer) {
            $this->tokens[] = 'REDUCE';
            foreach ($reducer->toTokens() as $token) {
                $this->tokens[] = $token;
            }
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
        $this->tokens[] = 'SORTBY';
        $this->tokens[] = (string) (count($fieldOrders) * 2);
        foreach ($fieldOrders as $field => $order) {
            $this->tokens[] = $field;
            $this->tokens[] = strtoupper($order);
        }
        if ($max !== null) {
            $this->tokens[] = 'MAX';
            $this->tokens[] = (string) $max;
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
        $this->tokens[] = 'APPLY';
        $this->tokens[] = $expression;
        $this->tokens[] = 'AS';
        $this->tokens[] = $alias;
        return $this;
    }

    /**
     * Add a FILTER clause.
     *
     * @param string $expression The filter expression.
     */
    public function filter(string $expression): self
    {
        $this->tokens[] = 'FILTER';
        $this->tokens[] = $expression;
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
        $this->tokens[] = 'LIMIT';
        $this->tokens[] = (string) $offset;
        $this->tokens[] = (string) $count;
        return $this;
    }

    /**
     * Convert the builder state into the arguments for ftAggregate().
     *
     * Returns ['index' => string, 'query' => string, 'options' => array|null].
     *
     * @internal Called by the C layer via ftAggregate().
     * @return array
     * @throws \ValkeyGlideException if index or query is not set.
     */
    public function toArray(): array
    {
        if ($this->indexName === null) {
            throw new \ValkeyGlideException('FtAggregateBuilder: index name is required');
        }
        if ($this->queryStr === null) {
            throw new \ValkeyGlideException('FtAggregateBuilder: query is required');
        }

        return [
            'index'   => $this->indexName,
            'query'   => $this->queryStr,
            'options' => empty($this->tokens) ? null : $this->tokens,
        ];
    }
}
