// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ProcessQueryRequestAsyncOperation : public ProcessRequestAsyncOperation
        {
            DENY_COPY(ProcessQueryRequestAsyncOperation)
        public:
            ProcessQueryRequestAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
                Transport::MessageUPtr && request,
                Federation::RequestReceiverContextUPtr && requestContext,
                Common::TimeSpan const timeout, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~ProcessQueryRequestAsyncOperation();

        protected:            
            virtual void HandleRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void OnProcessQueryMessageComplete(
                Common::AsyncOperationSPtr const & operation, 
                bool expectedCompletedSynchronously);

            Query::QueryMessageHandlerUPtr & queryMessageHandler_;
        };
    }
}
