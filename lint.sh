#!/bin/bash
set -e

echo "Running PHP linters..."

# Check if we're in the php directory
if [ ! -f "config.m4" ]; then
    echo "Error: This script must be run from the php directory"
    exit 1
fi

# Install composer dependencies if composer.json exists and vendor doesn't
if [ -f "composer.json" ] && [ ! -d "vendor" ]; then
    echo "Installing composer dependencies..."
    if command -v composer &> /dev/null; then
        composer install --dev --no-progress --quiet
    else
        echo "Warning: composer not found, some PHP linting tools may not be available"
    fi
fi

echo "1. Running PHP CodeSniffer..."
if command -v phpcs &> /dev/null || [ -f "vendor/bin/phpcs" ]; then
    if [ -f "vendor/bin/phpcs" ]; then
        ./vendor/bin/phpcs --standard=phpcs.xml --colors
    else
        phpcs --standard=phpcs.xml --colors
    fi
    echo "✓ PHP CodeSniffer passed"
else
    echo "Warning: phpcs not found, skipping PHP coding standards check"
fi

echo "2. Checking C code formatting..."
if command -v clang-format &> /dev/null; then
    # Find C files but exclude protobuf generated files
    if find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | grep -q .; then
        find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | xargs clang-format --dry-run --Werror
        echo "✓ C code formatting check passed"
    else
        echo "No C files found to check"
    fi
else
    echo "Warning: clang-format not found, skipping C code formatting check"
fi


echo ""
echo "🎉 All linting checks passed!"
