<?php

/*
 * Used by testPersistentClientAcrossFork. Runs in its own process so the only
 * GLIDE client is the persistent one (GLIDE clients cannot be used or closed in a
 * child once the parent has used them).
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON: whether a forked child that exits with the inherited client still
 * listed exits normally; whether a second forked child, which connects with the
 * parent's configuration and so replaces the inherited client, exits normally and
 * what its persistent_clients_discarded_fork count is; and whether the parent still
 * reuses its client afterwards.
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

/* Forks a child that runs $body and exits with its status (0 = success). Returns
 * whether the child exited normally with status 0. */
$fork = function (callable $body): bool {
    $pid = pcntl_fork();
    if ($pid === -1) {
        echo json_encode(['fork_failed' => true]);
        exit(1);
    }
    if ($pid === 0) {
        exit($body());
    }
    $status = 0;
    for ($i = 0; $i < 100; $i++) {
        if (pcntl_waitpid($pid, $status, WNOHANG) === $pid) {
            return pcntl_wifexited($status) && pcntl_wexitstatus($status) === 0;
        }
        usleep(100000);
    }
    function_exists('posix_kill') ? posix_kill($pid, SIGKILL) : exec('kill -9 ' . (int) $pid);
    pcntl_waitpid($pid, $status);
    return false;
};

/* Process shutdown destroys the inherited persistent list, with the inherited entry
 * still listed: it must not close the parent's client */
$exited = $fork(fn () => 0);

/* The second child writes its counter here */
$report = tempnam(sys_get_temp_dir(), 'glide_fork_stats');

/* The same configuration replaces the inherited client with a new one */
$reconnect_exited = $fork(function () use ($connect, $client_id, $parent_id, $report) {
    $child = $connect();
    $child_id = $client_id($child);
    $stats = $child->getStatistics();
    file_put_contents($report, (string) $stats['persistent_clients_discarded_fork']);
    return $child_id === $parent_id ? 1 : 0;
});

$discarded = file_get_contents($report);
unlink($report);

echo json_encode([
    'child_exited_cleanly' => $exited,
    'reconnecting_child_exited_cleanly' => $reconnect_exited,
    'child_discarded_fork' => ctype_digit($discarded) ? (int) $discarded : $discarded,
    'parent_reused' => (string) $client_id($connect()) === (string) $parent_id,
]);
