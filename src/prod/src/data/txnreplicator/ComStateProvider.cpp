// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

ComStateProvider::ComStateProvider(__in Data::Utilities::PartitionedReplicaId const & traceId)
    : KObject()
    , KShared()
    , innerStateProvider_()
    , PartitionedReplicaTraceComponent(traceId)
{
    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ComStateProvider",
        reinterpret_cast<uintptr_t>(this));
}

ComStateProvider::~ComStateProvider()
{ 
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComStateProvider",
        reinterpret_cast<uintptr_t>(this));
}

NTSTATUS ComStateProvider::Create(
    __in KAllocator & allocator,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out ComStateProvider::SPtr & object)
{
    ComStateProvider * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComStateProvider(prId);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    object = pointer;
    return STATUS_SUCCESS;
}

void ComStateProvider::Initialize(__in IStateProvider & innerStateProvider)
{
    innerStateProvider_ = &innerStateProvider;
}

HRESULT ComStateProvider::BeginUpdateEpoch(
    __in FABRIC_EPOCH const * epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (!epoch)
    {
        return E_POINTER;
    }

    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    IStateProvider::AsyncUpdateEpochContext::SPtr asyncOperation;

    HRESULT hr = innerStateProvider_->CreateAsyncUpdateEpochContext(asyncOperation);
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
            IStateProvider::AsyncUpdateEpochContext& operation, 
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
        {
            return operation.StartUpdateEpoch(
                *epoch,
                previousEpochLastSequenceNumber,
                nullptr,
                completionCallback);
        });
}

HRESULT ComStateProvider::EndUpdateEpoch(__in IFabricAsyncOperationContext * context)
{
    if (!context)
    {
        return E_POINTER;
    }

    IStateProvider::AsyncUpdateEpochContext::SPtr asyncOperation;

    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    return hr;
}

HRESULT ComStateProvider::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    IStateProvider::AsyncOnDataLossContext::SPtr asyncOperation;

    HRESULT hr = innerStateProvider_->CreateAsyncOnDataLossContext(asyncOperation);
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
            IStateProvider::AsyncOnDataLossContext& operation, 
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
        {
            return operation.StartOnDataLoss(
                nullptr,
                completionCallback);
        });
}

HRESULT ComStateProvider::EndOnDataLoss(
    __in IFabricAsyncOperationContext * context,
    __out BOOLEAN * isStateChanged)
{
    if (!context)
    {
        return E_POINTER;
    }

    if (!isStateChanged)
    {
        return E_POINTER;
    }

    IStateProvider::AsyncOnDataLossContext::SPtr asyncOperation;

    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    if (SUCCEEDED(hr))
    {
        hr = asyncOperation->GetResult(*isStateChanged);
    }

    return hr;
}

HRESULT ComStateProvider::GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER * sequenceNumber)
{
    if (!sequenceNumber)
    {
        return E_POINTER;
    }

    HRESULT error = innerStateProvider_->GetLastCommittedSequenceNumber(sequenceNumber);
    return error;
}

HRESULT ComStateProvider::GetCopyContext(__out IFabricOperationDataStream ** copyContextStream)
{
    if (!copyContextStream)
    {
        return E_POINTER;
    }

    ComPointer<IFabricOperationDataStream> comOperationDataStream;
    IOperationDataStream::SPtr copyContext;

    HRESULT hr = innerStateProvider_->GetCopyContext(copyContext);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    NTSTATUS status = ComOperationDataStream::Create(
        *PartitionedReplicaIdentifier,
        *copyContext,
        GetThisAllocator(),
        comOperationDataStream);

    if (NT_SUCCESS(status))
    {
        *copyContextStream = comOperationDataStream.DetachNoRelease();
    }

    return StatusConverter::ToHResult(status);
}

HRESULT ComStateProvider::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream * copyContextStream,
    __out IFabricOperationDataStream ** copyStateStream)
{
    if (!copyStateStream)
    {
        return E_POINTER;
    }

    ComPointer<IFabricOperationDataStream> comCopyState;
    ComPointer<IFabricOperationDataStream> comCopyContext;
    
    if (copyContextStream != nullptr)
    {
        comCopyContext.SetAndAddRef(copyContextStream);
    }

    IOperationDataStream::SPtr copyState;

    ComProxyOperationDataStream::SPtr copyContext;
    NTSTATUS status = ComProxyOperationDataStream::Create(
        *PartitionedReplicaIdentifier,
        comCopyContext,
        GetThisAllocator(),
        copyContext);

    if (!NT_SUCCESS(status))
    {
        return StatusConverter::ToHResult(status);
    }
    
    HRESULT hr = innerStateProvider_->GetCopyState(
        uptoSequenceNumber,
        *copyContext,
        copyState);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    status = ComOperationDataStream::Create(
        *PartitionedReplicaIdentifier,
        *copyState,
        GetThisAllocator(),
        comCopyState);

    if (NT_SUCCESS(status))
    {
        *copyStateStream = comCopyState.DetachNoRelease();
    }

    return StatusConverter::ToHResult(status);
}
