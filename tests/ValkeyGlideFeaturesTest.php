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
        // Clean up
        $valkey_glide?->close();
    }

    public function testConstructorWithRequestTimeoutExceeded()
    {
        // Test constructor with request timeout in milliseconds
        $addresses = [
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ];

        if (!$this->getTLS()) {
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, 10); // 10 milliseconds.
        } else {
            $advancedConfig = [
                'tls_config' => ['use_insecure_tls' => true]
            ];
            $valkey_glide = new ValkeyGlide($addresses, use_tls: true, read_from: ValkeyGlide::READ_FROM_PRIMARY, request_timeout: 10, advanced_config: $advancedConfig);
        }
        $res = $valkey_glide->rawcommand("DEBUG", "SLEEP", "2");

        // Sleep the test runner so that the server can finish the sleep command.
        sleep(2);
        // Clean up
        $valkey_glide?->close();

        // Validate that the debug command failed.
        $this->assertFalse($res);
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
        $this->assertStringContains("db=0", $valkey_glide->client("info"));
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
        $this->assertStringContains("db=1", $valkey_glide->client("info"));
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
        $this->assertStringContains("name=" . $clientName, $valkey_glide->client("info"));
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
            $valkey_glide = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, $advancedConfig);
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
                $valkey_glide_monitoring = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, null, true);
                $clients = $valkey_glide_monitoring->client('list');
                $client_count = count($clients);

                // Create the lazy connection.
                $valkey_glide_lazy = new ValkeyGlide($addresses, $this->getTLS(), null, ValkeyGlide::READ_FROM_PRIMARY, null, null, null, null, null, null, true);
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

    // Helper methods for logger testing
    private function createTempLogFile()
    {
        return sys_get_temp_dir() . '/valkey-glide-test-' . uniqid() . '.log';
    }

    private function verifyLogFileCreated($logFile)
    {
        $this->assertTrue(file_exists($logFile), "Log file should be created: $logFile");
        $this->assertTrue(is_readable($logFile), "Log file should be readable: $logFile");
        $this->assertGT(0, filesize($logFile), "Log file should contain data: $logFile");
    }

    private function verifyLogContains($logFile, $expectedMessage, $shouldContain = true)
    {
        $this->assertTrue(file_exists($logFile), "Log file must exist to verify content: $logFile");
        $content = file_get_contents($logFile);

        if ($shouldContain) {
            $this->assertStringContains($expectedMessage, $content);
        } else {
            // Since assertStringNotContains doesn't exist, we check that it's NOT found
            $this->assertTrue(strpos($content, $expectedMessage) === false, "Log should NOT contain: $expectedMessage");
        }
    }

    private function cleanupLogFile($logFile)
    {
        if (file_exists($logFile)) {
            unlink($logFile);
        }
    }

    private function findLogSuffix(string $baseFilePath): ?string
    {
        $dir = dirname($baseFilePath);
        $baseName = basename($baseFilePath);

        foreach (scandir($dir) as $file) {
            if (str_starts_with($file, $baseName) && $file !== $baseName) {
                return substr($file, strlen($baseName)); // Return the suffix
            }
        }

        return null; // No match found
    }

    public function testLoggerBasicFunctionality()
    {
        // Test comprehensive logger functionality with file output and verification
        $logFile = $this->createTempLogFile();

        try {
            // Initialize logger with info level and file output
            $this->assertTrue(valkey_glide_logger_set_config("info", $logFile));
            $this->assertTrue(valkey_glide_logger_is_initialized());
            $this->assertEquals(2, valkey_glide_logger_get_level()); // Info = 2

            // Test convenience functions at different levels
            $errorMsg = "Test error message - " . uniqid();
            $warnMsg = "Test warning message - " . uniqid();
            $infoMsg = "Test info message - " . uniqid();
            $debugMsg = "Test debug message - " . uniqid();

            valkey_glide_logger_error("basic-test", $errorMsg);
            valkey_glide_logger_warn("basic-test", $warnMsg);
            valkey_glide_logger_info("basic-test", $infoMsg);
            valkey_glide_logger_debug("basic-test", $debugMsg); // Should be filtered out

            // Test generic log function
            $genericErrorMsg = "Generic error - " . uniqid();
            $genericWarnMsg = "Generic warning - " . uniqid();
            $genericInfoMsg = "Generic info - " . uniqid();
            $genericDebugMsg = "Generic debug - " . uniqid();

            valkey_glide_logger_log("error", "basic-test", $genericErrorMsg);
            valkey_glide_logger_log("warn", "basic-test", $genericWarnMsg);
            valkey_glide_logger_log("info", "basic-test", $genericInfoMsg);
            valkey_glide_logger_log("debug", "basic-test", $genericDebugMsg); // Should be filtered out

            // Test parameter validation with special characters and edge cases
            $specialCharsMsg = "Message with émojis: 🚀 and unicode: ñáéíóú";
            $newlinesMsg = "Message with\nnewlines\nand\ttabs";
            $hierarchicalMsg = "Hierarchical identifier";
            $dotNotationMsg = "Dot notation identifier";

            valkey_glide_logger_info("special-chars", $specialCharsMsg);
            valkey_glide_logger_warn("newlines", $newlinesMsg);
            valkey_glide_logger_error("component:subcomponent", $hierarchicalMsg);
            valkey_glide_logger_error("component.method", $dotNotationMsg);

            // Test with empty parameters (should not crash)
            valkey_glide_logger_log("info", "", "Empty identifier");
            valkey_glide_logger_log("info", "test", "");

            // Give time for file writing
            usleep(200000); // 200ms
            $log_file_with_date = $logFile . $this->findLogSuffix($logFile);
            // Verify log file was created and contains data
            $this->verifyLogFileCreated($log_file_with_date);


            // Verify expected messages appear (info level and above)
            $this->verifyLogContains($log_file_with_date, $errorMsg, true);
            $this->verifyLogContains($log_file_with_date, $warnMsg, true);
            $this->verifyLogContains($log_file_with_date, $infoMsg, true);
            $this->verifyLogContains($log_file_with_date, $genericErrorMsg, true);
            $this->verifyLogContains($log_file_with_date, $genericWarnMsg, true);
            $this->verifyLogContains($log_file_with_date, $genericInfoMsg, true);
            $this->verifyLogContains($log_file_with_date, $specialCharsMsg, true);
            $this->verifyLogContains($log_file_with_date, $hierarchicalMsg, true);
            $this->verifyLogContains($log_file_with_date, $dotNotationMsg, true);

            // Verify debug messages are filtered out (should NOT appear)
            $this->verifyLogContains($log_file_with_date, $debugMsg, false);
            $this->verifyLogContains($log_file_with_date, $genericDebugMsg, false);
        } finally {
            $this->cleanupLogFile($log_file_with_date);
        }
    }

    public function testLoggerLevelFiltering()
    {
        // Test that log level filtering works correctly at info level
        $logFile = $this->createTempLogFile();

        try {
            // Initialize with info level
            $this->assertTrue(valkey_glide_logger_set_config("info", $logFile));
            $this->assertTrue(valkey_glide_logger_is_initialized());
            $this->assertEquals(2, valkey_glide_logger_get_level()); // Info = 2

            // Log messages at all levels with unique identifiers
            $errorMsg = "Error level message - " . uniqid();
            $warnMsg = "Warn level message - " . uniqid();
            $infoMsg = "Info level message - " . uniqid();
            $debugMsg = "Debug level message - " . uniqid();
            $traceMsg = "Trace level message - " . uniqid();

            valkey_glide_logger_log("error", "filter-test", $errorMsg);
            valkey_glide_logger_log("warn", "filter-test", $warnMsg);
            valkey_glide_logger_log("info", "filter-test", $infoMsg);
            valkey_glide_logger_log("debug", "filter-test", $debugMsg);
            valkey_glide_logger_log("trace", "filter-test", $traceMsg);

            // Test edge cases
            $emptyIdentifierMsg = "Empty identifier test";
            $debugEmptyMsg = "Debug with empty identifier (should be filtered)";

            valkey_glide_logger_log("info", "", $emptyIdentifierMsg);
            valkey_glide_logger_log("debug", "", $debugEmptyMsg);

            // Give time for file writing
            usleep(200000);

            // Verify file creation
            $log_file_with_date = $logFile . $this->findLogSuffix($logFile);
            $this->verifyLogFileCreated($log_file_with_date);

            // Verify info level and above appear (error=0, warn=1, info=2)
            $this->verifyLogContains($log_file_with_date, $errorMsg, true);
            $this->verifyLogContains($log_file_with_date, $warnMsg, true);
            $this->verifyLogContains($log_file_with_date, $infoMsg, true);
            $this->verifyLogContains($log_file_with_date, $emptyIdentifierMsg, true);

            // Verify below info level are filtered out (debug=3, trace=4)
            $this->verifyLogContains($log_file_with_date, $debugMsg, false);
            $this->verifyLogContains($log_file_with_date, $traceMsg, false);
            $this->verifyLogContains($log_file_with_date, $debugEmptyMsg, false);
        } finally {
            $this->cleanupLogFile($log_file_with_date);
        }
    }

    public function testLoggerWithValkeyGlideIntegration()
    {
        // Test that ValkeyGlide client integration works with logger system
        $logFile = $this->createTempLogFile();

        try {
            // Initialize logger with info level to capture client logs
            $this->assertTrue(valkey_glide_logger_set_config("info", $logFile));
            $this->assertTrue(valkey_glide_logger_is_initialized());
            $this->assertEquals(2, valkey_glide_logger_get_level());

            $addresses = [
                ['host' => $this->getHost(), 'port' => $this->getPort()]
            ];

            // Log before client creation
            $preClientMsg = "Before client creation - " . uniqid();
            valkey_glide_logger_info("integration-test", $preClientMsg);

            // Create ValkeyGlide client (may trigger internal logging)
            if (!$this->getTLS()) {
                $valkey_glide = new ValkeyGlide($addresses);
            } else {
                $advancedConfig = [
                    'tls_config' => ['use_insecure_tls' => true]
                ];
                $valkey_glide = new ValkeyGlide($addresses, use_tls: true, advanced_config: $advancedConfig);
            }

            // Verify connection works
            $this->assertTrue($valkey_glide->ping());

            // Log after successful connection
            $postConnectionMsg = "ValkeyGlide client created and connected - " . uniqid();
            valkey_glide_logger_info("integration-test", $postConnectionMsg);

            // Perform some operations that might trigger logging
            $testKey = 'logger_integration_test_' . uniqid();
            $testValue = 'test_value_' . time();

            $this->assertTrue($valkey_glide->set($testKey, $testValue));
            $this->assertEquals($testValue, $valkey_glide->get($testKey));

            // Log after operations
            $postOpsMsg = "Operations completed successfully - " . uniqid();
            valkey_glide_logger_info("integration-test", $postOpsMsg);

            // Clean up test data
            $this->assertEquals(1, $valkey_glide->del($testKey));
            $valkey_glide->close();

            // Final log message
            $finalMsg = "Integration test completed - " . uniqid();
            valkey_glide_logger_info("integration-test", $finalMsg);

            // Give time for file writing
            usleep(200000);

            // Verify file creation and content
            $log_file_with_date = $logFile . $this->findLogSuffix($logFile);
            $this->verifyLogFileCreated($log_file_with_date);

            // Verify our PHP-level log messages appear
            $this->verifyLogContains($log_file_with_date, $preClientMsg, true);
            $this->verifyLogContains($log_file_with_date, $postConnectionMsg, true);
            $this->verifyLogContains($log_file_with_date, $postOpsMsg, true);
            $this->verifyLogContains($log_file_with_date, $finalMsg, true);

            // Verify integration-test identifier appears
            $this->verifyLogContains($log_file_with_date, "integration-test", true);
        } finally {
            $this->cleanupLogFile($log_file_with_date);
        }
    }

    public function testClientCreateDeleteLoop()
    {
        // Simple test that creates and deletes ValkeyGlide clients in a loop
        $loopCount = 100;
        $successCount = 0;
        $errorCount = 0;
        $startMemory = memory_get_usage(true);

        echo "Testing client create/delete loop with {$loopCount} iterations...\n";

        for ($i = 1; $i <= $loopCount; $i++) {
            try {
                // Create a new client using the base class method
                $client = $this->newInstance();

                // Verify the client works
                $this->assertTrue($client->ping(), "Client ping failed on iteration {$i}");

                // Close the client
                $client->close();

                // Explicitly unset to help with cleanup
                unset($client);

                $successCount++;

                // Log progress every 10 iterations
                if ($i % 10 == 0) {
                    echo "Completed {$i}/{$loopCount} iterations...\n";
                }
            } catch (Exception $e) {
                $errorCount++;
                echo "Error on iteration {$i}: " . $e->getMessage() . "\n";

                // Continue with the test even if some iterations fail
                continue;
            }
        }

        $endMemory = memory_get_usage(true);
        $memoryGrowth = $endMemory - $startMemory;

        // Log final results
        echo "Create/Delete Loop Test Results:\n";
        echo "- Total iterations: {$loopCount}\n";
        echo "- Successful iterations: {$successCount}\n";
        echo "- Failed iterations: {$errorCount}\n";
        echo "- Memory growth: " . round($memoryGrowth / 1024, 2) . " KB\n";

        // Assert that most iterations were successful
        $successRate = $successCount / $loopCount;
        $this->assertTrue($successRate > 0.9, "Success rate should be > 90%, got " . round($successRate * 100, 1) . "%");

        // Warn if memory growth is significant
        if ($memoryGrowth > 5 * 1024 * 1024) { // More than 5MB
            echo "WARNING: Significant memory growth detected: " . round($memoryGrowth / 1024 / 1024, 2) . " MB\n";
        }
    }
}
