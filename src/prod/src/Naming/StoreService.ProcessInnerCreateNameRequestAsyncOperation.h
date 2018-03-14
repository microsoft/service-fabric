// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessInnerCreateNameRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessInnerCreateNameRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( NOCreateName )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);
        void OnCompleted();

    private:
        void StartCreateName(Common::AsyncOperationSPtr const &);
        void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
    };
}
