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

namespace
{
    StringLiteral const SetSuccessfullyTag(" set successfully.");
    StringLiteral const SetIgnoredAsTimerIsClosedTag(" ignored because timer is closed");
    StringLiteral const SetIgnoredAsTimerAlreadySetTag(" ignored as timer already set");
}

RetryTimer::RetryTimer(
    std::wstring const & id,
    ReconfigurationAgent & component, 
    TimeSpanConfigEntry const & retryInterval,
    wstring const & activityIdPrefix,
    RetryTimerCallback const & callback)
    : component_(component),
      callback_(callback),
      retryInterval_(retryInterval),
      id_(id),
      sequenceNumber_(0),
      isSet_(false),
      activityIdPrefix_(activityIdPrefix)
{
    RAEventSource::Events->RetryTimerCreate(id_, GetRetryInterval());

    // safe to assume that lock is available when this object is being constructed
    CreateTimerUnderLock();
}

RetryTimer::~RetryTimer()
{
}

void RetryTimer::Set()
{
    Common::AcquireWriteLock grab(lock_);

    if (IsClosedUnderLock())
    {
        RAEventSource::Events->RetryTimerSetCalled(id_, sequenceNumber_, SetIgnoredAsTimerIsClosedTag);
        return;
    }

    sequenceNumber_++;

    if (!isSet_)
    {
        RAEventSource::Events->RetryTimerSetCalled(id_, sequenceNumber_, SetSuccessfullyTag);

        SetTimerUnderLock();
    }
    else
    {
        RAEventSource::Events->RetryTimerSetCalled(id_, sequenceNumber_, SetIgnoredAsTimerAlreadySetTag);
    }
}

bool RetryTimer::TryCancel(int64 sequenceNumber)
{
    Common::AcquireWriteLock grab(lock_);

    ASSERT_IF(sequenceNumber_ < sequenceNumber, "Given sequence number cannot be greater than current sequence number");

    RAEventSource::Events->RetryTimerTryCancel(id_, sequenceNumber, sequenceNumber_);

    if (sequenceNumber_ == sequenceNumber)
    {
        CancelTimerUnderLock(true);

        return true;
    }

    return false;
}

void RetryTimer::Close()
{
    Common::AcquireWriteLock grab(lock_);

    RAEventSource::Events->RetryTimerClosed(id_);

    CancelTimerUnderLock(false);
}

void RetryTimer::SetTimerUnderLock()
{
    ASSERT_IF(timer_ == nullptr, "Timer cannot be null if set is being called");

    isSet_ = true;
    timer_->Change(GetRetryInterval());
}

void RetryTimer::CancelTimerUnderLock(bool recreate)
{
    if (IsClosedUnderLock())
    {
        return;
    }

    ASSERT_IF(timer_ == nullptr, "Timer cannot be null if cancel is being called");

    isSet_ = false;

    timer_->Cancel();
    timer_.reset();

    if (recreate)
    {
        // once the timer has been cancelled it cannot be reused so create a new one
        CreateTimerUnderLock();
    }
}

void RetryTimer::CreateTimerUnderLock()
{
    auto root = component_.Root.CreateComponentRoot();
    timer_ = Common::Timer::Create("RA.Retry", [this, root] (Common::TimerSPtr const &)
    {
        int64 currentSequenceNumber = 0;

        {
            Common::AcquireWriteLock grab(lock_);

            isSet_ = false;
            currentSequenceNumber = sequenceNumber_;
        }

        auto activityId = wformatString("{0}:{1}", activityIdPrefix_, currentSequenceNumber);
        callback_(activityId, component_);
    },
    true /* allow concurrency */);
}

TimeSpan RetryTimer::GetRetryInterval() const
{
    return retryInterval_.GetValue();
}
