<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . '/ValkeyGlideClusterBaseTest.php';
require_once __DIR__ . '/TestConstants.php';

class AddressResolverClusterTest extends ValkeyGlideClusterBaseTest
{
    public function testAddressResolverWithFakeAddress()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();

        $resolver = function (string $host, int $port) use ($realHost, $realPort): array {
            return ['host' => $realHost, 'port' => $realPort];
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => 'fake.nonexistent.host', 'port' => 9999]],
            address_resolver: $resolver,
        );

        $this->assertConnected($client);
        $client->close();
    }

    public function testAddressResolverExceptionFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): array {
            $called = true;
            throw new RuntimeException("resolver error");
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertTrue($called);
        $this->assertConnected($client);
        $client->close();
    }

    public function testAddressResolverReturnsInvalidFallsBackToOriginal()
    {
        $this->skipIfTlsEnabled();

        $called = false;
        $resolver = function (string $host, int $port) use (&$called): mixed {
            $called = true;
            return null;
        };

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: $resolver,
        );

        $this->assertTrue($called);
        $this->assertConnected($client);
        $client->close();
    }

    /**
     * Two cluster clients with different resolvers don't interfere with each other.
     */
    public function testMultiClientResolversAreIsolated()
    {
        $this->skipIfTlsEnabled();

        $realHost = $this->getHost();
        $realPort = $this->getPort();

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

        $clientA = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-a.nonexistent', 'port' => 9998]],
            address_resolver: $resolverA,
        );

        $clientB = new ValkeyGlideCluster(
            addresses: [['host' => 'fake-b.nonexistent', 'port' => 9999]],
            address_resolver: $resolverB,
        );

        $this->assertTrue($calledA, 'Resolver A must have been invoked for client A');
        $this->assertTrue($calledB, 'Resolver B must have been invoked for client B');
        $this->assertConnected($clientA);
        $this->assertConnected($clientB);

        $clientA->close();
        $clientB->close();
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

    public function testNullAddressResolverConnectsNormally()
    {
        $this->skipIfTlsEnabled();

        $client = new ValkeyGlideCluster(
            addresses: [['host' => $this->getHost(), 'port' => $this->getPort()]],
            address_resolver: null,
        );

        $this->assertConnected($client);
        $client->close();
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
