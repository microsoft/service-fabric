// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    INITIALIZE_COUNTER_SET(AsyncWorkJobQueuePerfCounters)

    AsyncWorkJobQueuePerfCountersSPtr AsyncWorkJobQueuePerfCounters::CreateInstance(
        std::wstring const & ownerName,
        std::wstring const & ownerInstance)
    {
        std::wstring id;
        Common::StringWriter writer(id);
        writer.Write("{0}:{1}:{2}", ownerName, ownerInstance, SequenceNumber::GetNext());
        return AsyncWorkJobQueuePerfCounters::CreateInstance(id);
    }

    void AsyncWorkJobQueuePerfCounters::OnItemAdded()
    {
        NumberOfItems.Increment();
        NumberOfItemsInsertedPerSecond.Increment();
    }

    void AsyncWorkJobQueuePerfCounters::OnItemRejected()
    {
        NumberOfDroppedItems.Increment();
    }

    void AsyncWorkJobQueuePerfCounters::OnItemStartWork(StopwatchTime const & enqueueTime)
    {
        TimeSpan delta = Stopwatch::Now() - enqueueTime;
        AverageTimeSpentInQueueBeforeStartWorkBase.Increment();
        AverageTimeSpentInQueueBeforeStartWork.IncrementBy(delta.TotalMilliseconds());
    }

    void AsyncWorkJobQueuePerfCounters::OnItemAsyncWorkReady(StopwatchTime const & enqueueTime)
    {
        TimeSpan delta = Stopwatch::Now() - enqueueTime;
        AverageTimeSpentInQueueBeforeWorkIsAsyncReadyBase.Increment();
        AverageTimeSpentInQueueBeforeWorkIsAsyncReady.IncrementBy(delta.TotalMilliseconds());
    }

    void AsyncWorkJobQueuePerfCounters::OnItemCompleted(bool completedSync, StopwatchTime const & enqueueTime)
    {
        TimeSpan delta = Stopwatch::Now() - enqueueTime;
        NumberOfItems.Decrement();
        if (completedSync)
        {
            NumberOfItemsCompletedSync.Increment();
        }
        else
        {
            NumberOfItemsCompletedAsync.Increment();
        }

        AverageTimeSpentInQueueBase.Increment();
        AverageTimeSpentInQueue.IncrementBy(delta.TotalMilliseconds());
    }
}
