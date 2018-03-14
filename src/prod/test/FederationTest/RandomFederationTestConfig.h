// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class RandomFederationTestConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(RandomFederationTestConfig, "FederationTest")

        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxAllowedMemoryInMB, 400, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxAllowedMemoryInMBForSsl, 450, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxAllowedMemoryInMBForKerberos, 500, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationRandomTest", MaxAllowedMemoryTimeout, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", TotalNodes, 120, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxNodes, 60, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MinNodes, 20, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", AbortRatio, 40, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxDynamism, 3, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxRoutes, 10, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxBroadcasts, 2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", SeedNodeCount, 7, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxStoreWrites, 5, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"FederationRandomTest", MaxBlockedLeaseAgent, 5, Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationRandomTest", WaitTime, Common::TimeSpan::FromSeconds(130), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationRandomTest", OpenTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationRandomTest", RouteTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(bool, L"FederationRandomTest", AssertOnMemoryCheckFailure, true, Common::ConfigEntryUpgradePolicy::Static)

    public:
        __declspec (property(get=getSeedNodes)) std::vector<Common::LargeInteger> const& SeedNodes;
        std::vector<Common::LargeInteger> const& getSeedNodes() const { return seedNodes_; }

        __declspec (property(get=getNonSeedNodeVotes))int NonSeedNodeVotes;
        int getNonSeedNodeVotes() const { return nonSeedNodeVotes_; }

    private:
        void Initialize();

        std::vector<Common::LargeInteger> seedNodes_;
        int nonSeedNodeVotes_;
    };
}
