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

    /** @internal */
    public function toArray(): array
    {
        $f = ['name' => $this->name, 'type' => 'NUMERIC'];
        if ($this->alias !== null) {
            $f['alias'] = $this->alias;
        }
        if ($this->sortable) {
            $f['sortable'] = true;
        }
        return $f;
    }
}
