#!/bin/bash

set -e

# Get the directory where this script is located and use it as working directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BASE_DIR="$(pwd)/valkey_data"

# Create data directories with full path
for port in 6379 6380 6381 6382 6383 6384; do
  mkdir -p "$BASE_DIR/$port"
done

# Start primary (6379)
valkey-server --port 6379 \
  --bind 127.0.0.1 ::1 \
  --dir "$BASE_DIR/6379" \
  --daemonize yes \
  --logfile "$BASE_DIR/6379/valkey.log" \
  --enable-debug-command yes

# Start replicas (6380, 6381)
valkey-server --port 6380 \
  --bind 127.0.0.1 ::1 \
  --dir "$BASE_DIR/6380" \
  --daemonize yes \
  --logfile "$BASE_DIR/6380/valkey.log" \
  --enable-debug-command yes


valkey-server --port 6381 \
  --bind 127.0.0.1 ::1 \
  --dir "$BASE_DIR/6381" \
  --daemonize yes \
  --logfile "$BASE_DIR/6381/valkey.log" \
  --enable-debug-command yes

# Start independent standalone server for replicaof/failover tests and migrate tests (6382)
valkey-server --port 6382 \
  --bind 0.0.0.0 \
  --dir "$BASE_DIR/6382" \
  --daemonize yes \
  --logfile "$BASE_DIR/6382/valkey.log" \
  --enable-debug-command yes \
  --protected-mode no

# Start replica of 6382 for failover tests (6383)
valkey-server --port 6383 \
  --bind 0.0.0.0 \
  --dir "$BASE_DIR/6383" \
  --daemonize yes \
  --logfile "$BASE_DIR/6383/valkey.log" \
  --enable-debug-command yes \
  --protected-mode no

# Start independent standalone server with no replicas (6384)
valkey-server --port 6384 \
  --bind 0.0.0.0 \
  --dir "$BASE_DIR/6384" \
  --daemonize yes \
  --logfile "$BASE_DIR/6384/valkey.log" \
  --enable-debug-command yes \
  --protected-mode no

# Handle TLS setup with graceful failure
echo "Setting up TLS standalone server..."
if ../valkey-glide/utils/cluster_manager.py --tls start --prefix tls-standalone -p 6400 -r 0; then
    echo "✅ TLS standalone server started on port 6400"
else
    echo "⚠️  WARNING: TLS standalone setup failed (port 6400 may be in use), continuing without TLS..."
fi

# Handle mTLS setup (client-certificate verification) with graceful failure
echo "Setting up mTLS standalone server (client-cert verification)..."
if ../valkey-glide/utils/cluster_manager.py --tls start --tls-auth-clients --prefix mtls-standalone -p 6405 -r 0; then
    echo "✅ mTLS standalone server started on port 6405"
else
    echo "⚠️  WARNING: mTLS standalone setup failed (port 6405 may be in use), continuing without mTLS..."
fi

# Wait a moment for servers to start
sleep 2

# Make 6380 and 6381 replicas of 6379
valkey-cli -p 6380 REPLICAOF 127.0.0.1 6379
valkey-cli -p 6381 REPLICAOF 127.0.0.1 6379

# Make 6383 a replica of 6382 (for failover tests)
valkey-cli -p 6383 REPLICAOF 127.0.0.1 6382

echo "✅ Valkey setup complete:"
echo "- Primary: 127.0.0.1:6379"
echo "- Replica: 127.0.0.1:6380"
echo "- Replica: 127.0.0.1:6381"
