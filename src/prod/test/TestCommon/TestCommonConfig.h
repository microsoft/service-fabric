// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestCommonConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(TestCommonConfig, "TestCommon")

        INTERNAL_CONFIG_ENTRY(bool, L"TestCommon", DumpCoreAtExit, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"TestCommon", AssertOnVerifyTimeout, false, Common::ConfigEntryUpgradePolicy::Static)
    };
}
