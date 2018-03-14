// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestRAStopwatch
{
protected:
    TestRAStopwatch()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestRAStopwatch()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void Start()
    {
        stopwatch_.Start(clock_);
    }

    void Stop()
    {
        stopwatch_.Stop(clock_);
    }

    void ValidateIsRunning()
    {
        Verify::IsTrue(stopwatch_.IsRunning, L"IsRunning");
    }

    void ValidateIsNotRunning()
    {
        Verify::IsTrue(!stopwatch_.IsRunning, L"IsRunning");
    }

    void ValidateElapsed(int64 expectedSeconds)
    {
        Verify::AreEqual(expectedSeconds, stopwatch_.Elapsed.TotalSeconds(), L"Elapsed");
    }

    void AdvanceTime(int elapsed)
    {
        clock_.AdvanceTimeBySeconds(elapsed);
    }

private:
    TestClock clock_;
    RAStopwatch stopwatch_;
};

bool TestRAStopwatch::TestSetup()
{
    clock_.SetManualMode();
    return true;
}

bool TestRAStopwatch::TestCleanup()
{
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestRAStopwatchSuite, TestRAStopwatch)

BOOST_AUTO_TEST_CASE(StopwatchIsInitiallyStopped)
{
    ValidateIsNotRunning();
}

BOOST_AUTO_TEST_CASE(ElapsedIsInitiallyZero)
{
    ValidateElapsed(0);
}

BOOST_AUTO_TEST_CASE(StopwatchIsRunningAfterFirstStart)
{
    Start();

    ValidateIsRunning();
}

BOOST_AUTO_TEST_CASE(ElapsedIsZeroAfterFirstStart)
{
    Start();
    
    ValidateElapsed(0);
}

BOOST_AUTO_TEST_CASE(StopwatchIsStoppedAfterFirstMeasurement)
{
    Start();

    AdvanceTime(1);

    Stop();

    ValidateIsNotRunning();
}

BOOST_AUTO_TEST_CASE(StopwatchReturnsElapsedAfterFirstStop)
{
    Start();

    AdvanceTime(1);

    Stop();

    ValidateElapsed(1);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
