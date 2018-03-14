// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Base operation for all message processing async operations.
        class ProcessRequestAsyncOperation
            : public Common::TimedAsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::HealthManager>
        {
            DENY_COPY(ProcessRequestAsyncOperation)
        public:
            ProcessRequestAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                Transport::MessageUPtr && request,
                Federation::RequestReceiverContextUPtr && requestContext,
                Common::TimeSpan const timeout, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ProcessRequestAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                Common::ErrorCode && passedThroughError,
                Transport::MessageUPtr && request,
                Federation::RequestReceiverContextUPtr && requestContext,
                Common::TimeSpan const timeout, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~ProcessRequestAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply,
                __out Federation::RequestReceiverContextUPtr & requestContext);
            
        protected:
            
            __declspec(property(get=get_EntityManager)) EntityCacheManager & EntityManager;
            EntityCacheManager & get_EntityManager() const { return entityManager_; }
                        
            __declspec(property(get=get_Request)) Transport::Message & Request;
            Transport::Message & get_Request() const { return *request_; }
            
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

            void SetReply(Transport::MessageUPtr && reply);

            void SetReplyAndComplete(
                Common::AsyncOperationSPtr const & thisSPtr,
                Transport::MessageUPtr && reply,
                Common::ErrorCode && error);

            virtual void HandleRequest(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            EntityCacheManager & entityManager_;
            Common::ErrorCode passedThroughError_;
            Transport::MessageUPtr request_;
            Transport::MessageUPtr reply_;
            Federation::RequestReceiverContextUPtr requestContext_;
        };
    }
}
