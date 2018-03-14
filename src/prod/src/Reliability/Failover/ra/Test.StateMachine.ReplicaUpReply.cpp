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

namespace
{
    const wstring FT = L"SP1";
    const wstring Volatile = L"SV1";
    const wstring Persisted = L"SP1";

    struct Info
    {
        Info() : IsUp(true) {}

        int64 ReplicaId;
        int64 ReplicaInstanceId;
        Reliability::ReplicaStates::Enum State;
        bool IsUp;
    };
}

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
                template<>
                struct ReaderImpl<Info>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Info & rv)
                    {
                        ErrorLogger errLogger(L"Info", value);
                        Reader reader(value, context);

                        rv = Info();
                        if (!reader.Read<Reliability::ReplicaStates::Enum>(L' ', rv.State))
                        {
                            errLogger.Log(L"State");
                            return false;
                        }

                        if (!reader.Read<int64>(L':', rv.ReplicaId))
                        {
                            errLogger.Log(L"ReplicaId");
                            return false;
                        }

                        if (!reader.Read<int64>(L' ', rv.ReplicaInstanceId))
                        {
                            errLogger.Log(L"ReplicaInstanceId");
                            return false;
                        }

                        if (!reader.IsEOF)
                        {
                            if (!reader.Read<bool>(L' ', rv.IsUp))
                            {
                                errLogger.Log(L"IsUp");
                                return false;
                            }
                        }

                        return true;
                    }
                };
            }
        }
    }
}

class TestStateMachine_ReplicaUpReply : public StateMachineTestBase
{
protected:

    void AddReplicaDroppedReplyPendingFT()
    {
        Test.AddFT(FT, L"C None 000/000/411 1:2 G");
    }

    void AddReplicaUpReplyPendingFT()
    {
        Test.AddFT(FT, L"O None 000/000/411 1:1 CMK [N/N/P SB U N F 1:1]");
    }

    void AddClosedDownReplyPendingFT(wstring const & ft)
    {
        AddFT(ft, L"C None 000/000/411 1:1 I");
    }

    void AddDownDownReplyPendingFT(wstring const & ft)
    {
        AddFT(ft, L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]");
    }

    void AddReopeningDownReplyPendingFT(wstring const & ft)
    {
        AddFT(ft, L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]");
    }

    void AddFT(wstring const & ft, wstring const & state)
    {
        Test.AddFT(ft, state);
    }

    void ProcessMessage(wstring const & msg)
    {
        Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(msg);
    }

    CommitTypeDescriptionUtility ProcessOnFT(wstring const & replicaDesc)
    {
        return ProcessOnFT(FT, replicaDesc);
    }

    CommitTypeDescriptionUtility ProcessOnFT(wstring const & ftShortName, wstring const & replicaDesc)
    {
        auto & ft = Test.GetFT(ftShortName);

        auto info = Reader::ReadHelper<Info>(replicaDesc);
        ReplicaDescription desc;
        desc.ReplicaId = info.ReplicaId;
        desc.InstanceId = info.ReplicaInstanceId;
        desc.State = info.State;
        desc.IsUp = info.IsUp;

        return Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            ftShortName,
            [&](Infrastructure::EntityExecutionContext& base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.ProcessReplicaUpReply(desc, context);
        });
    }

    void VerifyReplicaDroppedPending()
    {
        Verify(FMMessageStage::ReplicaDropped);
    }

    void VerifyCleared()
    {
        Verify(FMMessageStage::None);
    }

    void VerifyCleared(wstring const & ft)
    {
        Verify(ft, FMMessageStage::None);
    }

    void Verify(FMMessageStage::Enum expected)
    {
        Verify(FT, expected);
    }

    void Verify(wstring const & ftShortName, FMMessageStage::Enum expected)
    {
        auto & ft = Test.GetFT(ftShortName);

        Verify::AreEqual(expected, ft.FMMessageStage, L"FMMessageStage");
    }

    void RunReplicaUpReplyTest(bool isNodeActivated, wstring const & testName, wstring const & shortName, wstring const & initial, wstring const & message, wstring const & final)
    {
        Recreate();

        Test.SetNodeActivationState(isNodeActivated, 10);
        Test.LogStep(testName);

        Test.AddFT(shortName, initial);
        ProcessMessage(message);

        Test.ValidateFT(shortName, final);
    }

    void RunReplicaUpReplyTestWithNodeActivated(wstring const & testName, wstring const & shortName, wstring const & initial, wstring const & message, wstring const & final)
    {
        RunReplicaUpReplyTest(true, testName, shortName, initial, message, final);
    }

    void RunReplicaUpReplyTestWithNodeDeactivated(wstring const & testName, wstring const & shortName, wstring const & initial, wstring const & message, wstring const & final)
    {
        RunReplicaUpReplyTest(false, testName, shortName, initial, message, final);
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaUpReplySuite, TestStateMachine_ReplicaUpReply)

BOOST_AUTO_TEST_CASE(ReplicaUp_SuccessEndToEnd)
{
    AddReplicaUpReplyPendingFT();

    ProcessMessage(L"false false {SP1 000/000/411 [N/N/P SB U 1:1]} | ");

    VerifyCleared();
}

BOOST_AUTO_TEST_CASE(ReplicaUp_SuccessOnFTUpdates)
{
    AddReplicaUpReplyPendingFT();

    ProcessOnFT(L"SB 1:1").VerifyInMemoryCommit();

    VerifyCleared();
}

BOOST_AUTO_TEST_CASE(ReplicaUp_MismatchReplicaInstanceIsNoOp)
{
    AddReplicaUpReplyPendingFT();

    ProcessOnFT(L"SB 1:2").VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(ReplicaDropped_StaleStateIsNoOp)
{
    AddReplicaDroppedReplyPendingFT();

    // Replica up was sent for 1:2 and then replica completed drop
    ProcessOnFT(L"SB 1:2").VerifyNoOp();

    VerifyReplicaDroppedPending();
}

BOOST_AUTO_TEST_CASE(ReplicaDropped_Success)
{
    AddReplicaDroppedReplyPendingFT();

    ProcessOnFT(L"DD 1:2").VerifyInMemoryCommit();

    VerifyCleared();
}

BOOST_AUTO_TEST_CASE(ReplicaDown_SuccessEndToEnd)
{
    AddClosedDownReplyPendingFT(Volatile);

    ProcessMessage(L"false false {SV1 000/000/411 [N/N/N DD D 1:1]} | ");

    VerifyCleared(Volatile);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Volatile_Success)
{
    AddClosedDownReplyPendingFT(Volatile);

    ProcessOnFT(Volatile, L"DD 1:1 false").VerifyInMemoryCommit();

    VerifyCleared(Volatile);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Volatile_MismatchInstanceId_NoOp)
{
    AddClosedDownReplyPendingFT(Volatile);

    ProcessOnFT(Volatile, L"DD 1:2 false").VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Volatile_DownReplyNotPending_NoOp)
{
    AddFT(Volatile, L"C None 000/000/411 1:1 -");

    ProcessOnFT(Volatile, L"DD 1:1 false").VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_Success)
{
    AddDownDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:1 false").VerifyInMemoryCommit();

    VerifyCleared(Persisted);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_PreReopen_Success)
{
    AddReopeningDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:1 false").VerifyInMemoryCommit();

    VerifyCleared(Persisted);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_MismatchInstance)
{
    AddDownDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:2 true").VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_ReplicaIsUp)
{
    // Replica up for down replica
    AddDownDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:1 true").VerifyNoOp();

    Verify(Persisted, FMMessageStage::ReplicaDown);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_PreReopen_MismatchInstance)
{
    AddReopeningDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:2").VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(ReplicaDown_Persisted_DownReplyPending_PreReopen_NotPending)
{
    AddReopeningDownReplyPendingFT(Persisted);

    ProcessOnFT(Persisted, L"SB 1:1 false");

    ProcessOnFT(Persisted, L"SB 1:1 false").VerifyNoOp();
}


BOOST_AUTO_TEST_CASE(Tests_NodeActivated)
{
    // Closed FT when received as replica up reply should be marked as deleted if dropped on FM
    RunReplicaUpReplyTestWithNodeActivated(
        L"Closed FT when received as replica up reply should be marked as deleted if dropped on FM",
        L"SP1",
        L"C None 000/000/411 1:1 -",
        L"false false {SP1 000/000/411 [N/N/N DD D 1:1]} |",
        L"C None 000/000/411 1:1 LP");

    // ClosedFT when
    RunReplicaUpReplyTestWithNodeActivated(
        L"Closed FT when received as replica up reply (in Dropped list) should be marked as deleted ",
        L"SP1",
        L"C None 000/000/411 1:1 -",
        L"false false | {SP1 000/000/411 [N/N/N DD D 1:1]}",
        L"C None 000/000/411 1:1 LP");

    // FT Is Open and replica up reply is pending
    // Replica Up reply is coming for an earlier instance because of reopen expiry
    // Start delete as even if the earlier instance is deleted the current instance is also deleted
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case3",
        L"SP1",
        L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:1]}",
        L"O None 000/000/411 1:2 CMHfL [N/N/P ID U N F 1:2]");

    // FT Is open and reply comes in with correct instance in dropped list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case4",
        L"SP1",
        L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:2]}",
        L"O None 000/000/411 1:2 CMHfL [N/N/P ID U N F 1:2]");

    // FT is open and reply comes in with correct instance in up list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case5",
        L"SP1",
        L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]",
        L"false false {SP1 000/000/411 [N/N/P SB U 1:2]} |",
        L"O None 000/000/411 1:2 CM [N/N/P SB U N F 1:2]");

    // FT has registration and reply comes in with correct instance dropped list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case6",
        L"SP1",
        L"O None 000/000/411 1:2 SM [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:2]}",
        L"O None 000/000/411 1:2 MLHf [N/N/P ID U N F 1:2]");

    // FT does not have registration and reply comes in with correct instance dropped list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case7",
        L"SP1",
        L"O None 000/000/411 1:2 S [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:2]}",
        L"C None 000/000/411 1:2 LP");

    // FT has registration and reply comes in with lower instance in dropped list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case8",
        L"SP1",
        L"O None 000/000/411 1:2 SM [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:1]}",
        L"O None 000/000/411 1:2 MHfL [N/N/P ID U N F 1:2]");

    // FT does not have registration and reply comes in with lower instance in dropped list
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case9",
        L"SP1",
        L"O None 000/000/411 1:2 S [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:1]}",
        L"C None 000/000/411 1:2 PL");

    // ForceDelete and reply comes in with lower instance id should reset the ReplicaDown flag
    RunReplicaUpReplyTestWithNodeActivated(
        L"Case10",
        L"SP1",
        L"O None 000/000/411 1:2 1 ISM [N/N/P SB U N F 1:2]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:1]",
        L"O None 000/000/411 1:2 MLHf [N/N/P ID U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(Tests_NodeDeactivated)
{
    // Closed FT when received as replica up reply should be marked as deleted if dropped on FM
    RunReplicaUpReplyTestWithNodeDeactivated(
        L"Closed FT when received as replica up reply should be marked as deleted if dropped on FM",
        L"SP1",
        L"C None 000/000/411 1:1 -",
        L"false false {SP1 000/000/411 [N/N/N DD D 1:1]} |",
        L"C None 000/000/411 1:1 LP");

    // ClosedFT when
    RunReplicaUpReplyTestWithNodeDeactivated(
        L"Closed FT when received as replica up reply (in Dropped list) should be marked as deleted ",
        L"SP1",
        L"C None 000/000/411 1:1 -",
        L"false false | {SP1 000/000/411 [N/N/N DD D 1:1]}",
        L"C None 000/000/411 1:1 LP");

    // FT Is open and reply comes in with correct instance in dropped list
    // It should be ignored
    RunReplicaUpReplyTestWithNodeDeactivated(
        L"Case4",
        L"SP1",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]",
        L"false false | {SP1 000/000/411 [N/N/P DD D 1:1]}",
        L"O None 000/000/411 1:1 L [N/N/P ID D N F 1:1]");

    // FT is open and reply comes in with correct instance in up list
    // This case should not happen as the FM should send it in the dropped list if
    // it wants the RA to delete it as the RA still has the replica
    RunReplicaUpReplyTestWithNodeDeactivated(
        L"Case5",
        L"SP1",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]",
        L"false false {SP1 000/000/411 [N/N/P DD D 1:1]} |",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(FTInNormalListButDownIsIgnored)
{
    // ReplicaUp was sent for last replica up and the replica was SB D 1:1 (because reopen success wait interval expired)
    // ReplicaUpRpely from FM acknowledges this
    // Replica got reopened in the meantime so the replica up for this instance is still pending
    Test.AddFT(L"SP1", L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]");
    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
    Verify::AreEqual(FMMessageStage::ReplicaUp, Test.GetFT(L"SP1").FMMessageStage, L"ReplicaUp should be pending");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
