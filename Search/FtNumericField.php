<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * A NUMERIC field definition for FT.CREATE schema.
 *
 * @see FtCreateBuilder
 * @see https://valkey.io/commands/ft.create/
 */
class FtNumericField
{
    private string $name;
    private ?string $alias = null;
    private bool $sortable = false;
    private bool $unf = false;

    public function __construct(string $name)
    {
        $this->name = $name;
    }

    public function alias(string $alias): self
    {
        $this->alias = $alias;
        return $this;
    }

    public function sortable(bool $sortable = true): self
    {
        $this->sortable = $sortable;
        return $this;
    }

    /**
     * Emits UNF alongside SORTABLE (valkey-search 1.3.0,
     * src/commands/ft_create_parser.cc). NUMERIC fields are never
     * normalized for sorting, so this has no observable effect beyond
     * emitting the keyword, but it's accepted here for API symmetry
     * with TEXT/TAG. Requires sortable() to also be set; toArray()
     * throws otherwise.
     */
    public function unf(bool $unf = true): self
    {
        $this->unf = $unf;
        return $this;
    }

    /**
     * @internal
     * @throws \ValkeyGlideException if unf() is set without sortable().
     */
    public function toArray(): array
    {
        if ($this->unf && !$this->sortable) {
            throw new \ValkeyGlideException('FtNumericField: unf() requires sortable() to be set');
        }
        $f = ['name' => $this->name, 'type' => 'NUMERIC'];
        if ($this->alias !== null) {
            $f['alias'] = $this->alias;
        }
        if ($this->sortable) {
            $f['sortable'] = true;
        }
        if ($this->unf) {
            $f['unf'] = true;
        }
        return $f;
    }
}
