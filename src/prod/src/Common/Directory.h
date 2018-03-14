// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ErrorCode;

    class Directory
    {
        DENY_COPY(Directory);
    public:
        static inline void CreateDirectory(
            std::wstring const & path)
        {
            Directory::Create(path);
        }

        static void Create(
            std::wstring const & path);

        static ErrorCode Create2(std::string const & path);
        static ErrorCode Create2(std::wstring const & path);

        static ErrorCode Delete(
            std::wstring const & path,
            bool recursive);

        static ErrorCode Delete(
            std::wstring const & path,
            bool recursive,
            bool deleteReadOnlyFiles);

        static ErrorCode Delete_WithRetry(
            std::wstring const & path, 
            bool recursive, 
            bool deleteReadOnlyFiles);

        static bool Exists(
            std::string const & path);
        static bool Exists(
            std::wstring const & path);

        static ErrorCode Exists(
            std::wstring const & path,
            __out bool& exists);

        static std::wstring GetCurrentDirectory();

        static void SetCurrentDirectory(
            std::wstring const & path);

        static ErrorCode Copy(std::wstring const & src, std::wstring const & dest, bool overWrite);

        static ErrorCode SafeCopy(
            std::wstring const & src,
            std::wstring const & dest,
            bool overWrite,
            Common::TimeSpan const timeout = TimeSpan::Zero);

        static ErrorCode Rename(std::wstring const & src, std::wstring const & dest, bool const overwrite = false);

        static ErrorCode Rename_WithRetry(std::wstring const & src, std::wstring const & dest, bool const overwrite = false);

        static std::vector<std::wstring> GetSubDirectories(std::wstring const & path);
        static std::vector<std::wstring> GetSubDirectories(std::wstring const & path, std::wstring const & pattern, bool fullPath, bool topDirectoryOnly);

        static std::vector<std::wstring> GetFiles(std::wstring const & path);
        static std::vector<std::string> GetFiles(std::string const & path, std::string const & pattern, bool fullPath, bool topDirectoryOnly);
        static std::vector<std::wstring> GetFiles(std::wstring const & path, std::wstring const & pattern, bool fullPath, bool topDirectoryOnly);

        static std::vector<std::wstring> Find(std::wstring const & path, std::wstring const & filePattern, uint expectedFileCount, bool findRecursively);
        static std::map<std::wstring, WIN32_FIND_DATA> FindWithFileProperties(std::wstring const & path, std::wstring const & filePattern, uint expectedFileCount, bool findRecursively);

        static ErrorCode GetLastWriteTime(std::wstring const & path, __out DateTime & lastWriteTime);

        static ErrorCode Echo(std::wstring const & src, std::wstring const & dest, Common::TimeSpan const timeout = TimeSpan::Zero);

        static bool IsSymbolicLink(std::wstring const & path);

        static int64 GetSize(std::wstring const & path);

        static ErrorCode CreateArchive(std::wstring const & src, std::wstring const & dest);
        static ErrorCode ExtractArchive(std::wstring const & src, std::wstring const & dest);

    private:
        Directory() { } // deny construction

#if defined(PLATFORM_UNIX)
        static int TryZipDirectory(wchar_t const * srcDirectoryPath, wchar_t const * destZipFileName, wchar_t * errorMessageBuffer, int errorMessageBufferSize);
        static int TryUnzipDirectory(wchar_t const * srcFileName, wchar_t const * destDirectoryPath, wchar_t * errorMessageBuffer, int errorMessageBufferSize);
#endif

        static ErrorCode DeleteInternal(
            std::wstring const & path,
            bool recursive,
            bool deleteReadOnlyFiles);

        static ErrorCode MoveFile(const std::wstring & src, const std::wstring & dest, bool overwrite);

        static bool HasFileLockExtension(std::wstring const & fileName);

        static std::vector<std::wstring> GetSubDirectoriesInternal(std::wstring const & path, std::wstring const & pattern, bool fullPath);
        static std::vector<std::wstring> GetFilesInternal(std::wstring const & path, std::wstring const & pattern, bool fullPath);

        static void GetSubDirectoriesHelper(std::wstring const & path, std::wstring const & pattern, bool fullPath, std::vector<std::wstring> & result);
        static void GetFilesHelper(std::wstring const & path, std::wstring const & pattern, bool fullPath, std::vector<std::wstring> & result);

        static const int DirectoryOperationMaxRetryCount;
        static const int DirectoryOperationRetryIntervalInMillis;
    };
}
