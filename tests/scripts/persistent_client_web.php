<?php

/*
 * Router for the PHP built-in web server, used by testPersistentClientAcrossRequests.
 * One server process handles every request, so a persistent client kept by one
 * request is visible to the next, as in a PHP-FPM worker.
 *
 * Query parameters: cluster, host, port (required), persistent, pconnect, pconnect_id,
 * timeout, action.
 * Prints JSON with the server-side CLIENT ID of the connection the request used.
 */

final class PersistentClientHolder
{
    /** Freed during executor shutdown, after the extension's RSHUTDOWN */
    public static $client;
}

if (!isset($_GET['port'])) {
    http_response_code(400);
    exit('port is required');
}

$is_cluster = ($_GET['cluster'] ?? '0') === '1';
$host       = $_GET['host'] ?? '127.0.0.1';
$port       = (int) $_GET['port'];
$persistent = ($_GET['persistent'] ?? '1') === '1';
$pconnect   = ($_GET['pconnect'] ?? '0') === '1';
/* With pconnect_id, pconnect() is called PHPRedis-style: host, port, timeout, persistent_id */
$pconnect_id = $_GET['pconnect_id'] ?? null;
/* A different timeout is a different configuration, so a different kept client */
$timeout    = isset($_GET['timeout']) ? (int) $_GET['timeout'] : null;
$action     = $_GET['action'] ?? '';
$key        = '{persistent}:watched';

$connect = function () use ($is_cluster, $host, $port, $persistent, $pconnect, $pconnect_id, $timeout) {
    if ($is_cluster) {
        return new ValkeyGlideCluster(
            addresses: [['host' => $host, 'port' => $port]],
            request_timeout: $timeout,
            persistent: $persistent
        );
    }
    $client = new ValkeyGlide();
    if ($pconnect && $pconnect_id !== null) {
        $client->pconnect($host, $port, 0.0, $pconnect_id);
        return $client;
    }
    if ($pconnect) {
        $client->pconnect(addresses: [['host' => $host, 'port' => $port]], request_timeout: $timeout);
        return $client;
    }
    $client->connect(
        addresses: [['host' => $host, 'port' => $port]],
        request_timeout: $timeout,
        persistent_id: $persistent ? 'persistent_test' : null
    );
    return $client;
};

$client_id = function ($client) use ($is_cluster) {
    return $is_cluster
        ? $client->rawCommand('{persistent}', 'CLIENT', 'ID')
        : $client->rawCommand('CLIENT', 'ID');
};

$client = $connect();
$out    = ['id' => $client_id($client)];
if (!$is_cluster) {
    preg_match('/\bdb=(\d+)/', $client->rawCommand('CLIENT', 'INFO'), $m);
    $out['db'] = (int) $m[1];
}

switch ($action) {
    case 'close':
        $client->close();
        break;
    case 'select':
        $client->select(1);
        break;
    case 'reset':
        $client->reset();
        break;
    case 'setname':
        $is_cluster
            ? $client->client('{persistent}', 'SETNAME', 'persistent_test_name')
            : $client->client('SETNAME', 'persistent_test_name');
        break;
    case 'raw_select':
        $is_cluster ? $client->rawCommand('{persistent}', 'SELECT', '1') : $client->rawCommand('SELECT', '1');
        break;
    case 'stringable_select':
        /* The command name is a Stringable, converted when the command is sent */
        $name = new class () {
            public function __toString(): string
            {
                return 'SELECT';
            }
        };
        $is_cluster ? $client->rawCommand('{persistent}', $name, '1') : $client->rawCommand($name, '1');
        break;
    case 'watch':
        /* Ends the request with the key still watched */
        $client->watch($key);
        break;
    case 'transaction':
        /* Would abort if WATCH from an earlier request were still active */
        $client->multi();
        $client->set($key, 'from_transaction');
        $out['exec'] = $client->exec();
        break;
    case 'two':
        /* A second object with the same configuration gets its own connection */
        $other = $connect();
        $out['other_id'] = $client_id($other);
        $other->close();
        break;
    case 'static':
        PersistentClientHolder::$client = $client;
        break;
    case 'paused_select':
        /* The server runs the SELECT after the client gave up waiting for it */
        $client->rawCommand('CLIENT', 'PAUSE', '700', 'ALL');
        try {
            $out['select'] = $client->select(1);
        } catch (Exception $e) {
            $out['select'] = $e->getMessage();
        }
        break;
    case 'watch_two_slots_exec':
        /* WATCH on two slots (two nodes in a cluster); EXEC runs on one of them */
        $client->watch($key, '{persistent_other}:watched');
        $client->multi();
        $client->set($key, 'from_transaction');
        $out['exec'] = $client->exec();
        break;
    case 'transaction_other':
        /* Would abort if WATCH on the other slot were still active */
        $client->multi();
        $client->set('{persistent_other}:watched', 'from_transaction');
        $out['exec'] = $client->exec();
        break;
    case 'watch_queued_unwatch':
        /* UNWATCH queued in a pipeline that is discarded never reaches the server */
        $client->watch($key);
        $client->pipeline();
        $client->unwatch();
        $client->discard();
        break;
    case 'watch_static':
        $client->watch($key);
        PersistentClientHolder::$client = $client;
        break;
}

echo json_encode($out);
