<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

/**
 * ClusterScanCursor represents a cursor for cluster scan operations.
 *
 * This class provides API compatibility with the Go implementation and
 * manages cluster scan cursor lifecycle to prevent memory leaks.
 */
final class ClusterScanCursor
{
    /**
     * Create a new ClusterScanCursor instance.
     *
     * @param string|null $cursorId The cursor ID string. Defaults to "0" for initial cursor.
     */
    public function __construct(?string $cursorId = null)
    {
    }

    /**
     * Get the cursor ID string.
     *
     * @return string The cursor ID
     */
    public function getCursor(): string
    {
    }

    /**
     * Get the cursor ID string.
     *
     * @return string The next cursor ID, avaliable after a scan operation
     */
    public function getNextCursor(): string
    {
    }

    /**
     * Check if the cursor indicates scan completion.
     *
     * @return bool True if cursor equals "finished", false otherwise
     */
    public function isFinished(): bool
    {
    }
}
