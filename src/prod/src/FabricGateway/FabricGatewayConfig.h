// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricGateway
{
    class FabricGatewayConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricGatewayConfig, "FabricGateway")

        // This is set to true by tests that dont want to activate fabricgateway as a separate exe.
        TEST_CONFIG_ENTRY(bool, L"FabricGateway", ActivateGatewayInproc, false, Common::ConfigEntryUpgradePolicy::Static);
        // Determines the size of the Gateway job queue.
        INTERNAL_CONFIG_ENTRY(int, L"FabricGateway", RequestQueueSize, 5000, Common::ConfigEntryUpgradePolicy::Static);
        // Expiration for a secure session with clients, set to 0 to disable session expiration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricGateway", SessionExpiration, Common::TimeSpan::FromSeconds(60 * 60 * 24), Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}
