// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    static StringLiteral const PublicTestSource("TESTDecayAverage");

    class TestDecayAverage
    {
    protected:
        TestDecayAverage() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)
    };

    BOOST_FIXTURE_TEST_SUITE(TestDecayAverageSuite, TestDecayAverage)

        BOOST_AUTO_TEST_CASE(NoDecay_Basic)
    {
        DecayAverage avg(0.0, TimeSpan::FromTicks(1));
        avg.Update(TimeSpan::FromSeconds(10.0));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromSeconds(10));
        avg.Update(TimeSpan::FromSeconds(20.0));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromSeconds(20));
    }

    BOOST_AUTO_TEST_CASE(NoDecay_Overflow)
    {
        DecayAverage avg(0.0, TimeSpan::FromTicks(1));
        avg.Update(TimeSpan::FromMinutes(numeric_limits<double>::max()));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromMilliseconds(std::numeric_limits<uint>::max()));
        avg.Update(TimeSpan::FromMinutes(numeric_limits<double>::max()));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromMilliseconds(std::numeric_limits<uint>::max()));
    }

    BOOST_AUTO_TEST_CASE(SimpleAverage)
    {
        DecayAverage avg(1.0, TimeSpan::FromTicks(9999));
        avg.Update(TimeSpan::FromSeconds(10));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromSeconds(10));
        avg.Update(TimeSpan::FromSeconds(1));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromSeconds(5.5));
        avg.Update(TimeSpan::FromSeconds(100));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromSeconds(37));
    }

    BOOST_AUTO_TEST_CASE(SimpleAverageOverflow)
    {
        DecayAverage avg(1.0, TimeSpan::FromTicks(1));
        Sleep(1000);
        avg.Update(TimeSpan::FromMinutes(numeric_limits<double>::max()));
        VERIFY_ARE_EQUAL2(avg.Value, TimeSpan::FromMilliseconds(std::numeric_limits<uint>::max()));
    }

    BOOST_AUTO_TEST_CASE(SimpleStandardDeviation)
    {
        StandardDeviation sd;
        sd.Add(TimeSpan::FromSeconds(60));
        sd.Add(TimeSpan::FromSeconds(60));
        sd.Add(TimeSpan::FromSeconds(60));

        VERIFY_ARE_EQUAL2(sd.Average, TimeSpan::FromSeconds(60));
        VERIFY_ARE_EQUAL2(sd.StdDev, TimeSpan::FromSeconds(0));

        sd.Clear();
        VERIFY_ARE_EQUAL2(sd.Average, TimeSpan::Zero);
        VERIFY_ARE_EQUAL2(sd.StdDev, TimeSpan::FromSeconds(0));


        sd.Add(TimeSpan::FromSeconds(4));
        sd.Add(TimeSpan::FromSeconds(4));
        sd.Add(TimeSpan::FromSeconds(9));
        sd.Add(TimeSpan::FromSeconds(9));
        VERIFY_ARE_EQUAL2(sd.Average, TimeSpan::FromSeconds(6.5));
        VERIFY_ARE_EQUAL2(sd.StdDev, TimeSpan::FromSeconds(2.5));
    }

    BOOST_AUTO_TEST_CASE(OverflowStandardDeviation)
    {
        StandardDeviation sd;
        // NOTE - Currently the max TimeSpan value does not exceed max value accepted by the Add() method and hence this test case is moot
        // Keeping in the test in case that ever changes and we fail this variation that time
        sd.Add(TimeSpan::MaxValue);
        sd.Add(TimeSpan::MaxValue);
        sd.Add(TimeSpan::Zero);
        sd.Add(TimeSpan::Zero);
        VERIFY_ARE_EQUAL2(sd.Average.TotalMilliseconds(), (TimeSpan::MaxValue/2).TotalMilliseconds());
        VERIFY_ARE_EQUAL2(sd.StdDev > TimeSpan::Zero, true);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestDecayAverage::Setup()
    {
        return true;
    }
}
