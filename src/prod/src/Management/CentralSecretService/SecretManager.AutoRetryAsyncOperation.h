// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        typedef std::function<Common::AsyncOperationSPtr(Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parentSPtr)> CreateAndStartCallback;

        class SecretManager::AutoRetryAsyncOperation
            : public Common::TimedAsyncOperation
        {
        public:
            AutoRetryAsyncOperation(
                CreateAndStartCallback const & createAndStartCallback,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operationSPtr, __out Common::AsyncOperationSPtr & innerOperationSPtr);

        protected:
            __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
            Common::ActivityId const & get_ActivityId() const { return activityId_; }

            virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCancel() override;
            virtual void OnTimeout(AsyncOperationSPtr const & thisSPtr) override;

        private:
            Common::AsyncOperationSPtr innerOperationSPtr_;
            CreateAndStartCallback const createAndStartCallback_;
            Common::ActivityId const activityId_;
            Common::TimerSPtr retryTimer_;
            int retriedTimes_;
            bool cancellationRequested_;

            static int const MaxRetryTimes;
            static Common::TimeSpan const MaxRetryDelay;

            bool IsRetryable(Common::ErrorCode const & error);
            void Retry(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);
            void CompleteOperation(AsyncOperationSPtr const & operationSPtr, bool expectedCompletedSynchronously);
        };
    }
}