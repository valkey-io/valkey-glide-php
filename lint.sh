#!/bin/bash
set -e

# Parse arguments
FIX=""
if [[ "$1" == "--fix" ]]; then
    FIX="$1"
fi

echo "Running all linting..."

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run PHP linting
echo ""
echo "=== PHP Code Linting ==="
"$SCRIPT_DIR/lint-php.sh" "$FIX"

# Run C linting
echo ""
echo "=== C Code Linting ==="
"$SCRIPT_DIR/lint-c.sh" "$FIX"

echo ""
echo "✓ All linting completed!"
