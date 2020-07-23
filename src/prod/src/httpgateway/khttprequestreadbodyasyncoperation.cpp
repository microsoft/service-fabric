// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

KHttpRequestReadBodyAsyncOperation::KHttpRequestReadBodyAsyncOperation(
    KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext,
    KHttpServerRequest::SPtr const &request,
    ULONG size,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : KtlProxyAsyncOperation(asyncContext.RawPtr(), nullptr, callback, parent)
    , asyncContext_(asyncContext)
    , request_(request)
    , size_(size)
{
}

ErrorCode KHttpRequestReadBodyAsyncOperation::End(AsyncOperationSPtr const& operation, KBuffer::SPtr &buffer)
{
    auto thisSPtr = AsyncOperation::End<KHttpRequestReadBodyAsyncOperation>(operation);
    if (thisSPtr->Error.IsSuccess())
    {
        buffer = move(thisSPtr->body_);
    }

    return thisSPtr->Error;
}

NTSTATUS KHttpRequestReadBodyAsyncOperation::StartKtlAsyncOperation(
    __in KAsyncContextBase * const parentAsyncContext,
    __in KAsyncContextBase::CompletionCallback callback)
{
    asyncContext_->StartReceiveRequestData(
        *request_,
        size_,
        body_,
        parentAsyncContext,
        callback);

    return STATUS_SUCCESS;
}

