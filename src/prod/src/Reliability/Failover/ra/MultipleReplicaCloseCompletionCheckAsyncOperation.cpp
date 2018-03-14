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

MultipleReplicaCloseCompletionCheckAsyncOperation::MultipleReplicaCloseCompletionCheckAsyncOperation(
    Parameters && parameters,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) :
    AsyncOperation(callback, parent, true),
activityId_(move(parameters.ActivityId)),
reason_(parameters.TerminateReason),
closeMode_(parameters.CloseMode),
callback_(parameters.Callback),
monitoringInterval_(*parameters.MonitoringIntervalEntry),
maxWaitBeforeTermination_(parameters.MaxWaitBeforeTerminationEntry),
ra_(*parameters.RA),
ftsToClose_(move(parameters.FTsToClose)),
isCompleted_(false),
hasTerminateBeenCalled_(false),
clock_(parameters.Clock),
check_(parameters.Check)
{
    check_ = static_cast<JobItemCheck::Enum>(check_ | JobItemCheck::FTIsNotNull); // do not rely on the caller

    if (parameters.IsReplicaClosedFunction != nullptr)
    {
        isReplicaClosedFunction_ = parameters.IsReplicaClosedFunction;
    }
    else
    {
        isReplicaClosedFunction_ = MultipleReplicaCloseCompletionCheckAsyncOperation::IsReplicaClosed;
    }

    startTime_ = clock_->Now();
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::OnStart(
    AsyncOperationSPtr const& thisSPtr)
{
    if (ftsToClose_.empty())
    {
        Complete(thisSPtr);
        return;
    }

    StartCloseCompletionCheck(thisSPtr);
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::OnCancel()
{
    Complete(shared_from_this());
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::StartCloseCompletionCheck(
    AsyncOperationSPtr const& thisSPtr)
{
    auto now = clock_->Now();

    auto ftsToClose = GetFailoverUnitsForWhichToCreateJobItems();

    auto work = make_shared<MultipleEntityWork>(
        activityId_,
        [thisSPtr, this, now](MultipleEntityWork & inner, ReconfigurationAgent&)
    {
        OnCloseCompletionCheckComplete(thisSPtr, now, inner);
    });

    auto handler = [this, thisSPtr](Infrastructure::HandlerParameters & handlerParameters, CheckReplicaCloseProgressJobItemContext & context)
    {
        return CloseCompletionCheckProcessor(handlerParameters, context);
    };

    auto check = check_;
    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        ftsToClose,
        [handler, check](EntityEntryBaseSPtr const & entry, std::shared_ptr<MultipleEntityWork> const & innerWork)
    {
        CheckReplicaCloseProgressJobItem::Parameters parameters(
            entry,
            innerWork,
            handler,
            check,
            *JobItemDescription::CheckReplicaCloseProgressJobItem,
            CheckReplicaCloseProgressJobItemContext());

        return make_shared<CheckReplicaCloseProgressJobItem>(move(parameters));
    });
}

bool MultipleReplicaCloseCompletionCheckAsyncOperation::IsReplicaClosed(
    ReplicaCloseMode closeMode, 
    FailoverUnit & ft)
{
    return ft.IsLocalReplicaClosed(closeMode);
}

bool MultipleReplicaCloseCompletionCheckAsyncOperation::CloseCompletionCheckProcessor(
    Infrastructure::HandlerParameters & handlerParameters,
    CheckReplicaCloseProgressJobItemContext & context)
{
    auto & ft = handlerParameters.FailoverUnit;
    if (!ft)
    {
        return false;
    }

    if (!isReplicaClosedFunction_(closeMode_, *ft))
    {
        context.MarkAsStillOpen(ft->ServiceTypeRegistration);
        return true;
    }

    return false;
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::OnCloseCompletionCheckComplete(
    AsyncOperationSPtr const & thisSPtr,
    StopwatchTime now,
    Infrastructure::MultipleEntityWork const & work)
{
    EntityEntryBaseList pending;
    ServiceTypeRegistrationList toBeTerminatedHosts;

    ProcessFailoverUnitWorkCompletion(work, now, pending, toBeTerminatedHosts);

    if (pending.empty())
    {
        Complete(thisSPtr);
    }
    else
    {
        // callback so that the owner of this op can trace etc
        callback_(activityId_, pending, ra_);

        UpdateStateAndPerformAction(thisSPtr, move(pending), toBeTerminatedHosts);
    }
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::Complete(
    AsyncOperationSPtr const& thisSPtr)
{
    TimerSPtr timerToCancel;

    {
        AcquireWriteLock grab(lock_);
        if (isCompleted_)
        {
            return;
        }

        swap(timerToCancel, timer_);
        isCompleted_ = true;
    }

    CloseTimerIfNotNull(timerToCancel);

    TryComplete(thisSPtr, ErrorCodeValue::Success);
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::ProcessFailoverUnitWorkCompletion(
    Infrastructure::MultipleEntityWork const & work,
    Common::StopwatchTime now,
    __out EntityEntryBaseList & pendingFTs,
    __out vector<Hosting2::ServiceTypeRegistrationSPtr> & toBeTerminatedHosts) const
{
    bool hasTerminationWindowExpired = IsTerminateEnabled && (now - startTime_) >= maxWaitBeforeTermination_->GetValue();
    set<wstring> terminatedHostIds;

    for (auto const & it : work.JobItems)
    {        
        auto const & context = it->As<CheckReplicaCloseProgressJobItem>().Context;
        if (context.IsClosed)
        {
            continue;
        }

        pendingFTs.push_back(it->EntrySPtr);
        
        bool shouldTerminateHost =
            hasTerminationWindowExpired &&
            context.Registration != nullptr &&
            terminatedHostIds.find(context.Registration->HostId) == terminatedHostIds.end();

        if (shouldTerminateHost)
        {
            toBeTerminatedHosts.push_back(context.Registration);
            terminatedHostIds.insert(context.Registration->HostId);
        }
    }
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::UpdateStateAndPerformAction(
    AsyncOperationSPtr const& thisSPtr,
    EntityEntryBaseList && pending,
    ServiceTypeRegistrationList const & toBeTerminatedHosts)
{
    ASSERT_IF(pending.empty(), "Pending cannot be empty");

    bool shouldTerminate = false;
    auto timer = CreateTimer(thisSPtr);

    {
        // Check for isCompleted is needed because the async op may complete before timer can be armed
        // In that case the timer created above must be released
        // The rationale for creating a new timer is that otherwise the timer interval is always fixed
        // and cannot be updated in case of a fabric upgrade 
        AcquireWriteLock grab(lock_);
        if (!isCompleted_)
        {
            swap(timer_, timer); 
            ftsToClose_ = move(pending);

            if (!toBeTerminatedHosts.empty() && !hasTerminateBeenCalled_)
            {
                shouldTerminate = true;
                hasTerminateBeenCalled_ = true;
            }
        }
    }

    CloseTimerIfNotNull(timer);

    if (shouldTerminate)
    {
        for (auto const & it : toBeTerminatedHosts)
        {
            ra_.HostingAdapterObj.TerminateServiceHost(activityId_, reason_, it);
        }
    }
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::Test_StartCompletionCheck(AsyncOperationSPtr const& thisSPtr)
{
    StartCloseCompletionCheck(thisSPtr);
}

TimerSPtr MultipleReplicaCloseCompletionCheckAsyncOperation::CreateTimer(AsyncOperationSPtr const & thisSPtr) 
{
    auto timer = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const &)
    {
        {
            AcquireReadLock grab(lock_);
            if (isCompleted_)
            {
                return;
            }
        }

        StartCloseCompletionCheck(thisSPtr);
    });

    timer->Change(monitoringInterval_.GetValue(), TimeSpan::MaxValue);

    return timer;
}

void MultipleReplicaCloseCompletionCheckAsyncOperation::CloseTimerIfNotNull(TimerSPtr const & timer)
{
    if (timer != nullptr)
    {
        timer->Cancel();
    }
}

EntityEntryBaseList MultipleReplicaCloseCompletionCheckAsyncOperation::GetFailoverUnitsForWhichToCreateJobItems() const
{
    AcquireReadLock grab(lock_);
    TESTASSERT_IF(ftsToClose_.empty(), "Cannot have nothing to schedule");
    return ftsToClose_;
}
