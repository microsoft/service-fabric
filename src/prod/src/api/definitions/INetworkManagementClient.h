// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class INetworkManagementClient
    {
    public:
        virtual ~INetworkManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateNetwork(
            ServiceModel::ModelV2::NetworkResourceDescription const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndCreateNetwork(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteNetwork(
            ServiceModel::DeleteNetworkDescription const &deleteDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndDeleteNetwork(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNetworkList(
            ServiceModel::NetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetNetworkList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::ModelV2::NetworkResourceDescriptionQueryResult> &networkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNetworkApplicationList(
            ServiceModel::NetworkApplicationQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetNetworkApplicationList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::NetworkApplicationQueryResult> &networkApplicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNetworkNodeList(
            ServiceModel::NetworkNodeQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetNetworkNodeList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::NetworkNodeQueryResult> &networkNodeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationNetworkList(
            ServiceModel::ApplicationNetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetApplicationNetworkList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::ApplicationNetworkQueryResult> &applicationNetworkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedNetworkList(
            ServiceModel::DeployedNetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetDeployedNetworkList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::DeployedNetworkQueryResult> &deployedNetworkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedNetworkCodePackageList(
            ServiceModel::DeployedNetworkCodePackageQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation) = 0;
        virtual Common::ErrorCode EndGetDeployedNetworkCodePackageList(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<ServiceModel::DeployedNetworkCodePackageQueryResult> &deployedNetworkCodePackageList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;
    };
}
