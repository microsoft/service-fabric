// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/Common.h"

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NetworkInventoryManagerConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(NetworkInventoryManagerConfig, "NetworkInventoryManager")

        public:
            bool static IsNetworkInventoryManagerEnabled();

            PUBLIC_CONFIG_ENTRY(bool, L"NetworkInventoryManager", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

            // The global isolated networks MAC pool start
            INTERNAL_CONFIG_ENTRY(std::wstring, L"NetworkInventoryManager", NIMMacPoolStart, L"00-00-00-01-00-00", Common::ConfigEntryUpgradePolicy::Static);

            // The global isolated networks MAC pool end
            INTERNAL_CONFIG_ENTRY(std::wstring, L"NetworkInventoryManager", NIMMacPoolEnd, L"00-00-00-02-00-00", Common::ConfigEntryUpgradePolicy::Static);

            // Cleanup task for nodes period in secs.
            INTERNAL_CONFIG_ENTRY(int, L"NetworkInventoryManager", NIMCleanupPeriodInSecs, 60, Common::ConfigEntryUpgradePolicy::Static);

            // The VSID lower end
            INTERNAL_CONFIG_ENTRY(int, L"NetworkInventoryManager", NIMVSIDRangeStart, 5555, Common::ConfigEntryUpgradePolicy::Static);

            // The VSID upper end
            INTERNAL_CONFIG_ENTRY(int, L"NetworkInventoryManager", NIMVSIDRangeEnd, 800000, Common::ConfigEntryUpgradePolicy::Static);

            // The allocation request batch size
            INTERNAL_CONFIG_ENTRY(int, L"NetworkInventoryManager", NIMAllocationRequestBatchSize, 1, Common::ConfigEntryUpgradePolicy::Static);
        };
    }
}
