// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Parse.h"
#include <stdlib.h>

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(ParseTests)

    BOOST_AUTO_TEST_CASE(Smoke0)
    {
        int value = 101;
        auto retval = StringUtility::TryFromWString(L"0", value);
        VERIFY_IS_TRUE(retval);
        VERIFY_ARE_EQUAL2(value, 0);

        uint uvalue = 101;
        retval = StringUtility::TryFromWString(L"0", uvalue);
        VERIFY_IS_TRUE(retval);
        VERIFY_ARE_EQUAL2(uvalue, 0U);
    }

    BOOST_AUTO_TEST_CASE(Smoke1)
    {
        __int32 i = 5;

        std::string s = "5";

        VERIFY_IS_TRUE(Int32_Parse(s) == i);
    }

    BOOST_AUTO_TEST_CASE(Smoke2)
    {
        VERIFY_IS_TRUE(Int32_Parse("    12348 ") == 12348);
    }

    BOOST_AUTO_TEST_CASE(Smoke3)
    {
        VERIFY_IS_TRUE(Int32_Parse("-42") == -42);
    }

    BOOST_AUTO_TEST_CASE(Smoke4)
    {
        VERIFY_IS_TRUE(Int32_Parse(L"-42   ") == -42);
    }

    BOOST_AUTO_TEST_CASE(Smoke5)
    {
        VERIFY_IS_TRUE(Int32_Parse(std::wstring(L"5")) == 5);
    }

    BOOST_AUTO_TEST_CASE(Smoke6)
    {
        // Expected format: [whitespace] [sign] [digits] [.digits] [ {e | E}[sign]digits]
        VERIFY_ARE_EQUAL2(Double_Parse(std::wstring(L"  3.14")), 3.14);
        VERIFY_ARE_EQUAL2(Double_Parse(std::string("-3.14")), -3.14);

        VERIFY_ARE_EQUAL2(Double_Parse(L"-3.14   "), -3.14);
        VERIFY_ARE_EQUAL2(Double_Parse("3.14 Followed by some string"), 3.14);

        VERIFY_ARE_EQUAL2(Double_Parse(L"3.14e-2"), 3.14e-2);
        VERIFY_ARE_EQUAL2(Double_Parse("3.14e8"), 3.14e8);
        VERIFY_ARE_EQUAL2(Double_Parse("-3.14e3"), -3.14e3);
        VERIFY_ARE_EQUAL2(Double_Parse("-3.14e8"), -3.14e8);
        VERIFY_ARE_EQUAL2(Double_Parse("0"), 0); 
        VERIFY_ARE_EQUAL2(Double_Parse("-0"), 0); 
        VERIFY_ARE_EQUAL2(Double_Parse("0.0"), 0); 
        VERIFY_ARE_EQUAL2(Double_Parse("-0.0"), 0); 

        {
            auto value = numeric_limits<double>::min();
            stringstream ss;
            ss << value;

            auto parsed_by_strtod = strtod(ss.str().c_str(), nullptr);
            auto delta = std::abs(value - parsed_by_strtod);
            printf("value = %e, strtod = %e, abs(delta) = %e\n", value, parsed_by_strtod, delta);

            double parsedValue = 0;
            auto success = TryParseDouble(ss.str(), parsedValue);
            VERIFY_IS_TRUE(success);
            delta = std::abs(value - parsedValue);
            printf("value = %e, parsed = %e, abs(delta) = %e\n", value, parsedValue, delta);
            VERIFY_IS_TRUE(delta <= 1e-300);
        }

        {
            auto value = numeric_limits<double>::lowest();
            stringstream ss;
            ss << value;

            auto parsed_by_strtod = strtod(ss.str().c_str(), nullptr);
            auto delta = std::abs(value - parsed_by_strtod);
            printf("value = %e, strtod = %e, abs(delta) = %e\n", value, parsed_by_strtod, delta);

            double parsedValue = 0;
            auto success = TryParseDouble(ss.str(), parsedValue);
            VERIFY_IS_TRUE(success);

            delta = std::abs(value - parsedValue);
            printf("value = %e, parsed = %e, abs(delta) = %e\n", value, parsedValue, delta);
            VERIFY_IS_TRUE(delta <= 0.00001*std::abs(value));

            VERIFY_ARE_EQUAL2(parsedValue, parsed_by_strtod);
        }

        {
            auto value =  numeric_limits<double>::max();
            stringstream ss;
            ss << value;

            auto parsed_by_strtod = strtod(ss.str().c_str(), nullptr);
            auto delta = std::abs(value - parsed_by_strtod);
            printf("value = %e, strtod = %e, abs(delta) = %e\n", value, parsed_by_strtod, delta);

            double parsedValue = 0;
            auto success = TryParseDouble(ss.str(), parsedValue);
            VERIFY_IS_TRUE(success);

            delta = std::abs(value - parsedValue);
            printf("value = %e, parsed = %e, abs(delta) = %e\n", value, parsedValue, delta);
            VERIFY_IS_TRUE(delta <= 0.00001*value);

            VERIFY_ARE_EQUAL2(parsedValue, parsed_by_strtod);
        }

        Random r;
        for (int i = 0; i < 10; ++i)
        {
            auto value = r.NextDouble() * r.Next(numeric_limits<int>::max());
            stringstream ss;
            ss << value;

            auto parsed_by_strtod = strtod(ss.str().c_str(), nullptr);
            auto delta = std::abs(value - parsed_by_strtod);
            printf("value = %e, strtod = %e, abs(delta) = %e\n", value, parsed_by_strtod, delta);

            double parsedValue = 0;
            auto success = TryParseDouble(ss.str(), parsedValue);
            VERIFY_IS_TRUE(success);

            delta = std::abs(value - parsedValue);
            printf("value = %e, parsed = %e, abs(delta) = %e\n", value, parsedValue, delta);
            VERIFY_IS_TRUE(delta <= 0.00001*std::abs(value));

            VERIFY_ARE_EQUAL2(parsedValue, parsed_by_strtod);
        }
    }
	
    BOOST_AUTO_TEST_CASE(Smoke7)
    {
        std::string sMin;
        StringWriterA(sMin).Write("{0}", (std::numeric_limits<__int64>::min()));

        std::string sMax;
        StringWriterA(sMax).Write("{0}", (std::numeric_limits<__int64>::max()));

        VERIFY_IS_TRUE(Int64_Parse(sMin) == std::numeric_limits<__int64>::min());  
        VERIFY_IS_TRUE(Int64_Parse(sMax) == std::numeric_limits<__int64>::max());  
        VERIFY_IS_TRUE(Int64_Parse("2767011611056432742") == 2767011611056432742);
        VERIFY_ARE_EQUAL2(Int64_Parse("0"), 0);
        VERIFY_ARE_EQUAL2(Int64_Parse("0x0"), 0);
        VERIFY_ARE_EQUAL2(Int64_Parse("0x123456789a"), 0x123456789a);
        VERIFY_ARE_EQUAL2(Int64_Parse("0x0123456789a"), 0x123456789a);
        VERIFY_ARE_EQUAL2(Int64_Parse("0x000123456789a"), 0x123456789a);

        int64 result = 0;
        auto success = TryParseInt64("123456", result, 16);
        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL2(result, 0x123456);

        success = TryParseInt64("0xffffffffffffffff", result);
        VERIFY_IS_FALSE(success);

        uint64 uresult = 0;
        success = TryParseUInt64("0xffffffffffffffff", uresult);
        VERIFY_IS_TRUE(success);
        success = TryParseUInt64("ffffffffffffffff", uresult, 16);
        VERIFY_IS_TRUE(success);

        uint32 uint32Result = 0;
        success = StringUtility::TryFromWString(L"0xffffffff", uint32Result);
        VERIFY_IS_TRUE(success);
        success = StringUtility::TryFromWString(L"ffffffff", uint32Result, 16);
        VERIFY_IS_TRUE(success);
    }

    BOOST_AUTO_TEST_CASE(TryFromWStringError)
    {
        bool b = false;
        auto retval = StringUtility::TryFromWString(L"notbool", b);
        VERIFY_IS_FALSE(retval);

        int i = 0;
        retval = StringUtility::TryFromWString(L"notint", i);
        VERIFY_IS_FALSE(retval);

        retval = StringUtility::TryFromWString(L"0x333", i, 10);
        VERIFY_IS_FALSE(retval);

        unsigned short s = 0;
        retval = StringUtility::TryFromWString(L"notshort", s);
        VERIFY_IS_FALSE(retval);

        retval = StringUtility::TryFromWString(L"999999", s);
        VERIFY_IS_FALSE(retval);

        retval = StringUtility::TryFromWString(L"-1", s);
        VERIFY_IS_FALSE(retval);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int16)
    {
        std::string s;
        StringWriterA(s).Write("{0}", (std::numeric_limits<__int16>::min() - 1));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int16_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Overflow_Int16)
    {
        std::string s;
        StringWriterA(s).Write("{0}", (std::numeric_limits<__int16>::max() + 1));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int16_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int32)
    {
        std::string s;
        StringWriterA(s).Write("{0}1", (std::numeric_limits<__int32>::min()));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int32_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Overflow_Int32)
    {
        std::string s;
        StringWriterA(s).Write("{0}1", (std::numeric_limits<__int32>::max()));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int32_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int64)
    {
        std::string s;
        StringWriterA(s).Write("{0}1", std::numeric_limits<__int64>::min());

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int64_Parse(s), std::system_error);
    }
	
    BOOST_AUTO_TEST_CASE(Overflow_Int64)
    {
        std::string s = "27670116110564327422";  //0X26666,6666,6666,6666*0xa+2        		

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int64_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int16_W)
    {
        std::wstring s;
        StringWriter(s).Write("{0}", (std::numeric_limits<__int16>::min() - 1));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int16_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Overflow_Int16_W)
    {
        std::wstring s;
        StringWriter(s).Write("{0}", (std::numeric_limits<__int16>::max() + 1));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int16_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int32_W)
    {
        std::wstring s;
        StringWriter(s).Write("{0}1", (std::numeric_limits<__int32>::min()));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int32_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Overflow_Int32_W)
    {
        std::wstring s;
        StringWriter(s).Write("{0}1", (std::numeric_limits<__int32>::max()));

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int32_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Underflow_Int64_W)
    {
        std::wstring s;
        StringWriter(s).Write("{0}1", std::numeric_limits<__int64>::min());

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int64_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(Overflow_Int64_W)
    {
        std::wstring s = L"27670116110564327422";  //0X26666,6666,6666,6666*0xa+2

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Int64_Parse(s), std::system_error);
    }

    BOOST_AUTO_TEST_SUITE_END()
};
