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
    wstring const Persisted = L"SP1";
    wstring const Persisted2 = L"SP2";
    wstring const FM = L"FM";

    namespace FTState
    {
        enum Enum
        {
            Up,
            Down,
            DownInDrop,
        };
    }
}

class TestStateMachine_LastReplicaUp : public StateMachineTestBase
{
protected:
    TestStateMachine_LastReplicaUp()
    {
        Test.SetNodeUpAckProcessed(false, false);
        Test.UTContext.Config.PerNodeMinimumIntervalBetweenMessageToFMEntry.Test_SetValue(TimeSpan::Zero);
        Test.UTContext.Config.PerReplicaMinimumIntervalBetweenMessageToFMEntry.Test_SetValue(TimeSpan::Zero);
        Test.SetFindServiceTypeRegistrationSuccess(Persisted);
        Test.SetFindServiceTypeRegistrationSuccess(Persisted2);
    }

    void RequestLastReplicaUp()
    {
        RequestLastReplicaUp(*FailoverManagerId::Fm);
        Test.DrainJobQueues();
    }

    void RequestLastReplicaUp(FailoverManagerId const & fm)
    {
        Test.ResetAll();
        Test.StateItemHelpers.LastReplicaUpHelper.Request(fm);
    }

    void ValidateLastReplicaUp()
    {
        ValidateLastReplicaUp(false);
    }

    void ValidateLastReplicaUp(bool isFmm)
    {
        if (isFmm)
        {
            Test.ValidateFmmMessage<MessageType::ReplicaUp>(L"true true | ");
        }
        else
        {
            Test.ValidateFMMessage<MessageType::ReplicaUp>(L"true false | ");
        }
    }

    void ValidateLastReplicaUpAcknowledged()
    {
        Test.StateItemHelpers.LastReplicaUpHelper.ValidateLastReplicaUpReplyState(*FailoverManagerId::Fm, true);
    }

    void RequestFMWork()
    {
        Test.StateItemHelpers.FMMessageRetryHelperObj.Request(*FailoverManagerId::Fm);
        Test.DrainJobQueues();
    }

    void ValidateNoReplicaUp()
    {
        ValidateNoReplicaUp(false);
    }

    void ValidateNoReplicaUp(bool isFmm)
    {
        if (isFmm)
        {
            Test.ValidateNoFmmMessagesOfType<MessageType::ReplicaUp>();
        }
        else
        {
            Test.ValidateNoFMMessagesOfType<MessageType::ReplicaUp>();
        }
    }

    void AddDownFT()
    {
        AddDownFT(Persisted);
    }

    void AddDownFT(wstring const & shortName)
    {
        AddFT(shortName, FTState::Down);
    }

    void AddFT(wstring const & shortName, FTState::Enum state)
    {
        wstring replicaState = state == FTState::DownInDrop ? L"ID" : L"SB";
        wstring isUp = state == FTState::Up ? L"U" : L"D";

        Test.AddFT(shortName, wformatString("O None 000/000/411 1:1 - [N/N/P {0} {1} N F 1:1]", replicaState, isUp));
    }

    void DumpFT()
    {
        Test.DumpFT(Persisted);
    }

    void ExecuteUpdateStateOnLfumLoad()
    {
        ExecuteUpdateStateOnLfumLoad(Persisted);
    }    
    
    void ExecuteUpdateStateOnLfumLoad(wstring const & shortName)
    {
        Test.ExecuteUpdateStateOnLfumLoad(shortName);
    }

    void ProcessNodeUpAck(wstring const & msg)
    {
        ProcessNodeUpAck(msg, false);
    }

    void ProcessNodeUpAck(wstring const & msg, bool isFmm)
    {
        Test.ProcessNodeUpAckAndDrain(msg, isFmm);
        Test.RequestWorkAndDrain(Test.StateItemHelpers.ReplicaOpenRetryHelper);
        Test.RequestWorkAndDrain(Test.StateItemHelpers.ReplicaCloseRetryHelper);
    }

    void ProcessReopenReply()
    {
        ProcessReopenReply(Persisted);
    }

    void ProcessReopenReply(wstring const & shortName)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(shortName, L"000/411 [N/P SB U 1:2] Success -");
    }

    void ProcessLastReplicaUpReply()
    {
        ProcessLastReplicaUpReply(false);
    }

    void ProcessLastReplicaUpReply(bool isFmm)
    {
        Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(wformatString("true {0} | ", isFmm));
    }    
    
    void ValidateReplicaUp(bool isDown, bool isLast)
    {
        ValidateReplicaUp(Persisted, isDown ? FTState::Down : FTState::Up, isLast);
    }

    void ValidateReplicaUp(FTState::Enum state, bool isLast)
    {
        ValidateReplicaUp(Persisted, state, isLast);
    }

    void ValidateReplicaUp(wstring const & shortName, bool isDown, bool isLast)
    {
        ValidateReplicaUp(shortName, isDown ? FTState::Down : FTState::Up, isLast);
    }

    void ValidateReplicaUp(wstring const & shortName, FTState::Enum state, bool isLast)
    {
        auto msg = GetReplicaUpMessageBody(shortName, state, isLast);

        Test.ValidateFMMessage<MessageType::ReplicaUp>(msg);
    }

    void ProcessReplicaUpReply(FTState::Enum state)
    {
        ProcessReplicaUpReply(state, false);
    }

    void ProcessReplicaUpReply(FTState::Enum state, bool isLast)
    {
        auto msg = GetReplicaUpMessageBody(Persisted, state, isLast);
        
        Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(msg);
    }

    wstring GetStringForReplicaForReplicaUpBody(wstring const& shortName, FTState::Enum state)
    {
        wstring replicaState = state == FTState::DownInDrop ? L"DD" : L"SB";
        wstring isUp = state == FTState::Up ? L"U" : L"D";
        wstring instance = state == FTState::Down ? L"1:1" : L"1:2";
        wstring role = state == FTState::DownInDrop ? L"N" : L"P";
        wstring suffix = state == FTState::DownInDrop ? L"n" : L"-1 -1 (1.0:1.0:1)";

        auto replica = L"{" + wformatString("{0} 000/000/411 [N/N/{4} {1} {2} {3} {5}]", shortName, replicaState, isUp, instance, role, suffix) + L"}";
        if (state == FTState::DownInDrop)
        {
            return L" | " + replica;
        }
        else
        {
            return replica + L" | ";
        }
    }

    wstring GetReplicaUpMessageBody(wstring const & shortName, FTState::Enum state, bool isLast)
    {
        auto replica = GetStringForReplicaForReplicaUpBody(shortName, state);
        return wformatString("{0} false {1} ", isLast, replica);
    }

    void LastReplicaUpWithNoFTTestHelper(bool isFmm)
    {
        ProcessNodeUpAck(L"true 1", isFmm);

        ValidateLastReplicaUp(isFmm);

        ProcessLastReplicaUpReply(isFmm);

        RequestLastReplicaUp(FailoverManagerId(isFmm));

        ValidateNoReplicaUp(isFmm);
    }

    void SetReopenSuccessWaitIntervalToZero()
    {
        Test.UTContext.Config.ReopenSuccessWaitIntervalEntry.Test_SetValue(TimeSpan::Zero);
    }

    void TestSetup(FTState::Enum initialState, bool isNodeActivated)
    {
        AddFT(Persisted, initialState);

        ExecuteUpdateStateOnLfumLoad();

        if (isNodeActivated)
        {
            ProcessNodeUpAck(L"true 1");
        }
        else
        {
            ProcessNodeUpAck(L"false 1");
        }
    }

    void SetupForLruNoInstanceTest()
    {
        TestSetup(FTState::Down, true);

        ProcessReopenReply();

        InvokeClose(ReplicaCloseMode::Close);

        Test.DumpFT(Persisted);

        SetReopenSuccessWaitIntervalToZero();

        Test.StateItemHelpers.LastReplicaUpHelper.Request(*FailoverManagerId::Fm);
    }

    void InvokeClose(ReplicaCloseMode mode)
    {
        auto & ft = Test.GetFT(Persisted);
        Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            Persisted,
            [&](Infrastructure::EntityExecutionContext& base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.StartCloseLocalReplica(mode, NodeInstance(), context, ActivityDescription::Empty);
        });
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_LastReplicaUpSuite, TestStateMachine_LastReplicaUp)

BOOST_AUTO_TEST_CASE(ReplicaUpForMultipleServices)
{
    AddDownFT();
    AddDownFT(Persisted2);

    ExecuteUpdateStateOnLfumLoad();
    ExecuteUpdateStateOnLfumLoad(Persisted2);

    ProcessNodeUpAck(L"true 1");

    // Reopen
    ProcessReopenReply();
    ValidateReplicaUp(false, false);
    Test.ResetAll();

    ProcessReplicaUpReply(FTState::Up);

    // Last Replica up still needs to be sent
    RequestLastReplicaUp();
    ValidateNoReplicaUp();

    Test.ResetAll();
    Test.DumpFT(Persisted2);
    ProcessReopenReply(Persisted2);

    ValidateReplicaUp(Persisted2, false, true);
}

BOOST_AUTO_TEST_CASE(ReplicaUpForServicesBeingReopened)
{
    TestSetup(FTState::Down, true);
    
    ValidateNoReplicaUp();
    
    // Reopen
    ProcessReopenReply();
    ValidateReplicaUp(false, true);
    Test.ResetAll();

    ProcessReplicaUpReply(FTState::Up);
    
    // Last Replica up still needs to be sent
    RequestLastReplicaUp();
    
    ValidateLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(ReplicaUpWhereReopenTakesALongTime)
{
    TestSetup(FTState::Down, true);

    SetReopenSuccessWaitIntervalToZero();

    RequestLastReplicaUp();

    DumpFT();

    ValidateReplicaUp(true, true);
    Test.ResetAll();

    ProcessReplicaUpReply(FTState::Down);

    RequestLastReplicaUp();

    ValidateLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(ReplicaUpWherePriorToUpAckFTIsClosed)
{
    TestSetup(FTState::Down, true);

    // FT is now open and replica up is pending
    ProcessReopenReply();

    InvokeClose(ReplicaCloseMode::Close);

    // Starting close cancels replica up
    // So now it should say replica upload
    SetReopenSuccessWaitIntervalToZero();
    RequestLastReplicaUp();
    DumpFT();

    ValidateReplicaUp(FTState::Up, true);

    ProcessReplicaUpReply(FTState::Up, true);

    ValidateLastReplicaUpAcknowledged();
}

BOOST_AUTO_TEST_CASE(LastReplicaUpWhereDropTakesALongTime)
{
    // LRU should not wait for ID 
    TestSetup(FTState::DownInDrop, true);

    SetReopenSuccessWaitIntervalToZero();

    RequestLastReplicaUp();

    DumpFT();

    ValidateReplicaUp(FTState::DownInDrop, true);

    ProcessReplicaUpReply(FTState::DownInDrop);

    RequestLastReplicaUp();

    ValidateLastReplicaUp();

}

BOOST_AUTO_TEST_CASE(LastReplicaUpWithNoFTs)
{
    LastReplicaUpWithNoFTTestHelper(false);
}

BOOST_AUTO_TEST_CASE(LastReplicaUpForFmm)
{
    LastReplicaUpWithNoFTTestHelper(true);
}

BOOST_AUTO_TEST_CASE(LastReplicaUpForDeactivateNode)
{
    TestSetup(FTState::Down, false);

    Test.ResetAll();

    RequestFMWork();
    ValidateReplicaUp(true, true);

    ProcessReplicaUpReply(FTState::Down);

    DumpFT();

    RequestLastReplicaUp();

    ValidateLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(LastReplicaUpReplyWhenNodeIsClosed)
{
    TestSetup(FTState::Down, true);

    ProcessReopenReply();

    Test.ResetAll();

    // Now process the replica up reply with isLast set to true
    // Close the RA before the job item is processed so that when the queue is drained
    // Before the call to the pending replica upload state
    // The replica up processinger must ensure that the job item
    // is processed and succeeded must be validated
    // prior to telling the replica upload state that 
    // the last replica up reply message was fully processed by the RA
    auto msg = GetReplicaUpMessageBody(Persisted, FTState::Up, true);

    Test.ProcessFMMessage<MessageType::ReplicaUpReply>(msg);

    Test.UTContext.Close();
}

BOOST_AUTO_TEST_CASE(LastReplicaUpReplyWithCommitFailure)
{
    TestSetup(FTState::Down, true);

    ProcessReopenReply();

    Test.ResetAll();

    auto msg = wformatString("true false | {SP1 000/000/411 [N/N/P DD D 1:2]");

    Test.EnableLfumStoreFaultInjection();

    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(msg);

    Test.DumpFT(Persisted);
}

BOOST_AUTO_TEST_CASE(LRUWithNoInstanceAndClosing)
{
    SetupForLruNoInstanceTest();
    // Now take down app host
    Test.ProcessAppHostClosedAndDrain(Persisted);

    Test.DumpFT(Persisted);
}

BOOST_AUTO_TEST_CASE(LRUWithNoInstanceAndReplicaGoingDown)
{
    SetupForLruNoInstanceTest();

    // Now directly take down ft
    Test.ProcessRAPMessageAndDrain<MessageType::ReadWriteStatusRevokedNotification>(L"SP1", L"000/411 [N/P RD U 1:2]");
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
