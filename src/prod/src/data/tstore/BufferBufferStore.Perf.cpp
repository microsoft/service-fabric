// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'psBL'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

#define TRACE_MEMORY_DIFF(scenario) TRACE_MEMORY_DIFF_1(K_MAKE_UNIQUE_NAME(start), scenario)

#define TRACE_MEMORY_DIFF_1(start_name, scenario) \
    auto start_name = Diagnostics::GetProcessMemoryUsageBytes(); \
    KFinally([&] { Trace.WriteInfo("Test", scenario": {0} bytes. Size={1}", Diagnostics::GetProcessMemoryUsageBytes() - start_name, this->Store->Size);});

    class BufferBufferStorePerfTest : public TStorePerfTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        KBuffer::SPtr CreateKey(__in ULONG32 index) override
        {
            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 15) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;

            KBuffer::SPtr result = nullptr;
            KBuffer::CreateOrCopy(result, str.data(), static_cast<ULONG>(str.length() * sizeof(wchar_t)), GetAllocator());
            return result;
        }

        KBuffer::SPtr CreateValue(__in ULONG32 index) override
        {
            return CreateBuffer(500'000, index);
        }

        Awaitable<void> KeepAddingAsync(__in CancellationToken cancellationToken)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            ULONG32 keyIndex = 0;
            while (!cancellationToken.IsCancellationRequested)
            {
                KBuffer::SPtr key = CreateBuffer(8, keyIndex);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                keyIndex++;
            }
        }

        BufferBufferStorePerfTest()
        {
            Setup(1);
        }

        ~BufferBufferStorePerfTest()
        {
            Cleanup();
        }
    };

    BOOST_FIXTURE_TEST_SUITE(BufferBufferStorePerfSuite, BufferBufferStorePerfTest)

        BOOST_AUTO_TEST_CASE(CRUD_UniqueKeys_Perf, *boost::unit_test::label("perf-cit"))
    {
        CRUDPerf_UniqueKeys(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(AddMemoryUsage_5Minutes)
    {
        CancellationTokenSource::SPtr tokenSourceSPtr = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSourceSPtr);

        auto memoryTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(15), true, tokenSourceSPtr->Token);
        auto addTask = KeepAddingAsync(tokenSourceSPtr->Token);

        Trace.WriteInfo("Test", "Sleeping for 5 minutes");
        KNt::Sleep(300'000);

        tokenSourceSPtr->Cancel();

        SyncAwait(memoryTask);
        SyncAwait(addTask);
    }

    BOOST_AUTO_TEST_CASE(AddKey_MemoryUsage)
    {
        KBuffer::SPtr key = nullptr;

        Trace.WriteInfo("Test", "Start usage: {0}", Diagnostics::GetProcessMemoryUsageBytes());

        for (int i = 0; i < 1; i++)
        {
            {
                key = CreateBuffer(8, i);
            }

            {
                TRACE_MEMORY_DIFF("AddAsync");

                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr txn = nullptr;

                {
                    TRACE_MEMORY_DIFF("Creating store transaction");
                    txn = CreateWriteTransaction();
                }

                {
                    TRACE_MEMORY_DIFF("Add to writeset");
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None));
                }

                {
                    TRACE_MEMORY_DIFF("Apply");
                    SyncAwait(txn->CommitAsync());
                }
            }
            cout << endl;
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
