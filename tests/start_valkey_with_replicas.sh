#!/bin/bash

set -e

BASE_DIR="$(pwd)/valkey_data"

# Create data directories with full path
for port in 6379 6380 6381; do
  mkdir -p "$BASE_DIR/$port"
done

# Start primary (6379)
valkey-server --port 6379 \
  --dir "$BASE_DIR/6379" \
  --daemonize yes \
  --logfile "$BASE_DIR/6379/valkey.log"

# Start replicas (6380, 6381)
valkey-server --port 6380 \
  --dir "$BASE_DIR/6380" \
  --daemonize yes \
  --logfile "$BASE_DIR/6380/valkey.log"

valkey-server --port 6381 \
  --dir "$BASE_DIR/6381" \
  --daemonize yes \
  --logfile "$BASE_DIR/6381/valkey.log"

# Wait a moment for servers to start
sleep 2

# Make 6380 and 6381 replicas of 6379
valkey-cli -p 6380 REPLICAOF 127.0.0.1 6379
valkey-cli -p 6381 REPLICAOF 127.0.0.1 6379

echo "✅ Valkey setup complete:"
echo "- Primary: 127.0.0.1:6379"
echo "- Replica: 127.0.0.1:6380"
echo "- Replica: 127.0.0.1:6381"

