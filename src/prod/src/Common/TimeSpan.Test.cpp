// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/TimeSpan.h"
#include "Common/Random.h"

namespace Common
{
    class TestTimeSpan
    {
    };

    static int64  const Int64MaxValue = std::numeric_limits<int64>::max();
    static int64  const Int64MinValue = std::numeric_limits<int64>::min();

    static int64 const TicksPerMillisecond = 10000;
    static int64 const TicksPerSecond = TicksPerMillisecond * 1000;
    static int64 const TicksPerMinute = TicksPerSecond * 60;

    static double const MaxMilliseconds = static_cast<double>(Int64MaxValue / TicksPerMillisecond);
    static double const MaxSeconds = static_cast<double>(Int64MaxValue / TicksPerSecond);
    static double const MaxMinutes = static_cast<double>(Int64MaxValue / TicksPerMinute);

    BOOST_FIXTURE_TEST_SUITE(TestTimeSpanSuite,TestTimeSpan)

    BOOST_AUTO_TEST_CASE(CreateTimeSpanTest)
    {
        //Testing int64.MaxValue
        int64 const maxValue = std::numeric_limits<int64>::max();
        int seconds = 5;
        int milliseconds = 477;
        int minutes = 48;
        int hours = 2;
        int days = 10675199;

        TimeSpan timeSpan = TimeSpan::FromTicks(maxValue);
        VERIFY_IS_TRUE(timeSpan.Seconds == seconds);
        VERIFY_IS_TRUE(timeSpan.Milliseconds == milliseconds);
        VERIFY_IS_TRUE(timeSpan.Minutes == minutes);
        VERIFY_IS_TRUE(timeSpan.Hours == hours);
        VERIFY_IS_TRUE(timeSpan.Days == days);

        //Testing max values for milli, seconds and minutes

        timeSpan = TimeSpan::FromMilliseconds(MaxMilliseconds + 1);
        VERIFY_IS_TRUE(timeSpan == TimeSpan::MaxValue);

        timeSpan = TimeSpan::FromSeconds(MaxSeconds + 1);
        VERIFY_IS_TRUE(timeSpan == TimeSpan::MaxValue);

        timeSpan = TimeSpan::FromMinutes(MaxMinutes + 1);
        VERIFY_IS_TRUE(timeSpan == TimeSpan::MaxValue);

        //Testing negative timespan
        int64 const negativeValue = -837453874837489;
        seconds = -27;
        milliseconds = -483;
        minutes = -36;
        hours = -6;
        days = -969;

        timeSpan = TimeSpan::FromTicks(negativeValue);
        VERIFY_IS_TRUE(timeSpan.Seconds == seconds);
        VERIFY_IS_TRUE(timeSpan.Milliseconds == milliseconds);
        VERIFY_IS_TRUE(timeSpan.Minutes == minutes);
        VERIFY_IS_TRUE(timeSpan.Hours == hours);
        VERIFY_IS_TRUE(timeSpan.Days == days);

        //Testing TimeSpan::Zero
        timeSpan = TimeSpan::FromSeconds(0);
        VERIFY_IS_TRUE(timeSpan == TimeSpan::Zero);
    }

    BOOST_AUTO_TEST_CASE(TotalPositiveMillisecondsTest)
    {
        double seconds = -10;
        double milliseconds = static_cast<double>(Int64MinValue);

        TimeSpan timeSpan1 = TimeSpan::FromSeconds(seconds);
        TimeSpan timeSpan2 = TimeSpan::FromMilliseconds(milliseconds);

        VERIFY_IS_TRUE(timeSpan1.TotalPositiveMilliseconds() == 0);
        VERIFY_IS_TRUE(timeSpan2.TotalPositiveMilliseconds() == 0);
    }

    BOOST_AUTO_TEST_CASE(AddTimeSpanTest)
    {
        Random random = Random();
        //Testing unsafe add. 
        double seconds1 = static_cast<double>(random.Next());
        double seconds2 = static_cast<double>(random.Next());

        TimeSpan timeSpan1 = TimeSpan::FromSeconds(seconds1);
        TimeSpan timeSpan2 = TimeSpan::FromSeconds(seconds2);

        TimeSpan result = timeSpan1 + timeSpan2;
        VERIFY_IS_TRUE(result.TotalSeconds() == ((seconds1 * TicksPerSecond + seconds2 * TicksPerSecond)/TicksPerSecond));

        //Testing safe add
        timeSpan1 = TimeSpan::MaxValue;
        timeSpan2 = TimeSpan::FromSeconds(seconds2);

        result = timeSpan1.AddWithMaxAndMinValueCheck(timeSpan2);
        VERIFY_IS_TRUE(result == TimeSpan::MaxValue);
    }

    BOOST_AUTO_TEST_CASE(SubtractTimeSpanTest)
    {
        Random random = Random();
        //Testing unsafe subtract. 
        double seconds1 = static_cast<double>(random.Next());
        double seconds2 = static_cast<double>(random.Next());

        TimeSpan timeSpan1 = TimeSpan::FromSeconds(seconds1);
        TimeSpan timeSpan2 = TimeSpan::FromSeconds(seconds2);

        TimeSpan result = timeSpan1 - timeSpan2;
        VERIFY_IS_TRUE(result.TotalSeconds() == ((seconds1 * TicksPerSecond - seconds2 * TicksPerSecond)/TicksPerSecond));

        //Testing safe subtract
        timeSpan1 = TimeSpan::MinValue;
        timeSpan2 = TimeSpan::FromSeconds(seconds2);

        result = timeSpan1.SubtractWithMaxAndMinValueCheck(timeSpan2);
        VERIFY_IS_TRUE(result == TimeSpan::MinValue);

        timeSpan1 = TimeSpan::MinValue;
        timeSpan2 = TimeSpan::MaxValue;
        result = timeSpan1.SubtractWithMaxAndMinValueCheck(timeSpan2);
        VERIFY_IS_TRUE(result == TimeSpan::MinValue);
    }

    BOOST_AUTO_TEST_CASE(TimeSpanOperatorsTest)
    {
        TimeSpan timeSpan1 = TimeSpan::FromMinutes(100);
        TimeSpan timeSpan2 = TimeSpan::FromMinutes(200);
        TimeSpan timeSpan3 = TimeSpan::FromMinutes(200);
        TimeSpan timeSpan4 = TimeSpan::FromMinutes(400);

        VERIFY_IS_TRUE(timeSpan1 < timeSpan2);
        VERIFY_IS_TRUE(timeSpan1 != timeSpan2);
        VERIFY_IS_TRUE(timeSpan3 == timeSpan2);
        VERIFY_IS_TRUE(timeSpan4 > timeSpan2);
        VERIFY_IS_TRUE(timeSpan3 <= timeSpan2);
        VERIFY_IS_TRUE(timeSpan4 >= timeSpan1);
    }

    BOOST_AUTO_TEST_CASE(TimeSpanIsoStringTest)
    {
        TimeSpan timeSpan1 = TimeSpan::FromMinutes(10);
        std::wstring str = timeSpan1.ToIsoString();
        TimeSpan timeSpan2;
        VERIFY_IS_TRUE(TimeSpan::TryFromIsoString(str, timeSpan2));
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(59);
        str = timeSpan1.ToIsoString();
        VERIFY_IS_TRUE(TimeSpan::TryFromIsoString(str, timeSpan2));
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(59) + TimeSpan::FromSeconds(1);
        str = timeSpan1.ToIsoString();
        VERIFY_IS_TRUE(TimeSpan::TryFromIsoString(str, timeSpan2));
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(1) + TimeSpan::FromSeconds(1);
        str = timeSpan1.ToIsoString();
        VERIFY_IS_TRUE(TimeSpan::TryFromIsoString(str, timeSpan2));
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(1441);// 1 day 1 minute
        str = timeSpan1.ToIsoString();
        VERIFY_IS_TRUE(TimeSpan::TryFromIsoString(str, timeSpan2));
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(2880);// 2 days
        str = timeSpan1.ToIsoString();
        TimeSpan::TryFromIsoString(str, timeSpan2);
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::FromMinutes(1440 * 30);// 1 month
        str = timeSpan1.ToIsoString();
        TimeSpan::TryFromIsoString(str, timeSpan2);
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::MaxValue;
        str = timeSpan1.ToIsoString();
        TimeSpan::TryFromIsoString(str, timeSpan2);
        str = timeSpan2.ToIsoString();
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);

        timeSpan1 = TimeSpan::MinValue;
        str = timeSpan1.ToIsoString();
        TimeSpan::TryFromIsoString(str, timeSpan2);
        auto str2 = timeSpan2.ToIsoString();
        VERIFY_ARE_EQUAL(timeSpan1, timeSpan2);
        VERIFY_ARE_EQUAL(str, str2);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
