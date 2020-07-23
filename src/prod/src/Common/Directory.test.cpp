// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#if defined(PLATFORM_UNIX)
#include <sys/stat.h>
#endif

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
            int dirDepth,
            wstring const & fileExt = L"");

        void VerifyTestDirectoryExists(
            wstring const & rootDirName,
            wstring const & fileNamePrefix,
            wstring const & subDirPrefix,
            int fileCount,
            int subDirCount,
            int dirDepth,
            wstring const & fileExt = L"");

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

    BOOST_AUTO_TEST_CASE(GetSubDirectoriesTest)
    {
        Trace.WriteInfo(DirectoryTest::TestSource, "*** GetSubDirectoriesTest");

        auto testDir = Path::Combine(GetTestRootDir(), L"Directory.test");

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

        vector<wstring> subDirectories = Directory::GetSubDirectories(testDir, wformatString("*{0}*", "MySubDir"), true, true);
        auto subDirectoryCount = subDirectories.size();
        VERIFY_IS_TRUE(subDirectoryCount == subDirCount, wformatString("Number of subdirectories doesnot match. ExpecteCount:{0}, SubDirectoryCount:{1}", subDirectoryCount, subDirCount).c_str());

        vector<wstring> subDirectoryWithSinglePattern = Directory::GetSubDirectories(testDir, wformatString("*{0}", "MySubDir"), true, true);
        auto subDirectoryWithPatternCount = subDirectoryWithSinglePattern.size();
        VERIFY_IS_TRUE(subDirectoryWithPatternCount == 0, wformatString("Number of subdirectories doesnot match. ExpecteCount:{0}, SubDirectoryCount:{1}", subDirectoryWithPatternCount, 0).c_str());

        vector<wstring> singleSubDirectory = Directory::GetSubDirectories(testDir, wformatString("*{0}*", "MySubDir1"), true, true);
        auto singleSubDirectoryCount = singleSubDirectory.size();
        VERIFY_IS_TRUE(singleSubDirectoryCount == 1, wformatString("Number of subdirectories doesnot match. ExpecteCount:{0}, SubDirectoryCount:{1}", singleSubDirectoryCount, 1).c_str());

        vector<wstring> missingDirectory = Directory::GetSubDirectories(testDir, wformatString("*{0}*", "Foo"), true, true);
        auto missingDirectoryCount = missingDirectory.size();
        VERIFY_IS_TRUE(missingDirectoryCount == 0, wformatString("Number of subdirectories doesnot match. ExpecteCount:{0}, SubDirectoryCount:{1}", missingDirectoryCount, 0).c_str());

        vector<wstring> singleSubDirectoryWithSinglePattern = Directory::GetSubDirectories(testDir, wformatString("*{0}", "MySubDir1"), true, true);
        auto singleSubDirectoryWithPatternCount = singleSubDirectoryWithSinglePattern.size();
        VERIFY_IS_TRUE(singleSubDirectoryWithPatternCount == 1, wformatString("Number of subdirectories doesnot match. ExpecteCount:{0}, SubDirectoryCount:{1}", singleSubDirectoryWithPatternCount, 1).c_str());
    }

    BOOST_AUTO_TEST_CASE(GetFilesInDirectoriesTest)
    {
        Trace.WriteInfo(DirectoryTest::TestSource, "*** GetFilesInDirectoriesTest");

        auto testDir = Path::Combine(GetTestRootDir(), L"Directory.test");

        int fileCount = 5;
        int subDirCount = 5;
        int subDirDepth = 3;

        CreateTestDirectory(
            testDir,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth,
            L".xml");

        VerifyTestDirectoryExists(
            testDir,
            L"MyFile",
            L"MySubDir",
            fileCount,
            subDirCount,
            subDirDepth,
            L".xml");

        CreateTestDirectory(
            testDir,
            L"MyFile1",
            L"MySubDir",
            1,
            0,
            0,
            L".txt");

        // There is only 1 txt file in top directory. Serach all subdirectories should return only 1 file.
        vector<wstring> topDirectoriesTxtFiles = Directory::GetFiles(testDir, L"*.txt", true, false);
        Trace.WriteInfo("GetFilesTest", " txtFiles ExpectedCount:{0}, SubDirectoryCount:{1}", 1, topDirectoriesTxtFiles.size());
        VERIFY_IS_TRUE(1 == topDirectoriesTxtFiles.size(), wformatString("Number of files doesnot match. ExpecteCount:{0}, FilesCount:{1}", 1, topDirectoriesTxtFiles.size()).c_str());

        // Get the file with the absolute name in top directory
        vector<wstring> topDirectoriesXmlFile = Directory::GetFiles(testDir, L"MyFile0.xml", true, true);
        Trace.WriteInfo("GetFilesTest", " txtFiles ExpectedCount:{0}, SubDirectoryCount:{1}", 1, topDirectoriesXmlFile.size());
        VERIFY_IS_TRUE(1 == topDirectoriesXmlFile.size(), wformatString("Number of files doesnot match. ExpecteCount:{0}, FilesCount:{1}", 1, topDirectoriesXmlFile.size()).c_str());

        // All the subdirectories have xml files. Serach to top directory should result in only top directory files.
        vector<wstring> subDirectoriesFiles = Directory::GetFiles(testDir, L"*.xml", true, true);
        Trace.WriteInfo("GetFilesTest", "xmlFiles ExpectedCount:{0}, SubDirectoryCount:{1}", fileCount, subDirectoriesFiles.size());
        VERIFY_IS_TRUE(fileCount == subDirectoriesFiles.size(), wformatString("Number of files doesnot match. ExpecteCount:{0}, FilesCount:{1}", fileCount, subDirectoriesFiles.size()).c_str());
    }

#if defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(DirectoryPermissionsTest)
    {
        Trace.WriteInfo(DirectoryTest::TestSource, "*** DirectoryPermissionsTest");

        auto testDir = Path::Combine(GetTestRootDir(), L"Directory.test");

        int fileCount = 1;
        int subDirCount = 1;
        int subDirDepth = 0;

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

        string pathA = FileNormalizePath(Path::ConvertToNtPath(testDir).c_str());
        struct stat stat_data;
        VERIFY_IS_TRUE(stat(pathA.c_str(), &stat_data) == 0, wformatString("Unable to retreive the stat for the directory").c_str());
        int mode = stat_data.st_mode;
        // Permission should be 755
        VERIFY_IS_TRUE(mode & S_IRUSR, L"User reda permission not set");
        VERIFY_IS_TRUE(mode & S_IWUSR, L"User write permission not set");
        VERIFY_IS_TRUE(mode & S_IXUSR, L"User execute permission not set");
        VERIFY_IS_TRUE(mode & S_IRGRP, L"Group read permission not set");
        VERIFY_IS_TRUE(mode & S_IXGRP, L"Group execute permission not set");
        VERIFY_IS_TRUE(mode & ~S_IWGRP, L"Group write permission set");
        VERIFY_IS_TRUE(mode & S_IROTH, L"Others read permission not set");
        VERIFY_IS_TRUE(mode & S_IXOTH, L"Others read permission not set");
        VERIFY_IS_TRUE(mode & ~S_IWOTH, L"Others write permission set");

        auto path = Path::Combine(Directory::GetCurrentDirectory(), testDir);
        VERIFY_IS_TRUE(Directory::Exists(path), L"Directory doesn;t exist");
        auto error = File::AllowAccessToAll(path);
        Trace.WriteInfo("DirectoryPermissionsTest", "SetAttributes for {0} completed with {1}", testDir, error);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Unable to set permissions for the directory {0}", testDir).c_str());
        struct stat stat_data_1;
        VERIFY_IS_TRUE(stat(pathA.c_str(), &stat_data_1) == 0, wformatString("Unable to retreive the stat for the directory").c_str());
        int mode1 = stat_data_1.st_mode;
        // Permission should be 777
        VERIFY_IS_TRUE(mode1 & S_IRUSR, L"User reda permission not set");
        VERIFY_IS_TRUE(mode1 & S_IWUSR, L"User write permission not set");
        VERIFY_IS_TRUE(mode1 & S_IXUSR, L"User execute permission not set");
        VERIFY_IS_TRUE(mode1 & S_IRGRP, L"Group read permission not set");
        VERIFY_IS_TRUE(mode1 & S_IXGRP, L"Group execute permission not set");
        VERIFY_IS_TRUE(mode1 & S_IWGRP, L"Group write permission not set");
        VERIFY_IS_TRUE(mode1 & S_IROTH, L"Others read permission not set");
        VERIFY_IS_TRUE(mode1 & S_IXOTH, L"Others read permission not set");
        VERIFY_IS_TRUE(mode1 & S_IWOTH, L"Others write permission not set");
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()

    void DirectoryTest::CreateTestDirectory(
        wstring const & rootDirName,
        wstring const & fileNamePrefix,
        wstring const & subDirPrefix,
        int fileCount,
        int subDirCount,
        int dirDepth,
        wstring const & fileExt)
    {
        if (!Directory::Exists(rootDirName))
        {
            Directory::Create(rootDirName);
        }

        if (dirDepth < 0) { return; }

        for (int fx=0; fx<fileCount; ++fx)
        {
            auto fileName = Path::Combine(rootDirName, wformatString("{0}{1}{2}", fileNamePrefix, fx, fileExt));

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
                dirDepth - 1,
                fileExt);
        }
    }
    
    void DirectoryTest::VerifyTestDirectoryExists(
        wstring const & rootDirName,
        wstring const & fileNamePrefix,
        wstring const & subDirPrefix,
        int fileCount,
        int subDirCount,
        int dirDepth,
        wstring const & fileExt)
    {
        bool exists = Directory::Exists(rootDirName);

        VERIFY_IS_TRUE_FMT( exists, "Directory {0} exists={1}", rootDirName, exists );

        if (dirDepth < 0) { return; }

        for (int fx=0; fx<fileCount; ++fx)
        {
            auto fileName = Path::Combine(rootDirName, wformatString("{0}{1}{2}", fileNamePrefix, fx, fileExt));

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
                dirDepth - 1,
                fileExt);
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
