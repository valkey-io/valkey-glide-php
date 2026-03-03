#!/bin/bash

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Stopping all Valkey servers..."

# Standalone servers
# ------------------

# Non-TLS standalone servers.
for port in 6379 6380 6381; do
    valkey-cli -p $port shutdown nosave 2>/dev/null || true
done

rm -rf valkey_data 2>/dev/null || true

# TLS standalone servers.
# TODO #5494: Extra 'tls-' prefix needed to handle 'cluster_manager.py' bug
# https://github.com/valkey-io/valkey-glide/issues/5494
../valkey-glide/utils/cluster_manager.py --tls stop --prefix tls-tls-standalone || true

# Cluster servers
# ---------------

# Non-TLS cluster servers.
for port in 7001 7002 7003 7004 7005 7006; do
    valkey-cli -p $port shutdown nosave 2>/dev/null || true
done

rm -rf valkey_cluster 2>/dev/null || true 

# TLS cluster servers.
# TODO #5494: Extra 'tls-' prefix needed to handle 'cluster_manager.py' bug
# https://github.com/valkey-io/valkey-glide/issues/5494
../valkey-glide/utils/cluster_manager.py --tls stop --prefix tls-tls-cluster || true
rm -rf ../valkey-glide/utils/clusters/tls-* 2>/dev/null || true

# Auth cluster servers.
../valkey-glide/utils/cluster_manager.py --auth dummy_password stop --prefix auth-cluster || true
rm -rf ../valkey-glide/utils/clusters/auth-* 2>/dev/null || true

echo "✅ All Valkey servers stopped and cleaned up"
