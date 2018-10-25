// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if defined(PLATFORM_UNIX)
#include "../stdafx.h"

#include <utime.h>
#include <fstream>

#define WRITEBUFFERSIZE 8192

namespace Common
{
    using namespace std;

    int Zip::TryZipDirectory(
        wchar_t const * srcDirectoryPath,
        wchar_t const * destZipFileName,
        wchar_t * errorMessageBuffer,
        int errorMessageBufferSize)
    {
        zipFile zipFile = zipOpen64(StringUtility::Utf16ToUtf8(destZipFileName).c_str(), APPEND_STATUS_CREATE);
        if (zipFile == nullptr)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Zip_Failure), "zipOpen64", ""), errorMessageBuffer, errorMessageBufferSize);
            return ErrorCodeValue::OperationFailed;
        }

        wstring const currentPath = Directory::GetCurrentDirectory();
        Directory::SetCurrentDirectory(srcDirectoryPath);
        vector<wstring> allDirectories = Directory::GetSubDirectories(L".", L"*", true /* fullpath */, false);
        vector<wstring> allFiles = Directory::GetFiles(L".", L"*", true /* fullpath */, false);
        Directory::SetCurrentDirectory(currentPath);

        int err = ZIP_OK;

        for (auto directoryW : allDirectories)
        {
            if (directoryW.length() > 2 && directoryW.substr(0,2) == L"./")
            {
                directoryW = directoryW.substr(2, directoryW.length()-2);
            }
            string directoryName = StringUtility::Utf16ToUtf8(directoryW);
            string directoryNameFullPath = Path::Combine(StringUtility::Utf16ToUtf8(srcDirectoryPath), directoryName);

            // ensure directory ends with '/'
            if (directoryName[directoryName.length() -1] != '/')
            {
                directoryName.append("/");
            }

            err = Zip::ArchiveEntry(zipFile, directoryName, directoryNameFullPath, errorMessageBuffer, errorMessageBufferSize);
            if (err != ZIP_OK)
            {
                return err;
            }
        }

        for (auto filenameW : allFiles)
        {
            if (filenameW.length() > 2 && filenameW.substr(0,2) == L"./")
            {
                filenameW = filenameW.substr(2, filenameW.length() -2);
            }
            string filename = StringUtility::Utf16ToUtf8(filenameW);
            string filenameFullPath = Path::Combine(StringUtility::Utf16ToUtf8(srcDirectoryPath), filename);

            err = Zip::ArchiveEntry(zipFile, filename, filenameFullPath, errorMessageBuffer, errorMessageBufferSize);
            if (err != ZIP_OK)
            {
                return err;
            }
        }

        err = zipClose(zipFile, nullptr);

        return err;
    }

    int Zip::ArchiveEntry(
        zipFile const & zipFile,
        string const & filename,
        string const & filenameFullPath,
        wchar_t * errorMessageBuffer,
        int const errorMessageBufferSize)
    {
        ifstream input(filenameFullPath.c_str(), ios::binary);

        zip_fileinfo zi;

        zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
            zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
        zi.dosDate = 0;
        zi.internal_fa = 0;
        zi.external_fa = 0;
        Zip::GetFileDate(filenameFullPath, zi.tmz_date);

        int err = zipOpenNewFileInZip64(zipFile,
            filename.c_str(),
            &zi,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            Z_DEFLATED, // method
            1, //compress level (faster)
            static_cast<int>(IsLargeFile(input))); // zip64

        if (err != ZIP_OK)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Zip_Failure), "zipOpenNewFileInZip64", err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        size_t sizeRead;
        vector<char> buff;
        buff.resize(WRITEBUFFERSIZE);

        do
        {
            input.read(buff.data(), buff.size());
            sizeRead = static_cast<size_t>(input.gcount());

            if (sizeRead < buff.size() && !input.eof() && !input.good())
            {
                err = ZIP_ERRNO;
            }
            else if (sizeRead > 0)
            {
                err = zipWriteInFileInZip(zipFile, buff.data(), static_cast<unsigned int>(sizeRead));
            }
        }
        while (err == ZIP_OK && sizeRead > 0);

        if (err == ZIP_OK)
        {
            err = zipCloseFileInZip(zipFile);
        }
        else
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Zip_Failure), "zipWriteInFileInZip", err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        if (err != ZIP_OK)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Zip_Failure), "zipCloseFileInZip", err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        input.close();
        return err;
    }

    void Zip::GetFileDate(
        string filename,
        tm_zip & tmzip)
    {
        struct tm* filedate;
        time_t tm_t=0;

        DateTime lastWriteTime;
        File::GetLastWriteTime(StringUtility::Utf8ToUtf16(filename), lastWriteTime);

        ::SYSTEMTIME st;
        ::FILETIME ft = lastWriteTime.AsFileTime;
        DateTime::FileTimeToSystemTime(&ft, &st);
        
        tmzip.tm_sec  = st.wSecond; 
        tmzip.tm_min  = st.wMinute;
        tmzip.tm_hour = st.wHour;
        tmzip.tm_mday = st.wDay;
        tmzip.tm_mon  = st.wMonth;
        tmzip.tm_year = st.wYear;
    }

    bool Zip::IsLargeFile(istream& inputStream)
    {
        ZPOS64_T pos = 0;
        inputStream.seekg(0, std::ios::end);
        pos = inputStream.tellg();
        inputStream.seekg(0);

        return pos >= 0xffffffff;
    }

    int Zip::TryUnzipDirectory(
        wchar_t const * srcFileName,
        wchar_t const * destDirectoryPath,
        wchar_t * errorMessageBuffer,
        int errorMessageBufferSize)
    {
        unzFile zipFile = unzOpen64(StringUtility::Utf16ToUtf8(srcFileName).c_str());
        if (zipFile == nullptr)
        {
            Zip::CopyFromString(StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FILE_NOT_FOUND), errorMessageBuffer, errorMessageBufferSize);
            return ErrorCodeValue::FileNotFound;
        }

        int err = unzGoToFirstFile(zipFile);
        if (err != UNZ_OK)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), "unzGoToFirstFile", err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        err = Directory::Create2(destDirectoryPath).ReadValue();
        if (err != ErrorCodeValue::Success)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), wformatString("Directory::Create2 {0}", destDirectoryPath), err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        do
        {
            err = Zip::ExtractCurrentEntry(zipFile, destDirectoryPath, errorMessageBuffer, errorMessageBufferSize);
            if (err != UNZ_OK)
            {
                return err;
            }

            err = unzGoToNextFile(zipFile);
        }
        while (err == UNZ_OK);

        if (err==UNZ_END_OF_LIST_OF_FILE)
        {
            err = UNZ_OK;
        }

        return err;
    }

    int Zip::ExtractCurrentEntry(unzFile const & zipFile, wchar_t const * destDirectoryPath, wchar_t * errorMessageBuffer, int const errorMessageBufferSize)
    {
        // Linux has a maximum filename length of 255 characters for most filesystems (including EXT4)
        char filename_inzip[256];
        unz_file_info64 file_info;
        int err = unzGetCurrentFileInfo64(zipFile, &file_info, filename_inzip, 256, NULL, 0, NULL, 0);
        if (err != UNZ_OK)
        {
            Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), "unzGetCurrentFileInfo64", err), errorMessageBuffer, errorMessageBufferSize);
            return err;
        }

        string filenameStr = Path::Combine(StringUtility::Utf16ToUtf8(destDirectoryPath), filename_inzip);
        char filename[filenameStr.length() + 1];
        strncpy(filename, filenameStr.c_str(), filenameStr.length()+1);
        // Is a directory
        if (filename[strlen(filename) - 1] == '/')
        {
            err = Directory::Create2(filename).ReadValue();
        }
        // Is a file, extract it
        else
        {
            char* directoryEnding = strrchr(filename, '/');
            if (directoryEnding != nullptr)
            {
                char temp = *(directoryEnding + 1);
                *(directoryEnding + 1) = '\0';
                err =  Directory::Create2(filename).ReadValue();
                if (err != ErrorCodeValue::Success)
                {
                    Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), wformatString("Directory::Create2 {0}", filename), err), errorMessageBuffer, errorMessageBufferSize);
                    return err;
                }
                *(directoryEnding + 1) = temp;
            }

            err = unzOpenCurrentFile(zipFile);
            if (err != UNZ_OK)
            {
                Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), "unzOpenCurrentFile", err), errorMessageBuffer, errorMessageBufferSize);
                return err;

            }

            ofstream outputFile(filename, ofstream::binary);
            if (!outputFile.good())
            {
                Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), wformatString("open file: {0}", filename), ""), errorMessageBuffer, errorMessageBufferSize);
                return err;
            }

            vector<char> buffer;
            buffer.resize(WRITEBUFFERSIZE);

            do
            {
                err = unzReadCurrentFile(zipFile, buffer.data(), static_cast<unsigned int>(buffer.size()));

                if (err > 0)
                {
                    outputFile.write(buffer.data(), err);
                    if (!outputFile.good())
                    {
                        Zip::CopyFromString(wformatString(StringResource::Get(IDS_COMMON_UNIX_Unzip_Failure), "Write to file", ""), errorMessageBuffer, errorMessageBufferSize);
                        return err;
                    }
                }
            }
            while (err > 0);

            outputFile.flush();
            outputFile.close();

            Zip::ChangeFileDate(filename, file_info.tmu_date);
        }
        return err;
    }

    void Zip::CopyFromString(
        wstring const & str,
        wchar_t * out,
        int outSize)
    {
        auto temp = str.c_str();
        wcsncpy(out, temp, min(wcslen(temp) + 1, static_cast<size_t>(outSize)));
    }

    void Zip::ChangeFileDate(char const * filename, tm_unz tmuDate)
    {
        struct utimbuf ut;
        struct tm newDate;
        newDate.tm_sec = tmuDate.tm_sec;
        newDate.tm_min=tmuDate.tm_min;
        newDate.tm_hour=tmuDate.tm_hour;
        newDate.tm_mday=tmuDate.tm_mday;
        newDate.tm_mon=tmuDate.tm_mon;
        if (tmuDate.tm_year > 1900)
        {
            newDate.tm_year = tmuDate.tm_year - 1900;
        }
        else
        {
            newDate.tm_year = tmuDate.tm_year;
        }
        newDate.tm_isdst = -1;

        ut.actime = ut.modtime = mktime(&newDate);
        utime(filename,&ut);
    }
}

#endif
