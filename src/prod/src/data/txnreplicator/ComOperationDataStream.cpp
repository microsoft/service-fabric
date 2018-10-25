// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace TxnReplicator;
using namespace Data::Utilities;

ComOperationDataStream::ComOperationDataStream(
    __in PartitionedReplicaId const & traceId,
    __in IOperationDataStream & innerOperationDataStream)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , innerOperationDataStream_(&innerOperationDataStream)
{
    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ComOperationDataStream",
        reinterpret_cast<uintptr_t>(this));
}

ComOperationDataStream::~ComOperationDataStream()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComOperationDataStream",
        reinterpret_cast<uintptr_t>(this));

    innerOperationDataStream_->Dispose();
}

NTSTATUS ComOperationDataStream::Create(
    __in PartitionedReplicaId const & traceId,
    __in IOperationDataStream & innerOperationDataStream,
    __in KAllocator & allocator,
    __out Common::ComPointer<IFabricOperationDataStream> & object) noexcept
{
    ComOperationDataStream * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComOperationDataStream(traceId, innerOperationDataStream);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    object.SetAndAddRef(pointer);
    return STATUS_SUCCESS;
}

HRESULT ComOperationDataStream::BeginGetNext(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context) noexcept
{
    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    IOperationDataStream::AsyncGetNextContext::SPtr asyncOperation;

    HRESULT hr = innerOperationDataStream_->CreateAsyncGetNextContext(asyncOperation);
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncOperation),
        callback,
        context,
        [&](Ktl::Com::AsyncCallInAdapter&,
            IOperationDataStream::AsyncGetNextContext& operation, 
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
        {
            return operation.StartGetNext(
                nullptr,
                completionCallback);
        });
}

HRESULT ComOperationDataStream::EndGetNext(
    __in IFabricAsyncOperationContext * context,
    __out IFabricOperationData ** comOperationData) noexcept
{
    if (!context)
    {
        return E_POINTER;
    }

    IOperationDataStream::AsyncGetNextContext::SPtr asyncOperation;

    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    OperationData::CSPtr operationData;
    if (SUCCEEDED(hr))
    {
        hr = asyncOperation->GetResult(operationData);
    }

    if (SUCCEEDED(hr))
    {
        ComPointer<IFabricOperationData> comPointer;

        if (operationData.RawPtr() != nullptr)
        {
            NTSTATUS status = ComOperationData::Create(
                *operationData,
                GetThisAllocator(),
                comPointer);

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        *comOperationData = comPointer.DetachNoRelease();
    }

    return hr;
}

void ComOperationDataStream::Dispose()
{
    innerOperationDataStream_->Dispose();
}
