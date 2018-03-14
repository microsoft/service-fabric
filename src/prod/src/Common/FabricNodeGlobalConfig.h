// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricNodeGlobalConfig : ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricNodeGlobalConfig, "NodeGlobal")

    public:
        // FabricNode open timeout
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NodeGlobal", OpenTimeout, Common::TimeSpan::FromSeconds(300), ConfigEntryUpgradePolicy::Static);
        // FabricNode close timeout
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"NodeGlobal", CloseTimeout, Common::TimeSpan::FromSeconds(300), ConfigEntryUpgradePolicy::Static);
    };
}
