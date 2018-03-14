// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ProcessQueryAsyncOperation :
            public Common::TimedAsyncOperation,
            public Common::TextTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>
        {

        public:

            ProcessQueryAsyncOperation(
                __in FaultAnalysisService::FaultAnalysisServiceAgent & owner,
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
                : TimedAsyncOperation(timeout, callback, parent)
                , queryName_(queryName)
                , queryArgs_(queryArgs)
                , activityId_(activityId)
                , owner_(owner)
                , retryTimer_()
                , timerLock_()
            {
            }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr &reply);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
            void OnTimeout(Common::AsyncOperationSPtr const& thisSPtr);
            void OnCancel();
            void OnCompleted();

        private:

            bool IsRetryable(Common::ErrorCode const & error);
            void CancelRetryTimer();

            void ProcessQueryAsyncOperation::OnRequestCompleted(
                Common::AsyncOperationSPtr const & asyncOperation,
                bool isCommandStatusQuery,
                bool expectedCompletedSynchronously);
            
            ServiceModel::QueryArgumentMap queryArgs_;
            Query::QueryNames::Enum queryName_;
            Common::ActivityId const &activityId_;
            FaultAnalysisService::FaultAnalysisServiceAgent & owner_;
            Transport::MessageUPtr reply_;
            Common::TimerSPtr retryTimer_;
            // Synchronizes scheduling and cancelling of the retry timer.
            Common::ExclusiveLock timerLock_;
        };
    }
}

