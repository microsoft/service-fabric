// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __REGISTRYWRAPPER_H
#define __REGISTRYWRAPPER_H

#if !defined(FEATURE_UTILCODE_NO_DEPENDENCIES) && !defined(FEATURE_CORECLR)

// Registry API wrappers

LONG ClrRegCreateKeyEx(
        HKEY hKey,
        LPCWSTR lpSubKey,
        DWORD Reserved,
        __in_opt LPWSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition
        );

LONG ClrRegOpenKeyEx(
        HKEY hKey,
        LPCWSTR lpSubKey,
        DWORD ulOptions,
        REGSAM samDesired,
        PHKEY phkResult
        );

bool IsNgenOffline();

#else

#define ClrRegCreateKeyEx RegCreateKeyExW
#define ClrRegOpenKeyEx RegOpenKeyExW
#define IsNgenOffline() false

#endif

#endif // __REGISTRYWRAPPER_H
