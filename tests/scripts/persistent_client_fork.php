<?php

/*
 * Used by testPersistentClientAcrossFork. Runs in its own process so the only
 * GLIDE client is the persistent one (GLIDE clients cannot be used or closed in a
 * child once the parent has used them).
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON: whether the forked child exited normally, its
 * persistent_clients_discarded_fork count after it connected with the parent's
 * configuration, and whether the parent still reuses its client afterwards.
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

/* The child writes its counter here */
$report = tempnam(sys_get_temp_dir(), 'glide_fork_stats');

$pid = pcntl_fork();
if ($pid === -1) {
    echo json_encode(['child_exited_cleanly' => false, 'child_discarded_fork' => null, 'parent_reused' => false]);
    exit(1);
}
if ($pid === 0) {
    /* The same configuration replaces the inherited client with a new one */
    $child = $connect();
    $child_id = $client_id($child);
    $stats = $child->getStatistics();
    file_put_contents($report, (string) $stats['persistent_clients_discarded_fork']);
    /* Process shutdown destroys the persistent list, with the inherited entry gone */
    exit($child_id === $parent_id ? 1 : 0);
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

$discarded = file_get_contents($report);
unlink($report);

echo json_encode([
    'child_exited_cleanly' => $exited,
    'child_discarded_fork' => ctype_digit($discarded) ? (int) $discarded : $discarded,
    'parent_reused' => (string) $client_id($connect()) === (string) $parent_id,
]);
