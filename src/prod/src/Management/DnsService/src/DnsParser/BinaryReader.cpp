// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "BinaryReader.h"

BinaryReader::BinaryReader(
    __in KBuffer& buffer,
    __in ULONG size
) : _buffer(buffer),
_size(size),
_readPosition(0)
{
}

BinaryReader::BinaryReader(
    __in KBuffer& buffer,
    __in ULONG size,
    __in ULONG position
) : _buffer(buffer),
_size(size),
_readPosition(position)
{
}

BinaryReader::~BinaryReader()
{
}

bool BinaryReader::PeekByte(
    __out char& value
) const
{
    return Peek(/*out*/value);
}

bool BinaryReader::ReadByte(
    __out char& value
)
{
    return Read(/*out*/value);
}

bool BinaryReader::ReadUInt16(
    __out USHORT& value
)
{
    USHORT nvalue;
    if (Read(/*out*/nvalue))
    {
        value = ntohs(nvalue);
        return true;
    }

    return false;
}

bool BinaryReader::ReadUInt32(
    __out ULONG& value
)
{
    ULONG nvalue;
    if (Read(/*out*/nvalue))
    {
        value = ntohl(nvalue);
        return true;
    }

    return false;
}

bool BinaryReader::ReadBytes(
    __out KBuffer& outBuffer,
    __in ULONG readLength
)
{
    KInvariant(readLength <= outBuffer.QuerySize());

    if (GetReadPosition() + readLength > Size())
    {
        return false;
    }

    memcpy_s(outBuffer.GetBuffer(), outBuffer.QuerySize(), static_cast<const void*>(GetReadBuffer()), readLength);
    IncrementReadPositionBy(readLength);

    return true;
}

bool BinaryReader::Skip(
    __in ULONG length
)
{
    if (GetReadPosition() + length > Size())
    {
        return false;
    }

    IncrementReadPositionBy(length);

    return true;
}

void BinaryReader::SetReadPosition(
    __in ULONG value
)
{
    _readPosition = value;
    KInvariant(_readPosition <= Size());
}

void BinaryReader::IncrementReadPositionBy(
    __in ULONG value
)
{
    _readPosition += value;
    KInvariant(_readPosition <= Size());
}
