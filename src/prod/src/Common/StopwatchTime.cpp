// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Common/Stopwatch.h"

namespace Common
{
    StopwatchTime const StopwatchTime::Zero = StopwatchTime(0);
    StopwatchTime const StopwatchTime::MaxValue = StopwatchTime(std::numeric_limits<int64>::max());
    TimeSpan const StopwatchTime::TimerMargin = TimeSpan::FromTicks(300000);

    TimeSpan StopwatchTime::operator- (StopwatchTime const& rhs) const
    {
        if (*this == StopwatchTime::MaxValue)
        {
            return TimeSpan::MaxValue;
        }

        return TimeSpan::FromTicks(this->ticks_ - rhs.ticks_);
    }

    StopwatchTime & StopwatchTime::operator +=(TimeSpan const& rhs)
    {
        if (*this != StopwatchTime::MaxValue)
        {
            if (rhs == TimeSpan::MaxValue)
            {
                this->ticks_ = std::numeric_limits<int64>::max();
            }
            else
            {
                this->ticks_ += rhs.Ticks;
            }
        }

        return *this;
    }

    StopwatchTime const StopwatchTime::operator + (TimeSpan const& rhs) const
    {
        StopwatchTime newTime = *this;
        newTime += rhs;
        return newTime;
    }

    StopwatchTime & StopwatchTime::operator -=(TimeSpan const& rhs)
    {
        if (*this != StopwatchTime::MaxValue)
        {
            if (rhs == TimeSpan::MaxValue)
            {
                this->ticks_ = 0;
            }
            else
            {
                this->ticks_ -= rhs.Ticks;
            }
        }

        return *this;
    }

    StopwatchTime const StopwatchTime::operator - (TimeSpan const& rhs) const
    {
        StopwatchTime newTime = *this;
        newTime -= rhs;
        return newTime;
    }

    TimeSpan StopwatchTime::RemainingDuration() const
    {
        TimeSpan result = *this - Stopwatch::Now();
        if (result < TimeSpan::Zero)
        {
            return TimeSpan::Zero;
        }

        return result;
    }

    void StopwatchTime::WriteTo(TextWriter& w, FormatOptions const &) const
    {
        w.Write(ToDateTime(*this));
    }

    StopwatchTime StopwatchTime::FromDateTime(DateTime value)
    {
        if (value == DateTime::Zero)
        {
            return StopwatchTime::Zero;
        }

        if (value == DateTime::MaxValue)
        {
            return StopwatchTime::MaxValue;
        }

        TimeSpan delta = value - DateTime::Now();
        return Stopwatch::Now() + delta;
    }

    DateTime StopwatchTime::ToDateTime() const
    {
        if (*this == StopwatchTime::Zero)
        {
            return DateTime::Zero;
        }

        if (*this == StopwatchTime::MaxValue)
        {
            return DateTime::MaxValue;
        }

        TimeSpan delta = *this - Stopwatch::Now();
        return DateTime::Now() + delta;
    }

    DateTime StopwatchTime::ToDateTime(StopwatchTime value)
    {
        return value.ToDateTime();
    }

    StopwatchTime StopwatchTime::GetLowerBound(StopwatchTime value, TimeSpan delta)
    {
        return value - (delta + TimerMargin);
    }

    StopwatchTime StopwatchTime::GetUpperBound(StopwatchTime value, StopwatchTime baseTime)
    {
        return value + (Stopwatch::Now() - baseTime);
    }
} // end namespace Common
