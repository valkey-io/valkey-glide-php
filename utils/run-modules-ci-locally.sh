#!/usr/bin/env bash
# Reproduces the module-test shape of the main valkey-glide repo's
# "Modules Tests" job for this PHP client, against an overridable module
# image. Follows the same approach as that repo's
# utils/run-modules-ci-locally.sh, running both the standalone module
# server (ValkeyGlideSearchTest.php) and the cluster one
# (ValkeyGlideClusterSearchTest.php).
#
# No module CI job exists yet for this repo -- this script and
# utils/start-valkey-docker-modules.sh (vendored verbatim from the main
# repo, see that file's own header) are new.
#
# Usage: the same three configurations used across every language's port:
#   utils/run-modules-ci-locally.sh                                       # 1: rc2, emulate-release 1.3.0 (default; run-proof)
#   IMAGE=valkey/valkey-bundle:9.1.3 EMULATE_RELEASE="" utils/run-modules-ci-locally.sh   # 2: 9.1.3, search 1.2.1 (skip-proof)
#   EMULATE_RELEASE="" utils/run-modules-ci-locally.sh                    # 3: rc2, module's own default emulate-release
#
# SEARCH_MODULE_HOST_PATH overrides the in-container search module .so
# with a host-built one; SEARCH_MODULE_PATH overrides
# only the in-container path baked into the generated conf. Both are
# read directly by start-valkey-docker-modules.sh.
#
# Requires: docker, valkey-cli, phpize/php already built
# (./configure --enable-valkey-glide && make build-modules-pre && make).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRATCH_DIR="${REPO_ROOT}/.module-ci-local-scratch"
mkdir -p "$SCRATCH_DIR"

IMAGE="${IMAGE:-valkey/valkey-bundle:9.2.0-rc2}"
ENGINE_VERSION="${ENGINE_VERSION:-9.0}"
CLUSTER_PORT_START="${CLUSTER_PORT_START:-8101}"
STANDALONE_PORT="${STANDALONE_PORT:-6489}"
EMULATE_RELEASE="${EMULATE_RELEASE-1.3.0}"

if [ -n "$EMULATE_RELEASE" ] && ! [[ "$EMULATE_RELEASE" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "ERROR: EMULATE_RELEASE must be X.Y.Z (e.g. 1.3.0) or empty, got: ${EMULATE_RELEASE}" >&2
    exit 1
fi

cleanup() {
    echo "=== Tearing down module containers ==="
    docker rm -f "valkey-modules-standalone-${STANDALONE_PORT}" >/dev/null 2>&1 || true
    for port in $(seq "$CLUSTER_PORT_START" "$((CLUSTER_PORT_START + 5))"); do
        docker rm -f "valkey-modules-cluster-${port}" >/dev/null 2>&1 || true
    done
}
trap cleanup EXIT

echo "=== Starting module servers (IMAGE=${IMAGE}, EMULATE_RELEASE=${EMULATE_RELEASE:-<module default>}) ==="
export IMAGE ENGINE_VERSION CLUSTER_PORT_START STANDALONE_PORT SCRATCH_DIR
if [ -n "$EMULATE_RELEASE" ]; then
    export SEARCH_MODULE_ARGS="--emulate-release ${EMULATE_RELEASE}"
else
    export SEARCH_MODULE_ARGS=""
fi
ENV_CAPTURE="${SCRATCH_DIR}/env-capture"
GITHUB_ENV="$ENV_CAPTURE" "${REPO_ROOT}/utils/start-valkey-docker-modules.sh"
# shellcheck disable=SC1090
source "$ENV_CAPTURE"
rm -f "$ENV_CAPTURE"

echo "=== Liveness: valkey_version, MODULE LIST, search.emulate-release ==="
for port in "$STANDALONE_PORT" $(seq "$CLUSTER_PORT_START" "$((CLUSTER_PORT_START + 5))"); do
    echo "--- port $port ---"
    valkey-cli -p "$port" INFO server | grep -i valkey_version
    valkey-cli -p "$port" MODULE LIST
    valkey-cli -p "$port" CONFIG GET search.emulate-release
done

echo "=== Running PHP module tests (standalone, port ${STANDALONE_PORT}) ==="
cd "${REPO_ROOT}/tests"
set +e
php -n -d extension="${REPO_ROOT}/modules/valkey_glide.so" TestValkeyGlide.php \
    --host 127.0.0.1 --port "$STANDALONE_PORT" --class valkeyglidesearch
STANDALONE_STATUS=$?
set -e

echo "=== Running PHP module tests (cluster slot primary, port ${CLUSTER_PORT_START}) ==="
set +e
php -n -d extension="${REPO_ROOT}/modules/valkey_glide.so" TestValkeyGlide.php \
    --host 127.0.0.1 --port "$CLUSTER_PORT_START" --class valkeyglideclustersearch
CLUSTER_STATUS=$?
set -e

if [ "$STANDALONE_STATUS" -ne 0 ] || [ "$CLUSTER_STATUS" -ne 0 ]; then
    echo "FAILED: standalone exit=${STANDALONE_STATUS}, cluster exit=${CLUSTER_STATUS}" >&2
    exit 1
fi
echo "All module tests passed."
