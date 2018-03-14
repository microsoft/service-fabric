// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CompletedAsyncOperation : public AsyncOperation
    {
        DENY_COPY(CompletedAsyncOperation)

    public:
        CompletedAsyncOperation(ErrorCode const & error, AsyncCallback const & callback, AsyncOperationSPtr const& parent);
        CompletedAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const& parent);
        virtual ~CompletedAsyncOperation(void);
        static ErrorCode End(AsyncOperationSPtr const& asyncOperation);

    protected:
        virtual void OnStart(AsyncOperationSPtr const& thisSPtr);

    private:
        ErrorCode completionError_;
    };
}

