//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "Path.h"
#include "WexString.h"
#include "WexTestClass.h"

using namespace std;
using namespace WEX::Common;
using namespace WEX::Logging;

std::wstring InvalidPathChars = L"\"<>|";

bool CheckInvalidPathChars(std::wstring const & path)
{
    size_t index = path.find_first_of(InvalidPathChars);

    return index == std::wstring::npos;
}

size_t GetRootLength(std::wstring const & path)
{
    if (path.empty())
        return 0;

    if (path.size() == 1)
        return (path[0] == '\\') ? 1 : 0;

    // path.size >= 2 at this point

    if (path[1] == ':')
        return 2;

    if (path[0] != '\\')
    {
        return 0;
    }

    // now path[0] == '\\'

    if (path[1] != '\\')
        return 1;

    // now path.StartsWith("\\\\")

    if (path.size() == 2)
        return 2;

    // get the \\server\share part
    size_t index = path.find('\\', 2);
    if (index == std::wstring::npos)
    {
        return path.size();
    }

    return index;
}

// Returns the directory path of a file path. This method effectively
// removes the last element of the given file path, i.e. it returns a
// string consisting of all characters up to but not including the last
// backslash ("\") in the file path. The returned value is null if the file
// path is null or if the file path denotes a root (such as "\", "C:", or
// "\\server\share").

std::wstring PathHelper::GetDirectoryName(std::wstring const & path)
{
    if (!CheckInvalidPathChars(path))
    {
        return L"";
    }

    size_t index = path.rfind('\\');

    if (index == std::wstring::npos)
        return L"";

    if (index == 0)
        return L"";

    if (index < GetRootLength(path))
    {
        return L"";
    }

    return path.substr(0, index);
}

// Expands the given path to a fully qualified path. The resulting string
// consists of a drive letter, a colon, and a root relative path. This
// function does not verify that the resulting path is valid or that it
// refers to an existing file or DirectoryInfo on the associated volume.

// Returns the name and extension parts of the given path. The resulting
// string contains the characters of path that follow the last
// backslash ("\"), slash ("/"), or colon (":") character in 
// path. The resulting string is the entire path if path 
// contains no backslash after removing trailing slashes, slash, or colon characters. The resulting 
// string is null if path is null.

std::wstring PathHelper::GetFileName(std::wstring const & path)
{
    size_t index = path.find_last_of(L"\\:");
    if (index == std::wstring::npos)
        return path;

    return path.substr(index + 1);
}

// Returns the root portion of the given path. The resulting string
// consists of those rightmost characters of the path that constitute the
// root of the path. Possible patterns for the resulting string are: An
// empty string (a relative path on the current drive), "\" (an absolute
// path on the current drive), "X:" (a relative path on a given drive,
// where X is the drive letter), "X:\" (an absolute path on a given drive),
// and "\\server\share" (a UNC path for a given server and share name).
// The resulting string is null if path is null.
//
//std::wstring PathHelper::GetPathRoot(std::wstring const & path);
//std::wstring PathHelper::GetFullPath(std::wstring const & path); // implement when needed
//std::wstring PathHelper::GetTempPath();
//std::wstring PathHelper::GetTempFileName();
//bool PathHelper::HasExtension(std::wstring const & path);
//bool PathHelper::IsPathRooted(std::wstring const & path);

void PathHelper::CombineInPlace(std::wstring& path1, std::wstring const & path2)
{
    if (path2.empty())
        return;

    if (path1.empty()) {
        path1.append(path2);
        return;
    }

    if (path1.back() != '\\') {
        path1.push_back('\\');
    }

    path1.append(path2);
}

std::wstring PathHelper::Combine(std::wstring const & path1, std::wstring const & path2)
{
    std::wstring result(path1);
    CombineInPlace(result, path2);
    return result;
}

wstring PathHelper::GetModuleLocation()
{
    // See http://msdn.microsoft.com/en-us/library/ms683200(v=VS.85).aspx
    // The second parameter of GetModuleHandleEx can be a pointer to any
    // address mapped to the module.  We define a static to use as that
    // pointer.
    static int staticInThisModule = 0;

    HMODULE currentModule = NULL;
    BOOL success = ::GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCTSTR>(&staticInThisModule),
        &currentModule);

    if (success == 0)
    {
        return L"";
    }

    return GetModuleLocation(currentModule);
}

wstring PathHelper::GetCurrentDirectory()
{
    std::wstring path;
    auto winSize = ::GetCurrentDirectory(0, NULL);
    auto stlSize = static_cast<size_t>(winSize);
    path.resize(stlSize);
    if (!::GetCurrentDirectory(winSize, const_cast<wchar_t *>(path.data())))
    {
        DWORD lastError = GetLastError();
        Log::Error(String().Format(L"::GetCurrentDirectory() failed (%lu)", lastError));
        return wstring(L"");
    }

    // Remove the null terminator written by ::GetCurrentDirectory()
    path.resize(winSize - 1);

    return path;
}

std::wstring PathHelper::GetModuleLocation(HMODULE hModule)
{
    std::wstring buffer;
    size_t length = MAX_PATH;

    for (;;)
    {
        buffer.resize(length);

        DWORD dwReturnValue = ::GetModuleFileName(hModule, &buffer[0], static_cast<DWORD>(buffer.size()));
        if (dwReturnValue == 0)
        {
            return L"";
        }
        else if (dwReturnValue == static_cast<DWORD>(buffer.size()) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // insufficient buffer case -> try with bigger size
            length = length * 2;
            continue;
        }
        else
        {
            // return value is length of string in characters not including terminating null character
            buffer.resize(dwReturnValue);
            break;
        }
    }

    return buffer;
}
