<?php

defined('VALKEY_GLIDE_PHP_TESTRUN') or die("Use TestValkeyGlide.php to run tests!\n");

require_once __DIR__ . "/ValkeyGlideBaseTest.php";

/**
 * ValkeyGlide PubSub Test
 * Tests publish/subscribe functionality for standalone ValkeyGlide client
 */
class ValkeyGlidePubSubTest extends ValkeyGlideBaseTest
{
    public function __construct($host, $port, $auth, $tls)
    {
        parent::__construct($host, $port, $auth, $tls);
    }

    public function testPubSubBasicSubscribe()
    {
        $valkey_glide = new ValkeyGlide([
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ]);
        
        $valkey_glide->subscribe(['test_channel'], function($redis, $channel, $message) {
            $redis->unsubscribe(['test_channel']);
        });
        
        $this->assertTrue(true);
        $valkey_glide->close();
    }

    public function testPubSubPublish()
    {
        $valkey_glide = new ValkeyGlide([
            ['host' => $this->getHost(), 'port' => $this->getPort()]
        ]);
        
        $count = $valkey_glide->publish('test_channel', 'test_message');
        $this->assertIsInt($count);
        $this->assertGreaterThanOrEqual(0, $count);
        
        $valkey_glide->close();
    }

    public function testPubSubActualMessageDelivery()
    {
        // Use separate PHP script files for clean multi-process testing
        $channel = 'test_channel_' . uniqid();
        $test_message = 'test_message_' . time();
        $sync_file = tempnam(sys_get_temp_dir(), 'pubsub_sync_');
        $result_file = tempnam(sys_get_temp_dir(), 'pubsub_result_');
        
        try {
            $this->runSeparateProcessTest($channel, $test_message, $sync_file, $result_file);
        } finally {
            if (file_exists($sync_file)) unlink($sync_file);
            if (file_exists($result_file)) unlink($result_file);
        }
    }
    
    private function runSeparateProcessTest($channel, $test_message, $sync_file, $result_file)
    {
        // Get current PHP context
        $php_cmd = PHP_BINARY;
        if (extension_loaded('valkey_glide')) {
            $php_cmd .= ' -d extension=valkey_glide';
        }
        
        // Path to script files
        $publisher_script = __DIR__ . '/pubsub_publisher.php';
        $subscriber_script = __DIR__ . '/pubsub_subscriber.php';
        
        if (!file_exists($publisher_script) || !file_exists($subscriber_script)) {
            $this->fail('PubSub script files not found');
        }
        
        // Build commands with arguments
        $host = $this->getHost();
        $port = $this->getPort();
        
        $subscriber_cmd = $php_cmd . ' ' . escapeshellarg($subscriber_script) . 
                         ' ' . escapeshellarg($host) . 
                         ' ' . escapeshellarg($port) . 
                         ' ' . escapeshellarg($channel) . 
                         ' ' . escapeshellarg($test_message) . 
                         ' ' . escapeshellarg($sync_file) . 
                         ' ' . escapeshellarg($result_file);
        
        $publisher_cmd = $php_cmd . ' ' . escapeshellarg($publisher_script) . 
                        ' ' . escapeshellarg($host) . 
                        ' ' . escapeshellarg($port) . 
                        ' ' . escapeshellarg($channel) . 
                        ' ' . escapeshellarg($test_message) . 
                        ' ' . escapeshellarg($sync_file);
        
        // Start processes
        $subscriber_proc = proc_open($subscriber_cmd, [
            0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']
        ], $subscriber_pipes);
        
        $publisher_proc = proc_open($publisher_cmd, [
            0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']
        ], $publisher_pipes);
        
        if (!is_resource($subscriber_proc) || !is_resource($publisher_proc)) {
            $this->fail('Could not start subprocess');
        }
        
        // Wait for completion
        $timeout = time() + 10;
        while (time() < $timeout) {
            $sub_status = proc_get_status($subscriber_proc);
            $pub_status = proc_get_status($publisher_proc);
            
            if (!$sub_status['running'] && !$pub_status['running']) {
                break;
            }
            usleep(50000);
        }
        
        // Cleanup
        foreach ($subscriber_pipes as $pipe) fclose($pipe);
        foreach ($publisher_pipes as $pipe) fclose($pipe);
        proc_close($subscriber_proc);
        proc_close($publisher_proc);
        
        // Verify result
        $result = file_exists($result_file) && file_get_contents($result_file) === 'SUCCESS';
        $this->assertTrue($result, 'Expected message was not received in multi-process test');
    }
}
