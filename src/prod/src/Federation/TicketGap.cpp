// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/TicketGap.h"

using namespace Common;
using namespace std;
using namespace Federation;

TicketGap::TicketGap(NodeIdRange const & range, TimeRange const & interval)
    :   range_(range),
        interval_(interval)
{
}

// When a gap is sent to another node, this is used to adjust the gap as
// the lower bound of the interval on the receiving side.
// The delta is typically the received time of the request (which triggered
// the propagation of the gap) on the sending side minus the send time of
// the request.
TicketGap::TicketGap(TicketGap const & original, TimeSpan delta)
    :   range_(original.range_),
        interval_(original.Interval.Begin - delta, original.Interval.End - delta)
{
}

bool TicketGap::Merge(TicketGap const & rhs)
{
    if (range_ != rhs.range_)
    {
        return false;
    }

    interval_.Merge(rhs.interval_);
    return true;
}

bool TicketGap::IsEffective() const
{
    return (interval_.End > interval_.Begin);
}

bool TicketGap::IsExpired() const
{
    // We keep the gap for T (global lease interval) after it becomes effective.  This basically
    // gives around 2*T for a gap to exist in the system, which is usually sufficient to reach
    // a quorum.
    return (Stopwatch::Now() - interval_.Begin > FederationConfig::GetConfig().GlobalTicketLeaseDuration);
}

void TicketGap::AdjustTime(TimeSpan delta, StopwatchTime baseTime)
{
    StopwatchTime begin = StopwatchTime::GetUpperBound(interval_.Begin, baseTime);
    StopwatchTime end = StopwatchTime::GetLowerBound(interval_.End, delta);

    interval_ = TimeRange(begin, end);
}

// Updated by the vote owner on an existing gap to extend the end time.
void TicketGap::UpdateInterval()
{
    interval_ = TimeRange(interval_.Begin, Stopwatch::Now());
}

void TicketGap::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0}:{1}", range_, interval_);
}
