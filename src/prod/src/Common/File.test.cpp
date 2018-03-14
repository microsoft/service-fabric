// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    static wstring const fprefix = L"FileTest-";

    class FileTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(FileTestsSuite, FileTests)

#if defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(CreateHardLink)
    {
            wstring fname = fprefix + Guid::NewGuid().ToString();
            File linkFrom;
            auto err = linkFrom.TryOpen(fname, FileMode::CreateNew);
            BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);

            wstring linkName = fprefix + Guid::NewGuid().ToString();
            auto linked = File::CreateHardLink(linkName, fname);
            BOOST_REQUIRE(linked);

            auto linkedAgain = File::CreateHardLink(linkName, fname);
            BOOST_REQUIRE(!linkedAgain);

            BOOST_REQUIRE(File::Exists(linkName));

            BOOST_REQUIRE_EQUAL(File::Delete2(fname).ReadValue(), ErrorCodeValue::Success);
            BOOST_REQUIRE(!File::Exists(fname));
            BOOST_REQUIRE(File::Exists(linkName));

            BOOST_REQUIRE_EQUAL(File::Delete2(linkName).ReadValue(), ErrorCodeValue::Success);
            BOOST_REQUIRE(!File::Exists(linkName));
    }

    BOOST_AUTO_TEST_CASE(FileAccessDiagnostics)
    {
        auto dstr = File::GetFileAccessDiagnostics("/tmp");
        RealConsole c;
        c.WriteLine("file access diagnostistics: {0}", dstr);
        BOOST_REQUIRE(!dstr.empty());
    }

#endif // defined(PLATFORM_UNIX)

    BOOST_AUTO_TEST_CASE(ReplaceFileNoBackup)
    {
        wstring guid = Guid::NewGuid().ToString();
        wstring original = fprefix + L"-original-" + guid;
        {
            File originalFile;
            auto err = originalFile.TryOpen(original, FileMode::CreateNew);
            BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);
            BOOST_REQUIRE_EQUAL(originalFile.size(), (int64)0);
        }

        wstring replacement = fprefix + L"-replacement-" + guid;
        char c = 'a';
        size_t dataSize = sizeof(c);
        {
            File replacementFile;
            auto err = replacementFile.TryOpen(replacement, FileMode::CreateNew, FileAccess::Write);
            BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);

            replacementFile.Write(&c, (int)dataSize);
            BOOST_REQUIRE_EQUAL(replacementFile.size(), (int64)dataSize);
        }

        auto err = File::Replace(original, replacement, L"", true);
        BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);

        BOOST_REQUIRE(!File::Exists(replacement));

        int64 newSize = 0;
        err = File::GetSize(original, newSize);
        BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);
        BOOST_REQUIRE_EQUAL(newSize, (int64)dataSize);

        BOOST_REQUIRE_EQUAL(File::Delete2(original).ReadValue(), ErrorCodeValue::Success);
    }

    BOOST_AUTO_TEST_CASE(ReplaceFileWithBackup)
    {
        wstring guid = Guid::NewGuid().ToString();
        wstring original = fprefix + L"-original-" + guid;
        int64 originalFileSize = 0;
        {
            File originalFile;
            auto err = originalFile.TryOpen(original, FileMode::CreateNew);
            BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);
            BOOST_REQUIRE_EQUAL(originalFile.size(), originalFileSize);
        }

        wstring replacement = fprefix + L"-replacement-" + guid;
        char c = 'a';
        size_t dataSize = sizeof(c);
        {
            File replacementFile;
            auto err = replacementFile.TryOpen(replacement, FileMode::CreateNew, FileAccess::Write);
            BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);

            replacementFile.Write(&c, (int)dataSize);
            BOOST_REQUIRE_EQUAL(replacementFile.size(), (int64)dataSize);
        }

        wstring backup = fprefix + L"-backup-" + guid;
        auto err = File::Replace(original, replacement, backup, true);
        BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);

        BOOST_REQUIRE(!File::Exists(replacement));

        int64 newSize = 0;
        err = File::GetSize(original, newSize);
        BOOST_REQUIRE_EQUAL(err.ReadValue(), ErrorCodeValue::Success);
        BOOST_REQUIRE_EQUAL(newSize, (int64)dataSize);

        BOOST_REQUIRE(File::Exists(backup));
        int64 backupFileSize = -1;
        err = File::GetSize(backup, backupFileSize);
        BOOST_REQUIRE_EQUAL(backupFileSize, originalFileSize);

        BOOST_REQUIRE_EQUAL(File::Delete2(original).ReadValue(), ErrorCodeValue::Success);
        BOOST_REQUIRE_EQUAL(File::Delete2(backup).ReadValue(), ErrorCodeValue::Success);
    }

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(FlushFileVolume)
    {
        // Find valid system drive
        WCHAR drive = NULL;
        wstring rootPath(L"");
        for (WCHAR testDrive = L'a'; testDrive <= L'z'; testDrive++)
        {
            rootPath = testDrive;
            rootPath.append(L":\\");
            if (Directory::Exists(rootPath))
            {
                drive = testDrive;
                break;
            }
        }
        VERIFY_IS_TRUE(drive != NULL);

        // Flush
        ErrorCode error(ErrorCodeValue::Success);
        error = File::FlushVolume(drive);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("FlushVolume failed against {0} drive with error {1}", drive, error).c_str());

        error = File::FlushVolume(NULL);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ArgumentNull), wformatString("FlushVolume did not receive correct errorcode {0} for null drive.", drive, error).c_str());

        error = File::FlushVolume(L'.');
        VERIFY_IS_TRUE(error.IsWin32Error(ERROR_FILE_NOT_FOUND), wformatString("FlushVolume did not receive correct errorcode {0} for incorrect volume.", drive, error).c_str());
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()
} // end namespace Common
