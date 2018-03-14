// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'fsPT'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class FileStreamPerfTest
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        FileStreamPerfTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~FileStreamPerfTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG32 fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, GetAllocator());
            
            ULONG32* buffer = static_cast<ULONG32 *>(resultSPtr->GetBuffer());
            
            auto numElements = sizeInBytes / sizeof(ULONG32);
            for (ULONG32 i = 0; i < numElements; i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

        KString::SPtr CreateFileString(__in KStringView const & name)
        {
            KAllocator& allocator = GetAllocator();
            KString::SPtr fileName;

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        ktl::Awaitable<void> CreateFileStreamAsync(
            __in KStringView const & filename,
            __out ktl::io::KFileStream::SPtr& filestreamSPtr,
            __out KBlockFile::SPtr& fileSPtr)
        {
            KWString filePath(GetAllocator(), filename);

            auto createOptions = KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite;

            NTSTATUS status;
            //status = co_await KBlockFile::CreateSparseFileAsync(
            status = co_await KBlockFile::CreateAsync(
                filePath,
                FALSE,
                KBlockFile::eOpenAlways,
                static_cast<KBlockFile::CreateOptions>(createOptions),
                fileSPtr,
                nullptr,
                GetAllocator(),
                ALLOC_TAG);
            KInvariant(NT_SUCCESS(status));

            status = ktl::io::KFileStream::Create(filestreamSPtr, GetAllocator(), ALLOC_TAG);
            Diagnostics::Validate(status);

            status = co_await filestreamSPtr->OpenAsync(*fileSPtr);
            KInvariant(NT_SUCCESS(status));
        }

        ktl::Awaitable<void> CreateBlockFile(
            __in KStringView const & filename,
            __out KBlockFile::SPtr& fileSPtr)
        {
            KWString filePath(GetAllocator(), filename);

            auto createOptions = KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite;

            NTSTATUS status;
            //status = co_await KBlockFile::CreateSparseFileAsync(
            status = co_await KBlockFile::CreateAsync(
                filePath,
                FALSE,
                KBlockFile::eOpenAlways,
                static_cast<KBlockFile::CreateOptions>(createOptions),
                fileSPtr,
                nullptr,
                GetAllocator(),
                ALLOC_TAG);
            KInvariant(NT_SUCCESS(status));
        }

        ktl::Awaitable<void> CreateFileStreamAsync(
            __in KBlockFile & fileSPtr,
            __out ktl::io::KFileStream::SPtr& filestreamSPtr)
        {
            ktl::io::KFileStream::SPtr fs2;

            NTSTATUS status = ktl::io::KFileStream::Create(filestreamSPtr, GetAllocator(), ALLOC_TAG);
            Diagnostics::Validate(status);

            status = co_await filestreamSPtr->OpenAsync(fileSPtr);
            KInvariant(NT_SUCCESS(status));

            status = ktl::io::KFileStream::Create(fs2, GetAllocator(), ALLOC_TAG);
            Diagnostics::Validate(status);

            status = co_await fs2->OpenAsync(fileSPtr);
            KInvariant(NT_SUCCESS(status));
        }

        ktl::Awaitable<void> OpenCloseAsync(
            __in KStringView const & filename,
            __in ULONG32 count)
        {
            KString::SPtr filenameSPtr = nullptr;
            KString::Create(filenameSPtr, GetAllocator(), filename);

            KBlockFile::SPtr fileSPtr = nullptr;
            co_await CreateBlockFile(*filenameSPtr, fileSPtr);

            for (ULONG32 i = 0; i < count; i++)
            {
                ktl::io::KFileStream::SPtr streamSPtr = nullptr;
                ktl::io::KFileStream::Create(streamSPtr, GetAllocator(), ALLOC_TAG);

                ktl::io::KFileStream::SPtr streamSPtr1 = nullptr;
                ktl::io::KFileStream::Create(streamSPtr1, GetAllocator(), ALLOC_TAG);

                ktl::io::KFileStream::SPtr streamSPtr2 = nullptr;
                ktl::io::KFileStream::Create(streamSPtr2, GetAllocator(), ALLOC_TAG);

                auto status = co_await streamSPtr->OpenAsync(*fileSPtr);
                Diagnostics::Validate(status);
                status = co_await streamSPtr1->OpenAsync(*fileSPtr);
                Diagnostics::Validate(status);
                status = co_await streamSPtr2->OpenAsync(*fileSPtr);
                Diagnostics::Validate(status);
                co_await streamSPtr->CloseAsync();
                co_await streamSPtr1->CloseAsync();
                co_await streamSPtr2->CloseAsync();
            }

            fileSPtr->Close();
        }
        
        ktl::Awaitable<void> ConcurrentOpenCloseMultipleFilesAsync(
            __in KStringView const & filename,
            __in ULONG32 count,
            __in ULONG32 concurrency)
        {
            KString::SPtr filenameSPtr = nullptr;
            KString::Create(filenameSPtr, GetAllocator(), filename);

            for (ULONG32 i = 0; i < concurrency; i++)
            {
                auto filepath = CreateFileString(*filenameSPtr);
                filepath->Concat(to_wstring(i).c_str());
                // Open and close once to create file
                co_await OpenCloseAsync(*filepath, 1);
            }
            
            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            Common::Stopwatch stopwatch;
            stopwatch.Start();
            for (ULONG32 i = 0; i < concurrency; i++)
            {
                auto filepath = CreateFileString(*filenameSPtr);
                filepath->Concat(to_wstring(i).c_str());
                tasks->Append(OpenCloseAsync(*filepath, count / concurrency));
            }
            co_await StoreUtilities::WhenAll<void>(*tasks, GetAllocator());
            stopwatch.Stop();

            Trace.WriteInfo(
                BoostTestTrace,
                "OpenClose {0} times with {1} tasks: {2} ms",
                count,
                concurrency,
                stopwatch.ElapsedMilliseconds);
            
            // Putting cleanup code into separate method as compiler bug workaround
            // When contents of function are inlined, filepathToDelete->operator LPCWSTR()
            // causes an AV in its copy constructor in retail builds
            Cleanup(*filenameSPtr, concurrency);

            co_return;
        }

        void Cleanup(__in KStringView const & prefix, __in ULONG32 count)
        {
            for (ULONG32 i = 0; i < count; i++)
            {
                KString::SPtr filepathToDelete = CreateFileString(prefix);
                filepathToDelete->Concat(to_wstring(i).c_str());
                Common::File::Delete(filepathToDelete->operator LPCWSTR());
            }
        }

        ktl::Awaitable<void> ConcurrentOpenCloseSingleFileAsync(
            __in KStringView const & filename,
            __in ULONG32 count,
            __in ULONG32 concurrency)
        {
            auto filepath = CreateFileString(filename);
            // Open and close once to create file
            co_await OpenCloseAsync(*filepath, 1);
            
            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            Common::Stopwatch stopwatch;
            stopwatch.Start();
            for (ULONG32 i = 0; i < concurrency; i++)
            {
                tasks->Append(OpenCloseAsync(*filepath, count / concurrency));
            }
            co_await StoreUtilities::WhenAll<void>(*tasks, GetAllocator());
            stopwatch.Stop();

            Trace.WriteInfo(
                BoostTestTrace,
                "OpenClose {0} times with {1} tasks: {2} ms",
                count,
                concurrency,
                stopwatch.ElapsedMilliseconds);

            Common::File::Delete(filepath->operator LPCWSTR());

            co_return;
        }


        private:
            KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(FileStreamPerfTestSuite, FileStreamPerfTest)

    BOOST_AUTO_TEST_CASE(FileStream_OpenClose_1files_5000xEach_100readers)
    {
        // This test creates 100 tasks that each open and close a filestream 5000 times each
        // The aim of this test is to quantify the perf overhead of creating a filestream for each read
        // instead of pooling filestreams in a StreamPool

        SyncAwait(ConcurrentOpenCloseMultipleFilesAsync(L"FileStream_OpenClose_100files_5000xEach", 500000, 100));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
