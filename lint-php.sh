#!/bin/bash
set -e

# Parse arguments
FIX=false
if [[ "$1" == "--fix" ]]; then
    FIX=true
fi

echo "Running PHP linting..."

# Check working directory.
if [ ! -f "config.m4" ]; then
    echo "Error: This script must be run from the project root directory"
    exit 1
fi

# Install composer dependencies if needed.
if [ ! -d "vendor" ]; then
    echo "Installing composer dependencies..."
    if command -v composer &> /dev/null; then
        composer install --no-interaction --prefer-dist --optimize-autoloader
    else
        echo "Error: composer not found"
        exit 1
    fi
fi

# Check that phpcs is available.
if [ ! -f "vendor/bin/phpcs" ]; then
    echo "Error: phpcs not found. Run 'composer install' first."
    exit 1
fi

# Run PHP CodeSniffer or PHP Code Beautifier
if [ "$FIX" = true ]; then
    ./vendor/bin/phpcbf --standard=phpcs.xml
else
    ./vendor/bin/phpcs --standard=phpcs.xml
fi

echo "✓ PHP linting completed!"
