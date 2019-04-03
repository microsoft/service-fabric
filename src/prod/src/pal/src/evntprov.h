// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EVNTAPI
#ifndef MIDL_PASS
#ifdef _EVNT_SOURCE_
#define EVNTAPI __stdcall
#else
#define EVNTAPI DECLSPEC_IMPORT __stdcall
#endif // _EVNT_SOURCE_
#endif // MIDL_PASS
#endif // EVNTAPI

#define MAX_LTTNG_EVENT_DATA_SIZE 65536
#define MAX_LTTNG_UNSTRUCTURED_EVENT_LEN (MAX_LTTNG_EVENT_DATA_SIZE / sizeof(wchar_t) - 1)

typedef ULONGLONG REGHANDLE, *PREGHANDLE;

typedef struct _EVENT_DATA_DESCRIPTOR {
    ULONGLONG   Ptr;        // Pointer to data
    ULONG       Size;       // Size of data in bytes
    ULONG       Reserved;
} EVENT_DATA_DESCRIPTOR, *PEVENT_DATA_DESCRIPTOR;

typedef struct _EVENT_DESCRIPTOR {
    USHORT      Id;
    UCHAR       Version;
    UCHAR       Channel;
    UCHAR       Level;
    UCHAR       Opcode;
    USHORT      Task;
    ULONGLONG   Keyword;
} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;

typedef const EVENT_DESCRIPTOR *PCEVENT_DESCRIPTOR;

typedef struct _EVENT_FILTER_DESCRIPTOR {
    ULONGLONG   Ptr;
    ULONG       Size;
    ULONG       Type;
} EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;

typedef struct _EVENT_HEADER {
    USHORT           Size;
    USHORT           HeaderType;
    USHORT           Flags;
    USHORT           EventProperty;
    ULONG            ThreadId;
    ULONG            ProcessId;
    LARGE_INTEGER    TimeStamp;
    GUID             ProviderId;
    EVENT_DESCRIPTOR EventDescriptor;
    union {
        struct {
            ULONG KernelTime;
            ULONG UserTime;
        };
        ULONG64 ProcessorTime;
    };
    GUID             ActivityId;
} EVENT_HEADER, *PEVENT_HEADER;

typedef struct _EVENT_HEADER_EXTENDED_DATA_ITEM {
    USHORT    Reserved1;
    USHORT    ExtType;
    struct {
        USHORT Linkage  :1;
        USHORT Reserved2  :15;
    };
    USHORT    DataSize;
    ULONGLONG DataPtr;
} EVENT_HEADER_EXTENDED_DATA_ITEM, *PEVENT_HEADER_EXTENDED_DATA_ITEM;

typedef struct _ETW_BUFFER_CONTEXT {
    UCHAR  ProcessorNumber;
    UCHAR  Alignment;
    USHORT LoggerId;
} ETW_BUFFER_CONTEXT, *PETW_BUFFER_CONTEXT;

typedef struct _EVENT_RECORD {
    EVENT_HEADER                     EventHeader;
    ETW_BUFFER_CONTEXT               BufferContext;
    USHORT                           ExtendedDataCount;
    USHORT                           UserDataLength;
    PEVENT_HEADER_EXTENDED_DATA_ITEM ExtendedData;
    PVOID                            UserData;
    PVOID                            UserContext;
} EVENT_RECORD, *PEVENT_RECORD;

typedef
VOID
(NTAPI *PENABLECALLBACK) (
    __in LPCGUID SourceId,
    __in ULONG IsEnabled,
    __in UCHAR Level,
    __in ULONGLONG MatchAnyKeyword,
    __in ULONGLONG MatchAllKeyword,
    __in_opt PEVENT_FILTER_DESCRIPTOR FilterData,
    __in_opt PVOID CallbackContext
    );

// 115
ULONG
EVNTAPI
EventRegister(
    __in LPCGUID ProviderId,
    __in_opt PENABLECALLBACK EnableCallback,
    __in_opt PVOID CallbackContext,
    __out PREGHANDLE RegHandle
    );

ULONG
EVNTAPI
EventUnregister(
    __in REGHANDLE RegHandle
    );

BOOLEAN
EVNTAPI
EventEnabled(
    __in REGHANDLE RegHandle,
    __in PCEVENT_DESCRIPTOR EventDescriptor
    );

ULONG
EVNTAPI
EventWrite(
    __in REGHANDLE RegHandle,
    __in PCEVENT_DESCRIPTOR EventDescriptor,
    __in ULONG UserDataCount,
    __in_ecount_opt(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

FORCEINLINE
VOID
EventDataDescCreate(
    __out PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
    __in  const VOID* DataPtr,
    __in  ULONG DataSize
    )
{
    EventDataDescriptor->Ptr = (ULONGLONG) DataPtr;
    EventDataDescriptor->Size = DataSize;
    EventDataDescriptor->Reserved = 0;
}

// 228
FORCEINLINE
VOID
EventDescCreate(
    __out PEVENT_DESCRIPTOR         EventDescriptor,
    __in  USHORT                    Id,
    __in  UCHAR                     Version,
    __in  UCHAR                     Channel,
    __in  UCHAR                     Level,
    __in  USHORT                    Task,
    __in  UCHAR                     Opcode,
    __in  ULONGLONG                 Keyword
    )
{
    EventDescriptor->Id         = Id;
    EventDescriptor->Version    = Version;
    EventDescriptor->Channel    = Channel;
    EventDescriptor->Level      = Level;
    EventDescriptor->Task       = Task;
    EventDescriptor->Opcode     = Opcode;
    EventDescriptor->Keyword    = Keyword;
}

#ifdef __cplusplus
}
#endif
