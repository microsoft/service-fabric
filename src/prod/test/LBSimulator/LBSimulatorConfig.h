// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class LBSimulatorConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(LBSimulatorConfig, "LBSimulator");
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", RandomSeed, 12345, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"LBSimulator", VerifyTimeout, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(bool, L"LBSimulator", FailTestIfVerifyTimeout, false, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MaxNodes, 50, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MaxServices, 20, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", InitialServices, 2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MaxPartitions, 200, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MaxReplicas, 4, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MaxDynamism, 3, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", ServiceDynamicIterations, 2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", Level1FaultDomainCount, 2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", Level2FaultDomainCount, 2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", UpgradeDomainCount, 3, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"LBSimulator", MetricCount, 20, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(double, L"LBSimulator", ReportLoadProbability, 0.2, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(double, L"LBSimulator", AffinitySimulationThreshold, 0.25, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(double, L"LBSimulator", CapacityOrCapacityRatioChangeThreshold, 0.3, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(uint, L"LBSimulator", DefaultMoveCost, 1, Common::ConfigEntryUpgradePolicy::Dynamic);
    };
};
