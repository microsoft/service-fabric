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
using namespace Node;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    const FailoverManagerId Owner(false);
}

class TestNodePendingReplicaUploadStateProcessor : public StateMachineTestBase
{

public:
    TestNodePendingReplicaUploadStateProcessor()
    {
        startTime_ = Test.UTContext.Clock.SetManualMode();
        RA.Config.ReopenSuccessWaitIntervalEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    }

protected:
    void Close()
    {
        Get().Close();
    }

    void OnNodeUpAck(bool isDeferred)
    {
        Test.ResetAll();

        Get().ProcessNodeUpAck(L"a", isDeferred);
    }

    void OnNodeUpAckAndDrain(bool isDeferred)
    {
        OnNodeUpAck(isDeferred);

        Drain();
    }

    void OnLastReplicaUpReply()
    {
        Test.ResetAll();

        Get().ProcessLastReplicaUpReply(L"b");
    }

    void VerifyTimer(bool isSet)
    {
        Verify::AreEqual(isSet, Get().Test_IsTimerSet, L"IsTimerSet");
    }

    void VerifyLastReplicaUpMessage()
    {
        Test.ValidateFMMessage<MessageType::ReplicaUp>(L"true false |");
    }

    void VerifyNoMessage()
    {
        Test.ValidateNoFMMessages();
    }

    void OnTimer(int amountToAdvanceTime)
    {
        Test.ResetAll();

        Test.UTContext.Clock.AdvanceTimeBySeconds(amountToAdvanceTime);

        Get().Test_OnTimer(L"a");
    }

    void AddPendingFT()
    {
        Test.AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");

        auto & ft = Test.GetFT(L"SP1");

        Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            L"SP1",
            [&](EntityExecutionContext & base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.UpdateStateOnLFUMLoad(Test.RA.NodeInstance, context);
            ft.ProcessNodeUpAck(vector<UpgradeDescription>(), true, context);
        });
    }

    PendingReplicaUploadStateProcessor & Get()
    {
        return RA.NodeStateObj.GetPendingReplicaUploadStateProcessor(Owner);
    }

    void VerifyMessageStage(FMMessageStage::Enum expected)
    {
        Verify::AreEqual(expected, Test.GetFT(L"SP1").FMMessageStage, L"FM Message Stage");
    }

    void DumpFT()
    {
        TestLog::WriteInfo(wformatString(Test.GetFT(L"SP1")));
    }

    Common::StopwatchTime startTime_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestNodePendingReplicaUploadStateProcessorSuite, TestNodePendingReplicaUploadStateProcessor)

BOOST_AUTO_TEST_CASE(NodeUpAckAfterCloseIsNoOp)
{
    Close();

    OnNodeUpAckAndDrain(true);

    VerifyTimer(false);
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithNoFTsCausesMessageToBeSent)
{
    OnNodeUpAckAndDrain(true);

    VerifyLastReplicaUpMessage();
}

BOOST_AUTO_TEST_CASE(NodeUpAckArmsTimer)
{
    OnNodeUpAckAndDrain(true);

    VerifyTimer(true);
}

BOOST_AUTO_TEST_CASE(CloseAfterNodeUpAck)
{
    OnNodeUpAckAndDrain(true);

    Close();

    VerifyTimer(false);
}

BOOST_AUTO_TEST_CASE(LastReplicaUpReplyDisarmsTimer)
{
    OnNodeUpAckAndDrain(true);

    OnLastReplicaUpReply();

    VerifyTimer(false);
}

BOOST_AUTO_TEST_CASE(TimerAfterLastReplicaUpReplyIsNoOp)
{
    OnNodeUpAckAndDrain(true);

    OnLastReplicaUpReply();

    OnTimer(1);

    VerifyNoMessage();
}

BOOST_AUTO_TEST_CASE(TimerSendsLastReplicaUp)
{
    OnNodeUpAckAndDrain(true);

    OnTimer(1);

    VerifyLastReplicaUpMessage();
}

BOOST_AUTO_TEST_CASE(TimerDoesNotGetDisarmedOnTimer)
{
    OnNodeUpAckAndDrain(true);

    OnTimer(1);

    VerifyTimer(true);
}

BOOST_AUTO_TEST_CASE(TimerExecutesPendingUploadAction)
{
    AddPendingFT();

    OnNodeUpAckAndDrain(true);

    OnTimer(6);

    Drain();

    DumpFT();

    VerifyMessageStage(FMMessageStage::ReplicaUpload);
}

BOOST_AUTO_TEST_CASE(TimerDoesNotExecutesPendingUploadActionIfNotRequested)
{
    AddPendingFT();

    OnNodeUpAckAndDrain(true);

    OnTimer(1);

    Drain();

    DumpFT();

    VerifyMessageStage(FMMessageStage::None);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
