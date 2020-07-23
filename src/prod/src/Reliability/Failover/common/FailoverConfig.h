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
#include "../../../ServiceModel/data/txnreplicator/SLConfigMacros.h"

namespace Reliability
{
    class FailoverConfig :
        public Common::ComponentConfig,
        public Reliability::ReplicationComponent::REConfigBase,
        public TxnReplicator::TRConfigBase,
        public TxnReplicator::SLConfigBase
    {
        #pragma region Failover
        DECLARE_SINGLETON_COMPONENT_CONFIG(FailoverConfig, "Failover")

        // Retry interval for node up messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", NodeUpRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Retry interval for LFUM upload messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", LfumUploadRetryInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Retry interval for Change Notification Messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", ChangeNotificationRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Maximum jitter allowed in the retry intervals defined above
        // Must be less than 1.0 and greater than 0.0
        INTERNAL_CONFIG_ENTRY(double, L"Failover", MaxJitterForSendToFMRetry, 0.3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // When sending messages from nodes to the FM, if an acknowledgment is not received within this timeout, then schedule
        // a retry.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", SendToFMTimeout, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);

        // When sending a message from the FM to the node, if an acknowledgment is not received within this timeout, then retry
        // sending the message
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", SendToNodeTimeout, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);

        // When sending a routed message, if an ack is not received within this timeout, then retry the message
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", RoutingRetryTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);

        // The interval after which the Old state of an FT will be traced out
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Failover", FTDetailedTraceInterval, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This configuration has been deprecated. It is not being consumed by the code and present just for upgrade compatibility.
        // It was used as the retry interval for routing layer retries for some of the Failover messages. Now, higher layers manage retries
        // themselves and this entry is no longer needed.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Failover", SendToFMRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        #pragma endregion

        #pragma region FM
        // When the cluster is first starting up, there will be many nodes joining.  However, even after the MinReplicaSetSize of
        // the cluster is met, there may still be many nodes which are in the process of joining.  In order to prevent placing services
        // when there are still nodes joining, once the FM sees a node come up it will wait for this duration before declaring the ring
        // stable. If during this interval the FM observes another node come up, it will reset this timer.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ClusterStableWaitDuration, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When opening up the local store that the FM uses, the FM waits this long for a response
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", StoreOpenTimeout, Common::TimeSpan::FromSeconds(300.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // If opening the store fails, then the FM waits this long before retrying
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", StoreRetryInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The PeriodicStateScanInterval determines how often the FM background thread activates to scan for changes and kick off actions
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PeriodicStateScanInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the FM sends a particular action for a specific replica, it starts this timer.  Before it expires, the FM will not send additional
        // actions to the replica
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MinActionRetryIntervalPerReplica, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the FM sends a particular action for a specific replica, if the state has not been updated for
        // this interval, message will be retried with this interval.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MaxActionRetryIntervalPerReplica, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Timer which determines how often the FM scans its state to clean unnecessary information such as service tombstones (Default: 5min)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PeriodicStateCleanupScanInterval, Common::TimeSpan::FromSeconds(300.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This timer defines the maximum amount of time that the FM will keep track of replicas which are down
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", OfflineReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When a replica is removed from the system, the FM creates a tombstone record for that replica, which are kept for the DroppedReplicaKeepDuration
        // While the replica is tombstoned the FM can distinguish between replicas which existed but have been dropped and replicas which do not exist.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", DroppedReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::Dynamic);
        // When a replica is deleted on RA, the FM still keeps the replica for DeletedReplicaKeepDuration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", DeletedReplicaKeepDuration, Common::TimeSpan::FromSeconds(600.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // When a failover unit is in the InBuild cache for this duration, FM will delete it.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", InBuildFailoverUnitKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MinRebuildRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MaxRebuildRetryInterval, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the FM attempts to acquire a lock on a resource like the local FM store, then wait at least this much time to see if the lock can be obtained
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", LockAcquireTimeout, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Interval used to trace updated stateless replicas. All replicas last updated in this interval, down and with flags != none will be printed.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", StatelessPartitionTracingInterval, Common::TimeSpan::FromSeconds(600.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the cluster is initially started up, the FM will wait for this many nodes to report themselves up before it begins
        // placing other services, including the system services like naming.  Increasing this value increases the time it takes a cluster
        // to start up, but prevents the early nodes from becoming overloaded and also the additional moves that will be necessary as more nodes
        // come online.  This value should generally be set to some small fraction of the initial cluster size.
        PUBLIC_CONFIG_ENTRY(int, L"FailoverManager", ExpectedClusterSize, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

        // If the number of nodes in system go below this value then placement, load balancing, and failover is stopped.
        PUBLIC_CONFIG_ENTRY(int, L"FailoverManager", ClusterPauseThreshold, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

        // This is the target number of FM replicas that Windows Fabric will maintain.  A higher number results in higher reliability of the FM
        // data, with a small performance trade-off
        PUBLIC_CONFIG_ENTRY(int, L"FailoverManager", TargetReplicaSetSize, 7, Common::ConfigEntryUpgradePolicy::NotAllowed);

        // This is the minimum replica set size for the FM.  If the number of active FM replicas drops below this value,
        // the FM will reject changes to the cluster until at least the min number of replicas is recovered
        PUBLIC_CONFIG_ENTRY(int, L"FailoverManager", MinReplicaSetSize, 3, Common::ConfigEntryUpgradePolicy::NotAllowed);

        // This is the ReplicaRestartWaitDuration for the FMService
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::NotAllowed);

        // When the FM enters quorum loss, it first waits this amount of time before starting a full rebuild.  The purpose of
        // waiting for this period of time is to give other nodes and FM replicas time to come up.  Increasing this time will help
        // prevent the chances of accidental dataloss during FM rebuild, at the cost of some additional startup/recovery time
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", FullRebuildWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::NotAllowed);

        // This is the StandByReplicaKeepDuration for the FMService
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", StandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::NotAllowed);

        // Any placement constraints for the failover manager replicas
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FailoverManager", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);

        // Uses TStore for persisted stateful storage when set to true
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);

        // If set to true, FM ensures that any new node is at the correct version before it is admitted to the system.
        // If the node is not a the correct version, it is upgraded to the right version.
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", IsFabricUpgradeGatekeepingEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // This is the expected duration for a node to be upgraded during Windows Fabric upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ExpectedNodeFabricUpgradeDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This is the expected duration for all the replicas to be upgraded on a node during application upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ExpectedReplicaUpgradeDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This is the expected duration for a node to complete deactivation in.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ExpectedNodeDeactivationDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // If set to true, replicas with a target replica set size of 1 will be permitted to move during upgrade.
        PUBLIC_CONFIG_ENTRY(bool, L"FailoverManager", IsSingletonReplicaMoveAllowedDuringUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // If set to true, move/swap replica to original location after upgrade
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", RestoreReplicaLocationAfterUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specifies the duration for which FM waits for PLB to trigger a swap-primary when deactivating a node intent
        // Restart. This so that all the all the primary replicas are swapped out of the node before closing the
        // replicas. If PLB is unable to start the swap and this much time has elapsed, FM proceeds with node deactivation.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", SwapPrimaryRequestTimeout, Common::TimeSpan::FromSeconds(60.0 * 10), Common::ConfigEntryUpgradePolicy::Dynamic);

        //Specifies the duration for which FM waits for an up replica to move out to a different node during remove node or remove data deactivation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RemoveNodeOrDataUpReplicaTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);

        //Specifies whether FM should send remove instance to RA for node/data removal during safety checks
        //If set to true the stateless instance close will be initiated after safety checks are done
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckComplete, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specified if FM should perform a stronger safety check for upgrades and node deactivations.
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", IsStrongSafetyCheckEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // A seed node can only function as arbitrator after LeaseDuration+ArbitrationTimeout.
        // During upgrade or node deactivation, FM should only consider a seed node that is up
        // after the above duration to be effective.  To account for clock skew and configuration update,
        // the duration is multiplied by this factor as the interval for which FM will wait.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", SeedNodeWaitSafetyFactor, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The EnableConstraintCheckDuringFabricUpgrade determines should PLB try to fix constraint check violations during Fabric upgrade.
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", EnableConstraintCheckDuringFabricUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Determines the number of threads that the FM background task should use.
        // The default value of 0 indicates that the FM should use a number of threads
        // equal to the number of cores on the machine
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", BackgroundThreadCount, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of threads that the FM should use for FailoverUnit specific message processing and PLB action consumption
        // The default value of 0 indicates that the FM should use a number of threads equal to the number of cores on the machine
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", ProcessingQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The size of FailoverUnit job queue
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", ProcessingQueueSize, 10000, Common::ConfigEntryUpgradePolicy::Static);

        // The number of threads that the FM should use for completing the post-commit jobs.
        // The default value of 0 indicates that the FM should use a number of threads equal to the number of cores on the machine
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", CommitQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The number of threads that the FM should use for non-FailoverUnit specific messages
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", CommonQueueThreadCount, 50, Common::ConfigEntryUpgradePolicy::Static);

        // The size of the common job queue
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", CommonQueueSize, 500, Common::ConfigEntryUpgradePolicy::Static);

        // The number of threads that the FM should use for query messages
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", QueryQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The size of the query job queue
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", QueryQueueSize, 500, Common::ConfigEntryUpgradePolicy::Static);

        // When the FM runs, it generates a series of actions which need to be taken for the cluster.  This setting defines the maximum number
        // of actions which the FM will issue in a single iteration of the FM state machine.  Increasing this number will allow the FM to reconfigure
        // the system faster when necessary (no actions will be held back), at the cost of additional resource consumption in terms of messages that
        // are sent from the FM to the replicas and also the concurrent handling of those actions by all of the affected replicas).
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", MaxActionsPerIteration, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the FM broadcasts service locations these broadcasts can fail
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", ServiceLookupTableBroadcastAttemptCount, 2, Common::ConfigEntryUpgradePolicy::Static);

        // Periodically the FM broadcasts changes to the locations of services. This broadcasts are picked up by naming and cached as an optimization.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ServiceLocationBroadcastInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Interval between empty service table update broadcast messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ServiceLookupTableEmptyBroadcastInterval, Common::TimeSpan::FromSeconds(15.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The fraction of MaxMessageSize to use as the available buffer limit when calculating how much data
        // to put in a single message (should be in the range [0.0, 1.0])
        INTERNAL_CONFIG_ENTRY(double, L"FailoverManager", MessageContentBufferRatio, 0.75, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));

        // The FM persistent store is used to store load reports which arrive from replicas and nodes.  Since the load balancing
        // data is best effort, for performance reasons this information can be lazily committed.  The MaxParallelLoadUpdates
        // setting defines the maximum number of load updates that will be applied in parallel.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", MaxParallelLoadUpdates, 3, Common::ConfigEntryUpgradePolicy::Static);

        // When the FM is writing load reports into its persistent store, it should batch together no more than this many load reports
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", MaxLoadUpdateBatchSize, 50, Common::ConfigEntryUpgradePolicy::Static);

        // Timer which determines how often the FM check if there are new load reports and persist them into store
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PeriodicLoadPersistInterval, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval for tracing system services to the admin channel on FM/FMM.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", AdminStateTraceInterval, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Static);

        // The interval for tracing to the DCA channel on FM/FMM.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", DcaTraceInterval, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of services to trace per AdminStateTraceInterval
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", PeriodicServiceTraceCount, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The directory where the FM will store its local data
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FailoverManager", FMStoreDirectory, L".\\", Common::ConfigEntryUpgradePolicy::Static);

        // The filename that the FM should use when creating it's local store
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FailoverManager", FMStoreFilename, L"FM.edb", Common::ConfigEntryUpgradePolicy::Static);

        // The time limit for reconfiguration, after which a warning health report will be initiated
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ReconfigurationTimeLimit, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time limit for building a stateful replica, after which a warning health report will be initiated
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", BuildReplicaTimeLimit, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time limit for creating a stateless instance, after which a warning health report will be initiated
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", CreateInstanceTimeLimit, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time limit for reaching target replica count, after which a warning health report will be initiated
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PlacementTimeLimit, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time limit for rebuilding the partition state, after which a warning health report is initiated
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RebuildPartitionTimeLimit, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The maximum number of replicas to include in the detailed health report description.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", MaxReplicasInHealthReportDescription, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // If a message processing takes more than this time, a warning is traced
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MessageProcessTimeLimit, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // If the PLB update takes more than this time, a warning is traced
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PlbUpdateTimeLimit, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration for which FM keeps track of an unknown node
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", UnknownNodeKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration for which FM keeps track of a node that has had its state removed
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RemovedNodeKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This configuration is the minimum number of seconds FM queues will wait before sending a reject message
        // back to the caller when Enqueue fails because the Queue was full.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", MinSecondsBetweenQueueFullRejectMessages, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This configuration defines if the safety checks should be performed or not for Node Deactivation.
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", PerformSafetyChecksForNodeDeactivationIntentPause, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // If set to true, upgrade domain names are sorted as numbers, e.g., "2" comes before "10"
        INTERNAL_CONFIG_ENTRY(bool, L"FailoverManager", SortUpgradeDomainNamesAsNumbers, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Time that FM will wait for PLB to finish its safety check, if we go beyond this time FM will continue with the upgrade regardless of PLB safety check status
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", PlbSafetyCheckTimeLimit, Common::TimeSpan::FromSeconds(60*60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Max number of nodes which can get upgraded in parallel. The default value of 0 disables node upgrade throttling.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", MaxParallelNodeUpgradeCount, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration for which certain updates requiring persistence are batched.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", LazyPersistWaitDuration, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // When all the primary/secondary replicas are dropped (data loss), we wait for this much duration for all the
        // remaining down replicas to come up. This is to to give a chance to elect the replia with the highest LSN as
        // the new primary.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RecoverOnDataLossWaitDuration, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);

        //Defines an amount of time for an fm rebuild to be deemed as stuck. A health report is created after this threshold has passed.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RebuildHealthReportThreshold, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        //The TTL for Rebuild stuck health reports used by FM and FMM
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RebuildHealthReportTTL, Common::TimeSpan::FromSeconds(40), Common::ConfigEntryUpgradePolicy::Dynamic);

        DEPRECATED_CONFIG_ENTRY(std::wstring, L"FailoverManager", StoreConnectionString, L"", Common::ConfigEntryUpgradePolicy::Static);

        // This configuration has been deprecated. It is not being consumed by the code and present just for upgrade compatibility.
        // Please use DeletedReplicaKeepDuration instead.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", ReplicaTombstoneDuration, Common::TimeSpan::FromSeconds(3600.0 * 10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This configuration is from v1 where the FM used it to schedule an internal retry for replica up messages
        // Since v1 has shipped keeping this around for the FM use case and defining a new entry for RA use case
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReplicaUpMessageRetryInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);

        // This configuration has been deprecated. It is not being consumed by the code and present just for upgrade compatibility.
        // Please use MinRebuildRetryInterval and MaxRebuildRetryInterval instead.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", RebuildRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Static);

        // Max entries a service lookup table update message can contain
        DEPRECATED_CONFIG_ENTRY(int, L"FailoverManager", ServiceLookupTableMessageMaxEntries, 300, Common::ConfigEntryUpgradePolicy::Dynamic);

        // When the FM attempts to access the store and fails, the FM should retry accessing the store
        DEPRECATED_CONFIG_ENTRY(int, L"FailoverManager", StoreRetryAttempt, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        TEST_CONFIG_ENTRY(bool, L"FailoverManager", DummyPLBEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        TEST_CONFIG_ENTRY(bool, L"FailoverManager", DummyPLBRandomPlacementEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        TEST_CONFIG_ENTRY(bool, L"FailoverManager", IsTestMode, false, Common::ConfigEntryUpgradePolicy::Static);
        TEST_CONFIG_ENTRY(bool, L"FailoverManager", DropAllPLBMovements, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        #pragma endregion

        #pragma region FM/Store
        // FMStore will batch simple transactions that are created within this period.  0 means no batching.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", CommitBatchingPeriod, 50, Common::ConfigEntryUpgradePolicy::Static);

        // When batched replications size reach this limit, FMStore will start a new group for new simple transaction.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", CommitBatchingSizeLimitInKB, 256, Common::ConfigEntryUpgradePolicy::Static);

        // When the number of FMStore pending completion transaction <= this, new simple transactions will not be batched.  -1 to disable.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", TransactionLowWatermark, 100, Common::ConfigEntryUpgradePolicy::Static);

        // When the number of FMStore pending completion transaction >= this, batching period will be extended every time the period elapses.  -1 to disable.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", TransactionHighWatermark, -1, Common::ConfigEntryUpgradePolicy::Static);

        // When batching period needs to be extended, it will be extended this much every time.  0 means extending CommitBatchingPeriod.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", CommitBatchingPeriodExtension, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The store will throttle operations once the number of operations in the replication queue reaches this value
        INTERNAL_CONFIG_ENTRY(int64, L"FailoverManager/Store", ThrottleOperationCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The store will throttle operations once the memory utilization (bytes) of the replication queue reaches this value
        INTERNAL_CONFIG_ENTRY(int64, L"FailoverManager/Store", ThrottleQueueSizeBytes, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager/Store", CompactionThresholdInMB, 1024, Common::ConfigEntryUpgradePolicy::Dynamic);

        // This config is no longer consumed in the code and is present only for upgrade compatibility
        DEPRECATED_CONFIG_ENTRY(int, L"FailoverManager/Store", TransactionThrottle, 4096, Common::ConfigEntryUpgradePolicy::Static);
        #pragma endregion

        #pragma region RA

        //Defines an amount of time for a reconfiguration to be deemed as stuck. A health report is created after this threshold has passed.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReconfigurationHealthReportThreshold, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Defines whether failover can mark the replica as down on revoking read write status
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", EnableReplicaDownOnReadWriteStatusRevoke, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Returns whether catchup prior to revoking write status during demote is enabled
        // NOTE: This configuration cannot be made dynamic without code change as the code that runs when catchup op is scheduled may read it as false
        // and the code that runs when catchup op completes may read it as true
        // The same race exists when scheduling UC - it may skip the UC action for providing catchup config and then the config becomes true causing the catchup to happen with UC skipped
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsPreWriteStatusRevokeCatchupEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

        // Determines whether to enable the optimization to perform phase3 and phase4 in parallel
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", EnablePhase3Phase4InParallel, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Message retry interval for ServiceTypeDisabled/Enabled
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", MessageRetryInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Message retry interval for messages related to reconfiguration
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReconfigurationMessageRetryInterval, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Minimum interval between retry of reconfiguration messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", MinimumIntervalBetweenReconfigurationMessageRetry, Common::TimeSpan::FromSeconds(1.5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between two messages to FM (ReplicaUp/ReplicaDown/ReplicaDropped)
        // Two replica up messages will not be sent by RA to FM in less than this interval
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerNodeMinimumIntervalBetweenMessageToFM, Common::TimeSpan::FromSeconds(0.02), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between two messages for the same replica sent by RA to FM
        // Successive messages for the same replica will not be sent in less than this interval
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerReplicaMinimumIntervalBetweenMessageToFM, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The maximum number of replicas in a replica message
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", MaxNumberOfReplicasInMessageToFM, 32, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The retry interval for message to FM (ReplicaUp/ReplicaDown)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FMMessageRetryInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The maximum number of service type to send in one message to fm
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", MaxNumberOfServiceTypeInMessageToFM, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between two messages for the same service type sent by RA to FM
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerServiceTypeMinimumIntervalBetweenMessageToFM, Common::TimeSpan::FromSeconds(45), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between two ServiceType messages to the FM
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerNodeMinimumIntervalBetweenServiceTypeMessageToFM, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The retry interval between two service type messages to FM
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceTypeMessageRetryInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The retry interval between cleanups on the service type update staleness map
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceTypeUpdateStalenessCleanupInterval, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration for which entries should be kept in the service type update staleness map
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceTypeUpdateStalenessEntryKeepDuration, Common::TimeSpan::FromMinutes(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The retry interval for ReplicaDropped message to FM
        // This is separate as it is a message targeted for a single FT and needs a higher retry interval than ReplicaUp/Down
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReplicaDroppedMessageRetryInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The retry interval for message to RAP for single FT messages related to FT Lifecycle
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RAPMessageRetryInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between message retries to RAP
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", MinimumIntervalBetweenRAPMessageRetry, Common::TimeSpan::FromSeconds(1.5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When opening up the local store that the RA uses, the RA waits this long for a response
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RAStoreOpenTimeout, Common::TimeSpan::FromSeconds(120.0), Common::ConfigEntryUpgradePolicy::Static);

        // When opening up the RA local store fails, the RA waits this long before trying again
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RAStoreOpenRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Static);

        // When a service is upgraded, the RA will periodically check to see if it is back up again
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RAUpgradeProgressCheckInterval, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify timespan in seconds. The duration for which the system will wait before terminating service hosts that have replicas that are stuck in close during Application Upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ApplicationUpgradeMaxReplicaCloseDuration, Common::TimeSpan::FromSeconds(900), Common::ConfigEntryUpgradePolicy::Dynamic);

        // SendLoadReportInterval defines the interval to send ReportLoad messages from RAP to RA
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", SendLoadReportInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        // FailoverUnitProxyCleanupInterval defines the interval to cleanup the failover unit proxy objects that were kept around for stateleness check
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FailoverUnitProxyCleanupInterval, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Dynamic);

        // MaxNumberOfLoadReportsPerMessage defines the batch size for ReportLoad messages from RAP to RA
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", MaxNumberOfLoadReportsPerMessage,  500, Common::ConfigEntryUpgradePolicy::Dynamic);

         // ProxyOutgoingMessageRetryTimerInterval defines the timer interval for outgoing messages, like ReportFault, in RAP
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ProxyOutgoingMessageRetryTimerInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // LocalMessageRetryInterval defines the local retry interval for failed messages in RAP
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", LocalMessageRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Static);

        // Used to trace and check for lru status and send that message
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", LastReplicaUpTimerInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // IsHealthReportingEnabled determines whether health reporting in RA is enabled or not
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsHealthReportingEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

        // ServiceApiHealthDuration defines how long do we wait for a service API to run before we report it unhealthy.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceApiHealthDuration, Common::TimeSpan::FromMinutes(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify timespan in seconds. ServiceReconfigurationApiHealthDuration defines how long does Service Fabric wait for a service API to run before we report unhealthy. This applies to API calls that impact availability.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceReconfigurationApiHealthDuration, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // ServiceReconfigurationApiTraceWarningThreshold defines a threshold to wait before tracing warnings for stuck reconfiguration.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ServiceReconfigurationApiTraceWarningThreshold, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // PeriodicApiSlowTraceInterval defines the interval over which slow API calls will be retraced by the API monitor.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PeriodicApiSlowTraceInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration RA will wait after a reconfig has started before which it publishes the endpoint (if it has it) of the primary
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", MaxWaitBeforePublishEndpointDuration, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // LocalHealthReportingTimerInterval defines the interval to check and report health in RAP
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", LocalHealthReportingTimerInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);

        // The maximum number of threads that can be used for processing per failoverunit work
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", FailoverUnitProcessingQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The maximum number of threads that can be used for processing messages received by the RAP. 0 = #cores
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", RAPMessageProcessingQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The maximum number of threads that can be used for processing per messages
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", MessageProcessingQueueThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The maximum time to wait for async ESE transactions to commit
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent/Store", MaxEseCommitWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify timespan in seconds. The duration for which the system will wait before terminating service hosts that have replicas that are stuck in close during node deactivation.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", NodeDeactivationMaxReplicaCloseDuration, Common::TimeSpan::FromSeconds(900), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration at which RA polls to check for the node deactivation to complete
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", NodeDeactivationReplicaCloseCompletionCheckInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify timespan in seconds. The duration for which the system will wait before terminating service hosts that have replicas that are stuck in close.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", GracefulReplicaShutdownMaxDuration, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration at which RA polls to check for replica close during graceful node close to complete
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", GracefulReplicaCloseCompletionCheckInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The amount of time the RA will wait for a persisted replica to be reopened before telling the FM about it
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReopenSuccessWaitInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration after which deleted FTs can be removed
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", DeletedFailoverUnitTombstoneDuration, Common::TimeSpan::FromSeconds(3600.0 * 12), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time to take to scan cleaned up failover units
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PeriodicStateCleanupInterval, Common::TimeSpan::FromSeconds(3600.0 * 3), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The minimum interval between which fts are scanned for cleanup
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", MinimumIntervalBetweenPeriodicStateCleanup, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration for which to wait before checking if all replicas are closed during a fabric upgrade
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FabricUpgradeReplicaCloseCompleteCheckInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify timespan in seconds. The duration for which the system will wait before terminating service hosts that have replicas that are stuck in close during fabric upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FabricUpgradeMaxReplicaCloseDuration, Common::TimeSpan::FromSeconds(900), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration after which a failed download is retried
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FabricUpgradeDownloadRetryInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration after which a failed upgrade is retried
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FabricUpgradeUpgradeRetryInterval, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration after which the validate call to Hosting is retried
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", FabricUpgradeValidateRetryInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding service type registration before replica is dropped
        // This is defined based on a count which is retried depending on the Reopen/Open retry interval and timeout
        // By default it is set to INT_MAX i.e. Infinite
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationMaxRetryThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding a service type registration before replica is restarted
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationRestartThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding a service type registration after which a warning will be reported
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationWarningReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding a service type registration after which an error will be reported
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationErrorReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding service type registration before replica is dropped on the RA
        // This value applies when the RA is reopening a replica for which the FM had requested a drop earlier
        // The RA will give up after 8 hours
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationMaxRetryThresholdAtDrop, 1920, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at finding a service type registration after which a warning will be reported
        // This value applies when the RA is reopening a replica for which the FM had requested a drop earlier
        // The RA will report warning after 1 hour
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ServiceTypeRegistrationWarningReportThresholdAtDrop, 240, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at dropping a replica on a node after which RA will give up
        // With defaults, a total of 40320 attempts will be made which is approximately 7 days, same as replicaReopen
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaCloseFailureMaxRetryThreshold, 40320, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which a warning will be reported to HM
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaCloseFailureWarningReportThreshold, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which an error will be reported to HM
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaCloseFailureErrorReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at dropping an existing replica on a node after which RA will restart the replica
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaCloseFailureRestartThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at deleting a replica on a node after which RA will give up
        // With defaults, a total of 10 minutes will be spent (40 times, 15 seconds each)
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaDeleteFailureMaxRetryThreshold, 40, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which a warning will be reported to HM
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaDeleteFailureWarningReportThreshold, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which an error will be reported to HM
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaDeleteFailureErrorReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which RA will restart the replica
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaDeleteFailureRestartThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at opening a new replica on a node after which RA will give up
        // This is based on a count which is retried every ReplicaOpenMessageRetryInterval seconds
        // With defaults, a total of 10 minutes will be spent (40 times, 15 seconds each)
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaOpenFailureMaxRetryThreshold, 40, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which a warning will be reported to HM
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaOpenFailureWarningReportThreshold, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which an error will be reported to HM
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaOpenFailureErrorReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at reopening an existing replica on a node after which RA will give up
        // This is based on a count which is retried every RAPMessageRetryInterval seconds
        // With defaults, a total of 40320 attempts will be made which is approximately 7 days
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaReopenFailureMaxRetryThreshold, 40320, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which a warning will be reported to HM
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaReopenFailureWarningReportThreshold, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts at open / reopening an existing replica on a node after which RA will restart the replica
        // Defaults to INT_MAX
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaOpenFailureRestartThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Integer. Specify the number of consecutive API failures during primary promotion after which auto-mitigation action (replica restart) will be applied
        PUBLIC_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaChangeRoleFailureRestartThreshold, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Integer. Specify the number of API failures during primary promotion after which warning health report will be raised.
        PUBLIC_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaChangeRoleFailureWarningReportThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of job items per ft after which the scheduler will trace a warning on each new job item that is added
        INTERNAL_CONFIG_ENTRY(int, L"ReconfigurationAgent", SchedulerWarningTraceThreshold, 100, Common::ConfigEntryUpgradePolicy::Static);

        // The duration for which reconfiguration will wait to query replicas for their progress
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RemoteReplicaProgressQueryWaitDuration, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When set to true, the local store will assert on fatal errors (such as log write failures).
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", AssertOnStoreFatalError, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Uses TStore for persisted stateful storage when set to true
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", EnableLocalTStore, false, Common::ConfigEntryUpgradePolicy::Static);
        
        // Use to disable a safety check against accidentally switching local store provider type (EnableLocalTStore)
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", AllowLocalStoreMigration, false, Common::ConfigEntryUpgradePolicy::Static);

        // When set to true, RA will assert if nodeid doesn't match with value loaded from lfum
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", AssertOnNodeIdMismatchAtLfumLoad, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Determines whether RA will use deactivation info for performing primary re-election
        // For new clusters this configuration should be set to true
        // For existing clusters that are being upgraded please see the release notes on how to enable this
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsDeactivationInfoEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // RAP health event duration for Api ok.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", RAPApiOKHealthEventDuration, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Check VersionInstance of completed upgrade with the one in NodeConfig.NodeVersion before sending NodeUp.
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsVersionInstanceCheckEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // For ESE->TStore migration support, disable some test asserts that normally check against LSN regression without data loss
        INTERNAL_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsDataLossLsnCheckEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Deprecated. Please use RAPMessageRetryInterval instead.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", ReopenReplicaMessageRetryInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);

        // The minimum interval between two Replica Up messages for the same replica sent by RA to FM
        // Successive Replica Up messages for the same replica will not be sent in less than this interval
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerReplicaMinimumIntervalBetweenReplicaUpMessages, Common::TimeSpan::FromSeconds(45), Common::ConfigEntryUpgradePolicy::Static);

        // The minimum interval between two Replica Up messages sent by RA to FM
        // Two replica up messages will not be sent by RA to FM in less than this interval
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PerNodeMinimumIntervalBetweenReplicaUpMessages, Common::TimeSpan::FromSeconds(0.5), Common::ConfigEntryUpgradePolicy::Static);

        // LocalMessageRetryTimerInterval defines the timer interval to retry pending messages in RAP
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", LocalMessageRetryTimerInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

        // MaxLocalMessageRetryCount defines the max number of local retries for a failed message in RAP
        DEPRECATED_CONFIG_ENTRY(int, L"ReconfigurationAgent", MaxLocalMessageRetryCount, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

        // IsLocalHealthReportingEnabled determines whether health reporting in RAP is enabled or not
        DEPRECATED_CONFIG_ENTRY(bool, L"ReconfigurationAgent", IsLocalHealthReportingEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The duration after which dropped FTs can be removed from the LFUM
        // Deprecated because this is no longer allowed - only deleted ft are removed
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", DroppedFailoverUnitTombstoneDuration, Common::TimeSpan::FromSeconds(3600.0 * 24), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Deprecated (Query do not have timeout interval). The duration after which pending Queries on the RA are completed with Timeout error
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"ReconfigurationAgent", PeriodicQueryCleanupInterval, Common::TimeSpan::FromMinutes(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts after which an error will be reported to HM
        // Deprecated. Not currently used
        DEPRECATED_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaChangeRoleFailureErrorReportThreshold, 2, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of attempts for continuous API failures during replica change role after which RA will drop the replica
        // Defaults to INT_MAX
        // Not currently used
        DEPRECATED_CONFIG_ENTRY(int, L"ReconfigurationAgent", ReplicaChangeRoleFailureMaxRetryThreshold, 2147483647, Common::ConfigEntryUpgradePolicy::Dynamic);

        #pragma endregion

        #pragma region Failover/Replication
        RE_INTERNAL_CONFIG_PROPERTIES(L"Failover/Replication", 50, 0, 524288000, 0, 524288000);
        #pragma endregion

        #pragma region Failover/TransactionalReplicator2
        TR_INTERNAL_CONFIG_PROPERTIES(L"Failover/TransactionalReplicator2");
        #pragma endregion

        SL_CONFIG_PROPERTIES(L"Failover/SharedLog");
    };
}
