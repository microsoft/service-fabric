// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        //--------
        // This class represents the SF service that manages (overlay) network definition and pool allocation.
        // Network definition defines a network: name, id, subnets, ip and mac pools.
        // When a pool allocation is completed, route updates are sent to every node where the network is active.
        class NetworkInventoryService :
            public Common::ComponentRoot,
            public Common::TextTraceComponent<Common::TraceTaskCodes::FM>
        {
            DENY_COPY(NetworkInventoryService);

        public:

            NetworkInventoryService(Reliability::FailoverManagerComponent::FailoverManager & fm);
            ~NetworkInventoryService();

            Common::ErrorCode Initialize();
            void Close();

            __declspec (property(get=get_FM)) Reliability::FailoverManagerComponent::FailoverManager & FM;
            Reliability::FailoverManagerComponent::FailoverManager & get_FM() { return fm_; }

            __declspec (property(get=get_Id)) std::wstring const& Id;
            std::wstring const& get_Id() const { return fm_.Id; }


            AsyncOperationSPtr BeginSendPublishNetworkTablesRequestMessage(
                PublishNetworkTablesRequestMessage const & params,
                const Federation::NodeInstance& nodeInstance,
                TimeSpan const timeout,
                AsyncCallback const & callback);

            ErrorCode EndSendPublishNetworkTablesRequestMessage(AsyncOperationSPtr const & operation,
                __out NetworkErrorCodeResponseMessage & reply);

            Common::ErrorCode EnumerateNetworks(
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            ErrorCode ProcessNIMQuery(Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            Common::ErrorCode GetNetworkApplicationList(
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            Common::ErrorCode GetNetworkNodeList(
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            Common::ErrorCode GetApplicationNetworkList(
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            std::vector<ServiceModel::ApplicationNetworkQueryResult> GetApplicationNetworkList(
                std::wstring const & applicationName);

            Common::ErrorCode GetDeployedNetworkList(
                ServiceModel::QueryArgumentMap const & queryArgs,
                ServiceModel::QueryResult & queryResult,
                Common::ActivityId const & activityId);

            Common::AsyncOperationSPtr BeginNetworkCreate(
                CreateNetworkMessageBody & requestBody,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                AsyncOperationSPtr const & state);

            Common::ErrorCode EndNetworkCreate(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkCreateResponseMessage> & reply);

            Common::AsyncOperationSPtr BeginNetworkAssociation(
                std::wstring const & networkName,
                std::wstring const & applicationName,
                Common::AsyncCallback const & callback,
                AsyncOperationSPtr const & state);

            Common::ErrorCode EndNetworkAssociation(
                Common::AsyncOperationSPtr const & operation);

            Common::AsyncOperationSPtr BeginNetworkDissociation(
                std::wstring const & networkName,
                std::wstring const & applicationName,
                Common::AsyncCallback const & callback,
                AsyncOperationSPtr const & state);

            Common::ErrorCode EndNetworkDissociation(
                Common::AsyncOperationSPtr const & operation);

        private:

            //--------
            // Register to handle federation messages from nodes.
            void RegisterRequestHandler();
            void UnregisterRequestHandler();

            void ProcessMessage(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);

            void OnNetworkAllocationRequest(
                __in Transport::MessageUPtr & message, 
                __in Federation::RequestReceiverContextUPtr & context);

            void OnNetworkCreateRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);

            void OnNetworkRemoveRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);

            void OnNetworkValidateRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);
            
            void OnNetworkNodeRemoveRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);

            void OnRequestPublishNetworkTablesRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);

            void OnNetworkEnumerationRequest(
                __in Transport::MessageUPtr & message,
                __in Federation::RequestReceiverContextUPtr & context);
            
            ErrorCode InternalStartRefreshProcessing();
            void OnTimeout(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);


            template <class TRequest, class TReply>
            class NIMRequestAsyncOperation;
            AsyncOperationSPtr periodicAsyncTask_;

            Reliability::FailoverManagerComponent::FailoverManager & fm_;
            MUTABLE_RWLOCK(FM.NetworkInventoryService, lockObject_);
   
            std::unique_ptr<NetworkInventoryAllocationManager> allocationManagerUPtr_;
            MACAllocationPoolSPtr macAddressPool_;
            VSIDAllocationPoolSPtr vsidPool_;
        };

        typedef std::unique_ptr<NetworkInventoryService> NetworkInventoryServiceUPtr;
    }
}
