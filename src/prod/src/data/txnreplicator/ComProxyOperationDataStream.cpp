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

ComProxyOperationDataStream::ComProxyOperationDataStream(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricOperationDataStream> comOperationDataStream)
    : OperationDataStream()
    , PartitionedReplicaTraceComponent(traceId)
    , comOperationDataStream_(comOperationDataStream)
{
    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyOperationDataStream",
        reinterpret_cast<uintptr_t>(this));
}

ComProxyOperationDataStream::~ComProxyOperationDataStream()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyOperationDataStream",
        reinterpret_cast<uintptr_t>(this));
}

NTSTATUS ComProxyOperationDataStream::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricOperationDataStream> comOperationDataStream,
    __in KAllocator & allocator,
    __out ComProxyOperationDataStream::SPtr & comProxyOperationDataStream) noexcept
{
    ComProxyOperationDataStream * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComProxyOperationDataStream(traceId, comOperationDataStream);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    comProxyOperationDataStream = pointer;

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> ComProxyOperationDataStream::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    __out OperationData::CSPtr & result) noexcept
{
    UNREFERENCED_PARAMETER(cancellationToken);
    AsyncGetNextContext::SPtr context = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) AsyncGetNextContext();
    if (!context)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    context->parent_ = this;
    
    NTSTATUS status = co_await context->GetNextAsync(result);

    co_return status;
}

//
// Callout adapter to invoke getnext API on v1 replicator to get the next copy context operation
//
ComProxyOperationDataStream::AsyncGetNextContext::AsyncGetNextContext()
    : parent_()
    , result_()
    , tcs_()
{
}

ComProxyOperationDataStream::AsyncGetNextContext::~AsyncGetNextContext()
{
}

Awaitable<NTSTATUS> ComProxyOperationDataStream::AsyncGetNextContext::GetNextAsync(__out OperationData::CSPtr & operation) noexcept
{
    NTSTATUS status = AwaitableCompletionSource<NTSTATUS>::Create(GetThisAllocator(), TRANSACTIONALREPLICATOR_TAG, tcs_);
    
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    if (!parent_->comOperationDataStream_)
    {
        operation = nullptr;
        co_return status;
    }

    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = parent_->comOperationDataStream_->BeginGetNext(
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
        operation = GetResult();
    }

    co_return status;
}
            
HRESULT ComProxyOperationDataStream::AsyncGetNextContext::OnEnd(__in IFabricAsyncOperationContext & context) noexcept
{
    ComPointer<IFabricOperationData> comOperationData;

    HRESULT hr = parent_->comOperationDataStream_->EndGetNext(
        &context,
        comOperationData.InitializationAddress());

    if (SUCCEEDED(hr))
    {
        if (comOperationData.GetRawPointer() == nullptr)
        {
            result_ = nullptr;
        }
        else
        {
            ULONG count;
            FABRIC_OPERATION_DATA_BUFFER const * buffers;

            hr = comOperationData->GetData(&count, &buffers);

            if (SUCCEEDED(hr))
            {
                NTSTATUS status = ComProxyOperationData::Create(
                    count,
                    buffers,
                    GetThisAllocator(),
                    result_);

                if (!NT_SUCCESS(status))
                {
                    hr = StatusConverter::ToHResult(status);
                }
            }
        }
    }

    NTSTATUS status = StatusConverter::Convert(hr);
    tcs_->SetResult(status);
    return hr;
}

OperationData::CSPtr ComProxyOperationDataStream::AsyncGetNextContext::GetResult() noexcept
{
    return result_;
}
