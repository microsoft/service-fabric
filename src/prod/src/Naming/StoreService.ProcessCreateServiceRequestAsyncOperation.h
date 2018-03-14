// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessCreateServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessCreateServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_RequestBody, put=set_RequestBody)) CreateServiceMessageBody const & RequestBody;
        CreateServiceMessageBody const & get_RequestBody() const { return body_; }
        void set_RequestBody(CreateServiceMessageBody && value) { body_ = value; }

        void OnCompleted();

        DEFINE_PERF_COUNTERS ( AOCreateService )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:
        // ******************************************
        // Please refer to comments for name creation
        //
        // Note that CreateService will automatically 
        // create the name if it does not exist.
        // ******************************************

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartCreateService(Common::AsyncOperationSPtr const & thisSPtr);

        void OnTentativeWriteComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRecursiveCreateName(Common::AsyncOperationSPtr const &);

        void OnRecursiveCreateNameComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartResolveNameOwner(Common::AsyncOperationSPtr const &);

        void OnResolveNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToNameOwner(Common::AsyncOperationSPtr const &);

        void OnRequestReplyToPeerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishCreateService(Common::AsyncOperationSPtr const &);

        void OnWriteServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void RevertTentativeCreate(Common::AsyncOperationSPtr const &);

        void OnRevertTentativeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        bool ShouldTerminateProcessing() const override;

        CreateServiceMessageBody body_;
        bool hasNamedLock_;
        SystemServices::SystemServiceLocation nameOwnerLocation_;
        bool isRebuildFromFM_;

        Common::ErrorCode revertError_;

    };
}
