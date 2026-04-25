<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * Builder for FT.SEARCH commands.
 *
 * Usage:
 *   $client->ftSearch(
 *       (new FtSearchBuilder())
 *           ->index('myindex')
 *           ->query('@title:hello')
 *           ->sortBy('price', 'ASC')
 *           ->limit(0, 10)
 *   );
 *
 * @see \ValkeyGlide::ftSearch()
 * @see https://valkey.io/commands/ft.search/
 */
class FtSearchBuilder
{
    private ?string $indexName = null;
    private ?string $queryStr = null;
    private bool $noContent = false;
    private bool $verbatim = false;
    private bool $inOrder = false;
    private ?int $slop = null;
    private ?array $limit = null;
    private ?array $sortBy = null;
    private bool $withSortKeys = false;
    private ?array $returnFields = null;
    private ?int $timeout = null;
    private ?array $params = null;
    private ?int $dialect = null;
    private bool $allShards = false;
    private bool $someShards = false;
    private bool $consistent = false;
    private bool $inconsistent = false;

    /** Set the index name. */
    public function index(string $name): self
    {
        $this->indexName = $name;
        return $this;
    }

    /** Set the search query string. */
    public function query(string $query): self
    {
        $this->queryStr = $query;
        return $this;
    }

    /** Return keys only, no field data. */
    public function noContent(bool $noContent = true): self
    {
        $this->noContent = $noContent;
        return $this;
    }

    /** Disable stemming in the query. */
    public function verbatim(bool $verbatim = true): self
    {
        $this->verbatim = $verbatim;
        return $this;
    }

    /** Require proximity terms in order. */
    public function inOrder(bool $inOrder = true): self
    {
        $this->inOrder = $inOrder;
        return $this;
    }

    /** Set max distance between proximity terms. */
    public function slop(int $slop): self
    {
        $this->slop = $slop;
        return $this;
    }

    /**
     * Set pagination.
     *
     * @param int $offset Starting offset.
     * @param int $count  Number of results.
     */
    public function limit(int $offset, int $count): self
    {
        $this->limit = [$offset, $count];
        return $this;
    }

    /**
     * Sort results by a field.
     *
     * @param string $field Field name.
     * @param string $order 'ASC' or 'DESC'.
     */
    public function sortBy(string $field, string $order = 'ASC'): self
    {
        $this->sortBy = [$field, strtoupper($order)];
        return $this;
    }

    /** Include sort key in each result. */
    public function withSortKeys(bool $withSortKeys = true): self
    {
        $this->withSortKeys = $withSortKeys;
        return $this;
    }

    /**
     * Set fields to return.
     *
     * Pass a simple array for field names: ['title', 'price']
     * Pass an associative array for aliases: ['title' => 't', 'price' => 'p']
     * Mix is allowed: ['title' => 't', 'price']
     *
     * @param array $fields
     */
    public function returnFields(array $fields): self
    {
        $this->returnFields = $fields;
        return $this;
    }

    /** Override module timeout in milliseconds. */
    public function timeout(int $ms): self
    {
        $this->timeout = $ms;
        return $this;
    }

    /**
     * Set query parameters (key => value pairs).
     *
     * @param array $params Associative array of parameter names to values.
     */
    public function params(array $params): self
    {
        $this->params = $params;
        return $this;
    }

    /** Set query dialect version. */
    public function dialect(int $dialect): self
    {
        $this->dialect = $dialect;
        return $this;
    }

    /** Query all shards (cluster mode). Mutually exclusive with someShards(). */
    public function allShards(bool $allShards = true): self
    {
        $this->allShards = $allShards;
        return $this;
    }

    /** Query subset of shards (cluster mode). Mutually exclusive with allShards(). */
    public function someShards(bool $someShards = true): self
    {
        $this->someShards = $someShards;
        return $this;
    }

    /** Require consistent results (cluster mode). Mutually exclusive with inconsistent(). */
    public function consistent(bool $consistent = true): self
    {
        $this->consistent = $consistent;
        return $this;
    }

    /** Allow inconsistent results (cluster mode). Mutually exclusive with consistent(). */
    public function inconsistent(bool $inconsistent = true): self
    {
        $this->inconsistent = $inconsistent;
        return $this;
    }

    /**
     * Convert the builder state into the arguments for ftSearch().
     *
     * Returns ['index' => string, 'query' => string, 'options' => array|null].
     *
     * @internal Called by the C layer via ftSearch().
     * @return array
     * @throws \ValkeyGlideException if index or query is not set, or if
     *         mutually exclusive options are combined.
     */
    public function toArray(): array
    {
        if ($this->indexName === null) {
            throw new \ValkeyGlideException('FtSearchBuilder: index name is required');
        }
        if ($this->queryStr === null) {
            throw new \ValkeyGlideException('FtSearchBuilder: query is required');
        }
        if ($this->allShards && $this->someShards) {
            throw new \ValkeyGlideException(
                'FtSearchBuilder: ALLSHARDS and SOMESHARDS are mutually exclusive'
            );
        }
        if ($this->consistent && $this->inconsistent) {
            throw new \ValkeyGlideException(
                'FtSearchBuilder: CONSISTENT and INCONSISTENT are mutually exclusive'
            );
        }

        $options = $this->buildOptions();

        return [
            'index'   => $this->indexName,
            'query'   => $this->queryStr,
            'options' => empty($options) ? null : $options,
        ];
    }

    /** @internal */
    private function buildOptions(): array
    {
        $options = [];
        if ($this->noContent) {
            $options['NOCONTENT'] = true;
        }
        if ($this->verbatim) {
            $options['VERBATIM'] = true;
        }
        if ($this->inOrder) {
            $options['INORDER'] = true;
        }
        if ($this->slop !== null) {
            $options['SLOP'] = $this->slop;
        }
        if ($this->limit !== null) {
            $options['LIMIT'] = $this->limit;
        }
        if ($this->sortBy !== null) {
            $options['SORTBY'] = $this->sortBy;
        }
        if ($this->withSortKeys) {
            $options['WITHSORTKEYS'] = true;
        }
        if ($this->returnFields !== null) {
            $options['RETURN'] = $this->returnFields;
        }
        if ($this->timeout !== null) {
            $options['TIMEOUT'] = $this->timeout;
        }
        if ($this->params !== null) {
            $options['PARAMS'] = $this->params;
        }
        if ($this->dialect !== null) {
            $options['DIALECT'] = $this->dialect;
        }
        if ($this->allShards) {
            $options['ALLSHARDS'] = true;
        }
        if ($this->someShards) {
            $options['SOMESHARDS'] = true;
        }
        if ($this->consistent) {
            $options['CONSISTENT'] = true;
        }
        if ($this->inconsistent) {
            $options['INCONSISTENT'] = true;
        }
        return $options;
    }
}
