// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTestCommon
{
    class FederationTestCommonConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FederationTestCommonConfig, "FederationTestCommon")

        INTERNAL_CONFIG_ENTRY(bool, L"FederationTestCommon", UseLoopbackAddress, true, Common::ConfigEntryUpgradePolicy::Static)
    };
}
