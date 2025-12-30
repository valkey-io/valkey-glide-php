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
  --logfile "$BASE_DIR/6379/valkey.log" \
  --enable-debug-command yes

# Start replicas (6380, 6381)
valkey-server --port 6380 \
  --dir "$BASE_DIR/6380" \
  --daemonize yes \
  --logfile "$BASE_DIR/6380/valkey.log" \
  --enable-debug-command yes


valkey-server --port 6381 \
  --dir "$BASE_DIR/6381" \
  --daemonize yes \
  --logfile "$BASE_DIR/6381/valkey.log" \
  --enable-debug-command yes

# Handle TLS setup with graceful failure
echo "Setting up TLS standalone server on port 6400..."
echo "DEBUG: Current directory: $(pwd)"
echo "DEBUG: Checking for cluster_manager.py: $(ls -la ../valkey-glide/utils/cluster_manager.py 2>&1)"
echo "DEBUG: Checking for TLS certs: $(ls -la ../valkey-glide/utils/tls_crts/ 2>&1 | head -5)"

if python3 ../valkey-glide/utils/cluster_manager.py --tls start --prefix tls-standalone -p 6400 -r 0; then
    echo "✅ TLS standalone server started on port 6400"
    
    # Verify TLS server is responding
    echo "Verifying TLS server connectivity..."
    for i in {1..10}; do
        if nc -z 127.0.0.1 6400 2>/dev/null; then
            echo "✅ TLS server verified on port 6400"
            break
        fi
        echo "Waiting for TLS server... attempt $i/10"
        sleep 1
    done
else
    echo "⚠️  WARNING: TLS standalone setup failed, continuing without TLS..."
    echo "TLS tests will be skipped or may fail."
fi

# Wait a moment for servers to start
sleep 2

# Make 6380 and 6381 replicas of 6379
valkey-cli -p 6380 REPLICAOF 127.0.0.1 6379
valkey-cli -p 6381 REPLICAOF 127.0.0.1 6379

echo "✅ Valkey setup complete:"
echo "- Primary: 127.0.0.1:6379"
echo "- Replica: 127.0.0.1:6380"
echo "- Replica: 127.0.0.1:6381"
