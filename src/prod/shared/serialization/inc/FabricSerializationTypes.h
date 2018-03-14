// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    namespace FabricSerializationTypes
    {
        // MSB is reserved for determining if it is an array or not.  Array types typically have a compressed ULONG following it with no metadata
        enum Enum : unsigned char
        {
            EmptyValueBit   = 0x40,   // 0b0100 0000 - This bit set means the value is empty
            Array           = 0x80,   // 0b1000 0000 - This bit set indicates an array
            BaseTypeMask    = 0x0F,   // 0b0000 1111
            BoolFalseFlag   = 0x30,   // 0b0011 0000

            Object  = 0x00,
            Pointer = 0x01,

            Bool        = 0x02,
            BoolTrue    = Bool,
            BoolFalse   = Bool | BoolFalseFlag,

            Char    = 0x03,
            UChar   = 0x04,

            Short   = 0x05,
            UShort  = 0x06,
            Int32   = 0x07,
            UInt32  = 0x08,
            Int64   = 0x09,
            UInt64  = 0x0A,

            Double  = 0x0B,
            Guid    = 0x0C,

            WString         = 0x0D,

            ByteArrayNoCopy = 0x0E | Array,

            ScopeBegin      = 0x1F,
            ScopeEnd        = 0x2F,
            ObjectEnd       = 0x3F,
        };

        bool IsArray(FabricSerializationTypes::Enum value);

        bool IsEmpty(FabricSerializationTypes::Enum value);

        bool IsOfBaseType(FabricSerializationTypes::Enum value, FabricSerializationTypes::Enum baseValue);

        FabricSerializationTypes::Enum MakeArray(FabricSerializationTypes::Enum value);

        FabricSerializationTypes::Enum MakeEmpty(FabricSerializationTypes::Enum value);
    }
}
