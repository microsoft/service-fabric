// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;

    internal enum TdhInType : ushort 
    {
        Null,
        UnicodeString,
        AnsiString,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float,
        Double,
        Boolean,
        Binary,
        Guid,
        Pointer,
        FileTime,
        SystemTime,
        SID,
        HexInt32,
        HexInt64,  // End of winmeta intypes
        CountedString = 300, // Start of TDH intypes for WBEM
        CountedAnsiString,
        ReversedCountedString,
        ReversedCountedAnsiString,
        NonNullTerminatedString,
        NonNullTerminatedAnsiString,
        UnicodeChar,
        AnsiChar,
        SizeT,
        HexDump,
        WbemSID
    }
}