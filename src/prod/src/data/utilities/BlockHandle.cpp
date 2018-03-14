// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define BLOCKHANDLE_TAG 'hbFF'

using namespace Data::Utilities;

NTSTATUS BlockHandle::Create(
    __in ULONG64 Offset,
    __in ULONG64 Size,
    __in KAllocator & allocator,
    __out BlockHandle::SPtr& result) noexcept
{
    result = _new(BLOCKHANDLE_TAG, allocator) BlockHandle(Offset, Size);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

BlockHandle::BlockHandle(
    __in ULONG64 Offset,
    __in ULONG64 Size)
    : Offset_(Offset)
    , Size_(Size)
{
}

BlockHandle::~BlockHandle()
{
}

ULONG32 BlockHandle::SerializedSize()
{
    return sizeof(ULONG64) + sizeof(ULONG64);
}

ULONG64 BlockHandle::EndOffset() const
{
    return Offset_ + Size_;
}

BlockHandle::SPtr BlockHandle::Read(
    __in BinaryReader & binaryReader,
    __in KAllocator & allocator)
{
    ULONG64 offset;
    ULONG64 size;
    binaryReader.Read(offset);
    binaryReader.Read(size);

    BlockHandle::SPtr result;
    NTSTATUS status = BlockHandle::Create(offset, size, allocator, result);
    THROW_ON_FAILURE(status);

    return result;
}

void BlockHandle::Write(__in BinaryWriter & binaryWriter)
{
    binaryWriter.Write(Offset_);
    binaryWriter.Write(Size_);
}
