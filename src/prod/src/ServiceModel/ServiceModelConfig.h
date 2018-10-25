// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceModelConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ServiceModelConfig, "ServiceModel")

        // -------------------------------
        // FailoverManager
        // -------------------------------

        // When a persisted system replica goes down, Windows Fabric waits for this duration for the replica to come
        // back up before creating new replacement  replicas (which would require a copy of the state).
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", SystemReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When a persisted replica goes down, Windows Fabric waits for this duration for the replica to come back up
        // before creating new replacement  replicas (which would require a copy of the state).
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", UserReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This is the max duration for which we allow a partition to be in a state of quorum loss. If the partition
        // is still in quorum loss after this duration, the partition is recovered from quorum loss by considering
        // the down replicas as lost. Note that this can potentially incur data loss.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);

        // When a persisted system replica come back from a down state, it may have already been replaced.
        // This timer determines how long the FM will keep the standby replica before discarding it.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", SystemStandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::Dynamic);

        // When a persisted replica come back from a down state, it may have already been replaced.
        // This timer determines how long the FM will keep the standby replica before discarding it.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FailoverManager", UserStandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The default max number of StandBy replicas that the system keeps for system services.
        INTERNAL_CONFIG_ENTRY(int, L"FailoverManager", SystemMaxStandByReplicaCount, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The default max number of StandBy replicas that the system keeps for user services.
        PUBLIC_CONFIG_ENTRY(int, L"FailoverManager", UserMaxStandByReplicaCount, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------------------------
        // Federation
        // -------------------------------

        // This specifies a list of roles for which V1 node id generator should be used.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Federation", NodeNamePrefixesForV1Generator, L"", Common::ConfigEntryUpgradePolicy::Static);

        // This configuration specifies whether or not to use V2 node id generator.
        INTERNAL_CONFIG_ENTRY(bool, L"Federation", UseV2NodeIdGenerator, false, Common::ConfigEntryUpgradePolicy::Static);

        // This configuration specifies whether to use V3 node id generator.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Federation", NodeIdGeneratorVersion, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);

        // -------------------------------
        // Naming Store
        // -------------------------------

        //Maximum allowed property name length (depends on local store limits)
        INTERNAL_CONFIG_ENTRY(int, L"NamingService", MaxPropertyNameLength, 256, Common::ConfigEntryUpgradePolicy::Static);

        // -------------------------------
        // Entree service
        // -------------------------------

        //The maximum message size for client node communication when using naming.
        //DOS attack alleviation, default value is 4MB
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MaxMessageSize, 4*1024*1024, Common::ConfigEntryUpgradePolicy::Static);

        // The fraction of MaxMessageSize to use as the available buffer limit when calculating how much data
        // to put in a single message (should be in the range [0.0, 1.0])
        INTERNAL_CONFIG_ENTRY(double, L"NamingService", MessageContentBufferRatio, 0.75, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));

        //The maximum timeout allowed for file store service operation. Requests specifying a larger timeout will be rejected.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", MaxFileOperationTimeout, Common::TimeSpan::FromMinutes(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        //The maximum timeout allowed for client operations. Requests specifying a larger timeout will be rejected.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", MaxOperationTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The maximum allowed number of client connections per gateway.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MaxClientConnections, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        
        //The timeout used when delivering service notifications to the client.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"NamingService", ServiceNotificationTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        //The maximum number of outstanding notifications before a client registration is forcibly closed by the gateway.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MaxOutstandingNotificationsPerClient, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        
        //The maximum number of empty partitions that will remain indexed in the notification cache for synchronizing reconnecting clients.
        //Any empty partitions above this number will be removed from the index in ascending lookup version order. Reconnecting clients
        //can still synchronize and receive missed empty partition updates, but the synchronization protocol becomes more expensive.
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", MaxIndexedEmptyPartitions, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The maximum number of entries maintained in the LRU service description cache at the Naming Gateway (set to 0 for no limit).
        PUBLIC_CONFIG_ENTRY(int, L"NamingService", GatewayServiceDescriptionCacheLimit, 0, Common::ConfigEntryUpgradePolicy::Static);

        // -------------------------------
        // Fabric Client Configuration
        // -------------------------------
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", MaxApplicationParameterLength, 1024*1024, Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------------------------
        // Query
        // -------------------------------
        // The fraction of MaxMessageSize to use as the available buffer limit when calculating how much data
        // to put in a single message (should be in the range [0.0, 1.0])
        TEST_CONFIG_ENTRY(double, L"Query", QueryPagerContentRatio, 1.0, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));

        // --------------- Application Shared Log Container settings

        //Path and file name to location to place shared log container.
        //Use L"" for using default path under fabric data root.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"KtlLogger", SharedLogPath, L"", Common::ConfigEntryUpgradePolicy::Static);
        
        //Unique guid for shared log container. Use L"" if using default path under fabric data root.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"KtlLogger", SharedLogId, L"", Common::ConfigEntryUpgradePolicy::Static);

#if defined(PLATFORM_UNIX)
        // TODO: Change this back to 8192 when the ktllogger daemon is
        //       in place
        //The number of MB to allocate in the shared log container
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", SharedLogSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static);
#else
        //The number of MB to allocate in the shared log container
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", SharedLogSizeInMB, 8192, Common::ConfigEntryUpgradePolicy::Static);
#endif
        
        //The number of log streams to allow in the shared log container
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", SharedLogNumberStreams, 24576, Common::ConfigEntryUpgradePolicy::Static);
        
        //The maximum record size written into the shared log. Use 0 for the default value
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", SharedLogMaximumRecordSizeInKB, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        //Shared Log creation flags. Use 0 for non sparse log.
        INTERNAL_CONFIG_ENTRY(uint, L"KtlLogger", SharedLogCreateFlags, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        //Default application shared log sub-directory to use when SharedLogPath is L"".
        INTERNAL_CONFIG_ENTRY(std::wstring, L"KtlLogger", DefaultApplicationSharedLogSubdirectory, L"ReplicatorLog", Common::ConfigEntryUpgradePolicy::Static);

        //Default application shared log file name to use when SharedLogPath is L"".
        INTERNAL_CONFIG_ENTRY(std::wstring, L"KtlLogger", DefaultApplicationSharedLogFileName, L"replicatorshared.log", Common::ConfigEntryUpgradePolicy::Static);

        //Unique guid for default application shared log container to use when SharedLogId is L"".
        INTERNAL_CONFIG_ENTRY(std::wstring, L"KtlLogger", DefaultApplicationSharedLogId, L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62", Common::ConfigEntryUpgradePolicy::Static);
		
		// Maximum number of node types that can be passed into StartChaos API through ChaosTargetFilter in ChaosParameters
		INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", MaxNumberOfNodeTypesInChaosTargetFilter, 100, Common::ConfigEntryUpgradePolicy::Static);

		// Maximum number of applications names that can be passed into StartChaos API through ChaosTargetFilter in ChaosParameters
		INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", MaxNumberOfApplicationsInChaosTargetFilter, 1000, Common::ConfigEntryUpgradePolicy::Static);

        // This is set to true by tests that dont want to activate fabricgateway as a separate exe.
        TEST_CONFIG_ENTRY(bool, L"FabricGateway", ActivateGatewayInproc, false, Common::ConfigEntryUpgradePolicy::Static);
        // Determines the size of the Gateway job queue.
        INTERNAL_CONFIG_ENTRY(int, L"FabricGateway", RequestQueueSize, 5000, Common::ConfigEntryUpgradePolicy::Static);
        // Expiration for a secure session between gateway and clients, set to 0 to disable session expiration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricGateway", SessionExpiration, Common::TimeSpan::FromSeconds(60 * 60 * 24), Common::ConfigEntryUpgradePolicy::Dynamic);

    };
}
