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
using namespace Node;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestNodeDeactivationStateProcessor
{
protected:
    TestNodeDeactivationStateProcessor()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestNodeDeactivationStateProcessor()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void AddOpenFT()
    {
        GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    }

    void AddDownFT()
    {
        GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }

    void VerifyDown()
    {
        Verify::IsTrueLogOnError(!GetFT().LocalReplica.IsUp, L"FT is down");
    }

    void VerifyNotClosing()
    {
        Verify::IsTrueLogOnError(!GetFT().LocalReplicaClosePending.IsSet, L"FT is not closing");
    }

    void VerifyOpening()
    {
        Verify::AreEqualLogOnError(RAReplicaOpenMode::Reopen, GetFT().OpenMode, L"FT is opening");
    }

    void VerifyClosing()
    {
        Verify::AreEqualLogOnError(ReplicaCloseMode::DeactivateNode, GetFT().CloseMode, L"FT is closing");
    }

    FailoverUnit & GetFT()
    {
        return GetScenarioTest().GetFT(L"SP1");
    }

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    void PerformStateChange(NodeDeactivationInfo const & newInfo)
    {
        processor_->ProcessActivationStateChange(L"a", *state_, newInfo);
    }

    void PerformStateChangeAndDrain(NodeDeactivationInfo const & newInfo)
    {
        PerformStateChange(newInfo);

        GetScenarioTest().DrainJobQueues();
    }

protected:
    NodeDeactivationStateUPtr state_;
    NodeDeactivationStateProcessorUPtr processor_;
    ScenarioTestHolderUPtr holder_;
};

bool TestNodeDeactivationStateProcessor::TestSetup()
{
    holder_ = ScenarioTestHolder::Create(UnitTestContext::Option::StubJobQueueManager | UnitTestContext::Option::TestAssertEnabled);

    state_ = make_unique<NodeDeactivationState>(
        *FailoverManagerId::Fm,
        GetScenarioTest().RA);

    processor_ = make_unique<NodeDeactivationStateProcessor>(GetScenarioTest().RA);
    
    return true;
}

bool TestNodeDeactivationStateProcessor::TestCleanup()
{
    state_->Close();
    return true;
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestNodeDeactivationStateProcessorSuite, TestNodeDeactivationStateProcessor)

BOOST_AUTO_TEST_CASE(Deactivate_Closes_FTs)
{
    AddOpenFT();

    PerformStateChangeAndDrain(NodeDeactivationInfo(false, 4));

    VerifyClosing();
}

BOOST_AUTO_TEST_CASE(Activate_Opens_FTs)
{
    AddDownFT();

    PerformStateChangeAndDrain(NodeDeactivationInfo(true, 4));

    VerifyOpening();
}

BOOST_AUTO_TEST_CASE(Stale_Activate_Is_Rejected)
{
    AddDownFT();

    PerformStateChange(NodeDeactivationInfo(false, 5));

    PerformStateChangeAndDrain(NodeDeactivationInfo(true, 4));

    VerifyDown();
}

BOOST_AUTO_TEST_CASE(Stale_Deactivate_Is_Rejected)
{
    AddDownFT();

    PerformStateChange(NodeDeactivationInfo(true, 5));

    PerformStateChangeAndDrain(NodeDeactivationInfo(false, 4));

    VerifyOpening();
}

BOOST_AUTO_TEST_CASE(Stale_EnqueuedActivate_Is_Rejected)
{
    AddDownFT();

    PerformStateChange(NodeDeactivationInfo(true, 4));

    PerformStateChangeAndDrain(NodeDeactivationInfo(false, 5));

    VerifyDown();
}

BOOST_AUTO_TEST_CASE(Stale_EnqueuedDeactivate_Is_Rejected)
{
    AddOpenFT();

    PerformStateChange(NodeDeactivationInfo(false, 4));

    PerformStateChangeAndDrain(NodeDeactivationInfo(true, 5));

    VerifyNotClosing();
}

BOOST_AUTO_TEST_CASE(End_Calls_Finish)
{
    PerformStateChangeAndDrain(NodeDeactivationInfo(false, 5));

    Verify::IsTrueLogOnError(state_->Test_GetAsyncOp() != nullptr, L"deactivate op should start");
}

BOOST_AUTO_TEST_CASE(End_Calls_Finish_Staleness)
{
    PerformStateChange(NodeDeactivationInfo(false, 5));
    
    PerformStateChangeAndDrain(NodeDeactivationInfo(true, 6));

    Verify::IsTrueLogOnError(state_->Test_GetAsyncOp() == nullptr, L"deactivate op should be null");
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
