// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

FailoverUnitCacheEntry::FailoverUnitCacheEntry(FailoverManager& fm, FailoverUnitUPtr && failoverUnit)
    :   fm_(fm),
        failoverUnit_(move(failoverUnit)),
        lock_(),
        wait_(lock_),
        waitCount_(0),
        isFree_(true),
        isDeleted_(false)
{
}

void FailoverUnitCacheEntry::ProcessTaskAsync(DynamicStateMachineTaskUPtr && task, Federation::NodeInstance from, bool isFromPLB)
{
    if (task)
    {
#if !defined(PLATFORM_UNIX)
        AcquireWriteLock grab(lock_);
#else
        AcquireMutexLock grab(&lock_);
#endif

        pendingTasks_.push_back(move(task));
    }

    FailoverUnitJob job(failoverUnit_->Id, from, isFromPLB);
    fm_.ProcessingQueue.Enqueue(move(job));
}

vector<DynamicStateMachineTaskUPtr> const & FailoverUnitCacheEntry::GetExecutingTasks()
{
    return executingTasks_;
}

bool FailoverUnitCacheEntry::Lock(
    TimeSpan timeout,
    bool executeStateMachine,
    bool & isDeleted)
{
#if !defined(PLATFORM_UNIX)
    AcquireWriteLock grab(lock_);
#else
    AcquireMutexLock grab(&lock_);
#endif

    isDeleted = false;

    if (isFree_)
    {
        if (executeStateMachine)
        {
            executingTasks_ = move(pendingTasks_);
        }
        
        isFree_ = false;
        return true;
    }
    
    StopwatchTime endTime = Stopwatch::Now() + timeout;
    do
    {
        TimeSpan waitTime = endTime - Stopwatch::Now();
        if (waitTime <= TimeSpan::Zero)
        {
            return false;
        }

        waitCount_++;
        wait_.Sleep(waitTime);
        waitCount_--;
    } while (!isFree_);

    if (isDeleted_)
    {
        isDeleted = true;
        return true;
    }

    if (isFree_)
    {
        if (executeStateMachine)
        {
            executingTasks_ = move(pendingTasks_);
        }

        isFree_ = false;

        return true;
    }

    return false;
}

bool FailoverUnitCacheEntry::Release(bool restoreExecutingTask, bool processPendingTask)
{
    bool waiting;
    {
#if !defined(PLATFORM_UNIX)
        AcquireWriteLock grab(lock_);
#else
        AcquireMutexLock grab(&lock_);
#endif

        ASSERT_IF(isFree_,
            "lock is free when released: {0}",
            failoverUnit_->IdString);

        if (restoreExecutingTask)
        {
            for (auto it = executingTasks_.begin(); it != executingTasks_.end(); ++it)
            {
                pendingTasks_.push_back(move(*it));
            }
        }

        executingTasks_.clear();

        if (waitCount_ > 0)
        {
            waiting = true;
        }
        else
        {
            waiting = false;
            if (processPendingTask && pendingTasks_.size() > 0)
            {
                executingTasks_ = move(pendingTasks_);
                return false;
            }
        }

        isFree_ = true;
    }

    if (waiting)
    {
        wait_.Wake();
    }

    // Consider to post to processing queue when there is pending task and
    // restoreExecutingTask is false.  At this moment the code path that
    // can benefit from this optimization is quite rare.

    return true;
}
