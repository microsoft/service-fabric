// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class FaultAnalysisServiceConfig :
            public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(FaultAnalysisServiceConfig, "FaultAnalysisService")

//NOT_PLATFORM_UNIX_START
            // The TargetReplicaSetSize for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The ReplicaRestartWaitDuration for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FaultAnalysisService", ReplicaRestartWaitDuration, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Static);
            // The QuorumLossWaitDuration for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FaultAnalysisService", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);
            // The StandByReplicaKeepDuration for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FaultAnalysisService", StandByReplicaKeepDuration, Common::TimeSpan::FromMinutes(60 * 24 * 7), Common::ConfigEntryUpgradePolicy::Static);
            // The PlacementConstraints for FaultAnalysisService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FaultAnalysisService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);

            // This is how often the store will be cleaned up.  Only actions in a terminal state, and that completed at least CompletedActionKeepDurationInSeconds ago will be removed. 
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", StoredActionCleanupIntervalInSeconds, 3600, Common::ConfigEntryUpgradePolicy::Static);

            // This is approximately how long to keep actions that are in a terminal state.  This also depends on StoredActionCleanupIntervalInSeconds, since the work to cleanup is only done on that interval. 604800 is 7 days.
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", CompletedActionKeepDurationInSeconds, 604800, Common::ConfigEntryUpgradePolicy::Static);

            // This is how often the store will be audited for cleanup; if the number of events is more than 30000, the cleanup will kick in.
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", StoredChaosEventCleanupIntervalInSeconds, 3600, Common::ConfigEntryUpgradePolicy::Static);

            // The total amount of time, in seconds, that the system will wait for data loss to happen.  This is internally used when the StartPartitionDataLossAsync() api is called.
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", DataLossCheckWaitDurationInSeconds, 25, Common::ConfigEntryUpgradePolicy::Static);            
            
            // The is the time between the checks the system performs  while waiting for data loss to happen.  The number of times the data loss number will be checked per internal iteration is DataLossCheckWaitDurationInSeconds/this.
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", DataLossCheckPollIntervalInSeconds, 5, Common::ConfigEntryUpgradePolicy::Static);            
                        
            // This parameter is used when the data loss api is called.  It controls how long the system will wait for a replica to get dropped after remove replica is internally invoked on it.
            PUBLIC_CONFIG_ENTRY(int, L"FaultAnalysisService", ReplicaDropWaitDurationInSeconds, 600, Common::ConfigEntryUpgradePolicy::Static);            

            // This parameter is used to enable or disable ClusterAnalysis.
            PUBLIC_CONFIG_ENTRY(bool, L"FaultAnalysisService", ClusterAnalysisEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

            // The maximum delay for internal retries when failures are encountered
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FaultAnalysisService", MaxOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum timeout for internal communications between RepairManager and other system services (e.g. Cluster Manager, Failover Manager, etc.)
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FaultAnalysisService", MaxCommunicationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

            // If this is true Fault Analysis Service will run internal tests
            INTERNAL_CONFIG_ENTRY(bool, L"FaultAnalysisService", TestModeEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

            // This is for testing Chaos APIs and other future ones
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", ApiTestMode, 0, Common::ConfigEntryUpgradePolicy::Static);

            // The timeout for each request made by the testability service
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", RequestTimeoutInSeconds, 60, Common::ConfigEntryUpgradePolicy::Static);

            // The time to retry requests made by the testability service.  This wrappers RequestTimeoutInSeconds
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", OperationTimeoutInSeconds, 300, Common::ConfigEntryUpgradePolicy::Static);

            INTERNAL_CONFIG_ENTRY(int64, L"FaultAnalysisService", MaxStoredActionCount, 10000, Common::ConfigEntryUpgradePolicy::Static);

            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", MaxStoredChaosEventCount, 25000, Common::ConfigEntryUpgradePolicy::Static);

            // The time (in seconds) to backoff before retrying the same step again.
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", CommandStepRetryBackoffInSeconds, 1, Common::ConfigEntryUpgradePolicy::Static);

            // The min allowed duration for stop node through StartNodeTransition().  Api calls that provide a value below this will fail.
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", StopNodeMinDurationInSeconds, 5 * 60, Common::ConfigEntryUpgradePolicy::Static);

            // The max allowed duration for stop node through StartNodeTransition().  Api calls that provide a value above this will fail.
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", StopNodeMaxDurationInSeconds, 60 * 60 * 4, Common::ConfigEntryUpgradePolicy::Static);

            // The max number of concurrent requests FAS can run together.
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", ConcurrentRequests, 100, Common::ConfigEntryUpgradePolicy::Static);

            // ChaosSnapshot and ChaosValidationFailed telemetry traces would be emitted per iteration of Chaos, and the frequency of iterations depends on the
            // user specified settings; this config is used to limit the number of such events per day. So, these two telemetry traces will be emitted
            // every (1/ChaosSnapshotTelemetrySamplingProbability) iterations on average. [Note, ChaosExecutingFaults trace is also emitted per iteration, 
            // but this is just a summing across telemetry events, so at any time, only one such event would exist in memory.]
            INTERNAL_CONFIG_ENTRY(double, L"FaultAnalysisService", ChaosTelemetrySamplingProbability, 0.05, Common::ConfigEntryUpgradePolicy::Static);

            // ChaosFound telemetry trace is emitted every ChaostelemetryReportPeriodInSeconds seconds.
            INTERNAL_CONFIG_ENTRY(int, L"FaultAnalysisService", ChaostelemetryReportPeriodInSeconds, 3600, Common::ConfigEntryUpgradePolicy::Static);
//NOT_PLATFORM_UNIX_END
        };
    }
}
