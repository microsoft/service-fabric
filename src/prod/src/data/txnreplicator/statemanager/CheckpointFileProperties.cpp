// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;
using namespace Data::Utilities;

NTSTATUS CheckpointFileProperties::Create(
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(CHECKPOINTFILEPROPERTIES_TAG, allocator) CheckpointFileProperties(
        nullptr,                            // BlocksHandle
        nullptr,                            // MetadataHandle
        KGuid(TRUE),                        // PartitionId
        0,                                  // ReplicaId
        0,                                  // RootStateProviderCount
        0,                                  // StateProviderCount
        false,                              // DoNotWritePrepareCheckpointLSN
        FABRIC_INVALID_SEQUENCE_NUMBER);    // PrepareCheckpointLSN
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

NTSTATUS CheckpointFileProperties::Create(
    __in BlockHandle * const blocksHandle,
    __in BlockHandle * const metadataHandle,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in ULONG32 rootStateProviderCount,
    __in ULONG32 stateProviderCount,
    __in bool doNotWritePrepareCheckpointLSN,
    __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(CHECKPOINTFILEPROPERTIES_TAG, allocator) CheckpointFileProperties(
        blocksHandle,
        metadataHandle,
        partitionId,
        replicaId,
        rootStateProviderCount,
        stateProviderCount,
        doNotWritePrepareCheckpointLSN,
        prepareCheckpointLSN);
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

void CheckpointFileProperties::Write(__in BinaryWriter & writer)
{
    // Allow the base class to write first.    
    FileProperties::Write(writer);
    
    // 'metadata' - BlockHandle
    writer.Write(MetadataHandlePropertyName_);
    VarInt::Write(writer, BlockHandle::SerializedSize());
    MetadataHandle->Write(writer);

    // 'blocks' - BlockHandle
    writer.Write(BlocksHandlePropertyName_);
    VarInt::Write(writer, BlockHandle::SerializedSize());
    BlocksHandle->Write(writer);

    // 'count' - ULONG32
    writer.Write(StateProviderCountPropertyName_);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
    writer.Write(StateProviderCount);

    // 'roots' - ULONG32
    writer.Write(RootStateProviderCountPropertyName_);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
    writer.Write(RootStateProviderCount);

    // 'partitionid' - KGuid
    writer.Write(PartitionIdPropertyName_);
    // Guid 16 bytes
    ULONG32 length = 16;
    VarInt::Write(writer, length);
    writer.Write(PartitionId);

    // 'replicaid' - ULONG64
    writer.Write(ReplicaIdPropertyName_);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG64)));
    writer.Write(ReplicaId);

    if (doNotWritePrepareCheckpointLSN_ == false && test_Ignore_ == false)
    {
        // 'checkpointlsn' - FABRIC_SEQUENCE_NUMBER
        writer.Write(PrepareCheckpointLSNPropertyName_);
        VarInt::Write(writer, static_cast<ULONG32>(sizeof(FABRIC_SEQUENCE_NUMBER)));
        writer.Write(PrepareCheckpointLSN);
    }
}

void CheckpointFileProperties::ReadProperty(
    __in BinaryReader & reader, 
    __in KStringView const & property, 
    __in ULONG32 valueSize)
{
    // Properties are key-value pairs, so read in corresponding properties
    if (property.Compare(MetadataHandlePropertyName_) == 0)
    {
        metadataHandle_.Put(BlockHandle::Read(reader, GetThisAllocator()));
    }
    else if (property.Compare(BlocksHandlePropertyName_) == 0)
    {
        blocksHandle_.Put(BlockHandle::Read(reader, GetThisAllocator()));
    }
    else if (property.Compare(StateProviderCountPropertyName_) == 0)
    {
        reader.Read(stateProviderCount_);
    }
    else if (property.Compare(RootStateProviderCountPropertyName_) == 0)
    {
        reader.Read(rootStateProviderCount_);
    }
    else if (property.Compare(PartitionIdPropertyName_) == 0)
    {
        reader.Read(partitionId_);
    }
    else if (property.Compare(ReplicaIdPropertyName_) == 0)
    {
        reader.Read(replicaId_);
    }
    else if (property.Compare(PrepareCheckpointLSNPropertyName_) == 0)
    {
        if (test_Ignore_)
        {
            // If test ignore set to true, we need to jump over the value block.
            FileProperties::ReadProperty(reader, property, valueSize);
            return;
        }

        reader.Read(prepareCheckpointLSN_);
        ASSERT_IFNOT(
            prepareCheckpointLSN_ >= FABRIC_AUTO_SEQUENCE_NUMBER,
            "CheckpointFileProperties::ReadProperty If the file has PrepareCheckpointLSN property, it must larger than or equal to 0, PrepareCheckpointLSN: {0}.",
            prepareCheckpointLSN_);
    }
    else
    {
        // If the properties is unknown, just skip it.
        FileProperties::ReadProperty(reader, property, valueSize);
    }
}

BlockHandle::SPtr CheckpointFileProperties::get_BlocksHandle() const { return blocksHandle_.Get(); }

BlockHandle::SPtr CheckpointFileProperties::get_MetadataHandle() const { return metadataHandle_.Get(); }

KGuid CheckpointFileProperties::get_PartitionId() const { return partitionId_; }

FABRIC_REPLICA_ID CheckpointFileProperties::get_ReplicaId() const { return replicaId_; }

ULONG32 CheckpointFileProperties::get_RootStateProviderCount() const { return rootStateProviderCount_; }

ULONG32 CheckpointFileProperties::get_StateProviderCount() const { return stateProviderCount_; }

FABRIC_SEQUENCE_NUMBER CheckpointFileProperties::get_PrepareCheckpointLSN() const { return prepareCheckpointLSN_; }

void CheckpointFileProperties::put_Test_Ignore(__in bool test_Ignore) { test_Ignore_ = test_Ignore; };

CheckpointFileProperties::CheckpointFileProperties(
    __in BlockHandle * const blocksHandle,
    __in BlockHandle * const metadataHandle,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in ULONG32 rootStateProviderCount,
    __in ULONG32 stateProviderCount,
    __in bool doNotWritePrepareCheckpointLSN,
    __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN)
    : FileProperties()
    , blocksHandle_(blocksHandle)
    , metadataHandle_(metadataHandle)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , rootStateProviderCount_(rootStateProviderCount)
    , stateProviderCount_(stateProviderCount)
    , prepareCheckpointLSN_(prepareCheckpointLSN)
    , test_Ignore_(false)
    , doNotWritePrepareCheckpointLSN_(doNotWritePrepareCheckpointLSN)
{
}

CheckpointFileProperties::~CheckpointFileProperties()
{
}
