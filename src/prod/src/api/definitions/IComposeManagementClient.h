// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class IComposeManagementClient
    {
    public:
        virtual ~IComposeManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateComposeDeployment(
            std::wstring const & deploymentName,
            std::wstring const &composeFilePath,
            std::wstring const &overrideFilePath,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool const isPasswordEncrypted,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateComposeDeployment(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateComposeDeployment(
            std::wstring const & deploymentName,
            Common::ByteBuffer && composeFile,
            Common::ByteBuffer && overrideFile,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool const isPasswordEncrypted,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetComposeDeploymentStatusList(
            ServiceModel::ComposeDeploymentStatusQueryDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetComposeDeploymentStatusList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ComposeDeploymentStatusQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginUpgradeComposeDeployment(
            ServiceModel::ComposeDeploymentUpgradeDescription const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndUpgradeComposeDeployment(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetComposeDeploymentUpgradeProgress(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetComposeDeploymentUpgradeProgress(
            Common::AsyncOperationSPtr const &,
            __inout ServiceModel::ComposeDeploymentUpgradeProgress &result) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteComposeDeployment(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteComposeDeployment(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRollbackComposeDeployment(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRollbackComposeDeployment(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
