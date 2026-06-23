#!/bin/bash
# Resolves and checks out the correct valkey-glide submodule commit.
# Used by config.m4 and Makefile.frag when git submodule update fails.
#
# Usage: ./utils/resolve_submodule.sh
#
# This script will:
# 1. Determine the correct commit from git tree or .submodule-commits
# 2. Clone the submodule repository
# 3. Checkout the pinned commit

set -e

SUBMODULE_PATH="valkey-glide"
SUBMODULE_URL="https://github.com/valkey-io/valkey-glide.git"

# Skip if submodule is already initialized
if [ -d "$SUBMODULE_PATH/.git" ] || [ -f "$SUBMODULE_PATH/.git" ]; then
    echo "Submodule already initialized"
    exit 0
fi

# Determine the correct commit
SUBMODULE_COMMIT=""
if [ -d ".git" ]; then
    SUBMODULE_COMMIT=$(git ls-tree HEAD "$SUBMODULE_PATH" 2>/dev/null | awk '{print $3}')
fi
if [ -z "$SUBMODULE_COMMIT" ] && [ -f ".submodule-commits" ]; then
    SUBMODULE_COMMIT=$(grep "^${SUBMODULE_PATH}=" .submodule-commits | cut -d= -f2)
fi

if [ -z "$SUBMODULE_COMMIT" ]; then
    echo "ERROR: Cannot determine submodule commit" >&2
    exit 1
fi

echo "Resolving submodule at commit $SUBMODULE_COMMIT"

# Remove existing directory if not a git repo
if [ -d "$SUBMODULE_PATH" ]; then
    rm -rf "$SUBMODULE_PATH"
fi

# Clone and checkout the specific commit
git clone --depth 1 "$SUBMODULE_URL" "$SUBMODULE_PATH"
cd "$SUBMODULE_PATH"
git fetch --depth 1 origin "$SUBMODULE_COMMIT"
git checkout "$SUBMODULE_COMMIT"
cd ..

echo "Submodule checked out at $SUBMODULE_COMMIT"
