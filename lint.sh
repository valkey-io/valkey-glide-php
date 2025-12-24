#!/bin/bash
set -e

echo "Running all linters..."

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run PHP linting
echo ""
echo "=== PHP Code Linting ==="
"$SCRIPT_DIR/lint-php.sh"

# Run C linting
echo ""
echo "=== C Code Linting ==="
"$SCRIPT_DIR/lint-c.sh"

echo ""
echo "✓ All linting checks passed!"
