// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ProcessReportRequestAsyncOperation
            : public ProcessRequestAsyncOperation
            , public IReportRequestContextProcessor
            , public ISequenceStreamRequestContextProcessor
        {
            DENY_COPY(ProcessReportRequestAsyncOperation)
        public:
            ProcessReportRequestAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                Transport::MessageUPtr && request,
                Federation::RequestReceiverContextUPtr && requestContext,
                Common::TimeSpan const timeout, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~ProcessReportRequestAsyncOperation();

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

            // IRequestContextProcessor methods
            virtual void OnReportProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                ReportRequestContext const & requestContext,
                Common::ErrorCode const & error) override;

            virtual void OnSequenceStreamProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                SequenceStreamRequestContext const & requestContext,
                Common::ErrorCode const & error) override;

        protected:
            virtual void HandleRequest(Common::AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnTimeout(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            static Common::TimeSpan TimeoutWithoutSendBuffer(Common::TimeSpan const timeout);
            static bool CanConsiderSuccess(Common::ErrorCode const & error);

            void CheckIfAllRequestsProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                uint64 pendingRequests);

        private:
            // Number of reports batched in the message
            // plus the number of sequence streams that need to be persisted to store
            uint64 requestCount_;
            
            // The sequence stream information contained in the message
            // associated with the number of pending contexts.
            // When the number gets to 0, a sequence stream context is created to 
            // persist the new range to store.
            // If persistance is successful, an ACK is added to reply
            using SequenceStreamWithPendingCountPair = std::pair<SequenceStreamInformation, uint64>;
            std::vector<SequenceStreamWithPendingCountPair> sequenceStreams_;
            
            // Vector with results for each request
            // It will be sent back to requestor on Success
            std::vector<HealthReportResult> reportResults_;

            // Vector with ACKs for sequence stream results
            std::vector<SequenceStreamResult> sequenceStreamResults_;

            Common::ExclusiveLock lock_;

            Common::ActivityId currentNestedActivityId_;
        };
    }
}
