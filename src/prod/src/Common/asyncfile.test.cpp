// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace std;

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Common/Common.h"

#include "Common/AssertWF.h"

Common::StringLiteral ReadFileAsyncTestTraceType("ReadFileAsyncOperationTest");

namespace Common
{
    class AsyncFileTest : public TextTraceComponent<TraceTaskCodes::Common>
    {
        public:

        wstring GetPath()
        {
            wchar_t tempPath[MAX_PATH];
            DWORD dwRetVal = ::GetCurrentDirectory(MAX_PATH, tempPath);
            ASSERT_IF(dwRetVal <= 0, "GetCurrentDirectory returned {0}", dwRetVal);

            return wstring(tempPath);
        }

        DWORD WriteTestFile(std::wstring const & filePath, BOOL isLargeFileTest)
        {
            // This is synchronous because FILE_FLAG_OVERLAPPED within the dwFlagsAndAttributes parameter is not set.
            HANDLE fileHandle = CreateFile(
                filePath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_DELETE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            HRESULT error = S_OK;

            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                error = GetLastError();
                error = HRESULT_FROM_WIN32(error);

                // Return from the test with error
                VERIFY_FAIL(L"File handle is invalid. Last error: {0}", error);
            }

            BOOL success = true;
            std::wstring DataBuffer = L"This is some test data to write to the file.";
            DWORD dwBytesToWrite = (DWORD)wcslen(DataBuffer.c_str())* sizeof(wchar_t);
            DWORD dwBytesWritten = 0;

            DWORD bytes = 0;
            DWORD totalBytes = 0;

            if (isLargeFileTest)
            {
                totalBytes = 1024 * 1024 * 10;
            }
            else
            {
                totalBytes = dwBytesToWrite;
            }

            while (bytes < totalBytes)
            {
                // Reset this to 0 so we don't get false readings to bytesWritten
                dwBytesWritten = 0;

                // fileHandle is synchronous, which means that WriteFile is synchronous.
                success = WriteFile(fileHandle, DataBuffer.c_str(), dwBytesToWrite, &dwBytesWritten, NULL);
                if (!success)
                {
                    // Gets the last error on the thread.
                    error = GetLastError();
                    WriteWarning(ReadFileAsyncTestTraceType, "Unable to write to file {0} with error: {1}", filePath, error);

                    // Retry writing if we were unable to write.
                    continue;
                }

                // This will make sure that the file is actually writing something.
                VERIFY_ARE_EQUAL(dwBytesWritten, dwBytesToWrite);
                bytes += dwBytesWritten;
            }

            WriteInfo(ReadFileAsyncTestTraceType, "Bytes Written : {0}", bytes);
            VERIFY_IS_TRUE(success);

            success = CloseHandle(fileHandle);

            VERIFY_IS_TRUE(success);

            return bytes;
        }

        void DeleteTestFile(std::wstring const & filePath)
        {
            BOOL success = DeleteFile(filePath.c_str());
            WriteInfo(ReadFileAsyncTestTraceType,"Deleting from path {0}", filePath.c_str());
            if (!success)
            {
                DWORD error = GetLastError();
                WriteError(ReadFileAsyncTestTraceType, "Writing Error:{0}", error);
            }

            VERIFY_IS_TRUE(success);
        }

        void ReadTestFile(std::wstring const & filePath, DWORD bytesWritten, TimeSpan timeout)
        {
            VERIFY_IS_FALSE(filePath.size() == 0);
            ByteBuffer buffer;
            ManualResetEvent requestCompleted(false);

            AsyncFile::BeginReadFile(
                filePath,
                timeout,
                [this, filePath, bytesWritten, &buffer, &requestCompleted](AsyncOperationSPtr const& operation)
                {
                    WriteInfo(ReadFileAsyncTestTraceType, "File Path:{0}", filePath);
                    auto error = AsyncFile::EndReadFile(operation, buffer);
                    VERIFY_IS_TRUE(error.IsSuccess());
                    requestCompleted.Set();
                },
                AsyncOperationSPtr());
            VERIFY_IS_TRUE(requestCompleted.WaitOne(timeout));

            // Verification and test clean up
            DWORD bytesRead = static_cast<DWORD>(buffer.size());
            WriteInfo(ReadFileAsyncTestTraceType, "Bytes Read :{0}", bytesRead); // BytesWritten is already printed in WriteTestFile
            VERIFY_ARE_EQUAL(bytesWritten, bytesRead);
            DeleteTestFile(filePath);
        }
    };

    BOOST_FIXTURE_TEST_SUITE(AsyncFileTestSuite, AsyncFileTest)

    BOOST_AUTO_TEST_CASE(ReadFileTest)
    {
        // Get Path
        wstring tempPath = AsyncFileTest::GetPath();

        // Concatenating Path with folder Name
        wstring path = Path::Combine(tempPath, L"ReadFileTest");

        VERIFY_IS_TRUE(Directory::Create2(path).IsSuccess());

        // Concatenating Temp Path with file Name
        path = Path::Combine(path, L"TestFile.txt");

        DWORD bytesWritten = WriteTestFile(path, false);
        ReadTestFile(path, bytesWritten, TimeSpan::MaxValue);
    }

    // BOOST_AUTO_TEST_CASE(ReadLargeFileTest)
    // {
        // // Get Path
        // wstring tempPath = AsyncFileTest::GetPath();

        // // Concatenating Path with folder Name
        // wstring path = Path::Combine(tempPath, L"ReadLargeFileTest");

        // VERIFY_IS_TRUE(Directory::Create2(path).IsSuccess());

        // // Concatenating Path with file Name
        // path = Path::Combine(path, L"TestLargeFile.txt");

        // DWORD bytesWritten = WriteTestFile(path, true);
        // ReadTestFile(path, bytesWritten, TimeSpan::MaxValue);
    // }

    BOOST_AUTO_TEST_SUITE_END()
}
