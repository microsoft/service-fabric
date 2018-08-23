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
    namespace RepairManager
    {
        class RepairManagerConfig :
            public Common::ComponentConfig,
            public Reliability::ReplicationComponent::REConfigBase,
            public TxnReplicator::TRConfigBase,
            public TxnReplicator::SLConfigBase
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(RepairManagerConfig, "RepairManager")

            // The TargetReplicaSetSize for RepairManager
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for RepairManager
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The ReplicaRestartWaitDuration for RepairManager
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", ReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 60), Common::ConfigEntryUpgradePolicy::Static);
            // The QuorumLossWaitDuration for RepairManager
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);
            // The StandByReplicaKeepDuration for RepairManager
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", StandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(60.0 * 60 * 24 * 7), Common::ConfigEntryUpgradePolicy::Static);
            // The PlacementConstraints for RepairManager
            INTERNAL_CONFIG_ENTRY(std::wstring, L"RepairManager", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The maximum delay for internal retries when failures are encountered
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", MaxOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum timeout for internal communications between RepairManager and other system services (e.g. Cluster Manager, Failover Manager, etc.)
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", MaxCommunicationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The processing interval used by the repair task background processing (in Preparing, Restoring states)
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", RepairTaskProcessingInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Allows new repair requests to be created
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", AllowNewTasks, true, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum number of repair requests that can be in a state other than Completed
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", MaxActiveTasks, 250, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum number of repair requests that can exist, regardless of state
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", MaxTotalTasks, 2000, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Determines whether automatic cleanup of completed tasks is enabled
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", CompletedTaskCleanupEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The interval between task cleanup attempts
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", CleanupInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The desired total task count; RM automatic cleanup will delete tasks older than min age to try to keep the count no higher than this number
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", DesiredTotalTasks, 300, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The minimum amount of time a completed task will be kept before being eligible for automatic cleanup
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", MinCompletedTaskAge, Common::TimeSpan::FromMinutes(2 * 60), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum amount of time a completed task will be kept, regardless of the total task count
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", MaxCompletedTaskAge, Common::TimeSpan::FromMinutes(7 * 24 * 60), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
            INTERNAL_CONFIG_ENTRY(int, L"RepairManager", CompactionThresholdInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
            
            // Determines whether health checks are enabled for repair tasks in the repair manager            
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", EnableHealthChecks, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Performs the actual query with the health client API
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", EnableClusterHealthQuery, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Determines whether the names of impacted nodes are validated against the cluster node list
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", ValidateImpactedNodeNames, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Determines whether the health check in the Preparing phase counts health samples taken before the start of the health check to count as part of the stable duration
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", PreparingHealthCheckAllowLookback, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The interval with which health checks are scheduled by the repair manager
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", HealthCheckInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The time after which the validity of a health check sample expires. This resets the successful health check duration
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", HealthCheckSampleValidDuration, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The amount of time to observe consecutive passed health samples before a repair task is said to have passed checks in either the Preparing or Restoring stage. Observing a failed health sample will reset this.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", HealthCheckStableDuration, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
            
            // The amount of time to retry health checks for a repair task in the Preparing stage.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", PreparingHealthCheckRetryTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
            
            // The amount of time to retry health checks for a repair task in the Restoring stage.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", RestoringHealthCheckRetryTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The time window to continue doing health checks even when there are no active repair tasks. This is an optimization for
            // new repair tasks that may arrive a few minutes after all previous tasks have completed.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", HealthCheckOnIdleDuration, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

            // When retrieving timestamps from the persisted replicated store (LastHealthyAt and LastErrorAt), there may be a possibility of clock skews
            // since another RM replica may have persisted this timestamp data in the first place.
            // If this clock skew is larger than the below threshold, then ignore the persisted data and instead recreate the timestamps.
            // E.g. if one replica had a clock skew of 1 day (for whatever reason) and it persisted LastErrorAt to a future date and failed over to
            // another replica, then no repair task will pass health check since it can never build up a stable window beyond LastErrorAt till the 1 day passes.
            // The below config helps avoid this problem.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"RepairManager", HealthCheckClockSkewTolerance, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
            
            // Uses TStore for persisted stateful storage when set to true
            INTERNAL_CONFIG_ENTRY(bool, L"RepairManager", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);

            RE_INTERNAL_CONFIG_PROPERTIES(L"RepairManager/Replication", 50, 8192, 314572800, 16384, 314572800);
            TR_INTERNAL_CONFIG_PROPERTIES(L"RepairManager/TransactionalReplicator2");
            SL_CONFIG_PROPERTIES(L"RepairManager/SharedLog");
        };
    }
}
