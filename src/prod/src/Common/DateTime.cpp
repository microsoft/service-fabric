// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/DateTime.h"
#include "Common/ErrorCodeValue.h"


namespace Common
{
    DateTime const DateTime::Zero = DateTime(0);
    DateTime const DateTime::MaxValue = DateTime(std::numeric_limits<int64>::max());
#if defined(PLATFORM_UNIX)
    const int64 DateTime::SECS_BETWEEN_1601_AND_1970_EPOCHS = 11644473600LL;
    const __int64 DateTime::SECS_TO_100NS = 10000000;
#endif

    DateTime::DateTime(FILETIME filetime)
    {
        ULARGE_INTEGER ticks;
        ticks.LowPart = filetime.dwLowDateTime;
        ticks.HighPart = filetime.dwHighDateTime;
        ticks_ = ticks.QuadPart;
    }

    DateTime::DateTime(LARGE_INTEGER largeIntTime)
    {
        ticks_ = largeIntTime.QuadPart;
    }

    DateTime DateTime::get_Now()
    {
#if defined(PLATFORM_UNIX)
        FILETIME filetime;
        struct timeval time;
        if (gettimeofday(&time, NULL) != 0)
        {
            Assert::CodingError("gettimeofday() failed");
        }
        else
        {
            filetime = UnixTimeToFileTime(time.tv_sec, time.tv_usec * 1000);
        }
        return DateTime(filetime);
#else
        FILETIME filetime;
        ::GetSystemTimeAsFileTime(&filetime);
        return DateTime(filetime);
#endif
    }

    TimeSpan DateTime::operator- (DateTime const& rhs) const
    {
        return TimeSpan(this->ticks_ - rhs.ticks_);
    }

    std::wstring DateTime::ToString(bool utc) const
    {
        std::wstring result;
        if (utc)
        {
            Common::StringWriter(result).Write(*this);
        }
        else
        {
            Common::StringWriter(result).Write("{0:local}", *this);
        }

        return result;
    }

    bool DateTime::TryParse (std::wstring const & str, __out DateTime & result)
    {
        // assumes string in the forms
        // 2005/07/06-12:30:08.869, 2016-04-20T10:30:30.999Z
        // 01234567890123456789012  012345678901234567890123
        // 0         1         2    0         1         2

        //to support docker format 
        //2018-02-23T11:22:12.1630849Z
        if (str.size() >= 24)
        {
            if (str[10] != L'T' || str[str.size() - 1] != L'Z')
            {
                return false;
            }
        }
        else if (str.size() != 23)
        {
            return false;
        }

        SYSTEMTIME st;
        FILETIME ft;

        st.wYear    = Int16_Parse(str.substr(0,4));
        st.wMonth   = Int16_Parse(str.substr(5,2));
        st.wDay     = Int16_Parse(str.substr(8,2));
        st.wHour    = Int16_Parse(str.substr(11,2));
        st.wMinute  = Int16_Parse(str.substr(14,2));
        st.wSecond  = Int16_Parse(str.substr(17,2));
        st.wMilliseconds = Int16_Parse(str.substr(20,3));

        if (!SystemTimeToFileTime(&st, &ft))
        {
            return false;
        }

        result = DateTime( *(int64*)&ft );
        return true;
    }

    DateTime DateTime::Parse (std::wstring const & str)
    {
        DateTime result;

        if (TryParse(str, result))
        {
            return result;
        }

        throw std::invalid_argument("Invalid date time format");
    }

    void DateTime::WriteTo(TextWriter& w, FormatOptions const & fmt) const
    {
        if (ticks_ == 0)
        {
            w.Write("0");
            return;
        }

        if (*this == DateTime::MaxValue)
        {
            w.Write("Infinite");
            return;
        }

        ::SYSTEMTIME st;
        ::FILETIME local;
        ::FILETIME localUtc = this->AsFileTime;

        if (fmt.formatString == "local")
        {
            BOOL success = ::FileTimeToLocalFileTime(&localUtc, &local);
            if (!success)
            {
                //
                // This might happned when someone converts a date, which is
                // close Zero time to a local time. Substracting timezone from
                // Zero time produces ironiose result.
                //
                w.Write("00-00-00 00:00:000.000");
                return;
            }
        }
        else
        {
            local = localUtc;
        }

        if (!FileTimeToSystemTime( &local, &st))
        {
            w.Write("00-00-00 00:00:000.000");
            return;
        }

        w.Write("{0:04d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d}.{6:03d}",
            st.wYear,st.wMonth,st.wDay,
            st.wHour,st.wMinute,st.wSecond, st.wMilliseconds);
    }

    std::wstring DateTime::ToIsoString()
    {
        if (ticks_ == 0)
        {
            // DateTime - Min value
            return L"0001-01-01T00:00:00.000Z";
        }

        ::SYSTEMTIME st;
        ::FILETIME ft = this->AsFileTime;
        if (!FileTimeToSystemTime(&ft, &st))
        {
            return L"0001-01-01T00:00:00.000Z";
        }

        std::wstringstream ss;
        ss << std::setfill(L'0');
        ss << std::setw(4);
        ss << st.wYear;
        ss << L"-";
        ss << std::setw(2);
        ss << st.wMonth;
        ss << L"-";
        ss << std::setw(2);
        ss << st.wDay;
        ss << L"T";
        ss << std::setw(2);
        ss << st.wHour;
        ss << L":";
        ss << std::setw(2);
        ss << st.wMinute;
        ss << L":";
        ss << std::setw(2);
        ss << st.wSecond;
        ss << L".";
        ss << std::setw(3);
        ss << st.wMilliseconds;
        ss << L"Z";
        
        return ss.str();
    }

    FILETIME DateTime::get_AsFileTime() const
    {
        ULARGE_INTEGER ticks;
        ticks.QuadPart = ticks_;

        FILETIME filetime;
        filetime.dwLowDateTime = ticks.LowPart;
        filetime.dwHighDateTime = ticks.HighPart;

        return filetime;
    }
    
    SYSTEMTIME DateTime::GetSystemTime() const
    {
        FILETIME fileTime = get_AsFileTime();
        SYSTEMTIME systemTime;
        BOOL result = FileTimeToSystemTime(&fileTime, &systemTime);
        if (!result)
        {
            ASSERT("FileTimeToSystemTime() failed");
        }
        
        return systemTime;
    }

    LARGE_INTEGER DateTime::get_AsLargeInteger() const
    {
        LARGE_INTEGER largeInteger;
        largeInteger.QuadPart = ticks_;

        return largeInteger;
    }


    int64 DateTime::SafeAdd (int64 lhsTicks, int64 rhsTicks) const
    {
        int64 tempTicks = lhsTicks + rhsTicks;
        if (tempTicks < 0) 
        {
            tempTicks = 1; // we don't want to kill cluster service if they adjust machine clock
            // exception will be used only in a debug builds
#ifdef DBG
            FAIL_CODING_ERROR_FMT("Result is negative: {0} {1}", lhsTicks, rhsTicks);
#endif
        }
        return tempTicks;
    }

    TimeSpan DateTime::RemainingDuration () const
    {
        TimeSpan result = *this - Now();
        if (result < TimeSpan::Zero)
        {
            return TimeSpan::Zero;
        }

        return result;
    }

    DateTime DateTime::AddWithMaxValueCheck (TimeSpan rhs) const
    {
        if (*this == DateTime::MaxValue || rhs == TimeSpan::MaxValue)
        {
            return DateTime::MaxValue;
        }

        return DateTime(SafeAdd(this->ticks_, rhs.ticks_));
    }

    DateTime DateTime::SubtractWithMaxValueCheck (TimeSpan rhs) const
    {
        if (*this == DateTime::MaxValue)
        {
            return DateTime::MaxValue;
        }
        else if(rhs == TimeSpan::MaxValue)
        {
            return DateTime::Zero;
        }

        return DateTime(SafeAdd(this->ticks_, -rhs.ticks_));
    }

    TimeSpan DateTime::SubtractWithMaxValueCheck (DateTime const& rhs) const
    {
        if (*this == DateTime::MaxValue)
        {
            return TimeSpan::MaxValue;
        }
        else if(rhs == DateTime::MaxValue)
        {
            return TimeSpan::MinValue;
        }

        //TimeSpan can be negative
        return TimeSpan(this->ticks_ - rhs.ticks_);
    }

#if defined(PLATFORM_UNIX)
    FILETIME DateTime::UnixTimeToFileTime (time_t sec, long nsec)
    {
        int64 result;
        FILETIME fileTime;
        
        result = ((int64)sec + SECS_BETWEEN_1601_AND_1970_EPOCHS) * SECS_TO_100NS + (nsec / 100);
        
        fileTime.dwLowDateTime = (DWORD)result;
        fileTime.dwHighDateTime = (DWORD)(result >> 32);
        
        return fileTime;
    }
    
    BOOL DateTime::FileTimeToSystemTime (CONST FILETIME * lpFileTime, LPSYSTEMTIME lpSystemTime)
    {
        UINT64 fileTime = 0;
        
        fileTime = lpFileTime->dwHighDateTime;
        fileTime <<= 32;
        fileTime |= (UINT)lpFileTime->dwLowDateTime;
        fileTime -= SECS_BETWEEN_1601_AND_1970_EPOCHS * SECS_TO_100NS;
        
        time_t unixFileTime = 0;
        
        if (((INT64)fileTime) < 0)
        {
            unixFileTime =  -1 - ( ( -fileTime - 1 ) / SECS_TO_100NS );
        }
        else
        {
            unixFileTime = fileTime / SECS_TO_100NS;
        }
        
        struct tm * unixSystemTime = 0;
        struct tm timeBuf;
        
        unixSystemTime = gmtime_r(&unixFileTime, &timeBuf);
        lpSystemTime->wDay = unixSystemTime->tm_mday;
        lpSystemTime->wMonth = unixSystemTime->tm_mon + 1;
        lpSystemTime->wYear = unixSystemTime->tm_year + 1900;
        lpSystemTime->wSecond = unixSystemTime->tm_sec;
        lpSystemTime->wMinute = unixSystemTime->tm_min;
        lpSystemTime->wHour = unixSystemTime->tm_hour;
        lpSystemTime->wMilliseconds = (fileTime / 10000) % 1000;
        
        return TRUE;
    }
#endif
} // end namespace Common
