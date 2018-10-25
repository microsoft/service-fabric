// ----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReliableConcurrentQueue.h"
#include "ReliableConcurrentQueue.TestBase.h"
#include "../tstore/TStoreTestBase.h"

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data::TStore;
    using namespace Data::Utilities;

    class StressTest : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        StressTest() : ReliableConcurrentQueueTestBase<int>(3)
        {
        }

        ~StressTest()
        {
        }

        ktl::Awaitable<void> EnqueueAsync(__in int numOfIterations)
        {
            for (int i = 0; i < numOfIterations; i++)
            {
                int value = i;
                auto tx = CreateReplicatorTransaction();
                KFinally([&] { tx->Dispose(); });
                co_await RCQ->EnqueueAsync(*tx, value, Common::TimeSpan::MaxValue, ktl::CancellationToken::None);
                co_await tx->CommitAsync();
            }

            co_return;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(StressTestSuite, StressTest)

    BOOST_AUTO_TEST_CASE(Concurrent_Enqueue_ShouldSucceed)
    {
        const int numOfTasks = 100;
        const int numOfIterations = 100;
        KAllocator& allocator = StressTest::GetAllocator();
        KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

        // Concurrently enqueue 
        for (int t = 0; t < numOfTasks; t++)
        {
            // Enqueue numOfIterations items with key = 1..numOfIterations
            tasks.Append(EnqueueAsync(numOfIterations));
        }

        // Wait for all concurrent tasks to complete
        SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

        // Verify the concurrent Enqueue succeeded in all replicas 
        for (ULONG j = 0; j < RCQs->Count(); j++)
        {
            auto rcq = (*RCQs)[j];

            // We dequeue the internal queue which puts the queue in a bad state
            // but this is OK for internal testing for secondaries until we have a real iterator
            auto & concurrentQueue = rcq->GetInternalQueue();

            int allValues[numOfIterations] = { 0 };

            auto tx = CreateWriteTransaction(*rcq);

            for (int t = 0; t < numOfTasks; t++)
            {
                for (int i = 0; i < numOfIterations; i++)
                {
                    LONG64 key;
                    bool succeeded;
                    NTSTATUS status = concurrentQueue.TryDequeue(key, succeeded);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));

                    int value = SyncAwait(GetValueAsync(*rcq, *tx->StoreTransactionSPtr, key));
                    allValues[value]++;
                }
            }

            SyncAwait(tx->AbortAsync());

            for (int t = 0; t < numOfIterations; t++)
            {
                CODING_ERROR_ASSERT(allValues[t] == numOfTasks);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Concurrent_Dequeue_ShouldSucceed)
    {
        const int numOfTasks = 100;
        const int numOfIterations = 100;
        KAllocator& allocator = StressTest::GetAllocator();

        {
            KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

            // Concurrently enqueue 
            for (int t = 0; t < numOfTasks; t++)
            {
                // Enqueue numOfIterations items with key = 1..numOfIterations
                tasks.Append(EnqueueAsync(numOfIterations));
            }

            // Wait for all concurrent tasks to complete
            SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));
        }

        {
            KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

            unsigned int allValues[numOfIterations] = { 0 };

            // Concurrently dequeue 
            for (int t = 0; t < numOfTasks; t++)
            {
                // Enqueue numOfIterations items with key = 1..numOfIterations
                tasks.Append([&]() -> ktl::Awaitable<void> {
                    for (int t = 0; t < numOfIterations; ++t)
                    {
                        int value;
                        bool succeeded = co_await DequeueItem(value);
                        CODING_ERROR_ASSERT(succeeded);

                        InterlockedIncrement((volatile LONG*)&allValues[value]);
                    }
                }());
            }

            // Wait for all concurrent tasks to complete
            SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

            for (int t = 0; t < numOfIterations; t++)
            {
                CODING_ERROR_ASSERT(allValues[t] == numOfTasks);
            }
        }

        // Verify all replicas are now empty 
        for (ULONG j = 0; j < RCQs->Count(); j++)
        {
            auto rcq = (*RCQs)[j];

            // We dequeue the internal queue which puts the queue in a bad state
            // but this is OK for internal testing for secondaries until we have a real iterator
            auto & concurrentQueue = rcq->GetInternalQueue();

            LONG64 value;
            bool succeeded;
            NTSTATUS status = concurrentQueue.TryDequeue(value, succeeded);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CODING_ERROR_ASSERT(!succeeded);
        }
    }

    BOOST_AUTO_TEST_CASE(Concurrent_Enqueue_Dequeue_Interleaved_ShouldSucceed)
    {
        const int numOfTasks = 100;
        const int numOfIterations = 100;
        KAllocator& allocator = StressTest::GetAllocator();

        {
            KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

            unsigned int allValues[numOfIterations] = { 0 };

            for (int t = 0; t < numOfTasks; t++)
            {
                tasks.Append([&]() -> ktl::Awaitable<void> {
                    for (int t = 0; t < numOfIterations; ++t)
                    {
                        co_await EnqueueItem(t);

                        int value;
                        bool succeeded = co_await DequeueItem(value);
                        CODING_ERROR_ASSERT(succeeded);

                        InterlockedIncrement((volatile LONG*)&allValues[value]);
                    }
                }());;
            }

            // Wait for all concurrent tasks to complete
            SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

            for (int t = 0; t < numOfIterations; t++)
            {
                CODING_ERROR_ASSERT(allValues[t] == numOfTasks);
            }
        }

        // Verify all replicas are now empty 
        for (ULONG j = 0; j < RCQs->Count(); j++)
        {
            auto rcq = (*RCQs)[j];

            // We dequeue the internal queue which puts the queue in a bad state
            // but this is OK for internal testing for secondaries until we have a real iterator
            auto & concurrentQueue = rcq->GetInternalQueue();

            LONG64 value;
            bool succeeded;
            NTSTATUS status = concurrentQueue.TryDequeue(value, succeeded);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CODING_ERROR_ASSERT(!succeeded);
        }
    }

    BOOST_AUTO_TEST_CASE(Concurrent_Enqueue_Dequeue_ShouldSucceed)
    {
        const int numOfTasks = 100;
        const int numOfIterations = 100;
        KAllocator& allocator = StressTest::GetAllocator();

        {
            KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

            unsigned int allValues[numOfIterations] = { 0 };

            for (int t = 0; t < numOfTasks; t++)
            {
                tasks.Append([&]() -> ktl::Awaitable<void> {
                    for (int t = 0; t < numOfIterations; ++t)
                    {
                        co_await EnqueueItem(t);
                    }

                    for (int t = 0; t < numOfIterations; ++t)
                    {
                        int value;
                        bool succeeded = co_await DequeueItem(value);
                        CODING_ERROR_ASSERT(succeeded);

                        InterlockedIncrement((volatile LONG*)&allValues[value]);
                    }
                }());;
            }

            // Wait for all concurrent tasks to complete
            SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

            for (int t = 0; t < numOfIterations; t++)
            {
                CODING_ERROR_ASSERT(allValues[t] == numOfTasks);
            }
        }

        // Verify all replicas are now empty 
        for (ULONG j = 0; j < RCQs->Count(); j++)
        {
            auto rcq = (*RCQs)[j];

            // We dequeue the internal queue which puts the queue in a bad state
            // but this is OK for internal testing for secondaries until we have a real iterator
            auto & concurrentQueue = rcq->GetInternalQueue();

            LONG64 value;
            bool succeeded;
            NTSTATUS status = concurrentQueue.TryDequeue(value, succeeded);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CODING_ERROR_ASSERT(!succeeded);
        }
    }
    BOOST_AUTO_TEST_SUITE_END()
}
