// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This explicit include allows a preprocessing script to expand
// the SL_CONFIG_PROPERTIES macro into individual CONFIG_ENTRY
// macros for the fabric setting CSV script to parse.
// (see src\prod\src\scripts\preprocess_config_macros.pl)
// 
#include "../../ServiceModel/data/txnreplicator/SLConfigMacros.h"

namespace Management
{
    class ManagementConfig 
        : public Common::ComponentConfig
        , public Reliability::ReplicationComponent::REConfigBase
        , public TxnReplicator::TRConfigBase
        , public TxnReplicator::SLConfigBase
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ManagementConfig, "Management")

        // Connection string to the Root for ImageStore
        PUBLIC_CONFIG_ENTRY(Common::SecureString, L"Management", ImageStoreConnectionString, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
        // The minimum transfer rate between the cluster and ImageStore. This value is used to determine the timeout when accessing the external ImageStore. 
        // Change this value only if the latency between the cluster and ImageStore is high to allow more time for the cluster to download from the external ImageStore.
        PUBLIC_CONFIG_ENTRY(int, L"Management", ImageStoreMinimumTransferBPS, 1024, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of worker threads in parallel
        PUBLIC_CONFIG_ENTRY(int, L"Management", AzureStorageMaxWorkerThreads, 25, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of concurrent connections to azure storage
        PUBLIC_CONFIG_ENTRY(int, L"Management", AzureStorageMaxConnections, 5000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Timeout for xstore operation to complete
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Management", AzureStorageOperationTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // This configuration allows us to enable or disable caching.
        PUBLIC_CONFIG_ENTRY(bool, L"Management", ImageCachingEnabled, true, Common::ConfigEntryUpgradePolicy::Static);
        // This configuration allows us to enable or disable checksum validation during application provisioning.
        PUBLIC_CONFIG_ENTRY(bool, L"Management", DisableChecksumValidation, false, Common::ConfigEntryUpgradePolicy::Static);
        // This configuration enables or disables server side copy of application package on the ImageStore during application provisioning.
        PUBLIC_CONFIG_ENTRY(bool, L"Management", DisableServerSideCopy, false, Common::ConfigEntryUpgradePolicy::Static);
        // Folder name for the ImageCache for a node.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Management", ImageCacheDirectory, L"ImageCache", Common::ConfigEntryUpgradePolicy::Static);
        // Folder name for the application deployment folder. This is relative to the work folder for the node.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Management", DeploymentDirectory, L"Applications", Common::ConfigEntryUpgradePolicy::Static);
        // Enable deployment of apps inside FabricDataRoot folder.
        INTERNAL_CONFIG_ENTRY(bool, L"Management", EnableDeploymentAtDataRoot, false, Common::ConfigEntryUpgradePolicy::NotAllowed);
        // Folder name for the fabric upgrade folder. This is relative to the work folder for the node.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Management", FabricUpgradeDeploymentDirectory, L"FabricUpgrade", Common::ConfigEntryUpgradePolicy::Static);
        // Allows the ImageStoreConnectionString to be modified during FabricUpgrade
        INTERNAL_CONFIG_ENTRY(bool, L"Management", AllowImageStoreConnectionStringChange, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The node will skip waiting cluster upgrade complete during system service initialization when this setting is true
        INTERNAL_CONFIG_ENTRY(bool, L"Management", SkipUpgradeWaitOnSystemServiceInitialization, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The processing interval used by the infrastructure task processing state machine
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", InfrastructureTaskProcessingInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The TargetReplicaSetSize for ClusterManager
        PUBLIC_CONFIG_ENTRY(int, L"ClusterManager", TargetReplicaSetSize, 7, Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The MinReplicaSetSize for ClusterManager
        PUBLIC_CONFIG_ENTRY(int, L"ClusterManager", MinReplicaSetSize, 3, Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The ReplicaRestartWaitDuration for ClusterManager
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", ReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The QuorumLossWaitDuration for ClusterManager
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The StandByReplicaKeepDuration for ClusterManager
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", StandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The PlacementConstraints for ClusterManager
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ClusterManager", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);
        // Uses TStore for persisted stateful storage when set to true
        INTERNAL_CONFIG_ENTRY(bool, L"ClusterManager", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);

        // The frequency of polling for application upgrade status. This value determines the rate of update for any GetApplicationUpgradeProgress call
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", UpgradeStatusPollInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The frequency of polling for Fabric upgrade status. This value determines the rate of update for any GetFabricUpgradeProgress call
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", FabricUpgradeStatusPollInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The frequency of checking health status for monitored application upgrades
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", UpgradeHealthCheckInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The frequency of checking health status for monitored Fabric upgrades
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", FabricUpgradeHealthCheckInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The CM will skip reverting updated default services during application upgrade rollback
        PUBLIC_CONFIG_ENTRY(bool, L"ClusterManager", SkipRollbackUpdateDefaultService, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Enable upgrading default services during application upgrade. Default service descriptions would be overwritten after upgrade.
        PUBLIC_CONFIG_ENTRY(bool, L"ClusterManager", EnableDefaultServicesUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time to wait before starting health checks after post-processing an infrastructure task
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", InfrastructureTaskHealthCheckWaitDuration, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time to observe consecutive passed health checks before post-processing of an infrastructure task finishes successfully. Observing a failed health check will reset this timer.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", InfrastructureTaskHealthCheckStableDuration, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time to spend retrying failed health checks while post-processing an infrastructure task. Observing a passed health check will reset this timer.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", InfrastructureTaskHealthCheckRetryTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time to allow for Image Builder specific timeout errors to return to the client. If this buffer is too small, then the client times out before the server and gets a generic timeout error.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", ImageBuilderTimeoutBuffer, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The minimum global timeout for internally processing operations on ClusterManager
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MinOperationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum global timeout for internally processing operations on ClusterManager. 
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxOperationTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum operation timeout when internally retrying due to timeouts is <Original Timeout> + <MaxTimeoutRetryBuffer>.
        // Additional timeout is added in increments of MinOperationTimeout.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxTimeoutRetryBuffer, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum timeout for internal communications between ClusterManager and other system services (i.e., Naming Service, Failover Manager and etc).
        // This timeout should be smaller than global MaxOperationTimeout (as there might be multiple communications between system components for each client operation)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxCommunicationTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum timeout for data migration recovery operations after a Fabric upgrade has taken place
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxDataMigrationTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum delay for internal retries when failures are encountered
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum exponential delay for internal retries when failures are encountered repeatedly
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", MaxExponentialOperationRetryDelay, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // If ReplicaSetCheckTimeout is set to the maximum value of DWORD, then it's overridden with the value of this config for the purposes of rollback. The value used for rollforward is never overridden.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", ReplicaSetCheckTimeoutRollbackOverride, Common::TimeSpan::FromSeconds(1200), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Thread count throttle for Image Builder proxy job queue on application requests
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", ImageBuilderJobQueueThrottle, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Thread count throttle for Image Builder proxy job queue on application upgrade requests. If set to a value greater than zero, then all application upgrades will go through a separate job queue. Otherwise, application upgrades will share the same job queue as all other application requests.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", ImageBuilderUpgradeJobQueueThrottle, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Delays execution of Image Builder proxy job queue items (mainly for testing purposes)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", ImageBuilderJobQueueDelay, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Interval at which CM will poll for operation progress information from Image Builder (<= 0 to disable)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", ImageBuilderProgressTrackingInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Maximum application type name string allowed when provisioning (depends on the underlying local store)
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", MaxApplicationTypeNameLength, 256, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Maximum application type version string allowed when provisioning (depends on the underlying local store)
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", MaxApplicationTypeVersionLength, 256, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Maximum service type name string allowed when provisioning (depends on the underlying local store)
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", MaxServiceTypeNameLength, 256, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Retry count if the ImageStore becomes unavailable during application or Fabric upgrade. The retry interval is MaxOperationRetryDelay.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", ImageStoreErrorDuringUpgradeRetryCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The CM will attempt to archive the Image Builder temp working directory on validation errors.
        // The archives will still be cleaned up by new CM primaries.
        INTERNAL_CONFIG_ENTRY(bool, L"ClusterManager", DisableImageBuilderDirectoryArchives, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The CM will automatically perform a baseline cluster upgrade if possible when this setting is true.
        INTERNAL_CONFIG_ENTRY(bool, L"ClusterManager", EnableAutomaticBaseline, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Timeout used for uploading and provisioning when performing a self-baseline upgrade
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", AutomaticBaselineTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", CompactionThresholdInMB, 5 * 1024, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Timeout used for stopping retrying and failing application & service operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager", RolloutFailureTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max number of threads that the ClusterManager can use to process Naming requests in parallel. 0 represents the number of CPUs.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", NamingJobQueueThreadCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max size of the ClusterManager job queue that handles Naming requests. Determines how many requests can be queued to be processed on job queue threads.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", NamingJobQueueSize, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max number Naming work items that can be started in parallel.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager", NamingJobQueueMaxPendingWorkCount, 500, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // These config entries help in mocking out some features so that we can test the docker compose based application deployment path
        // without having a machine that has docker enabled.
        //
        // Indicates that we should use a mock versions of imagebuilder and apptype name/version generator to test docker compose scenarios via fabrictest.
        TEST_CONFIG_ENTRY(bool, L"ClusterManager", TestComposeDeploymentTestMode, false, Common::ConfigEntryUpgradePolicy::Static);

        // These configs are used by Image Builder when generating the package for ContainerGroup app model. 
        // It determines if service fabric runtime information should be propagated to the apps or not.
        INTERNAL_CONFIG_ENTRY(bool, L"ClusterManager/ContainerGroup", RemoveServiceFabricRuntimeAccess, true, Common::ConfigEntryUpgradePolicy::Static);
        //
        // This config controls the replica restart wait duration for the services created with the container group app model.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager/ContainerGroup", CG_ReplicaRestartWaitDurationSeconds, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // This config controls the quorum loss wait duration for the services created with the container group app model.
        INTERNAL_CONFIG_ENTRY(int, L"ClusterManager/ContainerGroup", CG_QuorumLossWaitDurationSeconds, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // This specifies the azure file volume plugin name to use with the container group app model.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ClusterManager/ContainerGroup", AzureFileVolumePluginName, L"sfazurefile", Common::ConfigEntryUpgradePolicy::Static);
        //
        // The number of containers to retain incase of crashes in container group.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ClusterManager/ContainerGroup", CG_ContainersRetentionCount, L"1", Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // The isolation level for containers created in the container group.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ClusterManager/ContainerGroup", CG_IsolationLevel, L"default", Common::ConfigEntryUpgradePolicy::Dynamic);
        // The default health check stable duration for single instance application upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager/ContainerGroup", SBZ_HealthCheckStableDuration, Common::TimeSpan::FromMinutes(2), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The default health check retry duration for single instance application upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager/ContainerGroup", SBZ_HealthCheckRetryDuration, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The default upgrade timeout for single instance application upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager/ContainerGroup", SBZ_UpgradeTimeout, Common::TimeSpan::FromMinutes(90), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The default upgrade domain timeout for single instance application upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager/ContainerGroup", SBZ_UpgradeDomainTimeout, Common::TimeSpan::FromMinutes(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The default replica set check timeout for single instance application upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ClusterManager/ContainerGroup", SBZ_ReplicaSetCheckTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Azure Only: The Azure blob storage account (connection string) for uploading application log collection
        DEPRECATED_CONFIG_ENTRY(Common::SecureString, L"Management", MonitoringAgentStorageAccount, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
        // Azure Only: The maximum quota for local directory used for buffering the logs.
        DEPRECATED_CONFIG_ENTRY(int, L"Management", MonitoringAgentDirectoryQuota, 1024, Common::ConfigEntryUpgradePolicy::Static); // in MB
        // Azure Only: The transfer interval for application logs to Azure blob storage
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Management", MonitoringAgentTransferInterval, Common::TimeSpan::FromMinutes(10), Common::ConfigEntryUpgradePolicy::Static);

        // The period at which the clean up of expired entries is executed. 
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreCleanupInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        // The minimum time an entity is kept in the store after is marked for deletion. 
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreCleanupGraceInterval, Common::TimeSpan::FromSeconds(30*60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The minimum time an invalidated entity is kept in memory by the health manager after it is removed from store.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreInvalidatedEntityGraceInterval, Common::TimeSpan::FromSeconds(10*60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The time an entity can be kept without a corresponding system report.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreEntityWithoutSystemReportKeptInterval, Common::TimeSpan::FromSeconds(60*60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The time a node entity can be kept without a corresponding system report.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreNodeEntityInvalidationDuration, Common::TimeSpan::FromSeconds(60*5), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of threads that can be used to process entity events
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", HealthProcessingQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(-1));
        // The delay before retrying store operations
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", HealthStoreReadRetryInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Static);
        // The maximum number of reports that can be accepted for processing.
        // Used if EnableMaxPendingHealthReportSizeEstimation is false.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxPendingHealthReportCount, 8192, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));
        // If enabled, Health Manager uses MaxPendingHealthReportSizeBytes to determine whether incoming reports can be accepted for processing.
        // If disabled, Health Manager uses MaxPendingHealthReportCount instead.
        INTERNAL_CONFIG_ENTRY(bool, L"HealthManager", EnableMaxPendingHealthReportSizeEstimation, true, Common::ConfigEntryUpgradePolicy::Static);
        // The maximum size of queued reports that can be accepted for processing, in bytes. Default: 50 MB.
        // Used if EnableMaxPendingHealthReportSizeEstimation is true.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxPendingHealthReportSizeBytes, 52428800, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of queries that can be accepted for processing
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxPendingHealthQueryCount, 8192, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(0));
        // The maximum number of cleanup job items that can be queued for processing
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxPendingHealthCleanupJobCount, 8192, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));
        // The maximum message size for health related messages.
        // DOS attack alleviation
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxMessageSize, 16*1024*1024, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(0)); 
        // The fraction of MaxMessageSize to use as the available buffer limit when calculating how much data
        // to put in a single message (should be in the range [0.0, 1.0])
        INTERNAL_CONFIG_ENTRY(double, L"HealthManager", MessageContentBufferRatio, 0.75, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));
        // The maximum number of job items that can be executed in the same transaction by an entity
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxEntityJobItemBatchCount, 32, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));
        // The time close replica waits for the job queue to finish before bringing down the process
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", MaxCloseJobQueueWaitDuration, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanGreaterThan(Common::TimeSpan::FromMinutes(3)));
        // Enable extra tracing for the query job queue, to show thread ids for processing threads.
        INTERNAL_CONFIG_ENTRY(bool, L"HealthManager", EnableQueryJobQueueTraceProcessingThreads, false, Common::ConfigEntryUpgradePolicy::Static);
        // Configures whether the health manager should check periodically if the in-memory cache is consistent with the data in the persisted health store. Enabled by default.
        INTERNAL_CONFIG_ENTRY(bool, L"HealthManager", EnableHealthCacheConsistencyCheck, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The minimum interval at which the health manager checks the consistency of its in-memory data against the store persisted data.
        // Every HealthStoreCleanupInterval, the cleanup timer checks whether it should perform the consistency check.
        // The consistency check is done at a random interval between min and max, so the entity checks are staggered to decrease performance impact on health manager functionality.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", MinHealthCacheConsistencyCheckInterval, Common::TimeSpan::FromMinutes(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum interval at which the health manager checks the consistency of its in-memory data against the store persisted data.
        // Every HealthStoreCleanupInterval, the cleanup timer checks whether it should perform the consistency check.
        // The consistency check is done at a random interval between min and max, so the entity checks are staggered to decrease performance impact on health manager functionality.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", MaxHealthCacheConsistencyCheckInterval, Common::TimeSpan::FromMinutes(240), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum duration of the health statistics evaluation that prevents caching.
        // If the estimation takes more than this duration, the health manager caches the statistics and returns them for a period of time.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", MaxStatisticsDurationBeforeCaching, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum interval for the cache statistics update.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", MaxCacheStatisticsTimerInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum size for the unhealthy evaluations. If the evaluations have many children, they are automatically trimmed.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxUnhealthyEvaluationsSizeBytes, 64 * 1024, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));
        // The maximum number of health reports that an entity can have before raising concerns about the watchdog's health reporting logic.
        // Each health entity is supposed to have a relatively small number of health reports. If the report count goes above this number, there may be issues with the watchdog's implementation.
        // An entity with too many reports is flagged through a Warning health report when the entity is evaluated.
        PUBLIC_CONFIG_ENTRY(int, L"HealthManager", MaxSuggestedNumberOfEntityHealthReports, 500, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));
        // The maximum number of health reports that can be batched in a transaction. If an entity has more reports, delete entity could get stuck because the max replication message size is reached.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager", MaxEntityHealthReportsAllowedPerTransaction, 32000, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(0));

        // Artificial delay added to the cluster health evaluation to test cluster health caching.
        TEST_CONFIG_ENTRY(Common::TimeSpan, L"HealthManager", StatisticsDurationOffset, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The period of time when generated simple transactions are batched. To disable batching, pass 0.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreCommitBatchingPeriod, 50, Common::ConfigEntryUpgradePolicy::Static);
        // The limit for simple transaction batch. When batched replications size reach this limit, the store will start a new group for new simple transaction.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreCommitBatchingSizeLimitInKB, 256, Common::ConfigEntryUpgradePolicy::Static);
        // When the number of pending completion transaction is less or equal the low watermark, new simple transactions will not be batched. -1 to disable.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreTransactionLowWatermark, 100, Common::ConfigEntryUpgradePolicy::Static);
        // When the number of pending completion transaction is greater or equal the high watermark, batching period will be extended every time the period elapses. -1 to disable.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreTransactionHighWatermark, -1, Common::ConfigEntryUpgradePolicy::Static);
        // The batch period extension. When batching period needs to be extended, it will be extended this much every time.  0 means extending CommitBatchingPeriod.
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreCommitBatchingPeriodExtension, 0, Common::ConfigEntryUpgradePolicy::Static);
        // The store will throttle operations once the number of operations in the replication queue reaches this value
        INTERNAL_CONFIG_ENTRY(int64, L"HealthManager/Store", HealthStoreThrottleOperationCount, 0, Common::ConfigEntryUpgradePolicy::Static);
        // The store will throttle operations once the memory utilization (bytes) of the replication queue reaches this value
        INTERNAL_CONFIG_ENTRY(int64, L"HealthManager/Store", HealthStoreThrottleQueueSizeBytes, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        // DEPRECATED - Not being consumed by the code and present just for upgrade compatibility
        INTERNAL_CONFIG_ENTRY(int, L"HealthManager/Store", HealthStoreTransactionThrottle, 4096, Common::ConfigEntryUpgradePolicy::Static);

        // Cluster health evaluation policy: warnings are treated as errors
        PUBLIC_CONFIG_ENTRY(bool, L"HealthManager/ClusterHealthPolicy", ConsiderWarningAsError, false, Common::ConfigEntryUpgradePolicy::Static);
        // Cluster health evaluation policy: maximum percent of unhealthy nodes allowed for the cluster to be healthy
        PUBLIC_CONFIG_ENTRY(int, L"HealthManager/ClusterHealthPolicy", MaxPercentUnhealthyNodes, 0, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<int>(0, 100));
        // Cluster health evaluation policy: maximum percent of unhealthy applications allowed for the cluster to be healthy
        PUBLIC_CONFIG_ENTRY(int, L"HealthManager/ClusterHealthPolicy", MaxPercentUnhealthyApplications, 0, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<int>(0, 100));

        // Cluster upgrade health evaluation policy: maximum percent of delta unhealthy nodes allowed for the cluster to be healthy
        PUBLIC_CONFIG_ENTRY(int, L"HealthManager/ClusterUpgradeHealthPolicy", MaxPercentDeltaUnhealthyNodes, 10, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<int>(0, 100));
        // Cluster upgrade health evaluation policy: maximum percent of delta of unhealthy nodes in an upgrade domain allowed for the cluster to be healthy
        PUBLIC_CONFIG_ENTRY(int, L"HealthManager/ClusterUpgradeHealthPolicy", MaxPercentUpgradeDomainDeltaUnhealthyNodes, 15, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<int>(0, 100));

        // Enables or disables the automatic cleanup of application package on successful provision.
        PUBLIC_CONFIG_ENTRY(bool, L"Management", CleanupApplicationPackageOnProvisionSuccess, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Enables or disables the automatic cleanup of unused application types.
        PUBLIC_CONFIG_ENTRY(bool, L"Management", CleanupUnusedApplicationTypes, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Maximum number of latest unused application types/versions to be skipped during application type cleanup.
        PUBLIC_CONFIG_ENTRY(int, L"Management", MaxUnusedAppTypeVersionsToKeep, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The cleanup interval for allowed for unregister application type during automatic application type cleanup
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Management", AutomaticUnprovisionInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Enables or disables the periodic automatic cleanup of unused application types.
        INTERNAL_CONFIG_ENTRY(bool, L"Management", PeriodicCleanupUnusedApplicationTypes, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The initial cleanup interval when the CM comes up
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Management", InitialPeriodicAppTypeCleanupInterval, Common::TimeSpan::FromMinutes(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The periodic cleanup interval for after the initial cleanup by CM
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Management", PeriodicAppTypeCleanupInterval, Common::TimeSpan::FromMinutes(24 * 60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Enables or disables the trigger of automatic app type cleanup on provision success.
        TEST_CONFIG_ENTRY(bool, L"Management", TriggerAppTypeCleanupOnProvisionSuccess, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        /* Primary and secondary replication queues: limit memory to 500 MB, ignore the number of items in the queue */
        RE_INTERNAL_CONFIG_PROPERTIES(L"ClusterManager/Replication", 50, 0, 524288000, 0, 524288000);
        TR_INTERNAL_CONFIG_PROPERTIES(L"ClusterManager/TransactionalReplicator2");
        SL_CONFIG_PROPERTIES(L"ClusterManager/SharedLog");

    public:
        ServiceModel::ClusterHealthPolicy GetClusterHealthPolicy() const;
        ServiceModel::ClusterUpgradeHealthPolicy GetClusterUpgradeHealthPolicy() const;

        // ISSUE: Is there a better way of exposing this?
        void GetSections(Common::StringCollection & sectionNames) const;
        void GetKeyValues(std::wstring const & section, Common::StringMap & entries) const;

    private:
        static Common::GlobalWString const ApplicationTypeMaxPercentUnhealthyApplicationsPrefix;
    };
}
