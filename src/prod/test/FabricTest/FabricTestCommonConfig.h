// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestCommonConfig 
        : public Common::ComponentConfig
        , public TxnReplicator::SLConfigBase
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricTestCommonConfig, "FabricTest")

        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", UseRandomReplicatorSettings, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", StoreReplicationTimeout, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", LeakDetectionTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", MinReplicationQueueMemoryForV2ReplicatorInKB, 25000, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", IsBackupTest, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", IsFalseProgressTest, false, Common::ConfigEntryUpgradePolicy::Static)

        // TStore settings
        //
#ifdef PLATFORM_UNIX
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", EnableTStore, true, Common::ConfigEntryUpgradePolicy::NotAllowed)
#else
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", EnableTStore, false, Common::ConfigEntryUpgradePolicy::NotAllowed)
#endif
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", EnableHashSharedLogId, true, Common::ConfigEntryUpgradePolicy::NotAllowed)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", EnableSparseSharedLogs, true, Common::ConfigEntryUpgradePolicy::NotAllowed)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", SharedLogSize, 128 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::NotAllowed)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", SharedLogMaximumRecordSize, 8 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::NotAllowed)

        SL_CONFIG_PROPERTIES(L"FabricTest")
    };
}
