// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // REST Handler for FabricClient API's IFabricNetworkManagementClient
    //
    class NetworksHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(NetworksHandler)

    public:

        NetworksHandler(HttpGatewayImpl & server);
        virtual ~NetworksHandler();

        Common::ErrorCode Initialize();

    private:

        void GetAllNetworks(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllNetworksComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetNetworkByName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetNetworkByNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAllNetworkApplications(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllNetworkApplicationsComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetNetworkApplicationByName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetNetworkApplicationByNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAllNetworkNodes(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllNetworkNodesComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetNetworkNodeByName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetNetworkNodeByNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CreateNetwork(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateNetworkComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeleteNetwork(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteNetworkComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        bool GetNetworkQueryDescription(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool includeNetworkName,
            __out ServiceModel::NetworkQueryDescription & queryDescription);

        bool GetNetworkApplicationQueryDescription(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool includeApplicationName,
            __out ServiceModel::NetworkApplicationQueryDescription & queryDescription);

        bool GetNetworkNodeQueryDescription(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool includeNodeName,
            __out ServiceModel::NetworkNodeQueryDescription & queryDescription);
    };
}
