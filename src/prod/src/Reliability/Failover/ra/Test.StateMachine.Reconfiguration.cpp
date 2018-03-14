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

class TestStateMachine_Reconfiguration2
{
protected:
    TestStateMachine_Reconfiguration2()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestStateMachine_Reconfiguration2()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

protected:
    ScenarioTest& GetScenarioTest()
    {
        return scenarioTestHolder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr scenarioTestHolder_;
};

bool TestStateMachine_Reconfiguration2::TestSetup()
{
    scenarioTestHolder_ = ScenarioTestHolder::Create(UnitTestContext::Option::StubJobQueueManager | UnitTestContext::Option::TestAssertEnabled);
    return true;
}

bool TestStateMachine_Reconfiguration2::TestCleanup()
{
    scenarioTestHolder_.reset();
    return true;
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_Reconfiguration2Suite, TestStateMachine_Reconfiguration2)

BOOST_AUTO_TEST_CASE(SToIInBuildReconfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/I RD D 3:1]");
    
    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/I SB U 3:1:3:2]");
    test.DumpFT(L"SP1");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/I IB U 3:1:3:2]");
    test.DumpFT(L"SP1");

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/I IB U 3:1:3:2] Success C");
    test.DumpFT(L"SP1");

    test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, L"SP1", L"411/412 [S/S RD U 2:1] Success");
    test.DumpFT(L"SP1");

    test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"411/412 [S/S RD U 2:1] Success");
    test.DumpFT(L"SP1");

    test.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplicaReply>(2, L"SP1", L"411/412 [S/I IB U 3:1:3:2] Success");
    test.DumpFT(L"SP1");

    test.ProcessRAPMessageAndDrain<MessageType::ReplicatorBuildIdleReplicaReply>(L"SP1", L"411/412 [P/P RD U 1:1] [S/I IB U 3:1:3:2] Success -");
    test.DumpFT(L"SP1");

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/I IB U 3:1:3:2] Success ER");
    test.DumpFT(L"SP1");

    test.ValidateFT(L"SP1", L"O None 000/000/412 1:1 CMN [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/I RD U N F 3:1:3:2]");
}

BOOST_AUTO_TEST_CASE(SToIIDownReconfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/I RD D 3:1]");

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/I RD D 3:1] Success C");
    test.DumpFT(L"SP1");

    test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, L"SP1", L"411/412 [S/S RD U 2:1] Success");
    test.DumpFT(L"SP1");

    test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"411/412 [S/S RD U 2:1] Success");
    test.DumpFT(L"SP1");

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/I RD D 3:1] Success ER");
    test.DumpFT(L"SP1");

    test.ValidateFT(L"SP1", L"O None 000/000/412 1:1 CMN [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(IToPWithChangeConfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(L"SP1", L"O Phase1_GetLSN 411/000/422 1:1 CM [I/N/P RD U N F 1:1 1 1] [P/N/S SB U N F 2:1] [S/N/S RD U N F 3:1 2 2]");
    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [I/P RD U 1:1] [P/S DD D 2:1] [S/S RD U 3:1]");

    test.ValidateFMMessage<MessageType::ChangeConfiguration>();

    test.ValidateFT(L"SP1", L"O None 411/000/422 1:1 CM [I/N/I RD U N F 1:1] [P/N/P SB D N F 2:1] [S/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(PrimaryInBuildReconfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CMK [N/N/P SB U N F 1:1]");
    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/P IB U 1:1]");

    test.ResetAll();

    test.ProcessRAPMessageAndDrain<MessageType::ReplicatorGetStatusReply>(L"SP1", L"411/422 [P/P RD U 1:1 1 1] Success -");

    auto & ft = test.GetFT(L"SP1");
    test.DumpFT(L"SP1");
    Verify::AreEqual(FMMessageStage::None, ft.Test_GetFMMessageState().MessageStage, L"FMMessageState");
    Verify::AreEqual(ReconfigurationAgentComponent::ReplicaStates::InBuild, ft.LocalReplica.State, L"Local Replica state");
}

BOOST_AUTO_TEST_CASE(ActivateMustUpdateDeactivationInfoIfMessageToRAPIsPendingAndFailoverHasHappened)
{
    auto & test = GetScenarioTest();

    test.AddFT(L"SP1", L"O None 411/000/412 {411:0} 1:1 CM [I/N/S RD U RAP F 1:1] [P/N/P RD U N F 2:1] [S/N/S RD U N F 3:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(3, L"SP1", L"411/423 [I/S RD U 1:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(3, L"SP1", L"411/423 {422:0} [I/S RD U 1:1] [P/S DD D 2:1] [S/P RD U 3:1]");

    auto & ft = test.GetFT(L"SP1");

    test.DumpFT(L"SP1");

    Verify::AreEqual(StateManagement::Reader::ReadHelper<Epoch>(L"422"), ft.DeactivationInfo.DeactivationEpoch, L"Deactivation epoch");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
