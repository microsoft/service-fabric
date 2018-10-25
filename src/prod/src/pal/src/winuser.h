// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_USER32_)
#define WINUSERAPI DECLSPEC_IMPORT
#define WINABLEAPI DECLSPEC_IMPORT
#else
#define WINUSERAPI
#define WINABLEAPI
#endif

// 4903
WINUSERAPI
DWORD
WINAPI
CharLowerBuffA(
    __inout_ecount(cchLength) LPSTR lpsz,
    __in DWORD cchLength);
WINUSERAPI
DWORD
WINAPI
CharLowerBuffW(
    __inout_ecount(cchLength) LPWSTR lpsz,
    __in DWORD cchLength);
#ifdef UNICODE
#define CharLowerBuff  CharLowerBuffW
#else
#define CharLowerBuff  CharLowerBuffA
#endif // !UNICODE

// 4993
WINUSERAPI
BOOL
WINAPI
IsCharAlphaA(
    __in CHAR ch);
WINUSERAPI
BOOL
WINAPI
IsCharAlphaW(
    __in WCHAR ch);
#ifdef UNICODE
#define IsCharAlpha  IsCharAlphaW
#else
#define IsCharAlpha  IsCharAlphaA
#endif // !UNICODE

#ifdef __cplusplus
}
#endif
