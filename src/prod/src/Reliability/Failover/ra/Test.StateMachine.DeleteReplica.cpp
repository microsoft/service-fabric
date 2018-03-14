// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestStateMachine_DeleteReplica
{
protected:
    void DeleteTest(wstring const & shortName, wstring const & initial, wstring const & finalState, function<void()> action, function<void()> verification);

    void CreateAndValidate(wstring const & shortName, wstring const & initial, function<void()> action, function<void()> verification);
    void CreateAndValidateReplicaCloseMessage(wstring const & shortName, wstring const & initial, function<void()> action);
    void CreateAndValidateReplicaCloseMode(wstring const & shortName, wstring const & initial, function<void()> action);

    void DeleteOfClosedReplicaWithIncomingHigherReplicaId(wstring const & shortName, function<void()> action, function<void()> verification);
    void DeleteOfClosedReplicaDoesNotSetDeletedOnStaleReplicaInstance(wstring const & shortName, function<void()> action, function<void()> verification);
    void DeleteOfClosedReplicaSetsDeletedOnCorrectReplicaInstance(wstring const & shortName, function<void()> action, function<void()> verification);
    
    void ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(wstring const & inital, function<void()> action);
    void ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(function<void()> action);
    void ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(wstring const & inital, function<void()> action);
    void ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(function<void()> action);

    void ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(wstring const & initial, function<void()> action);
    void ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(function<void()> action);
    void ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(wstring const & initial, function<void()> action);
    void ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(function<void()> action);

    void DeleteReplicaDownReplyTest(
        wstring const & shortName,
        wstring const & message,
        bool expected);

    TestStateMachine_DeleteReplica() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_DeleteReplica() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_DeleteReplica::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_DeleteReplica::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_DeleteReplica::DeleteTest(wstring const & shortName, wstring const & initial, wstring const & finalState, function<void()> action, function<void()> verification)
{
    GetScenarioTest().AddFT(shortName, initial);
    action();
    GetScenarioTest().ValidateFT(shortName, finalState);
    verification();
}

void TestStateMachine_DeleteReplica::DeleteOfClosedReplicaDoesNotSetDeletedOnStaleReplicaInstance(wstring const & shortName, function<void()> action, function<void()> verification)
{
    DeleteTest(shortName, L"C None 000/000/411 1:2 -", L"C None 000/000/411 1:2 LP", action, verification);
}

void TestStateMachine_DeleteReplica::DeleteOfClosedReplicaSetsDeletedOnCorrectReplicaInstance(wstring const & shortName, function<void()> action, function<void()> verification)
{
    DeleteTest(shortName, L"C None 000/000/411 1:1 -", L"C None 000/000/411 1:1 LP", action, verification);
}

void TestStateMachine_DeleteReplica::DeleteOfClosedReplicaWithIncomingHigherReplicaId(wstring const & shortName, function<void()> action, function<void()> verification)
{
    DeleteTest(shortName, L"C None 000/000/411 1:1 -", L"C None 000/000/411 2:2 LP", action, verification);
}

void TestStateMachine_DeleteReplica::CreateAndValidate(wstring const & shortName, wstring const & initial, function<void()> action, function<void()> verification)
{
    GetScenarioTest().AddFT(shortName, initial);
    action();
    verification();
}

void TestStateMachine_DeleteReplica::CreateAndValidateReplicaCloseMessage(wstring const & shortName, wstring const & initial, function<void()> action)
{
    GetScenarioTest().AddFT(shortName, initial);
    action();
    GetScenarioTest().ValidateRAPMessage<MessageType::ReplicaClose>(shortName);
}

void TestStateMachine_DeleteReplica::CreateAndValidateReplicaCloseMode(wstring const & shortName, wstring const & initial, function<void()> action)
{
    GetScenarioTest().AddFT(shortName, initial);
    action();
    auto & ft = GetScenarioTest().GetFT(shortName);
    Verify::AreEqual(ft.CloseMode, ReplicaCloseMode::Obliterate, L"ReplicaCloseMode");
}

void TestStateMachine_DeleteReplica::ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(wstring const & initial, function<void()> action)
{
    CreateAndValidateReplicaCloseMessage(
        L"SV1",
        initial,
        action);
}

void TestStateMachine_DeleteReplica::ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(function<void()> action)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        action);
}

void TestStateMachine_DeleteReplica::ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(wstring const & initial, function<void()> action)
{
    CreateAndValidateReplicaCloseMode(
        L"SV1",
        initial,
        action);
}

void TestStateMachine_DeleteReplica::ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(function<void()> action)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        action);
}

void TestStateMachine_DeleteReplica::ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(wstring const & initial, function<void()> action)
{
    CreateAndValidateReplicaCloseMessage(
        L"SL1",
        initial,
        action);
}

void TestStateMachine_DeleteReplica::ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(function<void()> action)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]",
        action);
}

void TestStateMachine_DeleteReplica::ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(wstring const & initial, function<void()> action)
{
    CreateAndValidateReplicaCloseMode(
        L"SL1",
        initial,
        action);
}

void TestStateMachine_DeleteReplica::ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(function<void()> action)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]",
        action);
}

void TestStateMachine_DeleteReplica::DeleteReplicaDownReplyTest(
    wstring const & shortName,
    wstring const & message,
    bool expected)
{
    auto & test = holder_->Recreate();

    test.AddFT(shortName, L"C None 000/000/411 1:2 I");

    if (shortName == L"SL1")
    {
        test.ProcessFMMessageAndDrain<MessageType::RemoveInstance>(shortName, message);
    }
    else
    {
        test.ProcessFMMessageAndDrain<MessageType::DeleteReplica>(shortName, message);
    }

    auto & ft = test.GetFT(shortName);

    auto expectedMessageState = expected ? FMMessageStage::ReplicaDown : FMMessageStage::None;
    Verify::AreEqual(expectedMessageState, ft.FMMessageStage, L"FMMessageStage");
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_DeleteReplicaSuite, TestStateMachine_DeleteReplica)

BOOST_AUTO_TEST_CASE(DeleteReplicaClosedFTStaleMessageTest)
{
    DeleteOfClosedReplicaDoesNotSetDeletedOnStaleReplicaInstance(
        L"SV1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/N RD U 1:1]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::DeleteReplicaReply>(L"SV1", L"000/411 [N/N DD D 1:2 n] Success"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceClosedFTStaleMessageTest)
{
    DeleteOfClosedReplicaDoesNotSetDeletedOnStaleReplicaInstance(
        L"SL1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 1:1]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::RemoveInstanceReply>(L"SL1", L"000/411 [N/N DD D 1:2 n] Success"); });
}


BOOST_AUTO_TEST_CASE(DeleteReplicaClosedFTTest)
{
    DeleteOfClosedReplicaSetsDeletedOnCorrectReplicaInstance(
        L"SV1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/N RD U 1:1]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::DeleteReplicaReply>(L"SV1", L"000/411 [N/N DD D 1:1 n] Success"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceClosedFTTest)
{
    DeleteOfClosedReplicaSetsDeletedOnCorrectReplicaInstance(
        L"SL1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 1:1]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::RemoveInstanceReply>(L"SL1", L"000/411 [N/N DD D 1:1 n] Success"); });
}

BOOST_AUTO_TEST_CASE(DeleteUpdatesStateIfClosed)
{
    GetScenarioTest().AddFT(L"SP1", L"C None 000/000/411 1:1 -");
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP1", L"000/522 [N/N RD U 1:1]");
    GetScenarioTest().ValidateFMMessage<MessageType::DeleteReplicaReply>(L"SP1", L"000/522 [N/N DD D 1:1 n] Success");
    GetScenarioTest().ValidateFT(L"SP1", L"C None 000/000/522 1:1 LP");
}


BOOST_AUTO_TEST_CASE(DeleteReplicaClosedFTHigherReplicaIdTest)
{
    DeleteOfClosedReplicaWithIncomingHigherReplicaId(
        L"SV1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/N RD U 2:2]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::DeleteReplicaReply>(L"SV1", L"000/411 [N/N DD D 1:1:2:2 n] Success"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceClosedFTHigherReplicaIdTest)
{
    DeleteOfClosedReplicaWithIncomingHigherReplicaId(
        L"SL1",
        [this]() { GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 2:2]"); },
        [this]() { GetScenarioTest().ValidateFMMessage<MessageType::RemoveInstanceReply>(L"SL1", L"000/411 [N/N DD D 1:1:2:2 n] Success"); });
}

BOOST_AUTO_TEST_CASE(DeleteClearsReplicaDownReplyPendingForClosedIfReplicaIdIsSame)
{
    auto message = L"000/411 [N/N DD D 1:2]";
    DeleteReplicaDownReplyTest(L"SL1", message, false);
    DeleteReplicaDownReplyTest(L"SV1", message, false);
}

BOOST_AUTO_TEST_CASE(DeleteDoesNotClearReplicaDownReplyPendingForClosedIfReplicaIdIsLower)
{
    auto message = L"000/411 [N/N DD D 1:1:0:1]";
    DeleteReplicaDownReplyTest(L"SL1", message, true);
    DeleteReplicaDownReplyTest(L"SV1", message, true);
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithHigherCCEpoch)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/412 [N/P RD U 1:2]"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithHigherCCEpochAndForce)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/412 [N/P RD U 1:2] true"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithLowerCCEpoch)
{
    CreateAndValidate(
        L"SV1",
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/410 [N/P RD U 1:2]"); },
        [this]() {GetScenarioTest().ValidateFT(L"SV1", L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]"); }); // stale DeleteReplica with lower Epoch will be ignored
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithLowerCCEpochAndForce)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/410 [N/P RD U 1:2] true"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithLowerLowerReplicaInstance)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithLowerLowerReplicaInstanceAndForce)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaForReplicaWithOpenPending)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:2 SM [N/N/P IC U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithForceForOpenPendingReplica)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:2 SM [N/N/P IC U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithForceForClosePendingReplica)
{
    CreateAndValidate(
        L"SV1",
        L"O None 000/000/411 1:2 CHc [N/N/P RD U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1] true"); },
        [this]() {GetScenarioTest().ValidateFT(L"SV1", L"C None 000/000/411 1:2 G -"); });
}

BOOST_AUTO_TEST_CASE(DeleteReplicaWithForceForDownReplica)
{
    ProcessDeleteReplicaMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:2 CM [N/N/P RD D N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD D 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithHigherCCEpoch)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/412 [N/N RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithHigherCCEpochAndForce)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/412 [N/N RD U 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithLowerCCEpoch)
{
    CreateAndValidate(
        L"SL1",
        L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/410 [N/N RD U 1:1]"); },
        [this]() {GetScenarioTest().ValidateFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]"); }); // stale RemoveInstance with lower Epoch will be ignored
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithLowerCCEpochAndForce)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/410 [N/N RD U 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithLowerLowerReplicaInstance)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:2 CM [N/N/N RD U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithLowerLowerReplicaInstanceAndForce)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:2 CM [N/N/N RD U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 1:1] true"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceForReplicaWithOpenPending)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:2 SM [N/N/N IC U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SL1", L"000/411 [N/N RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceWithForceForOpenPendingReplica)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMessage(
        L"O None 000/000/411 1:2 SM [N/N/N IC U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SL1", L"000/411 [N/N RD U 1:1]"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceForClosePendingReplica)
{
    CreateAndValidate(
        L"SL1",
        L"O None 000/000/411 1:2 CHc [N/N/N RD U N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SL1", L"000/411 [N/N RD U 1:1] true"); },
        [this]() {GetScenarioTest().ValidateFT(L"SL1", L"C None 000/000/411 1:2 G -"); });
}

BOOST_AUTO_TEST_CASE(RemoveInstanceForDownReplica)
{
    ProcessRemoveInstanceMessageAndValidateReplicaCloseMode(
        L"O None 000/000/411 1:2 CM [N/N/N RD D N F 1:2]",
        [this]() {GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SL1", L"000/411 [N/N RD D 1:1] true"); });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
