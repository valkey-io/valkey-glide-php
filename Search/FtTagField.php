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

    /** @internal */
    public function toArray(): array
    {
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
        return $f;
    }
}
