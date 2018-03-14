// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class RandomSessionConfig : public Common::ComponentConfig
    {
        DECLARE_COMPONENT_CONFIG(RandomSessionConfig, "FabricTest")

        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxNodes, 50, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MinNodes, 10, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxDynamism, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", CalculatorServiceCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TestStoreServiceCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TestPersistedStoreServiceCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxPartitions, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxReplicas, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", ClientThreadCount, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", RandomVoteCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", DeleteServiceInterval, 200, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", UpdateServiceInterval, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", ActivateDeactivateNodeInterval, 10, Common::ConfigEntryUpgradePolicy::Static)

        //*****************************************************************************************************************************************
        // These configs are to induce queue full scenarios due to slow copy pump on the test store service 
        // sets the number of test persisted store services that will use small replication queue sizes
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TestPersistedStoreServiceWithSmallReplicationQueueCount, 1, Common::ConfigEntryUpgradePolicy::Static)
        // sets the primary and secondary replication queue size
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TestPersistedStoreServiceSmallReplicationQueueSize, 512, Common::ConfigEntryUpgradePolicy::Static)
        // sets the period for which copy pump delay fault is induced. The amount of delay itself on individual pump operations is based on the 'max' and 'min' api delay configuration
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", TestPersistedStoreServiceDelayCopyFaultPeriod, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static)
        // Induces the above copy pump delay fault once after this configured count of test iterations
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TestPersistedStoreServiceDelayCopyPumpIterationInterval, 5, Common::ConfigEntryUpgradePolicy::Static)
        // The Replica Restart Wait Duration for the services that have repl fault enabled
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", TestPersistedStoreServiceReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Static);
        //*****************************************************************************************************************************************

        // Not more than this ratio of nodes would be deactivated in a cluster
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", MaxRatioOfFabricNodesToDeactivate, 0.2, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NamingThreadCount, 10, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NameCount, 100, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", PropertyPerNameCount, 10, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", FullRebuildInterval, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", UpgradeInterval, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", ApplicationCount, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", ApplicationVersionsCount, 10, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", AppMaxServicePackage, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", AppMaxCodePackage, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", AppMaxServiceTypes, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", AppMaxPartitionCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", AppMaxReplicaCount, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", UpgradeDomainSize, 4, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxAllowedMemoryInMB, 400, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", WatchDogCount, 3, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", VerifyQueryCount, 1, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", MaxDynamismInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", VerifyTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", OpenTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", ClientOperationInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", NamingOperationInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", NamingOperationTimeout, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", MaxAllowedMemoryTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", MaxApiDelayInterval, Common::TimeSpan::FromSeconds(6), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", MinApiDelayInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", WatchDogReportInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", AbortRatio, 0.1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", KillRuntimeRatio, 0.1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", ClientPutRatio, 0.5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", AppStatelessServiceRatio, 0.15, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", AppPersistedServiceRatio, 0.7, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", RatioOfVerifyDurationWhenFaultsAreAllowed, 0.5, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", CalculatorServices, L"", Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", TestStoreServices, L"", Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", TestPersistedStoreServices, L"", Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", TXRServiceCount, 2, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", UnreliableTransportBehaviors, L"", Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", Properties, L"", Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricRandomTest", NodeOpenRetryErrors, L"AddressAlreadyInUse", Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(bool, L"FabricRandomTest", DoRandomUpgrade, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricRandomTest", AssertOnMemoryCheckFailure, true, Common::ConfigEntryUpgradePolicy::Static)

        // Resource governance node, service and code package configurations and probabilities
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", NodeRgCapacityProbability, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NodeCpuCapacityMin, 1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NodeCpuCapacityMax, 32, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NodeMemoryCapacityMin, 1024, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", NodeMemoryCapacityMax, 32768, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", ServicePackageCpuLimitRatio, 0.5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", ServicePackageMemoryLimitRatio, 0.5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", CodePackageCpuLimitRatio, 0.5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", CodePackageMemoryLimitRatio, 0.5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MinPackageCpuLimit, 1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MinPackageMemoryLimit, 256, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(double, L"FabricRandomTest", PreWriteStatusCatchupEnabledProbability, 0.1, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MinClientPutDataSizeInBytes, -1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricRandomTest", MaxClientPutDataSizeInBytes, -1, Common::ConfigEntryUpgradePolicy::Static)

        // Test config for CM max internal retry timeout.
        // The timeout for inner CM operations (eg. CM to Naming requests) is (originalOperationTimeout + MinOperationTimeout * timeoutCount),
        // with minTimeout*timeoutCount capped at MaxTimeoutRetryBuffer.
        // Total timeout is capped at MaxOperationTimeout.
        // Default value for MaxTimeoutRetryBuffer is 5 minutes in ManagementConfig.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricRandomTest", CMMaxTimeoutRetryBuffer, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic)

    private:
        void Initialize();
    };
};
