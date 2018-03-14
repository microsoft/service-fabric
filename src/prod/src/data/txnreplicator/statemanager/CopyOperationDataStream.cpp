// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;
using namespace ktl;

NTSTATUS CopyOperationDataStream::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceType, 
    __in KArray<SerializableMetadata::CSPtr> const & state, 
    __in FABRIC_REPLICA_ID targetReplicaId,
    __in SerializationMode::Enum serailizationMode,
    __in KAllocator & allocator, 
    __out SPtr & result)
{
    result = _new(NAMED_OPERATION_DATA_STREAM_TAG, allocator) CopyOperationDataStream(traceType, state, targetReplicaId, serailizationMode);

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

ktl::Awaitable<NTSTATUS> CopyOperationDataStream::GetNextAsync(
    __in CancellationToken const& cancellationToken,
    __out Data::Utilities::OperationData::CSPtr & result) noexcept
{
    KShared$ApiEntry();
   
    result = nullptr;
    if (checkpointOperationDataEnumerator_->MoveNext())
    {
        result = checkpointOperationDataEnumerator_->Current();
        currentIndex_++;
    }

    co_return STATUS_SUCCESS;
}

void CopyOperationDataStream::Dispose()
{
}

CopyOperationDataStream::CopyOperationDataStream(
    __in Data::Utilities::PartitionedReplicaId const & traceType,
    __in KArray<SerializableMetadata::CSPtr> const & state,
    __in FABRIC_REPLICA_ID targetReplicaId,
    __in SerializationMode::Enum serailizationMode)
    : OperationDataStream()
    , PartitionedReplicaTraceComponent(traceType)
    , targetReplicaId_(targetReplicaId)
    , checkpointOperationDataEnumerator_(CheckpointFile::GetCopyData(traceType, state, targetReplicaId, serailizationMode, GetThisAllocator()))
{
}

CopyOperationDataStream::~CopyOperationDataStream()
{
}
