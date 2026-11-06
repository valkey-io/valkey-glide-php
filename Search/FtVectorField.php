<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * A VECTOR field definition for FT.CREATE schema.
 *
 * Use the static factory methods to create HNSW or FLAT vector fields
 * with the required parameters (dim, metric) enforced at construction.
 *
 * @see FtCreateBuilder
 * @see https://valkey.io/commands/ft.create/
 */
class FtVectorField
{
    private string $name;
    private string $algorithm;
    private int $dim;
    private string $metric;
    private string $vectorType;
    private ?string $alias = null;
    private ?int $initialCap = null;
    private ?int $m = null;
    private ?int $efConstruction = null;
    private ?int $efRuntime = null;
    private bool $sortable = false;

    private function __construct(
        string $name,
        string $algorithm,
        int $dim,
        string $metric,
        string $vectorType = 'FLOAT32'
    ) {
        $this->name = $name;
        $this->algorithm = $algorithm;
        $this->dim = $dim;
        $this->metric = $metric;
        $this->vectorType = $vectorType;
    }

    /**
     * Create an HNSW vector field.
     *
     * @param string $name       Field name or JSON path.
     * @param int    $dim        Number of dimensions.
     * @param string $metric     Distance metric: 'L2', 'IP', or 'COSINE'.
     * @param string $vectorType Element type: 'FLOAT32' (default), 'FLOAT16', or
     *                           'BFLOAT16' (valkey-search 1.3.0,
     *                           src/indexes/vector_base.h). Callers must pass
     *                           already-encoded bytes for the vector value --
     *                           this builder does not encode FLOAT16/BFLOAT16
     *                           on the caller's behalf. PHP has no built-in
     *                           float16 packer; pack('v', ...) with manually
     *                           computed half-precision bits, or an FFI call
     *                           into a C half-float routine, both produce the
     *                           correct byte layout. For BFLOAT16, take the
     *                           high 2 bytes of each FLOAT32 element's
     *                           little-endian pack('g', $value) result.
     */
    public static function hnsw(string $name, int $dim, string $metric, string $vectorType = 'FLOAT32'): self
    {
        return new self($name, 'HNSW', $dim, $metric, $vectorType);
    }

    /**
     * Create a FLAT vector field.
     *
     * @param string $name       Field name or JSON path.
     * @param int    $dim        Number of dimensions.
     * @param string $metric     Distance metric: 'L2', 'IP', or 'COSINE'.
     * @param string $vectorType Element type: 'FLOAT32' (default), 'FLOAT16', or
     *                           'BFLOAT16'. See hnsw()'s docstring for the
     *                           byte-encoding notes, which apply identically
     *                           here.
     */
    public static function flat(string $name, int $dim, string $metric, string $vectorType = 'FLOAT32'): self
    {
        return new self($name, 'FLAT', $dim, $metric, $vectorType);
    }

    public function alias(string $alias): self
    {
        $this->alias = $alias;
        return $this;
    }

    /**
     * Emit SORTABLE for this vector field. Accepted by the server for
     * compatibility but has no effect on vector fields -- vector values
     * are never used for sorting. UNF is not accepted after SORTABLE on a
     * VECTOR field (the parser only recognises the pairing on TEXT/TAG/
     * NUMERIC); there is no unf() method here for that reason.
     */
    public function sortable(bool $sortable = true): self
    {
        $this->sortable = $sortable;
        return $this;
    }

    public function initialCap(int $cap): self
    {
        $this->initialCap = $cap;
        return $this;
    }

    /** HNSW only: max outgoing edges per node. */
    public function m(int $m): self
    {
        $this->m = $m;
        return $this;
    }

    /** HNSW only: vectors examined during index build. */
    public function efConstruction(int $efConstruction): self
    {
        $this->efConstruction = $efConstruction;
        return $this;
    }

    /** HNSW only: vectors examined during query. */
    public function efRuntime(int $efRuntime): self
    {
        $this->efRuntime = $efRuntime;
        return $this;
    }

    /** @internal */
    public function toArray(): array
    {
        $f = [
            'name' => $this->name,
            'type' => 'VECTOR',
            'algorithm' => $this->algorithm,
            'dim' => $this->dim,
            'metric' => $this->metric,
            'vector_type' => $this->vectorType,
        ];
        if ($this->alias !== null) {
            $f['alias'] = $this->alias;
        }
        if ($this->initialCap !== null) {
            $f['initial_cap'] = $this->initialCap;
        }
        if ($this->m !== null) {
            $f['m'] = $this->m;
        }
        if ($this->efConstruction !== null) {
            $f['ef_construction'] = $this->efConstruction;
        }
        if ($this->efRuntime !== null) {
            $f['ef_runtime'] = $this->efRuntime;
        }
        if ($this->sortable) {
            $f['sortable'] = true;
        }
        return $f;
    }
}
