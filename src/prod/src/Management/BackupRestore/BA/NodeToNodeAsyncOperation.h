// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class NodeToNodeAsyncOperation
            : public RequestResponseAsyncOperationBase
        {
        public:
            NodeToNodeAsyncOperation(
                Transport::MessageUPtr && message,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Reliability::ServiceResolver& serviceResolver,
                Federation::FederationSubsystem & federation,
                Api::IQueryClientPtr queryClientPtr,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ~NodeToNodeAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply,
                __out Transport::IpcReceiverContextUPtr & receiverContext);

            __declspec (property(get = get_ReceiverContext)) Transport::IpcReceiverContextUPtr & ReceiverContext;
            Transport::IpcReceiverContextUPtr & get_ReceiverContext() { return receiverContext_; }

            __declspec(property(get = get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
            Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

            __declspec(property(get = get_Reply, put = set_Reply)) Transport::MessageUPtr & Reply;
            Transport::MessageUPtr const & get_Reply() const { return reply_; }
            void set_Reply(Transport::MessageUPtr && value) { reply_ = std::move(value); }

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCompleted() override;

            virtual void OnRouteToNodeSuccessful(
                Common::AsyncOperationSPtr const & thisSPtr,
                Transport::MessageUPtr & reply);

            virtual void OnRouteToNodeFailed(
                Common::AsyncOperationSPtr const & thisSPtr,
                Common::ErrorCode const & error);

        private:

            void OnResolveServicePartitionCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnGetReplicaListCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnGetNodeListCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            void BeginRouteToNode(Federation::NodeId const nodeId, uint64 instanceId, Common::AsyncOperationSPtr const & operation);

            void OnRouteToNodeRequestComplete(
                Common::AsyncOperationSPtr const& asyncOperation,
                bool expectedCompletedSynchronously);

            Transport::IpcReceiverContextUPtr receiverContext_; 
            Reliability::ServiceResolver & serviceResolver_;
            Federation::FederationSubsystem & federation_;
            Api::IQueryClientPtr queryClientPtr_;
        };
    }
}
