// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "winnt.h"

LONGLONG
__cdecl
InterlockedAdd64 (
    LONGLONG volatile *lpAddend,
    LONGLONG addent
    )
{
   return __sync_add_and_fetch(lpAddend, (LONGLONG)addent);
}

SHORT
InterlockedCompareExchange16 (
    __inout SHORT volatile *Destination,
    __in SHORT ExChange,
    __in SHORT Comperand
    )
{
    return __sync_val_compare_and_swap(Destination, Comperand, ExChange);
}

VOID
__movsb (
    IN PBYTE  Destination,
    IN BYTE  const *Source,
    IN SIZE_T Count
    )
{
    SIZE_T i;

    for (i = 0; i < Count; ++i) {
        Destination[i] = Source[i];
    }

    return;
}

#ifdef __aarch64__ 
VOID
__stosq (
    IN PDWORD64 Destination,
    IN DWORD64 Value,
    IN SIZE_T Count
    )
{
    for(size_t i = 0 ;i < Count;i++)
    {
        Destination[i] = Value;
    }
}
#else
VOID
__stosq (
    IN PDWORD64 Destination,
    IN DWORD64 Value,
    IN SIZE_T Count
    )
{
    __asm__("rep stosq" : : "D"(Destination), "a"(Value), "c"(Count));
}
#endif

PVOID
RtlSecureZeroMemory(
    __in_bcount(cnt) PVOID ptr,
    __in SIZE_T cnt
    )
{
    return memset(ptr, 0, cnt);
}
