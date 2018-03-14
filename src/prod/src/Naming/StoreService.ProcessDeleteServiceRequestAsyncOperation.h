// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessDeleteServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessDeleteServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_RequestBody, put=set_RequestBody)) DeleteServiceMessageBody const & RequestBody;
        DeleteServiceMessageBody const & get_RequestBody() const { return body_; }
        void set_RequestBody(DeleteServiceMessageBody && value) { body_ = value; }

        DEFINE_PERF_COUNTERS ( AODeleteService )

        void CompleteOrScheduleRetry(Common::AsyncOperationSPtr const &, Common::ErrorCode &&, RetryCallback const &) override;
        void UpdateForceDeleteFlag();
        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);
        
        void OnCompleted();
    private:
        // ******************************************
        // Please refer to comments for name creation
        // ******************************************

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRemoveService(Common::AsyncOperationSPtr const &);

        void OnTentativeRemoveComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartResolveNameOwner(Common::AsyncOperationSPtr const &);

        void OnResolveNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToNameOwner(Common::AsyncOperationSPtr const &);

        void OnRequestToNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishRemoveService(Common::AsyncOperationSPtr const &);

        void OnRemoveServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRecursiveDeleteName(Common::AsyncOperationSPtr const &);

        void OnRecursiveDeleteNameComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        bool ShouldTerminateProcessing() const override;

        bool hasNamedLock_;
        SystemServices::SystemServiceLocation nameOwnerLocation_;
        DeleteServiceMessageBody body_;
        bool isForceDelete_;
        bool serviceNotFoundOnAuthorityOwner_;
    };
}
