<?php

/*
 * Used by testClientOpenAtRequestShutdown. Runs in its own process so it can
 * end the request with clients still open.
 *
 * Arguments: host port extension marker
 * Prints "done" before the request ends. The clients live in a static
 * property, so PHP frees them only after the extension's RSHUTDOWN. Until
 * then their background threads may still call the address resolver. The
 * child process below writes "killed" to the marker file once it has dropped
 * the clients' connections.
 */

[$script, $host, $port, $extension, $marker] = $argv;

class ResolverShutdownHolder
{
    public static array $clients = [];
}

/* Periodic checks would also call the resolver, but how often depends on the
 * check interval: rely on reconnects only */
$resolver = fn (string $h, int $p): array => ['host' => $h, 'port' => $p];
for ($i = 0; $i < 3; $i++) {
    $client = new ValkeyGlideCluster(
        addresses: [['host' => $host, 'port' => (int) $port]],
        periodic_checks: ValkeyGlideCluster::PERIODIC_CHECK_DISABLED,
        address_resolver: $resolver,
    );
    $client->set("resolver-shutdown-$i", 'v');
    ResolverShutdownHolder::$clients[] = $client;
}

/* Request shutdown closes this stream (zend_close_rsrc_list()) after RSHUTDOWN
 * and before the clients are freed, and pclose() waits for the child. The
 * child drops the clients' connections while it is waiting, so their
 * reconnects call the resolver between RSHUTDOWN and the close. */
$child = 'usleep(300000);'
    . '$c = new ValkeyGlideCluster(addresses: [["host" => $argv[1], "port" => (int) $argv[2]]]);'
    . 'if ($c->rawcommand("allNodes", "CLIENT", "KILL", "TYPE", "normal", "SKIPME", "yes")) {'
    . '    file_put_contents($argv[3], "killed");'
    . '}'
    . 'usleep(700000);';
$GLOBALS['resolver_shutdown_wait'] = popen(implode(' ', array_map('escapeshellarg', [
    PHP_BINARY, '-n', '-d', "extension=$extension", '-r', $child, $host, $port, $marker,
])) . ' >/dev/null 2>&1', 'r');

echo "done\n";
