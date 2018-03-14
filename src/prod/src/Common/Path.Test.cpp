// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Path.h"

namespace Common
{
    class TestPaths
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestPathsSuite,TestPaths)

    BOOST_AUTO_TEST_CASE(ChangeExtensionTest)
    {
        std::wstring path;

        path = L"c:\\asdfsd\\pr.exe";
        Path::ChangeExtension(path, L".com");
        VERIFY_IS_TRUE(path == L"c:\\asdfsd\\pr.com");

        path = L"c:\\asdfsd\\pr.exe";
        Path::ChangeExtension(path, L"com");
        VERIFY_IS_TRUE(path == L"c:\\asdfsd\\pr.com");
    }

    BOOST_AUTO_TEST_CASE(GetDirectoryName)
    {
#if !defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(Path::GetDirectoryName(L"c:\\asdfs\\pr.exe") == L"c:\\asdfs");
        VERIFY_IS_TRUE(Path::GetDirectoryName(L"\\\\asdfs\\share\\pr.exe") == L"\\\\asdfs\\share");
#else
        VERIFY_IS_TRUE(Path::GetDirectoryName(L"/asdfs/pr.exe") == L"/asdfs");
        VERIFY_IS_TRUE(Path::GetDirectoryName(L"//asdfs/share/pr.exe") == L"//asdfs/share");
#endif
    }

    BOOST_AUTO_TEST_CASE(GetFileName)
    {
#if !defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(Path::GetFileName(L"c:\\asdfs\\pr.exe") == L"pr.exe");
        VERIFY_IS_TRUE(Path::GetFileName(L"\\\\asdfs\\share\\pr.exe") == L"pr.exe");

        VERIFY_IS_TRUE(Path::GetFileNameWithoutExtension(L"c:\\asdfs\\pr.exe") == L"pr");
        VERIFY_IS_TRUE(Path::GetFileNameWithoutExtension(L"\\\\asdfs\\share\\pr.exe") == L"pr");
#else
        VERIFY_IS_TRUE(Path::GetFileName(L"/asdfs/pr.exe") == L"pr.exe");
        VERIFY_IS_TRUE(Path::GetFileName(L"//asdfs/share/pr.exe") == L"pr.exe");

        VERIFY_IS_TRUE(Path::GetFileNameWithoutExtension(L"/asdfs/pr.exe") == L"pr");
        VERIFY_IS_TRUE(Path::GetFileNameWithoutExtension(L"//asdfs/share/pr.exe") == L"pr");
#endif
    }

    BOOST_AUTO_TEST_CASE(GetModuleFileName)
    {
        std::wstring path = Path::GetModuleLocation(NULL);

        WCHAR buffer[1024];
        VERIFY_IS_TRUE(::GetModuleFileName(NULL, buffer, ARRAYSIZE(buffer)) != 0);
        VERIFY_IS_TRUE(path == buffer);
    }

    BOOST_AUTO_TEST_CASE(GetFilePathInModuleLocation)
    {
        const std::wstring filename = L"hello.txt";
        std::wstring path = Path::GetModuleLocation(NULL);

        WCHAR buffer[1024];
        VERIFY_IS_TRUE(::GetModuleFileName(NULL, buffer, ARRAYSIZE(buffer)) != 0);
        ::PathRemoveFileSpec(buffer);

        WCHAR bufferCombined[1024];
        ::PathCombine(bufferCombined, buffer, filename.c_str());

        std::wstring pathInModuleLocation = Path::GetFilePathInModuleLocation(NULL, filename);
        VERIFY_IS_TRUE(pathInModuleLocation == bufferCombined);
    }

    BOOST_AUTO_TEST_CASE(ConvertToNtPathTest)
    {
#if !defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"D:\\path") == L"\\\\?\\D:\\path");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"\\\\server\\share") == L"\\\\?\\UNC\\server\\share");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"D:\\path\\\\abc") == L"\\\\?\\D:\\path\\abc");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"\\\\?\\D:\\path") == L"\\\\?\\D:\\path");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"path") == L"path");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"D:\\path\\..\\abc") == L"D:\\path\\..\\abc");
#else
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"//server/share") == L"//server/share");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"/path/abc") == L"/path/abc");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"path/abc") == L"path/abc");
        VERIFY_IS_TRUE(Path::ConvertToNtPath(L"path") == L"path");
#endif
    }

    BOOST_AUTO_TEST_CASE(GetFullPathTest)
    {
        std::wstring workingDirectory = L"test";
        std::wstring fullPath = Path::Combine(Directory::GetCurrentDirectory(), workingDirectory);
        VERIFY_IS_TRUE(Path::GetFullPath(workingDirectory) == fullPath);

#if !defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(Path::GetFullPath(L"c:\\test\\") == L"c:\\test");
        VERIFY_IS_TRUE(Path::GetFullPath(L"c:\\test\\\\") == L"c:\\test");
#else
        VERIFY_IS_TRUE(Path::GetFullPath(L"./test") == fullPath);
        VERIFY_IS_TRUE(Path::GetFullPath(L"test") == fullPath);
        VERIFY_IS_TRUE(Path::GetFullPath(L"/home/test") == L"/home/test");
#endif
    }

// Tests for apis implemented only on Windows
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(GetPathRootTest)
    {
        VERIFY_IS_TRUE(Path::GetPathRoot(L"c:") == L"c:");
        VERIFY_IS_TRUE(Path::GetPathRoot(L"d:\\") == L"d:");
        VERIFY_IS_TRUE(Path::GetPathRoot(L"e:\\test") == L"e:");
        VERIFY_IS_TRUE(Path::GetPathRoot(L"F:\\test\\") == L"F:");
    }

    BOOST_AUTO_TEST_CASE(GetDriveLetterTest)
    {
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"c:") == L'c');
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"d:\\") == L'd');
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"e:\\test") == L'e');
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"F:\\test\\") == L'F');
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"\\") == NULL);
        VERIFY_IS_TRUE(Path::GetDriveLetter(L"") == NULL);
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()
} // end namespace Common
