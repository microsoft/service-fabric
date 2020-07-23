// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "evntprov.h"
#include "retail/native/FabricCommon/TraceWrapper.Linux.h"

ULONG
EVNTAPI
EventRegister(
    __in LPCGUID ProviderId,
    __in_opt PENABLECALLBACK EnableCallback,
    __in_opt PVOID CallbackContext,
    __out PREGHANDLE RegHandle
    )
{
    return 0;
}

ULONG
EVNTAPI
EventUnregister(
    __in REGHANDLE RegHandle
    )
{
    return 0;
}

BOOLEAN
EVNTAPI
EventEnabled(
    __in REGHANDLE RegHandle,
    __in PCEVENT_DESCRIPTOR EventDescriptor
    )
{
    return TRUE;
}

ULONG
EVNTAPI
EventWrite(
    __in REGHANDLE RegHandle,
    __in PCEVENT_DESCRIPTOR EventDescriptor,
    __in ULONG UserDataCount,
    __in_ecount_opt(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    )
{
    // Hardcoded ServiceFabric providerId.
    // This is done for simplification since the ProviderId is
    // already hardcoded in TraceProvider
    // "cbd93bc2-71e5-4566-b3a7-595d8eeca6e8"
    static_assert (sizeof(GUID) == 16, "GUID size is not 16");

    static GUID serviceFabricProviderId = {
        0xcbd93bc2,                                         // DWORD Data1
        0x71e5,                                             // WORD  Data2
        0x4566,                                             // WORD  Data3
        { 0xb3, 0xa7, 0x59, 0x5d, 0x8e, 0xec, 0xa6, 0xe8 }  // BYTE  Data4[8]
    };

    int i;
    size_t totalBytes = 0;

    // getting total size for all event_data_descriptors
    for (i = 0; i < UserDataCount; i++)
    {
        totalBytes += UserData[i].Size;
    }

    // event size limit should not be bigger than 64KB
    // ref: https://msdn.microsoft.com/en-us/library/windows/desktop/aa363752(v=vs.85).aspx
    if(totalBytes > MAX_LTTNG_EVENT_DATA_SIZE)
    {
        return ERROR_ARITHMETIC_OVERFLOW;
    }

    // writing all data to single contiguous buffer
    unsigned char data[(totalBytes > 0) ? totalBytes : 1];
    unsigned char *dataPtr = data;
    for (i = 0; i < UserDataCount; i++)
    {
        memcpy(dataPtr, (unsigned char*) (UserData[i].Ptr), UserData[i].Size);
        dataPtr += UserData[i].Size;
    }

    TraceWrapperBinaryStructured(
        (unsigned char*) &serviceFabricProviderId,
        (unsigned short) EventDescriptor->Id,
        (unsigned char) EventDescriptor->Version,
        (unsigned char) EventDescriptor->Channel,
        (unsigned char) EventDescriptor->Level,
        (unsigned char) EventDescriptor->Opcode,
        (unsigned short) EventDescriptor->Task,
        (unsigned long long) EventDescriptor->Keyword,
        (unsigned char*) data,
        (unsigned short) totalBytes);

    return 0;
}
