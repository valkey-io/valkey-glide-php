#!/bin/bash

set -e

# Get the directory where this script is located and use it as working directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

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

# Handle TLS setup with improved debugging and verification
echo "Setting up TLS standalone server on port 6400..."
echo "DEBUG: Current directory: $(pwd)"
echo "DEBUG: Checking for cluster_manager.py..."
ls -la ../valkey-glide/utils/cluster_manager.py 2>&1 | head -2
echo "DEBUG: Checking for TLS certs..."
ls -la ../valkey-glide/utils/tls_crts/ 2>&1 | head -5

if python3 ../valkey-glide/utils/cluster_manager.py --tls start --prefix tls-standalone -p 6400 -r 0; then
    echo "✅ TLS standalone server start command succeeded"
    
    # Verify TLS server is actually responding
    echo "Verifying TLS server connectivity on port 6400..."
    TLS_READY=false
    for i in {1..10}; do
        if nc -z 127.0.0.1 6400 2>/dev/null; then
            echo "✅ TLS server verified and responding on port 6400"
            TLS_READY=true
            break
        fi
        echo "Waiting for TLS server... attempt $i/10"
        sleep 1
    done
    
    if [ "$TLS_READY" = false ]; then
        echo "⚠️  WARNING: TLS server started but not responding on port 6400"
    fi
else
    echo "⚠️  WARNING: TLS standalone setup command failed"
    echo "TLS tests will likely fail. Check cluster_manager.py output above for errors."
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
