<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideClusterBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

class AddressResolverClusterTest extends ValkeyGlideClusterBaseTest
{
    public function setUp(): void
    {
        // Intentionally do not call parent::setUp(); each test creates its own client.
    }

    public function tearDown()
    {
        // No shared client to close.
    }

    private function newClusterClient(array $addresses, ?callable $resolver): ValkeyGlideCluster
    {
        return new ValkeyGlideCluster(
            addresses: $addresses,
            address_resolver: $resolver,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED,
        );
    }

    /**
     * Return the first reachable cluster seed address, probing available seeds.
     *
     * @return array{host: string, port: int}
     */
    private function getReachableClusterAddress(): array
    {
        $addresses = $this->getClusterAddresses();
        foreach ($addresses as $addr) {
            $fp = @fsockopen($addr['host'], $addr['port'], $errno, $errstr, 0.5);
            if ($fp) {
                fclose($fp);
                return $addr;
            }
        }
        return $addresses[0] ?? ['host' => $this->getHost(), 'port' => $this->getPort()];
    }

    public function testAddressResolverWithFakeAddress()
    {
        $this->skipIfTlsEnabled();

        $primaryAddress = $this->getReachableClusterAddress();
        $realHost = $primaryAddress['host'];
        $realPort = $primaryAddress['port'];

        $resolver = function (string $host, int $port) use ($realHost, $realPort): array {
            return ['host' => $realHost, 'port' => $realPort];
        };

        $client = $this->newClusterClient(
            addresses: [['host' => 'fake.nonexistent.host', 'port' => 9999]],
            resolver: $resolver,
        );

        try {
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    public function testAddressResolverExceptionFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): array {
            $called = true;
            throw new RuntimeException("resolver error");
        };

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: $resolver,
        );

        try {
            $this->assertTrue($called);
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    public function testAddressResolverReturnsInvalidFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): mixed {
            $called = true;
            return null;
        };

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: $resolver,
        );

        try {
            $this->assertTrue($called);
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    /**
     * Two cluster clients with different resolvers don't interfere with each other.
     */
    public function testMultiClientResolversAreIsolated()
    {
        $this->skipIfTlsEnabled();

        $primaryAddress = $this->getReachableClusterAddress();
        $realHost = $primaryAddress['host'];
        $realPort = $primaryAddress['port'];

        $calledA = false;
        $resolverA = function (string $host, int $port) use ($realHost, $realPort, &$calledA): array {
            $calledA = true;
            return ['host' => $realHost, 'port' => $realPort];
        };

        $calledB = false;
        $resolverB = function (string $host, int $port) use ($realHost, $realPort, &$calledB): array {
            $calledB = true;
            return ['host' => $realHost, 'port' => $realPort];
        };

        $clientA = null;
        $clientB = null;
        try {
            $clientA = $this->newClusterClient(
                addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
                resolver: $resolverA,
            );

            $clientB = $this->newClusterClient(
                addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
                resolver: $resolverB,
            );

            $this->assertTrue($calledA, 'Resolver A must have been invoked for client A');
            $this->assertTrue($calledB, 'Resolver B must have been invoked for client B');
            $this->assertConnected($clientA);
            $this->assertConnected($clientB);
        } finally {
            $clientA?->close();
            $clientB?->close();
        }
    }

    /**
     * A client still open when the request ends (here in a static property) is
     * freed after RSHUTDOWN. Its resolver must stay valid until then, because the
     * client's background threads can still call it.
     */
    public function testClientOpenAtRequestShutdown()
    {
        $this->skipIfTlsEnabled();
        if ($this->getAuth()) {
            $this->markTestSkipped('The helper script connects without auth');
        }
        if (!function_exists('proc_open')) {
            $this->markTestSkipped('proc_open is not available');
        }

        $extension_path = __DIR__ . '/../modules/valkey_glide.so';
        $extension = file_exists($extension_path) ? $extension_path : 'valkey_glide';
        $marker = tempnam(sys_get_temp_dir(), 'resolver_shutdown_');
        $cmd = [PHP_BINARY, '-n', '-d', "extension=$extension",
                __DIR__ . '/scripts/resolver_client_open_at_shutdown.php',
                $this->getHost(), (string) $this->getPort(), $extension, $marker];
        /* Overwrite freed memory, so a call into a freed resolver crashes. On
         * Linux this also needs libffi to allocate closures with malloc (static
         * trampolines, libffi 3.4+); musl ignores MALLOC_PERTURB_ */
        $env = getenv() + ['MallocScribble' => '1', 'MALLOC_PERTURB_' => '85'];

        try {
            for ($run = 0; $run < 3; $run++) {
                file_put_contents($marker, '');
                $proc = proc_open($cmd, [1 => ['pipe', 'w'], 2 => ['file', '/dev/null', 'w']], $pipes, null, $env);
                $this->assertTrue(is_resource($proc));
                $out = stream_get_contents($pipes[1]);
                fclose($pipes[1]);
                $status = proc_close($proc);

                /* The core also logs to stdout */
                $this->assertStringContains("done\n", $out);
                $this->assertEquals(0, $status, "Run $run exited with status $status");
                /* Without the reconnects nothing calls the resolver after RSHUTDOWN */
                $this->assertEquals('killed', file_get_contents($marker), "Run $run did not drop the connections");
            }
        } finally {
            @unlink($marker);
        }
    }

    /**
     * The core does not wait for a client's threads when it is dropped inside a
     * resolver callable, so a client closed there is closed once the callable
     * has returned, here when the constructor that called it returns.
     */
    public function testCloseClientFromResolver()
    {
        $this->closeClientFromResolver(false);
    }

    /**
     * A lazy client calls its resolver from its first command, so a client
     * closed there is closed when that command returns.
     */
    public function testCloseClientFromLazyClientResolver()
    {
        $this->closeClientFromResolver(true);
    }

    /**
     * Pub/sub commands call the core directly, not through the command
     * helpers, and must close them too.
     */
    public function testCloseClientFromLazyClientResolverOnPublish()
    {
        $this->closeClientFromResolver(
            true,
            fn (ValkeyGlideCluster $client) => $this->assertIsInt($client->publish('resolver_closed_' . uniqid(), 'x'))
        );
    }

    private function closeClientFromResolver(bool $lazy, ?callable $firstCommand = null): void
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();
        $nameB = 'resolver_closed_' . uniqid();

        /* Every GLIDE command closes the deferred clients once it returns, so a
         * command cannot observe whether they were already closed. Resolver B's
         * callable is released right after client B is closed: its sentinel
         * reports that without a command */
        $state = new stdClass();
        $state->released = false;
        $sentinel = new class ($state) {
            public function __construct(private stdClass $state)
            {
            }

            public function __destruct()
            {
                $this->state->released = true;
            }
        };
        $resolverB = function (string $host, int $port) use ($realHost, $realPort, $sentinel): array {
            return ['host' => $realHost, 'port' => $realPort];
        };
        unset($sentinel);

        $clientB = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
            credentials: $this->getAuth(),
            client_name: $nameB,
            address_resolver: $resolverB,
        );
        unset($resolverB);
        $this->assertConnected($clientB);

        $resolverA = function (string $host, int $port) use ($realHost, $realPort, &$clientB): array {
            if ($clientB !== null) {
                $clientB->close();
                $clientB = null;
            }
            return ['host' => $realHost, 'port' => $realPort];
        };
        $clientA = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
            credentials: $this->getAuth(),
            lazy_connect: $lazy,
            address_resolver: $resolverA,
        );

        if (!$lazy) {
            $this->assertNull($clientB, 'Resolver A must have closed client B');
            $this->assertTrue($state->released, 'Client B must be closed when the constructor returns');
        }
        /* A lazy client runs resolver A from this first command; check before
         * another command closes B anyway */
        if ($firstCommand) {
            $firstCommand($clientA);
        } else {
            $this->assertConnected($clientA);
        }
        $this->assertNull($clientB, 'Resolver A must have closed client B');
        $this->assertTrue($state->released, 'Client B must be closed when the call that ran resolver A returns');
        $this->assertConnected($clientA);

        /* The server sees B's connections go once they are closed */
        $deadline = microtime(true) + 2;
        while (true) {
            $lists = $clientA->rawcommand('allNodes', 'CLIENT', 'LIST');
            $this->assertIsArray($lists);
            $open = str_contains(implode("\n", $lists), "name=$nameB ");
            if (!$open || microtime(true) >= $deadline) {
                break;
            }
            usleep(50000);
        }
        $this->assertFalse($open, 'Client B must be closed once the call that ran resolver A returns');
        $clientA->close();
    }

    /**
     * OPT_SLAVE_FAILOVER creates a new core client with the object's resolver.
     * If that resolver closes the object, the new client must not stay open on
     * the resolver the close frees.
     */
    public function testCloseClientFromResolverDuringSlaveFailover()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();
        $client = null;
        $armed = false;
        $resolver = function (string $host, int $port) use ($realHost, $realPort, &$client, &$armed): array {
            if ($armed) {
                $armed = false;
                $client->close();
            }
            return ['host' => $realHost, 'port' => $realPort];
        };
        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'fake.nonexistent', 'port' => 9999]],
            credentials: $this->getAuth(),
            address_resolver: $resolver,
        );
        $this->assertConnected($client);

        $armed = true;
        $this->assertFalse(
            $client->setOption(ValkeyGlideCluster::OPT_SLAVE_FAILOVER, ValkeyGlideCluster::FAILOVER_DISTRIBUTE)
        );
        $this->assertFalse($armed, 'The resolver must have run');
        $this->assertEquals('OPT_SLAVE_FAILOVER: client is closed', $client->getLastError());
        $this->assertFalse(@$client->ping('allPrimaries'), 'The client must stay closed');
    }

    /**
     * Closing the clients a failover's resolver closed can run a destructor
     * that closes the failing-over client. That close must also close its new
     * client, not leave it installed on the resolver the close frees.
     */
    public function testCloseClientFromDestructorDuringSlaveFailover()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();
        $holder = new stdClass();
        $holder->client = null;
        $sentinel = new class ($holder) {
            public function __construct(private stdClass $holder)
            {
            }

            public function __destruct()
            {
                $this->holder->client?->close();
            }
        };
        $resolverB = function (string $host, int $port) use ($realHost, $realPort, $sentinel): array {
            return ['host' => $realHost, 'port' => $realPort];
        };
        unset($sentinel);
        $clientB = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
            credentials: $this->getAuth(),
            address_resolver: $resolverB,
        );
        unset($resolverB);

        $armed = false;
        $resolverA = function (string $host, int $port) use ($realHost, $realPort, &$clientB, &$armed): array {
            if ($armed) {
                $armed = false;
                $clientB->close();
                $clientB = null;
            }
            return ['host' => $realHost, 'port' => $realPort];
        };
        $clientA = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
            credentials: $this->getAuth(),
            address_resolver: $resolverA,
        );
        $holder->client = $clientA;
        $this->assertConnected($clientA);

        $armed = true;
        $clientA->setOption(ValkeyGlideCluster::OPT_SLAVE_FAILOVER, ValkeyGlideCluster::FAILOVER_DISTRIBUTE);
        $this->assertNull($clientB, 'Resolver A must have closed client B');
        $this->assertFalse(@$clientA->ping('allPrimaries'), 'B\'s destructor must have closed client A');
    }

    /**
     * updateConnectionPassword() with immediate auth makes two core calls. If
     * the first runs a resolver that closes the client, the second must not
     * get a closed handle.
     */
    public function testCloseClientFromLazyClientResolverOnUpdateConnectionPassword()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();
        $client = null;
        $resolver = function (string $host, int $port) use ($realHost, $realPort, &$client): array {
            $client?->close();
            return ['host' => $realHost, 'port' => $realPort];
        };
        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'fake.nonexistent', 'port' => 9999]],
            credentials: $this->getAuth(),
            lazy_connect: true,
            address_resolver: $resolver,
        );

        $this->assertThrows(
            ValkeyGlideException::class,
            fn () => $client->updateConnectionPassword('resolver_closed', true)
        );
    }

    public function testNullAddressResolverConnectsNormally()
    {
        $this->skipIfTlsEnabled();

        $client = $this->newClusterClient(
            addresses: $this->getClusterAddresses(),
            resolver: null,
        );

        try {
            $this->assertConnected($client);
        } finally {
            $client->close();
        }
    }

    /**
     * The core also resolves addresses on background threads (reconnects, periodic
     * topology checks). The PHP resolver must only run on the PHP thread: running it
     * on another thread corrupted the Zend heap and crashed the process.
     */
    public function testAddressResolverNotCalledFromBackgroundThreads()
    {
        $this->skipIfTlsEnabled();

        $calls = 0;
        $resolver = function (string $host, int $port) use (&$calls): array {
            $calls++;
            return ['host' => $host, 'port' => $port];
        };

        /* Periodic checks disabled, so only the reconnects below resolve in the background */
        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED,
        );
        $this->assertTrue($client->ping('allPrimaries'));
        $this->assertGT(0, $calls);
        $calls_after_connect = $calls;

        /* Drop the client's connections; the core reconnects in the background
         * while this thread sleeps */
        $admin = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED,
        );
        $admin->rawCommand('allNodes', 'CLIENT', 'KILL', 'SKIPME', 'yes', 'TYPE', 'normal');
        $admin->close();
        sleep(2);

        $this->assertEquals($calls_after_connect, $calls);

        /* Reconnects using the addresses resolved on this thread */
        $reconnected = false;
        for ($attempt = 0; $attempt < 20 && !$reconnected; $attempt++) {
            try {
                $reconnected = $client->ping('allPrimaries') === true;
            } catch (ValkeyGlideException $e) {
            }
            if (!$reconnected) {
                usleep(500000);
            }
        }
        $this->assertTrue($reconnected);
        $client->close();
    }
}
