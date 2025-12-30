#!/bin/bash

# Get the directory where this script is located and use it as working directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BASE_DIR="$HOME/valkey-cluster"
PORTS=(7001 7002 7003 7004 7005 7006)
VALKEY_BIN=$(which valkey-server)
CLI_BIN=$(which valkey-cli)

if [ -z "$VALKEY_BIN" ] || [ -z "$CLI_BIN" ]; then
  echo "valkey-server or valkey-cli not found in PATH"
  exit 1
fi

# Check Valkey version for multi-database support
VALKEY_VERSION=$("$VALKEY_BIN" --version | grep -o 'v=[0-9]\+\.[0-9]\+' | cut -d'=' -f2)
MAJOR_VERSION=$(echo "$VALKEY_VERSION" | cut -d'.' -f1)

echo "Detected Valkey version: $VALKEY_VERSION"

# 1. Clean previous setup
echo "Cleaning up old cluster data..."
rm -rf "$BASE_DIR"
mkdir -p "$BASE_DIR"

# 2. Create config and data folders
for port in "${PORTS[@]}"; do
  NODE_DIR="$BASE_DIR/$port"
  mkdir -p "$NODE_DIR"

  cat > "$NODE_DIR/valkey.conf" <<EOF
port $port
cluster-enabled yes
cluster-config-file nodes.conf
cluster-node-timeout 5000
appendonly no
dbfilename dump.rdb
dir $NODE_DIR
logfile "$NODE_DIR/valkey.log"
protected-mode no
enable-debug-command yes
EOF

  # Add multi-database support for Valkey 9+
  if [ "$MAJOR_VERSION" -ge 9 ]; then
    echo "cluster-databases 16" >> "$NODE_DIR/valkey.conf"
    echo "Added multi-database support for Valkey $VALKEY_VERSION"
  fi
done

# 3. Start each node
echo "Starting Valkey nodes..."
for port in "${PORTS[@]}"; do
  "$VALKEY_BIN" "$BASE_DIR/$port/valkey.conf" &
  sleep 0.2
done

sleep 2

# 4. Create the cluster
echo "Creating cluster..."
"$CLI_BIN" --cluster create \
  127.0.0.1:7001 127.0.0.1:7002 127.0.0.1:7003 \
  127.0.0.1:7004 127.0.0.1:7005 127.0.0.1:7006 \
  --cluster-replicas 1 \
  --cluster-yes

# 5. Use cluster_manager.py to create the TLS cluster
echo "Setting up TLS cluster..."
if ../valkey-glide/utils/cluster_manager.py --tls start --prefix tls-cluster --cluster-mode -p 8001 8002 8003 8004 8005 8006; then
    echo "✅ TLS cluster started on ports 8001-8006"
else
    echo "⚠️  WARNING: TLS cluster setup failed (ports 8001-8006 may be in use), continuing without TLS cluster..."
fi

# 6. Test multi-database support if Valkey 9+
if [ "$MAJOR_VERSION" -ge 9 ]; then
    echo "Testing multi-database support in cluster..."
    if "$CLI_BIN" -c -h 127.0.0.1 -p 7001 SELECT 1 >/dev/null 2>&1; then
        echo "✅ Multi-database support confirmed (SELECT command works)"
    else
        echo "⚠️  Multi-database support not working (SELECT command failed)"
    fi
fi

echo "Cluster setup complete!"

# 6. Use cluster_manager.py to create cluster with auth
echo "Setting up auth cluster..."
if ../valkey-glide/utils/cluster_manager.py --auth dummy_password start --prefix auth-cluster --cluster-mode -p 5001 5002 5003 5004 5005 5006; then
    echo "✅ Auth cluster started on ports 5001-5006"
else
    echo "⚠️  WARNING: Auth cluster setup failed (ports 5001-5006 may be in use), continuing without auth cluster..."
fi
