// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;
using namespace Data::StateManager;

NTSTATUS SerializableMetadataEnumerator::Create(
    __in PartitionedReplicaId const & traceId,
    __in KBuffer const & readBuffer,
    __in KAllocator& allocator,
    __out SPtr & result)
{
    result = _new(SERIALIZABLE_METADATA_ENUMERATOR, allocator) SerializableMetadataEnumerator(traceId, readBuffer);

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

SerializableMetadata::CSPtr SerializableMetadataEnumerator::Current()
{
    SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
    NTSTATUS status = CheckpointFileAsyncEnumerator::ReadMetadata(
        *PartitionedReplicaIdentifier, 
        itemReader_, 
        GetThisAllocator(),
        serializableMetadataCSPtr);
    THROW_ON_FAILURE(status);

    return serializableMetadataCSPtr;
}

bool SerializableMetadataEnumerator::MoveNext()
{
    return itemReader_.Position < endPos_ ? true : false;
}

SerializableMetadataEnumerator::SerializableMetadataEnumerator(
    __in PartitionedReplicaId const & traceId,
    __in KBuffer const & readBuffer)
    : PartitionedReplicaTraceComponent(traceId)
    , itemReader_(readBuffer, GetThisAllocator())
    , endPos_(readBuffer.QuerySize() - 1)
{
    if (NT_SUCCESS(itemReader_.Status()) == false)
    {
        SetConstructorStatus(itemReader_.Status());
        return;
    }
}

SerializableMetadataEnumerator::~SerializableMetadataEnumerator()
{
}
