// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::StartNodeAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:        
        static const std::wstring TraceId;
        
        StartNodeAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr);

        Common::ErrorCode InitializeClient(
            std::wstring const &ipAddressOrFQDN, 
            ULONG port);
        
        void ForwardToNode(Common::AsyncOperationSPtr const& thisSPtr);

        void MakeRequest(
            Common::AsyncOperationSPtr const& thisSPtr,
            std::wstring const &ipAddressOrFQDN,
            ULONG port);

        void OnRequestCompleted(
             Common::AsyncOperationSPtr const & asyncOperation,
             bool expectedCompletedSynchronously);

        void GetEndpointForZombieNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetEndpointForZombieNodeComplete(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool expectedCompletedSynchronously);

        Common::ErrorCode IncomingInstanceIdMatchesFile(StartNodeRequestMessageBody const & body);
        Common::ErrorCode TryCreateQueryClient();

        Api::IClusterManagementClientPtr clusterManagementClientPtr_;
        Api::IQueryClientPtr queryClientPtr_;
        StartNodeRequestMessageBody startNodeRequestBody_;
        
    private:              
        static bool IsNodeRunning(FABRIC_QUERY_NODE_STATUS nodeStatus);
        static bool IsNodeInvalid(FABRIC_QUERY_NODE_STATUS nodeStatus);
    };
}
