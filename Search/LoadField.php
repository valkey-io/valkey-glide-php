<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * A single entry in the LOAD clause of FT.AGGREGATE or FT.HYBRID,
 * optionally renaming the loaded field via AS. Modelled on the RETURN
 * field concept for FT.SEARCH.
 *
 * `AS <alias>` in the LOAD clause is a compatibility fix introduced in
 * valkey-search 1.3.0 (src/commands/ft_aggregate_parser.cc, gated by
 * the `ft_aggregate_load_as` compatibility flag -- see
 * COMPATIBILITY.md). Below `search.emulate-release 1.3.0`, the legacy
 * branch does not recognise AS as a rename keyword at all: it is
 * consumed as just another LOAD field token rather than a rename
 * directive, and the server rejects the command outright with
 * "Index: field `AS` does not exist". Requires `search.emulate-release`
 * >= 1.3.0 to take effect -- see upstream COMPATIBILITY.md's
 * "Sunsetting of Incompatible Behavior" section.
 *
 * @see FtAggregateBuilder::load()
 * @see FtHybridBuilder::load()
 */
class LoadField
{
    private string $fieldIdentifier;
    private ?string $alias;

    public function __construct(string $fieldIdentifier, ?string $alias = null)
    {
        $this->fieldIdentifier = $fieldIdentifier;
        $this->alias = $alias;
    }

    /** @internal Flat token list for this LOAD entry: [field] or [field, 'AS', alias]. */
    public function toTokens(): array
    {
        $tokens = [$this->fieldIdentifier];
        if ($this->alias !== null) {
            $tokens[] = 'AS';
            $tokens[] = $this->alias;
        }
        return $tokens;
    }
}
