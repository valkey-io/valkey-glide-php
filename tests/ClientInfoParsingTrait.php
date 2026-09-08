<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

/**
 * Shared helpers for parsing `CLIENT INFO` responses in feature tests.
 *
 * Used by both ValkeyGlideFeaturesTest and ValkeyGlideClusterFeaturesTest so the
 * field-extraction logic lives in exactly one place.
 */
trait ClientInfoParsingTrait
{
    /**
     * Extract a single field value from a CLIENT INFO response line.
     *
     * CLIENT INFO returns space-separated key=value pairs. Accepted metadata
     * values never contain whitespace, so the value runs up to the next space.
     *
     * @param string $info  The raw CLIENT INFO string.
     * @param string $field The field name to extract (e.g. "lib-name").
     * @return string|null The field value, or null when the field is absent.
     */
    protected function getClientInfoField(string $info, string $field): ?string
    {
        /* L-6: \S+ (not \S*) so a present-but-empty field is a parse miss rather
         * than an empty string indistinguishable from absent, and so a value that
         * wrongly leaked a space is not silently truncated to its prefix. */
        if (preg_match('/(?:^| )' . preg_quote($field, '/') . '=(\S+)/', $info, $m)) {
            return $m[1];
        }
        return null;
    }
}
