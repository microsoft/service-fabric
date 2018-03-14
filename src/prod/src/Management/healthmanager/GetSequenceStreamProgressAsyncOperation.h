// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class GetSequenceStreamProgressAsyncOperation : 
            public ProcessRequestAsyncOperation,
            public ISequenceStreamRequestContextProcessor
        {
        public:
            GetSequenceStreamProgressAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                Transport::MessageUPtr && request,
                Federation::RequestReceiverContextUPtr && requestContext,
                Common::TimeSpan const timeout, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ~GetSequenceStreamProgressAsyncOperation();

            // IRequestContextProcessor methods
            virtual void OnSequenceStreamProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                SequenceStreamRequestContext const & requestContext,
                Common::ErrorCode const & error) override;
            
        protected:
            
            virtual void HandleRequest(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void GetProgress(Common::AsyncOperationSPtr const & thisSPtr, bool isRetry);

            SequenceStreamId sequenceStreamId_;
            FABRIC_INSTANCE_ID instance_;
        };
    }
}
