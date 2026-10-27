<?php

/*
 * Used by testPersistentClientAcrossFork. Runs in its own process so the only
 * GLIDE client is the persistent one (GLIDE clients cannot be used or closed in a
 * child once the parent has used them).
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON: whether the forked child exited normally, and whether the parent
 * still reuses its client afterwards.
 */

[$script, $cluster, $host, $port] = $argv;
$is_cluster = $cluster === '1';

$connect = function () use ($is_cluster, $host, $port) {
    if ($is_cluster) {
        return new ValkeyGlideCluster(addresses: [['host' => $host, 'port' => (int) $port]], persistent: true);
    }
    $client = new ValkeyGlide();
    $client->connect(addresses: [['host' => $host, 'port' => (int) $port]], persistent_id: 'persistent_fork_test');
    return $client;
};
$client_id = fn ($c) => $is_cluster ? $c->rawCommand('{persistent}', 'CLIENT', 'ID') : $c->rawCommand('CLIENT', 'ID');

/* Kept in the persistent list once the object is gone */
$parent_id = $client_id($connect());

$pid = pcntl_fork();
if ($pid === 0) {
    /* Process shutdown destroys the inherited persistent list */
    exit(0);
}

$status = 0;
$exited = false;
for ($i = 0; $i < 100; $i++) {
    if (pcntl_waitpid($pid, $status, WNOHANG) === $pid) {
        $exited = pcntl_wifexited($status) && pcntl_wexitstatus($status) === 0;
        break;
    }
    usleep(100000);
}
if ($i === 100) {
    function_exists('posix_kill') ? posix_kill($pid, SIGKILL) : exec('kill -9 ' . (int) $pid);
    pcntl_waitpid($pid, $status);
}

echo json_encode([
    'child_exited_cleanly' => $exited,
    'parent_reused' => (string) $client_id($connect()) === (string) $parent_id,
]);
