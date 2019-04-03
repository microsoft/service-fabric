// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SetupConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(SetupConfig, "Setup")

    public:
        // The flag used to decide if flat container network should be set up
#if defined(PLATFORM_UNIX)
        PUBLIC_CONFIG_ENTRY(bool, L"Setup", ContainerNetworkSetup, false, Common::ConfigEntryUpgradePolicy::Static);
#else
        PUBLIC_CONFIG_ENTRY(bool, L"Setup", ContainerNetworkSetup, true, Common::ConfigEntryUpgradePolicy::Static);
#endif
        // The network name to use when setting up a flat container network
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Setup", ContainerNetworkName, L"", Common::ConfigEntryUpgradePolicy::Static);

        // The flag used to decide if isolated container network should be set up
        PUBLIC_CONFIG_ENTRY(bool, L"Setup", IsolatedNetworkSetup, false, Common::ConfigEntryUpgradePolicy::Static);

        // The network name to use when setting up an isolated container network
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Setup", IsolatedNetworkName, L"", Common::ConfigEntryUpgradePolicy::Static);

        // The network interface to be used for isolated container network set up
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Setup", IsolatedNetworkInterfaceName, L"", Common::ConfigEntryUpgradePolicy::Static);
    };
}
