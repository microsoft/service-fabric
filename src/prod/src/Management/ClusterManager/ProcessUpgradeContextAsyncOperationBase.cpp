// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Management::ClusterManager;

// This helper class encapsulates committing various timer updates during the lifetime of the upgrade. There are two timers of
// interest:
//
// 1) 'upgradeProgressStopwatch_' tracks the time spent processing the upgrade and is started after upgrade initialization completes.
//    It is used to persist the overall upgrade duration and the duration of the current processing UD as two separate values. 
//
//    For monitored upgrades, the current UD duration is reset after each health check phase (pass or fail) - it does not reset when 
//    health evaluation policies or monitoring policies are modified. 
//
//    For manual upgrades, the current UD duration is reset when the UD completes and remains at 0 until MoveNext is called and the 
//    next UD starts processing. At which point the UD duration starts getting tracked again.
//
//    For auto upgrades, the current UD duration is reset as each UD completes.
//
//    The overall upgrade duration is never reset.
//
// 2) 'healthCheckProgressStopwatch_' tracks the time spent in the health check phase, which is included in the overall upgrade processing time.
//    This timer is started/stopped before/after each health check phase (pass or fail). The persisted value resets to the wait duration if the health check
//    result changes between consecutive health evaluations, which effectively resets the retry and stable timers. 
//
// These timers are only persisted during the CM/FM protocol and health check phases.
//
template <class TUpgradeContext, class TUpgradeRequestBody>
class ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::CommitUpgradeContextAsyncOperation : public Common::AsyncOperation
{
public:

    static AsyncOperationSPtr CreateForUpdateAndCommit(
        __in ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody> & owner,
        StoreTransaction && storeTx,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CommitUpgradeContextAsyncOperation>(
            owner,
            move(storeTx),
            true, // update timers
            false, // don't reset UD timers
            callback,
            parent);
    }

    static AsyncOperationSPtr CreateForCommitOnly(
        __in ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody> & owner,
        StoreTransaction && storeTx,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CommitUpgradeContextAsyncOperation>(
            owner,
            move(storeTx),
            false, // don't update timers
            false, // don't reset UD timers
            callback,
            parent);
    }

    static AsyncOperationSPtr CreateForUpgradeDomainReset(
        __in ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody> & owner,
        StoreTransaction && storeTx,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CommitUpgradeContextAsyncOperation>(
            owner,
            move(storeTx),
            true, // update timers
            true, // reset UD timers
            callback,
            parent);
    }

    CommitUpgradeContextAsyncOperation(
        __in ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody> & owner,
        StoreTransaction && storeTx,
        bool doUpdate,
        bool resetUDTimeouts,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , storeTx_(move(storeTx))
        , updateElapsedTimes_(doUpdate)
        , resetUpgradeDomainTimeouts_(resetUDTimeouts)
        , commitStopwatch_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitUpgradeContextAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);

        auto & upgradeContext = owner_.upgradeContext_;

        if (updateElapsedTimes_)
        {
            // Account for time it takes to persist updates
            //
            commitStopwatch_.Start();

            if (owner_.upgradeProgressStopwatch_.IsRunning)
            {
                upgradeContext.UpdateHealthMonitoringTimeouts(owner_.upgradeProgressStopwatch_.Elapsed);
            }

            if (owner_.healthCheckProgressStopwatch_.IsRunning)
            {
                upgradeContext.UpdateHealthCheckElapsedTime(owner_.healthCheckProgressStopwatch_.Elapsed);
            }

            // Continue tracking overall upgrade duration, but reset the elapsed
            // UD and health check times in preparation for processing the next UD.
            // If commit fails, then we will reload the uncommited, non-reset values
            // on retry.
            //
            // Do this after updating both persisted timer values since the health check
            // stopwatch will continue running until the commit completes. Otherwise,
            // the persisted health check value will be off by one interval.
            //
            if (resetUpgradeDomainTimeouts_)
            {
                upgradeContext.ResetUpgradeDomainTimeouts();
            }

            error = storeTx_.Update(upgradeContext);
        }

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx_),
                upgradeContext,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

private:

    void OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = StoreTransaction::EndCommit(operation);

        if (error.IsSuccess() && updateElapsedTimes_)
        {
            // Elapsed time has been persisted - restart in-memory timers
            //
            if (owner_.upgradeProgressStopwatch_.IsRunning)
            {
                owner_.upgradeProgressStopwatch_ = commitStopwatch_;
            }

            if (owner_.healthCheckProgressStopwatch_.IsRunning)
            {
                owner_.healthCheckProgressStopwatch_ = commitStopwatch_;
            }
        }

        this->TryComplete(thisSPtr, error);
    }

    ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody> & owner_;
    bool updateElapsedTimes_;
    bool resetUpgradeDomainTimeouts_;
    Stopwatch commitStopwatch_;
    StoreTransaction storeTx_;
};

template <class TUpgradeContext, class TUpgradeRequestBody>
ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ProcessUpgradeContextAsyncOperationBase(
    __in RolloutManager & rolloutManager,
    __in TUpgradeContext & upgradeContext,
    string const & traceComponent,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root )
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        upgradeContext.ReplicaActivityId,
        timeout,
        callback,
        root)
    , upgradeContext_(upgradeContext)
    , rollforwardRequestBody_()
    , rollbackRequestBody_()
    , traceComponent_(traceComponent)
    , verifiedUpgradeDomains_()
    , imageStoreErrorCount_(0)
    , isConfigOnly_(false)
    , timerSPtr_()
    , timerLock_()
    , healthCheckProgressStopwatch_()
    , upgradeProgressStopwatch_()
    , healthAggregator_()
{
}

template <class TUpgradeContext, class TUpgradeRequestBody>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessUpgradeContextAsyncOperationBase>(operation)->Error;
}

//
// Common upgrade workflow (CM/FM protocol and health checks)
//

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    // Set the health aggregator, which wil be cached and used by the async operation for all duration.
    auto error = this->Replica.GetHealthAggregator(healthAggregator_);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} could not get health aggregator {1}",
            this->TraceId,
            error);
        this->TryComplete(thisSPtr, error);
        return;
    }

    this->StartUpgrade(thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TryRefreshContextAndScheduleRetry(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback)
{
    ErrorCode innerError = this->RefreshUpgradeContext();

    if (!innerError.IsSuccess())
    {
        if (this->ShouldScheduleRetry(innerError, thisSPtr))
        {
            this->StartTimer(
                thisSPtr,
                [this, error, callback](AsyncOperationSPtr const & thisSPtr) { this->TryRefreshContextAndScheduleRetry(error, thisSPtr, callback); },
                this->Replica.GetRandomizedOperationRetryDelay(innerError));
        }
    }
    else
    {
        this->TryScheduleRetry(error, thisSPtr, callback);
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TryScheduleRetry(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback)
{
    if (this->ShouldScheduleRetry(error, thisSPtr))
    {
        this->StartTimer(
            thisSPtr,
            callback,
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ResetUpgradeContext(TUpgradeContext && context)
{
    upgradeContext_ = move(context);

    healthCheckProgressStopwatch_.Stop();
    upgradeProgressStopwatch_.Stop();
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::RestartUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    this->StartUpgrade(thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::StartUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->BeginInitializeUpgrade(
        [this](AsyncOperationSPtr const & operation) { this->OnInitializeUpgradeComplete(operation, false); },
        thisSPtr);
    this->OnInitializeUpgradeComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnInitializeUpgradeComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndInitializeUpgrade(operation);
    if (!error.IsSuccess())
    {
        this->TryScheduleStartUpgrade(error, thisSPtr);
        return;
    }

    error = this->LoadRollforwardMessageBody(rollforwardRequestBody_);
    if (!error.IsSuccess())
    {
        this->TryScheduleStartUpgrade(error, thisSPtr);
        return;
    }

    this->FinishStartUpgrade(thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::FinishStartUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto error = this->OnFinishStartUpgrade(storeTx);
    if (!error.IsSuccess())
    {
        this->TryScheduleStartUpgrade(error, thisSPtr);
        return;
    }

    upgradeContext_.ResetPreparingUpgrade();

    upgradeContext_.ClearIsFMRequestModified();

    upgradeContext_.SetIsHealthPolicyModified(false);

    auto operation = this->BeginUpdateAndCommitUpgradeContext(
        move(storeTx),
        [this](AsyncOperationSPtr const & operation) { this->OnFinishStartUpgradeCommitComplete(operation, false); },
        thisSPtr);
    this->OnFinishStartUpgradeCommitComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnFinishStartUpgradeCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not finish starting upgrade {1}",
            this->TraceId,
            error);

        this->TryScheduleStartUpgrade(error, thisSPtr);

        return;
    }
    else
    {
        // Start tracking upgrade duration after initialization is complete
        //
        upgradeProgressStopwatch_.Restart();

        CMEvents::Trace->UpgradeStart(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        this->TraceQueryableUpgradeStart();
    }
    
    // Signal success to the client now that we've validated the upgrade and
    // accepted the operation for processing. Do not wait for the overall upgrade
    // process to complete since it is a long-running operation.
    //
    upgradeContext_.CompleteClientRequest();

    if (upgradeContext_.IsUpgradeCompletedState)
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else if (upgradeContext_.IsRollforwardState && upgradeContext_.IsAutoBaseline)
    {
        // no need to call FM for auto-baseline upgrades. Just persist the state in CM and exit
        this->FinalizeUpgrade(thisSPtr);
    }
    else if (upgradeContext_.IsRollforwardState)
    {
        this->StartSendRequest(thisSPtr);
    }
    else if (upgradeContext_.IsRollbackState)
    {
        this->PrepareStartRollback(thisSPtr);
    }
    else
    {
        this->Replica.TestAssertAndTransientFault(wformatString(
            "{0} invalid upgrade state {1}", 
            this->TraceId, 
            upgradeContext_.UpgradeState));
        this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::RequestToFM(
    AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RequestToFM(thisSPtr); });
        return;
    }

    MessageUPtr request;
    if (!this->TryGetRequestForFM(thisSPtr, request))
    {
        return;
    }

    auto operation = this->Router.BeginRequestToFM(
        move(request),
        TimeSpan::MaxValue,
        this->GetCommunicationTimeout(),
        [this] (AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
        thisSPtr);
    this->OnRequestToFMComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
struct ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::UpgradeReplyDetails
{
    std::vector<std::wstring> CompletedUpgradeDomains;
    std::vector<std::wstring> PendingUpgradeDomains;
    std::vector<std::wstring> SkippedUpgradeDomains;
    Reliability::UpgradeDomainProgress CurrentUpgradeDomainProgress;
};

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnRequestToFMComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    MessageUPtr reply;
    auto error = this->Router.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        UpgradeReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.ErrorCodeValue;

            if (error.IsSuccess())
            {
                auto replyTrace = wformatString("{0}", body);

                auto const & statusDetails = this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails;
                if (!statusDetails.empty())
                {
                    replyTrace.append(wformatString("\nstatus='{0}'", statusDetails));
                }

                CMEvents::Trace->UpgradeReply(this->ReplicaActivityId, replyTrace);

                auto replyDetails = make_shared<UpgradeReplyDetails>();

                replyDetails->CompletedUpgradeDomains = move(body.CompletedUpgradeDomains);
                replyDetails->PendingUpgradeDomains = move(body.PendingUpgradeDomains);
                replyDetails->SkippedUpgradeDomains = move(body.SkippedUpgradeDomains);
                replyDetails->CurrentUpgradeDomainProgress = move(body.CurrentUpgradeDomainProgress);

                this->ProcessFMReply(move(replyDetails), thisSPtr);

                return;
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} FM upgrade reply body: error={1}",
                    this->TraceId,
                    error);

                if (this->IsFmReplyRetryable(error))
                {
                    error = ErrorCodeValue::OperationCanceled;
                }
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    if (error.IsError(ErrorCodeValue::ApplicationNotFound))
    {
        WriteInfo(
            TraceComponent, 
            "{0} application does not exist: no-op upgrade at FM",
            this->TraceId);

        this->FinishRequestToFM(thisSPtr);  
    }
    else if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RequestToFM(thisSPtr); });
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ProcessFMReply(
    UpgradeReplyDetailsSPtr && replyDetails,
    AsyncOperationSPtr const & thisSPtr)
{
    auto completedUpgradeDomains = move(replyDetails->CompletedUpgradeDomains);
    auto pendingUpgradeDomains = move(replyDetails->PendingUpgradeDomains);
    auto skippedUpgradeDomains = move(replyDetails->SkippedUpgradeDomains);
    auto currentUpgradeDomainProgress = move(replyDetails->CurrentUpgradeDomainProgress);

    vector<wstring> recentlyCompletedUDs;
    vector<wstring> unverifiedUDs;

    for (auto const & ud : completedUpgradeDomains)
    {
        // Find all newly completed UDs and trace to queryable events
        //
        auto findCompleted = find_if(
            upgradeContext_.CompletedUpgradeDomains.begin(),
            upgradeContext_.CompletedUpgradeDomains.end(),
            [&ud] (wstring const & item) -> bool { return item == ud; });
        if (findCompleted == upgradeContext_.CompletedUpgradeDomains.end())
        {
            recentlyCompletedUDs.push_back(ud);
        }

        // Both "unmonitored manual" and "monitored" upgrades from the client
        // perspective map to "monitored" upgrades in terms of CM/FM
        // interaction. In both cases, the FM will wait for the CM to send a
        // list of verified upgrade domains.
        //
        // For unmonitored manual upgrades, the client must explicitly
        // report the list.
        //
        // For monitored upgrades, the CM will internally set this list
        // equal to the list of completed UDs once health checks succeed.
        //
        auto findVerified = find_if(
            verifiedUpgradeDomains_.begin(), 
            verifiedUpgradeDomains_.end(),
            [&ud] (wstring const & item) -> bool { return item == ud; } );
        if (findVerified == verifiedUpgradeDomains_.end())
        {
            unverifiedUDs.push_back(ud);
        }
    }

    // Trace and reset upgrade domain timeouts when there are newly
    // completed UDs
    //
    bool resetUpgradeDomainTimeouts = false;

    if (!recentlyCompletedUDs.empty())
    {
        this->TraceQueryableUpgradeDomainComplete(recentlyCompletedUDs);

        if (!upgradeContext_.UpgradeDescription.IsInternalMonitored || upgradeContext_.IsRollbackState)
        {
            // Reset immediately upon UD completion for auto upgrades
            //
            resetUpgradeDomainTimeouts = true;
        }
    }

    // Do not perform health check or wait for MoveNext() if all completed UDs were skipped.
    // The CM will automatically set the skipped UDs to "verified".
    //
    bool skippedAllCompletedUDs = false;

    if (upgradeContext_.UpgradeDescription.IsHealthMonitored)
    {
        if (!unverifiedUDs.empty() && !skippedUpgradeDomains.empty())
        {
            skippedAllCompletedUDs = true;

            for (auto const & ud : unverifiedUDs)
            {
                auto findSkipped = find_if(
                    skippedUpgradeDomains.begin(),
                    skippedUpgradeDomains.end(),
                    [&ud](wstring const & item) { return item == ud; });
                if (findSkipped == skippedUpgradeDomains.end())
                {
                    skippedAllCompletedUDs = false;
                    break;
                }
            }

            if (skippedAllCompletedUDs)
            {
                unverifiedUDs.clear();
            }
        }
    }

    // The FM currently sorts all upgrade domains lexicographically and proceeds in that order
    //
    if (unverifiedUDs.empty() && !pendingUpgradeDomains.empty())
    {
        upgradeContext_.UpdateInProgressUpgradeDomain(move(pendingUpgradeDomains.front()));
        pendingUpgradeDomains.erase(pendingUpgradeDomains.begin());
    }
    else
    {
        upgradeContext_.UpdateInProgressUpgradeDomain(wstring());

        if (upgradeContext_.UpgradeDescription.IsUnmonitoredManual)
        {
            // Keep the UD duration at 0 while waiting for move-next call or upgrade is complete
            //
            resetUpgradeDomainTimeouts = true;
        }
    }

    upgradeContext_.UpdateCompletedUpgradeDomains(move(completedUpgradeDomains));
    upgradeContext_.UpdatePendingUpgradeDomains(move(pendingUpgradeDomains));

    upgradeContext_.UpdateUpgradeProgress(move(currentUpgradeDomainProgress));

    auto storeTx = this->CreateTransaction();

    auto error = this->OnProcessFMReply(storeTx);
    if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RequestToFM(thisSPtr); });
        return;
    }

    if (skippedAllCompletedUDs)
    {
        // Verify all skipped UDs automatically to bypass health check
        // so that FM can continue upgrading
        //
        auto verifiedDomains = upgradeContext_.CompletedUpgradeDomains;
        
        resetUpgradeDomainTimeouts = true;

        WriteInfo(
            TraceComponent, 
            "{0} skipping verification for no-op UDs: {1}",
            this->TraceId,
            skippedUpgradeDomains);

        this->UpdateVerifiedUpgradeDomains(storeTx, move(verifiedDomains));
    }

    AsyncOperationSPtr operation;
    if (resetUpgradeDomainTimeouts)
    {
        operation = this->BeginResetUpgradeDomainAndCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnFMReplyCommitComplete(operation, false); },
            thisSPtr);
    }
    else
    {
        operation = this->BeginUpdateAndCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnFMReplyCommitComplete(operation, false); },
            thisSPtr);
    }
    this->OnFMReplyCommitComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnFMReplyCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        bool isHealthMonitoredRollforward = (upgradeContext_.UpgradeDescription.IsHealthMonitored && !upgradeContext_.IsRollbackState);

        // CM/FM protocol completion is implied when there are no more pending or in-progress UDs in the FM reply
        //
        bool shouldFinishUpgrade = (upgradeContext_.PendingUpgradeDomains.empty() && upgradeContext_.InProgressUpgradeDomain.empty());

        if (isHealthMonitoredRollforward)
        {
            // For monitored upgrades, we must also perform a final health check after all UDs are complete before finishing
            // the upgrade.
            //
            shouldFinishUpgrade = shouldFinishUpgrade && upgradeContext_.CommonUpgradeContextData.IsPostUpgradeHealthCheckComplete;
        }

        if (shouldFinishUpgrade)
        {
            this->FinishRequestToFM(thisSPtr);
        }
        else
        {
            // CheckHealth() already contains logic for handling upgrade timeouts, so re-use that
            // code path. Once monitored upgrade has failed (upgrade timeout or health check failure),
            // then it will either revert to manual mode or start rolling back, so we will just
            // continue the CM/FM protocol in that case rather than entering this code path again.
            // 
            bool shouldCheckHealth = isHealthMonitoredRollforward
                && (upgradeContext_.InProgressUpgradeDomain.empty() || this->IsUpgradeTimeoutExpired());

            if (shouldCheckHealth)
            {
                healthCheckProgressStopwatch_.Restart();

                this->CheckHealth(thisSPtr);
            }
            else
            {
                this->ScheduleRequestToFM(this->GetUpgradeStatusPollInterval(), thisSPtr);
            }
        }
    }
    else
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RequestToFM(thisSPtr); });
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::FinishRequestToFM(AsyncOperationSPtr const & thisSPtr)
{
    if (upgradeContext_.IsRollforwardState)
    {
        this->FinalizeUpgrade(thisSPtr);
    }
    else if (upgradeContext_.IsRollbackState)
    {
        this->FinalizeRollback(thisSPtr);
    }
    else
    {
        this->Replica.TestAssertAndTransientFault(wformatString(
            "{0} invalid upgrade state {1}", 
            this->TraceId, 
            upgradeContext_.UpgradeState));
        this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::UpdateHealthPolicy(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->BeginUpdateHealthPolicyForHealthCheck(
        [this](AsyncOperationSPtr const & operation){ this->OnUpdateHealthPolicyComplete(operation, false); },
        thisSPtr);
    this->OnUpdateHealthPolicyComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnUpdateHealthPolicyComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndUpdateHealthPolicy(operation);

    if (error.IsSuccess())
    {
        upgradeContext_.SetIsHealthPolicyModified(false);

        // false: Don't schedule. Perform health check immediately after commit
        //        since we are updating a modified policy.
        //
        this->CommitCheckHealthProgress(false, thisSPtr);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to update health policy: {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::CheckHealth(AsyncOperationSPtr const & thisSPtr)
{
    // Use the last in-memory health check result. This makes the health evaluation
    // more accurate even in the face of replication issues. We can still
    // perform health checks since the stopwatches continue to run
    // until the elapsed time has been persisted+replicated, at which
    // point they reset.
    //
    auto lastHealthCheckResult = upgradeContext_.LastHealthCheckResult;

    auto error = this->RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
        return;
    }

    // If there is progress information from the FM, then clear the previous
    // health evaluation results to avoid confusion.
    //
    if (!upgradeContext_.CurrentUpgradeDomainProgress.IsEmpty)
    {
        upgradeContext_.ClearHealthEvaluationReasons();
    }

    upgradeContext_.CommonUpgradeContextData.ClearFailureData();

    // Upgrade can still be interrupted at any time during health checks
    //
    if (upgradeContext_.IsInterrupted)
    {
        CMEvents::Trace->UpgradeInterruption(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        this->PrepareStartRollback(thisSPtr);

        return;
    }

    // Upgrade mode can be changed at any time during healthchecks
    //
    if (upgradeContext_.UpgradeDescription.IsHealthMonitored)
    {
        // Update health evaluation policies if this is the first time health checks are being 
        // performed after upgrade initialization or if the policies were modified. Modified 
        // monitoring policies are automatically picked up by refreshing the upgrade context, 
        // so nothing special needs to be done for them.
        //
        if (upgradeContext_.IsHealthPolicyModified)
        {
            // Update the policy used by HM but do not reset the UD or health check timers
            //
            this->UpdateHealthPolicy(thisSPtr);

            return;
        }
    }
    else
    {
        this->ScheduleRequestToFM(thisSPtr);

        return;
    }
    
    // Upgrade timeout can happen even if the UD hasn't completed and health checks haven't
    // actually started yet.
    //
    UpgradeFailureReason::Enum failureReason;
    if (this->IsUpgradeTimeoutExpired(failureReason))
    {
        WriteWarning(
            TraceComponent, 
            "{0} monitored upgrade timed out ({1}): persisted[overall={2} UD={3} health={4}] stopwatch[upgrade={5} health={6}] timeouts[overall={7} ud={8}]",
            this->TraceId,
            upgradeContext_.UpgradeDescription.MonitoringPolicy.FailureAction,
            upgradeContext_.OverallUpgradeElapsedTime,
            upgradeContext_.UpgradeDomainElapsedTime,
            upgradeContext_.HealthCheckElapsedTime,
            upgradeProgressStopwatch_.Elapsed,
            healthCheckProgressStopwatch_.Elapsed,
            upgradeContext_.UpgradeDescription.MonitoringPolicy.UpgradeTimeout,
            upgradeContext_.UpgradeDescription.MonitoringPolicy.UpgradeDomainTimeout);

        upgradeContext_.CommonUpgradeContextData.FailureReason = failureReason;

        // Keep a snapshot of the progress information from FM at the time of rollback since
        // the current progress will be overwritten once the FM starts rolling back.
        //
        upgradeContext_.CommonUpgradeContextData.UpgradeProgressAtFailure = upgradeContext_.CurrentUpgradeDomainProgress;

        this->FailCheckHealth(thisSPtr);

        return;
    }

    // No interruptions or upgrade modifications have occurred since the last health check
    //
    WriteInfo(
        TraceComponent, 
        "{0} [{1}] monitored upgrade health check: persisted[overall={2} UD={3} health={4}] stopwatch[upgrade={5} health={6}] wait={7}",
        this->TraceId,
        this->GetTraceAnalysisContext(),
        upgradeContext_.OverallUpgradeElapsedTime,
        upgradeContext_.UpgradeDomainElapsedTime,
        upgradeContext_.HealthCheckElapsedTime,
        upgradeProgressStopwatch_.Elapsed,
        healthCheckProgressStopwatch_.Elapsed,
        upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckWaitDuration);

    if (this->IsHealthCheckWaitDurationElapsed())
    {
        bool isHealthy = false;
        vector<HealthEvaluation> unhealthyEvaluations;
        error = this->EvaluateHealth(thisSPtr, isHealthy, unhealthyEvaluations);

        if (!error.IsSuccess())
        {
            this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
            return;
        }

        upgradeContext_.UpdateHealthEvaluationReasons(move(unhealthyEvaluations));

        if (lastHealthCheckResult != isHealthy)
        {
            WriteInfo(
                TraceComponent, 
                "{0} monitored upgrade health check result changed: previous={1} current={2}",
                this->TraceId,
                lastHealthCheckResult,
                isHealthy);

            // The health check result has changed. Setting the elapsed
            // health check time to the wait duration effectively resets
            // both the stable duration and retry timeout being tracked.
            // If either policy is 0, then the health check will pass or
            // fail immediately for the 0 policy.
            //
            // Account for elapsed stopwatch time, which will be added
            // back on commit.
            //
            upgradeContext_.SetHealthCheckElapsedTimeToWaitDuration(healthCheckProgressStopwatch_.Elapsed);
        }

        if (isHealthy)
        {
            if (this->IsHealthCheckStableDurationElapsed())
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} monitored upgrade health check passed",
                    this->TraceId);

                this->FinishCheckHealth(thisSPtr);

                return;
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} monitored upgrade health check waiting for stable duration: elapsed[persisted={1} stopwatch={2}] stable={3}",
                    this->TraceId,
                    upgradeContext_.HealthCheckElapsedTime,
                    healthCheckProgressStopwatch_.Elapsed,
                    upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckStableDuration);

                // no-op: do not return - continue and commit the elapsed health check time so far
            }
        }
        else
        {
            if (this->IsHealthCheckRetryTimeoutExpired())
            {
                // health reason details will show up in HM warning traces
                //
                WriteWarning(
                    TraceComponent, 
                    "{0} monitored upgrade health check failed: wait={1} stable={2} retry={3}",
                    this->TraceId,
                    upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckWaitDuration,
                    upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckStableDuration,
                    upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckRetryTimeout);

                upgradeContext_.CommonUpgradeContextData.FailureReason = UpgradeFailureReason::HealthCheck;

                // No need to snapshot health events since there is no monitoring during rollback, so
                // the health evaluation details at the time of rollback will not be overwritten.

                this->FailCheckHealth(thisSPtr);

                return;
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} monitored upgrade health check retrying: elapsed[persisted={1} stopwatch={2}] retry={3}",
                    this->TraceId,
                    upgradeContext_.HealthCheckElapsedTime,
                    healthCheckProgressStopwatch_.Elapsed,
                    upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckRetryTimeout);

                // no-op: do not return - continue and commit the elapsed health check time so far
            }
        }

        upgradeContext_.SetLastHealthCheckResult(isHealthy);

    } // wait duration elapsed

    // true: Schedule next CheckHealth after configured health check interval
    //
    this->CommitCheckHealthProgress(true, thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::CommitCheckHealthProgress(bool useDelay, AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto operation = this->BeginUpdateAndCommitUpgradeContext(
        move(storeTx),
        [this, useDelay](AsyncOperationSPtr const & operation) { this->OnCheckHealthCommitComplete(useDelay, operation, false); },
        thisSPtr);
    this->OnCheckHealthCommitComplete(useDelay, operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnCheckHealthCommitComplete(
    bool useDelay,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        this->ScheduleCheckHealth(
            (useDelay ? this->GetHealthCheckInterval() : TimeSpan::Zero), 
            thisSPtr);
    }
    else
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::FinishCheckHealth(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    if (upgradeContext_.PendingUpgradeDomains.empty())
    {
        upgradeContext_.CommonUpgradeContextData.IsPostUpgradeHealthCheckComplete = true;
    }

    auto verifiedDomains = upgradeContext_.CompletedUpgradeDomains;
    auto error = this->UpdateVerifiedUpgradeDomains(storeTx, move(verifiedDomains));

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not update verified upgrade domains due to {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
        return;
    }

    auto operation = this->BeginResetUpgradeDomainAndCommitUpgradeContext(
        move(storeTx),
        [this](AsyncOperationSPtr const & operation) { this->OnFinishCheckHealthCommitComplete(operation, false); },
        thisSPtr);
    this->OnFinishCheckHealthCommitComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnFinishCheckHealthCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        // Only stop health check timer once the commit has succeeded and
        // we stop (potentially) retrying the health check logic 
        //
        healthCheckProgressStopwatch_.Stop();

        this->ScheduleRequestToFM(thisSPtr);
    }
    else
    {
        // Start over from health check step since the upgrade can still
        // get interrupted or time out while trying to commit
        //
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::FailCheckHealth(AsyncOperationSPtr const & thisSPtr)
{
    // Set upgrade state that will trigger the failure action once committed
    //
    switch (upgradeContext_.UpgradeDescription.MonitoringPolicy.FailureAction)
    {
        case MonitoredUpgradeFailureAction::Rollback:
            //
            // commonUpgradeContextData_.IsRollbackAllowed should normally be checked before starting a rollback. 
            // In this case, health check won't trigger upon failover after UD completes and delete default service starts.
            //
            // Goal state is irrelevant since we are failing a monitored upgrade rather than interrupting.
            //
            upgradeContext_.SetRollingBack(false); // goalStateExists
            break;

        case MonitoredUpgradeFailureAction::Manual:
            upgradeContext_.RevertToManualUpgrade();
            break;

        default:
            WriteWarning(
                TraceComponent, 
                "{0} invalid failure action - defaulting to Manual",
                this->TraceId);
            upgradeContext_.RevertToManualUpgrade();
            break;
    }

    this->TrySetCommonContextDataForFailure();
    
    auto storeTx = this->CreateTransaction();

    // clear verified domains
    auto error = this->UpdateVerifiedUpgradeDomains(storeTx, vector<wstring>());

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not update verified upgrade domains due to {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
        return;
    }

    auto operation = this->BeginResetUpgradeDomainAndCommitUpgradeContext(
        move(storeTx),
        [this](AsyncOperationSPtr const & operation) { this->OnFailCheckHealthCommitComplete(operation, false); },
        thisSPtr);
    this->OnFailCheckHealthCommitComplete(operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnFailCheckHealthCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        // Only stop health check timer once we commit has succeeded and
        // we stop (potentially) retrying the health check logic 
        //
        healthCheckProgressStopwatch_.Stop();

        if (upgradeContext_.IsRollbackState)
        {
            this->PrepareStartRollback(thisSPtr);
        }
        else
        {
            this->ScheduleRequestToFM(thisSPtr);
        }
    }
    else
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CheckHealth(thisSPtr); });
    }
}

//
// Retry helper functions
//

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TryScheduleStartUpgrade(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr)
{
    if (this->IsRetryable(error))
    {
        WriteNoise(
            TraceComponent, 
            "{0} scheduling prepare upgrade retry: error={1}",
            this->TraceId,
            error);

        // We cannot start rolling back because we haven't been able to create the appropriate
        // message bodies yet.
        //
        // We must instead retry preparing to upgrade since we may be recovering the context
        // from disk, which means that there could be a pending upgrade at the FM. In this case,
        // we cannot simply fail this operation without getting an authoritative upgrade
        // success/fail from the FM.
        //
        this->StartTimer(
            thisSPtr,
            [this](AsyncOperationSPtr const & thisSPtr){ this->StartUpgrade(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
    else if (this->IsValidationError(error))
    {
        this->HandleValidationFailure(error, thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::StartSendRequest(
    AsyncOperationSPtr const & thisSPtr)
{
    this->ScheduleRequestToFM(thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ScheduleRequestToFM(
    AsyncOperationSPtr const & thisSPtr)
{
    this->ScheduleRequestToFM(TimeSpan::Zero, thisSPtr);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ScheduleRequestToFM(
    TimeSpan const delay,
    AsyncOperationSPtr const & thisSPtr)
{
    this->StartTimer(
        thisSPtr, 
        [this](AsyncOperationSPtr const & thisSPtr){ this->RequestToFM(thisSPtr); }, 
        delay);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ScheduleCheckHealth(TimeSpan const delay, AsyncOperationSPtr const & thisSPtr)
{
    this->StartTimer(thisSPtr, [this](AsyncOperationSPtr const & thisSPtr){ this->CheckHealth(thisSPtr); }, delay);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ScheduleHandleValidationFailure(ErrorCode const & validationError, AsyncOperationSPtr const & thisSPtr)
{
    this->StartTimer(thisSPtr, [this, validationError](AsyncOperationSPtr const & thisSPtr){ this->HandleValidationFailure(validationError, thisSPtr); }, this->Replica.GetRandomizedOperationRetryDelay(validationError));
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::HandleValidationFailure(ErrorCode const & validationError, AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->BeginOnValidationError(
        [this, validationError](AsyncOperationSPtr const & operation) { this->OnHandleValidationFailureComplete(validationError, operation, false); },
        thisSPtr);
    this->OnHandleValidationFailureComplete(validationError, operation, true);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::OnHandleValidationFailureComplete(
    ErrorCode const & validationError,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndOnValidationError(operation);

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, validationError);
    }
    else if (this->IsRetryable(error))
    {
        WriteNoise(
            TraceComponent, 
            "{0} scheduling retry: error = {1}",
            this->TraceId,
            error);

        this->ScheduleHandleValidationFailure(validationError, thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} failed to cleanup on validation error: validation={1} cleanup={2}",
            this->TraceId,
            validationError,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ShouldScheduleRetry(ErrorCode const & error, AsyncOperationSPtr const & thisSPtr)
{
    if (this->IsRetryable(error))
    {
        if (this->UpgradeContext.IsInterrupted)
        {
            // If we are already rolling back, then do not commit and restart the rollback.
            // Just continue retrying the rollback.
            //
            if (this->UpgradeContext.IsRollbackState)
            {
                WriteNoise(
                    TraceComponent, 
                    "{0} continuing rollback: error = {1}",
                    this->TraceId,
                    error);

                return true;
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} starting rollback: error = {1}",
                    this->TraceId,
                    error);

                this->PrepareStartRollback(thisSPtr);
                return false;
            }
        }
        else
        {
            WriteNoise(
                TraceComponent, 
                "{0} scheduling retry: error = {1}",
                this->TraceId,
                error);

            return true;
        }
    }
    else
    {
        thisSPtr->TryComplete(thisSPtr, error);

        return false;
    }
}

//
// Workflow helper functions
//

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TryGetRequestForFM(
    AsyncOperationSPtr const & thisSPtr,
    __out MessageUPtr & request)
{
    // Check for upgrade interruption and start internal rollback if necessary
    //
    if (upgradeContext_.IsInterrupted && !upgradeContext_.IsRollbackState)
    {
        // We are either still rolling forward or have finished rolling
        // back default services. In both cases, we must rollback changes
        // made at the FM or the upgrade delta calculations for subsequent
        // upgrade requests may be incorrect.
        //
        CMEvents::Trace->UpgradeInterruption(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        this->PrepareStartRollback(thisSPtr);

        return false;
    }

    // Check for upgrade description modification and update request message 
    // bodies if necessary
    //
    if (upgradeContext_.IsFMRequestModified)
    {
        this->ModifyRequestToFMBodies();
    }

    auto verifiedDomains = verifiedUpgradeDomains_;

    wstring requestTrace;
    if (upgradeContext_.IsRollforwardState)
    {
        rollforwardRequestBody_.UpdateVerifiedUpgradeDomains(move(verifiedDomains));
        request = this->GetUpgradeMessageTemplate().CreateMessage(rollforwardRequestBody_);
        requestTrace = wformatString("{0}", rollforwardRequestBody_);
    }
    else if (upgradeContext_.IsRollbackState)
    {
        rollbackRequestBody_.UpdateVerifiedUpgradeDomains(move(verifiedDomains));
        request = this->GetUpgradeMessageTemplate().CreateMessage(rollbackRequestBody_);
        requestTrace = wformatString("{0}", rollbackRequestBody_);
    }
    else
    {
        this->Replica.TestAssertAndTransientFault(wformatString(
            "{0} invalid upgrade state {1}", 
            this->TraceId, 
            upgradeContext_.UpgradeState));

        this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);

        return false;
    }

    CMEvents::Trace->UpgradeRequest(this->ReplicaActivityId, this->GetTraceAnalysisContext(), requestTrace);

    return true;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ModifyRequestToFMBodies()
{
    auto upgradeType = this->GetUpgradeType();
    auto isMonitored = upgradeContext_.UpgradeDescription.IsInternalMonitored;
    auto isManual = upgradeContext_.UpgradeDescription.IsUnmonitoredManual;
    auto replicaSetCheckTimeoutRollforward = this->GetRollforwardReplicaSetCheckTimeout();
    auto replicaSetCheckTimeoutRollback = this->GetRollbackReplicaSetCheckTimeout();
    auto sequenceNumber = upgradeContext_.SequenceNumber;

    WriteInfo(
        TraceComponent, 
        "{0} modifying request bodies: type={1} isMonitored={2} isManual={3} replicaSetCheckTimeout=[forward={4}, back={5}] seq={6}",
        this->TraceId,
        upgradeType,
        isMonitored,
        isManual,
        replicaSetCheckTimeoutRollforward,
        replicaSetCheckTimeoutRollback,
        sequenceNumber);

    rollforwardRequestBody_.MutableSpecification.SetUpgradeType(upgradeType);
    rollforwardRequestBody_.MutableSpecification.SetIsMonitored(isMonitored);
    rollforwardRequestBody_.MutableSpecification.SetIsManual(isManual);
    rollforwardRequestBody_.SetReplicaSetCheckTimeout(replicaSetCheckTimeoutRollforward);
    rollforwardRequestBody_.SetSequenceNumber(sequenceNumber);

    // Just modify rollback body regardless of whether we're actually rolling back or not.
    // If we are, then the updated body is used.
    // If we are not, then the body will be loaded with the most recent values at rollback time.
    //
    rollbackRequestBody_.MutableSpecification.SetUpgradeType(upgradeType);
    rollbackRequestBody_.MutableSpecification.SetIsMonitored(isMonitored);
    rollbackRequestBody_.MutableSpecification.SetIsManual(isManual);
    rollbackRequestBody_.SetReplicaSetCheckTimeout(replicaSetCheckTimeoutRollback);
    rollbackRequestBody_.SetSequenceNumber(sequenceNumber);

    upgradeContext_.ClearIsFMRequestModified();
}

template <class TUpgradeContext, class TUpgradeRequestBody>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::LoadRollbackMessageBody()
{
    return this->LoadRollbackMessageBody(rollbackRequestBody_);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::RefreshUpgradeContext()
{
    auto storeTx = this->CreateTransaction();

    auto error = upgradeContext_.Refresh(storeTx);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} unable to refresh upgrade context due to {1}",
            this->TraceId,
            error);

        return error;
    }

    error = this->LoadVerifiedUpgradeDomains(storeTx, verifiedUpgradeDomains_);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        verifiedUpgradeDomains_.clear();

        error = ErrorCodeValue::Success;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} unable to read verified upgrade domains due to {1}",
            this->TraceId,
            error);

        return error;
    }

    if (error.IsSuccess())
    {
        error = this->OnRefreshUpgradeContext(storeTx);
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }

    return error;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::SetCommonContextDataForUpgrade()
{
    if (this->UpgradeContext.CommonUpgradeContextData.StartTime == DateTime::Zero)
    {
        this->UpgradeContext.CommonUpgradeContextData.StartTime = DateTime::Now(); // UTC
        this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = L"";
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TrySetCommonContextDataForFailure()
{
    if (this->UpgradeContext.CommonUpgradeContextData.FailureTime == DateTime::Zero)
    {
        this->UpgradeContext.CommonUpgradeContextData.FailureTime = DateTime::Now(); // UTC

        return true;
    }
    else
    {
        return false;
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::SetCommonContextDataForRollback(bool goalStateExists)
{
    this->UpgradeContext.SetRollingBack(goalStateExists);

    if (this->TrySetCommonContextDataForFailure() && this->UpgradeContext.IsInterrupted)
    {
        this->UpgradeContext.UpdateInProgressUpgradeDomain(L"");
        this->UpgradeContext.UpdateCompletedUpgradeDomains(vector<wstring>());
        this->UpgradeContext.UpdatePendingUpgradeDomains(vector<wstring>());

        this->UpgradeContext.CommonUpgradeContextData.FailureReason = UpgradeFailureReason::Interrupted;
    }
}

//
// Upgrade progress helper functions
//

template <class TUpgradeContext, class TUpgradeRequestBody>
UpgradeType::Enum ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetUpgradeType() const
{
    return (upgradeContext_.UpgradeDescription.UpgradeType == UpgradeType::Rolling && this->IsConfigOnly)
        ? UpgradeType::Rolling_NotificationOnly
        : upgradeContext_.UpgradeDescription.UpgradeType;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
TimeSpan ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetRollforwardReplicaSetCheckTimeout() const
{
    return upgradeContext_.UpgradeDescription.ReplicaSetCheckTimeout;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
TimeSpan ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetRollbackReplicaSetCheckTimeout() const
{
    return this->Replica.GetRollbackReplicaSetCheckTimeout(upgradeContext_.UpgradeDescription.ReplicaSetCheckTimeout);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
AsyncOperationSPtr ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::BeginUpdateAndCommitUpgradeContext(
    StoreTransaction && storeTx,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return CommitUpgradeContextAsyncOperation::CreateForUpdateAndCommit(
        *this,
        move(storeTx),
        callback,
        parent);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
AsyncOperationSPtr ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::BeginResetUpgradeDomainAndCommitUpgradeContext(
    StoreTransaction && storeTx,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return CommitUpgradeContextAsyncOperation::CreateForUpgradeDomainReset(
        *this,
        move(storeTx),
        callback,
        parent);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
AsyncOperationSPtr ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::BeginCommitUpgradeContext(
    StoreTransaction && storeTx,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return CommitUpgradeContextAsyncOperation::CreateForCommitOnly(
        *this,
        move(storeTx),
        callback,
        parent);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::EndCommitUpgradeContext(
    AsyncOperationSPtr const & operation)
{
    return CommitUpgradeContextAsyncOperation::End(operation);
}


template
ErrorCode ProcessUpgradeContextAsyncOperationBase<ApplicationUpgradeContext, Reliability::UpgradeApplicationRequestMessageBody>::ClearVerifiedUpgradeDomains<StoreDataVerifiedUpgradeDomains>(
    StoreTransaction const &, 
    __inout StoreDataVerifiedUpgradeDomains &);

template
ErrorCode ProcessUpgradeContextAsyncOperationBase<FabricUpgradeContext, Reliability::UpgradeFabricRequestMessageBody>::ClearVerifiedUpgradeDomains<StoreDataVerifiedFabricUpgradeDomains>(
    StoreTransaction const &, 
    __inout StoreDataVerifiedFabricUpgradeDomains &);

template <class TUpgradeContext, class TUpgradeRequestBody>
template <class TStoreData>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ClearVerifiedUpgradeDomains(
    StoreTransaction const & storeTx,
    __inout TStoreData & storeData)
{
    verifiedUpgradeDomains_.clear();

    return this->Replica.ClearVerifiedUpgradeDomains(storeTx, storeData);
}

//
// Tracing and Timer helper functions
//

template <class TUpgradeContext, class TUpgradeRequestBody>
ErrorCode ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::TraceAndGetErrorDetails(
    ErrorCodeValue::Enum error, 
    std::wstring && msg) const
{
    return ProcessRolloutContextAsyncOperationBase::TraceAndGetErrorDetails(TraceComponent, error, move(msg));
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsHealthCheckWaitDurationElapsed() const
{
    auto waitDuration = upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckWaitDuration;

    auto stopwatch = healthCheckProgressStopwatch_.Elapsed;
    auto totalElapsed = upgradeContext_.HealthCheckElapsedTime.AddWithMaxAndMinValueCheck(stopwatch);

    return (totalElapsed >= waitDuration);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsHealthCheckStableDurationElapsed() const
{
    auto waitDuration = upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckWaitDuration;
    auto stableDuration = upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckStableDuration;
    auto totalDuration = waitDuration.AddWithMaxAndMinValueCheck(stableDuration);

    auto stopwatch = healthCheckProgressStopwatch_.Elapsed;
    auto totalElapsed = upgradeContext_.HealthCheckElapsedTime.AddWithMaxAndMinValueCheck(stopwatch);

    return (totalElapsed >= totalDuration);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsHealthCheckRetryTimeoutExpired() const
{
    auto waitDuration = upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckWaitDuration;
    auto retryTimeout = upgradeContext_.UpgradeDescription.MonitoringPolicy.HealthCheckRetryTimeout;
    auto totalTimeout = waitDuration.AddWithMaxAndMinValueCheck(retryTimeout);

    auto stopwatch = healthCheckProgressStopwatch_.Elapsed;
    auto totalElapsed = upgradeContext_.HealthCheckElapsedTime.AddWithMaxAndMinValueCheck(stopwatch);

    return (totalElapsed >= totalTimeout);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsOverallUpgradeTimeoutExpired() const
{
    auto upgradeTimeout = upgradeContext_.UpgradeDescription.MonitoringPolicy.UpgradeTimeout;

    auto stopwatch = upgradeProgressStopwatch_.Elapsed;
    auto totalElapsed = upgradeContext_.OverallUpgradeElapsedTime.AddWithMaxAndMinValueCheck(stopwatch);

    return (totalElapsed >= upgradeTimeout);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsUpgradeDomainTimeoutExpired() const
{
    auto udTimeout = upgradeContext_.UpgradeDescription.MonitoringPolicy.UpgradeDomainTimeout;

    auto stopwatch = upgradeProgressStopwatch_.Elapsed;
    auto totalElapsed = upgradeContext_.UpgradeDomainElapsedTime.AddWithMaxAndMinValueCheck(stopwatch);

    return (totalElapsed >= udTimeout);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsUpgradeTimeoutExpired() const
{
    UpgradeFailureReason::Enum unused;
    return this->IsUpgradeTimeoutExpired(unused);
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsUpgradeTimeoutExpired(
    __out UpgradeFailureReason::Enum & failureReason) const
{
    if (this->IsUpgradeDomainTimeoutExpired())
    {
        failureReason = UpgradeFailureReason::UpgradeDomainTimeout;
        return true;
    }
    else if (this->IsOverallUpgradeTimeoutExpired())
    {
        failureReason = UpgradeFailureReason::OverallUpgradeTimeout;
        return true;
    }

    failureReason = UpgradeFailureReason::None;
    return false;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
TimeSpan ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetCommunicationTimeout() const
{
    auto timeout = this->GetImageBuilderTimeout();

    auto maxTimeout = ManagementConfig::GetConfig().MaxCommunicationTimeout;
    if (timeout > maxTimeout)
    {
        timeout = maxTimeout;
    }

    return timeout;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
TimeSpan ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetImageBuilderTimeout() const
{
    return upgradeContext_.OperationTimeout;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::ResetImageStoreErrorCount()
{
    imageStoreErrorCount_ = 0;
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsRetryable(ErrorCode const & error)
{
    if (this->IsValidationError(error) || this->IsReconfiguration(error))
    {
        return false;
    }

    switch (error.ReadValue())
    {
        // unexpected errors
        case ErrorCodeValue::OperationFailed:
        case ErrorCodeValue::ApplicationNotFound:
        case ErrorCodeValue::InvalidNameUri:
        case ErrorCodeValue::ImageBuilderUnexpectedError:

        // non-retryable ImageStore errors
        case ErrorCodeValue::AccessDenied:
        case ErrorCodeValue::InvalidArgument:
        case ErrorCodeValue::InvalidDirectory:
        case ErrorCodeValue::PathTooLong:

        // Network validation error codes
        case ErrorCodeValue::NetworkNotFound:

        //
        // IB is responsible for its own cleanup on internal
        // validation errors, so don't call the application cleanup
        // handler on this error (return false for IsValidationError). 
        // However, also ensure that we do not retry on this error.
        //
        case ErrorCodeValue::ImageBuilderValidationError:
            return false;
        
        // potentially transient inaccessible ImageStore errors
        case ErrorCodeValue::FileNotFound:
        case ErrorCodeValue::DirectoryNotFound:
            if (imageStoreErrorCount_ < ManagementConfig::GetConfig().ImageStoreErrorDuringUpgradeRetryCount)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} ImageStore error: retry count = {1}/{2}",
                    this->TraceId,
                    imageStoreErrorCount_,
                    ManagementConfig::GetConfig().ImageStoreErrorDuringUpgradeRetryCount);

                ++imageStoreErrorCount_;
                return true;
            }

            return false;

        default:
            // Retry on all other unknown errors
            return true;
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsFmReplyRetryable(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::OperationFailed:
        case ErrorCodeValue::ApplicationNotFound:
            return false;

        default:
            // Retry on all other errors in the FM reply body
            return true;
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsValidationError(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::ApplicationUpgradeValidationError:
        case ErrorCodeValue::FabricUpgradeValidationError:
            return true;

        default:
            return false;
    }
}

template<class TUpgradeContext, class TUpgradeRequestBody>
bool ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::IsServiceOperationFatalError(ErrorCode const & error) const
{
    switch (error.ReadValue())
    {
        // PLB
        //
        case ErrorCodeValue::ConstraintKeyUndefined:
        case ErrorCodeValue::InsufficientClusterCapacity:
        case ErrorCodeValue::ServiceAffinityChainNotSupported:
        //
        // Naming (UpdateService)
        //
        case ErrorCodeValue::InvalidArgument:
            return true;

        default:
            return false;
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
void ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::StartTimer(
    AsyncOperationSPtr const & thisSPtr,
    RetryCallback const & callback,
    TimeSpan const delay)
{
    if (!this->IsRolloutManagerActive())
    {
        //
        // When there is a blip in the replica state P->!P->P, a new primary 
        // recovery process starts. This subsequent recovery will drain
        // any outstanding context processing before proceeding. Avoid blocking 
        // the new recovery process behind upgrades, which may be long running 
        // and not observe short state blips.
        //
        WriteInfo(
            TraceComponent, 
            "{0} rollout manager no longer active",
            this->TraceId);

        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::CMRequestAborted);
    }
    else if (upgradeContext_.IsExternallyFailed())
    {
        //
        // Allow forcefully failing the upgrade state-machine to unblock
        // other higher priority work - such as deleting an application.
        // This state will be persisted by the rollout manager.
        //
        WriteInfo(
            TraceComponent, 
            "{0} upgrade externally failed",
            this->TraceId);

        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
    }
    else
    {
        AcquireExclusiveLock lock(timerLock_);
        
        if (!this->InternalIsCompleted)
        {
            WriteNoise(
                TraceComponent, 
                "{0} scheduling upgrade step in {1}",
                this->TraceId,
                delay);

            auto root = this->Replica.CreateComponentRoot();
            timerSPtr_ = Timer::Create(TimerTagDefault, [this, thisSPtr, callback, root] (TimerSPtr const & timer) 
            { 
                timer->Cancel();
                callback(thisSPtr);
            },
            true); // allow concurrency
            timerSPtr_->Change(delay);
        }
    }
}

template <class TUpgradeContext, class TUpgradeRequestBody>
wstring ProcessUpgradeContextAsyncOperationBase<TUpgradeContext, TUpgradeRequestBody>::GetUpgradeContextStringTrace() const
{
    return wformatString("{0}", upgradeContext_);
}

template class Management::ClusterManager::ProcessUpgradeContextAsyncOperationBase<
    ApplicationUpgradeContext,
    Reliability::UpgradeApplicationRequestMessageBody>;

template class Management::ClusterManager::ProcessUpgradeContextAsyncOperationBase<
    FabricUpgradeContext,
    Reliability::UpgradeFabricRequestMessageBody>;
