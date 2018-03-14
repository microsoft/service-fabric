// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TextWriter;

    class DateTime
    {
    public:
        DateTime() : ticks_ (0) {} 
        explicit DateTime (std::wstring const & str) : ticks_ (Parse(str).ticks_) {}
        explicit DateTime(int64 ticks) : ticks_(ticks) {}
        explicit DateTime(FILETIME filetime);
        explicit DateTime(LARGE_INTEGER largeIntTime);

        __declspec (property(get = get_Ticks))      int64 Ticks;
        __declspec (property(get = get_AsFileTime)) FILETIME AsFileTime;
        __declspec (property(get = get_AsLargeInteger)) LARGE_INTEGER AsLargeInteger;

        TimeSpan operator- (DateTime const& rhs) const;
        bool operator == (DateTime const& rhs) const { return ticks_ == rhs.ticks_; }
        bool operator != (DateTime const& rhs) const { return ticks_ != rhs.ticks_; }
        bool operator < (DateTime const& rhs) const { return ticks_ < rhs.ticks_; }
        bool operator > (DateTime const& rhs) const { return ticks_ > rhs.ticks_; }
        bool operator >= (DateTime const& rhs) const { return ticks_ >= rhs.ticks_; }
        bool operator <= (DateTime const& rhs) const { return ticks_ <= rhs.ticks_; }

        TimeSpan RemainingDuration () const;
        DateTime AddWithMaxValueCheck (TimeSpan rhs) const;
        DateTime SubtractWithMaxValueCheck (TimeSpan rhs) const;
        TimeSpan SubtractWithMaxValueCheck (DateTime const & rhs) const;
        SYSTEMTIME GetSystemTime() const;
        std::wstring ToString(bool utc = true) const;
        void WriteTo(TextWriter& w, FormatOptions const &) const;
        std::wstring ToIsoString();

        FILETIME get_AsFileTime() const;
        LARGE_INTEGER get_AsLargeInteger() const;
        bool IsValid() const { return ticks_ != 0; }

        DateTime operator+ (TimeSpan const& rhs) const
        {
            return DateTime (this->ticks_ + rhs.ticks_);
        }

        DateTime operator- (TimeSpan const& rhs) const
        {
            return DateTime (this->ticks_ - rhs.ticks_);
        }

        TimeSpan UnSafeRemainingDuration () const
        {
            return *this - Now();
        }

        int64 get_Ticks() const
        {
            return ticks_;
        }

        static DateTime Now() { return get_Now() ; }
        static DateTime get_Now();
        static DateTime Parse (std::wstring const & str);
        static bool TryParse (std::wstring const & str, __out DateTime & result);

#if defined(PLATFORM_UNIX)
        static BOOL FileTimeToSystemTime(CONST FILETIME * lpFileTime, LPSYSTEMTIME lpSystemTime);
#endif
        static DateTime const Zero;
        static DateTime const MaxValue;

        FABRIC_PRIMITIVE_FIELDS_01(ticks_);

    private:
#if defined(PLATFORM_UNIX)
        static FILETIME UnixTimeToFileTime (time_t sec, long nsec);
#endif
        int64 SafeAdd (int64 lhsTicks, int64 rhsTicks) const;
        int64 ticks_;
#if defined(PLATFORM_UNIX)
        static const int64 SECS_BETWEEN_1601_AND_1970_EPOCHS;
        static const __int64 SECS_TO_100NS;
#endif
    };
};
