// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PERF_COUNTERSET_FLAG_MULTIPLE             2  // 0010
#define PERF_COUNTERSET_FLAG_AGGREGATE            4  // 0100
#define PERF_COUNTERSET_FLAG_HISTORY              8  // 1000
#define PERF_COUNTERSET_FLAG_INSTANCE            16  // 00010000

#define PERF_COUNTERSET_SINGLE_INSTANCE          0
#define PERF_COUNTERSET_MULTI_INSTANCES          (PERF_COUNTERSET_FLAG_MULTIPLE)
#define PERF_COUNTERSET_SINGLE_AGGREGATE         (PERF_COUNTERSET_FLAG_AGGREGATE)
#define PERF_COUNTERSET_MULTI_AGGREGATE          (PERF_COUNTERSET_FLAG_AGGREGATE | PERF_COUNTERSET_FLAG_MULTIPLE)
#define PERF_COUNTERSET_SINGLE_AGGREGATE_HISTORY (PERF_COUNTERSET_FLAG_HISTORY | PERF_COUNTERSET_SINGLE_AGGREGATE)
#define PERF_COUNTERSET_INSTANCE_AGGREGATE       (PERF_COUNTERSET_MULTI_AGGREGATE | PERF_COUNTERSET_FLAG_INSTANCE)

#define PERF_ATTRIB_BY_REFERENCE       0x0000000000000001
#define PERF_ATTRIB_NO_DISPLAYABLE     0x0000000000000002
#define PERF_ATTRIB_NO_GROUP_SEPARATOR 0x0000000000000004
#define PERF_ATTRIB_DISPLAY_AS_REAL    0x0000000000000008
#define PERF_ATTRIB_DISPLAY_AS_HEX     0x0000000000000010

typedef struct _PERF_COUNTERSET_INFO {
    GUID   CounterSetGuid;
    GUID   ProviderGuid;
    ULONG  NumCounters;
    ULONG  InstanceType;
} PERF_COUNTERSET_INFO, * PPERF_COUNTERSET_INFO;

typedef struct _PERF_COUNTER_INFO {
    ULONG      CounterId;     // max of 64K counters per GUID instance
    ULONG      Type;
    ULONGLONG  Attrib;
    ULONG      Size;
    ULONG      DetailLevel;
    LONG       Scale;
    ULONG      Offset;         // overlays to give the actual counter
} PERF_COUNTER_INFO, * PPERF_COUNTER_INFO;

typedef struct _PERF_COUNTERSET_INSTANCE {
    GUID   CounterSetGuid;
    ULONG  dwSize;
    ULONG  InstanceId;
    ULONG  InstanceNameOffset;
    ULONG  InstanceNameSize;
} PERF_COUNTERSET_INSTANCE, * PPERF_COUNTERSET_INSTANCE;

typedef ULONG (
#ifndef MIDL_PASS
WINAPI
#endif
* PERFLIBREQUEST)(
    IN ULONG  RequestCode,
    IN PVOID  Buffer,
    IN ULONG  BufferSize
);

ULONG __stdcall
PerfStartProvider(
    __in     LPGUID          ProviderGuid,
    __in_opt PERFLIBREQUEST  ControlCallback,
    __out    HANDLE        * phProvider
);

typedef LPVOID (* PERF_MEM_ALLOC)(IN SIZE_T AllocSize, IN LPVOID pContext);
typedef void (* PERF_MEM_FREE)(IN LPVOID pBuffer, IN LPVOID pContext);

typedef struct _PROVIDER_CONTEXT {
    DWORD          ContextSize; // should be sizeof(PERF_PROVIDER_CONTEXT)
    DWORD          Reserved;
    PERFLIBREQUEST ControlCallback;
    PERF_MEM_ALLOC MemAllocRoutine;
    PERF_MEM_FREE  MemFreeRoutine;
    LPVOID         pMemContext;
} PERF_PROVIDER_CONTEXT, * PPERF_PROVIDER_CONTEXT;

ULONG __stdcall
PerfStartProviderEx(
    __in     LPGUID                 ProviderGuid,
    __in_opt PPERF_PROVIDER_CONTEXT ProviderContext,
    __out    HANDLE               * phProvider
);

ULONG __stdcall
PerfStopProvider(
    __in HANDLE hProvider
);

__success(return == ERROR_SUCCESS)
ULONG __stdcall
PerfSetCounterSetInfo(
    __in     HANDLE                hProvider,
    __inout_bcount(dwTemplateSize) PPERF_COUNTERSET_INFO pTemplate,
    __in     ULONG                 dwTemplateSize
);

PPERF_COUNTERSET_INSTANCE __stdcall
PerfCreateInstance(
    __in HANDLE  hProvider,
    __in LPCGUID CounterSetGuid,
    __in LPCWSTR szInstanceName,
    __in ULONG   dwInstance
);

ULONG __stdcall
PerfDeleteInstance(
    __in HANDLE                    hProvider,
    __in PPERF_COUNTERSET_INSTANCE InstanceBlock
);

ULONG __stdcall
PerfSetCounterRefValue(
    __in HANDLE                    hProvider,
    __in PPERF_COUNTERSET_INSTANCE pInstance,
    __in ULONG                     CounterId,
    __in LPVOID                    lpAddr
);

#ifdef __cplusplus
}
#endif
