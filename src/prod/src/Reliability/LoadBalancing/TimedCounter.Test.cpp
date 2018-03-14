// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TimedCounter.h"
#include "IntervalCounter.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Reliability::LoadBalancingComponent;

    class TestTimedCounter
    {
    protected:
        TestTimedCounter() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);

        void Record(TimedCounter & counter, size_t count, wstring const & time)
        {
            counter.Record(count, GetTime(time));
        }

        size_t GetCount(TimedCounter & counter, wstring const & time)
        {
            counter.Refresh(GetTime(time));
            return counter.GetCount();
        }

        StopwatchTime GetTime(wstring const & time)
        {
            DateTime now;
            DateTime::TryParse(GetTimeString(time), now);

            if (!hasStartDateTime_)
            {
                hasStartDateTime_ = true;
                startDateTime_ = now;
            }

            return StartStopwatchTime + (now - startDateTime_);
        }

        wstring GetTimeString(wstring const & time) const
        {
            return L"2012/04/10-" + time + L".000";
        }

        static const StopwatchTime StartStopwatchTime;
        StopwatchTime testTime_;
        bool hasStartDateTime_;
        DateTime startDateTime_;
    };

    // fix the starting time to simplify the test cases
    StopwatchTime const TestTimedCounter::StartStopwatchTime(1000200000000LL);


    BOOST_FIXTURE_TEST_SUITE(TestTimedCounterSuite,TestTimedCounter)

    BOOST_AUTO_TEST_CASE(AFewWindowsTest)
    {
        TimedCounter counter(TimeSpan::FromMinutes(1), 10);

        Record(counter, 10, L"10:25:00");
        VERIFY_ARE_EQUAL(10u, GetCount(counter, L"10:25:00"));
        VERIFY_ARE_EQUAL(10u, GetCount(counter, L"10:25:10"));
        VERIFY_ARE_EQUAL(0u, GetCount(counter, L"10:26:00"));

        int64 expectedCount = 0;
        for (size_t i = 0; i < 60; i += 5)
        {
            size_t toAdd = 1 + i / 5;
            Record(counter, toAdd, wformatString("11:25:{0:02}", i));
            expectedCount += toAdd;
        }
        VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, L"11:25:59"));
        
        for (size_t i = 0; i < 60; i += 5)
        {
            if (i % 10 == 0)
            {
                expectedCount -= (1 + i / 5) * 2 + 1;
            }
            size_t toAdd = 101 + i / 5;
            Record(counter, toAdd, wformatString("11:26:{0:02}", i));
            expectedCount += toAdd;
        }
        VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, L"11:26:59"));
    }

    BOOST_AUTO_TEST_CASE(ManyWindowsTest)
    {
        TimedCounter counter(TimeSpan::FromMinutes(1), 120);

        int64 expectedCount = 0;
        for (size_t i = 0; i < 60; i += 5)
        {
            size_t toAdd = 1 + i / 5;
            Record(counter, toAdd, wformatString("11:25:{0:02}", i));
            expectedCount += toAdd;
        }
        VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, L"11:25:59"));
        
        for (size_t i = 0; i < 60; i += 5)
        {
            expectedCount -= 1 + i / 5;

            size_t toAdd = 101 + i / 5;
            Record(counter, toAdd, wformatString("11:26:{0:02}", i));
            expectedCount += toAdd;

            VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, wformatString("11:26:{0:02}", i + 1)));
        }
        
        for (int i = 0; i < 60; i += 5)
        {
            expectedCount -= 101 + i / 5;
            VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, wformatString("11:27:{0:02}", i + 4)));
        }
        VERIFY_ARE_EQUAL(0u, GetCount(counter, L"11:28:00"));
    }

    BOOST_AUTO_TEST_CASE(OneWindowTest)
    {
        TimedCounter counter(TimeSpan::FromMinutes(1), 1);

        int64 expectedCount = 0;

        for (size_t i = 0; i < 60; i += 5)
        {
            size_t toAdd = 1 + i / 5;
            Record(counter, toAdd, wformatString("11:25:{0:02}", i));
            expectedCount += toAdd;
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCount), GetCount(counter, L"11:25:59"));
        VERIFY_ARE_EQUAL(0u, GetCount(counter, L"11:26:00"));
    }

    BOOST_AUTO_TEST_CASE(MergeTest)
    {
        TimedCounter counterA(TimeSpan::FromMinutes(1), 10);
        
        TimedCounter counterB(TimeSpan::FromMinutes(1), 10);
        Record(counterA, 10, L"10:25:00");
        Record(counterB, 20, L"10:25:30");

        counterA.Merge(counterB, GetTime(L"10:25:31"));
        VERIFY_ARE_EQUAL(30u, GetCount(counterA, L"10:25:31"));

        TimedCounter counterC(TimeSpan::FromMinutes(1), 10);
        Record(counterC, 30, L"10:25:40");
        Record(counterC, 40, L"10:25:45");

        counterA.Merge(counterC, GetTime(L"10:26:01"));
        VERIFY_ARE_EQUAL(90u, GetCount(counterA, L"10:26:01"));
    }

    BOOST_AUTO_TEST_CASE(SmallStartTimeTest)
    {
        TimedCounter counter(TimeSpan::FromMinutes(60), 10);
        StopwatchTime start = StopwatchTime::Zero;
        counter.Record(11, start);

        counter.Refresh(start);
        VERIFY_ARE_EQUAL(11u, counter.GetCount());
        counter.Refresh(start + TimeSpan::FromMinutes(30));
        VERIFY_ARE_EQUAL(11u, counter.GetCount());
        counter.Refresh(start + TimeSpan::FromMinutes(59));
        VERIFY_ARE_EQUAL(11u, counter.GetCount());
        counter.Refresh(start + TimeSpan::FromMinutes(61));
        VERIFY_ARE_EQUAL(0u, counter.GetCount());
    }

    BOOST_AUTO_TEST_CASE(IntervalCounterBasicTest)
    {
        IntervalCounter counter(TimeSpan::FromSeconds(10));

        counter.Record(3000u, GetTime(L"10:10:10"));
        counter.Record(1u, GetTime(L"10:10:19"));

        VERIFY_ARE_EQUAL(3001u, counter.GetCount());
        VERIFY_ARE_EQUAL(2u, counter.GetCountsSize());

        counter.Record(5u, GetTime(L"10:10:21"));

        VERIFY_ARE_EQUAL(6u, counter.GetCount());
        VERIFY_ARE_EQUAL(2u, counter.GetCountsSize());

        counter.Refresh(GetTime(L"10:10:30"));

        VERIFY_ARE_EQUAL(1u, counter.GetCountsSize());
        VERIFY_ARE_EQUAL(5u, counter.GetCount());

        counter.Refresh(GetTime(L"11:10:31"));

        VERIFY_ARE_EQUAL(0u, counter.GetCountsSize());
        VERIFY_ARE_EQUAL(0u, counter.GetCount());
    }

    BOOST_AUTO_TEST_CASE(IntervalCounterSameRecordTimeTest)
    {
        IntervalCounter counter(TimeSpan::FromSeconds(1));

        for (size_t i = 1u; i <= 100u; i++)
        {
            counter.Record(i, GetTime(L"11:11:11"));
        }

        VERIFY_ARE_EQUAL(1u, counter.GetCountsSize());
        VERIFY_ARE_EQUAL(5050u, counter.GetCount());

        counter.Refresh(GetTime(L"11:11:12"));
        VERIFY_ARE_EQUAL(0u, counter.GetCount());
    }

    BOOST_AUTO_TEST_CASE(IntervalCounterMoveAndCopyTest)
    {
        IntervalCounter counter(TimeSpan::FromMinutes(1));

        counter.Record(1u, GetTime(L"10:10:10"));
        counter.Record(2u, GetTime(L"10:10:11"));
        counter.Record(3u, GetTime(L"10:10:12"));
        counter.Record(4u, GetTime(L"10:10:13"));
        counter.Record(5u, GetTime(L"10:10:14"));

        IntervalCounter copiedCounter(counter);
        IntervalCounter movedCounter(move(counter));

        VERIFY_ARE_EQUAL(5u, copiedCounter.GetCountsSize());
        VERIFY_ARE_EQUAL(15u, copiedCounter.GetCount());

        VERIFY_ARE_EQUAL(5u, movedCounter.GetCountsSize());
        VERIFY_ARE_EQUAL(15u, movedCounter.GetCount());

        counter.Refresh(GetTime(L"10:11:15"));
        VERIFY_ARE_EQUAL(0u, counter.GetCountsSize());

        VERIFY_ARE_EQUAL(5u, copiedCounter.GetCountsSize());
        VERIFY_ARE_EQUAL(15u, copiedCounter.GetCount());

        VERIFY_ARE_EQUAL(5u, movedCounter.GetCountsSize());
        VERIFY_ARE_EQUAL(15u, movedCounter.GetCount());
    }

    BOOST_AUTO_TEST_CASE(IntervalCounterExtensiveTest)
    {
        IntervalCounter counter(TimeSpan::FromMinutes(1));

        counter.Refresh(StopwatchTime::Zero);

        counter.Record(9999u, StopwatchTime::Zero);

        for (size_t i = 1u; i <= 100u; i++)
        {
            counter.Record(i, StopwatchTime::Zero + TimeSpan::FromSeconds(static_cast<double>(i)));
        }

        VERIFY_ARE_EQUAL(60u, counter.GetCountsSize());
        VERIFY_ARE_EQUAL(4230u, counter.GetCount());

        for (int i = 0; i < 100; i++)
        {
            counter.Record(0u, StopwatchTime::Zero + TimeSpan::FromSeconds(100));
        }

        VERIFY_ARE_EQUAL(60u, counter.GetCountsSize());
        VERIFY_ARE_EQUAL(4230u, counter.GetCount());

        counter.Refresh(StopwatchTime::Zero + TimeSpan::FromSeconds(101));

        VERIFY_ARE_EQUAL(59u, counter.GetCountsSize());

        VERIFY_ARE_EQUAL(4189u, counter.GetCount());

        counter.Reset();

        VERIFY_ARE_EQUAL(0u, counter.GetCountsSize());

        counter.Record(5u, StopwatchTime::Zero + TimeSpan::FromMinutes(3.141592));
        counter.Record(10u, StopwatchTime::Zero + TimeSpan::FromMinutes(4.141591));

        VERIFY_ARE_EQUAL(15u, counter.GetCount());

        counter.Refresh(StopwatchTime::Zero + TimeSpan::FromMinutes(4.15));

        VERIFY_ARE_EQUAL(10u, counter.GetCount());

        counter.Refresh(StopwatchTime::Zero + TimeSpan::FromMinutes(5.16));

        VERIFY_ARE_EQUAL(0u, counter.GetCount());

        counter.Record(1u, GetTime(L"10:59:59"));
        counter.Record(2u, GetTime(L"11:00:01"));

        VERIFY_ARE_EQUAL(3u, counter.GetCount());

        counter.Refresh(StopwatchTime::MaxValue);

        VERIFY_ARE_EQUAL(0u, counter.GetCountsSize());
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestTimedCounter::TestSetup()
    {
        hasStartDateTime_ = false;
        return true;
    }

}
