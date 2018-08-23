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
#include "../ServiceModel/data/txnreplicator/SLConfigMacros.h"

namespace Naming
{
    class NamingConfig 
        : public Common::ComponentConfig
        , public Reliability::ReplicationComponent::REConfigBase
        , public TxnReplicator::TRConfigBase
        , public TxnReplicator::SLConfigBase
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(NamingConfig, "Naming")
                
        // -------------------------------
        // General
        // -------------------------------

        //The interval between operation retries on retryable operations
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", OperationRetryInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The maximum number of entries to be put into the reply. Used to test the notification paging of the updates from gateway to client.
        // Default is 0, which shows that the feature is disabled.
        TEST_CONFIG_ENTRY(int, L"NamingService", MaxNotificationReplyEntryCount, 0, Common::ConfigEntryUpgradePolicy::Dynamic); 
        
        // -------------------------------
        // Store Service
        // -------------------------------

        //The number of partitions of the Naming Service store to be created. Each partition owns a single partition key that corresponds to its index, so partition keys [0, PartitionCount) exist. 
        //Increasing the number of Naming Service partitions increases the scale that the Naming Service can perform at by decreasing the average amount of data held by any backing replica set, at a cost of increased utilization of resources (since PartitionCount*ReplicaSetSize service replicas must be maintained).
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", PartitionCount, 3, Common::ConfigEntryUpgradePolicy::NotAllowed);
        //The number of replica sets for each partition of the Naming Service store. 
        //Increasing the number of replica sets increases the level of reliability for the information in the Naming Service Store, decreasing the change that the information will be lost as a result of node failures, at a cost of increased load on Windows Fabric and the amount of time it takes to perform updates to the naming data.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", TargetReplicaSetSize, 7, Common::ConfigEntryUpgradePolicy::NotAllowed);
        //The minimum number of Naming Service replicas required to write into to complete an update.  
        //If there are fewer replicas than this active in the system the Reliability System denies updates to the Naming Service Store until replicas are restored. 
        //This value should never be more than the TargetReplicaSetSize.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MinReplicaSetSize, 3, Common::ConfigEntryUpgradePolicy::NotAllowed);
        //When a Naming Service replica goes down, this timer starts.  When it expires the FM will begin to replace the replicas which are down (it does not yet consider them lost)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", ReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::NotAllowed);
        //When a Naming Service gets into quorum loss, this timer starts.  When it expires the FM will consider the down replicas as lost, and attempt to recover quorum. Not that this may result in data loss.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::NotAllowed);
        //When a Naming Service replicas come back from a down state, it may have already been replaced.  This timer determines how long the FM will keep the standby replica before discarding it.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", StandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::NotAllowed);
        //Placement constraint for the Naming Service
        PUBLIC_CONFIG_ENTRY(std::wstring, L"NamingService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);
        //The maximum number of entries maintained in the LRU service description cache at the Naming Store Service (set to 0 for no limit).
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", ServiceDescriptionCacheLimit, 0, Common::ConfigEntryUpgradePolicy::Static);
        // Uses TStore for persisted stateful storage when set to true
        INTERNAL_CONFIG_ENTRY(bool, L"NamingService", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);

        // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
        INTERNAL_CONFIG_ENTRY(int, L"NamingService", CompactionThresholdInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
        
        //Timeout time for each repair operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", RepairOperationTimeout, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Timeout used for stopping retrying and failing create service request
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", ServiceFailureTimeout, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Interval in which the naming inconsistency repair between the authority owner and name owner will start.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", RepairInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        // Determines the max number of threads that the NamingStore can use to process requests. 0 defaults to the number of CPUs.
        INTERNAL_CONFIG_ENTRY(int, L"NamingService", RequestQueueThreadCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Determines the size of the NamingStore job queue.
        INTERNAL_CONFIG_ENTRY(int, L"NamingService", RequestQueueSize, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Determines the number of parallel work items that Naming can perform.
        INTERNAL_CONFIG_ENTRY(int, L"NamingService", MaxPendingRequestCount, 500, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval to check and report health for operations executed on the naming service.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", NamingServiceHealthReportingTimerInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Static);
        // The max allowed interval for starting recovery of any pending operations before the Naming primary is reported unhealthy.
        // While recovery is not completely started, the Naming primary does not process any incoming requests.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", PrimaryRecoveryStartedHealthDuration, Common::TimeSpan::FromMinutes(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max allowed interval for processing a naming operation before the Naming primary is reported unhealthy.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", NamingServiceProcessOperationHealthDuration, Common::TimeSpan::FromMinutes(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The minimum time a naming operation that completed with error is kept in health monitor
        // to keep track of retries.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", NamingServiceFailedOperationHealthGraceInterval, Common::TimeSpan::FromMinutes(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The minimum time a naming operation that completed with error is kept in the health monitor to keep track of retries.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", NamingServiceFailedOperationHealthReportTimeToLive, Common::TimeSpan::FromSeconds(15.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The TTL for the health report associated with a naming operation that has been executed for longer than max allowed duration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", NamingServiceSlowOperationHealthReportTimeToLive, Common::TimeSpan::FromMinutes(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of slow operations that Naming store service reports unhealthy at one time. If 0, all slow operations are sent.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MaxNamingServiceHealthReports, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------------------------
        // Replication
        // -------------------------------

        RE_INTERNAL_CONFIG_PROPERTIES(L"Naming/Replication", 0, 1024, 104857600, 2048, 104857600);
        TR_INTERNAL_CONFIG_PROPERTIES(L"Naming/TransactionalReplicator2");
        SL_CONFIG_PROPERTIES(L"Naming/SharedLog");
    };

    typedef std::shared_ptr<NamingConfig> NamingConfigSPtr;
}
