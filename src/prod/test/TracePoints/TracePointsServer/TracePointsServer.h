// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved);

EXTERN_C ULONG WINAPI CallTrueEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData);

EXTERN_C ULONG WINAPI InterceptEventRegister(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext, PREGHANDLE RegHandle);

EXTERN_C ULONG WINAPI InterceptEventUnregister(REGHANDLE RegHandle);

EXTERN_C ULONG WINAPI InterceptEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData);

void AttachDetours();

void DetachDetours();
