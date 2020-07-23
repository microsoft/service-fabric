// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class BinaryReaderWriterPerfTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        BinaryReaderWriterPerfTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~BinaryReaderWriterPerfTest()
        {
            ktlSystem_->Shutdown();
        }

        void KStringWritePerfTest(
            __in ULONG32 numberOfTasks,
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength,
            __in Encoding encoding);

        void KStringReadPerfTest(
            __in ULONG32 numberOfTasks, 
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength,
            __in Encoding encoding);

        void KUriWritePerfTest(
            __in ULONG32 numberOfTasks,
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength);

        void KUriReadPerfTest(
            __in ULONG32 numberOfTasks,
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength);

        void KStringAWritePerfTest(
            __in ULONG32 numberOfTasks,
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength);

        void KStringAReadPerfTest(
            __in ULONG32 numberOfTasks,
            __in ULONG32 numberOfOperationsPerTask,
            __in ULONG32 inputLength);

    private:
        Awaitable<KBuffer::SPtr> WriteAsync(
            __in KString const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs,
            __in Encoding encoding);

        Awaitable<void> ReadStringAsync(
            __in KBuffer const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs,
            __in Encoding encoding);

        Awaitable<KBuffer::SPtr> WriteAsync(
            __in KUri const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs);

        Awaitable<void> ReadUriAsync(
            __in KBuffer const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs);

        Awaitable<KBuffer::SPtr> WriteAsync(
            __in KStringA const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs);

        Awaitable<void> ReadStringAAsync(
            __in KBuffer const & input,
            __in ULONG32 numberOfOperationsPerTask,
            __in AwaitableCompletionSource<void> & acs);

        void Trace(
            __in ULONG32 count, 
            __in LONG64 duration,
            __in ULONG32 serailizedSize)
        {
            LONG64 tmpDuration = duration == 0 ? 1 : duration;

            cout << Common::formatString(
                "Result: Duration: {0} ms SerializedSize: {1} Throughput: {2} ops/ms",
                duration,
                serailizedSize,
                count / tmpDuration) << endl;

#if DBG
            cout << Common::formatString(
                "Result (DBG): Allocations: {0}",
                GetAllocator().GetTotalAllocations()) << endl;
#endif

            cout << endl;
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

    public:
        KtlSystem* ktlSystem_;
    };

    void BinaryReaderWriterPerfTest::KStringWritePerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength,
        __in Encoding encoding)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        string encodingString = encoding == UTF8 ? "UTF8" : "UTF16";
        cout << Common::formatString(
            "Type: KString, Encoding: {0} Operation: Write Test",
            encodingString) << endl;
        cout << Common::formatString(
            "Task Count : {0} Operations Per Task : {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KString::SPtr writeString = nullptr;
        status = KString::Create(
            writeString,
            allocator,
            L"fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        
        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = writeString->Concat(L"Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }
        CODING_ERROR_ASSERT(writeString->Length() == inputLength);

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<KBuffer::SPtr>> writeTasksArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            writeTasksArray.Append(WriteAsync(*writeString, numberOfOperationsPerTask, *acs, encoding));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<KBuffer::SPtr>::WhenAll(writeTasksArray));

        stopwatch.Stop();

        KBuffer::SPtr buffer(SyncAwait(writeTasksArray[0]));
        Trace(
            numberOfTasks * numberOfOperationsPerTask, 
            stopwatch.ElapsedMilliseconds, 
            buffer->QuerySize());
    }

    void BinaryReaderWriterPerfTest::KStringAWritePerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        cout << Common::formatString(
            "Type: KStringA, Operation: Write Test") << endl;
        cout << Common::formatString(
            "Task Count : {0} Operations Per Task : {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KStringA::SPtr writeString = nullptr;
        status = KStringA::Create(
            writeString,
            allocator,
            "fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = writeString->Concat("Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }
        CODING_ERROR_ASSERT(writeString->Length() == inputLength);

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<KBuffer::SPtr>> writeTasksArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            writeTasksArray.Append(WriteAsync(*writeString, numberOfOperationsPerTask, *acs));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<KBuffer::SPtr>::WhenAll(writeTasksArray));

        stopwatch.Stop();

        KBuffer::SPtr buffer(SyncAwait(writeTasksArray[0]));
        Trace(
            numberOfTasks * numberOfOperationsPerTask,
            stopwatch.ElapsedMilliseconds,
            buffer->QuerySize());
    }

    void BinaryReaderWriterPerfTest::KUriWritePerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        cout << Common::formatString(
            "Uri Write Test: Task Count: {0} Operations Per Task: {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KString::SPtr inputString = nullptr;
        status = KString::Create(
            inputString,
            allocator,
            L"fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = inputString->Concat(L"Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }
        CODING_ERROR_ASSERT(inputString->Length() == inputLength);

        KUri::SPtr writeUri = nullptr;
        status = KUri::Create(*inputString, allocator, writeUri);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<KBuffer::SPtr>> writeTasksArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            writeTasksArray.Append(WriteAsync(*writeUri, numberOfOperationsPerTask, *acs));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<KBuffer::SPtr>::WhenAll(writeTasksArray));

        stopwatch.Stop();

        KBuffer::SPtr buffer(SyncAwait(writeTasksArray[0]));
        Trace(
            numberOfTasks * numberOfOperationsPerTask,
            stopwatch.ElapsedMilliseconds,
            buffer->QuerySize());
    }

    void BinaryReaderWriterPerfTest::KStringReadPerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength,
        __in Encoding encoding)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        string encodingString = encoding == UTF8 ? "UTF8" : "UTF16";
        cout << Common::formatString(
            "Type: KString, Encoding: {0} Operation: Read Test", 
            encodingString) << endl;
        cout << Common::formatString(
            "Task Count : {0} Operations Per Task : {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KString::SPtr writeString = nullptr;
        status = KString::Create(
            writeString,
            allocator,
            L"fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = writeString->Concat(L"Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }

        CODING_ERROR_ASSERT(writeString->Length() == inputLength);

        BinaryWriter writer(allocator);
        writer.Write(*writeString, encoding);
        KBuffer::SPtr buffer = writer.GetBuffer(0, writer.Position);

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> readTaskArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            readTaskArray.Append(ReadStringAsync(*buffer, numberOfOperationsPerTask, *acs, encoding));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<void>::WhenAll(readTaskArray));
        stopwatch.Stop();

        Trace(
            numberOfTasks * numberOfOperationsPerTask,
            stopwatch.ElapsedMilliseconds,
            buffer->QuerySize());
    }

    void BinaryReaderWriterPerfTest::KStringAReadPerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        cout << "Type: KStringA, Operation: Read Test" << endl;
        cout << Common::formatString(
            "Task Count : {0} Operations Per Task : {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KStringA::SPtr writeString = nullptr;
        status = KStringA::Create(
            writeString,
            allocator,
            "fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = writeString->Concat("Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }

        CODING_ERROR_ASSERT(writeString->Length() == inputLength);

        BinaryWriter writer(allocator);
        writer.Write(*writeString);
        KBuffer::SPtr buffer = writer.GetBuffer(0, writer.Position);

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> readTaskArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            readTaskArray.Append(ReadStringAAsync(*buffer, numberOfOperationsPerTask, *acs));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<void>::WhenAll(readTaskArray));
        stopwatch.Stop();

        Trace(
            numberOfTasks * numberOfOperationsPerTask,
            stopwatch.ElapsedMilliseconds,
            buffer->QuerySize());
    }

    void BinaryReaderWriterPerfTest::KUriReadPerfTest(
        __in ULONG32 numberOfTasks,
        __in ULONG32 numberOfOperationsPerTask,
        __in ULONG32 inputLength)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        cout << Common::formatString(
            "Uri Read Test: Task Count: {0} Operations Per Task: {1} Total: {2} StringLength: {3}",
            numberOfTasks,
            numberOfOperationsPerTask,
            numberOfTasks * numberOfOperationsPerTask,
            inputLength) << endl;

        KAllocator& allocator = GetAllocator();
        KString::SPtr inputString = nullptr;
        status = KString::Create(
            inputString,
            allocator,
            L"fa:/mcoskunmcoskunmcoskunmcoskun");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < (inputLength - 32) / 32; i++)
        {
            BOOLEAN isAppended = inputString->Concat(L"Know thyself. Do it Now! 32 char");
            CODING_ERROR_ASSERT(isAppended == TRUE);
        }

        CODING_ERROR_ASSERT(inputString->Length() == inputLength);

        KUri::SPtr writeUri = nullptr;
        status = KUri::Create(*inputString, allocator, writeUri);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryWriter writer(allocator);
        writer.Write(*writeUri);
        KBuffer::SPtr buffer = writer.GetBuffer(0, writer.Position);

        AwaitableCompletionSource<void>::SPtr acs;
        status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> readTaskArray(allocator, numberOfTasks);
        for (ULONG32 i = 0; i < numberOfTasks; i++)
        {
            readTaskArray.Append(ReadUriAsync(*buffer, numberOfOperationsPerTask, *acs));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        acs->Set();

        SyncAwait(Data::Utilities::TaskUtilities<void>::WhenAll(readTaskArray));
        stopwatch.Stop();

        Trace(
            numberOfTasks * numberOfOperationsPerTask,
            stopwatch.ElapsedMilliseconds,
            buffer->QuerySize());
    }

    Awaitable<KBuffer::SPtr> BinaryReaderWriterPerfTest::WriteAsync(
        __in KString const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs,
        __in Encoding encoding)
    {
        BinaryWriter writer(GetAllocator());

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            writer.Position = 0;
            writer.Write(input, encoding);
        }

        co_return writer.GetBuffer(0, writer.Position);
    }

    Awaitable<KBuffer::SPtr> BinaryReaderWriterPerfTest::WriteAsync(
        __in KStringA const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs)
    {
        BinaryWriter writer(GetAllocator());

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            writer.Position = 0;
            writer.Write(input);
        }

        co_return writer.GetBuffer(0, writer.Position);
    }

    Awaitable<KBuffer::SPtr> BinaryReaderWriterPerfTest::WriteAsync(
        __in KUri const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs)
    {
        BinaryWriter writer(GetAllocator());

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            writer.Position = 0;
            writer.Write(input);
        }

        co_return writer.GetBuffer(0, writer.Position);
    }

    Awaitable<void> BinaryReaderWriterPerfTest::ReadStringAsync(
        __in KBuffer const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs,
        __in Encoding encoding)
    {
        BinaryReader reader(input, GetAllocator());
        KString::SPtr output;

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            reader.Position = 0;
            reader.Read(output, encoding);
        }

        co_return;
    }

    Awaitable<void> BinaryReaderWriterPerfTest::ReadStringAAsync(
        __in KBuffer const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs)
    {
        BinaryReader reader(input, GetAllocator());
        KStringA::SPtr output;

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            reader.Position = 0;
            reader.Read(output);
        }

        co_return;
    }

    Awaitable<void> BinaryReaderWriterPerfTest::ReadUriAsync(
        __in KBuffer const & input,
        __in ULONG32 numberOfOperationsPerTask,
        __in AwaitableCompletionSource<void> & acs)
    {
        BinaryReader reader(input, GetAllocator());
        KUri::SPtr output;

        co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        co_await acs.GetAwaitable();

        for (ULONG32 index = 0; index < numberOfOperationsPerTask; index++)
        {
            reader.Position = 0;
            reader.Read(output);
        }

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(BinaryReaderWriterPerfTestSuite, BinaryReaderWriterPerfTest);

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF8_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }

    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF8_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF8_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF16_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128 * 8;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }

    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF16_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16 * 8;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_String_Write_UTF16_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1 * 8;
        const ULONG32 stringSize = 1024;

        KStringWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_String_Read_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }

    BOOST_AUTO_TEST_CASE(Perf_String_Read_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_String_Read_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF8);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_String_Read_UTF16_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128 * 8;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }

    BOOST_AUTO_TEST_CASE(Perf_String_Read_UTF16_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16 * 8;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_String_Read_UTF16_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1 * 8;
        const ULONG32 stringSize = 1024;

        KStringReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize, Encoding::UTF16);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_StringA_Write_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128 * 8;
        const ULONG32 stringSize = 1024;

        KStringAWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

    BOOST_AUTO_TEST_CASE(Perf_StringA_Write_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16 * 8;
        const ULONG32 stringSize = 1024;

        KStringAWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_StringA_Write_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1 * 8;
        const ULONG32 stringSize = 1024;

        KStringAWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_StringA_Read_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128 * 8;
        const ULONG32 stringSize = 1024;

        KStringAReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

    BOOST_AUTO_TEST_CASE(Perf_StringA_Read_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16 * 8;
        const ULONG32 stringSize = 1024;

        KStringAReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_StringA_Read_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1 * 8;
        const ULONG32 stringSize = 1024;

        KStringAReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_Uri_Write_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128;
        const ULONG32 stringSize = 1024;

        KUriWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

    BOOST_AUTO_TEST_CASE(Perf_Uri_Write_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16;
        const ULONG32 stringSize = 1024;

        KUriWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_Uri_Write_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1;
        const ULONG32 stringSize = 1024;

        KUriWritePerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

#if !DBG
    BOOST_AUTO_TEST_CASE(Perf_Uri_Read_ASCII_1_131072_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 1;
        const ULONG32 numberOfOperationsPerTask = 512 * 128;
        const ULONG32 stringSize = 1024;

        KUriReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

    BOOST_AUTO_TEST_CASE(Perf_Uri_Read_ASCII_8_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 8;
        const ULONG32 numberOfOperationsPerTask = 512 * 16;
        const ULONG32 stringSize = 1024;

        KUriReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }
#endif

    BOOST_AUTO_TEST_CASE(Perf_Uri_Read_ASCII_128_1024_1024_SUCCESS)
    {
        const ULONG32 numberOfTasks = 128;
        const ULONG32 numberOfOperationsPerTask = 512 * 1;
        const ULONG32 stringSize = 1024;

        KUriReadPerfTest(numberOfTasks, numberOfOperationsPerTask, stringSize);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
