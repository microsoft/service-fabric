// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

BinaryWriter::BinaryWriter(__in KAllocator & allocator)
    : KObject()
    , memChannel_(allocator)
    , allocator_(allocator)
{
    memChannel_.SetReadFullRequired(TRUE); // Ensures partial reads fails the call
}

KBuffer::SPtr BinaryWriter::GetBuffer(__in ULONG startPosition)
{
    return GetBuffer(
        startPosition,
        Position - startPosition);
}

KBuffer::SPtr BinaryWriter::GetBuffer(
    __in ULONG startPosition,
    __in ULONG numberOfBytesToRead)
{
    ASSERT_IFNOT(
        startPosition < Position, 
        "Start position: {0} is less than current position: {1}", 
        startPosition, Position);
    ASSERT_IFNOT(
        numberOfBytesToRead <= (Position - startPosition), 
        "Invalid number of bytes to read, current position: {0}, start position: {1}, bytes to read: {2}", 
        Position, startPosition, numberOfBytesToRead);

    KBuffer::SPtr outBuffer = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    status = KBuffer::Create(
        numberOfBytesToRead,
        outBuffer,
        allocator_,
        BINARYWRITER_TAG);

    THROW_ON_FAILURE(status);

    ULONG actuallyRead;
    
    ULONG currentPosition = Position;
    Position = startPosition;

    status = memChannel_.Read(
        numberOfBytesToRead,
        outBuffer->GetBuffer(),
        &actuallyRead);

    Position = currentPosition;

    THROW_ON_FAILURE(status);

    ASSERT_IFNOT(static_cast<int>(actuallyRead) == numberOfBytesToRead, "BinaryWriter: Could not copy buffer in GetBuffer. ActuallyRead: {0} and BytesToRead: {1}", actuallyRead, numberOfBytesToRead);

    return outBuffer;
}

void BinaryWriter::Reset()
{
    memChannel_.Reset();
}

void BinaryWriter::Write(__in_opt KBuffer const * const value)
{
    if (value == nullptr)
    {
        Write(NULL_KBUFFER_SERIALIZATION_CODE);
        return;
    }

    ULONG length = value->QuerySize();
    Write((ULONG32)length);

    NTSTATUS status = memChannel_.Write(length, value->GetBuffer());
    THROW_ON_FAILURE(status);
}
