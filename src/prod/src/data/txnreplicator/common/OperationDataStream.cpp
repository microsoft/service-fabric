// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

OperationDataStream::OperationDataStream()
    : KObject()
    , KShared()
    , IOperationDataStream()
{
}

OperationDataStream::~OperationDataStream()
{
}

void OperationDataStream::Dispose()
{
}

HRESULT OperationDataStream::CreateAsyncGetNextContext(__out AsyncGetNextContext::SPtr & asyncContext) noexcept
{
    AsyncGetNextContextImpl::SPtr context = _new(OPERATIONDATASTREAM_TAG, GetThisAllocator()) AsyncGetNextContextImpl();
    if (!context)
    {
        return StatusConverter::ToHResult(STATUS_INSUFFICIENT_RESOURCES);
    }

    NTSTATUS status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return StatusConverter::ToHResult(status);
    }

    context->parent_ = this;

    asyncContext.Attach(context.DownCast<AsyncGetNextContext>());
    context.Detach();

    return S_OK;
}

//
// GetNext Operation
//
OperationDataStream::AsyncGetNextContextImpl::AsyncGetNextContextImpl()
    : parent_()
    , result_()
{
}

OperationDataStream::AsyncGetNextContextImpl::~AsyncGetNextContextImpl()
{
}

HRESULT OperationDataStream::AsyncGetNextContextImpl::StartGetNext(
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback) noexcept
{
    Start(parentAsyncContext, callback);

    return S_OK;
}

void OperationDataStream::AsyncGetNextContextImpl::OnStart() noexcept
{
    DoWork();
}

Task OperationDataStream::AsyncGetNextContextImpl::DoWork() noexcept
{
    KCoShared$ApiEntry();
    
    NTSTATUS status = co_await parent_->GetNextAsync(
        CancellationToken::None,
        result_);

    Complete(status);
}

HRESULT OperationDataStream::AsyncGetNextContextImpl::GetResult(OperationData::CSPtr & result) noexcept
{
    result = result_;
    return Result();
}
