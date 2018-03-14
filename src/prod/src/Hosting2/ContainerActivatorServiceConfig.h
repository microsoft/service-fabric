// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerActivatorServiceConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ContainerActivatorServiceConfig, "ContainerActivatorService")

    public:
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ContainerActivatorService", ContainerActivatorServiceAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ContainerActivatorService", ContainerActivatorServiceId, L"", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ContainerActivatorService", StopTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ContainerActivatorService", StartTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}
