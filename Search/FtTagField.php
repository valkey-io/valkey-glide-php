<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * A TAG field definition for FT.CREATE schema.
 *
 * @see FtCreateBuilder
 * @see https://valkey.io/commands/ft.create/
 */
class FtTagField
{
    private string $name;
    private ?string $alias = null;
    private bool $sortable = false;
    private bool $unf = false;
    private ?string $separator = null;
    private bool $caseSensitive = false;

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
     * Adds the UNF attribute alongside SORTABLE. valkey-search stores and
     * reports UNF through FT.INFO but does not change the TAG sort order.
     * Requires sortable() to also be set; toArray() throws otherwise.
     */
    public function unf(bool $unf = true): self
    {
        $this->unf = $unf;
        return $this;
    }

    public function separator(string $separator): self
    {
        $this->separator = $separator;
        return $this;
    }

    public function caseSensitive(bool $caseSensitive = true): self
    {
        $this->caseSensitive = $caseSensitive;
        return $this;
    }

    /**
     * @internal
     * @throws \ValkeyGlideException if unf() is set without sortable().
     */
    public function toArray(): array
    {
        if ($this->unf && !$this->sortable) {
            throw new \ValkeyGlideException('FtTagField: unf() requires sortable() to be set');
        }
        $f = ['name' => $this->name, 'type' => 'TAG'];
        if ($this->alias !== null) {
            $f['alias'] = $this->alias;
        }
        if ($this->separator !== null) {
            $f['separator'] = $this->separator;
        }
        if ($this->caseSensitive) {
            $f['casesensitive'] = true;
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
