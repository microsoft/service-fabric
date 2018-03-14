// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessInnerDeleteServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessInnerDeleteServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_RequestBody, put=set_RequestBody)) DeleteServiceMessageBody const & RequestBody;
        DeleteServiceMessageBody const & get_RequestBody() const { return body_; }
        void set_RequestBody(DeleteServiceMessageBody && value) { body_ = value; }

        void OnCompleted();

        DEFINE_PERF_COUNTERS ( NODeleteService )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:        
        void StartRemoveService(Common::AsyncOperationSPtr const &);

        void OnTentativeRemoveComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToFM(Common::AsyncOperationSPtr const &);

        void OnRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishRemoveService(Common::AsyncOperationSPtr const &);

        void OnRemoveServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        DeleteServiceMessageBody body_;
    };
}
