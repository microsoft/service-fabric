// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/Directory.h"
#include "Common/File.h"

#if defined(PLATFORM_UNIX)
#include <dirent.h>
#include <sys/stat.h>

int rmdir_p(const char* dirname)
{
    DIR* dp;
    struct dirent *ep;
    char pbuf[MAX_PATH] = {0};
    int result = -1;

    // in case symlink
    struct stat lstat_data;
    if (lstat(dirname, &lstat_data) == 0 && S_ISLNK(lstat_data.st_mode))
    {
        return unlink(dirname);
    }

    dp = opendir(dirname);
    result = (dp != NULL ? 0 : -1);

    if (dp)
    {
        while ((ep = readdir(dp)) != NULL)
        {
            struct stat stat_data;
            if(strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
            {
                continue;
            }
            sprintf(pbuf, "%s/%s", dirname, ep->d_name);
            if(stat(pbuf, &stat_data) == 0 && S_ISDIR(stat_data.st_mode))
            {
                result = rmdir_p(pbuf);
            }
            else
            {
                result = unlink(pbuf);
            }
            if (result != 0)
            {
                closedir(dp);
                return result;
            }
        }

        closedir(dp);
        result = rmdir(dirname);
    }
    return result;
}

DWORD GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    char buf[MAX_PATH + 1];
    char* cwd = getcwd(buf, MAX_PATH + 1);
    if (!cwd)
    {
        return DirGetLastErrorFromErrno();
    }
    size_t len = strlen(cwd);
    if (nBufferLength <= len)
    {
        return len + 1;
    }
    wstring cwdW = Common::StringUtility::Utf8ToUtf16(cwd);
    memcpy(lpBuffer, cwdW.c_str(), (cwdW.length() + 1) * sizeof(wchar_t));
    return len;
}

BOOL SetCurrentDirectoryW(LPCWSTR lpPathName)
{
    string pathA = FileNormalizePath(lpPathName);
    if (pathA.empty())
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    if (chdir(pathA.c_str()) != 0)
    {
        struct stat stat_data;
        if ((errno == ENOTDIR || errno == ENOENT) &&
            (stat(pathA.c_str(), &stat_data) == 0 && (stat_data.st_mode & S_IFMT) == S_IFREG))
        {
            SetLastError(ERROR_DIRECTORY);  // Not a directory, it is a file.
        }
        else
        {
            SetLastError(DirGetLastErrorFromErrno());
        }
        return FALSE;
    }
    return TRUE;
}

BOOL CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    string pathA = FileNormalizePath(lpPathName);
    if (pathA.empty())
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (pathA.front() != '/')
    {
        char buf[MAX_PATH + 1];
        char* cwd = getcwd(buf, MAX_PATH + 1);
        if (!cwd)
        {
            SetLastError(DirGetLastErrorFromErrno());
            return FALSE;
        }
        pathA = string(cwd) + "/" + pathA;
    }

    int mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (mkdir(pathA.c_str(), mode) != 0)
    {
        SetLastError(DirGetLastErrorFromErrno());
        return FALSE;
    }
    return TRUE;
}

BOOL RemoveDirectoryW(LPCWSTR lpPathName)
{
    string pathA = FileNormalizePath(lpPathName);
    if (pathA.empty())
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (rmdir_p(pathA.c_str()) != 0)
    {
        if (errno != ENOENT)
        {
            SetLastError(DirGetLastErrorFromErrno());
        }
        return FALSE;
    }
    return TRUE;
}

#endif

namespace Common
{
    using namespace std;

    StringLiteral const TraceSource("Directory");

    const int Directory::DirectoryOperationMaxRetryCount = 50;
    const int Directory::DirectoryOperationRetryIntervalInMillis = 200;

    void Directory::Create(
        std::wstring const & path)
    {
        auto ntPath = Path::ConvertToNtPath(path);
        while (!::CreateDirectory(ntPath.c_str(), NULL))
        {
            switch (::GetLastError())
            {
            case ERROR_ALREADY_EXISTS:
                if (Directory::Exists(path))
                {
                    return;
                }
                else
                {
                    THROW_FMT(microsoft::GetLastErrorCode(), "CreateDirectory failed for '{0}'", path);
                }

            case ERROR_PATH_NOT_FOUND:
                {
                    auto offset = path.find_last_of(Path::GetPathSeparatorChar());
#if defined(PLATFORM_UNIX)
                    if (offset == std::wstring::npos)
                    {
                        offset = path.find_last_of('\\');
                    }
#endif
                    if (offset == std::wstring::npos)
                    {
                        THROW_FMT(microsoft::GetLastErrorCode(), "CreateDirectory failed for '{0}'", path);
                    }

                    std::wstring parent = path.substr(0, offset);
                    Directory::CreateDirectory(parent);
                    break;
                }

            default:
                THROW_FMT(microsoft::GetLastErrorCode(), "CreateDirectory failed for '{0}'", path);
            }
        }
    }

    ErrorCode Directory::Create2(std::string const & path)
    {
        return Create2(StringUtility::Utf8ToUtf16(path));
    }

    ErrorCode Directory::Create2(
        std::wstring const & path)
    {
        bool parentDirectoryCreated = false;
        auto ntPath = Path::ConvertToNtPath(path);
        while (!::CreateDirectory(ntPath.c_str(), NULL))
        {
            switch (::GetLastError())
            {
            case ERROR_ALREADY_EXISTS:
                if (Directory::Exists(path))
                {
                    return ErrorCodeValue::Success;
                }
                else
                {
                    return ErrorCode::FromHResult(HRESULT_FROM_WIN32(::GetLastError()));
                }

            case ERROR_PATH_NOT_FOUND:
                {
                    auto offset = path.find_last_of(Path::GetPathSeparatorChar());
#if defined(PLATFORM_UNIX)
                    if (offset == std::wstring::npos)
                    {
                        offset = path.find_last_of('\\');
                    }
#endif
                    if (offset == std::wstring::npos || parentDirectoryCreated)
                    {
                        return ErrorCodeValue::InvalidDirectory;
                    }   

                    std::wstring parent = path.substr(0, offset);
                    ErrorCode err = Directory::Create2(parent);
                    if (!err.IsSuccess())
                    {
                        return err;
                    }
                    else
                    {
                        parentDirectoryCreated = true;
                    }
                    break;
                }

            case ERROR_INVALID_NAME:
                {
                    return ErrorCodeValue::InvalidDirectory;
                }

            default:
                return ErrorCode::FromHResult(HRESULT_FROM_WIN32(::GetLastError()));

            }
        }
        return ErrorCodeValue::Success;
    }

    ErrorCode Directory::Delete(std::wstring const & path, bool recursive)
    {
        return Directory::DeleteInternal(path, recursive, false);
    }

    ErrorCode Directory::Delete(std::wstring const & path, bool recursive, bool deleteReadOnlyFiles)
    {
        return Directory::DeleteInternal(path, recursive, deleteReadOnlyFiles);
    }

    ErrorCode Directory::Delete_WithRetry(std::wstring const & path, bool recursive, bool deleteReadOnlyFiles)
    {
        ErrorCode error(ErrorCodeValue::Success);
        int count = 0;
        bool shouldRetry = true;
        do
        {
            error = Directory::Delete(path, recursive, deleteReadOnlyFiles);
            if(error.IsSuccess() || error.IsError(ErrorCodeValue::DirectoryNotFound))
            {
                shouldRetry = false;
            }
            else
            {
                Trace.WriteWarning(
                    TraceSource, 
                    L"Delete_WithRetry", 
                    "Delete conflict error {0}: from {1}.", 
                    error, 
                    path);

                if(++count < Directory::DirectoryOperationMaxRetryCount)
                {
                    ::Sleep(Directory::DirectoryOperationRetryIntervalInMillis);
                }
                else
                {
                    shouldRetry = false;
                }
            }
        } while(shouldRetry);

        return error;
    }

    ErrorCode Directory::DeleteInternal(std::wstring const & path, bool recursive, bool deleteReadOnlyFiles)
    {
        auto ntPath = Path::ConvertToNtPath(path);
        while (!::RemoveDirectory(ntPath.c_str()))
        {
            if (!recursive || (::GetLastError() != ERROR_DIR_NOT_EMPTY))
            {
                auto winErr = ::GetLastError();
                if (winErr == ERROR_FILE_NOT_FOUND)
                {
                    return ErrorCodeValue::DirectoryNotFound;
                }

                return ErrorCode::FromHResult(HRESULT_FROM_WIN32(winErr));
            }
            else
            {
                std::wstring pattern(path);
                pattern.append(Path::GetPathSeparatorWstr() + L"*");
                auto find = File::Search(move(pattern));

                std::wstring fullPath;

                ErrorCode err;
                while ((err = find->MoveNext()).IsSuccess())
                {
                    auto currentName = &(find->GetCurrent().cFileName[0]);
                    if (((currentName[0] == L'.') && (currentName[1] == 0)) ||
                        ((currentName[0] == L'.') && (currentName[1] == L'.') && (currentName[2] == 0)))
                    {
                        // Skip . and ..
                        continue;
                    }

                    fullPath = path;
                    fullPath.append(Path::GetPathSeparatorWstr());
                    fullPath.append(currentName);

                    if ((find->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0)
                    {
                        err = Directory::DeleteInternal(fullPath, recursive, deleteReadOnlyFiles);

                        if (!err.IsSuccess())
                        {
                            return err;
                        }
                    }
                    else
                    {
                        if (!File::Delete(fullPath, NOTHROW()))
                        {
                            bool skipErrorTrace = false;
                            HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
                            if(hr == E_ACCESSDENIED && deleteReadOnlyFiles)
                            {
                                FileAttributes::Enum attributes;
                                ErrorCode attribError = File::GetAttributes(fullPath, attributes);
                                bool isReadOnlySet = (attributes & FileAttributes::ReadOnly) != 0;
                                if(attribError.IsSuccess() && isReadOnlySet)
                                {
                                    attributes = FileAttributes::Enum(attributes & ~FileAttributes::ReadOnly);
                                    attribError = File::SetAttributes(fullPath, attributes);
                                    if (attribError.IsSuccess() && File::Delete(fullPath, NOTHROW()))
                                    {
                                        skipErrorTrace = true;
                                    }
                                }
                            }

                            if(!skipErrorTrace)
                            {
                                return ErrorCode::FromHResult(hr);
                            }
                        }
                    }
                }
                if (err.ToHResult() != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && err.ToHResult() != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
                {
                    return err;
                }
            }
        }
        return ErrorCodeValue::Success;
    }

    bool Directory::Exists(std::string const & path)
    {
        auto result = ::GetFileAttributesW(Path::ConvertToNtPath(StringUtility::Utf8ToUtf16(path)).c_str());
        return (result != INVALID_FILE_ATTRIBUTES) && ((result & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    bool Directory::Exists(std::wstring const & path)
    {
        auto ntPath = Path::ConvertToNtPath(path);
        auto result = ::GetFileAttributesW(ntPath.c_str());
        return (result != INVALID_FILE_ATTRIBUTES) && ((result & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    ErrorCode Directory::Exists(
        std::wstring const & path,
        __out bool & exists)
    {
        auto result = ::GetFileAttributesW(Path::ConvertToNtPath(path).c_str());
        ErrorCode error(ErrorCodeValue::Success);
        if (result == INVALID_FILE_ATTRIBUTES)
        {
            DWORD dwErr = ::GetLastError();
            if (dwErr != ERROR_FILE_NOT_FOUND)
            {
                error = ErrorCode::FromWin32Error();

                Trace.WriteWarning(
                    TraceSource,
                    L"Exists",
                    "Directory Exists error {0}: for {1}.",
                    error,
                    path);
            }

            exists = false;
        }
        else
        {
            exists = (result & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        return error;
    }

    std::wstring Directory::GetCurrentDirectory()
    {
        std::wstring path;
        auto winSize = ::GetCurrentDirectory(0, NULL);
        auto stlSize = static_cast<size_t>(winSize);
        path.resize(stlSize);

        if (!::GetCurrentDirectory(winSize, const_cast<wchar_t *>(path.data())))
        {
            THROW_FMT(microsoft::GetLastErrorCode(), "GetCurrentDirectory failed for '{0}'", path);
        }

        // Remove the null terminator written by ::GetCurrentDirectory()
        CODING_ERROR_ASSERT(winSize > 0);
        path.resize(winSize - 1);

        return path;
    }

    void Directory::SetCurrentDirectory(std::wstring const & path)
    {
        if (!::SetCurrentDirectory(path.c_str()))
        {
            THROW_FMT(microsoft::GetLastErrorCode(), "SetCurrentDirectory failed for '{0}'", path);
        }
    }

    ErrorCode Directory::Copy(std::wstring const & src, std::wstring const & dest, bool overwrite)
    {
        ErrorCode err = ErrorCodeValue::Success;
        if (!Directory::Exists(dest))
        {
            err = Directory::Create2(dest);
            if (!err.IsSuccess())
            {
                return err;
            }
        }

        std::wstring pattern(src);
        pattern.append(Path::GetPathSeparatorWstr() + L"*");
        auto find = File::Search(move(pattern));
        while ((err = find->MoveNext()).IsSuccess())
        {
            auto currentName = &(find->GetCurrent().cFileName[0]);
            if (((currentName[0] == L'.') && (currentName[1] == 0)) ||
                ((currentName[0] == L'.') && (currentName[1] == L'.') && (currentName[2] == 0)) ||
                HasFileLockExtension(currentName))
            {
                // Skip . and ..
                continue;
            }

            std::wstring sourcePath = src;
            sourcePath.append(Path::GetPathSeparatorWstr());
            sourcePath.append(currentName);

            std::wstring destinationPath = dest;
            destinationPath.append(Path::GetPathSeparatorWstr());
            destinationPath.append(currentName);

            if ((find->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                err = Directory::Copy(sourcePath, destinationPath, overwrite);
                if (!err.IsSuccess())
                {
                    return err;
                }
            }
            else
            {
                if (File::Exists(destinationPath))
                {
                    if (overwrite)
                    {
                        err = File::Copy(sourcePath, destinationPath, true);
                        if (!err.IsSuccess())
                        {
                            return err;
                        }
                    }
                }
                else
                {
                    err = File::Copy(sourcePath, destinationPath, true);
                    if (!err.IsSuccess())
                    {
                        return err;
                    }
                }
            }
        }

        if (err.ToHResult() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || err.ToHResult() == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            return err;
        }
    }

    ErrorCode Directory::MoveFile(const std::wstring& source, const std::wstring& destination, bool overwrite)
    {
        auto uncSrc = Path::ConvertToNtPath(source);
        auto uncDest = Path::ConvertToNtPath(destination);

#if defined(PLATFORM_UNIX)
        DWORD flag = MOVEFILE_COPY_ALLOWED;
#else
        DWORD flag = MOVEFILE_WRITE_THROUGH;
#endif
        if (overwrite)
        {
            bool exists;
            ErrorCode error = Directory::Exists(uncDest, exists);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceSource,
                    L"MoveFile",
                    "MoveFile overwrite directory exists failed for {0} to {1}: error={2}",
                    uncSrc,
                    uncDest,
                    error);
            }

            if (exists)
            {
                error = Directory::Delete(uncDest, true);
                if (!error.IsSuccess())
                {
                    Trace.WriteWarning(
                        TraceSource,
                        L"MoveFile",
                        "MoveFile overwrite directory delete failed for {0}: error={1}",
                        uncDest,
                        error);
                    return error;
                }
            }

            flag = flag | MOVEFILE_REPLACE_EXISTING;
        }

        if (!::MoveFileExW(uncSrc.c_str(), uncDest.c_str(), flag))
        {
            HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
            Trace.WriteError(
                TraceSource,
                L"MoveFile",
                "MoveFileEx from {0} to {1} failed: error={2}",
                uncSrc,
                uncDest,
                ::GetLastError());
            return ErrorCode::FromHResult(hr);
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode Directory::SafeCopy(
        std::wstring const & src,
        std::wstring const & dest,
        bool overwrite,
        Common::TimeSpan const timeout)
    {
        std::wstring path = Path::GetDirectoryName(dest);
        if (!path.empty())
        {
            ErrorCode err = Directory::Create2(path);
            if (!err.IsSuccess())
            {
                return err;
            }
        }
        else
        {
            path = L".";
        }

        std::wstring tempFilePath = File::GetTempFileName(path);

        if(tempFilePath.length() > MAX_PATH)
        {
            Trace.WriteError(
                TraceSource,
                L"SafeCopy",
                "Temp path {0} is greater than MAX_PATH {1}",
                tempFilePath,
                MAX_PATH);

            return ErrorCodeValue::OperationFailed;
        }

        Trace.WriteInfo(
            TraceSource,
            L"SafeCopy",
            "Doing Safecopy from {0} to temp file {1} {2} overwrite.",
            src,
            tempFilePath,
            overwrite ? L"with" : L"without");

        FileReaderLock srcLock(src);
        FileWriterLock cacheLock(tempFilePath), destLock(dest);

        ErrorCode error = srcLock.Acquire(timeout);
        if (!error.IsSuccess())
        {
            return error; 
        }

        error = cacheLock.Acquire(timeout);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = destLock.Acquire(timeout);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = Directory::Copy(src, tempFilePath, overwrite);
        if (!error.IsSuccess())
        {
            auto err = Directory::Delete(tempFilePath, true);
            if (!err.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceSource,
                    L"SafeCopy",
                    "SafeCopy failed to clean up '{0}' after copy from '{1}' failed. Error: {2}",
                    tempFilePath,
                    src,
                    err);
            }
            return error;
        }

        error = Directory::MoveFile(tempFilePath, dest, overwrite);
        if (!error.IsSuccess())
        {
            auto err = Directory::Delete(tempFilePath, true);
            if (!err.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceSource,
                    L"SafeCopy",
                    "SafeCopy failed to clean up '{0}' after move to '{1}' failed. Error: {2}",
                    tempFilePath,
                    dest,
                    err);
            }
        }

        return error;
    }

    ErrorCode Directory::Rename(std::wstring const & src, std::wstring const & dest, bool const overwrite)
    {
        return MoveFile(src, dest, overwrite);
    }

    ErrorCode Directory::Rename_WithRetry(std::wstring const & src, std::wstring const & dest, bool const overwrite)
    {
        ErrorCode error(ErrorCodeValue::Success);
        int count = 0;
        do
        {
            error = Directory::Rename(src, dest, overwrite);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceSource, 
                    L"Rename_WithRetry", 
                    "Move conflict error {0}: from {1} to {2}.", 
                    error, 
                    src, 
                    dest);

                ::Sleep(Directory::DirectoryOperationRetryIntervalInMillis);
                count++;
            }
        } while (!error.IsSuccess() && count < Directory::DirectoryOperationMaxRetryCount);

        return error;
    }

    ErrorCode Directory::Echo(
        std::wstring const & src,
        std::wstring const & dest,
        Common::TimeSpan const timeout)
    {
        ErrorCode err = ErrorCodeValue::Success;
        if (!Directory::Exists(dest))
        {
            err = Directory::Create2(dest);
            if (!err.IsSuccess())
            {
                return err;
            }
        }

        FileReaderLock srcLock(src);
        FileWriterLock destLock(dest);

        ErrorCode error = srcLock.Acquire(timeout);
        if (!error.IsSuccess())
        {
            return error; 
        }

        error = destLock.Acquire(timeout);
        if (!error.IsSuccess())
        {
            return error;
        }

        std::wstring pattern(src);
        pattern.append(Path::GetPathSeparatorWstr() + L"*");
        auto find = File::Search(move(pattern));
        while ((err = find->MoveNext()).IsSuccess())
        {
            auto currentName = &(find->GetCurrent().cFileName[0]);
            if (((currentName[0] == L'.') && (currentName[1] == 0)) ||
                ((currentName[0] == L'.') && (currentName[1] == L'.') && (currentName[2] == 0)) ||
                HasFileLockExtension(currentName))
            {
                // Skip ., .. and .ReadLock/.WriteLock
                continue;
            }

            std::wstring sourcePath = src;
            sourcePath.append(Path::GetPathSeparatorWstr());
            sourcePath.append(currentName);

            std::wstring destinationPath = dest;
            destinationPath.append(Path::GetPathSeparatorWstr());
            destinationPath.append(currentName);

            if ((find->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                err = Directory::Echo(sourcePath, destinationPath, timeout);
                if (!err.IsSuccess())
                {
                    return err;
                }
            }
            else
            {                
                err = File::Echo(sourcePath, destinationPath, timeout);
                if (!err.IsSuccess())
                {
                    return err;
                }
            }
        }

        if (err.ToHResult() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || err.ToHResult() == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            return err;
        }
    }

    vector<wstring> Directory::Find(wstring const & path, wstring const & filePattern, uint expectedFileCount, bool findRecursively)
    {                
        vector<wstring> fileList;
        map<std::wstring, WIN32_FIND_DATA> fileListMap = FindWithFileProperties(path, filePattern, expectedFileCount, findRecursively);
        typedef map<std::wstring, WIN32_FIND_DATA>::value_type pair_type;

        transform(
            fileListMap.begin(), 
            fileListMap.end(), 
            back_inserter(fileList),
            bind (&pair_type::first, placeholders::_1));

        return fileList;
    }

    map<std::wstring, WIN32_FIND_DATA> Directory::FindWithFileProperties(wstring const & path, wstring const & filePattern, uint expectedFileCount, bool findRecursively)
    {                
        map<std::wstring, WIN32_FIND_DATA> fileList;

#if defined(PLATFORM_UNIX)
        queue<string> directoriesToSearch;
        string pathA, filePatternA;
        StringUtility::Utf16ToUtf8(path, pathA);
        StringUtility::Utf16ToUtf8(filePattern, filePatternA);

        // normalize the path
        std::replace(pathA.begin(), pathA.end(), '\\', '/');
        if (pathA.back() == '/')
        {
            pathA = pathA.substr(0, pathA.length() - 1);
        }

        directoriesToSearch.push(pathA);

        while (!directoriesToSearch.empty())
        {
            string currentDirectory = directoriesToSearch.front();
            directoriesToSearch.pop();

            DIR *dp = opendir(currentDirectory.c_str());
            if (!dp) continue;
            struct dirent entry;
            struct dirent *dptr = NULL;
            while (true)
            {
                if (readdir_r(dp, &entry, &dptr) || !dptr) break;

                string currentName(dptr->d_name);
                string currentPath = currentDirectory + "/" + currentName;
                wstring currentDirectoryW, currentNameW, currentPathW;
                StringUtility::Utf8ToUtf16(currentDirectory, currentDirectoryW);
                StringUtility::Utf8ToUtf16(currentName, currentNameW);
                StringUtility::Utf8ToUtf16(currentPath, currentPathW);

                if (dptr->d_type == DT_REG && GlobMatch(currentName.c_str(), filePatternA.c_str()))
                {
                    WIN32_FIND_DATA findData;
                    memset(&findData, 0, sizeof(findData));
                    memcpy(findData.cFileName, currentNameW.c_str(), (currentNameW.length() + 1) * sizeof(wchar_t));
                    findData.dwFileAttributes = FILE_ATTRIBUTE_REPARSE_POINT;
                    struct stat stat_data;
                    if(stat(currentPath.c_str(), &stat_data) == 0)
                    {
                        findData.ftCreationTime   = UnixTimeToFILETIME(stat_data.st_ctime, stat_data.st_ctim.tv_nsec);
                        findData.ftLastAccessTime = UnixTimeToFILETIME(stat_data.st_atime, stat_data.st_atim.tv_nsec);
                        findData.ftLastWriteTime  = UnixTimeToFILETIME(stat_data.st_mtime, stat_data.st_mtim.tv_nsec);
                        findData.nFileSizeLow     = (DWORD) stat_data.st_size;
                        findData.nFileSizeHigh    = (DWORD)(stat_data.st_size >> 32);

                        fileList.insert(make_pair(currentPathW, findData));
                    }

                    if(fileList.size() == expectedFileCount)
                    {
                        return fileList;
                    }
                }

                if (dptr->d_type == DT_DIR && findRecursively && strcmp(dptr->d_name, ".") && strcmp(dptr->d_name, ".."))
                {
                    directoriesToSearch.push(currentPath);
                }
            }
            closedir(dp);
        }
#else
        queue<wstring> directoriesToSearch;
        directoriesToSearch.push(path);

        while(!directoriesToSearch.empty())
        {
            wstring currentDirectory = directoriesToSearch.front();
            directoriesToSearch.pop();

            wstring fileSearchPattern = Path::Combine(currentDirectory, filePattern);
            auto findFiles = File::Search(move(fileSearchPattern));

            ErrorCode err;
            while ((err = findFiles->MoveNext()).IsSuccess())
            {
                auto fileProperties = findFiles->GetCurrent();
                auto currentName = &(fileProperties.cFileName[0]);

                if (!(findFiles->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY))
                {
                    fileList.insert(
                        make_pair(Path::Combine(currentDirectory, currentName), fileProperties));

                    if(fileList.size() == expectedFileCount)
                    {
                        return fileList;
                    }
                }
            }

            if(findRecursively)
            {
                wstring directorySearchPattern = Path::Combine(currentDirectory, L"*");
                auto findDirectories = File::Search(move(directorySearchPattern));

                ErrorCode err2;
                while ((err2 = findDirectories->MoveNext()).IsSuccess())
                {
                    auto currentName = &(findDirectories->GetCurrent().cFileName[0]);

                    if (((currentName[0] == L'.') && (currentName[1] == 0)) ||
                        ((currentName[0] == L'.') && (currentName[1] == L'.') && (currentName[2] == 0)))
                    {
                        // Skip ., ..
                        continue;
                    }

                    if (findDirectories->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        directoriesToSearch.push(Path::Combine(currentDirectory, currentName));
                    }
                }
            }
        }
#endif

        return fileList;
    }

    vector<wstring> Directory::GetFiles(wstring const & path)
    {
        return GetFiles(path, L"*", false, true);
    }

    vector<string> Directory::GetFiles(string const & path, string const & pattern, bool fullPath, bool topDirectoryOnly)
    {
        auto pathw = StringUtility::Utf8ToUtf16(path);
        auto patternw = StringUtility::Utf8ToUtf16(pattern);

        vector<wstring> resultw;
        if (topDirectoryOnly)
        {
            resultw = GetFilesInternal(pathw, patternw, fullPath);
        }
        else
        {
            GetFilesHelper(pathw, patternw, fullPath, resultw);
        }

        vector<string> result;
        result.reserve(resultw.size());
        for(auto const & ws : resultw)
        {
            string s;
            StringUtility::Utf16ToUtf8(ws, s);
            result.emplace_back(move(s));
        }

        return result;
    }

    vector<wstring> Directory::GetFiles(wstring const & path, wstring const & pattern, bool fullPath, bool topDirectoryOnly)
    {
        if (topDirectoryOnly)
        {
           return GetFilesInternal(path, pattern, fullPath);
        }

        vector<wstring> result;
        GetFilesHelper(path, pattern, fullPath, result);
        return result;
    }

    void Directory::GetFilesHelper(wstring const & path, wstring const & pattern, bool fullPath, vector<wstring> & result)
    {
        vector<wstring> files = GetFilesInternal(path, pattern, fullPath);
        result.insert(result.begin(), files.begin(), files.end());

        vector<wstring> subdirectories = GetSubDirectoriesInternal(path, L"*", true);
        for (auto it = subdirectories.begin(); it != subdirectories.end(); it++)
        {
            GetFilesHelper(*it, pattern, fullPath, result);
        }
    }

    vector<wstring> Directory::GetFilesInternal(wstring const & path, wstring const & pattern, bool fullPath)
    {
        vector<wstring> fileList;

        wstring fileSearchPattern = Path::Combine(path, pattern);
        auto findFiles = File::Search(move(fileSearchPattern));

        ErrorCode err;
        while ((err = findFiles->MoveNext()).IsSuccess())
        {
            auto fileProperties = findFiles->GetCurrent();
            auto currentName = &(fileProperties.cFileName[0]);            

            if (!(findFiles->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY))            
            {
                if (fullPath)
                {
                    fileList.push_back(Path::Combine(path, currentName));
                }
                else
                {
                    fileList.push_back(currentName);
                }
            }
        }

        return fileList;
    }

    vector<wstring> Directory::GetSubDirectories(wstring const & path)
    {
        return GetSubDirectories(path, L"*", false, true);
    }

    vector<wstring> Directory::GetSubDirectories(wstring const & path, wstring const & pattern, bool fullPath, bool topDirectoryOnly)
    {
        if (topDirectoryOnly)
        {
            return GetSubDirectoriesInternal(path, pattern, fullPath);
        }

        vector<wstring> result;
        GetSubDirectoriesHelper(path, pattern, fullPath, result);
        return result;
    }

    void Directory::GetSubDirectoriesHelper(wstring const & path, wstring const & pattern, bool fullPath, vector<wstring> & result)
    {
        vector<wstring> subdirectories = GetSubDirectoriesInternal(path, pattern, false);
        for (auto it = subdirectories.begin(); it != subdirectories.end(); it++) {
            wstring nextPath = Path::Combine(path, *it);
            if (fullPath)
            {
                result.push_back(nextPath);
            }
            else
            {
                result.push_back(*it);
            }
            GetSubDirectoriesHelper(nextPath, pattern, fullPath, result);
        }
    }

    vector<wstring> Directory::GetSubDirectoriesInternal(wstring const & path, wstring const & pattern, bool fullPath)
    {
        vector<wstring> subDirectoryList;

        wstring fileSearchPattern = Path::Combine(path, pattern);
        auto findFiles = File::Search(move(fileSearchPattern));

        ErrorCode err;
        while ((err = findFiles->MoveNext()).IsSuccess())
        {
            auto fileProperties = findFiles->GetCurrent();
            auto currentName = &(fileProperties.cFileName[0]);

            if (((currentName[0] == L'.') && (currentName[1] == 0)) ||
                ((currentName[0] == L'.') && (currentName[1] == L'.') && (currentName[2] == 0)))                        
            {
                // Skip ., ..
                continue;
            }

            if (findFiles->GetCurrentAttributes() & FILE_ATTRIBUTE_DIRECTORY)             
            {
                if (fullPath)
                {
                    subDirectoryList.push_back(Path::Combine(path, currentName));
                }
                else
                {
                    subDirectoryList.push_back(currentName);
                }
            }
        }

        return subDirectoryList;
    }

    bool Directory::HasFileLockExtension(std::wstring const & fileName)
    {
        std::wstring fileExtension = Path::GetExtension(fileName);
        return fileExtension == L".ReaderLock" || fileExtension == L".WriterLock";
    }

    ErrorCode Directory::GetLastWriteTime(std::wstring const & path, __out DateTime & lastWriteTime)
    {
        WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
        auto ntPath = Path::ConvertToNtPath(path);
        if (!::GetFileAttributesEx(ntPath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileAttributes))
        {
            Trace.WriteWarning(
                TraceSource,
                L"File.GetLastWriteTime",
                "GetFileAttributesEx failed with the following error {0}",
                ::GetLastError());
            return ErrorCode::FromHResult(HRESULT_FROM_WIN32(::GetLastError()));
        }

        lastWriteTime = DateTime(fileAttributes.ftLastWriteTime);
        Trace.WriteNoise(
            TraceSource,
            L"File.GetLastWriteTime",
            "GetFileAttributesEx got file time {0} and set last writetime to {1}",
            fileAttributes.ftLastWriteTime.dwLowDateTime,
            lastWriteTime);

        return ErrorCodeValue::Success;
    }

    bool Directory::IsSymbolicLink(wstring const & path)
    {
        if(Directory::Exists(path))
        {
            DWORD attribs = ::GetFileAttributes(path.c_str());
            if(attribs & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                return true;
            }
        }
        return false;
    }

    int64 Directory::GetSize(wstring const & path)
    {
        if (!Directory::Exists(path))
        {
            return -1;
        }

        vector<wstring> files = GetFiles(path, L"*", true, false);

        ErrorCode error(ErrorCodeValue::Success);
        __int64 accumulator = 0;
        __int64 size = 0;
        for (auto& file : files)
        {
            error = File::GetSize(file, size);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceSource,
                    L"GetDirectorySize",
                    "GetDirectorySize could not get size of file={0} error={1}. Cumulative size may be inaccurate.",
                    file,
                    error);

                continue;
            }

            accumulator += size;
        }

        return accumulator;
    }

#define DllImport __declspec( dllimport )
#define ERROR_MESSAGE_BUFFER_SIZE 2048

#if defined(PLATFORM_UNIX)
    int Directory::TryZipDirectory(
        wchar_t const * srcDirectoryPath,
        wchar_t const * destZipFileName,
        wchar_t * errorMessageBuffer,
        int errorMessageBufferSize)
    {
        return Zip::TryZipDirectory(srcDirectoryPath, destZipFileName, errorMessageBuffer, errorMessageBufferSize);
    }
#else
extern "C" DllImport int TryZipDirectory(
    wchar_t const * srcDirectoryPath, 
    wchar_t const * destZipFileName, 
    wchar_t * errorMessageBuffer, 
    int errorMessageBufferSize);
#endif

    ErrorCode Directory::CreateArchive(
        wstring const & src, 
        wstring const & dest)
    {
        wchar_t errMessageBuffer[ERROR_MESSAGE_BUFFER_SIZE];

        auto err = TryZipDirectory(
            src.c_str(),
            dest.c_str(),
            &errMessageBuffer[0], 
            ERROR_MESSAGE_BUFFER_SIZE);

        bool success = (err == 0);

        if (!success)
        {
            auto msg = wformatString(GET_COMMON_RC( Zip_Failed ), src, dest);
            msg.append(L"\n");
            msg.append(GET_COMMON_RC( Win_Long_Paths ));
            msg.append(L"\n");
            msg.append(wstring(move(&errMessageBuffer[0])));

            Trace.WriteWarning(TraceSource, "{0}", msg);

            return ErrorCode(ErrorCodeValue::OperationFailed, move(msg));
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

#if defined(PLATFORM_UNIX)
    int Directory::TryUnzipDirectory(
        wchar_t const * srcFileName,
        wchar_t const * destDirectoryPath,
        wchar_t * errorMessageBuffer,
        int errorMessageBufferSize)
    {
        return Zip::TryUnzipDirectory(srcFileName, destDirectoryPath, errorMessageBuffer, errorMessageBufferSize);
    }
#else
extern "C" DllImport int TryUnzipDirectory(
    wchar_t const * srcFileName,
    wchar_t const * destDirectoryPath,
    wchar_t * errorMessageBuffer,
    int errorMessageBufferSize);
#endif

    ErrorCode Directory::ExtractArchive(
        wstring const & src, 
        wstring const & dest) 
    {
        wchar_t errMessageBuffer[ERROR_MESSAGE_BUFFER_SIZE];

        auto err = TryUnzipDirectory(
            src.c_str(),
            dest.c_str(),
            &errMessageBuffer[0], 
            ERROR_MESSAGE_BUFFER_SIZE);

        bool success = (err == 0);

        if (!success)
        {
            auto msg = wformatString(GET_COMMON_RC( Unzip_Failed ), src, dest);
            msg.append(L"\n");
            msg.append(GET_COMMON_RC( Win_Long_Paths ));
            msg.append(L"\n");
            msg.append(wstring(move(&errMessageBuffer[0])));

            Trace.WriteWarning(TraceSource, "{0}", msg);

            return ErrorCode(ErrorCodeValue::OperationFailed, move(msg));
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }
}
