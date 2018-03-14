// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class KtlAwaitableProxyAsyncOperation
        : public AllocableAsyncOperation
    {
        DENY_COPY(KtlAwaitableProxyAsyncOperation)

    public:

        virtual ~KtlAwaitableProxyAsyncOperation();

    protected:

        KtlAwaitableProxyAsyncOperation(
            AsyncCallback const& callback,
            AsyncOperationSPtr const& parent)
            : AllocableAsyncOperation(callback, parent)
        {
        }

        virtual ktl::Awaitable<NTSTATUS>
        ExecuteOperationAsync() = 0;

        void OnStart(AsyncOperationSPtr const & thisSPtr) override;

    private:

        ktl::Task StartTask();

        AsyncOperationSPtr thisSPtr_;
    };
}
