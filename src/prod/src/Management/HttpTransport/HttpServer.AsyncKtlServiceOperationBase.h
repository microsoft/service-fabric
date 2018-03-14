// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::AsyncKtlServiceOperationBase
        : public Common::AsyncOperation
    {
        DENY_COPY(AsyncKtlServiceOperationBase)

    protected:
        AsyncKtlServiceOperationBase(
            KAsyncContextBase* const parentAsyncContext,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : Common::AsyncOperation(callback, parent)
            , parentAsyncContext_(parentAsyncContext)
        {
        }

        //
        // Implement the actual KTL Service Open/Close call here.
        //
        virtual NTSTATUS StartKtlServiceOperation(
            __in KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase::OpenCompletionCallback callbackPtr) = 0;

        void OnStart(__in Common::AsyncOperationSPtr const& thisSPtr);
        virtual void OnCancel();

    private:

        void OnOperationComplete(__in KAsyncContextBase* const, __in KAsyncServiceBase&, __in NTSTATUS);
        KAsyncContextBase* const parentAsyncContext_;
        Common::AsyncOperationSPtr thisSPtr_;
    };
}
