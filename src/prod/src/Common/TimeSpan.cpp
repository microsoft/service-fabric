// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/TimeSpan.h"


namespace Common
{
    TimeSpan const TimeSpan::Zero = TimeSpan(0);
    TimeSpan const TimeSpan::MaxValue = TimeSpan(std::numeric_limits<int64>::max());
    TimeSpan const TimeSpan::MinValue = TimeSpan(std::numeric_limits<int64>::min());

    std::wstring TimeSpan::ToString() const
    {
        std::wstring result;
        Common::StringWriter(result).Write(*this);
        return result;
    }

    // ISO 8601 duration format PnYnMnDTnHnMnS
    std::wstring TimeSpan::ToIsoString() const
    {
        if (ticks_ < 0)
        {
            // int64 min is -9,223,372,036,854,775,808, max is 9,223,372,036,854,775,807
            if (ticks_ == std::numeric_limits<int64>::min())
            {
                return L"-P10675199DT2H48M5.4775808S";
            }

            std::wstring result;
            result.append(L"-");
            result.append(TimeSpan(-ticks_).ToIsoString());
            return result;
        }

        if (ticks_ == std::numeric_limits<int64>::max())
        {
            // std::fmod has roundoff's preventing timespan.maxval from getting the correct microsecond value
            return L"P10675199DT2H48M5.4775807S";
        }

        std::wstringstream ss;
        ss << L"P";

        if (ticks_ >= TicksPerDay)
        {
            ss << Days;
            ss << L"D";
        }
        ss << L"T";
        ss << Hours;
        ss << L"H";
        ss << Minutes;
        ss << L"M";
        ss << get_SecondsWithFraction();
        ss << L"S";

        return ss.str();
    }

    bool TimeSpan::TryFromIsoString(std::wstring & str, __out TimeSpan & timeSpan)
    {
        std::string temp;
        StringUtility::Utf16ToUtf8(str, temp);
        std::regex regex("^(-)?P([0-9]*([.][0-9]+)?Y)?([0-9]*([.][0-9]+)?M)?([0-9]*([.][0-9]+)?D)?T([0-9]*([.][0-9]+)?H)?([0-9]*([.][0-9]+)?M)?([0-9]*([.][0-9]+)?S)?$");
        if (!std::regex_match(temp, regex))
        {
            return false;
        }

        if (str == L"-P10675199DT2H48M5.4775808S" || str == L"-P10675199DT02H48M05.4775808S")
        {
            timeSpan = TimeSpan::MinValue;
            return true;
        }

        int64 ticks = 0;
        std::string cur;
        bool isNegative = false;
        bool isParseTime = false;
        for (int i = 0; i < temp.length(); i++)
        {
            char c = temp[i];
            if (i == 0 && c == '-')
            {
                isNegative = true;
                continue;
            }
            else if (i == 0 && c == 'P' && isNegative == false)
            {
                continue;
            }
            else if (i == 1 && c == 'P' && isNegative == true)
            {
                continue;
            }
            else if (isdigit(c) || c == '.')
            {
                cur += c;
            }
            else
            {
                if (!isParseTime)
                {
                    switch (c)
                    {
                    case 'Y':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerYear);
                        break;
                    case 'M':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerMonth);
                        break;
                    case 'D':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerDay);
                        break;
                    case 'T':
                        isParseTime = true;
                        break;
                    default:
                        return false;
                    }
                }
                else
                {
                    switch (c)
                    {
                    case 'H':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerHour);
                        break;
                    case 'M':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerMinute);
                        break;
                    case 'S':
                        ticks += (int64)(std::stod(cur) * TimeSpan::TicksPerSecond);
                        break;
                    default:
                        return false;
                    }
                }
                cur.clear();
            }
        }

        if (isNegative)
        {
            ticks *= -1;
        }

        timeSpan = TimeSpan(ticks);
        return true;
    }

    void TimeSpan::WriteTo(TextWriter& w, FormatOptions const &) const
    {
        if (*this == MaxValue)
        {
            w << "Infinite";
            return;
        }
        else if (*this == MinValue)
        {
            w << "-Infinite";
            return;
        }

        if (ticks_ < 0)
        {
            w << "-";
            w << TimeSpan(-ticks_);
            return;
        }

        if (ticks_ >= TicksPerDay)
        {
            w << Days << ':';
        }
        if (ticks_ >= TicksPerHour)
        {
            w << Hours << ':';
        }
        if (ticks_ >= TicksPerMinute)
        {
            if (ticks_ >= TicksPerHour)
                w.Write("{0:02d}:", Minutes);
            else
                w << Minutes << ':';
        }
        w.Write("{0:02d}.{1,03:d}", Seconds, Milliseconds);
    }

    std::string TimeSpan::AddField(Common::TraceEvent & traceEvent, std::string const & name)
    {   
        traceEvent.AddField<int64>(name + ".timespan");
        return "{0}ms";
    }

    void TimeSpan::FillEventData(Common::TraceEventContext & context) const
    {
        context.WriteCopy<int64>(TotalMilliseconds());
    }


    TimeSpan TimeSpan::CreateTimespanWithRangeCheck(double value, int64 scale) 
    {
        if (value >= MaxValue.ticks_/scale){
            return TimeSpan::MaxValue;
        }
        if (value <= MinValue.ticks_/scale){
            return TimeSpan::MinValue;
        }

        return TimeSpan(static_cast<int64>(value * scale));
    }

    TimeSpan TimeSpan::AddWithMaxAndMinValueCheck(TimeSpan const& rhs) const
    {
        if (*this == TimeSpan::MaxValue || rhs == TimeSpan::MaxValue)
        {
            return TimeSpan::MaxValue;
        }
        else if (*this == TimeSpan::MinValue || rhs == TimeSpan::MinValue)
        {
            return TimeSpan::MinValue;
        }

        return TimeSpan(this->ticks_ + rhs.ticks_);
    }

    TimeSpan TimeSpan::SubtractWithMaxAndMinValueCheck(TimeSpan const& rhs) const
    {
        if (*this == TimeSpan::MaxValue || rhs == TimeSpan::MinValue)
        {
            return TimeSpan::MaxValue;
        }
        else if (*this == TimeSpan::MinValue || rhs == TimeSpan::MaxValue)
        {
            return TimeSpan::MinValue;
        }

        return TimeSpan(this->ticks_ - rhs.ticks_);
    }

    //
    // Convert this timespan to minutes, rounding up or down based upon the number of seconds.
    //
    int64 TimeSpan::TotalRoundedMinutes() const
    {
        int64 seconds = TotalSeconds();
        int64 minutes = seconds / 60;
        if ((seconds - (minutes * 60)) >= 30)
        {
            ++minutes;
        }

        return minutes;
    }

    uint64 TimeSpan::TotalPositiveMilliseconds() const
    {
        if (ticks_ < 0)
        {
            return 0;
        }
        else
        {
            return (ticks_ / TicksPerMillisecond);
        }
    }
} // end namespace Common
