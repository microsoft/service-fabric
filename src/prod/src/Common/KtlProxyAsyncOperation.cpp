// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/KtlProxyAsyncOperation.h"

namespace Common
{
    KtlProxyAsyncOperation::~KtlProxyAsyncOperation()
    {
    }

    void KtlProxyAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        KAsyncContextBase::CompletionCallback callback(this, &KtlProxyAsyncOperation::OnKtlOperationCompleted);

        //
        // To keep this asyncoperation alive till the KTL async operation completes, we hold a 
        // self reference.
        //
        thisSPtr_ = shared_from_this();

        NTSTATUS status = StartKtlAsyncOperation(parentAsyncContext_, callback);
        if (!NT_SUCCESS(status))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
            thisSPtr_ = nullptr;
            return;
        }
    }

    void KtlProxyAsyncOperation::OnKtlOperationCompleted(__in KAsyncContextBase *const parent, __in KAsyncContextBase &context)
    {
        UNREFERENCED_PARAMETER(parent);
        UNREFERENCED_PARAMETER(context);
        
        //
        // KTL operations are completed on KTL worker thread and User callback 
        // has to be invoked on the System threadpool thread.
        //
        Threadpool::Post([this](){ this->ExecuteCallback(); });
    }

    void KtlProxyAsyncOperation::ExecuteCallback()
    {
        //
        // Complete this async operation and invoke the user callback.
        //
        TryComplete(shared_from_this(), ErrorCode::FromNtStatus(thisKtlOperation_->Status()));

        //
        // Release the self reference.
        //
        thisSPtr_ = nullptr;
    }

    void KtlProxyAsyncOperation::OnCancel()
    {
        thisKtlOperation_->Cancel();
    }
}
