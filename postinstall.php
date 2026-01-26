#!/usr/bin/env php
<?php
/**
 * Post-install script for Valkey GLIDE PHP extension
 */

echo "\n";
echo "================================================================================\n";
echo "Valkey GLIDE PHP Extension Installed Successfully!\n";
echo "================================================================================\n";
echo "\n";
echo "To enable the extension, add this line to your php.ini:\n";
echo "  extension=valkey_glide\n";
echo "\n";
echo "Optional: PHPRedis Compatibility Aliases\n";
echo "----------------------------------------\n";
echo "For easier migration from PHPRedis, you can use Redis/RedisCluster class names.\n";
echo "\n";
echo "The aliases file is located at:\n";

// Try to find the data directory
$dataDir = '';
if (function_exists('shell_exec')) {
    $dataDir = trim(shell_exec('pecl config-get data_dir 2>/dev/null'));
}

if ($dataDir && is_dir($dataDir)) {
    echo "  $dataDir/valkey_glide/phpredis_aliases.php\n";
} else {
    echo "  (Run 'pecl config-get data_dir' to find the location)\n";
}

echo "\n";
echo "Usage:\n";
echo "  require_once 'phpredis_aliases.php';\n";
echo "  \$redis = new Redis([['host' => 'localhost', 'port' => 6379]]);\n";
echo "\n";
echo "Requirements: PHP 8.3+ (for class_alias support with internal classes)\n";
echo "\n";
echo "Documentation: https://github.com/valkey-io/valkey-glide-php\n";
echo "================================================================================\n";
echo "\n";
