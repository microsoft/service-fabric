// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

KHttpRequestReadBodyChunkAsyncOperation::KHttpRequestReadBodyChunkAsyncOperation(
    KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext,
    KHttpServerRequest::SPtr const &request,
    KMemRef &memRef,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : KtlProxyAsyncOperation(asyncContext.RawPtr(), nullptr, callback, parent)
    , asyncContext_(asyncContext)
    , request_(request)
    , memRef_(memRef)
{
}

ErrorCode KHttpRequestReadBodyChunkAsyncOperation::End(AsyncOperationSPtr const& operation, KMemRef &memRef)
{
    auto thisSPtr = AsyncOperation::End<KHttpRequestReadBodyChunkAsyncOperation>(operation);
    if (thisSPtr->Error.IsSuccess())
    {
        memRef = move(thisSPtr->memRef_);
    }

    return thisSPtr->Error;
}

NTSTATUS KHttpRequestReadBodyChunkAsyncOperation::StartKtlAsyncOperation(
    __in KAsyncContextBase * const parentAsyncContext,
    __in KAsyncContextBase::CompletionCallback callback)
{
    asyncContext_->Reuse();
    asyncContext_->StartReceiveRequestData(
        *request_,
        &memRef_,
        parentAsyncContext,
        callback);

    return STATUS_SUCCESS;
}

