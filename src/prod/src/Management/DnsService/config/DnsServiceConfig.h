// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/Common.h"

namespace DNS
{
    class DnsServiceConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(DnsServiceConfig, "DnsService")

    public:
        PUBLIC_CONFIG_ENTRY(bool, L"DnsService", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(int, L"DnsService", InstanceCount, -1, Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(bool, L"DnsService", SetAsPreferredDns, true, Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(bool, L"DnsService", AllowMultipleListeners, false, Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"DnsService", PartitionPrefix, L"--", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"DnsService", PartitionSuffix, L"", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(bool, L"DnsService", EnablePartitionedQuery, false, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"DnsService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"DnsService", DnsPort, 53, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"DnsService", NumberOfConcurrentQueries, 100, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"DnsService", MaxMessageSizeInKB, 8, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"DnsService", MaxCacheSize, 5000, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"DnsService", NDots, 1, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(bool, L"DnsService", IsRecursiveQueryEnabled, true, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", TimeToLive, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", FabricQueryTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", RecursiveQueryTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", NodeDnsCacheHealthCheckInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(bool, L"DnsService", SetContainerDnsWhenPortBindingsAreEmpty, true, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", NodeDnsEnvironmentHealthCheckInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", ConfigureDnsEnvironmentTimeoutInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(bool, L"DnsService", EnableOnCloseTimeout, true, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"DnsService", OnCloseTimeoutInSeconds, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
    };
}
