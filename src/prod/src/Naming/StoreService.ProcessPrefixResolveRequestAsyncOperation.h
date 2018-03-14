// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessPrefixResolveRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessPrefixResolveRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( PrefixResolve )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr && request);
        void PerformRequest(Common::AsyncOperationSPtr const &);  

    private:
        void StartPrefixResolution(Common::AsyncOperationSPtr const & thisSPtr);
        void StartResolveNameOwner(Common::AsyncOperationSPtr const & thisSPtr);
        void OnResolveNameOwnerComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);
        void StartRequestToNameOwner(Common::AsyncOperationSPtr const & thisSPtr);
        void OnRequestReplyToPeerComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);
        Common::ErrorCode SetCurrentPrefix();

        Common::NamingUri currentPrefix_;
        SystemServices::SystemServiceLocation nameOwnerLocation_;
    };
}

