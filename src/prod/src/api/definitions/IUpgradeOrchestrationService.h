// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IUpgradeOrchestrationService
    {
    public:
        virtual ~IUpgradeOrchestrationService() {};

        //// region: keep for backward compatibility purpose only

        // StartUpgrade
        virtual Common::AsyncOperationSPtr BeginUpgradeConfiguration(
            FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndUpgradeConfiguration(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        // GetUpgradeStatus
        virtual Common::AsyncOperationSPtr BeginGetClusterConfigurationUpgradeStatus(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetClusterConfigurationUpgradeStatus(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out IFabricOrchestrationUpgradeStatusResult ** result) = 0;

        // GetClusterConfiguration
        virtual Common::AsyncOperationSPtr BeginGetClusterConfiguration(
            std::wstring const & apiVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetClusterConfiguration(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out IFabricStringResult ** result) = 0;

        // GetUpgradesPendingApproval
        virtual Common::AsyncOperationSPtr BeginGetUpgradesPendingApproval(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetUpgradesPendingApproval(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        // StartApprovedUpgrades
        virtual Common::AsyncOperationSPtr BeginStartApprovedUpgrades(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartApprovedUpgrades(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        //// endregion

        // SystemServiceCall
        virtual Common::AsyncOperationSPtr BeginCallSystemService(
            std::wstring const & action,
            std::wstring const & inputBlob,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCallSystemService(
            Common::AsyncOperationSPtr const & asyncOperation,
            __inout std::wstring & outputBlob) = 0;
    };
}
