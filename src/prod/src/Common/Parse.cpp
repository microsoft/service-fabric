// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace
{
    // Our own number parsing is implemented instead of using functions from stl or libc,
    // because both std and libc parsing code depends on static locale data, whose cleanup
    // during process shutdown can cause parsing failure or even memory access violation.
    // Also, we do not need to support locacle in number parsing.

    template<typename CharPtrType>
    bool TrySetSignAndBase(CharPtrType& beg, CharPtrType& end, uint& base, int& sign)
    {
        sign = 1;
        const bool canSetBase = (base == 0);
        if (canSetBase) base = 10;

        while (beg != end && beg[0] == ' ')
        {
            ++beg;
        }

        while (beg != end && end[-1] == ' ')
        {
            --end;
        }
        
        if (beg == end) return false;

        if (*beg == '+')
        {
            ++beg;
        }
        else if (*beg == '-')
        {
            ++beg;
            sign = -1;
        }
        else if (*beg == '0')
        {
            ++beg;
            if (beg == end) return true;

            if ((*beg == 'x') || (*beg == 'X'))
            {
                if (!canSetBase) return false;

                base = 16;
                ++beg;
            }
//            Octal prefix is not enabled, as we do not know if this will lead to backward compatibility issue
//            else if (canSetBase)
//            {
//                base = 8;
//            }
        }

        if (beg == end)
        {
            return false;
        }

        return true;
    }

    template<typename CharType>
    bool TryGetDigit(CharType ch, uint base, uint& delta)
    {
        if (('0' <= ch) && (ch <= '9'))
        {
            delta = (ch  - '0');
            return true;
        }

        if (base != 16) return false;

        if (('a' <= ch) && (ch <= 'f'))
        {
            delta = (ch - 'a') + 10;
            return true;
        }

        if (('A' <= ch) && (ch <= 'F'))
        {
            delta = (ch - 'A') + 10;
            return true;
        }

        return false;
    }

    double GetPowConstantAt(uint i)
    {
        static const double powerConstants[] =
            {1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128, 1e256 }; //1e512 > 1e308, 308 == max_exponent10

        Invariant(i < ARRAYSIZE(powerConstants));
        return powerConstants[i];
    }

    auto staticInitTrigger = GetPowConstantAt(0);

    double PowOf10(int64 e)
    {
        Invariant(e >= 0);

        double result = 1.0;

        for (uint i = 0; e > 0; ++i, e = e >> 1)
        {
            if (e & 1)
            {
                result *= GetPowConstantAt(i); 
            }
        }

        return result; 
    }

    template <typename StringType>
    bool TraceParseReturnT(bool retval, Common::StringLiteral traceType, StringType const & input, const char* targetType, uint base)
    {
        if (retval) return retval;

        if (base)
        {
            Trace.WriteInfo(traceType, "failed to parse '{0}' as {1} with base = {2}", input, targetType, base);
            return retval;
        }

        Trace.WriteInfo(traceType, "failed to parse '{0}' as {1}", input, targetType);
        return retval;
    }
}

namespace Common
{
    bool TraceParseReturn(bool retval, StringLiteral traceType, wstring const & input, const char* targetType, uint base)
    {
        return TraceParseReturnT(retval, traceType, input, targetType, base);
    }

    bool TraceParseReturn(bool retval, StringLiteral traceType, string const & input, const char* targetType, uint base)
    {
        return TraceParseReturnT(retval, traceType, input, targetType, base);
    }

    template <typename StringType>
    bool TryParseUInt64(StringType const & str, __out uint64 & value, uint base) 
    {
        value = 0;

        auto beg = str.c_str();
        auto end = beg + str.size();

        int sign = 1;
        if (!TrySetSignAndBase(beg, end, base, sign)) return false;
        if (sign < 0) return false;

        constexpr auto uint64Max = std::numeric_limits<uint64>::max();
        const auto mulOverflowCheck  = uint64Max/base;

        while(beg < end)
        {
            uint delta = 0;
            if (!TryGetDigit(*beg, base, delta)) return false; 

            if (value > mulOverflowCheck) return false; 

            value *= base;
            if ((value + delta) < value) return false;

            value += delta;

            ++beg;
        }

        return true;
    }

    template <typename CharPtrType>
    bool TryParseInt64(CharPtrType beg, CharPtrType end, __out int64 & value, uint base) 
    {
        value = 0;

        int sign = 1;
        if (!TrySetSignAndBase(beg, end, base, sign)) return false;

        constexpr auto int64Max = std::numeric_limits<int64>::max();
        const auto mulOverflowCheck  = int64Max/base;

        while(beg < end)
        {
            uint delta = 0;
            if (!TryGetDigit(*beg, base, delta)) return false; 

            //ID 987533: check overflow early to prevent overflow from positive to positive 
            //value=0x2666,6666,6666,6666 and ch = 3: ==>0x2666,6666,6666,6666 * 0xa + 0x3 = 0x1,7FFF,FFFF,FFFF,FFFFC + 0x3 = 0x1,7FFFF,FFFF,FFFF,FFFF
            //It also prevents overflow into Int64_MinValue. 			
            //value=0x2666,6666,6666,6666 and ch = 4: ==>0x2666,6666,6666,6666 * 0xa + 0x4 = 0x1,7FFF,FFFF,FFFF,FFFC + 0x4 = 0x1,8000,0000,0000,0000
            if (value > mulOverflowCheck) return false; 

            value = (value * base) + delta;
            if (value < 0)
            {
                constexpr auto int64Min = std::numeric_limits<int64>::min();
                if (value == int64Min && sign < 0 && beg + 1 == end)
                {
                    return true;
                }
                return false;
            }

            ++beg;
        }

        value *= sign;
        return true;
    }

    template <typename StringType>
    bool TryParseInt64(StringType const & str, __out int64 & value, uint base) 
    {
        return TryParseInt64(str.c_str(), str.c_str() + str.size(), value, base);
    }

    bool TryParseUInt64(std::wstring const & str, __out uint64 & value, uint base) 
    {
        return TryParseUInt64<std::wstring>(str, value, base);
    }

    bool TryParseUInt64(std::string const & str, __out uint64 & value, uint base) 
    {
        return TryParseUInt64<std::string>(str, value, base);
    }

    bool TryParseInt64(std::wstring const & str, __out int64 & value, uint base) 
    {
        return TryParseInt64<std::wstring>(str, value, base);
    }

    bool TryParseInt64(std::string const & str, __out int64 & value, uint base) 
    {
        return TryParseInt64<std::string>(str, value, base);
    }

    bool TryParseDouble(std::wstring const & str, double& value)
    {
        return TryParseDouble(string(str.cbegin(), str.cend()), value);
    }

    bool TryParseDouble(std::string const & str, double& value)
    {
        value = 0;

        auto beg = str.c_str();
        auto end = beg + str.size();

        while (beg != end && beg[0] == ' ')
        {
            ++beg;
        }

        if (beg == end) return false;

        int sign = 1;
        if (*beg == '+')
        {
            ++beg;
        }
        else if (*beg == '-')
        {
            ++beg;
            sign = -1;
        }

        if (beg == end) return false;

        bool foundE = false;
        const char* dotPtr = nullptr; 
        uint digitCount = 0;

        uint64 fraction = 0;
        while(beg < end)
        {
            auto ch = *beg;
            uint delta = 0;
            if (('0' <= ch) && (ch <= '9'))
            {
                ++digitCount;
                ++beg;
                if (digitCount <= 18)
                {
                    delta = ch  - '0';
                }
            }
            else if (ch == '.')
            {
                if (dotPtr) return false;

                dotPtr = beg;
                ++beg;
                continue;
            }
            else
            {
                foundE = ((ch == 'e' || ch == 'E'));
                break;
            }

            fraction = fraction * 10 + delta;
        }

        int64 expDelta = 0;
        if (dotPtr)
        {
            expDelta = beg - dotPtr - 1;
        }

        int64 exponent = 0;
        if (foundE)
        {
            ++beg;
            if (!TryParseInt64(beg, end, exponent, 10)) return false;
        }

        if (exponent > 0)
        {
            exponent -= expDelta;
            constexpr auto expMax = numeric_limits<double>::max_exponent10;
            if (exponent > expMax) return false;

            auto powerValue = PowOf10(exponent);

            constexpr auto doubleMax = numeric_limits<double>::max();
            if (fraction > (doubleMax/powerValue)) return false;

            value = fraction * powerValue;
        }
        else
        {
            constexpr auto emin = numeric_limits<double>::min_exponent10 - 1;
            if (exponent < emin) return false;

            exponent -= expDelta;

            if (exponent < emin)
            {
                auto powerValue = PowOf10(-emin);
                auto delta = emin - exponent;
                auto powerValueDelta = PowOf10(delta);

                value = (fraction / powerValueDelta) / powerValue;
            }
            else
            {
                auto powerValue = PowOf10(-exponent);
                value = fraction / powerValue;
            }
        }

        if (sign < 0) value *= -1;

        return true;
    }

    double Double_Parse(std::wstring const & str)
    {
        double result = 0;
        TraceParseReturn(
            TryParseDouble(str, result),
            __FUNCTION__,
            str,
            "double");

        return result;
    }

    double Double_Parse(std::string const & str)
    {
        double result = 0;
        TraceParseReturn(
            TryParseDouble(str, result),
            __FUNCTION__,
            str,
            "double"); 

        return result;
    }
}
