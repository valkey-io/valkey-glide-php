#!/bin/bash
set -e

echo "Running C code linting..."

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
    echo "Warning: clang-format version $CLANG_VERSION detected, but version 18.x is required."
    exit 1
fi

# Run clang-format:
# - Include all .c and .h files.
# - Exclude generated protobuf files (*.pb-c.*).
# - Exclude files in valkey-glide submodule.
# - Exclude files in 'include' directory.
find . -name "*.c" -o -name "*.h" | \
    grep -v "\.pb-c\." | \
    grep -v "valkey-glide/" | \
    grep -v "include/" | \
    xargs clang-format --dry-run --Werror

echo "✓ C code formatting check passed"
