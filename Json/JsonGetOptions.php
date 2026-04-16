<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Json;

/**
 * Options for formatting JSON data in the JSON.GET command.
 *
 * Usage:
 *   $client->jsonGet('doc', '$', JsonGetOptions::builder()
 *       ->indent('  ')
 *       ->newline("\n")
 *       ->space(' ')
 *   );
 *
 * @see \ValkeyGlide::jsonGet()
 * @see https://valkey.io/commands/json.get/
 */
class JsonGetOptions
{
    private ?string $indent = null;
    private ?string $newline = null;
    private ?string $space = null;

    public static function builder(): self
    {
        return new self();
    }

    /** Sets an indentation string for nested levels. */
    public function indent(string $indent): self
    {
        $this->indent = $indent;
        return $this;
    }

    /** Sets a string that's printed at the end of each line. */
    public function newline(string $newline): self
    {
        $this->newline = $newline;
        return $this;
    }

    /** Sets a string that's put between a key and a value. */
    public function space(string $space): self
    {
        $this->space = $space;
        return $this;
    }

    /** Convert to associative array for the C extension. */
    public function toArray(): array
    {
        $opts = [];
        if ($this->indent !== null) {
            $opts['indent'] = $this->indent;
        }
        if ($this->newline !== null) {
            $opts['newline'] = $this->newline;
        }
        if ($this->space !== null) {
            $opts['space'] = $this->space;
        }
        return $opts;
    }
}
