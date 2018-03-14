// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClusterManagerQueryMessageHandler: public ClusterManager::ClientRequestAsyncOperation
        {
        public:
            ClusterManagerQueryMessageHandler(
                __in ClusterManagerReplica &,
                __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
                Transport::MessageUPtr &&,
                Federation::RequestReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);            

        protected:
            virtual Common::AsyncOperationSPtr BeginAcceptRequest(
                std::shared_ptr<ClientRequestAsyncOperation> &&,
                Common::TimeSpan const, 
                Common::AsyncCallback const &, 
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &);

        private:
            Query::QueryMessageHandlerUPtr & queryMessageHandler_;

        };
    }
}
