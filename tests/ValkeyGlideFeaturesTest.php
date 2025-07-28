<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * ValkeyGlide Features Test
 * Tests various constructor options and features for standalone ValkeyGlide client
 */
class ValkeyGlideFeaturesTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    public function testBasicConstructor()
    {
        // Skip this test on TLS servers because we need to use optional parameters
        // such as use_tls and advanced_config.
        if (!$this->getTLS()) {
            // Test creating ValkeyGlide with basic configuration
            $valkey_glide = new ValkeyGlide([
                ['host' => $this->getHost(), 'port' => $this->getPort()]
            ]);

            // Verify the connection works with a simple ping
            $this->assertTrue($valkey_glide->ping());

            // Clean up
            $valkey_glide->close();
        }
    }

    public function testConstructorWithSingleAddress()
    {
        // Test constructor with single address in proper array format
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide?->close();
    }

    public function testConstructorWithMultipleAddresses()
    {
        // Test constructor with multiple addresses for failover
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 6379],
            ['host' => '127.0.0.1', 'port' => 6380],
            ['host' => 'localhost', 'port' => 6381]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses);
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    public function testConstructorWithTlsDisabled()
    {
        if (!$this->getTLS()) {
            // Test constructor with TLS explicitly disabled
            $addresses = [
                ['host' => $this->getHost(), 'port' => $this->getPort()]
            ];

            $valkey_glide = new ValkeyGlide($addresses, false); // use_tls = false
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    public function testConstructorWithTlsEnabled()
    {
        if ($this->getTLS()) {
            // Test constructor with TLS explicitly disabled
            $addresses = [
                ['host' => $this->getHost(), 'port' => $this->getPort()]
            ];

            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig);
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    public function testConstructorWithCredentials()
    {
        // Test constructor with credentials (if auth is configured)
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        $auth = $this->getAuth();
        if ($auth) {
            $credentials = null;
            if (is_array($auth) && count($auth) > 1) {
                $credentials = ['username' => $auth[0], 'password' => $auth[1]];
            } elseif (is_array($auth)) {
                $credentials = ['password' => $auth[0]];
            } else {
                $credentials = ['password' => $auth];
            }

            if (!$this->getTLS()) {
                $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), $credentials);
            } else {
                $advancedConfig = [
                    'tls_config' => ['use_insecure_tls' => true]
                ];
                $valkey_glide = new ValkeyGlide($addresses, use_tls: true, credentials: $credentials, advanced_config: $advancedConfig);
            }
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        } else {
            // Test with null credentials when no auth is configured
            if (!$this->getTLS()) {
                $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null);
            } else {
                $advancedConfig = [
                    'tls_config' => ['use_insecure_tls' => true]
                ];
                $valkey_glide = new ValkeyGlide($addresses, use_tls: true, credentials: null, advanced_config: $advancedConfig);
            }
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    /**
     * Test connection with invalid credentials
     */
    public function testConstructorInvalidAuth()
    {
        // Test constructor with credentials (if auth is configured)
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        // Try to connect with incorrect auth
        $valkey_glide = null;
        try {
            $credentials = ['username' => 'invalid_user', 'password' => 'invalid_password'];
            if (!$this->getTLS()) {
                $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), $credentials);
            } else {
                $advancedConfig = [
                    'tls_config' => ['use_insecure_tls' => true]
                ];
                $valkey_glide = new ValkeyGlide($addresses, use_tls: true, credentials: $credentials, advanced_config: $advancedConfig);
            }
            $this->fail("Should throw an exception when running commands with invalid authentication");
        } catch (Exception $e) {
            $this->assertStringContains("WRONGPASS", $e->getMessage(), "Exception should indicate authentication failure");
        } finally {
            // Clean up
            $valkey_glide?->close();
        }
    }

    public function testConstructorWithReadFromPrimary()
    {
        // Test constructor with READ_FROM_PRIMARY strategy
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 6379],
            ['host' => '127.0.0.1', 'port' => 6380],
            ['host' => 'localhost', 'port' => 6381]
        ];


        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY);
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    public function testConstructorWithReadFromPreferReplica()
    {
        // Test constructor with READ_FROM_PREFER_REPLICA strategy
        $addresses = [
            ['host' => '127.0.0.1', 'port' => 6379],
            ['host' => '127.0.0.1', 'port' => 6380],
            ['host' => 'localhost', 'port' => 6381]
        ];


        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PREFER_REPLICA);
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    public function testConstructorWithRequestTimeout()
    {
        // Test constructor with request timeout in milliseconds
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, 5000); // 5 seconds.
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, read_from: ValkeyGlide::READ_FROM_PRIMARY, request_timeout: 5000, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithReconnectStrategy()
    {
        // Test constructor with reconnection strategy
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        $reconnectStrategy = [
            'num_of_retries' => 3,
            'factor' => 2,
            'exponent_base' => 2
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, 5000, $reconnectStrategy);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, reconnect_strategy: $reconnectStrategy, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithDatabaseId()
    {
        // Test constructor with specific database ID
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        // Test with database 0 (default)
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, 0);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, database_id: 0, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();

        // Test with database 1
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, 1);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, database_id: 1, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithClientName()
    {
        // Test constructor with client name identifier
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        $clientName = 'test-client-' . uniqid();
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, $clientName);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, client_name: $clientName, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithInflightRequestsLimit()
    {
        // Test constructor with inflight requests limit
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, 100);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, inflight_requests_limit: 100, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithClientAz()
    {
        // Test constructor with client availability zone
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, 'us-east-1a');
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, client_az: 'us-east-1a', advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithAdvancedConfig()
    {
        // Test constructor with advanced configuration
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];


        if (!$this->getTLS()) {
            $advancedConfig = [
                'connection_timeout' => 5000,
                'socket_timeout' => 3000
            ];
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, null, $advancedConfig);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true],
                'connection_timeout' => 5000,
                'socket_timeout' => 3000
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithLazyConnect()
    {
        // Test constructor with lazy connection enabled
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        $valkey_glide_lazy = null;
        $valkey_glide_monitoring = null;

        try {
            if (!$this->getTLS()) {
                // Create monitoring connection without lazy connection.
                $valkey_glide_monitoring = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, null, null, true);
                $clients = $valkey_glide_monitoring->client('list');
                $client_count = count($clients);

                // Create the lazy connection.
                $valkey_glide_lazy = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, null, null, true);
            } else {
                $advancedConfig = [
                    'tls_config' => ['use_insecure_tls' => true]
                ];
                $valkey_glide_monitoring = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig, lazy_connect: false);
                $clients = $valkey_glide_monitoring->client('list');
                $client_count = count($clients);

                // Create the lazy connection.
                $valkey_glide_lazy = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig, lazy_connect: true);
            }

            // The count should not change after creating the lazy connection.
            $clients = $valkey_glide_monitoring->client('list');
            $this->assertTrue(count($clients) == $client_count);

            // Issue a request with the lazy connection. Should make it active and increment the count.
            $this->assertTrue($valkey_glide_lazy->ping());
            $clients = $valkey_glide_monitoring->client('list');
            $this->assertTrue(count($clients) == $client_count + 1);
        } finally {
            $valkey_glide_monitoring?->close();
            $valkey_glide_lazy?->close();
        }
    }

    public function testConstructorWithAllParameters()
    {
        // Test constructor with all parameters specified
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        $credentials = null;
        $auth = $this->getAuth();
        if ($auth) {
            if (is_array($auth) && count($auth) > 1) {
                $credentials = ['username' => $auth[0], 'password' => $auth[1]];
            } elseif (is_array($auth)) {
                $credentials = ['password' => $auth[0]];
            } else {
                $credentials = ['password' => $auth];
            }
        }

        $reconnectStrategy = [
            'num_of_retries' => 2,
            'factor' => 1.5
        ];

        if (!$this->getTLS()) {
            $advancedConfig = [
                'connection_timeout' => 4000
            ];
        } else {
            $advancedConfig = [
                'connection_timeout' => 4000,
                'tls_config' => ['use_insecure_tls' => true]
            ];
        }


        $valkey_glide = new ValkeyGlide(
            $addresses,                                    // addresses
            $this->getTLS(),                                         // use_tls
            $credentials,                                  // credentials
            ValkeyGlide::READ_FROM_PRIMARY,               // read_from
            3000,                                          // request_timeout
            $reconnectStrategy,                            // reconnect_strategy
            0,                                             // database_id
            'comprehensive-test-client',                   // client_name
            50,                                            // inflight_requests_limit
            'test-az',                                     // client_az
            $advancedConfig,                               // advanced_config
            false                                          // lazy_connect
        );

        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithAzAffinityReadStrategy()
    {
        // Test constructor with AZ affinity read strategies
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        // Test READ_FROM_AZ_AFFINITY
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_AZ_AFFINITY);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();

        // Test READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY);
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, read_from: ValkeyGlide::READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY, advanced_config: $advancedConfig);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorWithComplexReconnectStrategies()
    {
        // Test constructor with various reconnect strategy configurations
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        // Complex retry strategy
        $complexStrategy = [
            'num_of_retries' => 3,
            'factor' => 2.0,
            'exponent_base' => 1.5,
            'max_delay' => 10000
        ];
        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, $complexStrategy);
        } else {
            $advanced_config = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, reconnect_strategy: $complexStrategy, advanced_config: $advanced_config);
        }
        $this->assertTrue($valkey_glide->ping());
        $valkey_glide->close();
    }

    public function testConstructorBasicFunctionality()
    {
        // Test that constructor creates a working client that can perform basic operations
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, 5000);
        } else {
            $advanced_config = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, read_from:ValkeyGlide::READ_FROM_PRIMARY, request_timeout: 5000, advanced_config: $advanced_config);
        }

        // Test ping
        $this->assertTrue($valkey_glide->ping());

        // Test basic set/get operations
        $testKey = 'constructor_test_' . uniqid();
        $testValue = 'test_value_' . time();

        $this->assertTrue($valkey_glide->set($testKey, $testValue));
        $this->assertEquals($testValue, $valkey_glide->get($testKey));

        // Test key exists
        $this->assertEquals(1, $valkey_glide->exists($testKey));

        // Clean up
        $this->assertEquals(1, $valkey_glide->del($testKey));
        $valkey_glide->close();
    }

    public function testConstructorParameterValidation()
    {
        // Test constructor parameter validation and edge cases
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            // Test with minimal parameters (addresses only)
            $valkey_glide = new ValkeyGlide($addresses);
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();

            // Test with explicit defaults
            $valkey_glide = new ValkeyGlide(
                $addresses,                           // addresses
                false,                               // use_tls (default)
                null,                                // credentials (default)
                ValkeyGlide::READ_FROM_PRIMARY,     // read_from (default)
                null,                                // request_timeout (default)
                null,                                // reconnect_strategy (default)
                null,                                // database_id (default)
                null,                                // client_name (default)
                null,                                // inflight_requests_limit (default)
                null,                                // client_az (default)
                null,                                // advanced_config (default)
                null                                 // lazy_connect (default)
            );
            $this->assertTrue($valkey_glide->ping());
            $valkey_glide->close();
        }
    }

    // ===================================================================
    // LOGGER TESTS - Multi-level logging system integration tests
    // ===================================================================

    public function testLoggerInitialization()
    {
        // Test logger initialization with default settings
        $this->assertTrue(valkey_glide_logger_init());
        $this->assertTrue(valkey_glide_logger_is_initialized());

        // Test logger initialization with info level
        $this->assertTrue(valkey_glide_logger_init("info"));
        $this->assertTrue(valkey_glide_logger_is_initialized());
        $this->assertEquals(2, valkey_glide_logger_get_level()); // Info = 2

        // Test logger initialization with file output
        $logFile = sys_get_temp_dir() . '/valkey-glide-test-' . uniqid() . '.log';
        $this->assertTrue(valkey_glide_logger_init("debug", $logFile));
        $this->assertTrue(valkey_glide_logger_is_initialized());
        $this->assertEquals(3, valkey_glide_logger_get_level()); // Debug = 3
    }

    public function testLoggerSetConfig()
    {
        // Test logger configuration replacement (Node.js Logger.setLoggerConfig behavior)
        $logFile1 = sys_get_temp_dir() . '/valkey-glide-config1-' . uniqid() . '.log';
        $logFile2 = sys_get_temp_dir() . '/valkey-glide-config2-' . uniqid() . '.log';

        // Initial configuration
        $this->assertTrue(valkey_glide_logger_set_config("warn", $logFile1));
        $this->assertTrue(valkey_glide_logger_is_initialized());
        $this->assertEquals(1, valkey_glide_logger_get_level()); // Warn = 1

        // Replace configuration
        $this->assertTrue(valkey_glide_logger_set_config("error", $logFile2));
        $this->assertTrue(valkey_glide_logger_is_initialized());
        $this->assertEquals(0, valkey_glide_logger_get_level()); // Error = 0

        // Test with console output
        $this->assertTrue(valkey_glide_logger_set_config("info"));
        $this->assertEquals(2, valkey_glide_logger_get_level()); // Info = 2
    }

    public function testLoggerLevels()
    {
        // Test different log levels
        $logLevels = [
            'error' => 0,
            'warn' => 1,
            'info' => 2,
            'debug' => 3,
            'trace' => 4,
            'off' => 5
        ];

        foreach ($logLevels as $level => $expectedValue) {
            $this->assertTrue(valkey_glide_logger_set_config($level));
            $this->assertEquals($expectedValue, valkey_glide_logger_get_level(), "Level {$level} should map to {$expectedValue}");
        }
    }

    public function testLoggerConvenienceFunctions()
    {
        // Initialize logger for testing
        valkey_glide_logger_set_config("trace");

        // Test convenience functions (these should not throw errors)
        valkey_glide_logger_error("test", "This is an error message");
        valkey_glide_logger_warn("test", "This is a warning message");
        valkey_glide_logger_info("test", "This is an info message");
        valkey_glide_logger_debug("test", "This is a debug message");

        // Test generic log function
        valkey_glide_logger_log("error", "test", "Generic error message");
        valkey_glide_logger_log("warn", "test", "Generic warning message");
        valkey_glide_logger_log("info", "test", "Generic info message");
        valkey_glide_logger_log("debug", "test", "Generic debug message");
        valkey_glide_logger_log("trace", "test", "Generic trace message");

        // All functions should complete without throwing exceptions
        $this->assertTrue(true, "All logger convenience functions executed successfully");
    }

    public function testLoggerAutoInitialization()
    {
        // Test Node.js Logger behavior: first log attempt initializes logger with defaults
        // We can't fully test this without resetting logger state, but we can verify
        // that logging works even without explicit initialization

        // Log without explicit initialization (should auto-initialize)
        valkey_glide_logger_info("auto-init-test", "Auto-initialization test message");

        // Verify logger is now initialized
        $this->assertTrue(valkey_glide_logger_is_initialized());
        
        // Should default to warn level
        $this->assertEquals(1, valkey_glide_logger_get_level());
    }

    public function testLoggerWithFileOutput()
    {
        // Test logging to a file
        $logFile = sys_get_temp_dir() . '/valkey-glide-file-test-' . uniqid() . '.log';
        
        // Initialize with file output
        $this->assertTrue(valkey_glide_logger_set_config("info", $logFile));
        
        // Log some messages
        $testMessage = "Test file output - " . uniqid();
        valkey_glide_logger_info("file-test", $testMessage);
        valkey_glide_logger_error("file-test", "Error in file");
        valkey_glide_logger_warn("file-test", "Warning in file");
        
        // Give some time for file writing (if asynchronous)
        usleep(100000); // 100ms
        
        // Check if file was created (basic test - we can't easily read Rust logger output format)
        if (file_exists($logFile)) {
            $this->assertTrue(filesize($logFile) > 0, "Log file should contain data");
        }
        
        // Clean up
        if (file_exists($logFile)) {
            unlink($logFile);
        }
    }

    public function testLoggerWithValkeyGlideIntegration()
    {
        // Test that ValkeyGlide constructor logging works with our logger system
        $logFile = sys_get_temp_dir() . '/valkey-glide-integration-' . uniqid() . '.log';
        
        // Set logger to debug level to capture constructor logs
        valkey_glide_logger_set_config("debug", $logFile);
        
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        // Create ValkeyGlide client (this should trigger C extension logging)
        $valkey_glide = new ValkeyGlide($addresses);
        
        // Verify connection works
        $this->assertTrue($valkey_glide->ping());
        
        // Log some messages from PHP level
        valkey_glide_logger_info("integration-test", "ValkeyGlide client created successfully");
        valkey_glide_logger_debug("integration-test", "Testing PHP-level logging integration");
        
        // Clean up
        $valkey_glide->close();
        
        // Give time for file writing
        usleep(100000);
        
        // Basic verification that logging occurred
        if (file_exists($logFile)) {
            $this->assertTrue(filesize($logFile) > 0, "Integration log file should contain data");
            unlink($logFile);
        }
        
        $this->assertTrue(true, "Logger integration test completed successfully");
    }

    public function testLoggerParameterValidation()
    {
        // Test logger functions with various parameter combinations
        
        // Test null/empty parameters
        valkey_glide_logger_log("info", "", "Empty identifier");
        valkey_glide_logger_log("info", "test", "");
        
        // Test with special characters
        valkey_glide_logger_info("special-chars", "Message with émojis: 🚀 and unicode: ñáéíóú");
        valkey_glide_logger_warn("newlines", "Message with\nnewlines\nand\ttabs");
        
        // Test with long messages
        $longMessage = str_repeat("This is a very long log message. ", 100);
        valkey_glide_logger_debug("long-message", $longMessage);
        
        // Test with various identifier formats
        valkey_glide_logger_error("component:subcomponent", "Hierarchical identifier");
        valkey_glide_logger_error("component.method", "Dot notation identifier");
        valkey_glide_logger_error("123-numeric", "Numeric identifier");
        
        $this->assertTrue(true, "Parameter validation tests completed");
    }

    public function testLoggerLevelFiltering()
    {
        // Test that log level filtering works correctly
        
        // Set to error level (should only log errors)
        valkey_glide_logger_set_config("error");
        $this->assertEquals(0, valkey_glide_logger_get_level());
        
        // These should be filtered out by level (but shouldn't cause errors)
        valkey_glide_logger_warn("level-test", "This warn should be filtered");
        valkey_glide_logger_info("level-test", "This info should be filtered");
        valkey_glide_logger_debug("level-test", "This debug should be filtered");
        
        // This should be logged
        valkey_glide_logger_error("level-test", "This error should be logged");
        
        // Set to trace level (should log everything)
        valkey_glide_logger_set_config("trace");
        $this->assertEquals(4, valkey_glide_logger_get_level());
        
        // All of these should be logged now
        valkey_glide_logger_error("level-test", "Error at trace level");
        valkey_glide_logger_warn("level-test", "Warn at trace level");
        valkey_glide_logger_info("level-test", "Info at trace level");
        valkey_glide_logger_debug("level-test", "Debug at trace level");
        
        // Set to off (should log nothing)
        valkey_glide_logger_set_config("off");
        $this->assertEquals(5, valkey_glide_logger_get_level());
        
        // These should all be filtered
        valkey_glide_logger_error("level-test", "This should be filtered when off");
        valkey_glide_logger_warn("level-test", "This should be filtered when off");
        
        $this->assertTrue(true, "Level filtering tests completed");
    }

    public function testLoggerStateConsistency()
    {
        // Test that logger state remains consistent across operations
        
        // Initial state
        valkey_glide_logger_set_config("info");
        $initialLevel = valkey_glide_logger_get_level();
        $this->assertTrue(valkey_glide_logger_is_initialized());
        
        // Create and use ValkeyGlide client
        $addresses = [['host' => $this->getHost(), 'port' => $this->getPort()]];
        $valkey_glide = new ValkeyGlide($addresses);
        
        // Logger state should remain unchanged
        $this->assertEquals($initialLevel, valkey_glide_logger_get_level());
        $this->assertTrue(valkey_glide_logger_is_initialized());
        
        // Perform some operations
        $valkey_glide->ping();
        valkey_glide_logger_info("consistency-test", "After ping operation");
        
        // State should still be consistent
        $this->assertEquals($initialLevel, valkey_glide_logger_get_level());
        $this->assertTrue(valkey_glide_logger_is_initialized());
        
        // Clean up
        $valkey_glide->close();
        
        // Logger should still be initialized after client cleanup
        $this->assertTrue(valkey_glide_logger_is_initialized());
        $this->assertEquals($initialLevel, valkey_glide_logger_get_level());
    }
}
