// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class FederationTestConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FederationTestConfig, "FederationTest")

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", OpenTimeout, Common::TimeSpan::FromSeconds(100), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", RouteTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", CrossRingRouteTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", SendTimeout, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", RouteRetryTimeout, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", BroadcastReplyTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FederationTest", VerifyTimeout, Common::TimeSpan::FromSeconds(150), Common::ConfigEntryUpgradePolicy::Static)
    };
}
