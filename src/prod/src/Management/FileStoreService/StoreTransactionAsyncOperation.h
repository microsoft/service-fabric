// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        typedef std::function<Common::ErrorCode(Store::StoreTransaction const & )> StoreOperation;
        class StoreTransactionAsyncOperation
            : public Common::AsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(StoreTransactionAsyncOperation)
        public:
            StoreTransactionAsyncOperation(
                RequestManager & requestManager,
                bool const useSimpleTx,
                StoreOperation const & storeOperation,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
            void OnCompleted();
            void OnCancel();

        private:
            void CreateStoreTransaction(Common::AsyncOperationSPtr const & thisSPtr);
            void StartStoreTransaction(StoreTransactionUPtr && storeTx, Common::AsyncOperationSPtr const & thisSPtr);

            void OnRetriableError(Common::AsyncOperationSPtr const & thisSPtr);
            void OnCommitCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);            
            bool IsRetryableError(Common::ErrorCode const & error);
            Common::TimeSpan GetDueTime();

        private:
            RequestManager & requestManager_;
            StoreOperation storeOperation_;
            
            bool const useSimpleTx_;
            Common::TimeoutHelper timeoutHelper_;
            uint failureCount_;
            Common::ExclusiveLock lock_;
            Common::TimerSPtr retryTimer_;
            bool canceled_;
            bool initialEnqueueSucceeded_;
        };
    }
}
