// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class UpgradeOrchestrationServiceConfig :
            public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(UpgradeOrchestrationServiceConfig, "UpgradeOrchestrationService")

            // The TargetReplicaSetSize for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(int, L"UpgradeOrchestrationService", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(int, L"UpgradeOrchestrationService", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The ReplicaRestartWaitDuration for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeOrchestrationService", ReplicaRestartWaitDuration, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Static);
            // The QuorumLossWaitDuration for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeOrchestrationService", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);
            // The StandByReplicaKeepDuration for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeOrchestrationService", StandByReplicaKeepDuration, Common::TimeSpan::FromMinutes(60 * 24 * 7), Common::ConfigEntryUpgradePolicy::Static);
            // The PlacementConstraints for UpgradeOrchestrationService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeOrchestrationService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);

            // Automatic polling and upgrade action based on a goal-state file
            PUBLIC_CONFIG_ENTRY(bool, L"UpgradeOrchestrationService", AutoupgradeEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

            // Setting to make code upgrade require administrator approval before proceeding
            PUBLIC_CONFIG_ENTRY(bool, L"UpgradeOrchestrationService", UpgradeApprovalRequired, false, Common::ConfigEntryUpgradePolicy::Static);

            // The maximum delay for internal retries when failures are encountered
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeOrchestrationService", MaxOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum timeout for internal communications between RepairManager and other system services (e.g. Cluster Manager, Failover Manager, etc.)
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeOrchestrationService", MaxCommunicationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

            // If this is true UpgradeOrchestrationService will run internal tests  //??????
            INTERNAL_CONFIG_ENTRY(bool, L"UpgradeOrchestrationService", TestModeEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

            // The timeout for each request made by the UpgradeOrchestrationService
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeOrchestrationService", RequestTimeoutInSeconds, 60, Common::ConfigEntryUpgradePolicy::Static);

            // The ID for the cluster
            INTERNAL_CONFIG_ENTRY(std::wstring, L"UpgradeOrchestrationService", ClusterId, L"", Common::ConfigEntryUpgradePolicy::Static);

            // Dynamic reroute url pointing to latest production goal state file
            INTERNAL_CONFIG_ENTRY(std::wstring, L"UpgradeOrchestrationService", GoalStateFileUrl, L"", Common::ConfigEntryUpgradePolicy::Static);

            // If interval is not set to 24 hr, this time of day polling interval is ignored and this interval is used instead
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeOrchestrationService", GoalStateFetchIntervalInSeconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // If GoalStateFetchIntervalInSeconds is set to 24 hours (86400 sec), provisioning time of day is active by below setting. Default 22:00
            INTERNAL_CONFIG_ENTRY(std::wstring, L"UpgradeOrchestrationService", GoalStateProvisioningTimeOfDay, L"", Common::ConfigEntryUpgradePolicy::Static);

			// The upgrade flow in which the fault should be injected
			INTERNAL_CONFIG_ENTRY(std::wstring, L"UpgradeOrchestrationService", FaultFlow, L"", Common::ConfigEntryUpgradePolicy::Dynamic);

			// The upgrade step at which the fault should be injected
			INTERNAL_CONFIG_ENTRY(int, L"UpgradeOrchestrationService", FaultStep, -1, Common::ConfigEntryUpgradePolicy::Dynamic);
        };
    }
}
