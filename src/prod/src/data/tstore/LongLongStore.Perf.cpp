// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'psLL'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class LongLongStorePerfTest : public TStorePerfTestBase<LONG64, LONG64, LongComparer, TestStateSerializer<LONG64>, TestStateSerializer<LONG64>>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        LONG64 CreateKey(__in ULONG32 index) override
        {
            return static_cast<LONG64>(index);
        }

        LONG64 CreateValue(__in ULONG32 index) override
        {
            return static_cast<LONG64>(index);
        }

        LongLongStorePerfTest()
        {
            Setup(1);
        }

        ~LongLongStorePerfTest()
        {
            Cleanup();
        }

        typedef KDelegate<ktl::Awaitable<void>(LONG64 &, LONG64 &, ktl::AwaitableCompletionSource<void> &, ktl::CancellationToken const)> ThroughputTestFunctionAsync;

        ktl::Awaitable<void> AddThroughputFunctionAsync(
            __in LONG64 & globalIndex,
            __in LONG64 & globalOpCount,
            __in ktl::AwaitableCompletionSource<void> & startSignal,
            __in ktl::CancellationToken const cancellationToken)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());
            //co_await startSignal.GetAwaitable();

            LONG64 localOpCount = 0;

            while (!cancellationToken.IsCancellationRequested)
            {
                LONG64 key = InterlockedIncrement64(&globalIndex);

                {
                    auto txn = CreateWriteTransaction();
                    co_await this->Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                    localOpCount++;
                }
            }

            // Doing this outside of main loop to decrease contention in critical path
            InterlockedAdd64(&globalOpCount, localOpCount);
        }

        void AddThroughputTest(__in Common::TimeSpan duration, __in ULONG numTasks)
        {
            ThroughputTestFunctionAsync addThrougputFunction;
            addThrougputFunction.Bind(this, &LongLongStorePerfTest::AddThroughputFunctionAsync);
            SyncAwait(ThroughputTestRunnerAsync(addThrougputFunction, duration, numTasks));
        }

        ktl::Awaitable<void> SingleKeyReadThroughputFunctionAsync(
            __in LONG64 & globalIndex,
            __in LONG64 & globalOpCount,
            __in ktl::AwaitableCompletionSource<void> & startSignal,
            __in ktl::CancellationToken const cancellationToken)
        {
            UNREFERENCED_PARAMETER(globalIndex);

            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            // Every thread uses the same key
            LONG64 key = 8;

            co_await startSignal.GetAwaitable();

            LONG64 localOpCount = 0;

            while (!cancellationToken.IsCancellationRequested)
            {
                {
                    KeyValuePair<LONG64, LONG64> value(-1, -1);
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, value, ktl::CancellationToken::None);
                    co_await txn->AbortAsync();
                    localOpCount++;
                }
            }

            // Doing this outside of main loop to decrease contention in critical path
            InterlockedAdd64(&globalOpCount, localOpCount);
        }

        void SingleKeyReadThroughputTest(__in Common::TimeSpan duration, __in ULONG numTasks)
        {
            LONG64 key = 8;

            {
                auto txn = CreateWriteTransaction();
                SyncAwait(this->Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            ThroughputTestFunctionAsync readThrougputFunction;
            readThrougputFunction.Bind(this, &LongLongStorePerfTest::SingleKeyReadThroughputFunctionAsync);
            SyncAwait(ThroughputTestRunnerAsync(readThrougputFunction, duration, numTasks));
        }

        ktl::Awaitable<void> UniqueKeyThroughputFunctionAsync(
            __in LONG64 & globalIndex,
            __in LONG64 & globalOpCount,
            __in ktl::AwaitableCompletionSource<void> & startSignal,
            __in ktl::CancellationToken const cancellationToken)
        {
            // Each thread gets a unique key
            LONG64 key = InterlockedIncrement64(&globalIndex);

            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            co_await startSignal.GetAwaitable();

            LONG64 numThreads = globalIndex; // Since each thread increments it once

            LONG64 localOpCount = 0;

            while (!cancellationToken.IsCancellationRequested)
            {
                {
                    KeyValuePair<LONG64, LONG64> value(-1, -1);
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, value, ktl::CancellationToken::None);
                    co_await txn->AbortAsync();
                }

                localOpCount++;
                key = (key + numThreads) % Store->Count;
            }

            // Doing this outside of main loop to decrease contention in critical path
            InterlockedAdd64(&globalOpCount, localOpCount);
        }

        void UniqueKeyReadThroughputTest(__in Common::TimeSpan duration, __in ULONG numKeys, __in ULONG numTasks)
        {
            for (LONG64 key = 0; key < numKeys; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }

            ThroughputTestFunctionAsync readThrougputFunction;
            readThrougputFunction.Bind(this, &LongLongStorePerfTest::UniqueKeyThroughputFunctionAsync);
            SyncAwait(ThroughputTestRunnerAsync(readThrougputFunction, duration, numTasks));
        }


        ktl::Awaitable<void> UpdateThroughputFunctionAsync(
            __in LONG64 & globalIndex,
            __in LONG64 & globalOpCount,
            __in ktl::AwaitableCompletionSource<void> & startSignal,
            __in ktl::CancellationToken const cancellationToken)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            // Each thread keeps updating its own key
            LONG64 key = InterlockedIncrement64(&globalIndex);

            co_await startSignal.GetAwaitable();

            LONG64 numThreads = globalIndex; // Since each thread increments it once

            LONG64 localOpCount = 0;

            while (!cancellationToken.IsCancellationRequested)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await this->Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                localOpCount++;
                key = (key + numThreads) % Store->Count;
            }

            // Doing this outside of main loop to decrease contention in critical path
            InterlockedAdd64(&globalOpCount, localOpCount);
        }

        void UpdateThroughputTest(__in Common::TimeSpan duration, __in ULONG numKeys, __in ULONG numTasks)
        {
            for (LONG64 key = 0; key < numKeys; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }

            ThroughputTestFunctionAsync updateThrougputFunction;
            updateThrougputFunction.Bind(this, &LongLongStorePerfTest::UpdateThroughputFunctionAsync);
            SyncAwait(ThroughputTestRunnerAsync(updateThrougputFunction, duration, numTasks));
        }
        
        ktl::Awaitable<void> AddReadRemoveReadAsync(
            __in LONG64 & globalIndex,
            __in LONG64 & globalOpCount,
            __in ktl::AwaitableCompletionSource<void> & startSignal,
            __in ktl::CancellationToken const cancellationToken)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());
            co_await startSignal.GetAwaitable();

            LONG64 localOpCount = 0;

            while (!cancellationToken.IsCancellationRequested)
            {
                LONG64 key = InterlockedIncrement64(&globalIndex);

                {
                    auto txn = CreateWriteTransaction();
                    co_await this->Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                    localOpCount++;
                }

                {
                    KeyValuePair<LONG64, LONG64> value(-1, -1);
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, value, ktl::CancellationToken::None);
                    co_await txn->AbortAsync();
                    localOpCount++;
                }

                {
                    auto txn = CreateWriteTransaction();
                    co_await this->Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                    localOpCount++;
                }

                {
                    KeyValuePair<LONG64, LONG64> value(-1, -1);
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, value, ktl::CancellationToken::None);
                    co_await txn->AbortAsync();
                    localOpCount++;
                }
            }

            // Doing this outside of main loop to decrease contention in critical path
            InterlockedAdd64(&globalOpCount, localOpCount);
        }

        void AddReadRemoveReadThroughputTest(__in Common::TimeSpan duration, __in ULONG numTasks)
        {
            ThroughputTestFunctionAsync addReadRemoveReadAsyncFunction;
            addReadRemoveReadAsyncFunction.Bind(this, &LongLongStorePerfTest::AddReadRemoveReadAsync);
            SyncAwait(ThroughputTestRunnerAsync(addReadRemoveReadAsyncFunction, duration, numTasks));
        }

        ktl::Awaitable<void> ThroughputTestRunnerAsync(
            __in ThroughputTestFunctionAsync testFunction,
            __in Common::TimeSpan duration,
            __in ULONG numTasks)
        {
            TRACE_TEST();
            ktl::AwaitableCompletionSource<void>::SPtr startSignal = nullptr;
            ktl::AwaitableCompletionSource<void>::Create(this->GetAllocator(), ALLOC_TAG, startSignal);

            ktl::CancellationTokenSource::SPtr tokenSource = nullptr;
            ktl::CancellationTokenSource::Create(this->GetAllocator(), ALLOC_TAG, tokenSource);

            Awaitable<void> memoryTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(15), false, tokenSource->Token);

            LONG64 globalIndex = 0;
            LONG64 globalOpCount = 0;

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(testFunction(globalIndex, globalOpCount, *startSignal, tokenSource->Token));
            }

            startSignal->Set();
            cout << "Sleeping for " << duration.TotalSeconds() << " seconds" << endl;

            // Offloading cancellation to Common Threadpool because current KTL 
            // threadpool may starve this coroutine from running
            // TODO: Revisit with new KTL threadpool
            Common::Threadpool::Post([&] {
                tokenSource->Cancel();
            }, duration);

            //co_await StoreUtilities::WhenAll(*tasks, this->GetAllocator());
            for (ULONG i = 0; i < numTasks; i++)
            {
                co_await (*tasks)[i];
            }
            
            Trace.WriteInfo(
                "Perf",
                "Test Throughput: {0} ops / {1} second = {2} ops/sec",
                globalOpCount,
                duration.TotalSeconds(),
                globalOpCount / duration.TotalSeconds());

            co_await memoryTask;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(LongLongStorePerfSuite, LongLongStorePerfTest, *boost::unit_test::label("perf-cit"))

    BOOST_AUTO_TEST_CASE(CRUD_UniqueKeys_Perf)
    {
        CRUDPerf_UniqueKeys(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf)
    {
        SingleKeyReadTest(1'000'000, 200, true, false);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf_1MKeys_200Txns_Consolidated)
    {
        SingleKeyReadTest(1'000'000, 200, true, false);
    }

    BOOST_AUTO_TEST_CASE(Add_Throughput_10Seconds)
    {
        AddThroughputTest(Common::TimeSpan::FromSeconds(10), 6);
    }

    BOOST_AUTO_TEST_CASE(Add_Throughput_1Minute)
    {
        AddThroughputTest(Common::TimeSpan::FromMinutes(1), 6);
    }

    BOOST_AUTO_TEST_CASE(Add_Throughput_3Minute)
    {
        AddThroughputTest(Common::TimeSpan::FromMinutes(3), 1000);
    }

    BOOST_AUTO_TEST_CASE(Add_Throughput_10Minute)
    {
        AddThroughputTest(Common::TimeSpan::FromMinutes(10), 6);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyRead_Throughput_10Seconds)
    {
        SingleKeyReadThroughputTest(Common::TimeSpan::FromSeconds(10), 6);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyRead_Throughput_1Minute)
    {
        SingleKeyReadThroughputTest(Common::TimeSpan::FromMinutes(1), 6);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyRead_Throughput_10Minute)
    {
        SingleKeyReadThroughputTest(Common::TimeSpan::FromMinutes(10), 6);
    }

    BOOST_AUTO_TEST_CASE(UniqueKeyRead_Throughput_10Seconds)
    {
        UniqueKeyReadThroughputTest(Common::TimeSpan::FromSeconds(10), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(UniqueKeyRead_Throughput_1Minute)
    {
        UniqueKeyReadThroughputTest(Common::TimeSpan::FromMinutes(1), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(UniqueKeyRead_Throughput_10Minute)
    {
        UniqueKeyReadThroughputTest(Common::TimeSpan::FromMinutes(10), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(Update_Throughput_10Seconds)
    {
        UpdateThroughputTest(Common::TimeSpan::FromSeconds(10), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(Update_Throughput_1Minute)
    {
        UpdateThroughputTest(Common::TimeSpan::FromMinutes(1), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(Update_Throughput_10Minute)
    {
        UpdateThroughputTest(Common::TimeSpan::FromMinutes(10), 1000, 6);
    }

    BOOST_AUTO_TEST_CASE(AddReadRemoveRead_Throughput_10Seconds)
    {
        AddReadRemoveReadThroughputTest(Common::TimeSpan::FromSeconds(10), 6);
    }

    BOOST_AUTO_TEST_CASE(AddReadRemoveRead_Throughput_1Minute)
    {
        AddReadRemoveReadThroughputTest(Common::TimeSpan::FromMinutes(1), 6);
    }

    BOOST_AUTO_TEST_CASE(AddReadRemoveRead_Throughput_10Minute)
    {
        AddReadRemoveReadThroughputTest(Common::TimeSpan::FromMinutes(10), 6);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
