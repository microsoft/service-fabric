// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DLLEXPORT_(returnType) EXTERN_C returnType WINAPI

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved);

DLLEXPORT_(DWORD) TracePointsRemoveEventMap(ProviderMapHandle h, ProviderId providerId);

DLLEXPORT_(DWORD) TracePointsAddEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId, TracePointFunctionPtr action, void * context);

DLLEXPORT_(DWORD) TracePointsRemoveEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId);

DLLEXPORT_(DWORD) TracePointsClearEventMap(ProviderMapHandle h, ProviderId providerId);

DLLEXPORT_(DWORD) TracePointsSetup(DWORD processId, LPCWSTR dllName, LPCSTR setupFunctionName, LPCWSTR userData);

DLLEXPORT_(DWORD) TracePointsInvoke(DWORD processId, LPCWSTR dllName, LPCSTR invokeFunctionName, LPCWSTR userData);

DLLEXPORT_(DWORD) TracePointsCleanup(DWORD processId, LPCWSTR dllName, LPCSTR cleanUpFunctionName, LPCWSTR userData);

DWORD TracePointsRemoteCall(DWORD processId, LPCWSTR dllName, LPCSTR functionName, LPCWSTR userData, BYTE opCode);

TracePoints::ClientEventSource const & GetEventSource();
