// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Throw.h"

namespace Common
{
    bool TryParseUInt64(std::wstring const & str, __out uint64 & value, uint base = 0 /* will figure out base */);
    bool TryParseUInt64(std::string const & str, __out uint64 & value, uint base = 0);

    bool TryParseInt64(std::wstring const & str, __out int64 & value, uint base = 0); 
    bool TryParseInt64(std::string const & str, __out int64 & value, uint base = 0); 

    template <typename StringType>
    int64 Int64_Parse(StringType const & str)
    {
        int64 value;
        if (!TryParseInt64(str, value))
        {
            THROW(microsoft::MakeWindowsErrorCode(ERROR_ARITHMETIC_OVERFLOW), "Not a valid number");
        }

        return value;
    }

    template <int N>
    inline int64 Int64_Parse(char const (&str)[N])
    {
        return Int64_Parse(std::string(str, N - 1));
    }

    template <int N>
    inline int64 Int64_parse(wchar_t const (&str)[N])
    {
        return Int64_Parse(std::wstring(str, N - 1));
    }

    template <typename StringType>
    int32 Int32_Parse(StringType const & value)
    {
        int64 result = Int64_Parse(value);
        if (std::numeric_limits<int32>::min() <= result && result <= std::numeric_limits<int32>::max()) {
            return int32(result);
        }
        THROW(microsoft::MakeWindowsErrorCode(ERROR_ARITHMETIC_OVERFLOW), "value doesn't fit a Int32");
    }

    template <int N>
    inline int32 Int32_Parse(char const (&str)[N])
    {
        return Int32_Parse(std::string(str, N - 1));
    }

    template <int N>
    inline int32 Int32_Parse(wchar_t const (&str)[N])
    {
        return Int32_Parse(std::wstring(str, N - 1));
    }

    template <typename StringType>
    int16 Int16_Parse(StringType const & value)
    {
        int64 result = Int64_Parse(value);
        if (std::numeric_limits<int16>::min() <= result && result <= std::numeric_limits<int16>::max()) {
            return int16(result);
        }
        THROW(microsoft::MakeWindowsErrorCode(ERROR_ARITHMETIC_OVERFLOW), "value doesn't fit a Int16");
    }

    template <int N>
    inline int16 Int16_Parse(char const (&str)[N])
    {
        return Int16_Parse(std::string(str, N - 1));
    }

    template <int N>
    inline int16 Int16_Parse(wchar_t const (&str)[N])
    {
        return Int16_Parse(std::wstring(str, N - 1));
    }
        
    bool TryParseDouble(std::wstring const & str, double& value);
    bool TryParseDouble(std::string const & str, double& value);

    double Double_Parse(std::wstring const & value);
    double Double_Parse(std::string const & value);

    bool TraceParseReturn(bool retval, StringLiteral traceType, std::string const & input, const char* targetType, uint base = 0);
    bool TraceParseReturn(bool retval, StringLiteral traceType, std::wstring const & input, const char* targetType, uint base = 0);
}
