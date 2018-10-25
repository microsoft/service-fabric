// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_USERENV_)
#define USERENVAPI DECLSPEC_IMPORT
#else
#define USERENVAPI
#endif

USERENVAPI
BOOL
WINAPI
DeleteProfileA (
        IN LPCSTR lpSidString,
        IN LPCSTR lpProfilePath,
        IN LPCSTR lpComputerName);
USERENVAPI
BOOL
WINAPI
DeleteProfileW (
        IN LPCWSTR lpSidString,
        IN LPCWSTR lpProfilePath,
        IN LPCWSTR lpComputerName);
#ifdef UNICODE
#define DeleteProfile  DeleteProfileW
#else
#define DeleteProfile  DeleteProfileA
#endif // !UNICODE

#ifdef __cplusplus
}
#endif
