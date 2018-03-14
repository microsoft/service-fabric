// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "BinaryWriter.h"

BinaryWriter::BinaryWriter(
    __in KBuffer& buffer
) : _buffer(buffer),
_writePosition(0)
{
}

BinaryWriter::~BinaryWriter()
{
}

bool BinaryWriter::WriteByte(
    __in char value
)
{
    return Write(value);
}

bool BinaryWriter::WriteUInt16(
    __in USHORT value
)
{
    return Write(htons(value));
}

bool BinaryWriter::WriteUInt32(
    __in ULONG value
)
{
    return Write(htonl(value));
}

bool BinaryWriter::WriteBytes(
    __in_bcount(bufferSize) PVOID pBuffer,
    __in ULONG bufferSize,
    __in ULONG writeLength
)
{
    KInvariant(writeLength <= bufferSize);
    if (GetWritePosition() + writeLength > Size())
    {
        return false;
    }

    memcpy_s(GetWriteBuffer(), FreeSpace(), pBuffer, writeLength);
    IncrementWritePositionBy(writeLength);

    return true;
}

void BinaryWriter::IncrementWritePositionBy(
    __in ULONG value
)
{
    _writePosition += value;
    KInvariant(_writePosition < Size());
}
