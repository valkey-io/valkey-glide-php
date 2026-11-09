<?php

/*
 * Router script for the PHP built-in web server, used by
 * ValkeyGlideFeaturesTest::testOtelConfigSurvivesRequestEnd. Each HTTP request is a
 * separate PHP request in the same process, so the process-wide OpenTelemetry
 * configuration created by the first one is used by the later ones.
 *
 * Query parameters: step (0 = readiness check, 1, 2 or 3), host, port, traces (file path).
 */

use ValkeyGlide\OpenTelemetry\OpenTelemetryConfig;
use ValkeyGlide\OpenTelemetry\TracesConfig;

/* The OpenTelemetry builder classes are PHP files loaded by composer */
require_once __DIR__ . '/../../vendor/autoload.php';

$step = (int) ($_GET['step'] ?? 0);
$host = $_GET['host'] ?? '127.0.0.1';
$port = (int) ($_GET['port'] ?? 6379);
$tracesFile = $_GET['traces'] ?? (sys_get_temp_dir() . '/valkey_glide_otel_multi_request.json');

$connect = function (array $advancedConfig = []) use ($host, $port) {
    $client = new ValkeyGlide();
    $client->connect(addresses: [['host' => $host, 'port' => $port]], advanced_config: $advancedConfig);
    return $client;
};

try {
    if ($step === 0) {
        /* Readiness check: only this script answers it */
        echo 'ready';
        return;
    }
    if ($step === 1) {
        /* Initializes OTel for the whole process */
        $otelConfig = OpenTelemetryConfig::builder()
            ->traces(TracesConfig::builder()->endpoint('file://' . $tracesFile)->samplePercentage(1)->build())
            ->flushIntervalMs(100)
            ->build();
        $client = $connect(['otel' => $otelConfig]);
        ValkeyGlide::setOtelSamplePercentage(42);
        $client->set('otel:multi_request', 'v1');
        $client->close();
    } elseif ($step === 2) {
        /* The configuration from step 1 must still be readable and writable */
        $before = ValkeyGlide::getOtelSamplePercentage();
        $client = $connect();
        $client->set('otel:multi_request', 'v2');
        ValkeyGlide::setOtelSamplePercentage(100);
        $client->del('otel:multi_request');
        $client->close();
        echo "before=$before ";
    }
    echo 'percentage=' . ValkeyGlide::getOtelSamplePercentage();
} catch (Throwable $exception) {
    echo 'error=' . get_class($exception) . ': ' . $exception->getMessage();
}
