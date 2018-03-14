// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessInvalidRequestAsyncOperation :
        public ProcessRequestAsyncOperation
    {
    public:
        ProcessInvalidRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore & namingStore,
            __in StoreServiceProperties & properties,
            Common::ErrorCode const & error,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root)
            : ProcessRequestAsyncOperation(
                std::move(request),
                namingStore,
                properties,
                timeout,
                callback,
                root)
            , error_(error)
        {
        }

    protected:

        DEFINE_PERF_COUNTERS ( InvalidOperation )

        void OnStart(Common::AsyncOperationSPtr const & thisSPtr) { this->TryComplete(thisSPtr, error_); }
        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&) { return error_;}
        void PerformRequest(Common::AsyncOperationSPtr const &) { }

    private:
        Common::ErrorCode error_;
    };
}

