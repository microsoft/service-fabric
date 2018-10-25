/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kchecksum.h

Abstract:

    This file defines basic checksumming routines.

Author:

    Norbert P. Kusters (norbertk) 21-Nov-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KChecksum {

    public:

        static const ULONGLONG LargePrime1 = 4002904963;
        static const ULONGLONG LargePrime2 = 4002900007;
        static LONG DisableHwCrcAssist;

        static
        ULONG
        Crc32(
            __in_bcount(Size) const VOID* Source,
            __in ULONG Size,
            __in ULONG InitialCrc = 0
            );

        static
        ULONGLONG
        Crc64(
            __in_bcount(Size) const VOID* Source,
            __in ULONG Size,
            __in ULONGLONG InitialCrc = 0
            );

        static
        ULONGLONG
        Hash64(
            __in_bcount(Size) const VOID* Source,
            __in ULONG Size,
            __in ULONGLONG InitialHash = LargePrime1*LargePrime2
            );

};
