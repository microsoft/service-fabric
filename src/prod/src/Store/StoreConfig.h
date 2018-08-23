// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class StoreConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(StoreConfig, "Store");

        //
        // Settings that are related to performance
        //
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", DatabasePageSizeInKB, 8, Common::ConfigEntryUpgradePolicy::NotAllowed);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", VerPageSizeInKB, 8, Common::ConfigEntryUpgradePolicy::NotAllowed);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseLogFileSizeInKB, 5120, Common::ConfigEntryUpgradePolicy::NotAllowed);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseLogBufferSizeInKB, 1024, Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EsePoolMinSize, 100, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EsePoolAdjustmentSize, 10, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EsePoolEvictionPeriodInSeconds, 900, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxAsyncCommitDelayInMilliseconds, 200, Common::ConfigEntryUpgradePolicy::Static);

        // This is Linux only, for async commit job queue. We do not #if out this line for windows as it is required for generating config file
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MinAsyncCommitThreadNumber, 12, Common::ConfigEntryUpgradePolicy::Static);

        //
        // This setting is used to reduce the chance ESE runs out of buffer pages
        // It is a per instance setting. Each ESE instance will have this many pages
        // in buffer pool as a minimum.
        //
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseMinDatabasePagesInBufferPoolCache, 64, Common::ConfigEntryUpgradePolicy::Static);

        //
        // Internal retry counts and delays
        //
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", DeleteDatabaseRetryCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", DeleteDatabaseRetryDelayMilliseconds, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", OpenDatabaseRetryCount, 30, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", OpenDatabaseRetryDelayMilliseconds, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", SwapDatabaseRetryCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", SwapDatabaseRetryDelayMilliseconds, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", InvalidSessionThreadRetryDelayMilliseconds, 100, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", InvalidSessionThreadRetryCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", SecondaryApplyRetryCount, 300, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", SecondaryApplyRetryDelayMilliseconds, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // Settings that are to control ESE resource use, intended to catch bugs
        //
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxJetInstances, 500, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxSessions, 16384, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxCursors, 16384, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxOpenTables, 49152, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxVerPages, 16384, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", CheckpointDepthMaxInMB, 512, Common::ConfigEntryUpgradePolicy::Static);

        //
        // Value of -1 for int values uses ESE defaults
        //
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxCacheSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MinCacheSizeInMB, -1, Common::ConfigEntryUpgradePolicy::Static);

        // MaxDefragFrequencyInMinutes <= 0 to disable
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", MaxDefragFrequencyInMinutes, 60 * 24, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", DefragRetryDelayInSeconds, 30, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", DefragThresholdInMB, 500, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Offline compaction occurs during open if the database file size exceeds this threshold (<=0 to disable).
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", CompactionThresholdInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Value lengths less than or equal this threshold will be written to ESE using the JET_bitSetIntrinsicLV flag (<=0 to disable and use the ESE default of 1024 bytes).
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", IntrinsicValueThresholdInBytes, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The existing log file size is determined by checking the filesize of this file (current log file by default)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"EseStore", ComputeLogFileSizeSource, L"edb.log", Common::ConfigEntryUpgradePolicy::Dynamic);

        // Enables in-place replace of value (versus insert/delete) during update operation. 
        // Maps to JET_bitSetOverwriteLV on the local ESE store. 
        INTERNAL_CONFIG_ENTRY(bool, L"EseStore", EnableOverwriteOnUpdate, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // Enable the background database maintenance task that does ECC checks to fix bit errors, etc.
        // Value of -1 for int values uses ESE defaults
        //
        // Default for EseEnableScanSerialization = 20ms
        // Default for EseScanIntervalMinInSeconds = 1 * 24 * 60 * 60 = 1 day
        // Default for EseScanIntervalMaxInSeconds = 7 * 24 * 60 * 60 = 7 days
        //
        INTERNAL_CONFIG_ENTRY(bool, L"EseStore", EseEnableBackgroundMaintenance, true, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(bool, L"EseStore", EseEnableScanSerialization, false, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseScanThrottleInMilleseconds, -1, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseScanIntervalMinInSeconds, -1, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"EseStore", EseScanIntervalMaxInSeconds, -1, Common::ConfigEntryUpgradePolicy::Static);

        //
        // Debugging
        //
        INTERNAL_CONFIG_ENTRY(bool, L"EseStore", AssertOnFatalError, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // ReplicatedStore
        //
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", FabricTimePersistInterval, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", FabricTimeRefreshTimeoutValue, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Tombstone cleanup is triggered on delete when the total number of tombstones exceeds this value.
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", TombstoneCleanupLimit, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Tombstone cleanup will not attempt to cleanup more than this number of tombstones per cleanup round. Each cleanup round attempts
        // to reduce the number of tombstones by half.
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", MaxTombstonesPerCleanup, 200000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Enables optimized tombstone cleanup algorithm
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableTombstoneCleanup2, true, Common::ConfigEntryUpgradePolicy::Static);
        // Maximum number of tombstones to migrate per transaction batch (if needed)
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", TombstoneMigrationBatchSize, 200000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The transaction batch size to use when replaying on a secondary replica in FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_REBUILD mode (dynamic, but requires replica restart)
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", DatabaseRebuildBatchSizeInBytes, 2 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Dynamic);
        
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", ThrottleCountersRefreshIntervalInOperationCount, 256, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", ThrottleCountersRefreshInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", TargetCopyOperationSize, 2 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(double, L"ReplicatedStore", CopyOperationSizeRatio, 0.80, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", MaxEpochHistoryCount, 200, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableRepairPolicy, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", RepairPolicyTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableFileStreamFullCopy, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", MaxFileStreamFullCopyWaiters, -1, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", FileStreamFullCopyRetryDelay, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", WaitForCopyLsnRetryDelayInMillis, 500, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", MaxWaitForCopyLsnRetry, 120, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableSystemServiceFlushOnDrain, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableUserServiceFlushOnDrain, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableEndOfStreamAckOverride, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", AssertOnOpenSharingViolation, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", SlowCommitTraceThreshold, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", SlowCommitCountThreshold, 60, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", SlowCommitTimeThreshold, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableSlowCommitTest, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", DefaultHealthReportTimeToLive, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Tracks lifecycle operations and asserts if this timeout expires waiting for the operation to complete (0 to disable)
        //
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", LifecycleAssertTimeout, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", LifecycleOpenAssertTimeout, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", LifecycleChangeRoleAssertTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", LifecycleCloseAssertTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", LifecycleCommitAssertTimeout, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Threshold (in seconds of elapsed time) over which KVS enumerations will emit detailed performance traces. Only active API calls
        // are included as elapsed time.
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", UserServiceEnumerationPerfTraceThreshold, 5, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Dynamic for new replicas only (existing replicas will continue to use the old value until re-opened)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", TransactionDrainTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Throttles the number of concurrent local store open calls per process (0 to disable the throttle)
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", OpenLocalStoreThrottle, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Ignores the flag passed into open indicating whether or not the database is expected to exist
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", IgnoreOpenLocalStoreFlag, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", LogTruncationIntervalInMinutes, 45, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Maximum number of keys per transaction allowed during database migration
        INTERNAL_CONFIG_ENTRY(int, L"ReplicatedStore", MigrationBatchSize, 100, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Timeout used for migration stream commits
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", MigrationCommitTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Retry delay used to retry migration stream write conflicts
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", MigrationConflictRetryDelay, Common::TimeSpan::FromMilliseconds(100), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Name of the directory or container for storing backups after migration completes before swapping databases
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ReplicatedStore", MigrationBackupDirectory, L"migration-backups", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Name of the connection string override used when uploading backups (file: and xstore: schemes are supported)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ReplicatedStore", MigrationBackupConnectionString, L"", Common::ConfigEntryUpgradePolicy::Dynamic);

        // When true, overrides individual system service configurations to enable TStore in all supported system services (user services are unaffected)
#ifdef PLATFORM_UNIX
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableTStore, true, Common::ConfigEntryUpgradePolicy::NotAllowed);
#else
        INTERNAL_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableTStore, false, Common::ConfigEntryUpgradePolicy::NotAllowed);
#endif

        // When true, forces all KVS application services and FabricTest services to use TStore for testing purposes
#ifdef PLATFORM_UNIX
        TEST_CONFIG_ENTRY(bool, L"ReplicatedStore", Test_EnableTStore, true, Common::ConfigEntryUpgradePolicy::NotAllowed);
#else
        TEST_CONFIG_ENTRY(bool, L"ReplicatedStore", Test_EnableTStore, false, Common::ConfigEntryUpgradePolicy::NotAllowed);
#endif

        // Retry delay during TStore initialization
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", TStoreInitializationRetryDelay, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The lock timeout passed to TStore operations
        TEST_CONFIG_ENTRY(Common::TimeSpan, L"ReplicatedStore", TStoreLockTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
        
        // Enables reference tracking to help debug replica leaks in FabricTest runs
        TEST_CONFIG_ENTRY(bool, L"ReplicatedStore", EnableReferenceTracking, false, Common::ConfigEntryUpgradePolicy::Dynamic);
    };

    typedef std::shared_ptr<StoreConfig> StoreConfigSPtr;
}
