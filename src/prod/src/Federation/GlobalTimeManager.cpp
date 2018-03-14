// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

StringLiteral const TraceEpoch("Epoch");
StringLiteral const TraceGlobalTime("GlobalTime");

TimeSpan const ClockMargin = TimeSpan::FromMilliseconds(1);

class GlobalTimeManager::UpdateEpochAsyncOperation : public VoterStoreReadWriteAsyncOperation
{
    DENY_COPY(UpdateEpochAsyncOperation);

public:
    UpdateEpochAsyncOperation(
        SiteNode & site,
        GlobalTimeManager & manager,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : VoterStoreReadWriteAsyncOperation(site, Federation::Constants::GlobalTimestampEpochName, FederationConfig::GetConfig().VoterStoreRetryInterval, callback, parent),
        manager_(manager)
    {
    }

    static int64 GetEpoch(SerializedVoterStoreEntryUPtr const & value)
    {
        int64 result = 0;
        if (value)
        {
            VoterStoreSequenceEntry const* entry = VoterStoreSequenceEntry::Convert(*value);
            if (entry)
            {
                result = entry->Value;
            }
        }

        return result;
    }

protected:
    virtual SerializedVoterStoreEntryUPtr GenerateValue(SerializedVoterStoreEntryUPtr const & currentValue)
    {
        int64 newEpoch = manager_.GenerateEpoch(GetEpoch(currentValue));
        if (newEpoch <= 0)
        {
            return nullptr;
        }

        return make_unique<VoterStoreSequenceEntry>(newEpoch);
    }

private:
    GlobalTimeManager & manager_;
};

GlobalTimeManager::GlobalTimeManager(SiteNode & site, RwLock & lock)
  : site_(site),
    lock_(lock),
    epoch_(0),
    upperLimit_(TimeSpan::MaxValue),
    lowerLimit_(TimeSpan::MinValue),
    lastRefresh_(StopwatchTime::Zero),
    isAuthority_(false),
    isUpdatingEpoch_(false),
    leaderStartTime_(StopwatchTime::MaxValue),
    lastTraceTime_(StopwatchTime::Zero)
{
}

bool GlobalTimeManager::IsLeader() const
{
    return (leaderStartTime_ != StopwatchTime::MaxValue);
}

void GlobalTimeManager::BecomeLeader()
{
    AcquireWriteLock grab(lock_);
    if (!IsLeader())
    {
        leaderStartTime_ = Stopwatch::Now();
    }
}

void GlobalTimeManager::UpdateRange(GlobalTimeExchangeHeader const & header)
{
    if (header.Epoch < epoch_)
    {
        return;
    }

    Refresh();

    TimeSpan oldLowerLimit = lowerLimit_;

    bool epochUpdated = (header.Epoch > epoch_);
    if (epochUpdated)
    {
        IncreaseEpoch(header.Epoch);
    }

    StopwatchTime now = Stopwatch::Now();
    if (header.ReceiverUpperLimit < upperLimit_)
    {
        if (header.ReceiverUpperLimit >= lowerLimit_)
        {
            upperLimit_ = header.ReceiverUpperLimit;
        }
        else
        {
            VoteManagerEventSource::Events->InvalidUpperTimeRange(site_.IdString, header.ReceiverUpperLimit, epoch_, lowerLimit_, upperLimit_);
            if (!isAuthority_)
            {
                lowerLimit_ = TimeSpan::MinValue;
            }
        }
    }

    if (header.SenderLowerLimit != TimeSpan::MinValue)
    {
        TimeSpan lowerLimit = header.SendTime + header.SenderLowerLimit - now - ClockMargin;
        if (lowerLimit > lowerLimit_)
        {
            if (upperLimit_ >= lowerLimit)
            {
                lowerLimit_ = lowerLimit;
            }
            else
            {
                VoteManagerEventSource::Events->InvalidLowerTimeRange(site_.IdString, lowerLimit, epoch_, lowerLimit_, upperLimit_);
                if (!isAuthority_)
                {
                    upperLimit_ = TimeSpan::MaxValue;
                    site_.Table.IncreaseGlobalTimeUpperLimitForNodes(TimeSpan::MaxValue);
                }
            }
        }
    }

    if (epochUpdated)
    {
        site_.Table.IncreaseGlobalTimeUpperLimitForNodes(upperLimit_.SubtractWithMaxAndMinValueCheck(oldLowerLimit));
        VoteManager::WriteInfo(TraceEpoch, site_.IdString, "Updated epoch to {0}", epoch_);
    }

    VoteManager::WriteInfo(TraceGlobalTime, site_.IdString, "Updated to {0} with {1}, now={2}", *this, header, now.Ticks);
}

TimeSpan GlobalTimeManager::IncreaseEpoch(int64 epoch)
{
    ASSERT_IFNOT(epoch > epoch_, "New epoch {0} not larger than old {1}", epoch, epoch_);

    TimeSpan delta;
    if (upperLimit_ != TimeSpan::MaxValue)
    {
        delta = TimeSpan::FromTicks((epoch - epoch_) * FederationConfig::GetConfig().GlobalTimeUncertaintyMaxIncrease.Ticks);
        upperLimit_ = upperLimit_ + delta;
    }
    else
    {
        delta = TimeSpan::MaxValue;
    }

    epoch_ = epoch;
    isAuthority_ = false;

    return delta;
}

int64 GlobalTimeManager::GetUpperLimit(StopwatchTime time)
{
    if (upperLimit_ == TimeSpan::MaxValue)
    {
        return DateTime::MaxValue.Ticks;
    }

    return time.Ticks + upperLimit_.Ticks;
}

int64 GlobalTimeManager::GetLowerLimit(StopwatchTime time)
{
    if (lowerLimit_ == TimeSpan::MinValue)
    {
        return 0;
    }

    return time.Ticks + lowerLimit_.Ticks;
}

int64 GlobalTimeManager::GetInfo(int64 & lowerLimit, int64 & upperLimit, bool isAuthority)
{
    AcquireWriteLock grab(lock_);

    StopwatchTime now = Stopwatch::Now();
    lowerLimit = GetLowerLimit(now);
    upperLimit = GetUpperLimit(now);

    isAuthority = isAuthority_;
    return epoch_;
}

void GlobalTimeManager::AddGlobalTimeExchangeHeader(Message & message, PartnerNodeSPtr const & target)
{
    Refresh();

    message.Headers.Add(GlobalTimeExchangeHeader(epoch_, Stopwatch::Now(), lowerLimit_, target ? target->GlobalTimeUpperLimit : TimeSpan::MaxValue));
}

void GlobalTimeManager::Refresh()
{
    if (!isAuthority_)
    {
        StopwatchTime now = Stopwatch::Now();

        if (lastRefresh_ == StopwatchTime::Zero)
        {
            lastRefresh_ = now;
        }
        else
        {
            FederationConfig const & config = FederationConfig::GetConfig();
            TimeSpan elapsed = now - lastRefresh_;

            int64 delta = elapsed.Ticks / config.GlobalTimeClockDriftRatio;
            if (delta > 0)
            {
                if (upperLimit_ != TimeSpan::MaxValue)
                {
                    upperLimit_ = upperLimit_ + TimeSpan::FromTicks(delta);
                }
                if (lowerLimit_ != TimeSpan::MinValue)
                {
                    lowerLimit_ = lowerLimit_ - TimeSpan::FromTicks(delta);
                }

                lastRefresh_ += TimeSpan::FromTicks(delta * config.GlobalTimeClockDriftRatio);
            }
        }
    }
}

void GlobalTimeManager::UpdatePartnerNodeUpperLimit(GlobalTimeExchangeHeader const & header, PartnerNodeSPtr const & target) const
{
    if (upperLimit_ != TimeSpan::MaxValue)
    {
        target->SetGlobalTimeUpperLimit(Stopwatch::Now() + upperLimit_ - header.SendTime + ClockMargin);
    }
}

void GlobalTimeManager::CheckHealth()
{
    Refresh();

    FederationConfig const & config = FederationConfig::GetConfig();
    StopwatchTime now = Stopwatch::Now();

    if (now > lastTraceTime_ + config.GlobalTimeTraceInterval)
    {
        VoteManager::WriteInfo(TraceGlobalTime, site_.IdString, "Global time uncertainty: {0}", *this);
        lastTraceTime_ = now;
    }

    if (IsLeader() &&
        !isUpdatingEpoch_ &&
        now - config.GlobalTimeNewEpochWaitInterval > leaderStartTime_ &&
        upperLimit_ > lowerLimit_ + config.GlobalTimeUncertaintyIntervalUpperBound)
    {
        VoteManager::WriteInfo(TraceEpoch, site_.IdString, "Updating epoch: {0}", *this);

        isUpdatingEpoch_ = true;
        auto site = site_.CreateComponentRoot();
        Threadpool::Post([this, site]
        {
            AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
                this->site_,
                *this,
                [this](AsyncOperationSPtr const & operation) { this->OnUpdateCompleted(operation); },
                site->CreateAsyncOperationRoot());

        }
        );
    }
}

int64 GlobalTimeManager::GenerateEpoch(int64 currentEpoch)
{
    AcquireWriteLock grab(lock_);

    Refresh();

    if (currentEpoch > epoch_)
    {
        int64 oldEpoch = epoch_;
        TimeSpan delta = IncreaseEpoch(currentEpoch);
        site_.Table.IncreaseGlobalTimeUpperLimitForNodes(delta);

        VoteManager::WriteInfo(TraceEpoch, site_.IdString, "Increased epoch from {0} to {1}", oldEpoch, *this);
    }
    else if (currentEpoch < epoch_)
    {
        VoteManager::WriteError(TraceEpoch, site_.IdString, "Current epoch {0} less than local {1}", currentEpoch, *this);
        return 0;
    }

    if (upperLimit_ == TimeSpan::MaxValue || lowerLimit_ == TimeSpan::MinValue)
    {
        return epoch_ + 1;
    }
    else
    {
        TimeSpan range = upperLimit_ - lowerLimit_;
        return epoch_ + (range.Ticks / FederationConfig::GetConfig().GlobalTimeUncertaintyMaxDecrease.Ticks) + 1;
    }
}

void GlobalTimeManager::OnUpdateCompleted(AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<UpdateEpochAsyncOperation>(operation);
    int64 epoch = UpdateEpochAsyncOperation::GetEpoch(thisPtr->GetResult());

    AcquireWriteLock grab(lock_);

    VoteManager::WriteInfo(TraceEpoch, site_.IdString,
        "UpdateEpoch completed {0} with result {1}: {2}",
        thisPtr->Error, epoch, *this);

    isUpdatingEpoch_ = false;
    if (thisPtr->Error.IsSuccess() && epoch > epoch_)
    {
        Refresh();

        if (upperLimit_ == TimeSpan::MaxValue && lowerLimit_ == TimeSpan::MinValue)
        {
            upperLimit_ = lowerLimit_ = TimeSpan::FromTicks(DateTime::Now().Ticks - Stopwatch::Now().Ticks);
        }
        else
        {
            TimeSpan decreaseInterval = FederationConfig::GetConfig().GlobalTimeUncertaintyMaxDecrease;
            if (upperLimit_ != TimeSpan::MaxValue)
            {
                ASSERT_IF(lowerLimit_ == TimeSpan::MinValue, "LowerLimit not set on {0}: {1}", site_.Id, *this);
            }
            else
            {
                upperLimit_ = lowerLimit_ + decreaseInterval;
            }

            TimeSpan delta = min(TimeSpan::FromTicks(decreaseInterval.Ticks * (epoch - epoch_)), upperLimit_ - lowerLimit_);
            lowerLimit_ = lowerLimit_ + delta;

            site_.Table.IncreaseGlobalTimeUpperLimitForNodes(delta);
        }

        epoch_ = epoch;
        isAuthority_ = true;

        VoteManager::WriteInfo(TraceEpoch, site_.IdString, "Updated epoch: {0}", *this);
    }
}

void GlobalTimeManager::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0}:{1}/{2}", epoch_, lowerLimit_.Ticks, upperLimit_.Ticks);
    if (isAuthority_)
    {
        w.Write(" A");
    }
}
