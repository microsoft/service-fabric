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

class TestStateMachine_DoReconfiguration : public StateMachineTestBase
{
protected:
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_DoReconfigurationSuite, TestStateMachine_DoReconfiguration)

BOOST_AUTO_TEST_CASE(DoReconfigurationClearsSenderNode_ScenarioTest)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1] [N/N/S RD U N F 3:1]");

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(3, L"SP1", L"411/422 [S/S RD U 1:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD U 3:1]");

    auto & ft = Test.GetFT(L"SP1");
    Verify::AreEqual(ReconfigurationAgent::InvalidNode, ft.SenderNode, L"SenderNode");
}

BOOST_AUTO_TEST_CASE(DoReconfiguration_NullFT_CreatesReplica)
{
    // The fm may send doreconfig to a replica that does not exist on the node
    // example: fm send add replica to primary and primary got dropped
    // before create replica was processed so that node has no idea about ft
    // DoReconfig should create dropped ft and send replica dropped
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [I/P RD U 1:1] [P/S RD D 2:1]");

    Test.ValidateFT(L"SP1", L"C None 000/000/433 1:1 -");

    Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SP1 000/000/433 [N/N/N DD D 1:1 n]}");
}

BOOST_AUTO_TEST_CASE(DroppedReplica_DoReconfiguration_UpdatesInstance)
{
    /*
        5132079

        Consider the scenario:
        1. 0/e0 [N/P R1 n1] [N/S R2 n2] [N/S R3 n3] and min = target = 3
        2. R2 is dropped and FM decides to place R4 on n2
        3. e0/e1 [P/P R1 n1] [S/S R4 n3 IB] [S/S R3 n3]
        4. R1 is dropped and FM sends DoReconfig to R4
        5. e0/e2 [P/P R1 DD n1] [S/P R4 n3 IB] [S/S R3 n3]
        6. Since n3 has R2 as DD it will send R2 as ReplicaDropped to FM. However, FM has higher replica and it will consider it stale

        n2 should update its state to R4 DD
    */
    Test.AddFT(L"SP1", L"C None 000/000/411 1:1 -");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/513 [P/S DD D 2:1] [S/P IB U 1:1:4:1] [S/S RD U 3:1]");

    Test.ValidateFT(L"SP1", L"C None 000/000/513 4:1 -");
    Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SP1 000/000/513 [N/N/N DD D 1:1:4:1 n]}");
}

BOOST_AUTO_TEST_CASE(DeletedReplica_DoReconfiguration_UpdatesInstanceAndClearsDelete)
{
    /*
        Similar to the above scenario except that the replica is deleted on the node
        Now, even if it updates the instance it should clear the isdeleted on the node
    */
    Test.AddFT(L"SP1", L"C None 000/000/411 1:1 LP");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/513 [P/S DD D 2:1] [S/P IB U 1:1:4:1] [S/S RD U 3:1]");

    Test.ValidateFT(L"SP1", L"C None 000/000/513 4:1 -");
    Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SP1 000/000/513 [N/N/N DD D 1:1:4:1 n]}");
}

BOOST_AUTO_TEST_CASE(DeletedReplica_DoReconfiguration_Stale_DoesNotUpdateInstance)
{
    /*
        Similar to the above scenario except that the replica is deleted on the node
        Now, even if it updates the instance it should clear the isdeleted on the node
    */
    Test.AddFT(L"SP1", L"C None 000/000/411 1:1 LP");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/513 [P/S DD D 2:1] [S/P IB U 1:1] [S/S RD U 3:1]");

    Test.ValidateFT(L"SP1", L"C None 000/000/513 1:1 LP");
    Test.ValidateNoFMMessages();
}

BOOST_AUTO_TEST_CASE(DoReconfigurationDoesNotSendDuplicateUCMessage)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/I RD D 3:1]");
    Test.DumpFT(L"SP1");

    Test.ResetAll();
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD D 2:1] [S/I RD D 3:1]");
    Test.DumpFT(L"SP1");
    Test.StateItemHelpers.RAPMessageHelper.ValidateTotalMessageCount(1);
    Test.ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD D 2:1] [S/I RD D 3:1] C");
}

BOOST_AUTO_TEST_CASE(EpochShouldBeUpdatedOnDoReconfigurationWhenLocalReplicaIsClosed)
{
    /*
RA on node 3:131084654251651870 start processing message DoReconfiguration [4cc882fe-df20-44b8-a0a6-d03b7dec555c:16027186] with body
198e7494-1eb1-4098-b1b3-15d3dd3b5f2c 131084657160655563:95000001d7/131084657160655563:96000001d9 0
S/P IB U 3:131084654251651870 131084657237886133:131084657237886133 -1 -1 1.0:1.0:0
S/S DD D 2:131084653969287150 131084656229343051:131084656229343051 -1 -1 1.0:1.0:0
P/S DD D 1:131084655488406831 131084655778298584:131084655778298584 -1 -1 1.0:1.0:0
FT:
198e7494-1eb1-4098-b1b3-15d3dd3b5f2c Closed None 131084657160655563:95000001d5/0:0/131084657160655563:95000001d6 0:0:-2 131084657176241831:131084657176241831 3 3 0 | [N N ReplicaDropped 0] 0 0 0 0.0:0.0:0 2016-05-23 08:28:41.189
*/
    Test.AddFT(L"SP1", L"C None 000/000/666 3:1 G");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"777/999 [S/P IB U 3:1] [S/S DD D 2:1] [P/S DD D 1:1]");

    Test.ValidateFT(L"SP1", L"C None 000/000/999 3:1 G");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterChangeConfigurationReplyShouldNotForwardMessage)
{
    /*

1. RA performs change configuration e0/e1
2. RA receives change configuration reply
3. stale DoReconfiguration e0/e1 arrives prior to GetLSN e0/e2

RA considers this as a DoReconfiguration message during swap primary where demote is complete and fires test assert

*/
    Test.AddFT(L"SP1", L"O None 411/000/522 1:1 CM [S/N/S RD U N F 1:1] [P/N/N DD D N F 2:1] [S/N/S RD U N F 3:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/522 [S/P RD U 1:1] [P/N DD D 2:1] [S/S RD U 3:1]");

    Test.ValidateNoRAMessage();
    Test.ValidateFT(L"SP1", L"O None 411/000/522 1:1 CM [S/N/S RD U N F 1:1] [P/N/N DD D N F 2:1] [S/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterSwapForwardsMessage)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/P RD U 1:1] [S/P RD U 1:1] [P/S RD U 2:1] Success C");
    Test.ResetAll();

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");
    Test.ValidateRAMessage<MessageType::DoReconfiguration>(2);
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterCatchupIsCancelledDoesNotForwardMessage)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");

    Test.ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(L"SP1", L"411/422 [P/P RD U 1:1] Success -");
    Test.ResetAll();

    Test.DumpFT(L"SP1");
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");
    Test.ValidateNoRAMessage();
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterRAReportFault)
{
	Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");

	Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD U 3:1]");
	Test.DumpFT(L"SP1");

	Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD U 3:1] Success C");
	Test.DumpFT(L"SP1");

	Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, L"SP1", L"411/422 [N/N RD U 1:1]");
	Test.DumpFT(L"SP1");

	Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1] [S/S RD U 3:1]");
	Test.DumpFT(L"SP1");

	Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/433 {422:0} [N/S RD U 3:1 4 4] Success");
	Test.DumpFT(L"SP1");

	Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/433 [P/P RD U 1:1 0 2] Success -");
	Test.DumpFT(L"SP1");

	// After previous msg, FM sending DoReconfiguration to new primary on node 3. RA on node 3 sends GetLSN to RA on node 1
	Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(3, L"SP1", L"411/444 [N/N RD U 1:1]");
	Test.DumpFT(L"SP1");

	Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/444 [P/P RD U 1:1 0 4] Success -");
	Test.DumpFT(L"SP1");

	Test.ResetAll();
	Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/455 [P/P RD U 1:1] [S/S RD D 2:1] [S/S DD D 3:1]");
	Test.DumpFT(L"SP1");

	Verify::AreEqual(FailoverUnitReconfigurationStage::Phase1_GetLSN, Test.GetFT(L"SP1").ReconfigurationStage, L"ReconfigStage");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterChangeConfiguration)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [S/P RD U 1:1] [P/S IB U 2:2]");
    Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/422 [N/S RD U 1:1 2 2] Success -");
    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, L"SP1", L"411/422 {411:0} [N/S RD U 2:2 2 4] Success");
    Test.DumpFT(L"SP1");

    Test.DumpFT(L"SP1");
    Test.ResetAll();
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [S/P RD U 1:1] [P/S IB U 2:2]");
    Verify::IsTrue(!Test.GetFT(L"SP1").IsReconfiguring, L"FT should not be configuring");
    Test.ValidateFMMessage<MessageType::ChangeConfiguration>(L"SP1", L"411/422 [N/P RD U 1:1 -1 -1] [N/S IB U 2:2 2 4]");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterAbortSwapMovesToPhase2)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");

    Test.ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(L"SP1", L"411/422 [P/P RD U 1:1] Success -");
    Test.ResetAll();

    Test.DumpFT(L"SP1");
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");
    Test.DumpFT(L"SP1");
    Verify::AreEqual(FailoverUnitReconfigurationStage::Phase2_Catchup, Test.GetFT(L"SP1").ReconfigurationStage, L"ReconfigStage");
    Test.ValidateNoRAMessage();
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterAbortCompletesMovesToPhase1)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");

    Test.ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(L"SP1", L"411/422 [P/P RD U 1:1] RAProxyDemoteCompleted -");
    Test.ResetAll();

    Test.DumpFT(L"SP1");
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");
    Test.DumpFT(L"SP1");
    Verify::AreEqual(FailoverUnitReconfigurationStage::Phase1_GetLSN, Test.GetFT(L"SP1").ReconfigurationStage, L"ReconfigStage");
    Test.ValidateNoRAMessage();
}

BOOST_AUTO_TEST_CASE(DoReconfigurationIfFirstSwapPrimaryIsDroppedAndTheNewPrimaryGoesDown)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/433 [P/P RD U 1:1] [S/S RD D 2:1]");

    Test.DumpFT(L"SP1");
    Verify::AreEqual(FailoverUnitReconfigurationStage::Phase2_Catchup, Test.GetFT(L"SP1").ReconfigurationStage, L"ReconfigStage");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAfterGetLSNAndChangeConfiguration)
{
    // Regression test for 6873828
    Test.AddFT(L"SP1", L"O None 000/000/422 1:1 CM [N/N/P SB U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");
    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, L"SP1", L"411/433 [N/N RD U 1:1]");
    Test.DumpFT(L"SP1");
    
    Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorGetStatusReply>(L"SP1", L"422/433 [P/P SB U 1:1 1 1] Success -");
    Test.DumpFT(L"SP1");

    Test.ResetAll();
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/444 [S/P IB U 1:1] [P/S IB U 2:2] [S/S RD D 3:1]");

    Test.DumpFT(L"SP1");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationAddSecondaryTest)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S DD D N F 3:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/S IB U 3:1:4:4]");

    Test.DumpFT(L"SP1");
}

BOOST_AUTO_TEST_CASE(DoReconfigurationSwapSecondaryIBAsSBMarksDropPending)
{
    Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S IB U N F 3:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S SB U 3:2]");

    Test.ValidateFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S IB D N R 3:1]");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
