// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyUpgradeOrchestrationService :
        public ComProxySystemServiceBase<IFabricUpgradeOrchestrationService>,
        public IUpgradeOrchestrationService
    {
        DENY_COPY(ComProxyUpgradeOrchestrationService);

    public:
        ComProxyUpgradeOrchestrationService(Common::ComPointer<IFabricUpgradeOrchestrationService> const & comImpl);
        virtual ~ComProxyUpgradeOrchestrationService();

        virtual Common::AsyncOperationSPtr BeginUpgradeConfiguration(
            FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndUpgradeConfiguration(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetClusterConfigurationUpgradeStatus(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetClusterConfigurationUpgradeStatus(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricOrchestrationUpgradeStatusResult ** reply);

        virtual Common::AsyncOperationSPtr BeginGetClusterConfiguration(
            std::wstring const & apiVersion,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        virtual Common::ErrorCode EndGetClusterConfiguration(
            Common::AsyncOperationSPtr const &operation,
            IFabricStringResult** result);

        virtual Common::AsyncOperationSPtr BeginGetUpgradesPendingApproval(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetUpgradesPendingApproval(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginStartApprovedUpgrades(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStartApprovedUpgrades(
            Common::AsyncOperationSPtr const & asyncOperation);

        // SystemServiceCall
        virtual Common::AsyncOperationSPtr BeginCallSystemService(
            std::wstring const & action,
            std::wstring const & inputBlob,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode EndCallSystemService(
            Common::AsyncOperationSPtr const & asyncOperation,
            __inout std::wstring & outputBlob);

    private:
        class StartClusterConfigurationUpgradeOperation;
        class GetClusterConfigurationUpgradeStatusOperation;
        class GetClusterConfigurationOperation;
        class GetUpgradesPendingApprovalOperation;
        class StartApprovedUpgradesOperation;
    };
}
