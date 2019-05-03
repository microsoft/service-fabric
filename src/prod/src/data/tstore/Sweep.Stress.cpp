// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'ttSS'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Common;

    class SweepStressTest : public TStoreTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        SweepStressTest()
        {
            Setup(1);
        }

        ~SweepStressTest()
        {
            Cleanup();
        }

        Random GetRandom()
        {
            auto seed = Stopwatch::Now().Ticks;
            Random random(static_cast<int>(seed));
            //cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            Trace.WriteInfo("Test", "Random seed (use this seed to reproduce test failures): {0}", seed);
            return random;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG32 fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, this->GetAllocator());

            ULONG32* buffer = static_cast<ULONG32 *>(resultSPtr->GetBuffer());

            for (ULONG32 i = 0; i < sizeInBytes / sizeof(ULONG32); i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

        Awaitable<void> AddKeysAsync(
            __in ULONG32 startKey,
            __in ULONG32 endKey,
            __in ULONG32 valueSize)
        {
            KBuffer::SPtr value = CreateBuffer(valueSize, 0xCA);

            for (ULONG32 i = startKey; i < endKey; i++)
            {
                if (i % 1000 == 0) cout << ".";
                KBuffer::SPtr key = CreateBuffer(32, i);
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            cout << endl;
        }

        Awaitable<void> ReadRandomlyUntilCancelled(__in CancellationToken token)
        {
            Random random = GetRandom();

            // Using random reads to reduce chance of overcounting from concurrent loads

            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());

            while (true)
            {
                // Choose a random replica to read from, weighted to have vastly different cache sizes
                ULONG32 storeChoice = random.Next(0, static_cast<int>(Stores->Count()));
                ULONG32 storeIndex = 0;
                if (storeChoice == 2)
                {
                    storeIndex = 1;
                }
                else if (storeChoice == 3)
                {
                    storeIndex = 2;
                }
                else
                {
                    storeIndex = 0; // Primary
                }

                auto store = (*Stores)[storeIndex];

                ULONG32 choice = random.Next(0, 50'000);


                {
                    auto txn = CreateWriteTransaction(*store);
                    KBuffer::SPtr key = CreateBuffer(32, choice);

                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    co_await store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, value, CancellationToken::None);
                    co_await txn->AbortAsync();
                }

                if (token.IsCancellationRequested)
                {
                    Trace.WriteInfo("Test", "Stopping readers");
                    co_return;
                }
            }
        }

        Awaitable<void> Sweep_WithMultipleReaders_TestAsync(__in bool threeReplicas)
        {
            CancellationTokenSource::SPtr memoryTokenSource = nullptr;
            CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, memoryTokenSource);

            Awaitable<void> traceTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(15), false, memoryTokenSource->Token);

            Store->EnableSweep = true;
            Store->SweepManagerSPtr->TimeoutInMS = 60 * 1'000;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr firstSecondary = nullptr;
            Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr secondSecondary = nullptr;

            if (threeReplicas)
            {
                firstSecondary = CreateSecondary();
                firstSecondary->EnableSweep = true;
                firstSecondary->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
                firstSecondary->SweepManagerSPtr->TimeoutInMS = 60 * 1'000;

                secondSecondary = CreateSecondary();
                secondSecondary->EnableSweep = true;
                secondSecondary->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
                secondSecondary->SweepManagerSPtr->TimeoutInMS = 60 * 1'000;
            }

            co_await AddKeysAsync(0,20'000, 256*1024);
            co_await CheckpointAsync();

            LONG64 startSize = Store->Size;

            CancellationTokenSource::SPtr tokenSource = nullptr;
            CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSource);

            KSharedArray<Awaitable<void>>::SPtr readTasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

            // Start a lot of readers to ensure sweep can't catch up
            for (ULONG32 i = 0; i < 12; i++)
            {
                readTasks->Append(ReadRandomlyUntilCancelled(tokenSource->Token));
            }

            Store->StartBackgroundSweep();
            if (threeReplicas)
            {
                firstSecondary->StartBackgroundSweep();
                secondSecondary->StartBackgroundSweep();
            }

            // Run bulk of test
            cout << "Sleeping for 4 minutes" << endl;
            Common::Threadpool::Post([&] {
                tokenSource->Cancel();
            }, Common::TimeSpan::FromMinutes(4));

            co_await TaskUtilities<void>::WhenAll(*readTasks);

            // Stop readers, and let sweep run again to clean up everything
            cout << "Sleeping for 2 minutes" << endl;
            co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, static_cast<ULONG>(Common::TimeSpan::FromMinutes(2).TotalMilliseconds()), nullptr);

            LONG64 endSize = Store->Size;

            CODING_ERROR_ASSERT(endSize < startSize);

            memoryTokenSource->Cancel();
            co_await traceTask;
        }

        LONG64 GetSerializedSize(__in KBuffer & buffer)
        {
            BinaryWriter writer(GetAllocator());
            KBuffer::SPtr bufferSPtr = &buffer;
            Store->ValueConverterSPtr->Write(bufferSPtr, writer);
            return writer.GetBuffer(0)->QuerySize();
        }

    private:
        CommonConfig config; // load the config object as it's needed for the tracking to work
    };

    BOOST_FIXTURE_TEST_SUITE(SweepStressTestSuite, SweepStressTest)

    BOOST_AUTO_TEST_CASE(Sweep_WithMultipleReaders_1Replica_ShouldSucceed)
    {
        SyncAwait(Sweep_WithMultipleReaders_TestAsync(false));
    }

    BOOST_AUTO_TEST_CASE(Sweep_WithMultipleReaders_3Replica_ShouldSucceed)
    {
        SyncAwait(Sweep_WithMultipleReaders_TestAsync(true));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
