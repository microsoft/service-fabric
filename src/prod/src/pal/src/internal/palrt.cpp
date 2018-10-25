// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "palrt.h"
#include <string>

using namespace std;

static bool isslash(wchar_t c)
{
    return c == '/' || c == '\\';
}

STDAPI_(BOOL) PathAppendW(LPWSTR pszPath, LPCWSTR pszMore)
{
    int lenPath = wcslen(pszPath);
    int lenMore = wcslen(pszMore);
    if (lenPath > 0 && pszPath[lenPath - 1] == L'/' &&
        lenMore > 0 && pszMore[0] == L'/')
    {
        pszPath[lenPath - 1] = 0;
    }
    else if (lenPath > 0 && pszPath[lenPath - 1] != L'/' &&
             lenMore > 0 && pszMore[0] != L'/')
    {
        pszPath[lenPath] = L'/';
        pszPath[lenPath + 1] = 0;
    }
    wcsncat(pszPath, pszMore, lenMore);
    return TRUE;
}

STDAPI_(BOOL) PathRemoveFileSpecW(LPWSTR pFile)
{
    if (pFile)
    {
        LPWSTR slow, fast = pFile;

        for (slow = fast; *fast; fast++)
        {
            if (isslash(*fast))
            {
                slow = fast;
            }
        }

        if (*slow == 0)
        {
            return FALSE;
        }
        else if ((slow == pFile) && isslash(*slow))
        {
            if (*(slow + 1) != 0)
            {
                *(slow + 1) = 0;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            *slow = 0;
            return TRUE;
        }
    }
    return  FALSE;
}

STDAPI_(LPWSTR) PathCombineW(LPWSTR lpszDest, LPCWSTR lpszDir, LPCWSTR lpszFile)
{
    if (lpszDest)
    {
        wstring dir, file;
        if (lpszDir)
        {
            dir = lpszDir;
            if (!isslash(dir.back()))
            {
                dir.push_back('/');
            }
        }
        if (lpszFile)
        {
            file = lpszFile;
            if (!file.empty() && isslash(file.front()))
            {
                dir = L"";
            }
        }
        wstring path  = dir + file;
        memcpy(lpszDest, path.c_str(), (path.length() + 1) * sizeof(wchar_t));
    }
    return lpszDest;
}
