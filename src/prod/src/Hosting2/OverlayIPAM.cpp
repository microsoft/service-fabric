// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const OverlayIPAMProvider("OverlayIPAM");

// ********************************************************************************************************************
// OverlayIPAM::ReserveWithRetryAsyncOperation Implementation
// ********************************************************************************************************************
class OverlayIPAM::ReserveWithRetryAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ReserveWithRetryAsyncOperation)

public:
    ReserveWithRetryAsyncOperation(
        OverlayIPAM & owner,
        std::wstring const & reservationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        reservationId_(reservationId),
        retryCount_(0)
    {
    }

    virtual ~ReserveWithRetryAsyncOperation()
    {
    }

    static ErrorCode ReserveWithRetryAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        OverlayNetworkResourceSPtr & networkResource)
    {
        auto thisPtr = AsyncOperation::End<ReserveWithRetryAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            networkResource = move(thisPtr->networkResource_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto reserveErrorCode = this->owner_.Reserve(this->reservationId_, this->networkResource_);

        if (reserveErrorCode.ReadValue() == ErrorCodeValue::NotReady)
        {
            retryCount_++;
            if (retryCount_ > HostingConfig::GetConfig().ReserveNetworkResourceRetryCount)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }

            if (retryTimer_ != nullptr)
            {
                retryTimer_->Cancel();
            }

            int64 delay = HostingConfig::GetConfig().ReserveOverlayNetworkResourceRetryInterval.Ticks * retryCount_;

            WriteInfo(
                OverlayIPAMProvider,
                "Retrying request to reserve overlay network resource. Attempt: {0} Delay(ticks): {1}",
                retryCount_,
                delay);

            AcquireExclusiveLock lock(this->owner_.get_StateLock());
            auto state = this->owner_.State.get_Value_CallerHoldsLock();
            if (state == FabricComponentState::Opened)
            {
                retryTimer_ = Timer::Create(
                    "Hosting.OverlayIPAMReserve.Restart",
                    [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->OnStart(thisSPtr);
                });
                retryTimer_->Change(TimeSpan::FromTicks(delay));
            }
        }
        else
        {
            TryComplete(thisSPtr, reserveErrorCode);
        }
    }

private:
    OverlayIPAM & owner_;
    OverlayNetworkResourceSPtr networkResource_;
    Common::TimerSPtr retryTimer_;
    std::wstring reservationId_;
    TimeoutHelper timeoutHelper_;
    int retryCount_;
};

// ********************************************************************************************************************
// OverlayIPAM::ReleaseWithRetryAsyncOperation Implementation
// ********************************************************************************************************************
class OverlayIPAM::ReleaseWithRetryAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ReleaseWithRetryAsyncOperation)

public:
    ReleaseWithRetryAsyncOperation(
        OverlayIPAM & owner,
        wstring reservationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        reservationId_(reservationId),
        timeoutHelper_(timeout),
        retryCount_(0)
    {
    }

    virtual ~ReleaseWithRetryAsyncOperation()
    {
    }

    static ErrorCode ReleaseWithRetryAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ReleaseWithRetryAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto releaseErrorCode = this->owner_.Release(this->reservationId_);
        if (releaseErrorCode.ReadValue() == ErrorCodeValue::NotReady)
        {
            retryCount_++;
            if (retryCount_ > HostingConfig::GetConfig().ReleaseNetworkResourceRetryCount)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }

            if (retryTimer_ != nullptr)
            {
                retryTimer_->Cancel();
            }

            int64 delay = HostingConfig::GetConfig().ReleaseOverlayNetworkResourceRetryInterval.Ticks * retryCount_;

            WriteInfo(
                OverlayIPAMProvider,
                "Retrying request to release overlay network resource. Attempt: {0} Delay(ticks): {1}",
                retryCount_,
                delay);

            AcquireExclusiveLock lock(this->owner_.get_StateLock());
            auto state = this->owner_.State.get_Value_CallerHoldsLock();
            if (state == FabricComponentState::Opened)
            {
                retryTimer_ = Timer::Create(
                    "Hosting.OverlayIPAMRelease.Restart",
                    [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->OnStart(thisSPtr);
                });
                retryTimer_->Change(TimeSpan::FromTicks(delay));
            }
        }
        else
        {
            TryComplete(thisSPtr, releaseErrorCode);
        }
    }

private:
    OverlayIPAM & owner_;
    TimeoutHelper timeoutHelper_;
    Common::TimerSPtr retryTimer_;
    int retryCount_;
    wstring reservationId_;
};

OverlayIPAM::OverlayIPAM(
    ComponentRootSPtr const & root,
    InternalReplenishNetworkResourcesCallback const & internalReplenishNetworkResourcesCallback,
    GhostChangeCallback const & ghostChangeCallback,
    __in Common::TimeSpan const & poolRefreshRetryInterval)
    : RootedObject(*root),
      internalReplenishNetworkResourcesCallback_(internalReplenishNetworkResourcesCallback),
      ghostChangeCallback_(ghostChangeCallback),
      poolRefreshRetryInterval_(poolRefreshRetryInterval),
      lock_(),
      initialized_(false),
      lastRefreshTime_(DateTime::Zero),
      ghosts_(),
      pool_(),
      updatePoolTimerSPtr_()
{
}

OverlayIPAM::~OverlayIPAM()
{
    ASSERT_IF(this->updatePoolTimerSPtr_, "OverlayIPAM::~OverlayIPAM timer is set.");
}

Common::AsyncOperationSPtr OverlayIPAM::BeginReserveWithRetry(
    std::wstring const & reservationId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ReserveWithRetryAsyncOperation>(
        *this,
        reservationId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode OverlayIPAM::EndReserveWithRetry(Common::AsyncOperationSPtr const & operation, OverlayNetworkResourceSPtr & networkResource)
{
    return ReserveWithRetryAsyncOperation::End(operation, networkResource);
}

ErrorCode OverlayIPAM::Reserve(wstring const & reservationId, OverlayNetworkResourceSPtr & networkResource)
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        if (!this->initialized_)
        {
            return ErrorCode(ErrorCodeValue::NotReady);
        }
    }

    // Check if pool is already down to 0. 
    // If so, cancel existing refresh timer, call replenish and return NotReady status.
    if (this->pool_.FreeCount == 0)
    {
        this->CleanupPoolRefreshTimer();
        this->ReplenishReservationPool();
        { //lock
            AcquireExclusiveLock lock(this->lock_);

            if (!this->initialized_)
            {
                return ErrorCode(ErrorCodeValue::NotReady);
            }
        }
    }

    if (this->pool_.Reserve(reservationId, networkResource))
    {
        // Make this that this is not listed in the ghost list either.
        //
        RemoveGhostIf(reservationId);

        return ErrorCode(ErrorCodeValue::Success);
    }

    WriteWarning(
        OverlayIPAMProvider,
        "Failed to find a network resource to reserve for id {0}, current pool counts (approx): total: {1}, available: {2}",
        reservationId,
        this->pool_.TotalCount,
        this->pool_.FreeCount);

    return ErrorCode(ErrorCodeValue::OperationFailed);
}

Common::AsyncOperationSPtr OverlayIPAM::BeginReleaseWithRetry(
    std::wstring const & reservationId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ReleaseWithRetryAsyncOperation>(
        *this,
        reservationId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode OverlayIPAM::EndReleaseWithRetry(
    Common::AsyncOperationSPtr const & operation)
{
    return ReleaseWithRetryAsyncOperation::End(operation);
}

ErrorCode OverlayIPAM::Release(wstring const & reservationId)
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        if (!this->initialized_)
        {
            return ErrorCode(ErrorCodeValue::NotReady);
        }
    }

    if (!this->pool_.Release(reservationId))
    {
        // It may be a release of a ghost.  Check that here, and remove it if
        // it is.  If it is still not found, that is also fine.
        //
        RemoveGhostIf(reservationId);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

DateTime OverlayIPAM::get_LastRefreshTime()
{
    AcquireExclusiveLock lock(this->lock_);

    return this->lastRefreshTime_;
}

int OverlayIPAM::get_GhostCount()
{
    AcquireExclusiveLock lock(this->lock_);

    return (int) this->ghosts_.size();
}

list<wstring> OverlayIPAM::GetGhostReservations()
{
    list<wstring> ghosts;

    { //lock
        AcquireExclusiveLock lock(this->lock_);

        for (auto const & item : this->ghosts_)
        {
            ghosts.push_back(item);
        }
    }

    return ghosts;
}

void OverlayIPAM::RemoveGhostIf(wstring const & reservationId)
{
    AcquireExclusiveLock lock(this->lock_);

    auto item = this->ghosts_.find(reservationId);
    if (item != this->ghosts_.end())
    {
        this->ghosts_.erase(item);
    }
}

void OverlayIPAM::OnNewIpamData(std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeAdded, 
    std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeRemoved)
{
    int added;
    int removed;
    bool callbackNeeded = false;
    DateTime refreshTime;

    { //lock
        AcquireExclusiveLock lock(this->lock_);

        // Remember the number of currently known ghosts, to use to determine if
        // the list has changed.
        //
        // Note that this approach works for the initial call because there are
        // no existing reservations to declare ghosts, and no pre-existing ghosts
        // so the ghost list will never have changed.
        //
        auto ghostsCopy = this->ghosts_;

        // Add new network resources
        this->pool_.AddNetworkResources(networkResourcesToBeAdded, networkResourcesToBeRemoved, this->ghosts_, added, removed);

        // if the new conflict list is any different from the existing ghost list
        // then flag to issue an update notification
        //
        callbackNeeded = this->ghosts_ != ghostsCopy;

        // Update the refresh time.
        refreshTime = DateTime::Now();
        this->lastRefreshTime_ = refreshTime;

        if (added > 0 || removed > 0 || callbackNeeded)
        {
            // We have a change in the pool, so dump much detailed information
            // about the resulting state.
            //
            WriteInfo(
                OverlayIPAMProvider,
                "New data loaded, network resources added: {0}, removed: {1}, new ghosts: {2}, known ghosts:\n{3}\nCurrent Pool:\n{4}",
                added,
                removed,
                callbackNeeded,
                FormatSetAsWString(this->ghosts_),
                this->pool_.DumpCurrentState());
        }
        else
        {
            WriteInfo(
                OverlayIPAMProvider,
                "New data loaded, no changes found");
        }

        // We have successfully loaded the pool, so no matter what, we are now
        // initialized.
        //
        this->initialized_ = true;

        // Finally, if we need to notify the caller that the ghost list has changed,
        // do so now.
        if (callbackNeeded)
        {
            this->ghostChangeCallback_(refreshTime);
        }
    }

    // set up timer to refresh pool
    this->CleanupPoolRefreshTimer();
    this->SetupPoolRefreshTimer();
}

wstring OverlayIPAM::FormatSetAsWString(unordered_set<wstring> const & entries)
{
    bool first = true;
    wstringstream ss;

    for (auto const & item : entries)
    {
        if (!first)
        {
            ss << endl;
        }

        ss << item;
        first = false;
    }

    return ss.str();
}

ErrorCode OverlayIPAM::OnOpen()
{
    return ErrorCode::Success();
}

ErrorCode OverlayIPAM::OnClose()
{
    this->CleanupPoolRefreshTimer();
    return ErrorCode::Success();
}

void OverlayIPAM::OnAbort()
{
    OnClose().ReadValue();
}

void OverlayIPAM::SetupPoolRefreshTimer()
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        TimeSpan delay = TimeSpan::FromTicks(poolRefreshRetryInterval_.Ticks);

        this->updatePoolTimerSPtr_ = Timer::Create(
            "OverlayIPAM.UpdatePool",
            [this](TimerSPtr const & timer)
        {
            timer->Cancel();
            // force calling on thread pool thread
            Threadpool::Post([this]() { this->ReplenishReservationPool(); });
        });

        this->updatePoolTimerSPtr_->Change(delay);
    }
}

void OverlayIPAM::CleanupPoolRefreshTimer()
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        if (this->updatePoolTimerSPtr_ != nullptr)
        {
            this->updatePoolTimerSPtr_->Cancel();
            this->updatePoolTimerSPtr_ = nullptr;
        }
    }

}

void OverlayIPAM::ReplenishReservationPool()
{
    if (this->pool_.FreeCount == 0)
    {
        { //lock
            AcquireExclusiveLock lock(this->lock_);
            // Update initialized flag so that other api can respond correctly
            // while pool is being refreshed.
            this->initialized_ = false;

            // Invoke replenish callback on network manager on a thread pool thread.
            Threadpool::Post([this]() { this->internalReplenishNetworkResourcesCallback_(); });
        }
    }
    else
    {
        // set up pool refresh timer
        this->CleanupPoolRefreshTimer();
        this->SetupPoolRefreshTimer();
    }
}