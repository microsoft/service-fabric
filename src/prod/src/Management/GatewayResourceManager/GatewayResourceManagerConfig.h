// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class GatewayResourceManagerConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(GatewayResourceManagerConfig, "GatewayResourceManager")

        public:
            // The config section names used here are only to document the possible config parameters.
            // At runtime, the actual configuration section name will be a service name that's passed to
            // the service itself via initialization parameters so that the service can read additional
            // configurations from its own section.
            // The PlacementConstraints for GatewayResourceManager
            PUBLIC_CONFIG_ENTRY(std::wstring, L"GatewayResourceManager", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The TargetReplicaSetSize for GatewayResourceManager
            PUBLIC_CONFIG_ENTRY(int, L"GatewayResourceManager", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for GatewayResourceManager
            PUBLIC_CONFIG_ENTRY(int, L"GatewayResourceManager", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The image name for the linux proxy container 
            PUBLIC_CONFIG_ENTRY(std::wstring, L"GatewayResourceManager", LinuxProxyImageName, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The image name for the windows proxy container
            PUBLIC_CONFIG_ENTRY(std::wstring, L"GatewayResourceManager", WindowsProxyImageName, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The replica count for proxy
            PUBLIC_CONFIG_ENTRY(int, L"GatewayResourceManager", ProxyReplicaCount, 1, Common::ConfigEntryUpgradePolicy::Static);
            // CPUCores resource governance limits for the proxy container
            PUBLIC_CONFIG_ENTRY(std::wstring, L"GatewayResourceManager", ProxyCPUCores, L"1", Common::ConfigEntryUpgradePolicy::Static);
            // Proxy create to ready timeout in minutes
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"GatewayResourceManager", ProxyCreateToReadyTimeoutInMinutes, Common::TimeSpan::FromMinutes(30), Common::ConfigEntryUpgradePolicy::Static);
        };
    }
}
