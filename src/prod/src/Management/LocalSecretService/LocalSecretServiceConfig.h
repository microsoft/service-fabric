//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once
#include "Common/Common.h"

namespace Management
{
    namespace LocalSecretService
    {
        class LocalSecretServiceConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(LocalSecretServiceConfig, "LocalSecretService")

        public:
            PUBLIC_CONFIG_ENTRY(int, L"LocalSecretService", InstanceCount, -1, Common::ConfigEntryUpgradePolicy::Static);
            INTERNAL_CONFIG_ENTRY(std::wstring, L"LocalSecretService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);
        };
    }
}