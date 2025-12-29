#!/bin/bash
set -e

# Parse arguments
FIX=false
if [[ "$1" == "--fix" ]]; then
    FIX=true
fi

echo "Running C linting..."

# Check that clang-format is available.
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format not found"
    echo "Install with:"
    echo "- \`sudo apt install clang-format-18\` (Ubuntu)"
    echo "- \`brew install llvm@18\` (macOS)"
    exit 1
fi

# Verify clang-format version.
CLANG_VERSION=$(clang-format --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
if [[ ! "$CLANG_VERSION" =~ ^18\. ]]; then
    echo "Error: Expected clang-format version 18.x but got $CLANG_VERSION"
    exit 1
fi

# Run clang-format:
# - Include all .c and .h files.
# - Exclude generated protobuf files (*.pb-c.*).
# - Exclude files in valkey-glide submodule.
# - Exclude files in 'include' directory.
CLANG_FORMAT_OPTIONS="--dry-run --Werror"
if [ "$FIX" = true ]; then
    CLANG_FORMAT_OPTIONS="-i"
fi

find . \( -name "*.c" -o -name "*.h" \) -print0 | \
    grep -zv "\.pb-c\." | \
    grep -zv "valkey-glide/" | \
    grep -zv "include/" | \
    xargs -0 -r clang-format $CLANG_FORMAT_OPTIONS

echo "✓ C linting completed!"
