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

class ReconfigurationStateTest
{
protected:
    ReconfigurationStateTest()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~ReconfigurationStateTest()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void StartAndFinishPhase0()
    {
        StartDemote();

        clock_.AdvanceTimeBySeconds(3);

        reconfigState_->FinishDemote(clock_);
    }

    void VerifyIsNotReconfiguring(int64 expectedPhase0Duration)
    {
        VerifyPhase(FailoverUnitReconfigurationStage::None);
        Verify::IsTrue(!reconfigState_->IsReconfiguring, L"IsReconfiguring");
        Verify::AreEqual(StopwatchTime::Zero, reconfigState_->StartTime, L"Start Time");
        VerifyPhase0Duration(expectedPhase0Duration);
    }

    void VerifyPhase0Duration(int64 expectedPhase0Duration)
    {
        Verify::AreEqual(reconfigState_->Phase0Duration.TotalSeconds(), expectedPhase0Duration, L"ExpectedPhase0");
    }

    void VerifyPhase(FailoverUnitReconfigurationStage::Enum expected)
    {
        Verify::AreEqual(reconfigState_->ReconfigurationStage, expected, L"Stage");
    }

    void StartFailover()
    {
        reconfigState_->Start(ReconfigurationType::Failover, TimeSpan::Zero, clock_);
    }

    void StartDemote()
    {
        reconfigState_->Start(ReconfigurationType::SwapPrimary, TimeSpan::MaxValue, clock_);
    }

    void StartOther()
    {
        reconfigState_->Start(ReconfigurationType::Other, TimeSpan::Zero, clock_);
    }

    void StartContinueSwap(int64 expectedPhase0)
    {
        reconfigState_->Start(true, false, true, TimeSpan::FromSeconds(static_cast<double>(expectedPhase0)), clock_);
    }

    void StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::Enum type)
    {
        auto startTime = clock_.Now();
        reconfigState_->Start(type, TimeSpan::Zero, clock_);

        Verify::AreEqual(startTime, reconfigState_->StartTime, L"StartTime");
    }

    void StartPhase2Catchup()
    {
        reconfigState_->StartPhase2Catchup(clock_);
    }

    void StartPhase3Deactivate()
    {
        reconfigState_->StartPhase3Deactivate(clock_);
    }

    void StartPhase4Activate()
    {
        reconfigState_->StartPhase4Activate(clock_);
    }

    void Finish()
    {
        reconfigState_->Finish(Federation::NodeInstance(), clock_, queue_);
    }

    void FinishDemote()
    {
        reconfigState_->FinishDemote(clock_);
    }

    void VerifyResult(ReconfigurationResult::Enum result)
    {
        Verify::AreEqual(result, reconfigState_->Result, L"Result");
    }

    unique_ptr<ReconfigurationState> reconfigState_;
    TestClock clock_;
    UnitTestContextUPtr utContext_;
    FailoverUnitDescription ftDesc_;
    ServiceDescription sdDesc_;
    StateMachineActionQueue queue_;
};

bool ReconfigurationStateTest::TestSetup()
{
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled);
    reconfigState_ = make_unique<ReconfigurationState>(&ftDesc_, &sdDesc_);
    clock_.SetManualMode();
    return true;
}

bool ReconfigurationStateTest::TestCleanup()
{
    utContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(ReconfigurationStateTestSuite, ReconfigurationStateTest)

BOOST_AUTO_TEST_CASE(InitiallyObjectIsNone)
{
    VerifyIsNotReconfiguring(0);
}

BOOST_AUTO_TEST_CASE(DuringReconfigurationStartTimeIsSet_Failover)
{
    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::Failover);
}

BOOST_AUTO_TEST_CASE(DuringReconfigurationStartTimeIsSet_Swap)
{
    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::SwapPrimary);
}

BOOST_AUTO_TEST_CASE(DuringReconfigurationStartTimeIsSet_Other)
{
    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::Other);
}

BOOST_AUTO_TEST_CASE(DuringReconfigurationStartTimeIsSet_ContinueDemote)
{
    StartContinueSwap(10);

    VerifyPhase0Duration(10);
}

BOOST_AUTO_TEST_CASE(Start_ClearsPhase0Time_Promote)
{
    StartAndFinishPhase0();

    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::Failover);
}

BOOST_AUTO_TEST_CASE(Start_ClearsPhase0Time_Swap)
{
    StartAndFinishPhase0();

    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::SwapPrimary);
}

BOOST_AUTO_TEST_CASE(Start_ClearsPhase0Time_Other)
{
    StartAndFinishPhase0();

    StartReconfigurationAndVerifyTimeTestHelper(ReconfigurationType::Other);
}

BOOST_AUTO_TEST_CASE(Start_ClearsResult)
{
    StartAndFinishPhase0();

    StartFailover();

    VerifyResult(ReconfigurationResult::Invalid);
}

BOOST_AUTO_TEST_CASE(ResetClearsState)
{
    StartFailover();

    reconfigState_->Reset();

    VerifyIsNotReconfiguring(0);
    
    VerifyResult(ReconfigurationResult::Invalid);
}

BOOST_AUTO_TEST_CASE(Flow_WithoutPhase3)
{
    StartFailover();

    StartPhase2Catchup();

    StartPhase4Activate();

    Finish();

    VerifyIsNotReconfiguring(0);

    VerifyResult(ReconfigurationResult::Completed);
}

BOOST_AUTO_TEST_CASE(Finish_PromoteToPrimary_ClearsTime)
{
    StartFailover();

    StartPhase2Catchup();

    StartPhase3Deactivate();

    StartPhase4Activate();

    Finish();

    VerifyIsNotReconfiguring(0);

    VerifyResult(ReconfigurationResult::Completed);
}

BOOST_AUTO_TEST_CASE(Finish_ChangeConfiguration_ClearsTime)
{
    StartFailover();

    reconfigState_->FinishWithChangeConfiguration(Federation::NodeInstance(), clock_, queue_);

    VerifyIsNotReconfiguring(0);

    VerifyResult(ReconfigurationResult::ChangeConfiguration);
}

BOOST_AUTO_TEST_CASE(Finish_Demote)
{
    StartDemote();

    auto phase0Duration = 3;
    clock_.AdvanceTimeBySeconds(phase0Duration);

    reconfigState_->FinishDemote(clock_);

    VerifyIsNotReconfiguring(phase0Duration);

    VerifyResult(ReconfigurationResult::DemoteCompleted);
}

BOOST_AUTO_TEST_CASE(Finish_AbortSwap_ClearsTime)
{
    StartDemote();

    reconfigState_->StartAbortPhase0Demote(clock_);

    reconfigState_->FinishAbortSwapPrimary(Federation::NodeInstance(), clock_, queue_);

    VerifyIsNotReconfiguring(0);

    VerifyResult(ReconfigurationResult::AbortSwapPrimary);
}

BOOST_AUTO_TEST_CASE(GetElapsedTimeTest)
{
    StartFailover();

    auto expected = TimeSpan::FromSeconds(10);
    clock_.AdvanceTime(expected);

    auto actual = reconfigState_->GetCurrentReconfigurationDuration(clock_);

    Verify::AreEqual(expected, actual, L"ElapsedTime");
}

BOOST_AUTO_TEST_CASE(ReconfigurationTimeElapsedTest)
{
    StartFailover();

    StartPhase2Catchup();
    
    auto expected = TimeSpan::FromSeconds(10);
    clock_.AdvanceTime(expected);

    auto actual = reconfigState_->GetCurrentReconfigurationPhaseTimeElapsed(clock_);

    Verify::AreEqual(expected, actual, L"ReconfigurationElapsedTime");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
