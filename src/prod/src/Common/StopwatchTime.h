// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/DateTime.h"

namespace Common
{
    class TextWriter;

    class StopwatchTime
    {
    public:
        StopwatchTime() : ticks_ (0) {}
        explicit StopwatchTime(int64 ticks) : ticks_(ticks) {}

        __declspec (property(get=get_Ticks))      int64 Ticks;
        int64 get_Ticks() const
        {
            return ticks_;
        }

        bool operator == (StopwatchTime const& rhs) const { return ticks_ == rhs.ticks_; }
        bool operator != (StopwatchTime const& rhs) const { return ticks_ != rhs.ticks_; }
        bool operator < (StopwatchTime const& rhs) const { return ticks_ < rhs.ticks_; }
        bool operator > (StopwatchTime const& rhs) const { return ticks_ > rhs.ticks_; }
        bool operator >= (StopwatchTime const& rhs) const { return ticks_ >= rhs.ticks_; }
        bool operator <= (StopwatchTime const& rhs) const { return ticks_ <= rhs.ticks_; }

        void WriteTo(TextWriter& w, FormatOptions const &) const;

        TimeSpan operator - (StopwatchTime const& rhs) const;

        StopwatchTime & operator +=(TimeSpan const& rhs);
        StopwatchTime const operator + (TimeSpan const& rhs) const;

        StopwatchTime & operator -=(TimeSpan const& rhs);
        StopwatchTime const operator - (TimeSpan const& rhs) const;

        TimeSpan RemainingDuration () const;

        DateTime ToDateTime() const;

        static StopwatchTime FromDateTime(DateTime value);

        static DateTime ToDateTime(StopwatchTime value);

        FABRIC_PRIMITIVE_FIELDS_01(ticks_);

        static StopwatchTime GetLowerBound(StopwatchTime value, TimeSpan delta);
        static StopwatchTime GetUpperBound(StopwatchTime value, StopwatchTime baseTime);

        static TimeSpan const TimerMargin;
        static StopwatchTime const Zero;
        static StopwatchTime const MaxValue;
    private:
        int64 ticks_;
    };
};
