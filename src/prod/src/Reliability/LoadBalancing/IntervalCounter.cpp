// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "IntervalCounter.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

IntervalCounter::IntervalCounter(TimeSpan interval)
    : counts_(),
    totalCount_(0),
    intervalTicks_(interval.Ticks)
{
    ASSERT_IF(intervalTicks_ < 0, "Interval {0} is not long enough.", interval);
}

IntervalCounter::IntervalCounter(IntervalCounter const& other)
    : counts_(other.counts_),
    totalCount_(other.totalCount_),
    intervalTicks_(other.intervalTicks_)
{
}

IntervalCounter::IntervalCounter(IntervalCounter && other)
    : counts_(move(other.counts_)),
    totalCount_(other.totalCount_),
    intervalTicks_(other.intervalTicks_)
{
}

void IntervalCounter::Reset()
{
    counts_.clear();
    totalCount_ = 0;
}

void IntervalCounter::Record(size_t count, StopwatchTime now)
{
    if (!counts_.empty() && counts_.back().second > now.Ticks)
    {
        //Last record call had a larger time stamp than current one
        Reset();
    }
    if (!counts_.empty() && counts_.back().second == now.Ticks)
    {
        counts_.back().first += count;
    }
    else
    {
        counts_.push_back(make_pair(count, now.Ticks));
    }
    totalCount_ += count;
    AdjustWindow(now);
}

void IntervalCounter::Refresh(StopwatchTime now)
{
    AdjustWindow(now);
}

size_t IntervalCounter::GetCount() const
{
    return totalCount_;
}

size_t IntervalCounter::GetCountsSize() const
{
    return counts_.size();
}

void IntervalCounter::AdjustWindow(StopwatchTime now)
{
    int64 currentTicks = now.Ticks;

    while (!counts_.empty() && currentTicks - counts_.front().second >= intervalTicks_)
    {
        totalCount_ -= counts_.front().first;
        counts_.pop_front();
    }
}
