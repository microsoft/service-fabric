// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "perflib.h"

ULONG __stdcall
PerfStartProvider(
    __in     LPGUID          ProviderGuid,
    __in_opt PERFLIBREQUEST  ControlCallback,
    __out    HANDLE        * phProvider
)
{
    static int fakedHandle = 0;
    fakedHandle++;
    *phProvider = (HANDLE) fakedHandle; // Return a faked handle
    return 0;
}

ULONG __stdcall
PerfStopProvider(
    __in HANDLE hProvider
)
{
    return 0;
}

__success(return == ERROR_SUCCESS)
ULONG __stdcall
PerfSetCounterSetInfo(
    __in     HANDLE                hProvider,
    __inout_bcount(dwTemplateSize) PPERF_COUNTERSET_INFO pTemplate,
    __in     ULONG                 dwTemplateSize
)
{
    return 0;
}

PPERF_COUNTERSET_INSTANCE __stdcall
PerfCreateInstance(
    __in HANDLE  hProvider,
    __in LPCGUID CounterSetGuid,
    __in LPCWSTR szInstanceName,
    __in ULONG   dwInstance
)
{
    static PERF_COUNTERSET_INSTANCE dummy_instance{0};
    return &dummy_instance;
}

ULONG __stdcall
PerfDeleteInstance(
    __in HANDLE                    hProvider,
    __in PPERF_COUNTERSET_INSTANCE InstanceBlock
)
{
    return 0;
}

ULONG __stdcall
PerfSetCounterRefValue(
    __in HANDLE                    hProvider,
    __in PPERF_COUNTERSET_INSTANCE pInstance,
    __in ULONG                     CounterId,
    __in LPVOID                    lpAddr
)
{
    return 0;
}
