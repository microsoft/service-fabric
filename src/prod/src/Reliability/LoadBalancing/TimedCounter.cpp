// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TimedCounter.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

TimedCounter::TimedCounter(TimeSpan interval, size_t windowCount)
    : windowCount_(windowCount), 
    startTime_(StopwatchTime::Zero), 
    lastOperationTime_(StopwatchTime::Zero),
    intervalPerWindow_(TimeSpan::FromTicks(interval.Ticks / windowCount)),
    counts_(),
    totalCount_(0)
{
    ASSERT_IF(windowCount <= 0, "windowCount must be positive; provided value: {0}", windowCount);
    ASSERT_IF(intervalPerWindow_.Ticks == 0, "Interval {0} is not long enough for {1} windows", interval, windowCount);
}

TimedCounter::TimedCounter(TimedCounter const& other)
    : windowCount_(other.windowCount_),
      startTime_(other.startTime_),
      lastOperationTime_(other.lastOperationTime_),
      intervalPerWindow_(other.intervalPerWindow_),
      counts_(other.counts_),
      totalCount_(other.totalCount_)
{
}

TimedCounter::TimedCounter(TimedCounter && other)
    : windowCount_(other.windowCount_),
      startTime_(other.startTime_),
      lastOperationTime_(other.lastOperationTime_),
      intervalPerWindow_(other.intervalPerWindow_),
      counts_(move(other.counts_)),
      totalCount_(other.totalCount_)
{
}

void TimedCounter::Reset()
{
    startTime_ = StopwatchTime::Zero;
    lastOperationTime_ = StopwatchTime::Zero;
    counts_.clear();
    totalCount_ = 0;
}

void TimedCounter::Record(size_t count, StopwatchTime now)
{
    AdjustWindows(now);

    counts_[counts_.size() - 1] += count;
    totalCount_ += count;
}

void TimedCounter::Refresh(StopwatchTime now)
{
    AdjustWindows(now);
}

size_t TimedCounter::GetCount() const
{
    return totalCount_;
}

void TimedCounter::Merge(TimedCounter & other, StopwatchTime now)
{
    ASSERT_IFNOT(windowCount_ == other.windowCount_, 
                             "The TimedCounters being merged must have the same window count; actual: {0} and {1}", 
                             windowCount_, 
                             other.windowCount_);
    ASSERT_IFNOT(intervalPerWindow_ == other.intervalPerWindow_, 
                             "The TimedCounters being merged must have the same interval per window; actual: {0} and {1}", 
                             intervalPerWindow_, 
                             other.intervalPerWindow_);

    AdjustWindows(now);
    other.AdjustWindows(now);

    ASSERT_IFNOT(counts_.size() == other.counts_.size(), 
        "The TimedCounters being merged must have the same counts_.size(); actual: {0} and {1}", 
        counts_.size(), 
        other.counts_.size());

    for (size_t i = 0; i < counts_.size(); i++)
    {
        counts_[i] += other.counts_[i];
    }

    totalCount_ += other.totalCount_;
}

void TimedCounter::AdjustWindows(StopwatchTime now)
{
    now = max(now, lastOperationTime_);
    lastOperationTime_ = now;

    StopwatchTime roundedNow = StopwatchTime((now.Ticks / intervalPerWindow_.Ticks) * intervalPerWindow_.Ticks);
    StopwatchTime newStartTime = StopwatchTime(roundedNow.Ticks - intervalPerWindow_.Ticks * (windowCount_ - 1));

    newStartTime = max(newStartTime, StopwatchTime::Zero);

    ASSERT_IFNOT(startTime_ <= newStartTime, "The new start time for the tracked interval should be as large as the old one; new: {0}; old: {1}", newStartTime, startTime_);

    while (!counts_.empty() && startTime_ < newStartTime)
    {
        size_t value = counts_.front();
        ASSERT_IFNOT(value <= totalCount_, "Value to be deleted {0} should be <= total {1}", value, totalCount_);
        totalCount_ -= value;
        counts_.pop_front();
        startTime_ = startTime_ + intervalPerWindow_;
    }

    startTime_ = newStartTime;
    StopwatchTime lastStartTime = startTime_ + TimeSpan::FromTicks(counts_.size() * intervalPerWindow_.Ticks);

    while (lastStartTime <= roundedNow)
    {
        counts_.push_back(0);
        lastStartTime = lastStartTime + intervalPerWindow_;
    }

    ASSERT_IFNOT(counts_.size() > 0 && counts_.size() <= windowCount_, "counts_.size() should be within (0 and {0}]; {1} received", windowCount_, counts_.size());
}
