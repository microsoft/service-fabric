// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Fwd Declarations
    class TraceEvent;
    class TraceEventContext;
    class TextWriter;

    class TimeSpan
    {
    public:
        TimeSpan() : ticks_() {}

        __declspec (property(get=get_Milliseconds)) int Milliseconds;
        __declspec (property(get=get_Seconds))      int Seconds;
        __declspec (property(get=get_Minutes))      int Minutes;
        __declspec (property(get=get_Hours))        int Hours;
        __declspec (property(get=get_Days))         int Days;
        __declspec (property(get=get_Ticks))        int64 Ticks;

        bool operator == (TimeSpan const& rhs) const { return ticks_ == rhs.ticks_; }
        bool operator != (TimeSpan const& rhs) const { return ticks_ != rhs.ticks_; }
        bool operator < (TimeSpan const& rhs) const { return ticks_ < rhs.ticks_; }
        bool operator > (TimeSpan const& rhs) const { return ticks_ > rhs.ticks_; }
        bool operator >= (TimeSpan const& rhs) const { return ticks_ >= rhs.ticks_; }
        bool operator <= (TimeSpan const& rhs) const { return ticks_ <= rhs.ticks_; }

        TimeSpan operator+ (TimeSpan const& rhs) const
        {
            return TimeSpan(this->ticks_ + rhs.ticks_);
        }

        TimeSpan operator- (TimeSpan const& rhs) const
        {
            return TimeSpan(this->ticks_ - rhs.ticks_);
        }

        TimeSpan operator / (unsigned int divisor) const
        {
            return TimeSpan ( this->ticks_ / divisor );
        }

        friend double operator / (double a, TimeSpan b) 
        {
            return (a * static_cast<double>(TimeSpan::TicksPerSecond) / static_cast<double>(b.ticks_));
        }

        uint64 TotalPositiveMilliseconds() const;
        int64 TotalRoundedMinutes() const;
        TimeSpan AddWithMaxAndMinValueCheck (TimeSpan const& rhs) const;
        TimeSpan SubtractWithMaxAndMinValueCheck (TimeSpan const& rhs) const;
        std::wstring ToString() const;
        std::wstring ToIsoString() const;
        static bool TryFromIsoString(std::wstring & str, __out TimeSpan &);
        void WriteTo(TextWriter& w, FormatOptions const &) const;

        int get_Milliseconds() const 
        { 
            return static_cast<int>( (ticks_ / TicksPerMillisecond) % 1000) ;
        }

        double get_SecondsWithFraction() const
        {
            return std::fmod((((double)ticks_) / TicksPerSecond), 60);
        }

        int get_Seconds() const 
        { 
            return static_cast<int>( (ticks_ / TicksPerSecond) % 60) ;
        }

        int get_Minutes() const 
        { 
            return static_cast<int>( (ticks_ / TicksPerMinute) % 60) ;
        }

        int get_Hours() const 
        { 
            return static_cast<int>( (ticks_ / TicksPerHour) % 24) ;
        }

        int get_Days() const 
        { 
            return static_cast<int>(ticks_ / TicksPerDay) ;
        }

        int64 get_Ticks() const
        {
            return ticks_;
        }

        int64 TotalMilliseconds() const
        {
            return (ticks_ / TicksPerMillisecond);
        }

        int64 TotalSeconds() const
        {
            return (ticks_ / TicksPerSecond);
        }

        double TotalMillisecondsAsDouble() const
        {
            return ticks_ / static_cast<double>(TicksPerMillisecond);
        }

        static TimeSpan FromMilliseconds(double value) {
            return CreateTimespanWithRangeCheck(value, TicksPerMillisecond);
        }

        static TimeSpan FromSeconds(double value) {
            return CreateTimespanWithRangeCheck(value, TicksPerSecond);
        }

        static TimeSpan FromMinutes(double value) {
            return CreateTimespanWithRangeCheck(value, TicksPerMinute);
        }

        static TimeSpan FromHours(double value) {
            return CreateTimespanWithRangeCheck(value, TicksPerHour);
        }

        static TimeSpan FromTicks(int64 value) {
            return TimeSpan(value);
        }

        static TimeSpan const Zero;
        static TimeSpan const MaxValue;
        static TimeSpan const MinValue;
        static int64 const TicksPerMillisecond = 10000;
        static int64 const TicksPerSecond = TicksPerMillisecond * 1000;
        static int64 const TicksPerMinute = TicksPerSecond * 60;
        static int64 const TicksPerHour = TicksPerMinute * 60;
        static int64 const TicksPerDay = TicksPerHour * 24;
        static int64 const TicksPerMonth = TicksPerDay * 30;// Default to 30 days per month, may have accuracy issue that one might use 31
        static int64 const TicksPerYear = TicksPerMonth * 12;

        friend class DateTime;

        FABRIC_PRIMITIVE_FIELDS_01(ticks_);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        void FillEventData(Common::TraceEventContext & context) const;

    private:
        explicit TimeSpan(int64 ticks) : ticks_(ticks) {}

        static TimeSpan CreateTimespanWithRangeCheck(double value, int64 scale);

        int64 ticks_;

    };

    inline double operator / (TimeSpan const & lhs, TimeSpan const & rhs)
    {
        return static_cast<double>(lhs.Ticks) / static_cast<double>(rhs.Ticks);
    }
};
