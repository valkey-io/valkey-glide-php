<?php

/*
 * Used by testDelUnlinkLiteralArrayWithOpcache. Runs with opcache enabled, where an array
 * literal in the code is immutable and shared between calls. The extension must only read it:
 * a refcount or destroy on it corrupts the heap by the second call.
 *
 * Arguments: cluster(0|1) host port
 * Prints JSON with the results of repeated del() and unlink() calls.
 */

[$script, $cluster, $host, $port] = $argv;
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

$results = [];
for ($attempt = 0; $attempt < 3; $attempt++) {
    $client->set('{opcache}k1', 'a');
    $client->set('{opcache}k2', 'b');
    $results[] = delLiteral($client);
    $client->set('{opcache}k1', 'a');
    $client->set('{opcache}k2', 'b');
    $results[] = unlinkLiteral($client);
}

echo json_encode($results);
