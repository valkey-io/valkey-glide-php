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
    private ?string $alias = null;
    private ?int $initialCap = null;
    private ?int $m = null;
    private ?int $efConstruction = null;
    private ?int $efRuntime = null;

    private function __construct(string $name, string $algorithm, int $dim, string $metric)
    {
        $this->name = $name;
        $this->algorithm = $algorithm;
        $this->dim = $dim;
        $this->metric = $metric;
    }

    /**
     * Create an HNSW vector field.
     *
     * @param string $name   Field name or JSON path.
     * @param int    $dim    Number of dimensions.
     * @param string $metric Distance metric: 'L2', 'IP', or 'COSINE'.
     */
    public static function hnsw(string $name, int $dim, string $metric): self
    {
        return new self($name, 'HNSW', $dim, $metric);
    }

    /**
     * Create a FLAT vector field.
     *
     * @param string $name   Field name or JSON path.
     * @param int    $dim    Number of dimensions.
     * @param string $metric Distance metric: 'L2', 'IP', or 'COSINE'.
     */
    public static function flat(string $name, int $dim, string $metric): self
    {
        return new self($name, 'FLAT', $dim, $metric);
    }

    public function alias(string $alias): self
    {
        $this->alias = $alias;
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
        return $f;
    }
}
