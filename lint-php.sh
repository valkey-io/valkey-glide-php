#!/bin/bash
set -e

echo "Running PHP code linting..."

# Check working directory.
if [ ! -f "config.m4" ]; then
    echo "Error: This script must be run from the project root directory"
    exit 1
fi

# Install composer dependencies if needed
if [ -f "composer.json" ] && [ ! -d "vendor" ]; then
    echo "Installing composer dependencies..."
    if command -v composer &> /dev/null; then
        composer install --dev --no-progress --quiet
    else
        echo "Error: composer not found"
        exit 1
    fi
fi

# Check that phpcs is available.
if ! command -v phpcs &> /dev/null && [ ! -f "vendor/bin/phpcs" ]; then
    echo "Error: phpcs not found"
    exit 1
fi

# Run PHP CodeSniffer
echo "Running PHP CodeSniffer..."
if [ -f "vendor/bin/phpcs" ]; then
    ./vendor/bin/phpcs --standard=phpcs.xml --colors
else
    phpcs --standard=phpcs.xml --colors
fi
echo "✓ PHP CodeSniffer passed"
