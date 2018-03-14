// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if defined(PLATFORM_UNIX)
namespace DNS{
 
int IdnToUnicode(
    __in unsigned long dwFlags,
    __in LPCWSTR lpASCIICharStr,
    __in int cchASCIIChar,
    __out LPWSTR lpUnicodeCharStr,
    __in int cchUnicodeChar 
);

int IdnToAscii(
    __in unsigned long dwFlags,
    __in LPCWSTR unicodestr,
    __in int unicodelength,
    __out LPWSTR asciistr,
    __in int asciisize
);

 
}
#endif
