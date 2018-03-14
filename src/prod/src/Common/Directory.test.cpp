// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

Common::StringLiteral const TraceType("DirectoryTest");

namespace Common
{
    class DirectoryTest
    {
    protected:
        static StringLiteral const TestSource;
        static StringLiteral const TestRootDir;

        DirectoryTest() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP( MethodSetup );
        ~DirectoryTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP( MethodCleanup );

        void CreateTestDirectory(
            wstring const & rootDirName,
            wstring const & fileNamePrefix,
            wstring const & subDirPrefix,
            int fileCount,
            int subDirCount,
            int dirDepth);

        void VerifyTestDirectoryExists(
            wstring const & rootDirName,
            wstring const & fileNamePrefix,
            wstring const & subDirPrefix,
            int fileCount,
            int subDirCount,
            int dirDepth);

        void VerifyTestDirectoryNotExists(
            wstring const & rootDirName);

        void VerifyFileExists(
            wstring const & fileName);

        void VerifyFileNotExists(
            wstring const & fileName);

        static wstring GetTestRootDir();
    };

    StringLiteral const DirectoryTest::TestSource("DirectoryTest");
    StringLiteral const DirectoryTest::TestRootDir("DirectoryTest_Root");

    BOOST_FIXTURE_TEST_SUITE(DirectoryTestSuite,DirectoryTest)

    BOOST_AUTO_TEST_CASE(ArchiveTest)
    {
        Trace.WriteInfo(DirectoryTest::TestSource, "*** ArchiveTest");

        // Replace non-existent destination

        auto srcRoot = Path::Combine(GetTestRootDir(), L"ArchiveTestRoot");
        
        int fileCount = 5;
        int subDirCount = 5;
        int subDirDepth = 3;

        wstring src = Path::Combine(GetTestRootDir(), L"Directory.src");
        wstring zip = Path::Combine(GetTestRootDir(), L"Directory.zip");
        wstring zip2 = Path::Combine(GetTestRootDir(), L"Directory2.zip");
        wstring dest = Path::Combine(GetTestRootDir(), L"Directory.dest");

        CreateTestDirectory(
            src,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);

        VerifyTestDirectoryExists(
            src,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);

        VerifyTestDirectoryNotExists(dest);

        VerifyFileNotExists(zip);

        VerifyFileNotExists(zip2);

        auto error = Directory::CreateArchive(src, zip);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("CreateArchive failed: {0}", error).c_str());

        VerifyFileExists(zip);

        File srcFile;
        error = srcFile.TryOpen(zip, FileMode::Open, FileAccess::Read, FileShare::None, FileAttributes::Normal);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("TryOpen {0} failed: {1}", zip, error).c_str());

        File destFile;
        error = destFile.TryOpen(zip2, FileMode::CreateNew, FileAccess::Write, FileShare::None, FileAttributes::Normal);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("TryOpen {0} failed: {1}", zip2, error).c_str());

        size_t bufferSize = 64 * 1024;
        vector<byte> buffer;
        buffer.resize(bufferSize);

        DWORD bytesRead = 0;
        DWORD totalBytesRead = 0;

        do
        {
            error = srcFile.TryRead2(buffer.data(), static_cast<int>(buffer.size()), bytesRead);
            VERIFY_IS_TRUE(error.IsSuccess(), wformatString("TryRead2 failed: {0}", error).c_str());

            if (error.IsSuccess() && bytesRead > 0)
            {
                totalBytesRead += bytesRead;

                Trace.WriteInfo("ArchiveTest", "Read {0} bytes: total={1}", bytesRead, totalBytesRead);

                DWORD bytesWritten;
                error = destFile.TryWrite2(buffer.data(), bytesRead, bytesWritten);
                VERIFY_IS_TRUE(error.IsSuccess() && bytesWritten == bytesRead, wformatString("TryRead2 failed: {0} read={1} written={2}", error, bytesRead, bytesWritten).c_str());
            }
            else
            {
                Trace.WriteInfo("ArchiveTest", "Read failed bytes: error={0} read={1} total={2}", error, bytesRead, totalBytesRead);
            }

        } while (bytesRead > 0);

        error = srcFile.Close2();
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Close2 {0} failed: {1}", zip, error).c_str());

        error = destFile.Close2();
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Close2 {0} failed: {1}", zip2, error).c_str());

        VerifyFileExists(zip2);

        error = Directory::ExtractArchive(zip2, dest);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("ExtractArchive failed: {0}", error).c_str());

        VerifyTestDirectoryExists(
            dest,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);

        VerifyTestDirectoryExists(
            src,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);
    }

    BOOST_AUTO_TEST_CASE(DeleteTest)
    {
        Trace.WriteInfo(DirectoryTest::TestSource, "*** DeleteTest");

        // Replace non-existent destination

        auto testDir = Path::Combine(GetTestRootDir(), L"Directory.test");
        auto missingDir = Path::Combine(GetTestRootDir(), L"Directory.missing");

        int fileCount = 5;
        int subDirCount = 5;
        int subDirDepth = 3;

        CreateTestDirectory(
            testDir,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);

        VerifyTestDirectoryExists(
            testDir,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth);

        VerifyTestDirectoryNotExists(missingDir);

        // Happy path
        auto error = Directory::Delete(testDir, true);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Delete failed: {0}", error).c_str());

        error = Directory::Delete(missingDir, true);
        VERIFY_IS_TRUE(
            error.ReadValue() == ErrorCodeValue::DirectoryNotFound, 
            wformatString("Delete returned wrong ErrorCode: {0}, expected {1}", error, ErrorCodeValue::DirectoryNotFound).c_str());
    }

    BOOST_AUTO_TEST_SUITE_END()

    void DirectoryTest::CreateTestDirectory(
        wstring const & rootDirName,
        wstring const & fileNamePrefix,
        wstring const & subDirPrefix,
        int fileCount,
        int subDirCount,
        int dirDepth)
    {
        Directory::Create(rootDirName);

        if (dirDepth < 0) { return; }

        for (int fx=0; fx<fileCount; ++fx)
        {
            auto fileName = Path::Combine(rootDirName, wformatString("{0}{1}", fileNamePrefix, fx));

            File file;
            auto error = file.TryOpen(fileName, FileMode::OpenOrCreate, FileAccess::Write);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "TryOpen({0}) error={1}", fileName, error);

            file.Write(
                reinterpret_cast<const void*>(fileName.c_str()), 
                static_cast<int>(fileName.size() * sizeof(wchar_t)));
            file.Flush();
            file.Close();
        }

        for (int dx=0; dx<subDirCount; ++dx)
        {
            auto subDirName = Path::Combine(rootDirName, wformatString("{0}{1}", subDirPrefix, dx));

            CreateTestDirectory(
                subDirName,
                fileNamePrefix,
                subDirPrefix,
                fileCount,
                subDirCount,
                dirDepth - 1);
        }
    }
    
    void DirectoryTest::VerifyTestDirectoryExists(
        wstring const & rootDirName,
        wstring const & fileNamePrefix,
        wstring const & subDirPrefix,
        int fileCount,
        int subDirCount,
        int dirDepth)
    {
        bool exists = Directory::Exists(rootDirName);

        VERIFY_IS_TRUE_FMT( exists, "Directory {0} exists={1}", rootDirName, exists );

        if (dirDepth < 0) { return; }

        for (int fx=0; fx<fileCount; ++fx)
        {
            auto fileName = Path::Combine(rootDirName, wformatString("{0}{1}", fileNamePrefix, fx));

            exists = File::Exists(fileName);

            VERIFY_IS_TRUE_FMT( exists, "File {0} exists={1}", fileName, exists );
        }

        for (int dx=0; dx<subDirCount; ++dx)
        {
            auto subDirName = Path::Combine(rootDirName, wformatString("{0}{1}", subDirPrefix, dx));

            VerifyTestDirectoryExists(
                subDirName,
                fileNamePrefix,
                subDirPrefix,
                fileCount,
                subDirCount,
                dirDepth - 1);
        }
    }

    void DirectoryTest::VerifyTestDirectoryNotExists(
        wstring const & rootDirName)
    {
        bool exists = Directory::Exists(rootDirName);

        VERIFY_IS_TRUE_FMT( !exists, "Directory {0} exists={1}", rootDirName, exists );
    }

    void DirectoryTest::VerifyFileExists(
        wstring const & fileName)
    {
        bool exists = File::Exists(fileName);

        VERIFY_IS_TRUE_FMT( exists, "File {0} exists={1}", fileName, exists );
    }

    void DirectoryTest::VerifyFileNotExists(
        wstring const & fileName)
    {
        bool exists = File::Exists(fileName);

        VERIFY_IS_TRUE_FMT( !exists, "File {0} exists={1}", fileName, exists );
    }

    wstring DirectoryTest::GetTestRootDir()
    {
        return wformatString("{0}", TestRootDir);
    }

    bool DirectoryTest::MethodSetup()
    {
        Config cfg;

        if (Directory::Exists(GetTestRootDir()))
        {
            Directory::Delete(GetTestRootDir(), true);
        }

        Directory::Create(GetTestRootDir());

        return true;
    }

    bool DirectoryTest::MethodCleanup()
    {
        if (Directory::Exists(GetTestRootDir()))
        {
            Directory::Delete(GetTestRootDir(), true);
        }

        return true;
    }
}
