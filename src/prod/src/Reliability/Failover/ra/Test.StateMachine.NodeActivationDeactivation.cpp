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

class TestStateMachine_NodeActivateDeactivate
{
protected:
    TestStateMachine_NodeActivateDeactivate() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_NodeActivateDeactivate() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);
public:
    template<MessageType::Enum T, MessageType::Enum TReply>
    void StalenessCheckHelper(wstring const & message, bool isFmm)
    {
        scenarioTest_->ActivateNode(4);
        SetupFTsForActivationNoOpScenario();

        scenarioTest_->ProcessFMMessageAndDrain<T>(message);

        scenarioTest_->ValidateNodeIsActivated(4);
        ValidateFTsForActivationNoOpScenario();

        // The caller passes in 3 as the activation sequence number
        if (isFmm)
        {
            scenarioTest_->ValidateFmmMessage<TReply>(L"3");
        }
        else
        {
            scenarioTest_->ValidateFMMessage<TReply>(L"3");
        }
    }

protected:
    void DeactivateTerminatesHostTestHelper(bool isFmm);

    void ActivateCancelsPreviousRunningDeactivateTestHelper(bool isFmm);

    void NodeUpAckActivatesBothTestHelper(bool isFmm);

    void SetTerminateDurationToZero()
    {
        scenarioTest_->UTContext.Config.NodeDeactivationMaxReplicaCloseDuration = TimeSpan::FromSeconds(0);
    }

    void AddOpenFT(wstring const & shortName)
    {
        scenarioTest_->AddFT(shortName, L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
    }

    void AddDownFT(wstring const & shortName)
    {
        scenarioTest_->AddFT(shortName, L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }

    void SetupFTsForActivationNoOpScenario()
    {
        scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
        scenarioTest_->AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
        scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    }

    void ValidateFTsForActivationNoOpScenario()
    {
        scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
        scenarioTest_->ValidateFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
        scenarioTest_->ValidateFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    }

    void ValidateActivateDeactivateMessageWasNoOp()
    {
        ValidateFTsForActivationNoOpScenario();
        scenarioTest_->ValidateNoMessages();
    }

    void SetupForNodeUpAckTest(bool addFT)
    {
        scenarioTest_->SetNodeUpAckProcessed(false, false);
        scenarioTest_->SetNodeActivationState(false, 1);

        if (addFT)
        {
            AddDownFT(L"SP1");
            AddDownFT(L"FM");
        }
    }

    FailoverUnit & GetFM()
    {
        return scenarioTest_->GetFT(L"FM");
    }

    FailoverUnit & GetOtherFT()
    {
        return scenarioTest_->GetFT(L"SP1");
    }

    void VerifyIsUp(bool expected, FailoverUnit& ft)
    {
        Verify::AreEqual(expected, ft.LocalReplica.IsUp, wformatString("IsUp"));
    }

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_NodeActivateDeactivate::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();
    return true;
}

bool TestStateMachine_NodeActivateDeactivate::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

void TestStateMachine_NodeActivateDeactivate::DeactivateTerminatesHostTestHelper(bool isFmm)
{
    SetTerminateDurationToZero();

    auto shortName = isFmm ? L"FM" : L"SP1";
    auto message = wformatString("3 {0}", isFmm);

    AddOpenFT(shortName);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(message);

    scenarioTest_->FireNodeDeactivationProgressCheckTimer(isFmm);

    scenarioTest_->ValidateTerminateCall(shortName);
}

void TestStateMachine_NodeActivateDeactivate::ActivateCancelsPreviousRunningDeactivateTestHelper(bool isFmm)
{
    auto shortName = isFmm ? L"FM" : L"SP1";
    auto deactivateRequest = wformatString("3 {0}", isFmm);
    auto activateRequest = wformatString("4 {0}", isFmm);

    AddOpenFT(shortName);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(deactivateRequest);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(activateRequest);

    scenarioTest_->ValidateNodeDeactivationProgressCheckNotRunning(isFmm);
}

void TestStateMachine_NodeActivateDeactivate::NodeUpAckActivatesBothTestHelper(bool isFmm)
{
    scenarioTest_->SetNodeActivationState(false, 1);
    scenarioTest_->SetNodeUpAckProcessed(false, false);
    scenarioTest_->ProcessNodeUpAck(L"true 2", isFmm);
    scenarioTest_->ValidateNodeActivationState(true, 2);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_NodeActivateDeactivateSuite, TestStateMachine_NodeActivateDeactivate)

BOOST_AUTO_TEST_CASE(StaleActivateRequestSendsReply_FM)
{
    StalenessCheckHelper<MessageType::NodeActivateRequest, MessageType::NodeActivateReply>(L"3 false", false);
}

BOOST_AUTO_TEST_CASE(StaleActivateRequestSendsReply_Fmm)
{
    StalenessCheckHelper<MessageType::NodeActivateRequest, MessageType::NodeActivateReply>(L"3 true", true);
}

BOOST_AUTO_TEST_CASE(ActivateIsIgnoredUntilNodeIsReady1)
{
    // Node is initially deactivated and node up ack from fmm has not arrived
    // send activate message from fmm and it should be ignored
    scenarioTest_->DeactivateNode(2);
    scenarioTest_->SetNodeUpAckFromFmmProcessed(false);
    SetupFTsForActivationNoOpScenario();

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 true");

    ValidateActivateDeactivateMessageWasNoOp();
    scenarioTest_->ValidateNodeIsDeactivated(2);
}

BOOST_AUTO_TEST_CASE(ActivateIsIgnoredUntilNodeIsReady2)
{
    scenarioTest_->DeactivateNode(2);
    scenarioTest_->SetNodeUpAckFromFMProcessed(false);
    SetupFTsForActivationNoOpScenario();

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");

    ValidateActivateDeactivateMessageWasNoOp();
    scenarioTest_->ValidateNodeIsDeactivated(2);
}

BOOST_AUTO_TEST_CASE(DeactivateIsIgnoredUntilNodeIsReady1)
{
    scenarioTest_->ActivateNode(2);
    scenarioTest_->SetNodeUpAckFromFmmProcessed(false);
    SetupFTsForActivationNoOpScenario();

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 true");

    ValidateActivateDeactivateMessageWasNoOp();
    scenarioTest_->ValidateNodeIsActivated(2);
}

BOOST_AUTO_TEST_CASE(DeactivateIsIgnoredUntilNodeIsReady2)
{
    scenarioTest_->ActivateNode(2);
    scenarioTest_->SetNodeUpAckFromFMProcessed(false);
    SetupFTsForActivationNoOpScenario();

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");

    ValidateActivateDeactivateMessageWasNoOp();
    scenarioTest_->ValidateNodeIsActivated(2);
}

BOOST_AUTO_TEST_CASE(ActivationDeactivationSanityTest)
{
    scenarioTest_->DeactivateNode(3);

    // activating node should succeed
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"4 false");
    scenarioTest_->ValidateNodeActivationState(false, true, 4);
    scenarioTest_->ValidateFMMessage<MessageType::NodeActivateReply>(L"4");

    scenarioTest_->ResetAll();

    // deactivating node should succeed
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"5 false");
    scenarioTest_->ValidateNodeActivationState(false, false, 5);
    scenarioTest_->ValidateFMMessage<MessageType::NodeDeactivateReply>(L"5");
}

BOOST_AUTO_TEST_CASE(ActivateReopensAfterClosingDueToDeactivate)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"2 false");

    scenarioTest_->ResetAll();

    // At this time ReplicaClose has been sent to replica
    // Let the node get activated now
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    // ReplicaCloseReply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    // FT should be reopened
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 SI [N/N/P SB U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ActivateDoesNotReopensAfterClosingDueToUpgrade)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 ACMHc [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessage<MessageType::NodeDeactivateRequest>(L"2 false");

    scenarioTest_->ResetAll();

    // At this time ReplicaClose has been sent to replica
    // Let the node get activated now
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    // ReplicaCloseReply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    // FT should be marked as down
    // Upgrade will reopen
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ActivateDoesNotReopensAfterClosingDueToRuntimeDown)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->DeactivateNode(2);

    scenarioTest_->ProcessRuntimeClosedAndDrain(L"SP1");

    // At this time ReplicaClose has been sent to replica
    // Let the node get activated now
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    // ReplicaCloseReply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    // FT should be marked as down
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ActivateReopensAfterClosingDueToAppHostDown)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->DeactivateNode(2);

    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP1");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    // FT should be reopened
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 SI [N/N/P SB U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ActivateDoesNotReopensAfterClosingDueToDrop)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->DeactivateNode(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP1", L"000/411 [N/P RD U 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    // FT should be closed
    scenarioTest_->ValidateFT(L"SP1", L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(ActivateReopensDeactivatedReplicas)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 1 - [N/N/P SB D N F 1:1]");

    scenarioTest_->DeactivateNode(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(DeactivateForReplicasThatAreBeingOpenedSanityTest)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 MS [N/N/P IC U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    // replica close should be marked
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 MHn [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(DeactivateForReplicasThatAreBeingReopenedSanityTest)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:2 1 SM [N/N/P SB U N F 1:2]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"000/411 [N/P SB U 1:2] Success -");

    // replica close should be marked
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 MHn [N/N/P SB U N F 1:2]");

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:2] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 2 I [N/N/P SB D N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ActivateActivatesFromBoth_FM)
{
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 true");

    // Activate from FM
    scenarioTest_->ResetAll();
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"4 false");
    scenarioTest_->ValidateNodeActivationState(true, 4);

    scenarioTest_->ValidateNoFmmMessages();
}

BOOST_AUTO_TEST_CASE(ActivateActivatesONlyFM_FMM)
{
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 true");

    // Activate from FM
    scenarioTest_->ResetAll();
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"4 true");
    scenarioTest_->ValidateNodeActivationState(true, true, 4);
    scenarioTest_->ValidateNodeActivationState(false, false, 3);

    scenarioTest_->ValidateNoFMMessages();
}

BOOST_AUTO_TEST_CASE(DeactivateDeactivatesOnlySender_FM)
{
    scenarioTest_->SetNodeActivationState(true, 2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");

    scenarioTest_->ValidateNodeActivationState(false, false, 3);
    scenarioTest_->ValidateNodeActivationState(true, true, 2);

    scenarioTest_->ValidateNoFmmMessages();
}

BOOST_AUTO_TEST_CASE(DeactivateDeactivatesOnlySender_Fmm)
{
    scenarioTest_->SetNodeActivationState(true, 2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 true");

    scenarioTest_->ValidateNodeActivationState(false, true, 2);
    scenarioTest_->ValidateNodeActivationState(true, false, 3);

    scenarioTest_->ValidateNoFMMessages();
}

BOOST_AUTO_TEST_CASE(DeactivateTerminatesHost)
{
    DeactivateTerminatesHostTestHelper(false);
}

BOOST_AUTO_TEST_CASE(CloseIsHandledWithDeactivateInProgress)
{
    AddOpenFT(L"SP1");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
}

BOOST_AUTO_TEST_CASE(AppHostDownForDropping)
{
    AddOpenFT(L"SP1");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");
}

BOOST_AUTO_TEST_CASE(ActivateCancelsPreviousRunningDeactivate_FM)
{
    ActivateCancelsPreviousRunningDeactivateTestHelper(false);
}

BOOST_AUTO_TEST_CASE(ActivateCancelsPreviousRunningDeactivate_FMM)
{
    ActivateCancelsPreviousRunningDeactivateTestHelper(true);
}

BOOST_AUTO_TEST_CASE(DeactivateWithHigherInstanceWhilePreviousDeactivateIsPending)
{
    AddOpenFT(L"SP1");

    scenarioTest_->ProcessFMMessage<MessageType::NodeDeactivateRequest>(L"3 false");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"4 false");

    Verify::IsTrue(scenarioTest_->RA.NodeStateObj.GetNodeDeactivationState(FailoverManagerId(false)).Test_GetAsyncOp() != nullptr, L"Deactivation check should be running");
}

BOOST_AUTO_TEST_CASE(ActivateWithHigherInstanceWhilePreviousActivateIsPending)
{
    AddDownFT(L"SP1");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"3 false");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessFMMessage<MessageType::NodeActivateRequest>(L"4 false");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"5 false");

    auto & ft = scenarioTest_->GetFT(L"SP1");
    Verify::IsTrue(ft.LocalReplica.IsUp, L"Replica not up");
}

BOOST_AUTO_TEST_CASE(NodeUpAck_DelayedIsStale)
{
    SetupForNodeUpAckTest(true);
    
    // Process FM activated with higher instance
    // That should not open 
    // THen process FMM deactivated with lower instance
    // That should actually open as FM has been applied
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 3", false);
    
    VerifyIsUp(false, GetFM());

    scenarioTest_->ProcessNodeUpAckAndDrain(L"false 1", true);

    VerifyIsUp(true, GetFM());
}

BOOST_AUTO_TEST_CASE(NodeUpAck_DelayedIsNotStale)
{
    SetupForNodeUpAckTest(true);

    // Process FM activated with higher instance
    // That should not open 
    // THen process FMM deactivated with lower instance
    // That should actually open as FM has been applied
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 3", false);

    VerifyIsUp(false, GetFM());

    scenarioTest_->ProcessNodeUpAckAndDrain(L"false 4", true);

    VerifyIsUp(false, GetFM());
}

BOOST_AUTO_TEST_CASE(NodeUpAckAlreadyProcessed_StillUpdatesIfActivate)
{
    SetupForNodeUpAckTest(true);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"false 2", true);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 3", false);

    VerifyIsUp(true, GetFM());
}

BOOST_AUTO_TEST_CASE(NodeUpAckAlreadyProcessed_DoesNotUpdateIfDeactivate)
{
    SetupForNodeUpAckTest(true);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2", true);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"false 3", false);

    VerifyIsUp(true, GetFM());
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithFmActivatedActivatesBoth)
{
    NodeUpAckActivatesBothTestHelper(false);
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithFmmActivatedActivatesBoth)
{
    scenarioTest_->SetNodeActivationState(false, 1);
    scenarioTest_->SetNodeUpAckProcessed(false, false);
    scenarioTest_->ProcessNodeUpAck(L"true 2", true);
    scenarioTest_->ValidateNodeActivationState(true, true, 2);
    scenarioTest_->ValidateNodeActivationState(false, false, 1);
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithFmDeactivatedDeactivatesOnlyOne)
{
    SetupForNodeUpAckTest(false);
    scenarioTest_->ProcessNodeUpAck(L"false 2", false);
    scenarioTest_->ValidateNodeActivationState(false, false, 2);
    scenarioTest_->ValidateNodeActivationState(true, false, 1);
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithFmmDeactivatedDeactivatesOnlyFmm)
{
    SetupForNodeUpAckTest(false);
    scenarioTest_->ProcessNodeUpAck(L"false 2", true);
    scenarioTest_->ValidateNodeActivationState(false, false, 1);
    scenarioTest_->ValidateNodeActivationState(true, false, 2);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
