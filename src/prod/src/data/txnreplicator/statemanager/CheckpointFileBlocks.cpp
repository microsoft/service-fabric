// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;
using namespace Data::Utilities;

NTSTATUS CheckpointFileBlocks::Create(
    __in KSharedArray<ULONG32> & recordBlockSizes,
    __in KAllocator & allocator,
    __out SPtr& result) noexcept
{
    result = _new(CHECKPOINTFILEBLOCKS_TAG, allocator) CheckpointFileBlocks(recordBlockSizes);
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

CheckpointFileBlocks::SPtr CheckpointFileBlocks::Read(
    __in BinaryReader & reader, 
    __in BlockHandle const & handle, 
    __in KAllocator & allocator)
{
    if (handle.Size == 0)
    {
        return nullptr;
    }

    reader.Position = static_cast<ULONG>(handle.Offset);

    // Read the record block sizes.
    KSharedArray<ULONG32>::SPtr recordBlockSizes = ReadArray(reader, allocator);

    ASSERT_IFNOT(
        reader.Position == static_cast<ULONG>(handle.EndOffset()), 
        "CheckpointFileBlocks: Reader position should be the same as EndOffset specified in the BlockHandle, Position: {0}, EndOffset: {1}.",
        reader.Position,
        handle.EndOffset());

    SPtr blocks;
    NTSTATUS status = Create(*recordBlockSizes, allocator, blocks);
    THROW_ON_FAILURE(status)

    return blocks;
}

void CheckpointFileBlocks::Write(__in BinaryWriter & writer)
{
    // Write the record block sizes.
    WriteArray(writer, *RecordBlockSizes);
}

KSharedArray<ULONG32>::SPtr CheckpointFileBlocks::ReadArray(
    __in BinaryReader & reader, 
    __in KAllocator & allocator)
{
    LONG32 length;
    reader.Read(length);
    KSharedArray<ULONG32>::SPtr array = _new(CHECKPOINTFILEBLOCKS_TAG, allocator) KSharedArray<ULONG32>();

    // If the array is empty, just skip reading the record block size.
    for (LONG32 i = 0; i < length; i++)
    {
        ULONG32 readValue;
        reader.Read(readValue);
        array->Append(readValue);
    }

    return array;
}

void CheckpointFileBlocks::WriteArray(
    __in BinaryWriter & writer, 
    __in KArray<ULONG32> const & blockSizesArray)
{
    // RecordBlockSizes can not be null here, because we always create a array to track the metadata size.
    // It first writes the array count, then each record block size.
    ULONG length = blockSizesArray.Count();

    writer.Write(static_cast<LONG32>(length));
    for (ULONG i = 0; i < length; i++)
    {
        writer.Write(blockSizesArray[i]);
    }
}

KSharedArray<ULONG32>::SPtr CheckpointFileBlocks::get_RecordBlockSizes() const { return recordBlockSizes_; }

CheckpointFileBlocks::CheckpointFileBlocks(__in KSharedArray<ULONG32> & recordBlockSizes) noexcept
    : KObject()
    , KShared()
    , recordBlockSizes_(&recordBlockSizes)
{
}

CheckpointFileBlocks::~CheckpointFileBlocks()
{
}
