// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReliableConcurrentQueue.h"
#include "ReliableConcurrentQueue.TestBase.h"
#include "../tstore/TStoreTestBase.h"

#define RCQ_PERF_TAG 'pqcr'

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data::Utilities;
    using namespace Data::Collections;
    using namespace TxnReplicator;

    class ReliableConcurrentQueuePerf : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        Common::CommonConfig config;
        typedef KDelegate<Awaitable<void>(int)> PrintExecutionFunctionType1;
        typedef KDelegate<Awaitable<void>(int, int)> PrintExecutionFunctionType2;

    public:
        // Test Functions
        Awaitable<void> Test_OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async() noexcept;
        Awaitable<void> Test_MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async() noexcept;
        Awaitable<void> Test_ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync() noexcept;
        Awaitable<void> Test_ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync() noexcept;
        Awaitable<void> Test_ParallelEnqueuesDequeues_SinglePerTxnAsync() noexcept;
        Awaitable<void> Test_ParallelEnqueuesDequeues_MultiplePerTxnAsync() noexcept;

        Awaitable<void> Measure_EnqueueN_DequeueN_SingleTxnAsync(int numOps);
        Awaitable<void> Measure_EnqueueN_DequeueN_MultipleTxnAsync(int numOps);
        Awaitable<void> Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(int numTasks, int numOperationsPerTask);
        Awaitable<void> Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(int numTasks, int numOperationsPerTask);
        Awaitable<void> Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(int numTasks, int numOperationsPerTask);
        Awaitable<void> Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(int numTasks, int numOperationsPerTask);

        Awaitable<void> EnqueueN_MultipleOpsPerTxnAsync(int numEnqueues);
        Awaitable<void> DequeueN_MultipleOpsPerTxnAsync(int numDequeues);
        Awaitable<void> EnqueueN_SingleOpPerTxnAsync(int numEnqueues);
        Awaitable<void> DequeueN_SingleOpPerTxnAsync(int numEnqueues);
        Awaitable<void> ParallelEnqueues_SinglePerTxn(int numTasks, int numEnqueuePerTask);
        Awaitable<void> ParallelDequeues_SinglePerTxn(int numTasks, int numDequeuePerTask);
        Awaitable<void> ParallelEnqueues_MultipleEnqueuesPerTxn(int numTasks, int numEnqueuePerTask);
        Awaitable<void> ParallelDequeues_MultipleDequeuesPerTxn(int numTasks, int numDequeuePerTask);
        Awaitable<void> ParallelEnqueuesDequeues_SingleOpPerTxn(int numTasks, int numOperationsPerTask);
        Awaitable<void> ParallelEnqueuesDequeues_MultipleOpsPerTxn(int numTasks, int numOperationsPerTask);

        ktl::Awaitable<void> EnqueueItems(__in int numEnqueues)
        {
            int value = 10;
            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

            {
                KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
                KFinally([&] { transactionSPtr->Dispose(); });

                for (int i = 0; i < numEnqueues; ++i)
                {
                    co_await rcq->EnqueueAsync(*transactionSPtr, value, Common::TimeSpan::FromSeconds(4), CancellationToken::None);
                }

                co_await transactionSPtr->CommitAsync();
            }
        }

        LONG64 total = 0;
        KReaderWriterSpinLock lock;
        LONG64 deqCount = 0;

        ktl::Awaitable<void> DequeueItems(__in int numDequeues)
        {
            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();
            Common::Stopwatch stopwatch;
            stopwatch.Start();
            {
                KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
                KFinally([&] { transactionSPtr->Dispose(); });

                for (int i = 0; i < numDequeues; ++i)
                {
                    int value = -1;
                    bool succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                    CODING_ERROR_ASSERT(succeeded);
                }

                co_await transactionSPtr->CommitAsync();
            }
            stopwatch.Stop();

            lock.AcquireExclusive();
            total += stopwatch.ElapsedMilliseconds;
            deqCount++;
            // auto avg = total / deqCount;
            lock.ReleaseExclusive();
            // std::cout << avg << std::endl;
        }


        ktl::Awaitable<void> ContinuouslyEnqueueItems(
            __in LONG64 count, 
            __in ktl::AwaitableCompletionSource<NTSTATUS>& acs)
        {
            co_await acs.GetAwaitable();
            for(int i=0; i<count; i++)
            {
                co_await EnqueueItems(1);
            }
            co_return;
        }

        ktl::Awaitable<void> ContinuouslyEnqueueItems(
            __in ktl::AwaitableCompletionSource<NTSTATUS>& acs,
            __in Common::Stopwatch& stopwatch,
            __in LONG64 timeLimit)
        {
            UNREFERENCED_PARAMETER(timeLimit);
            co_await acs.GetAwaitable();
            while (stopwatch.ElapsedMilliseconds < 60*1000)
            {
                co_await EnqueueItems(1);
            }
        }

        ktl::Awaitable<void> ContinuouslyDequeueItems(
            __in LONG64 count, 
            __in ktl::AwaitableCompletionSource<NTSTATUS>& acs)
        {   
            co_await acs.GetAwaitable();
            for (int i = 0; i<count; i++)
            {
                co_await DequeueItems(1);
            }
        }

        ktl::Awaitable<void> ParallelDequeuesOneItemPerTxnAsync(
            __in ULONG32 count,
            __in ULONG32 numTasks)
        {
            KSharedArray<ktl::Awaitable<void>>::SPtr dequeueTasksSPtr = _new(RCQ_PERF_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            Common::Stopwatch stopwatch;
            stopwatch.Start();

            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr signalCompletion = nullptr;
            NTSTATUS status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(this->GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ULONG32 countPerTask = count / numTasks;
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                dequeueTasksSPtr->Append(ContinuouslyDequeueItems(countPerTask, *signalCompletion));
            }

            signalCompletion->SetResult(true);
            co_await StoreUtilities::WhenAll(*dequeueTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            std::cout << "ParallelEnqueuesOneItemPerTxnAsync(Count=" << count << ", Task=" << numTasks << ") = " << stopwatch.ElapsedMilliseconds << "ms" << std::endl;
            std::cout << "    = " << count * 1.0f / (stopwatch.ElapsedMilliseconds * 1.0f / 1000.0f) << " OP/S" << std::endl;
            std::cout << "    = " << stopwatch.ElapsedMilliseconds * 1.0f / count << " ms/OP" << std::endl;
            CODING_ERROR_ASSERT(this->get_RCQ()->Count == 0);
        }

        ktl::Awaitable<void> ParallelEnqueuesOneItemPerTxnAsync(
            __in ULONG32 count,
            __in ULONG32 numTasks)
        {
            CODING_ERROR_ASSERT(count%numTasks == 0);

            KSharedArray<ktl::Awaitable<void>>::SPtr enqueueTasksSPtr = _new(RCQ_PERF_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            Common::Stopwatch stopwatch;
            stopwatch.Start();

            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr signalCompletion = nullptr;
            NTSTATUS status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(this->GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ULONG32 countPerTask = count / numTasks;
            for (ULONG32 i = 0; i < numTasks; i++)
            {             
                enqueueTasksSPtr->Append(ContinuouslyEnqueueItems(countPerTask, *signalCompletion));
            }

            signalCompletion->SetResult(true);
            co_await StoreUtilities::WhenAll(*enqueueTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            std::cout << "ParallelEnqueuesOneItemPerTxnAsync(Count=" << count << ", Task=" << numTasks << ") = " << stopwatch.ElapsedMilliseconds << "ms" << std::endl;
            std::cout << "    = " << count * 1.0f / (stopwatch.ElapsedMilliseconds * 1.0f / 1000.0f) << " OP/S" << std::endl;
            std::cout << "    = " << stopwatch.ElapsedMilliseconds * 1.0f / count << " ms/OP" << std::endl;

            CODING_ERROR_ASSERT(this->get_RCQ()->Count == count);
        }

        ktl::Awaitable<void> ParallelEnqueuesOneItemPerTxnOneMinuteAsync(
            __in ULONG32 numTasks)
        {
            KSharedArray<ktl::Awaitable<void>>::SPtr enqueueTasksSPtr = _new(RCQ_PERF_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr signalCompletion = nullptr;
            NTSTATUS status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(this->GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            Common::Stopwatch stopwatch;

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                enqueueTasksSPtr->Append(ContinuouslyEnqueueItems(*signalCompletion, stopwatch, 60000));
            }

            stopwatch.Start();
            signalCompletion->SetResult(true);
            co_await StoreUtilities::WhenAll(*enqueueTasksSPtr, this->GetAllocator());

            std::cout << this->get_RCQ()->Count << std::endl;
        }

    private:
        // Helper Functions
        Awaitable<void> PrintExecutionTimeAsync(
            PrintExecutionFunctionType1,
            int argument,
            string message);

        Awaitable<void> PrintExecutionTimeAsync(
            PrintExecutionFunctionType2,
            int arg1,
            int arg2,
            string message);
    };

    BOOST_FIXTURE_TEST_SUITE(ReliableConcurrentQueuePerfTestSuite, ReliableConcurrentQueuePerf)

    BOOST_AUTO_TEST_CASE(ParallelDequeues_OneItemPerTxn_200Tasks)
    {
        std::cout << "Test Started : ParallelDequeues_OneItemPerTxn_200Tasks" << std::endl;
        const ULONG32 totalCount = 1000000;
        const ULONG32 tasks = 200;
        std::cout << "Enqueue " << totalCount << " items using " << tasks << " tasks..." << std::endl;
        SyncAwait(ParallelEnqueuesOneItemPerTxnAsync(totalCount, tasks));
        std::cout << "Dequeue " << totalCount << " items using " << tasks << " tasks..." << std::endl;
        SyncAwait(ParallelDequeuesOneItemPerTxnAsync(totalCount, tasks));
        std::cout << "Test Ended : ParallelDequeues_OneItemPerTxn_200Tasks" << std::endl;
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_OneItemPerTxn_OneMillion_1Task)
    {
        std::cout << "Test Started : ParallelEnqueues_OneItemPerTxn_OneMillion_1Task" << std::endl;
        const LONG64 totalCount = 1000000;
        SyncAwait(ParallelEnqueuesOneItemPerTxnAsync(totalCount, 1));
        std::cout << "Test Ended: ParallelEnqueues_OneItemPerTxn_OneMillion_1Task" << std::endl;
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_OneItemPerTxn_OneMillion_200Tasks)
    {
        std::cout << "Test Started : ParallelEnqueues_OneItemPerTxn_OneMillion_200Tasks" << std::endl;
        const LONG64 totalCount = 1000000;
        SyncAwait(ParallelEnqueuesOneItemPerTxnAsync(totalCount, 200));
        std::cout << "Test Ended: ParallelEnqueues_OneItemPerTxn_OneMillion_200Tasks" << std::endl;
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_OneItemPerTxn_1Task_1Minute)
    {
        std::cout << "Test Started : ParallelEnqueues_OneItemPerTxn_1Task_1Minute" << std::endl;
        SyncAwait(ParallelEnqueuesOneItemPerTxnOneMinuteAsync(1));
        std::cout << "Test Ended: ParallelEnqueues_OneItemPerTxn_1Task_1Minute" << std::endl;
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_OneItemPerTxn_200Tasks_1Minute)
    {
        std::cout << "Test Started : ParallelEnqueues_OneItemPerTxn_200Tasks_1Minute" << std::endl;
        SyncAwait(ParallelEnqueuesOneItemPerTxnOneMinuteAsync(200));
        std::cout << "Test Ended: ParallelEnqueues_OneItemPerTxn_200Tasks_1Minute" << std::endl;
    }

    BOOST_AUTO_TEST_CASE(OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async)
    {
        SyncAwait(Test_OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async());
    }

    BOOST_AUTO_TEST_CASE(MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async)
    {
        SyncAwait(Test_MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async());
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync)
    {
        SyncAwait(Test_ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync());
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync)
    {
        SyncAwait(Test_ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync());
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueuesDequeues_SinglePerTxnAsync)
    {
        SyncAwait(Test_ParallelEnqueuesDequeues_SinglePerTxnAsync());
    }

    BOOST_AUTO_TEST_CASE(ParallelEnqueuesDequeues_MultiplePerTxnAsync)
    {
        SyncAwait(Test_ParallelEnqueuesDequeues_MultiplePerTxnAsync());
    }
    BOOST_AUTO_TEST_SUITE_END()

#pragma region Test Functions

    Awaitable<void> ReliableConcurrentQueuePerf::Test_OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async() noexcept
    {
        std::cout << "Test Started : Test_OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async"<<std::endl;
        co_await Measure_EnqueueN_DequeueN_SingleTxnAsync(1);
        co_await Measure_EnqueueN_DequeueN_SingleTxnAsync(10);
        co_await Measure_EnqueueN_DequeueN_SingleTxnAsync(100);
        co_await Measure_EnqueueN_DequeueN_SingleTxnAsync(1000);
        co_await Measure_EnqueueN_DequeueN_SingleTxnAsync(10000);
        std::cout << "Test Ended : Test_OneEnqueue_OneDequeue_PerTxn_VaryEnqueueCount_Async" << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Test_MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async() noexcept
    {
        std::cout << "Test Started : Test_MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async" << std::endl;
        co_await Measure_EnqueueN_DequeueN_MultipleTxnAsync(1);
        co_await Measure_EnqueueN_DequeueN_MultipleTxnAsync(10);
        co_await Measure_EnqueueN_DequeueN_MultipleTxnAsync(100);
        co_await Measure_EnqueueN_DequeueN_MultipleTxnAsync(1000);
        co_await Measure_EnqueueN_DequeueN_MultipleTxnAsync(10000);
        std::cout << "Test Ended : Test_MultipleEnqueue_MultipleDequeue_PerTxn_VaryEnqueueCount_Async" << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Test_ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync() noexcept
    {
        std::cout << "Test Started : Test_ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync" << std::endl;
        co_await Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(10, 10);
        co_await Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(10, 1000);
        co_await Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(1000, 10);
        co_await Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(100, 100);
        std::cout << "Test Ended : Test_ParallelEnqueues_Then_ParallelDequeues_SinglePerTxnAsync" << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Test_ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync() noexcept
    {
        std::cout << "Test Started : Test_ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync" << std::endl;
        co_await Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(10, 10);
        co_await Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(10, 1000);
        co_await Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(1000, 10);
        co_await Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(100, 100);
        std::cout << "Test Ended : Test_ParallelEnqueues_Then_ParallelDequeues_MultiplePerTxnAsync" << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Test_ParallelEnqueuesDequeues_SinglePerTxnAsync() noexcept
    {
        std::cout << "Test Started : Test_ParallelEnqueuesDequeues_SinglePerTxnAsync" << std::endl;
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(10, 10);
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(10, 1000);
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(1000, 10);
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(100, 100);
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(10, 10000);
        co_await Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(100, 1000);
        std::cout << "Test Ended : Test_ParallelEnqueuesDequeues_SinglePerTxnAsync" << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Test_ParallelEnqueuesDequeues_MultiplePerTxnAsync() noexcept
    {
        std::cout << "Test Started : Test_ParallelEnqueuesDequeues_MultiplePerTxnAsync" << std::endl;
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(10, 10);
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(10, 1000);
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(1000, 10);
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(100, 100);
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(10, 10000);
        co_await Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(100, 1000);
        std::cout << "Test Ended : Test_ParallelEnqueuesDequeues_MultiplePerTxnAsync" << std::endl;
    }

#pragma endregion

#pragma region Helper Functions
    
    Awaitable<void> ReliableConcurrentQueuePerf::PrintExecutionTimeAsync(
        PrintExecutionFunctionType1 action,
        int argument,
        string message)
    {
        Common::Stopwatch stopwatch;
        stopwatch.Start();

        co_await action(argument);

        stopwatch.Stop();

        std::cout << message << " : " << stopwatch.ElapsedMilliseconds << " ms\n";
    }

    Awaitable<void> ReliableConcurrentQueuePerf::PrintExecutionTimeAsync(
        PrintExecutionFunctionType2 action,
        int arg1,
        int arg2,
        string message)
    {
        Common::Stopwatch stopwatch;
        stopwatch.Start();

        co_await action(arg1, arg2);

        stopwatch.Stop();

        std::cout << message << " : " << stopwatch.ElapsedMilliseconds << " ms\n";
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Measure_EnqueueN_DequeueN_SingleTxnAsync(int numOps)
    {
        std::cout<<"Measure_EnqueueN_DequeueN_SinglePerTxnAsync : numItems = "<< numOps <<std::endl;
        
        std::cout << "Enqueue " << numOps << " SingleTxnAsync = > \n";

        PrintExecutionFunctionType1 enqueueTask;
        enqueueTask.Bind(this, &ReliableConcurrentQueuePerf::EnqueueN_MultipleOpsPerTxnAsync);
        co_await PrintExecutionTimeAsync(enqueueTask, numOps, "RCQ - ");

        std::cout << "Dequeue " << numOps << " SingleTxnAsync = > \n";

        PrintExecutionFunctionType1 dequeueTask;
        dequeueTask.Bind(this, &ReliableConcurrentQueuePerf::DequeueN_MultipleOpsPerTxnAsync);
        co_await PrintExecutionTimeAsync(dequeueTask, numOps, "RCQ - ");

        std::cout << std::endl;
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Measure_EnqueueN_DequeueN_MultipleTxnAsync(int numOps)
    {
        std::cout << "Measure_EnqueueN_DequeueN_MultiplePerTxnAsync : numItems = " << numOps << std::endl;

        std::cout << "Enqueue " << numOps;

        PrintExecutionFunctionType1 enqueueTask;
        enqueueTask.Bind(this, &ReliableConcurrentQueuePerf::EnqueueN_SingleOpPerTxnAsync);
        co_await PrintExecutionTimeAsync(enqueueTask, numOps, "RCQ - ");

        std::cout << "Dequeue " << numOps;

        PrintExecutionFunctionType1 dequeueTask;
        dequeueTask.Bind(this, &ReliableConcurrentQueuePerf::DequeueN_SingleOpPerTxnAsync);
        co_await PrintExecutionTimeAsync(dequeueTask, numOps, "RCQ - ");

        std::cout << std::endl;
    }
    
    Awaitable<void> ReliableConcurrentQueuePerf::Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync(int numTasks, int numOperationsPerTask)
    {
        std::cout << "Measure_ParallelEnqueues_ParallelDequeues_SingleOpPerTxnAsync : numTasks = " << numTasks << ", numOperationsPerTask = " << numOperationsPerTask << ", totalItems = " << numTasks * numOperationsPerTask << std::endl;

        std::cout << "Enqueues => " << std::endl;

        PrintExecutionFunctionType2 enqueueTask;
        enqueueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelEnqueues_SinglePerTxn);
        co_await PrintExecutionTimeAsync(enqueueTask, numTasks, numOperationsPerTask, "RCQ - ");
        
        std::cout << "Dequeues => " << std::endl;

        PrintExecutionFunctionType2 dequeueTask;
        dequeueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelDequeues_SinglePerTxn);
        co_await PrintExecutionTimeAsync(dequeueTask, numTasks, numOperationsPerTask, "RCQ - ");
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync(int numTasks, int numOperationsPerTask)
    {
        std::cout << "Measure_ParallelEnqueues_ParallelDequeues_MultipleOpsPerTxnAsync : numTasks = " << numTasks << ", numOperationsPerTask = " << numOperationsPerTask << ", totalItems = " << numTasks * numOperationsPerTask << std::endl;

        std::cout << "Enqueues => " << std::endl;

        PrintExecutionFunctionType2 enqueueTask;
        enqueueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelEnqueues_MultipleEnqueuesPerTxn);
        co_await PrintExecutionTimeAsync(enqueueTask, numTasks, numOperationsPerTask, "RCQ - ");

        std::cout << "Dequeues => " << std::endl;

        PrintExecutionFunctionType2 dequeueTask;
        dequeueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelDequeues_MultipleDequeuesPerTxn);
        co_await PrintExecutionTimeAsync(dequeueTask, numTasks, numOperationsPerTask, "RCQ - ");
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync(int numTasks, int numOperationsPerTask)
    {
        // setup some state to simulate delay in Consumers starting.
        co_await EnqueueN_MultipleOpsPerTxnAsync(numOperationsPerTask);

        std::cout << "Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync : numTasks = " << numTasks << ", numOperationsPerTask = " << numOperationsPerTask << ", totalItems = " << numTasks * numOperationsPerTask << std::endl;

        std::cout << "EnqueueDequeue => " << std::endl;
        PrintExecutionFunctionType2 enqueueDequeueTask;
        enqueueDequeueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelEnqueuesDequeues_SingleOpPerTxn);
        co_await PrintExecutionTimeAsync(enqueueDequeueTask, numTasks, numOperationsPerTask, "RCQ - ");
    }

    Awaitable<void> ReliableConcurrentQueuePerf::Measure_ParallelEnqueuesDequeues_MultipleOpsPerTxnAsync(int numTasks, int numOperationsPerTask)
    {
        // setup some state to simulate delay in Consumers starting.
        co_await EnqueueN_MultipleOpsPerTxnAsync(numOperationsPerTask);

        std::cout << "Measure_ParallelEnqueuesDequeues_SingleOpPerTxnAsync : numTasks = " << numTasks << ", numOperationsPerTask = " << numOperationsPerTask << ", totalItems = " << numTasks * numOperationsPerTask << std::endl;

        std::cout << "EnqueueDequeue => " << std::endl;
        PrintExecutionFunctionType2 enqueueDequeueTask;
        enqueueDequeueTask.Bind(this, &ReliableConcurrentQueuePerf::ParallelEnqueuesDequeues_MultipleOpsPerTxn);
        co_await PrintExecutionTimeAsync(enqueueDequeueTask, numTasks, numOperationsPerTask, "RCQ - ");
    }

    // Enqueue numEnqueues items from the queue, all in one transaction
    Awaitable<void> ReliableConcurrentQueuePerf::EnqueueN_MultipleOpsPerTxnAsync(int numEnqueues)
    {
        auto queue = this->get_RCQ();
        int value = 10;
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();
        
        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            for (int i = 0; i < numEnqueues; ++i)
            {
                co_await rcq->EnqueueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            }
            co_await transactionSPtr->CommitAsync();
        }
    }

    // Dequeue numDequeues items from the queue, all in one transaction
    Awaitable<void> ReliableConcurrentQueuePerf::DequeueN_MultipleOpsPerTxnAsync(int numDequeues)
    {
        auto queue = this->get_RCQ();
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            bool succeeded;
            for (int i = 0; i < numDequeues; ++i)
            {
                do
                {
                    int value;
                    succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                } while (!succeeded);
            }
            co_await transactionSPtr->CommitAsync();
        }
    }

    // Enqueue numEnqueues items from the queue, 1 item per transaction
    Awaitable<void> ReliableConcurrentQueuePerf::EnqueueN_SingleOpPerTxnAsync(int numEnqueues)
    {
        auto queue = this->get_RCQ();
        int value = 10;
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        for (int i = 0; i < numEnqueues; ++i)
        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await rcq->EnqueueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            co_await transactionSPtr->CommitAsync();
        }
    }

    // Dequeue numDequeue items from the queue, 1 item per transaction
    Awaitable<void> ReliableConcurrentQueuePerf::DequeueN_SingleOpPerTxnAsync(int numDequeues)
    {
        auto queue = this->get_RCQ();
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        for (int i = 0; i < numDequeues; ++i)
        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });
            
            bool succeeded;
            do
            {
                int value;
                succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            } while (!succeeded);

            co_await transactionSPtr->CommitAsync();
        }
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelEnqueues_SinglePerTxn(int numTasks, int numEnqueuePerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr enqueueTasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            enqueueTasks->Append(EnqueueN_SingleOpPerTxnAsync(numEnqueuePerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*enqueueTasks);
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelDequeues_SinglePerTxn(int numTasks, int numDequeuePerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr dequeueTasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            dequeueTasks->Append(DequeueN_SingleOpPerTxnAsync(numDequeuePerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*dequeueTasks);
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelEnqueues_MultipleEnqueuesPerTxn(int numTasks, int numEnqueuePerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr enqueueTasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            enqueueTasks->Append(EnqueueN_MultipleOpsPerTxnAsync(numEnqueuePerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*enqueueTasks);
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelDequeues_MultipleDequeuesPerTxn(int numTasks, int numDequeuePerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr dequeueTasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            dequeueTasks->Append(DequeueN_MultipleOpsPerTxnAsync(numDequeuePerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*dequeueTasks);
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelEnqueuesDequeues_SingleOpPerTxn(int numTasks, int numOperationsPerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr tasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            tasks->Append(EnqueueN_SingleOpPerTxnAsync(numOperationsPerTask));
            tasks->Append(DequeueN_SingleOpPerTxnAsync(numOperationsPerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*tasks);
    }

    Awaitable<void> ReliableConcurrentQueuePerf::ParallelEnqueuesDequeues_MultipleOpsPerTxn(int numTasks, int numOperationsPerTask)
    {
        KSharedArray<Awaitable<void>>::SPtr tasks = _new(RCQ_PERF_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        for (int i = 0; i < numTasks; ++i)
        {
            tasks->Append(EnqueueN_MultipleOpsPerTxnAsync(numOperationsPerTask));
            tasks->Append(DequeueN_MultipleOpsPerTxnAsync(numOperationsPerTask));
        }

        co_await Data::Utilities::TaskUtilities<void>::WhenAll(*tasks);
    }
#pragma endregion

}
