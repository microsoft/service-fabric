// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LeaseWrapper
{
    class LeaseConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(LeaseConfig, "Lease")

        TEST_CONFIG_ENTRY(bool, L"Lease", DebugLeaseDriverEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
    };
}

