// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;

void HttpServerImpl::AsyncKtlServiceOperationBase::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{
    KAsyncServiceBase::OpenCompletionCallback callback(this, &AsyncKtlServiceOperationBase::OnOperationComplete);

    //
    // Self reference.
    //
    thisSPtr_ = shared_from_this();

    NTSTATUS status = StartKtlServiceOperation(parentAsyncContext_, callback);
    if (!NT_SUCCESS(status))
    {
        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        thisSPtr_ = nullptr;
    }
}

void HttpServerImpl::AsyncKtlServiceOperationBase::OnCancel()
{
    // no op.
}

void HttpServerImpl::AsyncKtlServiceOperationBase::OnOperationComplete(
    __in KAsyncContextBase* const parent,
    __in KAsyncServiceBase& currentInstance,
    __in NTSTATUS status)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(currentInstance);

    TryComplete(shared_from_this(), ErrorCode::FromNtStatus(status));
    thisSPtr_ = nullptr;
}
