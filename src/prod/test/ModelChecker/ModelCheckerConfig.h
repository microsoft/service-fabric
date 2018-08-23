// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ModelChecker
{
    class ModelCheckerConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ModelCheckerConfig, "ModelChecker")


        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ModelChecker", VerifyTimeout, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Static);
        //INTERNAL_CONFIG_ENTRY(int, L"ModelChecker", RandomSeed, 12345, Common::ConfigEntryUpgradePolicy::Static);
        //INTERNAL_CONFIG_ENTRY(bool, L"ModelChecker", FailTestIfVerifyTimeout, false, Common::ConfigEntryUpgradePolicy::Static);
    };
};
