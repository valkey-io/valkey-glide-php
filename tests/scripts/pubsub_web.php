<?php

/*
 * Router for the PHP built-in web server, used by testSubscribeInConsecutiveRequests.
 * One server process handles every request, as a PHP-FPM worker does, so the
 * second request reuses the worker that ran the first one's pub/sub cleanup.
 *
 * Query parameters: host, port (required), channel.
 * Subscribes to the channel, unsubscribes on the first message, and prints JSON.
 */

if (!isset($_GET['port'], $_GET['channel'])) {
    http_response_code(400);
    exit('port and channel are required');
}

$client = new ValkeyGlide();
$client->connect(addresses: [['host' => $_GET['host'] ?? '127.0.0.1', 'port' => (int) $_GET['port']]]);

$received = null;
$client->subscribe([$_GET['channel']], function ($client, $channel, $message) use (&$received) {
    $received = $message;
    $client->unsubscribe([$channel]);
});

echo json_encode(['received' => $received]);
