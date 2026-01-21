<?php

/**
 * Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0
 */

declare(strict_types=1);

/**
 * PHPRedis Compatibility Aliases
 *
 * This file provides class aliases to make Valkey Glide compatible with PHPRedis class names.
 *
 * Usage:
 *   require_once 'phpredis_aliases.php';
 *   $redis = new Redis(); // Uses ValkeyGlide
 *
 * Requirements:
 *   - PHP 8.3 or higher (class_alias support for internal classes)
 *
 * Note: This will fail if PHPRedis is already loaded.
 */

if (PHP_VERSION_ID < 80300) {
    trigger_error(
        'PHPRedis compatibility aliases require PHP 8.3 or higher. ' .
        'Current version: ' . PHP_VERSION,
        E_USER_ERROR
    );
    return;
}

if (class_exists('Redis')) {
    trigger_error(
        'Redis class already exists. Cannot create PHPRedis compatibility aliases. ' .
        'PHPRedis may already be installed.',
        E_USER_WARNING
    );
    return;
}

class_alias('ValkeyGlide', 'Redis');
class_alias('ValkeyGlideCluster', 'RedisCluster');
class_alias('ValkeyGlideException', 'RedisException');
