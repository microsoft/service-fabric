// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include "Common/boost-taef.h"
#include "Common/DateTime.h"
#include "Common/Random.h"

namespace Common
{
    class TestDateTime
    {
    };

    static int64 const TicksPerMillisecond = 10000;
    static int64 const TicksPerSecond = TicksPerMillisecond * 1000;

    BOOST_FIXTURE_TEST_SUITE(TestDateTimeSuite,TestDateTime)


    BOOST_AUTO_TEST_CASE(BasicDateTimeTest)
    {
        DateTime dateTime = DateTime::Now();
        VERIFY_IS_TRUE(dateTime > DateTime::Zero);
        VERIFY_IS_TRUE(dateTime < DateTime::MaxValue);
    }
    
    BOOST_AUTO_TEST_CASE(AddDateTimeTest)
    {
        Random random = Random();
        //Testing unsafe add. 
        double seconds = static_cast<double>(random.Next());

        DateTime dateTime = DateTime::Now();
        TimeSpan timeSpan = TimeSpan::FromSeconds(seconds);

        int64 dateTimeTicks = dateTime.Ticks;
        DateTime result = dateTime + timeSpan;
        VERIFY_IS_TRUE(result.Ticks == (dateTimeTicks + (static_cast<int64>(seconds) * TicksPerSecond)));
        
        //Testing checked add
        dateTime = DateTime::MaxValue;
        timeSpan = TimeSpan::FromSeconds(seconds);

        result = dateTime.AddWithMaxValueCheck(timeSpan);
        VERIFY_IS_TRUE(result == DateTime::MaxValue);
    }

   
    BOOST_AUTO_TEST_CASE(SubtractDateTimeTest)
    {
        Random random = Random();
        //Testing unsafe add. 
        double seconds = static_cast<double>(random.Next());

        DateTime dateTime = DateTime::Now();
        TimeSpan timeSpan = TimeSpan::FromSeconds(seconds);

        int64 dateTimeTicks = dateTime.Ticks;
        DateTime result = dateTime - timeSpan;
        VERIFY_IS_TRUE(result.Ticks == (dateTimeTicks - (static_cast<int64>(seconds) * TicksPerSecond)));

        //Testing checked subtract
        dateTime = DateTime::MaxValue;

        result = dateTime.SubtractWithMaxValueCheck(timeSpan);
        VERIFY_IS_TRUE(result == DateTime::MaxValue);

        DateTime dateTime1 = DateTime::Now();
        DateTime dateTime2 = DateTime::MaxValue;
        TimeSpan timeSpanResult = dateTime1.SubtractWithMaxValueCheck(dateTime2);
        VERIFY_IS_TRUE(timeSpanResult == TimeSpan::MinValue);

        dateTime = dateTime1 + timeSpan;
        timeSpanResult = dateTime - dateTime1;
        VERIFY_IS_TRUE(timeSpanResult == timeSpan);
    }

    BOOST_AUTO_TEST_CASE(IsoStringTest)
    {
        DateTime dateTime = DateTime::Now();
        std::wstring isoStr = dateTime.ToIsoString();
        DateTime dateTime2 = DateTime(isoStr);
        std::wstring isoStr2 = dateTime.ToIsoString();
        VERIFY_ARE_EQUAL(isoStr, isoStr2);
        std::wstring str = dateTime.ToString(true);
        DateTime dateTime3 = DateTime(str);
        VERIFY_ARE_EQUAL(isoStr, dateTime3.ToIsoString());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
