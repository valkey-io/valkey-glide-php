<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * Builder for FT.HYBRID commands (valkey-search 1.3.0).
 *
 * FT.HYBRID fuses a text/tag/numeric SEARCH arm with a vector VSIM arm
 * into one ranked result. Both arms are mandatory; COMBINE, LOAD and the
 * aggregate-style pipeline clauses (FILTER, LIMIT, GROUPBY, SORTBY,
 * APPLY) are optional and behave the same as in FT.AGGREGATE.
 *
 * Usage:
 *   $client->ftHybrid(
 *       (new FtHybridBuilder())
 *           ->index('idx')
 *           ->search('@title:wireless')
 *           ->vsim('vec', vector: $queryVectorBytes, k: 10)
 *           ->combineRrf(constant: 60.0)
 *           ->limit(0, 10)
 *   );
 *
 * The VSIM arm's query vector can be given two ways, mirroring other
 * ports' `vector_data` convention:
 *
 * - vsim()'s `$vector` parameter: raw vector bytes. The builder
 *   generates an internal `$`-prefixed PARAMS entry under the hood.
 * - vsim()'s `$paramRef` parameter: an explicit `$name` string
 *   referencing an entry the caller has placed in params() directly.
 *
 * Which one applies is decided ENTIRELY by which named parameter the
 * caller passes -- never by inspecting the string's content. PHP has no
 * separate "binary" string type (`IS_STRING` covers both human-readable
 * text and opaque binary data), so a packed vector's bytes can
 * legitimately start with 0x24 ('$') by pure coincidence; a "does this
 * string start with $" content check would misclassify that vector as a
 * parameter reference and silently corrupt the query. `$vector` is
 * therefore always raw bytes, written verbatim into an internal PARAMS
 * entry regardless of its first byte, and `$paramRef` is always a
 * caller-supplied `$name` reference -- the two parameters are mutually
 * exclusive and vsim() rejects the call if neither or both are given.
 *
 * @see \ValkeyGlide::ftHybrid()
 * @see https://valkey.io/commands/ft.hybrid/
 */
class FtHybridBuilder
{
    private ?string $indexName = null;

    /* SEARCH arm */
    private ?string $searchQuery = null;
    private ?string $searchScorer = null;
    private ?string $searchYieldScoreAs = null;

    /* VSIM arm */
    private ?string $vsimField = null;
    private $vsimVector = null;
    private bool $vsimVectorIsParamRef = false;
    private ?int $vsimK = null;
    private ?int $vsimEfRuntime = null;
    private ?float $vsimShardKRatio = null;
    private ?string $vsimYieldScoreAs = null;
    private ?array $vsimFilter = null;

    /* COMBINE */
    private ?array $combine = null;

    /* Aggregate-suffix tail */
    private bool $loadAll = false;
    private ?array $loadFields = null;
    private array $clauseTokens = [];
    private ?array $withCursor = null;
    private bool $localOnly = false;
    private ?int $timeout = null;
    private ?array $params = null;
    private ?int $dialect = null;

    /** Set the index name. */
    public function index(string $name): self
    {
        $this->indexName = $name;
        return $this;
    }

    /**
     * Set the SEARCH arm: an ordinary (non-vector) FT.SEARCH-style
     * text/tag/numeric query. A vector query (e.g. a `=>[KNN ...]`
     * clause) is rejected by the server here; use vsim() for the
     * vector half.
     *
     * @param string|null $scorer       Scoring function for this arm
     *                                   (e.g. 'BM25STD').
     * @param string|null $yieldScoreAs Expose this arm's score under
     *                                   this column name.
     */
    public function search(string $query, ?string $scorer = null, ?string $yieldScoreAs = null): self
    {
        $this->searchQuery = $query;
        $this->searchScorer = $scorer;
        $this->searchYieldScoreAs = $yieldScoreAs;
        return $this;
    }

    /**
     * Set the VSIM arm: a vector similarity search against a vector
     * field.
     *
     * Param reference vs. raw vector bytes is decided by WHICH parameter
     * you pass, never by inspecting the content of `$vector` -- a raw
     * vector's packed binary representation is arbitrary and can
     * legitimately start with the byte 0x24 ('$') by coincidence, so a
     * "does it start with $" content check would misclassify that vector
     * as a parameter reference. Pass `$vector` to supply raw bytes
     * directly (an internal PARAMS entry is generated automatically);
     * pass `$paramRef` instead to reference a `$name` entry the caller
     * has already placed in params(). Exactly one of the two must be
     * given.
     *
     * @param string       $field       Vector field identifier, without
     *                                   the leading `@`.
     * @param string|null  $vector      Raw vector bytes. Always treated
     *                                   as literal data, never as a
     *                                   parameter name, regardless of
     *                                   its first byte.
     * @param string|null  $paramRef    A `$name` string (including the
     *                                   leading `$`) naming an entry
     *                                   already present in params().
     *                                   Mutually exclusive with
     *                                   `$vector`.
     * @param int|null     $k           Nearest-neighbour count.
     *                                   Omitting the whole KNN block (k,
     *                                   efRuntime and shardKRatio all
     *                                   null) defaults to 10 server-side.
     * @param int|null     $efRuntime   HNSW search-time effort. Only
     *                                   valid with the KNN block written.
     * @param float|null   $shardKRatio Per-shard fanout tuning.
     * @param string|null  $yieldScoreAs Expose this arm's score (the
     *                                   vector distance) under this
     *                                   column name.
     * @param array|null   $filter      A pre-filter built with
     *                                   vsimFilter().
     *
     * @throws \ValkeyGlideException if neither or both of `$vector` and
     *         `$paramRef` are given.
     */
    public function vsim(
        string $field,
        ?string $vector = null,
        ?string $paramRef = null,
        ?int $k = null,
        ?int $efRuntime = null,
        ?float $shardKRatio = null,
        ?string $yieldScoreAs = null,
        ?array $filter = null
    ): self {
        if (($vector === null) === ($paramRef === null)) {
            throw new \ValkeyGlideException(
                'FtHybridBuilder::vsim: exactly one of $vector or $paramRef must be given'
            );
        }
        if ($shardKRatio !== null && !is_finite($shardKRatio)) {
            throw new \ValkeyGlideException('FtHybridBuilder::vsim: shardKRatio must be a finite number');
        }
        $this->vsimField = $field;
        if ($paramRef !== null) {
            $this->vsimVector = $paramRef;
            $this->vsimVectorIsParamRef = true;
        } else {
            $this->vsimVector = $vector;
            $this->vsimVectorIsParamRef = false;
        }
        $this->vsimK = $k;
        $this->vsimEfRuntime = $efRuntime;
        $this->vsimShardKRatio = $shardKRatio;
        $this->vsimYieldScoreAs = $yieldScoreAs;
        $this->vsimFilter = $filter;
        return $this;
    }

    /**
     * Build a VSIM pre-filter, for the `$filter` argument of vsim().
     *
     * @param string      $expression Filter expression, in the same
     *                                 query language FT.SEARCH uses.
     * @param string|null $policy     'ADHOC' or 'BATCHES'. Required for
     *                                 the server to recognise POLICY at
     *                                 all -- the uncounted FILTER
     *                                 extension (no policy/batchSize)
     *                                 can't carry one.
     * @param int|null    $batchSize  Requires $policy = 'BATCHES'.
     */
    public static function vsimFilter(string $expression, ?string $policy = null, ?int $batchSize = null): array
    {
        if ($batchSize !== null && $policy !== 'BATCHES') {
            throw new \ValkeyGlideException(
                'FtHybridBuilder::vsimFilter: batchSize requires policy=BATCHES'
            );
        }
        return ['expression' => $expression, 'policy' => $policy, 'batchSize' => $batchSize];
    }

    /**
     * Fuse the two arms using Reciprocal Rank Fusion.
     *
     * @param float|null  $constant     RRF constant. Must be non-negative.
     * @param int|null    $window       How many of each arm's results
     *                                   take part in fusion.
     * @param string|null $yieldScoreAs Expose the fused score under this
     *                                   column name instead of `__score`.
     */
    public function combineRrf(?float $constant = null, ?int $window = null, ?string $yieldScoreAs = null): self
    {
        if ($constant !== null && !is_finite($constant)) {
            throw new \ValkeyGlideException('FtHybridBuilder::combineRrf: constant must be a finite number');
        }
        if ($constant !== null && $constant < 0) {
            throw new \ValkeyGlideException('FtHybridBuilder::combineRrf: constant must not be negative');
        }
        $this->combine = [
            'method' => 'RRF',
            'constant' => $constant,
            'window' => $window,
            'yieldScoreAs' => $yieldScoreAs,
        ];
        return $this;
    }

    /**
     * Fuse the two arms linearly: `alpha * search_score + beta *
     * vsim_score`. Leaving both `$alpha` and `$beta` unset defaults them
     * to 0.3/0.7 server-side; setting exactly one without the other is
     * rejected by the server.
     */
    public function combineLinear(
        ?float $alpha = null,
        ?float $beta = null,
        ?int $window = null,
        ?string $yieldScoreAs = null
    ): self {
        if (($alpha === null) !== ($beta === null)) {
            throw new \ValkeyGlideException(
                'FtHybridBuilder::combineLinear: alpha and beta must both be set, or both left unset'
            );
        }
        if ($alpha !== null && !is_finite($alpha)) {
            throw new \ValkeyGlideException('FtHybridBuilder::combineLinear: alpha must be a finite number');
        }
        if ($beta !== null && !is_finite($beta)) {
            throw new \ValkeyGlideException('FtHybridBuilder::combineLinear: beta must be a finite number');
        }
        $this->combine = [
            'method' => 'LINEAR',
            'alpha' => $alpha,
            'beta' => $beta,
            'window' => $window,
            'yieldScoreAs' => $yieldScoreAs,
        ];
        return $this;
    }

    /**
     * Fuse the two arms using a caller-supplied expression (a
     * valkey-search extension not present in RediSearch's FT.HYBRID).
     *
     * @param string      $expr         Fusion expression, e.g.
     *                                   `'@__search_score + @__vector_score'`.
     * @param int|null    $window       Defaults to the widest window
     *                                   allowed if left unset.
     * @param string|null $yieldScoreAs Expose the fused score under this
     *                                   column name instead of `__score`.
     */
    public function combineFunction(string $expr, ?int $window = null, ?string $yieldScoreAs = null): self
    {
        $this->combine = [
            'method' => 'FUNCTION',
            'expr' => $expr,
            'window' => $window,
            'yieldScoreAs' => $yieldScoreAs,
        ];
        return $this;
    }

    /** Load every field declared in the index, plus both arms' score aliases. */
    public function loadAll(bool $loadAll = true): self
    {
        $this->loadAll = $loadAll;
        if ($loadAll) {
            $this->loadFields = null;
        }
        return $this;
    }

    /**
     * Load only the fields passed in `$fields`. Each entry is either a
     * plain field identifier (string), a LoadField (renamed via AS --
     * valkey-search 1.3.0, requires search.emulate-release >= 1.3.0 to
     * take effect, see LoadField's docstring), or
     * `['field' => ..., 'as' => alias]` (equivalent to a LoadField,
     * kept for shape parity with the array the C layer consumes
     * directly). Must NOT include either arm's `yieldScoreAs` alias --
     * the server rejects that outright even though the alias is
     * otherwise freely reachable from every other clause.
     *
     * @param array<string|LoadField|array{field: string, as?: string}> $fields
     */
    public function load(array $fields): self
    {
        $normalized = [];
        foreach ($fields as $field) {
            if ($field instanceof LoadField) {
                $tokens = $field->toTokens();
                $normalized[] = [
                    'field' => $tokens[0],
                    'as' => $tokens[2] ?? null,
                ];
            } else {
                $normalized[] = $field;
            }
        }
        $this->loadFields = $normalized;
        $this->loadAll = false;
        return $this;
    }

    /** Add a FILTER clause to the aggregate-suffix tail. */
    public function filter(string $expression): self
    {
        $this->clauseTokens[] = 'FILTER';
        $this->clauseTokens[] = $expression;
        return $this;
    }

    /** Add a LIMIT clause to the aggregate-suffix tail. */
    public function limit(int $offset, int $count): self
    {
        $this->clauseTokens[] = 'LIMIT';
        $this->clauseTokens[] = (string) $offset;
        $this->clauseTokens[] = (string) $count;
        return $this;
    }

    /** Add a GROUPBY clause with optional reducers, to the aggregate-suffix tail. */
    public function groupBy(array $fields, array $reducers = []): self
    {
        $this->clauseTokens[] = 'GROUPBY';
        $this->clauseTokens[] = (string) count($fields);
        foreach ($fields as $field) {
            $this->clauseTokens[] = $field;
        }
        foreach ($reducers as $reducer) {
            $this->clauseTokens[] = 'REDUCE';
            foreach ($reducer->toTokens() as $token) {
                $this->clauseTokens[] = $token;
            }
        }
        return $this;
    }

    /** Add a SORTBY clause to the aggregate-suffix tail. */
    public function sortBy(array $fieldOrders, ?int $max = null): self
    {
        $this->clauseTokens[] = 'SORTBY';
        $this->clauseTokens[] = (string) (count($fieldOrders) * 2);
        foreach ($fieldOrders as $field => $order) {
            $this->clauseTokens[] = $field;
            $this->clauseTokens[] = strtoupper($order);
        }
        if ($max !== null) {
            $this->clauseTokens[] = 'MAX';
            $this->clauseTokens[] = (string) $max;
        }
        return $this;
    }

    /** Add an APPLY clause to the aggregate-suffix tail. */
    public function apply(string $expression, string $alias): self
    {
        $this->clauseTokens[] = 'APPLY';
        $this->clauseTokens[] = $expression;
        $this->clauseTokens[] = 'AS';
        $this->clauseTokens[] = $alias;
        return $this;
    }

    /**
     * Request a paged reply instead of the whole result set at once.
     * See FtAggregateBuilder::withCursor() for the reply-shape and
     * cursor-routing notes, which apply identically here.
     */
    public function withCursor(?int $count = null, ?int $maxIdle = null): self
    {
        $this->withCursor = ['count' => $count, 'maxidle' => $maxIdle];
        return $this;
    }

    /**
     * Restrict the query to the local shard rather than fanning out
     * across the cluster (a valkey-search extension).
     */
    public function localOnly(bool $localOnly = true): self
    {
        $this->localOnly = $localOnly;
        return $this;
    }

    /** Override module timeout in milliseconds. */
    public function timeout(int $ms): self
    {
        $this->timeout = $ms;
        return $this;
    }

    /**
     * Set query parameters (key => value pairs), referenced as a `$name`
     * VSIM vector reference or from an aggregate-clause expression (FILTER,
     * APPLY). Not substituted in the SEARCH arm's query text: that arm goes
     * through the same query pipeline as FT.SEARCH, where `$name`
     * substitutes only inside a KNN vector clause, which the SEARCH arm
     * does not have. A `$name` written directly into the SEARCH query is
     * either rejected outright or, for a TAG filter like `@tag:{$t}`,
     * silently matched against the literal text `$t`.
     */
    public function params(array $params): self
    {
        $this->params = $params;
        return $this;
    }

    /**
     * Set query dialect version. FT.HYBRID is fixed at dialect 2
     * server-side; passing any other value is rejected by the server.
     */
    public function dialect(int $dialect): self
    {
        $this->dialect = $dialect;
        return $this;
    }

    /**
     * Convert the builder state into the arguments for ftHybrid().
     *
     * @internal Called by the C layer via ftHybrid().
     * @throws \ValkeyGlideException if required fields are missing or
     *         mutually exclusive options are combined.
     */
    public function toArray(): array
    {
        if ($this->indexName === null) {
            throw new \ValkeyGlideException('FtHybridBuilder: index name is required');
        }
        if ($this->searchQuery === null) {
            throw new \ValkeyGlideException('FtHybridBuilder: search() is required');
        }
        if ($this->vsimField === null) {
            throw new \ValkeyGlideException('FtHybridBuilder: vsim() is required');
        }
        if (
            $this->vsimFilter !== null
            && $this->vsimEfRuntime !== null
            && ($this->vsimFilter['policy'] ?? null) === 'ADHOC'
        ) {
            throw new \ValkeyGlideException(
                'FtHybridBuilder: efRuntime is not supported with filter policy=ADHOC'
            );
        }

        return [
            'index' => $this->indexName,
            'search' => [
                'query' => $this->searchQuery,
                'scorer' => $this->searchScorer,
                'yieldScoreAs' => $this->searchYieldScoreAs,
            ],
            'vsim' => [
                'field' => $this->vsimField,
                'vector' => $this->vsimVector,
                'isParamRef' => $this->vsimVectorIsParamRef,
                'k' => $this->vsimK,
                'efRuntime' => $this->vsimEfRuntime,
                'shardKRatio' => $this->vsimShardKRatio,
                'yieldScoreAs' => $this->vsimYieldScoreAs,
                'filter' => $this->vsimFilter,
            ],
            'combine' => $this->combine,
            'loadAll' => $this->loadAll,
            'loadFields' => $this->loadFields,
            'clauseTokens' => $this->clauseTokens,
            'withCursor' => $this->withCursor,
            'localOnly' => $this->localOnly,
            'timeout' => $this->timeout,
            'params' => $this->params,
            'dialect' => $this->dialect,
        ];
    }
}
