<?php

/*
 * Used by testClientInheritedThroughFork. Runs in its own process so the only
 * GLIDE client is the one created here.
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON: how the forked child ended ("ok" if using the inherited client
 * threw and the child exited normally), and whether the parent's client still
 * works afterwards.
 */

[$script, $cluster, $host, $port] = $argv;
$is_cluster = $cluster === '1';

/* A user subclass: its own methods reach the client through $this and parent:: */
class ForkTestClient extends ValkeyGlide
{
    public function get($key): mixed
    {
        return parent::get($key);
    }

    public function viaThis($key): mixed
    {
        return $this->exists($key);
    }
}

class ForkTestClusterClient extends ValkeyGlideCluster
{
    public function get($key): mixed
    {
        return parent::get($key);
    }

    public function viaThis($key): mixed
    {
        return $this->exists($key);
    }
}

if ($is_cluster) {
    $client = new ForkTestClusterClient(addresses: [['host' => $host, 'port' => (int) $port]]);
} else {
    $client = new ForkTestClient();
    $client->connect(addresses: [['host' => $host, 'port' => (int) $port]]);
}
$client->set('{fork}:key', 'parent');
$callable = $client->get(...);
/* exists() is not overridden, so these reach the extension's method directly */
$base_callable = $client->exists(...);
$monitor = $is_cluster ? null : new ValkeyGlideMonitor(addresses: [['host' => $host, 'port' => (int) $port]]);

$pid = pcntl_fork();
if ($pid === -1) {
    echo json_encode(['child' => 'fork failed', 'parent_works' => false]);
    exit(1);
}
if ($pid === 0) {
    /* Every way of calling a method must throw; exit code 10 + index if one does not */
    $calls = [
        fn () => $client->get('{fork}:key'),
        fn () => call_user_func([$client, 'get'], '{fork}:key'),
        fn () => call_user_func_array([$client, 'get'], ['{fork}:key']),
        fn () => array_map([$client, 'get'], ['{fork}:key']),
        fn () => (new ReflectionMethod($client, 'get'))->invoke($client, '{fork}:key'),
        fn () => $callable('{fork}:key'),
        fn () => Closure::fromCallable([$client, 'get'])('{fork}:key'),
        fn () => $client->viaThis('{fork}:key'),
        fn () => call_user_func([$client, 'exists'], '{fork}:key'),
        fn () => call_user_func_array([$client, 'exists'], ['{fork}:key']),
        fn () => array_map([$client, 'exists'], ['{fork}:key']),
        fn () => (new ReflectionMethod($client, 'exists'))->invoke($client, '{fork}:key'),
        fn () => $base_callable('{fork}:key'),
        fn () => Closure::fromCallable([$client, 'exists'])('{fork}:key'),
    ];
    if ($monitor) {
        $calls[] = fn () => $monitor->getMonitorMessage(0.1);
        $calls[] = fn () => call_user_func([$monitor, 'getMonitorMessage'], 0.1);
    }
    foreach ($calls as $index => $call) {
        try {
            $call();
            exit(10 + $index);
        } catch (ValkeyGlideException $e) {
            if (!str_contains($e->getMessage(), 'fork()')) {
                exit(3);
            }
        }
    }
    /* Introspection and methods that do not use the client are unaffected */
    if (method_exists($client, 'noSuchMethod') || !is_callable([$client, 'get'])) {
        exit(4);
    }
    try {
        $client->getLastError();
        $client->clearLastError();
        $client->getOption(ValkeyGlide::OPT_REPLY_LITERAL);
    } catch (Throwable $e) {
        exit(5);
    }
    /* Allowed: forgets the handle without closing the parent's client */
    $client->close();
    $monitor?->close();
    exit(0);
}

$status = 0;
$child = 'hung';
for ($i = 0; $i < 100; $i++) {
    $waited = pcntl_waitpid($pid, $status, WNOHANG);
    if ($waited === -1) {
        $child = 'wait failed';
        break;
    }
    if ($waited === $pid) {
        if (pcntl_wifexited($status)) {
            $child = pcntl_wexitstatus($status) === 0 ? 'ok' : 'exit ' . pcntl_wexitstatus($status);
        } else {
            $child = 'signal ' . pcntl_wtermsig($status);
        }
        break;
    }
    usleep(100000);
}
if ($child === 'hung') {
    function_exists('posix_kill') ? posix_kill($pid, SIGKILL) : exec('kill -9 ' . (int) $pid);
    pcntl_waitpid($pid, $status);
}

echo json_encode([
    'child' => $child,
    'parent_works' => $client->get('{fork}:key') === 'parent',
]);
$monitor?->close();
