// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TransactionTracker");

ReplicatedStore::TransactionTracker::TransactionTracker(__in ReplicatedStore & store)
    : PartitionedReplicaTraceComponent(store.PartitionedReplicaId)
    , replicatedStore_(store)
    , outstandingTxs_()
    , isDraining_(false)
    , drainTimeout_(store.Settings.TransactionDrainTimeout)
    , drainTimer_()
    , lock_()
{
    WriteInfo(
        TraceComponent,
        "{0} ctor: timeout={1}",
        this->TraceId,
        drainTimeout_);
}

bool ReplicatedStore::TransactionTracker::TryAdd(TransactionBaseSPtr const & txSPtr)
{
    // always allow transaction creation when tracking is disabled
    //
    if (drainTimeout_ <= TimeSpan::Zero) { return true; }

    AcquireExclusiveLock lock(lock_);

    if (!isDraining_)
    {
        outstandingTxs_.insert(make_pair(txSPtr->TrackerId, txSPtr));
    }

    return !isDraining_;
}

void ReplicatedStore::TransactionTracker::Remove(TransactionBase const & tx)
{
    if (drainTimeout_ <= TimeSpan::Zero) { return; }

    AcquireExclusiveLock lock(lock_);

    outstandingTxs_.erase(tx.TrackerId);
}

void ReplicatedStore::TransactionTracker::StartDrainTimer()
{
    this->ScheduleAbortOutstanding(true);
}

void ReplicatedStore::TransactionTracker::AbortOutstandingTransactions()
{
    this->ScheduleAbortOutstanding(false);
}

void ReplicatedStore::TransactionTracker::ScheduleAbortOutstanding(bool enableDrainTimer)
{
    if (drainTimeout_ <= TimeSpan::Zero) { return; }

    AcquireExclusiveLock lock(lock_);

    if (isDraining_) { return; }

    if (drainTimer_) { drainTimer_->Cancel(); }

    if (!outstandingTxs_.empty())
    {
        isDraining_ = enableDrainTimer;
        auto root = replicatedStore_.Root.CreateComponentRoot();
        auto toRelease = make_shared<TransactionMap>(move(outstandingTxs_));
        
        outstandingTxs_ = TransactionMap();

        WriteInfo(
            TraceComponent,
            "{0} schedule abort outstanding: count={1} timer={2}",
            this->TraceId,
            toRelease->size(),
            enableDrainTimer);

        Threadpool::Post([this, root, toRelease, enableDrainTimer]{ this->AbortOutstanding(root, toRelease, enableDrainTimer); });
    }
    else
    {
        isDraining_ = false;

        WriteInfo(
            TraceComponent,
            "{0} no outstanding transactions",
            this->TraceId);
    }
}

void ReplicatedStore::TransactionTracker::CancelDrainTimer()
{
    if (drainTimeout_ <= TimeSpan::Zero) { return; }

    AcquireExclusiveLock lock(lock_);

    isDraining_ = false;

    WriteInfo(
        TraceComponent,
        "{0} canceled",
        this->TraceId);

    if (drainTimer_)
    {
        drainTimer_->Cancel();
    }
}

void ReplicatedStore::TransactionTracker::AbortOutstanding(
    ComponentRootSPtr const & root,
    shared_ptr<TransactionMap> const & toRelease,
    bool enableDrainTimer)
{
    WriteInfo(
        TraceComponent,
        "{0} abort outstanding: count={1} timer={2}",
        this->TraceId,
        toRelease->size(),
        enableDrainTimer);

    for (auto pair : *toRelease)
    {
        auto txSPtr = pair.second.lock();

        if (txSPtr)
        {
            WriteInfo(
                TraceComponent,
                "{0} force releasing: id={1} activity={2} creation={3}Z",
                this->TraceId,
                txSPtr->TrackerId,
                txSPtr->ActivityId,
                txSPtr->CreationTime);

            if (txSPtr->TryForceReleaseInnerTransaction())
            {
                replicatedStore_.FinishTransaction();
            }
        }
    }

    AcquireExclusiveLock lock(lock_);

    if (isDraining_ && enableDrainTimer)
    {
        WriteInfo(
            TraceComponent,
            "{0} scheduling drain check: timeout={1}",
            this->TraceId,
            drainTimeout_);

        drainTimer_ = Timer::Create(TimerTagDefault, [this, root](TimerSPtr const&){ this->DrainTimerCallback(); });
        drainTimer_->Change(drainTimeout_);
    }
}

void ReplicatedStore::TransactionTracker::DrainTimerCallback()
{
    // All transactions should have either been released by the application
    // or forcefully released by the tracker, dropping the tx count to 0.
    // If this timeout is hit, then the problem is more likely to be
    // a ref-counting bug.
    //
    auto msg = wformatString("{0} drain timer exhausted", this->TraceId);

    WriteError(TraceComponent, "{0}", msg);

    Assert::CodingError("{0}", msg);
}
