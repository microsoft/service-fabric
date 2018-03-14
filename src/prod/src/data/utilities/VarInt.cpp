// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

LONG32 VarInt::GetSerializedSize(__in LONG32 value)
{
    return GetSerializedSize(static_cast<ULONG32>(value));
}

LONG32 VarInt::GetSerializedSize(__in ULONG32 value)
{
    if (value < MinTwoByteValue_)
    {
        return 1;
    }
    else if (value < MinThreeByteValue_)
    {
        return 2;
    }
    else if (value < MinFourByteValue_)
    {
        return 3;
    }
    else if (value < MinFiveByteValue_)
    {
        return 4;
    }

    return 5;
}

LONG32 VarInt::ReadInt32(__in BinaryReader & reader)
{
    ULONG32 result = ReadUInt32(reader);
    return static_cast<LONG32>(result);
}

ULONG32 VarInt::ReadUInt32(__in BinaryReader & reader)
{
    ULONG32 value = 0;

    for (LONG32 c = 0; c < Chunks; c++)
    {
        // Read one byte.
        byte b;
        reader.Read(b);

        // The bytes are serialized from the low-order bits to high-order bits in 7-bit chunks.
        LONG32 temp = (b & ByteMask_) << (7 * c);
        ULONG32 chunk;

        if (temp < 0) {
            chunk = UINT_MAX + temp + 1;
        }
        else {
            chunk = static_cast<ULONG32>(temp);
        }

        // Add in the new 7-bit chunk in the correct final place.
        value |= chunk;

        // Check if there are more 7-bit chunks.
        if ((b & MoreBytesMask_) == 0)
        {
            break;
        }
    }

    return value;
}

void VarInt::Write(__in BinaryWriter & writer, __in LONG32 value)
{
    Write(writer, static_cast<ULONG32>(value));
}

void VarInt::Write(__in BinaryWriter & writer, __in ULONG32 value)
{
    ULONG32 bytes = value;

    for (LONG32 c = 0; c < Chunks; c++)
    {
        // Serialize in 7-bit chunks from the low-order bits to the high-order bits.
        byte b = static_cast<byte>(bytes & ByteMask_);

        // Check if there are more byte chunks to serialize.
        if (bytes < MoreBytesMask_)
        {
            writer.Write(b);
            break;
        }
        else
        {
            // Write the 7-bit chunk with the MSB 1-bit set to indicate more bytes will be written.
            writer.Write(static_cast<byte>(MoreBytesMask_ | b));

            // Shift the value down by 7-bits to prepare for writing the next 7-bit chunk.
            bytes >>= 7;
        }
    }
}
