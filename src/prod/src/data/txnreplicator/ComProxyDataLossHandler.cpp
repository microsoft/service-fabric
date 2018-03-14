// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;

NTSTATUS ComProxyDataLossHandler::Create(
    __in ComPointer<IFabricDataLossHandler> & dataLossHandler,
    __in KAllocator & allocator,
    __out ComProxyDataLossHandler::SPtr & result)
{
    ComProxyDataLossHandler::SPtr tmpPtr(_new(COM_PROXY_DATALOSS_HANDLER_TAG, allocator)ComProxyDataLossHandler(dataLossHandler));

    if (!tmpPtr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(tmpPtr->Status()))
    {
        return tmpPtr->Status();
    }

    result = Ktl::Move(tmpPtr);
    return STATUS_SUCCESS;
}

Awaitable<bool> ComProxyDataLossHandler::DataLossAsync(__in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    BOOLEAN result = FALSE;

    AsyncDataLossContext::SPtr context = _new(COM_PROXY_DATALOSS_HANDLER_TAG, GetThisAllocator()) AsyncDataLossContext();
    if (!context)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    context->parent_ = this;

    NTSTATUS status = co_await context->DataLossAsync(&result);
    if (!NT_SUCCESS(status))
    {
        throw Exception(status);
    }

    co_return result == TRUE;
}

Awaitable<NTSTATUS> ComProxyDataLossHandler::AsyncDataLossContext::DataLossAsync(__out BOOLEAN * isStateChanged)
{
    NTSTATUS status = AwaitableCompletionSource<NTSTATUS>::Create(GetThisAllocator(), COM_PROXY_DATALOSS_HANDLER_TAG, tcs_);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = parent_->dataLossHandler_->BeginOnDataLoss(
        this,
        context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        status = StatusConverter::Convert(hr);
        co_return status;
    }

    OnComOperationStarted(
        *context.GetRawPointer(),
        nullptr,
        nullptr);

    status = co_await tcs_->GetAwaitable();

    if (NT_SUCCESS(status))
    {
        *isStateChanged = isStateChanged_;
    }

    co_return status;
}

HRESULT ComProxyDataLossHandler::AsyncDataLossContext::OnEnd(__in IFabricAsyncOperationContext & context)
{
    ASSERT_IFNOT(tcs_ != nullptr, "AwaitableCompletionSource should not be nullptr");

    HRESULT hr = parent_->dataLossHandler_->EndOnDataLoss(
        &context,
        &isStateChanged_);

    NTSTATUS status = StatusConverter::Convert(hr);
    tcs_->SetResult(status);
    return hr;
}

// Callout adapter
ComProxyDataLossHandler::AsyncDataLossContext::AsyncDataLossContext()
    : isStateChanged_(FALSE)
    , parent_()
    , tcs_()
{
}

ComProxyDataLossHandler::AsyncDataLossContext::~AsyncDataLossContext()
{
}

ComProxyDataLossHandler::ComProxyDataLossHandler(__in Common::ComPointer<IFabricDataLossHandler> & dataLossHandler)
    : KObject()
    , KShared()
    , dataLossHandler_(dataLossHandler)
{
}

ComProxyDataLossHandler::~ComProxyDataLossHandler()
{
}

