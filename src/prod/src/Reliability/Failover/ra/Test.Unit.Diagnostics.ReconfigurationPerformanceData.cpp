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

class TestReconfigurationPerformanceData
{
protected:
    TestReconfigurationPerformanceData()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestReconfigurationPerformanceData()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void Verify(int phase0, int phase1, int phase2, int phase3, int phase4, int total)
    {
        Verify::AreEqual(phase0, perfData_.Phase0Duration.TotalSeconds(), L"Phase0");
        Verify::AreEqual(phase1, perfData_.Phase1Duration.TotalSeconds(), L"Phase1");
        Verify::AreEqual(phase2, perfData_.Phase2Duration.TotalSeconds(), L"Phase2");
        Verify::AreEqual(phase3, perfData_.Phase3Duration.TotalSeconds(), L"Phase3");
        Verify::AreEqual(phase4, perfData_.Phase4Duration.TotalSeconds(), L"Phase4");
        Verify::AreEqual(total, perfData_.TotalDuration.TotalSeconds(), L"Total");
    }

    void Start()
    {
        Start(0);
    }

    void Start(int64 phase0DurationInSeconds)
    {
        perfData_.OnStarted(TimeSpan::FromSeconds(static_cast<double>(phase0DurationInSeconds)), clock_);
    }

    void Advance(int seconds)
    {
        clock_.AdvanceTimeBySeconds(seconds);
    }

    UnitTestContextUPtr utContext_;
    Diagnostics::ReconfigurationPerformanceData perfData_;
    TestClock clock_;
};

bool TestReconfigurationPerformanceData::TestSetup()
{
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled);
    clock_.SetManualMode();
    return true;
}

bool TestReconfigurationPerformanceData::TestCleanup()
{
    utContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestReconfigurationPerformanceDataSuite, TestReconfigurationPerformanceData)

BOOST_AUTO_TEST_CASE(Other_Deactivate_Activate)
{
    Start();

    Advance(1);

    perfData_.OnPhase2Finished(clock_);

    Advance(2);

    perfData_.OnPhase3Finished(clock_);

    Advance(3);

    perfData_.OnFinishedAtPhase4(clock_);

    Verify(0, 0, 1, 2, 3, 6);
}

BOOST_AUTO_TEST_CASE(Other_NoDeactivate_Activate)
{
    Start();

    Advance(1);

    perfData_.OnPhase2Finished(clock_);

    Advance(2);

    perfData_.OnFinishedAtPhase4(clock_);

    Verify(0, 0, 1, 0, 2, 3);
}

BOOST_AUTO_TEST_CASE(Swap_Continue)
{
    Start(1);

    Advance(2);

    perfData_.OnPhase1Finished(clock_);

    Advance(3);

    perfData_.OnPhase2Finished(clock_);

    Advance(4);

    perfData_.OnPhase3Finished(clock_);

    Advance(5);

    perfData_.OnFinishedAtPhase4(clock_);

    Verify(1, 2, 3, 4, 5, 15);
}

BOOST_AUTO_TEST_CASE(Swap_Abort)
{
    Start();

    Advance(1);

    perfData_.OnFinishedAtPhase0(clock_);

    Verify(1, 0, 0, 0, 0, 1);
}

BOOST_AUTO_TEST_CASE(Failover_Deactivate_Activate)
{
    Start();

    Advance(1);

    perfData_.OnPhase1Finished(clock_);

    Advance(2);

    perfData_.OnPhase2Finished(clock_);

    Advance(3);

    perfData_.OnPhase3Finished(clock_);

    Advance(4);

    perfData_.OnFinishedAtPhase4(clock_);

    Verify(0, 1, 2, 3, 4, 10);
}

BOOST_AUTO_TEST_CASE(Failover_NoDeactivate_Activate)
{
    Start();

    Advance(1);

    perfData_.OnPhase1Finished(clock_);

    Advance(2);

    perfData_.OnPhase2Finished(clock_);

    Advance(4);

    perfData_.OnFinishedAtPhase4(clock_);

    Verify(0, 1, 2, 0, 4, 7);
}

BOOST_AUTO_TEST_CASE(Failover_ChangeConfig)
{
    Start();

    Advance(1);

    perfData_.OnFinishedAtPhase1(clock_);

    Verify(0, 1, 0, 0, 0, 1);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
