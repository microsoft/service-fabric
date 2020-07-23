// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

static ULONG (WINAPI * TrueEventRegister)(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext, PREGHANDLE RegHandle) = EventRegister;
static ULONG (WINAPI * TrueEventUnregister)(REGHANDLE RegHandle) = EventUnregister;
static ULONG (WINAPI * TrueEventWrite)(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData) = EventWrite;

TracePointsState GlobalState;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(reserved);

    if (dwReason == DLL_PROCESS_ATTACH) 
    {
        // Detours must be attached AFTER we set this up (since it will call ETW functions)
        GlobalState.Setup(CallTrueEventWrite);
        AttachDetours();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // Detours must be detached BEFORE we clean this up (since it will call ETW functions)
        DetachDetours();
        GlobalState.Cleanup();
    }

    return TRUE;
}

// Needed so that we don't detour the internal EventWrite calls that TracePoints itself makes
EXTERN_C ULONG WINAPI CallTrueEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    return TrueEventWrite(RegHandle, EventDescriptor, UserDataCount, UserData);
}

EXTERN_C ULONG WINAPI InterceptEventRegister(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext, PREGHANDLE RegHandle)
{
    ULONG result = TrueEventRegister(ProviderId, EnableCallback, CallbackContext, RegHandle);
    GlobalState.OnEventRegister(ProviderId, RegHandle);
    return result;
}

EXTERN_C ULONG WINAPI InterceptEventUnregister(REGHANDLE RegHandle)
{
    ULONG result = TrueEventUnregister(RegHandle);
    GlobalState.OnEventUnregister(RegHandle);
    return result;
}

EXTERN_C ULONG WINAPI InterceptEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    ULONG result = TrueEventWrite(RegHandle, EventDescriptor, UserDataCount, UserData);
    GlobalState.OnEventWrite(RegHandle, EventDescriptor, UserDataCount, UserData);
    return result;
}

void AttachDetours()
{
    DetourManager detourManager(true);
    detourManager.Attach(reinterpret_cast<void **>(&TrueEventRegister), InterceptEventRegister);
    detourManager.Attach(reinterpret_cast<void **>(&TrueEventUnregister), InterceptEventUnregister);
    detourManager.Attach(reinterpret_cast<void **>(&TrueEventWrite), InterceptEventWrite);
}

void DetachDetours()
{
    DetourManager detourManager(false);
    detourManager.Detach(reinterpret_cast<void **>(&TrueEventRegister), InterceptEventRegister);
    detourManager.Detach(reinterpret_cast<void **>(&TrueEventUnregister), InterceptEventUnregister);
    detourManager.Detach(reinterpret_cast<void **>(&TrueEventWrite), InterceptEventWrite);
}
