// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;
using namespace ktl;

NTSTATUS NamedOperationDataStream::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceType, 
    __in SerializationMode::Enum serializationMode,
    __in KAllocator& allocator, 
    __out SPtr & result) noexcept
{
    result = _new(NAMED_OPERATION_DATA_STREAM_TAG, allocator) NamedOperationDataStream(traceType, serializationMode);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

void NamedOperationDataStream::Add(
    __in StateProviderIdentifier stateProviderInfo,
    __in OperationDataStream & operationDataStream)
{
    NTSTATUS status = operationDataStreamCollection_.Append(StateProviderDataStream(stateProviderInfo, operationDataStream));
    if (NT_SUCCESS(status) == false)
    {
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"Add: Append StateProviderDataStream to OperationDataStreamCollection.",
            Helper::NamedOperationDataStream);
    }
}


ktl::Awaitable<NTSTATUS> NamedOperationDataStream::GetNextAsync(
    __in ktl::CancellationToken const & cancellationToken,
    __out Data::Utilities::OperationData::CSPtr & result) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    while (currentIndex_ < operationDataStreamCollection_.Count())
    {
        StateProviderDataStream namedOperationDataStream = operationDataStreamCollection_[currentIndex_];
        ASSERT_IFNOT(
            namedOperationDataStream.OperationDataStream != nullptr,
            "{0}: GetNextAsync: OperationDataStream is null pointer.",
            TraceId);

        Utilities::OperationData::CSPtr operationData;
        status = co_await namedOperationDataStream.OperationDataStream->GetNextAsync(cancellationToken, operationData);

        if (NT_SUCCESS(status) == false)
        {
            co_return status;
        }

        if (operationData == nullptr)
        {
            currentIndex_++;
            continue;
        }

        CopyNamedOperationData::CSPtr namedOperationData = nullptr;
        status = CopyNamedOperationData::Create(
            namedOperationDataStream.Info.StateProviderId,
            serializationMode_,
            GetThisAllocator(),
            *operationData,
            namedOperationData);

        if (NT_SUCCESS(status) == false)
        {
            Helper::TraceErrors(
                status,
                TracePartitionId,
                ReplicaId,
                Helper::NamedOperationDataStream,
                L"GetNextAsync: Create NamedOperationData.");

            co_return status;
        }
        
        result = namedOperationData.RawPtr();
        co_return status;
    }

    result = nullptr;
    co_return status;
}

void NamedOperationDataStream::Dispose()
{
    if (isDisposed_ == true || operationDataStreamCollection_.Count() == 0)
    {
        return;
    }

    for (StateProviderDataStream namedOperationDataStream : operationDataStreamCollection_)
    {
        IDisposable * disposableStreamPtr = dynamic_cast<IDisposable *>(namedOperationDataStream.OperationDataStream.RawPtr());
        if (disposableStreamPtr != nullptr)
        {
            disposableStreamPtr->Dispose();
        }
    }

    operationDataStreamCollection_.Clear();
    isDisposed_ = true;
}

NamedOperationDataStream::NamedOperationDataStream(
    __in Data::Utilities::PartitionedReplicaId const & traceType,
    __in SerializationMode::Enum serializationMode)
    : TxnReplicator::OperationDataStream()
    , PartitionedReplicaTraceComponent(traceType)
    , serializationMode_(serializationMode)
    , operationDataStreamCollection_(GetThisAllocator())
{
}

NamedOperationDataStream::~NamedOperationDataStream()
{
    Dispose();
}


