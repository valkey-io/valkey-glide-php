<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

/**
 * Shared ValkeyGlideCluster client construction for cluster test suites.
 *
 * Cluster suites do not share a single base class (ValkeyGlideClusterTest extends
 * ValkeyGlideTest, ValkeyGlideClusterBatchTest extends ValkeyGlideBatchTest), so
 * the factory lives in a trait rather than in ValkeyGlideClusterBaseTest.
 */
trait ValkeyGlideClusterClientTrait
{
    /**
     * Get cluster seed addresses.
     *
     * Reads a comma-separated list from the VALKEY_CLUSTER_SEEDS env var
     * ("host:port" or bare "port" entries), falling back to the configured
     * host and cluster port.
     *
     * @return array<int, array{host: string, port: int}>
     */
    protected function getClusterAddresses(): array
    {
        $addresses = [];
        $envSeeds = getenv('VALKEY_CLUSTER_SEEDS');
        if (!empty($envSeeds)) {
            foreach (explode(',', $envSeeds) as $seed) {
                $seed = trim($seed);
                if ($seed === '') {
                    continue;
                }
                $sep = strrpos($seed, ':');
                if ($sep !== false) {
                    $addresses[] = [
                        'host' => trim(substr($seed, 0, $sep), '[]'),
                        'port' => (int) substr($seed, $sep + 1),
                    ];
                } else {
                    $addresses[] = ['host' => $this->getHost(), 'port' => (int) $seed];
                }
            }
        }
        if (empty($addresses)) {
            $addresses = [['host' => $this->getHost(), 'port' => $this->getPort()]];
        }
        return $addresses;
    }

    /**
     * Create a ValkeyGlideCluster client, retrying transient connection failures.
     *
     * @param int|null $databaseId Database to select, or null to not send one.
     */
    protected function newClusterInstance(?int $databaseId = null): ValkeyGlideCluster
    {
        $addresses = $this->getClusterAddresses();
        $options = [
            'addresses' => $addresses,
            'use_tls' => false,
            'credentials' => $this->getAuth(),
            'read_from' => ValkeyGlide::READ_FROM_PRIMARY,
            'request_timeout' => 10000,
        ];
        if ($databaseId !== null) {
            $options['database_id'] = $databaseId;
        }

        $attempts = 3;
        for ($attempt = 1; $attempt <= $attempts; $attempt++) {
            try {
                return new ValkeyGlideCluster(...$options);
            } catch (Exception $ex) {
                if ($attempt === $attempts) {
                    TestSuite::errorMessage("Fatal error: %s\n", $ex->getMessage());
                    TestSuite::errorMessage("Seeds: %s\n", json_encode($addresses));
                    exit(1);
                }
                echo "Warning: Cluster client connection attempt $attempt failed ({$ex->getMessage()}), " .
                    "retrying in 500ms...\n";
                usleep(500000);
            }
        }
    }
}
