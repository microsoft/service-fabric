// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "wdm.h"

NTSYSAPI
VOID
NTAPI
RtlCopyMemory (
    VOID UNALIGNED *Destination,
    CONST VOID UNALIGNED *Source,
    SIZE_T Length
    )
{
    memcpy(Destination, Source, Length);
    return;
}

NTSYSAPI
VOID
NTAPI
RtlMoveMemory (
    VOID UNALIGNED *Destination,
    CONST VOID UNALIGNED *Source,
    SIZE_T Length
    )
{
    memmove(Destination, Source, Length);
    return;
}

NTSYSAPI
VOID
NTAPI
RtlFillMemory (
    VOID UNALIGNED *Destination,
    SIZE_T Length,
    IN UCHAR Fill
    )
{
    memset(Destination, Fill, Length);
    return;
}

NTSYSAPI
VOID
NTAPI
RtlZeroMemory (
    VOID UNALIGNED *Destination,
    SIZE_T Length
    )
{
    memset(Destination, 0, Length);
    return;
}
