// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncWorkJobQueuePerfCounters
    {
        DENY_COPY(AsyncWorkJobQueuePerfCounters)
    public:
        BEGIN_COUNTER_SET_DEFINITION(
            L"9FB94052-4270-460A-BB68-73C37D90BC73",
            L"AsyncJobQueue",
            L"Counters for AsyncJobQueue used by Service Fabric Components",
            PerformanceCounterSetInstanceType::Multiple)
            COUNTER_DEFINITION(
                1,
                PerformanceCounterType::RateOfCountPerSecond64,
                L"# Items in the job queue",
                L"Number of items in the job queue")
            COUNTER_DEFINITION(
                2,
                PerformanceCounterType::RateOfCountPerSecond64,
                L"# Items/Second failed to be inserted into the job queue",
                L"Number of items failed to be inserted into the job queue per second, because the job queue was full")
            COUNTER_DEFINITION(
                3,
                PerformanceCounterType::AverageBase,
                L"Avg. time in ms an item spends in the queue/Operation Base",
                L"Base counter for measuring the average time each job queue item spends in the queue",
                noDisplay)
            COUNTER_DEFINITION_WITH_BASE(
                4,
                3,
                PerformanceCounterType::AverageCount64,
                L"Avg. time (ms) an item spends in the job queue until completed/Operation",
                L"The average time each job queue item spends in the job queue until completed (ms)")
            COUNTER_DEFINITION(
                5,
                PerformanceCounterType::RateOfCountPerSecond64,
                L"# Items/Second inserted into the job queue",
                L"Number of items inserted into the job queue per second")
            COUNTER_DEFINITION(
                6,
                PerformanceCounterType::RateOfCountPerSecond64,
                L"# Items completed synchronously",
                L"Number of items that completed work synchronously")
            COUNTER_DEFINITION(
                7,
                PerformanceCounterType::RateOfCountPerSecond64,
                L"# Items completed asynchronously",
                L"Number of items that completed work asynchronously")
            COUNTER_DEFINITION(
                8,
                PerformanceCounterType::AverageBase,
                L"Avg. time (ms) an item spends in the queue until work is started/Operation Base",
                L"Base counter for measuring the average time each job queue item spends in the queue before work is started",
                noDisplay)
            COUNTER_DEFINITION_WITH_BASE(
                9,
                8,
                PerformanceCounterType::AverageCount64,
                L"Avg. time (ms) an item spends in the queue before work is started/Operation",
                L"The average time each job queue item spends in the queue before work is started (ms)")
            COUNTER_DEFINITION(
                10,
                PerformanceCounterType::AverageBase,
                L"Avg. time (ms) an item spends in the queue before async work is ready to complete/Operation Base",
                L"Base counter for measuring the average time each job queue item spends in the queue before async work is ready to complete",
                noDisplay)
            COUNTER_DEFINITION_WITH_BASE(
                11,
                10,
                PerformanceCounterType::AverageCount64,
                L"Avg. time (ms) an item spends in the queue before async work is ready to complete/Operation",
                L"The average time each job queue item spends in the queue before async work is ready to complete (ms)")
        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(NumberOfItems)
        DECLARE_COUNTER_INSTANCE(NumberOfDroppedItems)
		DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueueBase)
		DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueue)
        DECLARE_COUNTER_INSTANCE(NumberOfItemsInsertedPerSecond)
        DECLARE_COUNTER_INSTANCE(NumberOfItemsCompletedSync)
        DECLARE_COUNTER_INSTANCE(NumberOfItemsCompletedAsync)
        DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeStartWorkBase)
        DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeStartWork)
        DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeWorkIsAsyncReadyBase)
        DECLARE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeWorkIsAsyncReady)

        BEGIN_COUNTER_SET_INSTANCE(AsyncWorkJobQueuePerfCounters)
            DEFINE_COUNTER_INSTANCE(NumberOfItems,                  1)
            DEFINE_COUNTER_INSTANCE(NumberOfDroppedItems,           2)
			DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueueBase,    3)
            DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueue,        4)
            DEFINE_COUNTER_INSTANCE(NumberOfItemsInsertedPerSecond, 5)
            DEFINE_COUNTER_INSTANCE(NumberOfItemsCompletedSync,     6)
            DEFINE_COUNTER_INSTANCE(NumberOfItemsCompletedAsync,    7)
            DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeStartWorkBase, 8)
            DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeStartWork, 9)
            DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeWorkIsAsyncReadyBase, 10)
            DEFINE_COUNTER_INSTANCE(AverageTimeSpentInQueueBeforeWorkIsAsyncReady, 11)
        END_COUNTER_SET_INSTANCE()

        static std::shared_ptr<AsyncWorkJobQueuePerfCounters> CreateInstance(
            std::wstring const & ownerName,
            std::wstring const & ownerInstance);

    public:
        void OnItemAdded();
        void OnItemRejected();
        void OnItemStartWork(StopwatchTime const & enqueueTime);
        void OnItemAsyncWorkReady(StopwatchTime const & enqueueTime);
        void OnItemCompleted(bool completedSync, StopwatchTime const & enqueueTime);
    };

    using AsyncWorkJobQueuePerfCountersSPtr = std::shared_ptr<AsyncWorkJobQueuePerfCounters>;
}
