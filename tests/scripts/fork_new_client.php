<?php

/*
 * Used by testNewClientInChildAfterFork. Runs in its own process so the only
 * GLIDE clients are the ones created here.
 *
 * Arguments: cluster(0|1) host port case
 * case: new_client         the parent uses its client, the child creates a new one
 *       persistent         the same with a persistent client, reconnected in the child
 *       parent_closed      the parent closes its client before forking
 * Prints JSON: how the forked child ended ("ok" if its new client worked), and
 * whether the parent can still read its key afterwards.
 */

[$script, $cluster, $host, $port, $case] = $argv;
$is_cluster = $cluster === '1';
$addresses = [['host' => $host, 'port' => (int) $port]];
$key = '{fork}:new-client:' . getmypid();
/* Set before the fork, so the child reconnects with the parent's id */
$persistent_id = 'fork_new_client_' . getmypid();

$connect = function (bool $persistent = false) use ($is_cluster, $addresses, $persistent_id) {
    if ($is_cluster) {
        return new ValkeyGlideCluster(addresses: $addresses, persistent: $persistent);
    }
    $client = new ValkeyGlide();
    if ($persistent) {
        $client->connect(addresses: $addresses, persistent_id: $persistent_id);
    } else {
        $client->connect(addresses: $addresses);
    }
    return $client;
};
$ping = fn ($c) => $is_cluster ? $c->ping(['type' => 'primarySlotKey', 'key' => $key]) : $c->ping();

$persistent = $case === 'persistent';
$client = $connect($persistent);
if ($ping($client) !== true || $client->set($key, 'parent') !== true) {
    echo json_encode(['child' => 'parent setup failed', 'parent_works' => false]);
    exit(1);
}
if ($case === 'parent_closed') {
    $client->close();
}

$pid = pcntl_fork();
if ($pid === -1) {
    echo json_encode(['child' => 'fork failed', 'parent_works' => false]);
    exit(1);
}
if ($pid === 0) {
    /* A persistent connect replaces the inherited entry with a new client */
    $new = $connect($persistent);
    if ($ping($new) !== true) {
        exit(10);
    }
    if ($new->get($key) !== 'parent') {
        exit(11);
    }
    if ($new->set("$key:child", 'child') !== true || $new->get("$key:child") !== 'child') {
        exit(12);
    }
    $new->del("$key:child");
    $new->close();
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

if ($case === 'parent_closed') {
    $client = $connect();
}
echo json_encode([
    'child' => $child,
    'parent_works' => $client->get($key) === 'parent',
]);
$client->del($key);
