<?php
$testFunctions = [
    "testMinimumVersion",
    "testPing",
    "testPipelinePublish",
    "testPubSub",
    "testBitcount",
    "testBitop",
    "testBitsets",
    "testLcs",
    "testLmpop",
    "testBLmpop",
    "testZmpop",
    "testBZmpop",
    "testBitPos",
    "testSetLargeKeys",
    "testEcho",
    "testErr",
    "testSet",
    "testExtendedSet",
    "testValkeyIfEq",
    "testGetSet",
    "testGetDel",
    "testRandomKey",
    "testRename",
    "testRenameNx",
    "testMultiple1",
    "testMultipleBin",
    "testExpireMember",
    "testExpire",
    "testExpireAt",
    "testExpireOptions",
    "testExpiretime",
    "testGetEx",
    "testSetEx",
    "testPSetEx",
    "testSetNX",
    "testExpireAtWithLong",
    "testIncr",
    "testIncrByFloat",
    "testDecr",
    "testExists",
    "testTouch",
    "testKeys",
    "testDelete",
    "testUnlink",
    "testType",
    "testStr",
    "testlPop",
    "testrPop",
    "testrPopSerialization",
    "testblockingPop",
    "testLLen",
    "testlPopx",
    "testlPos",
    "testltrim",
    "testSortPrefix",
    "testSortAsc",
    "testSortDesc",
    "testSortHandler",
    "testLindex",
    "testlMove",
    "testBlmove",
    "testLRem",
    "testSAdd",
    "testSCard",
    "testSRem",
    "testsMove",
    "testsPop",
    "testsPopWithCount",
    "testsRandMember",
    "testSRandMemberWithCount",
    "testSIsMember",
    "testSMembers",
    "testsMisMember",
    "testlSet",
    "testsInter",
    "testsInterStore",
    "testsUnion",
    "testsUnionStore",
    "testsDiff",
    "testsDiffStore",
    "testInterCard",
    "testLRange",
    "testdbSize",
    "testFlushDB",
    "testTTL",
    "testPersist",
    "testClient",
    "testSlowlog",
    "testWait",
    "testInfo",
    "testInfoCommandStats",
    "testSelect",
    "testSwapDB",
    "testMset",
    "testMsetNX",
    "testRpopLpush",
    "testBRpopLpush",
    "testZAddFirstArg",
    "testZaddIncr",
    "testZX",
    "testZRangeScoreArg",
    "testZRangeByLex",
    "testZLexCount",
    "testzDiff",
    "testzInter",
    "testzUnion",
    "testzDiffStore",
    "testzMscore",
    "testZRemRangeByLex",
    "testBZPop",
    "testZPop",
    "testZRandMember",
    "testHashes",
    "testHRandField",
    "testSetRange",
    "testObject",
    "testMultiExec",
    "testFailedTransactions",
    "testPipeline",
    "testPipelineMultiExec",
    "testMultiEmpty",
    "testPipelineEmpty",
    "testDoublePipeNoOp",
    "testDiscard",
    "testDifferentTypeString",
    "testDifferentTypeList",
    "testDifferentTypeSet",
    "testDifferentTypeSortedSet",
    "testDifferentTypeHash",
    "testSerializerPHP",
    "testIgnoreNumbers",
    "testIgnoreNumbersReturnTypes",
    "testSerializerIGBinary",
    "testSerializerMsgPack",
    "testSerializerJSON",
    "testCompressionLZF",
    "testCompressionZSTD",
    "testCompressionLZ4",
    "testDumpRestore",
    "testGetLastError",
    "testScript",
    "testEval",
    "testEvalSHA",
    "testSerialize",
    "testUnserialize",
    "testCompressHelpers",
    "testPackHelpers",
    "testGetWithMeta",
    "testPrefix",
    "testReplyLiteral",
    "testNullArray",
    "testNestedNullArray",
    "testConfig",
    "testReconnectSelect",
    "testTime",
    "testReadTimeoutOption",
    "testIntrospection",
    "testTransferredBytes",
    "testScan",
    "testScanPrefix",
    "testMaxRetriesOption",
    "testBackoffOptions",
    "testHScan",
    "testSScan",
    "testZScan",
    "testScanErrors",
    "testPFCommands",
    "testGeoAdd",
    "testGeoRadius",
    "testGeoRadiusByMember",
    "testGeoPos",
    "testGeoHash",
    "testGeoDist",
    "testGeoSearch",
    "testGeoSearchStore",
    "testRawCommand",
    "testXAdd",
    "testXRange",
    "testXLen",
    "testXGroup",
    "testXAck",
    "testXRead",
    "testXReadGroup",
    "testXPending",
    "testXDel",
    "testXTrim",
    "testXClaim",
    "testXAutoClaim",
    "testXInfo",
    "testXInfoEmptyStream",
    "testInvalidAuthArgs",
    "testAcl",
    "testUnixSocket",
    "testHighPorts",
    "testRequiresMode",
    "testMultipleConnect",
    "testConnectDatabaseSelect",
    "testConnectException",
    "testTlsConnect",
    "testReset",
    "testCopy",
    "testCommand",
    "testFunction",
    "testWaitAOF",
    "testBadOptionValue"
];

$passed = 0;
$failed = 0;
$skipped = 0;
$failedTests = [];
$skippedTests = [];

foreach ($testFunctions as $testFunction) {
    echo "Running {$testFunction}...\n";

    $cmd = "php TestValkeyGlide.php --class ValkeyGlide --test {$testFunction}";
    exec($cmd, $output, $returnVar);

    echo implode("\n", $output) . "\n";

    $isSkipped = false;
    foreach ($output as $line) {
        if (stripos($line, 'SKIPPED') !== false) {
            $isSkipped = true;
            break;
        }
    }

    if ($isSkipped) {
        echo "[SKIPPED] {$testFunction} was skipped.\n\n";
        $skipped++;
        $skippedTests[] = $testFunction;
    } elseif ($returnVar === 0) {
        echo "[SUCCESS] {$testFunction} completed.\n\n";
        $passed++;
    } else {
        echo "[FAILURE] {$testFunction} failed with exit code {$returnVar}.\n\n";
        $failed++;
        $failedTests[] = $testFunction;
    }

    $output = [];
}

echo "========================\n";
echo "Test summary:\n";
echo "Total tests run: " . count($testFunctions) . "\n";
echo "Passed: {$passed}\n";
echo "Failed: {$failed}\n";
echo "Skipped: {$skipped}\n";

if ($failed > 0) {
    echo "\nFailed tests:\n";
    foreach ($failedTests as $test) {
        echo "- {$test}\n";
    }
}

if ($skipped > 0) {
    echo "\nSkipped tests:\n";
    foreach ($skippedTests as $test) {
        echo "- {$test}\n";
    }
}

echo "========================\n";
