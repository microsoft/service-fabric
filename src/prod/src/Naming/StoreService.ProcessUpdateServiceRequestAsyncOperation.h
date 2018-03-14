// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessUpdateServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessUpdateServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_RequestBody, put=set_RequestBody)) UpdateServiceRequestBody const & RequestBody;
        UpdateServiceRequestBody const & get_RequestBody() const { return body_; }
        void set_RequestBody(UpdateServiceRequestBody && value) { body_ = value; }

        void OnCompleted();

        DEFINE_PERF_COUNTERS ( AOUpdateService )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:
        // ******************************************
        // Please refer to comments for name creation
        //
        // The AO drives consistency of the update operation, so
        // a ServiceUpdateDescription is persisted on the AO
        // until the operation completes successfully at the NO.
        //
        // While the update operation is pending, the UserServiceState
        // on the hierarchy name is marked Updating (the tentative
        // state for an update operation). The state is set back
        // to Created and the persisted ServiceUpdateDescription is 
        // deleted once the NO returns success.
        //
        // The NO does *not* set any tentative state while
        // the update request is pending at the FM. The 
        // updated ServiceDescription is only persisted
        // by the NO after the FM returns success. This is so
        // that GetServiceDescription can continue to return the
        // old ServiceDescription while an update is pending.
        //
        // From the client's perspective, you do not expect to
        // read the new ServiceDescription from Naming until
        // the update request completes successfully. Even then,
        // the various caches around the cluster may take some
        // time to update after the FM starts acting on the
        // updated ServiceDescription.
        //
        // ******************************************

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartUpdateService(Common::AsyncOperationSPtr const & thisSPtr);

        void OnTentativeUpdateComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartResolveNameOwner(Common::AsyncOperationSPtr const &);

        void OnResolveNameOwnerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToNameOwner(Common::AsyncOperationSPtr const &);

        void OnRequestReplyToPeerComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishUpdateService(Common::AsyncOperationSPtr const &);

        void OnUpdateServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        UpdateServiceRequestBody body_;
        bool hasNamedLock_;
        SystemServices::SystemServiceLocation nameOwnerLocation_;
        Common::ErrorCode validationError_;
    };
}

