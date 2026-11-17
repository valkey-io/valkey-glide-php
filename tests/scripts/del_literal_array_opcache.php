<?php

/*
 * Used by testDelUnlinkLiteralArrayWithOpcache. Runs with opcache enabled, where an array
 * literal in the code is immutable and shared between calls. The extension must only read it:
 * a refcount or destroy on it corrupts the heap by the second call.
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON with the results of repeated del() and unlink() calls, or an error string when
 * opcache is not active.
 */

[$script, $cluster, $host, $port] = $argv;

/* Without opcache the literals are ordinary arrays and the test would prove nothing */
if (!function_exists('opcache_get_status') || !(opcache_get_status(false)['opcache_enabled'] ?? false)) {
    echo json_encode('opcache is not active');
    exit(1);
}
$addresses = [['host' => $host, 'port' => (int) $port]];

if ($cluster === '1') {
    $client = new ValkeyGlideCluster(addresses: $addresses);
} else {
    $client = new ValkeyGlide();
    $client->connect(addresses: $addresses);
}

function delLiteral($client)
{
    return $client->del(['{opcache}k1', '{opcache}k2']);
}

function unlinkLiteral($client)
{
    return $client->unlink(['{opcache}k1', '{opcache}k2']);
}

/* Integer keys are converted to strings; the literal itself must stay untouched */
function delIntegerLiteral($client)
{
    return $client->del([1234567, 7654321]);
}

function unlinkIntegerLiteral($client)
{
    return $client->unlink([1234567, 7654321]);
}

$results = [];
for ($attempt = 0; $attempt < 3; $attempt++) {
    $client->set('{opcache}k1', 'a');
    $client->set('{opcache}k2', 'b');
    $results[] = delLiteral($client);
    $client->set('{opcache}k1', 'a');
    $client->set('{opcache}k2', 'b');
    $results[] = unlinkLiteral($client);
    $client->set('1234567', 'a');
    $client->set('7654321', 'b');
    $results[] = delIntegerLiteral($client);
    $client->set('1234567', 'a');
    $results[] = unlinkIntegerLiteral($client);
}

echo json_encode($results);
