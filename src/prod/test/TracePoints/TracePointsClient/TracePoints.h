// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <windows.h>
#include <evntprov.h>

typedef void * EventProviderHandle;
typedef void * ProviderMapHandle;
typedef void (*TracePointModuleFunctionPtr)(ProviderMapHandle h, LPCWSTR userData);
typedef void (*TracePointFunctionPtr)(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData);
typedef GUID ProviderId;
typedef USHORT EventId;

EXTERN_C DWORD WINAPI TracePointsRemoveEventMap(ProviderMapHandle h, ProviderId providerId);

EXTERN_C DWORD WINAPI TracePointsAddEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId, TracePointFunctionPtr action, void * context);

EXTERN_C DWORD WINAPI TracePointsRemoveEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId);

EXTERN_C DWORD WINAPI TracePointsClearEventMap(ProviderMapHandle h, ProviderId providerId);

EXTERN_C DWORD WINAPI TracePointsSetup(DWORD processId, LPCWSTR dllName, LPCSTR setupFunctionName, LPCWSTR userData);

EXTERN_C DWORD WINAPI TracePointsInvoke(DWORD processId, LPCWSTR dllName, LPCSTR invokeFunctionName, LPCWSTR userData);

EXTERN_C DWORD WINAPI TracePointsCleanup(DWORD processId, LPCWSTR dllName, LPCSTR cleanUpFunctionName, LPCWSTR userData);

EXTERN_C EventProviderHandle WINAPI TracePointsGetEventProvider(ProviderMapHandle h);

EXTERN_C DWORD WINAPI TracePointsEventWriteString(EventProviderHandle h, UCHAR Level, ULONGLONG Keyword, PCWSTR String);
