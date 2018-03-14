// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{
    class TestabilityConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(TestabilityConfig, "Testability")

        // The timeout value for checking the transport behaviors applied,
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Testability", UnreliableTransportRecurringTimer, Common::TimeSpan::FromSeconds(60 * 1), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The timeout to raise warning if tranport behavior is applied beyond threshold.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Testability", UnreliableTransportWarningTimeout, Common::TimeSpan::FromSeconds(60 * 30), Common::ConfigEntryUpgradePolicy::Dynamic);

        //The maximum number of parallel threads allowed to process
        INTERNAL_CONFIG_ENTRY(uint, L"Testability", MaxQueryOperationThreads, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The size of the query job queue
        INTERNAL_CONFIG_ENTRY(int, L"Testability", QueryQueueSize, 500, Common::ConfigEntryUpgradePolicy::Static);
    };
}
