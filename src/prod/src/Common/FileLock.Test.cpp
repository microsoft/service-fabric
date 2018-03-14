// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral const TraceType("FileLockTest");

namespace Common
{
    using std::string;
    using std::wstring;
    using std::for_each;

    class AutoCleanedFile
    {
    public:
        AutoCleanedFile(std::wstring const & path) : path_(path) { }

        ~AutoCleanedFile()
        {
            File::Delete(this->path_);
        }

        bool Create()
        {
            auto folder = Path::GetDirectoryName(path_);
            if (!Directory::Exists(folder))
            {
                Directory::Create2(folder);
            }

            File file;
            auto error = file.TryOpen(this->path_, FileMode::OpenOrCreate, FileAccess::ReadWrite, FileShare::None);
            if (!error.IsSuccess())
            {
                return false;
            }
            int x = 64;
            file.Write(&x, sizeof(int));
            file.Close();
            return true;
        }

    private:
        std::wstring path_;
    };

    // Tests FileConfigStore
    // For simplicity, it reads the config content from strings
    // Config tests load the config content from files.
    class TestFileLock
    {
    };

    BOOST_FIXTURE_TEST_SUITE(FileLockTest,TestFileLock)

    BOOST_AUTO_TEST_CASE(TestReadLocksDontConflict)
    {
        FileReaderLock lock1(L"Test");
        VERIFY_IS_TRUE(lock1.Acquire().IsSuccess());
        FileReaderLock lock2(L"Test");
        VERIFY_IS_TRUE(lock2.Acquire().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(TestReadAndWriteLocksConflict)
    {
        FileReaderLock lock1(L"Test");
        VERIFY_IS_TRUE(lock1.Acquire().IsSuccess());
        FileWriterLock lock2(L"Test");
        VERIFY_IS_FALSE(lock2.Acquire().IsSuccess());
        lock1.Release();
        VERIFY_IS_TRUE(lock2.Acquire().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(TestWriteAndReadLocksConflict)
    {
        FileWriterLock lock1(L"Test");
        VERIFY_IS_TRUE(lock1.Acquire().IsSuccess());
        FileReaderLock lock2(L"Test");
        VERIFY_IS_FALSE(lock2.Acquire().IsSuccess());
        lock1.Release();
        VERIFY_IS_TRUE(lock2.Acquire().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(TestWriteAndWriteLocksConflict)
    {
        FileWriterLock lock1(L"Test");
        VERIFY_IS_TRUE(lock1.Acquire().IsSuccess());
        FileWriterLock lock2(L"Test");
        VERIFY_IS_FALSE(lock2.Acquire().IsSuccess());
        lock1.Release();
        VERIFY_IS_TRUE(lock2.Acquire().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(TestAbandondenedWriteLockAndReadLocksConflict)
    {
        wstring fileName(L"Test");

#ifdef PLATFORM_UNIX
        AutoCleanedFile file(FileWriterLock::Test_GetWriteInProgressMarkerPath(fileName));
#else
        AutoCleanedFile file(fileName + L".WriterLock");
#endif

        VERIFY_IS_TRUE(file.Create());
        FileReaderLock lock2(fileName);
        VERIFY_IS_FALSE(lock2.Acquire().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(AccessWhileHoldingWriteLock)
    {
        auto file = L"Test";
        FileWriterLock lock(file);
        VERIFY_ARE_EQUAL2(lock.Acquire().ReadValue(), ErrorCodeValue::Success);

        {
            File fileWriter;
            auto error = fileWriter.TryOpen(file, FileMode::OpenOrCreate, FileAccess::Write, FileShare::None);
            VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success); 
        }

        File fileReader;
        auto error = fileReader.TryOpen(file, FileMode::Open, FileAccess::Read, FileShare::Read);
        VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success); 
    }


    BOOST_AUTO_TEST_SUITE_END()
}
