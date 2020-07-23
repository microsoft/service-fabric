//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------
#pragma once

#include <string>
#include <windows.h>

class PathHelper
{
public:

    static std::wstring GetDirectoryName(const std::wstring& path);

    // Returns the name and extension parts of the given path. The resulting
    // string contains the characters of path that follow the last
    // backslash ("\"), slash ("/"), or colon (":") character in 
    // path. The resulting string is the entire path if path 
    // contains no backslash after removing trailing slashes, slash, or colon characters. The resulting 
    // string is null if path is null.

    static std::wstring GetFileName(std::wstring const & path);

    static void CombineInPlace(std::wstring& path1, const std::wstring& path2);
    static std::wstring Combine(const std::wstring& path1, const std::wstring& path2);

    static std::wstring GetModuleLocation();

    static std::wstring GetCurrentDirectory();

private:

    // Returns the path to the module HMODULE
    // This is a wrapper around GetModuleFileName
    static std::wstring GetModuleLocation(HMODULE hModule);
};
