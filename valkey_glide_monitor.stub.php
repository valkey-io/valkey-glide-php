<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

/**
 * A single line received from the MONITOR command, in structured form.
 *
 * Mirrors the MonitorLine / MonitorMsg types exposed by the other Valkey GLIDE
 * clients (Go, Python, Node, Java) so that PHP users receive the same decoded
 * fields instead of a raw text line they must parse themselves.
 */
final class ValkeyGlideMonitorLine
{
    /** Server timestamp as a Unix time in seconds. */
    public float $timestamp;

    /** Database index the command was executed against. */
    public int $db;

    /** Client address in "host:port" form (may be empty for internal calls). */
    public string $clientAddr;

    /** The command name (e.g. "SET", "GET"). */
    public string $command;

    /** The command arguments, decoded into a list of strings. */
    public array $args;
}

/**
 * A dedicated client for the Valkey MONITOR command.
 *
 * MONITOR streams every command processed by the server. This client opens its
 * own connection (separate from any ValkeyGlide client) and delivers structured
 * ValkeyGlideMonitorLine records. It is standalone-only; cluster mode is not
 * supported, consistent with the other Valkey GLIDE clients.
 *
 * Warning: MONITOR is a debugging tool that degrades server performance. Do not
 * use it in production for extended periods.
 *
 * Two consumption styles are supported, matching the Valkey GLIDE
 * documentation (https://glide.valkey.io/how-to/monitoring/monitor-command/):
 *  - Queue: call getMonitorMessage()/tryGetMonitorMessage() from your own loop.
 *  - Callback: call listen() with a callback (PHP-specific convenience).
 *
 * @see https://glide.valkey.io/how-to/monitoring/monitor-command/
 *
 * @example
 * $monitor = new ValkeyGlideMonitor(addresses: [['host' => 'localhost', 'port' => 6379]]);
 * while (($line = $monitor->getMonitorMessage(timeout: 5.0)) !== null) {
 *     echo $line->command . "\n";
 * }
 * $monitor->close();
 */
final class ValkeyGlideMonitor
{
    /**
     * Create a MonitorClient and open a dedicated MONITOR connection.
     *
     * A MONITOR client opens a single direct connection and streams every
     * command the server processes.
     *
     * The connection is opened immediately and monitor lines begin accumulating
     * in the background.
     *
     * @param array|null      $addresses    List of ['host' => string, 'port' => int] entries.
     * @param bool            $use_tls      Whether to use TLS for the connection.
     * @param array|null      $credentials  ['username' => string, 'password' => string, ...].
     * @param int|null        $database_id  Database index to select.
     * @param string|null     $client_name  Client name reported to the server.
     * @param mixed           $context      Stream context for TLS.
     *
     * @throws ValkeyGlideException If the connection cannot be established.
     */
    public function __construct(
        ?array $addresses = null,
        bool $use_tls = false,
        ?array $credentials = null,
        ?int $database_id = null,
        ?string $client_name = null,
        mixed $context = null
    ) {
    }

    /**
     * Block until the next monitor line is available and return it.
     *
     * @param float|null $timeout Maximum seconds to wait. Null waits indefinitely.
     *
     * @return ValkeyGlideMonitorLine|null The next line, or null on timeout/closed.
     */
    public function getMonitorMessage(?float $timeout = null): ?ValkeyGlideMonitorLine;

    /**
     * Return the next monitor line if one is already queued, without blocking.
     *
     * @return ValkeyGlideMonitorLine|null The next line, or null if none is available.
     */
    public function tryGetMonitorMessage(): ?ValkeyGlideMonitorLine;

    /**
     * Enter a blocking loop, invoking the callback for each monitor line.
     *
     * This is a PHP-specific convenience over getMonitorMessage(). Return any
     * non-null value from the callback to exit the loop.
     *
     * @param callable $cb Receives one ValkeyGlideMonitorLine argument per line.
     *
     * @return bool True when the loop exits normally.
     *
     * @throws ValkeyGlideException If the callback is not callable.
     */
    public function listen(callable $cb): bool;

    /**
     * Number of monitor lines dropped because the internal queue was full.
     *
     * Consult this to detect whether the stream was complete. The count is
     * cumulative for the life of the client.
     *
     * @return int The number of dropped lines.
     */
    public function getDroppedCount(): int;

    /**
     * Stop monitoring and release the dedicated connection.
     *
     * @return void
     */
    public function close(): void;
}
