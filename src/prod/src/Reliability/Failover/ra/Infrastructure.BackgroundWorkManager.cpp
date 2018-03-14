// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

StringLiteral const WorkAlreadyPendingReason(" work already pending");
StringLiteral const BGMIsClosedOnRequest(" bgm is closed (request)");
StringLiteral const BGMIsClosedOnTimer(" bgm is closed (timer)");
StringLiteral const BGMIsClosedOnWorkComplete(" bgm is closed (work complete)");

BackgroundWorkManager::BackgroundWorkManager(
    std::wstring const & id,
    WorkFunctionPointer const & workFunction,
    TimeSpanConfigEntry const & minIntervalBetweenWork,
    ReconfigurationAgent & ra) 
    : workFunction_(workFunction),
      minIntervalBetweenWork_(minIntervalBetweenWork),
      lastExecutionStartTimeStamp_(StopwatchTime::Zero),
      isExecuting_(false),
      isPending_(false),
      id_(id),
      isWaitingForCompletion_(false),
      isClosed_(false),
      ra_(ra)
{
    auto root = ra.Root.CreateComponentRoot();
    timer_ = Timer::Create("RA.BackgroundWorkManager", [this, root] (TimerSPtr const &)
    {
        this->OnTimer();
    });
}

BackgroundWorkManager::~BackgroundWorkManager()
{
}

void BackgroundWorkManager::Request(wstring const & activityId)
{
    wstring copy = activityId;
    Request(move(copy));
}

void BackgroundWorkManager::Request(wstring && activityId)
{
    {
        AcquireExclusiveLock grab(lock_);

        if (isClosed_)
        {
            RAEventSource::Events->BackgroundWorkManagerDroppingWork(id_, activityId, BGMIsClosedOnRequest);
            return;
        }

        if (isPending_)
        {
            RAEventSource::Events->BackgroundWorkManagerDroppingWork(id_, activityId, WorkAlreadyPendingReason);
            return;
        }

        if (isExecuting_)
        {
            pendingActivityId_ = move(activityId);
            isPending_ = true;
            return;
        }
        else
        {
            ProcessQueuedExecution_UnderLock(move(activityId));
            if (isPending_)
            {
                return;
            }
        }

    }

    EnqueueWork();
}

void BackgroundWorkManager::Close()
{            
    // Always cancel timers
    AcquireExclusiveLock grab(lock_);
    isClosed_ = true;
    if (timer_ != nullptr)
    {
        timer_->Cancel();
        timer_ = nullptr;
    }
}

void BackgroundWorkManager::EnqueueWork()
{
    auto root = ra_.Root.CreateComponentRoot();

    auto cb = [this, root] { this->Process(ra_);  };

    ra_.Threadpool.ExecuteOnThreadpool(cb);
}

void BackgroundWorkManager::Process(ReconfigurationAgent& ra)
{
    wstring activityId;

    {
        AcquireExclusiveLock grab(lock_);
                
        ASSERT_IF(!isExecuting_, "isExecuting must be true otherwise we cannot be processing");
        ASSERT_IF(isWaitingForCompletion_, "isWaitingForCompletion cannot be true if we are not processing");

        lastExecutionStartTimeStamp_ = Stopwatch::Now();
        activityId = processingActivityId_;
        isWaitingForCompletion_ = true;
    }

    ASSERT_IF(activityId.empty(), "ActivityId cannot be empty in process");
    RAEventSource::Events->BackgroundWorkManagerProcess(id_, activityId);

    // execute work    
    workFunction_(activityId, ra, *this);
}

void BackgroundWorkManager::OnWorkComplete()
{
    {
        AcquireExclusiveLock grab(lock_);

        ASSERT_IF(!isExecuting_, "Must be in the executing state");
        ASSERT_IF(!isWaitingForCompletion_, "isWaitingForCompletion must be true");

        RAEventSource::Events->BackgroundWorkManagerOnComplete(id_, processingActivityId_);

        isWaitingForCompletion_ = false;

        if (isPending_)
        {
            if (isClosed_)
            {
                RAEventSource::Events->BackgroundWorkManagerDroppingWork(id_, processingActivityId_, BGMIsClosedOnWorkComplete);
                return;
            }


            // another execution of this work is pending
            // Decide when that execution should take place                    
            isPending_ = false;

            // The function below will decide whether the pending execution should be performed immediately
            // if the minInterval has elapsed or not
            // if not it will set up the timer
            
            // Work around Clang C++ library issue with x = move(x)
            wstring copy = move(pendingActivityId_);
            ProcessQueuedExecution_UnderLock(move(copy));            
            if (isPending_)
            {
                return;
            }
        }
        else
        {
            // all processing is done
            // set isRunning to false
            isExecuting_ = false;
            return;
        }
    }

    EnqueueWork();
}

void BackgroundWorkManager::ProcessQueuedExecution_UnderLock(wstring && activityId)
{
    StopwatchTime currentTime = Stopwatch::Now();
    TimeSpan timeElapsedSinceLastExecution = currentTime - lastExecutionStartTimeStamp_;
    if (timeElapsedSinceLastExecution >= minIntervalBetweenWork_.GetValue())
    {
        // Execute immediately 
        isExecuting_ = true;   
        isPending_ = false;
        processingActivityId_ = move(activityId);
    }
    else
    {
        // Mark is pending and arm timer
        // When the timer fires in the minIntervalBetweenWork we will run again
        // The in the future at which we need to execute this again is computed first
        StopwatchTime futureTimeAtWhichTaskShouldBeRun = lastExecutionStartTimeStamp_ + minIntervalBetweenWork_.GetValue();

        // The time between the future time at which we should run the task and now is the time for which we set the timer
        TimeSpan timerInterval = futureTimeAtWhichTaskShouldBeRun - currentTime;

        pendingActivityId_ = move(activityId);
        isExecuting_ = false;
        isPending_ = true;

        timer_->Change(timerInterval);                
    }
}

void BackgroundWorkManager::OnTimer()
{
    {
        AcquireExclusiveLock grab(lock_);

        ASSERT_IF(isExecuting_, "Cannot be running when the timer expires");
        ASSERT_IFNOT(isPending_, "If the timer was armed then we were pending");

        if (isClosed_)
        {
            RAEventSource::Events->BackgroundWorkManagerDroppingWork(id_, processingActivityId_, BGMIsClosedOnTimer);
            return;
        }

        isPending_ = false;
        isExecuting_ = true;
        processingActivityId_ = move(pendingActivityId_);
    }

    // Run the work
    EnqueueWork();
}
