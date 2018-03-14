// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

NTSTATUS TestStateProvider::UndoOperationData::Create(
    __in PartitionedReplicaId const & traceId,
    __in KString const & value,
    __in FABRIC_SEQUENCE_NUMBER lsn,
    __in KAllocator & allocator,
    __out CSPtr & result)
{
    result = _new(TEST_STATE_PROVIDER_TAG, allocator) TestStateProvider::UndoOperationData(
        traceId,
        value,
        lsn,
        true);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (TestStateProvider::UndoOperationData::CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS TestStateProvider::UndoOperationData::Create(
    __in PartitionedReplicaId const & traceId,
    __in OperationData const * const operationData,
    __in KAllocator & allocator,
    __out TestStateProvider::UndoOperationData::CSPtr & result)
{
    KString::SPtr value;
    FABRIC_SEQUENCE_NUMBER version;

    NTSTATUS status = Deserialize(traceId, operationData, allocator, value, version);
    RETURN_ON_FAILURE(status);

    result = _new(TEST_STATE_PROVIDER_TAG, allocator) TestStateProvider::UndoOperationData(
        traceId,
        *value,
        version,
        false);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (TestStateProvider::UndoOperationData::CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

KString::CSPtr TestStateProvider::UndoOperationData::get_Value() const
{
    return value_;
}

FABRIC_SEQUENCE_NUMBER TestStateProvider::UndoOperationData::get_Version() const
{
    return version_;
}

NTSTATUS TestStateProvider::UndoOperationData::Deserialize(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in OperationData const * const operationData,
    __in KAllocator & allocator,
    __out KString::SPtr & value,
    __out FABRIC_SEQUENCE_NUMBER & version) noexcept 
{
    ASSERT_IFNOT(
        operationData != nullptr,
        "{0}: Null undo operation data during deserialization.",
        traceId.TraceId);
    ASSERT_IFNOT(
        operationData->BufferCount == 2,
        "{0}: Uno operation data should have atleast one buffer during deserialization. BufferCount: {1}.",
        traceId.TraceId,
        operationData->BufferCount);

    {
        KBuffer::CSPtr valueBufferSPtr = (*operationData)[0];
        ASSERT_IFNOT(
            valueBufferSPtr != nullptr,
            "{0}: Null undo operation value buffer.",
            traceId.TraceId);

        Utilities::BinaryReader binaryReader(*valueBufferSPtr, allocator);
        RETURN_ON_FAILURE(binaryReader.Status());

        binaryReader.Read(value, UTF16);
    }

    {
        KBuffer::CSPtr versionBufferSPtr = (*operationData)[1];
        ASSERT_IFNOT(
            versionBufferSPtr != nullptr,
            "{0}: Null redo operation version buffer.",
            traceId.TraceId);

        Utilities::BinaryReader binaryReader(*versionBufferSPtr, allocator);
        RETURN_ON_FAILURE(binaryReader.Status());

        binaryReader.Read(version);
    }

    return STATUS_SUCCESS;
}

NTSTATUS TestStateProvider::UndoOperationData::Serialize() noexcept
{
    // Serialize Value
    {
        Utilities::BinaryWriter binaryWriter(GetThisAllocator());
        RETURN_ON_FAILURE(binaryWriter.Status());

        binaryWriter.Write(*value_, UTF16);
        Append(*binaryWriter.GetBuffer(0));
    }

    // Serialize Version
    {
        Utilities::BinaryWriter binaryWriter(GetThisAllocator());
        RETURN_ON_FAILURE(binaryWriter.Status());

        binaryWriter.Write(version_);
        Append(*binaryWriter.GetBuffer(0));
    }

    return STATUS_SUCCESS;
}

TestStateProvider::UndoOperationData::UndoOperationData(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KString const & value,
    __in FABRIC_SEQUENCE_NUMBER lsn,
    __in bool serializeData) noexcept
    : OperationData()
    , PartitionedReplicaTraceComponent(traceId)
    , value_(&value)
    , version_(lsn)
{
    if (serializeData)
    {
        NTSTATUS status = Serialize();
        SetConstructorStatus(status);

        ASSERT_IFNOT(BufferCount == 2, "Buffer count must be 2");
    }
}

TestStateProvider::UndoOperationData::~UndoOperationData() noexcept
{

}
