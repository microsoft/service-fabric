// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class VarInt
        {
        public:

            static LONG32 GetSerializedSize(__in LONG32 value);

            static LONG32 GetSerializedSize(__in ULONG32 value);

            static ULONG32 ReadUInt32(__in BinaryReader & reader);

            static LONG32 ReadInt32(__in BinaryReader & reader);

            static void Write(__in BinaryWriter & writer, __in ULONG32 value);

            static void Write(__in BinaryWriter & writer, __in LONG32 value);

        private:

            static const byte ByteMask_ = 0x7F;

            static const ULONG32 MinFiveByteValue_ = 0x01 << 28;

            static const ULONG32 MinFourByteValue_ = 0x01 << 21;

            static const ULONG32 MinThreeByteValue_ = 0x01 << 14;

            static const ULONG32 MinTwoByteValue_ = 0x01 << 7;

            static const byte MoreBytesMask_ = 0x80;

            static const LONG32 Chunks = 5;
        };
    }
}
