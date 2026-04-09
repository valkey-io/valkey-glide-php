<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * A TEXT field definition for FT.CREATE schema.
 *
 * @see FtCreateBuilder
 * @see https://valkey.io/commands/ft.create/
 */
class FtTextField
{
    private string $name;
    private ?string $alias = null;
    private bool $sortable = false;
    private bool $noStem = false;
    private ?float $weight = null;
    private bool $withSuffixTrie = false;
    private bool $noSuffixTrie = false;

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

    public function noStem(bool $noStem = true): self
    {
        $this->noStem = $noStem;
        return $this;
    }

    public function weight(float $weight): self
    {
        $this->weight = $weight;
        return $this;
    }

    public function withSuffixTrie(bool $enable = true): self
    {
        $this->withSuffixTrie = $enable;
        return $this;
    }

    public function noSuffixTrie(bool $disable = true): self
    {
        $this->noSuffixTrie = $disable;
        return $this;
    }

    /**
     * @internal
     * @throws \ValkeyGlideException if mutually exclusive options are combined.
     */
    public function toArray(): array
    {
        if ($this->withSuffixTrie && $this->noSuffixTrie) {
            throw new \ValkeyGlideException(
                'FtTextField: WITHSUFFIXTRIE and NOSUFFIXTRIE are mutually exclusive'
            );
        }

        $f = ['name' => $this->name, 'type' => 'TEXT'];
        if ($this->alias !== null) {
            $f['alias'] = $this->alias;
        }
        if ($this->noStem) {
            $f['nostem'] = true;
        }
        if ($this->weight !== null) {
            $f['weight'] = $this->weight;
        }
        if ($this->withSuffixTrie) {
            $f['withsuffixtrie'] = true;
        }
        if ($this->noSuffixTrie) {
            $f['nosuffixtrie'] = true;
        }
        if ($this->sortable) {
            $f['sortable'] = true;
        }
        return $f;
    }
}
