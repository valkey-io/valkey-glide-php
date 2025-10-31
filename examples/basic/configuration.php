<?php

/**
 * Configuration Options Example
 *
 * This example demonstrates various configuration options available
 * for both standalone and cluster Valkey GLIDE clients.
 */

// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Check if extension is loaded
if (!extension_loaded('valkey_glide')) {
    echo "❌ Valkey GLIDE extension is not loaded!\n";
    exit(1);
}

echo "🚀 Valkey GLIDE PHP - Configuration Examples\n";
echo "==========================================\n\n";

// =============================================================================
// BASIC CONFIGURATION
// =============================================================================
echo "📋 Basic Configuration:\n";
echo "----------------------\n";

// Minimal configuration - just host and port
$basicAddresses = [
    ['host' => 'localhost', 'port' => 6379]
];

try {
    echo "Creating client with minimal configuration...\n";
    $basicClient = new ValkeyGlide($basicAddresses);
    echo "✅ Basic client created successfully\n";
    $basicClient->ping();
    $basicClient->close();
} catch (Exception $e) {
    echo "❌ Basic client failed: " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// ADVANCED STANDALONE CONFIGURATION
// =============================================================================
echo "🔧 Advanced Standalone Configuration:\n";
echo "------------------------------------\n";

$standaloneAddresses = [
    ['host' => 'localhost', 'port' => 6379]
];

// All configuration options for standalone client
$use_tls = false;
$credentials = null; // ['username' => 'user', 'password' => 'pass']
$read_from = 0; // 0=PRIMARY, 1=PREFER_REPLICA, 2=AZ_AFFINITY
$request_timeout = 2000; // 2 seconds in milliseconds
$reconnect_strategy = [
    'num_of_retries' => 3,
    'factor' => 2.0,
    'exponent_base' => 2
];
$database_id = 0; // Database number (0 or higher for standalone)
$client_name = 'valkey-glide-example';
$inflight_requests_limit = 250;
$client_az = null; // Availability zone for AZ_AFFINITY reads
$advanced_config = [
    'connection_timeout' => 5000, // Connection timeout in milliseconds
    'socket_timeout' => 3000      // Socket timeout in milliseconds
];
$lazy_connect = false; // Whether to connect lazily

try {
    echo "Creating advanced standalone client...\n";
    $advancedClient = new ValkeyGlide(
        $standaloneAddresses,
        $use_tls,
        $credentials,
        $read_from,
        $request_timeout,
        $reconnect_strategy,
        $database_id,
        $client_name,
        $inflight_requests_limit,
        $client_az,
        $advanced_config,
        $lazy_connect
    );

    echo "✅ Advanced standalone client created successfully\n";

    // Show client info
    $info = $advancedClient->client_info();
    echo "Client info: " . json_encode($info) . "\n";

    $advancedClient->ping();
    $advancedClient->close();
} catch (Exception $e) {
    echo "❌ Advanced standalone client failed: " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// CLUSTER CONFIGURATION
// =============================================================================
echo "🌐 Cluster Configuration:\n";
echo "------------------------\n";

$clusterAddresses = [
    ['host' => 'localhost', 'port' => 7001],
    ['host' => 'localhost', 'port' => 7002],
    ['host' => 'localhost', 'port' => 7003]
];

try {
    echo "Creating cluster client with advanced options...\n";
    $clusterClient = new ValkeyGlideCluster(
        $clusterAddresses,
        false,                    // use_tls
        null,                     // credentials
        0,                        // read_from (PRIMARY)
        3000,                     // request_timeout
        [                         // reconnect_strategy
            'num_of_retries' => 5,
            'factor' => 1.5,
            'exponent_base' => 2
        ],
        null,                     // database_id (not used in cluster)
        'cluster-example-client', // client_name
        500,                      // inflight_requests_limit
        null,                     // client_az
        [                         // advanced_config
            'connection_timeout' => 10000,
            'socket_timeout' => 5000
        ],
        false                     // lazy_connect
    );

    echo "✅ Cluster client created successfully\n";
    $clusterClient->ping();
    $clusterClient->close();
} catch (Exception $e) {
    echo "❌ Cluster client failed (this is expected if no cluster is running): " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// CONFIGURATION WITH AUTHENTICATION
// =============================================================================
echo "🔐 Authentication Configuration:\n";
echo "-------------------------------\n";

// Example with username/password (for servers that support ACL)
$authAddresses = [['host' => 'localhost', 'port' => 6379]];

echo "Creating client with authentication (will fail if no auth configured)...\n";
try {
    $authClient = new ValkeyGlide(
        $authAddresses,
        false,                    // use_tls
        [                         // credentials
            'username' => 'default',
            'password' => 'your-password-here'
        ],
        0,                        // read_from
        5000                      // request_timeout
    );

    echo "✅ Authenticated client created\n";
    $authClient->ping();
    $authClient->close();
} catch (Exception $e) {
    echo "❌ Authentication failed (expected): " . $e->getMessage() . "\n";
}

// Password-only authentication (older Redis/Valkey style)
try {
    $passwordClient = new ValkeyGlide(
        $authAddresses,
        false,                    // use_tls
        ['password' => 'your-password-here'], // password only
        0,                        // read_from
        5000                      // request_timeout
    );

    echo "✅ Password-only client created\n";
    $passwordClient->close();
} catch (Exception $e) {
    echo "❌ Password authentication failed (expected): " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// IAM AUTHENTICATION (AWS ElastiCache/MemoryDB)
// =============================================================================
echo "☁️  IAM Authentication Configuration (AWS):\n";
echo "-------------------------------------------\n";

// IAM authentication for AWS ElastiCache
echo "Creating client with IAM authentication for ElastiCache...\n";
try {
    $iamClient = new ValkeyGlide(
        [['host' => 'my-cluster.xxxxx.use1.cache.amazonaws.com', 'port' => 6379]],
        true,                     // use_tls (REQUIRED for IAM)
        [                         // credentials
            'username' => 'my-iam-user',  // REQUIRED for IAM
            'iamConfig' => [
                ValkeyGlide::IAM_CONFIG_CLUSTER_NAME => 'my-cluster',
                ValkeyGlide::IAM_CONFIG_REGION => 'us-east-1',
                ValkeyGlide::IAM_CONFIG_SERVICE => ValkeyGlide::IAM_SERVICE_ELASTICACHE,
                ValkeyGlide::IAM_CONFIG_REFRESH_INTERVAL => 300  // Optional
            ]
        ],
        0,                        // read_from
        5000                      // request_timeout
    );

    echo "✅ IAM ElastiCache client created\n";
    $iamClient->ping();
    $iamClient->close();
} catch (Exception $e) {
    echo "❌ IAM ElastiCache authentication failed (expected if not on AWS): " . $e->getMessage() . "\n";
}

// IAM authentication for AWS MemoryDB
echo "Creating client with IAM authentication for MemoryDB...\n";
try {
    $memorydbClient = new ValkeyGlide(
        [['host' => 'clustercfg.my-memorydb.xxxxx.memorydb.us-east-1.amazonaws.com', 'port' => 6379]],
        true,                     // use_tls (REQUIRED for IAM)
        [                         // credentials
            'username' => 'my-iam-user',
            'iamConfig' => [
                ValkeyGlide::IAM_CONFIG_CLUSTER_NAME => 'my-memorydb',
                ValkeyGlide::IAM_CONFIG_REGION => 'us-east-1',
                ValkeyGlide::IAM_CONFIG_SERVICE => ValkeyGlide::IAM_SERVICE_MEMORYDB,
                ValkeyGlide::IAM_CONFIG_REFRESH_INTERVAL => 120  // Refresh every 2 minutes
            ]
        ]
    );

    echo "✅ IAM MemoryDB client created\n";
    $memorydbClient->ping();
    $memorydbClient->close();
} catch (Exception $e) {
    echo "❌ IAM MemoryDB authentication failed (expected if not on AWS): " . $e->getMessage() . "\n";
}

echo "\nℹ️  IAM Authentication Notes:\n";
echo "  - Requires TLS to be enabled (use_tls: true)\n";
echo "  - Username is REQUIRED for IAM authentication\n";
echo "  - AWS credentials must be configured (IAM role, env vars, or credentials file)\n";
echo "  - Tokens are automatically refreshed in the background\n";
echo "  - Default refresh interval is 300 seconds (5 minutes)\n";
echo "  - IAM permissions required: elasticache:Connect or memorydb:Connect\n";
echo "\n";

// =============================================================================
// TLS CONFIGURATION
// =============================================================================
echo "🔒 TLS Configuration:\n";
echo "--------------------\n";

$tlsAddresses = [['host' => 'localhost', 'port' => 6380]]; // Common TLS port

echo "Creating client with TLS (will fail if no TLS server)...\n";
try {
    $tlsClient = new ValkeyGlide(
        $tlsAddresses,
        true,                     // use_tls = true
        null,                     // credentials
        0,                        // read_from
        5000                      // request_timeout
    );

    echo "✅ TLS client created\n";
    $tlsClient->ping();
    $tlsClient->close();
} catch (Exception $e) {
    echo "❌ TLS connection failed (expected): " . $e->getMessage() . "\n";
}
echo "\n";

// =============================================================================
// READ PREFERENCE EXAMPLES
// =============================================================================
echo "📖 Read Preference Configuration:\n";
echo "--------------------------------\n";

// Read preferences for replica reads
$readPreferences = [
    0 => 'PRIMARY - Read from primary only',
    1 => 'PREFER_REPLICA - Prefer replicas, fallback to primary',
    2 => 'AZ_AFFINITY - Read from same availability zone'
];

foreach ($readPreferences as $readFrom => $description) {
    echo "Read preference {$readFrom}: {$description}\n";

    try {
        $readClient = new ValkeyGlide(
            $standaloneAddresses,
            false,                // use_tls
            null,                 // credentials
            $readFrom,            // read_from preference
            2000                  // request_timeout
        );

        echo "  ✅ Client created with read preference {$readFrom}\n";
        $readClient->close();
    } catch (Exception $e) {
        echo "  ❌ Read preference {$readFrom} failed: " . $e->getMessage() . "\n";
    }
}
echo "\n";

// =============================================================================
// TIMEOUT CONFIGURATION
// =============================================================================
echo "⏱️  Timeout Configuration Examples:\n";
echo "----------------------------------\n";

$timeoutExamples = [
    1000 => '1 second',
    5000 => '5 seconds',
    10000 => '10 seconds',
    30000 => '30 seconds'
];

foreach ($timeoutExamples as $timeout => $description) {
    echo "Creating client with {$description} timeout...\n";

    try {
        $timeoutClient = new ValkeyGlide(
            $standaloneAddresses,
            false,                // use_tls
            null,                 // credentials
            0,                    // read_from
            $timeout              // request_timeout
        );

        echo "  ✅ Client created with {$timeout}ms timeout\n";

        // Test with a quick operation
        $start = microtime(true);
        $timeoutClient->ping();
        $duration = (microtime(true) - $start) * 1000;
        echo "  PING took {$duration:.2f}ms\n";

        $timeoutClient->close();
    } catch (Exception $e) {
        echo "  ❌ Timeout {$timeout} failed: " . $e->getMessage() . "\n";
    }
}
echo "\n";

// =============================================================================
// RECONNECTION STRATEGY
// =============================================================================
echo "🔄 Reconnection Strategy Examples:\n";
echo "----------------------------------\n";

$reconnectStrategies = [
    'conservative' => [
        'num_of_retries' => 2,
        'factor' => 1.5,
        'exponent_base' => 2
    ],
    'aggressive' => [
        'num_of_retries' => 5,
        'factor' => 2.0,
        'exponent_base' => 2
    ],
    'patient' => [
        'num_of_retries' => 10,
        'factor' => 1.2,
        'exponent_base' => 1.5
    ]
];

foreach ($reconnectStrategies as $name => $strategy) {
    echo "Reconnection strategy '{$name}':\n";
    echo "  Retries: {$strategy['num_of_retries']}\n";
    echo "  Factor: {$strategy['factor']}\n";
    echo "  Exponent base: {$strategy['exponent_base']}\n";

    try {
        $reconnectClient = new ValkeyGlide(
            $standaloneAddresses,
            false,                // use_tls
            null,                 // credentials
            0,                    // read_from
            5000,                 // request_timeout
            $strategy             // reconnect_strategy
        );

        echo "  ✅ Client created with '{$name}' reconnection strategy\n";
        $reconnectClient->close();
    } catch (Exception $e) {
        echo "  ❌ Strategy '{$name}' failed: " . $e->getMessage() . "\n";
    }
    echo "\n";
}

// =============================================================================
// ENVIRONMENT-BASED CONFIGURATION
// =============================================================================
echo "🌍 Environment-based Configuration:\n";
echo "----------------------------------\n";

echo "Configuration via environment variables:\n";

// Show how to use environment variables
$envConfig = [
    'VALKEY_HOST' => getenv('VALKEY_HOST') ?: 'localhost',
    'VALKEY_PORT' => getenv('VALKEY_PORT') ?: '6379',
    'VALKEY_PASSWORD' => getenv('VALKEY_PASSWORD') ?: '',
    'VALKEY_USE_TLS' => getenv('VALKEY_USE_TLS') ?: 'false',
    'VALKEY_TIMEOUT' => getenv('VALKEY_TIMEOUT') ?: '5000',
    'VALKEY_DATABASE' => getenv('VALKEY_DATABASE') ?: '0'
];

foreach ($envConfig as $var => $value) {
    echo "  {$var} = '{$value}'\n";
}

// Create client from environment
$envAddresses = [['host' => $envConfig['VALKEY_HOST'], 'port' => (int)$envConfig['VALKEY_PORT']]];
$envUseTls = filter_var($envConfig['VALKEY_USE_TLS'], FILTER_VALIDATE_BOOLEAN);
$envCredentials = !empty($envConfig['VALKEY_PASSWORD']) ? ['password' => $envConfig['VALKEY_PASSWORD']] : null;
$envTimeout = (int)$envConfig['VALKEY_TIMEOUT'];
$envDatabase = (int)$envConfig['VALKEY_DATABASE'];

try {
    echo "\nCreating client from environment configuration...\n";
    $envClient = new ValkeyGlide(
        $envAddresses,
        $envUseTls,
        $envCredentials,
        0,                        // read_from
        $envTimeout,              // request_timeout
        null,                     // reconnect_strategy (default)
        $envDatabase              // database_id
    );

    echo "✅ Environment-based client created successfully\n";
    $envClient->ping();
    $envClient->close();
} catch (Exception $e) {
    echo "❌ Environment-based client failed: " . $e->getMessage() . "\n";
}

echo "\n📚 Configuration Best Practices:\n";
echo "-------------------------------\n";
echo "1. Use environment variables for deployment flexibility\n";
echo "2. Set appropriate timeouts based on your network latency\n";
echo "3. Configure reconnection strategy based on your availability needs\n";
echo "4. Use TLS in production environments\n";
echo "5. Use IAM authentication for AWS ElastiCache/MemoryDB (more secure than passwords)\n";
echo "6. Set client names for easier debugging and monitoring\n";
echo "7. Consider read preferences when using replicas\n";
echo "8. Limit in-flight requests to prevent memory issues\n";
echo "9. Use lazy connection for applications with conditional Redis usage\n";

echo "\n✅ Configuration examples completed!\n";
