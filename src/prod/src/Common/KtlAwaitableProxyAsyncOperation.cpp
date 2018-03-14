// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <KTpl.h>

namespace Common
{
    KtlAwaitableProxyAsyncOperation::~KtlAwaitableProxyAsyncOperation()
    {
    }

    void KtlAwaitableProxyAsyncOperation::OnStart(AsyncOperationSPtr const &)
    {
        //
        // To keep this asyncoperation alive till the KTL async operation completes, we hold a 
        // self reference.
        //
        thisSPtr_ = shared_from_this();

        StartTask();
    }

    ktl::Task KtlAwaitableProxyAsyncOperation::StartTask()
    {
        NTSTATUS status;

        try
        {
            status = co_await ExecuteOperationAsync();
        }
        catch (ktl::Exception & ex)
        {
            status = ex.GetStatus();
        }
        // Any other exception type should bring down the process.

        //
        // Complete this async operation and invoke the user callback.
        //
        TryComplete(shared_from_this(), ErrorCode::FromNtStatus(status));

        //
        // Release the self reference.
        //
        thisSPtr_ = nullptr;

        co_return;
    }
}
