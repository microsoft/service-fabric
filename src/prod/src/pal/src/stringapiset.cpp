// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stringapiset.h"
#include "util/pal_string_util.h"
#include <string>

using namespace std;
using namespace Pal;

WINBASEAPI
_Success_(return != 0)
         _When_((cbMultiByte == -1) && (cchWideChar != 0), _Post_equal_to_(_String_length_(lpWideCharStr)+1))
int
WINAPI
MultiByteToWideChar(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
    _In_ int cbMultiByte,
    _Out_writes_to_opt_(cchWideChar, return) LPWSTR lpWideCharStr,
    _In_ int cchWideChar
    )
{
    if ((cbMultiByte == 0) || (cchWideChar < 0) || (lpMultiByteStr == NULL)
        || ((cchWideChar != 0) && ((lpWideCharStr == NULL) || (lpMultiByteStr == (LPSTR)lpWideCharStr))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    int append_null = (cbMultiByte < 0);
    string in = (!append_null ? string(lpMultiByteStr, cbMultiByte) : lpMultiByteStr);
    wstring result = utf8to16(in.c_str());
    int len = result.length();
    if (cchWideChar >= len + append_null)
    {
        //memset(lpWideCharStr, 0, cchWideChar * sizeof(wchar_t));
        memcpy(lpWideCharStr, result.c_str(), (len + append_null) * sizeof(wchar_t));
        return (len + append_null);
    }
    return len + append_null;
}

WINBASEAPI
_Success_(return != 0)
         _When_((cchWideChar == -1) && (cbMultiByte != 0), _Post_equal_to_(_String_length_(lpMultiByteStr)+1))
int
WINAPI
WideCharToMultiByte(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
    _In_ int cchWideChar,
    _Out_writes_bytes_to_opt_(cbMultiByte, return) LPSTR lpMultiByteStr,
    _In_ int cbMultiByte,
    _In_opt_ LPCCH lpDefaultChar,
    _Out_opt_ LPBOOL lpUsedDefaultChar
    )
{
    if ((cchWideChar < -1) || (cbMultiByte < 0) || (lpWideCharStr == NULL)
        || ((cbMultiByte != 0) && ((lpMultiByteStr == NULL) || (lpWideCharStr == (LPWSTR)lpMultiByteStr))))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    int append_null = (cchWideChar < 0);
    wstring in = (!append_null ? wstring(lpWideCharStr, cchWideChar) : lpWideCharStr);
    string result = utf16to8(in.c_str());
    int len = result.length();
    if (cbMultiByte >= len + append_null)
    {
        //memset(lpMultiByteStr, 0, cbMultiByte);
        memcpy(lpMultiByteStr, result.c_str(), (len + append_null) * sizeof(char));
        return (len + append_null);
    }
    return len + append_null;
}
