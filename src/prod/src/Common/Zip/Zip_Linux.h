// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if defined(PLATFORM_UNIX)
#pragma once

namespace Common
{
    class Zip
    {
        DENY_COPY(Zip)
    public:
        static int TryZipDirectory(wchar_t const * srcDirectoryPath, wchar_t const * destZipFileName, wchar_t * errorMessageBuffer, int errorMessageBufferSize);
        static int TryUnzipDirectory(wchar_t const * srcFileName, wchar_t const * destDirectoryPath, wchar_t * errorMessageBuffer, int errorMessageBufferSize);
        
    private:
        static bool IsLargeFile(std::istream & inputStream);
        static int ArchiveEntry(zipFile const & zipFile, std::string const & filename, std::string const & filenameFullPath, wchar_t * errorMessageBuffer, int const errorMessageBufferSize);
        static int ExtractCurrentEntry(unzFile const & zipFile, wchar_t const * destDirectoryPath, wchar_t * errorMessageBuffer, int const errorMessageBufferSize);
        static void GetFileDate(std::string filename, tm_zip & tmzip);
        static void ChangeFileDate(char const * filename, tm_unz tmuDate);
        static void CopyFromString(std::wstring const & str, wchar_t * out, int outSize);
    };
}
#endif
