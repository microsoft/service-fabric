// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestQueryWeights : public std::map<std::wstring, int>
    {
    public:
        static FabricTestQueryWeights Parse(Common::StringMap const & entries);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
    };

    class FabricTestSessionConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricTestSessionConfig, "FabricTest")

        INTERNAL_CONFIG_ENTRY(int64, L"FabricTest", ServiceDescriptorLowRange, 0, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int64, L"FabricTest", ServiceDescriptorHighRange, 100, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", ResolveServiceBufferCount, 1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", NodeOpenMaxRetryCount, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", StoreClientCommandQuorumLossRetryCount, 5, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", QueryOperationRetryCount, 15, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", VerifyTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", VerifyUpgradeTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NamingOperationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NamingOperationTimeoutBuffer, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NamingOperationRetryTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NamingResolveRetryTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static)        
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NamingOperationRetryDelay, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", RefreshPsdTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", QueryOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", StoreClientTimeout, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", ReportLoadInterval, Common::TimeSpan::FromSeconds(100), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", HostingOpenCloseTimeout, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", MaxClientSessionIdleTimeout, Common::TimeSpan::FromSeconds(25), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", NodeDeallocationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", ReplicaDeallocationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", OpenTimeout, Common::TimeSpan::FromSeconds(140), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", ApiDelayInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", DelayOpenAfterAbortNode, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", PredeploymentTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", LastReplicaUpTimeout, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", AllowServiceAndFULossOnRebuild, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", AllowUnexpectedFUs, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", AllowHostFailureOnUpgrade, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", AllowQuoumLostFailoverUnitsOnVerify, true, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", ChangeServiceLocationOnChangeRole, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", UseEtw, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", SkipDeleteVerifyForQuorumLostServices, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", AsyncNodeCloseEnabled, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", StartNodeCloseOnSeparateThread, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", UseInternalHealthClient, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", ReportHealthThroughHMPrimary, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", HealthVerificationEnabled, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", HealthFullVerificationEnabled, false, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(bool, L"FabricTest", CloseAllNodesOnTestQuit, false, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(double, L"FabricTest", RatioOfVerifyDurationWhenFaultsAreAllowed, 0.5, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricTest", TraceFileName, L"FabricTest.trace", Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_GROUP(FabricTestQueryWeights, L"FabricTestQueryWeights", QueryWeights, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", KtlSharedLogSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static)

        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", MinClientPutDataSizeInBytes, -1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(int, L"FabricTest", MaxClientPutDataSizeInBytes, -1, Common::ConfigEntryUpgradePolicy::Static)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricTest", TXRServiceOperationTimeout, Common::TimeSpan::FromMinutes(1), Common::ConfigEntryUpgradePolicy::Static)
    };
}
