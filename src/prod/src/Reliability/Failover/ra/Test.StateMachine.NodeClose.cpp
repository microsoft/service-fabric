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
    wstring const ShortName = L"SP1";
}

class TestStateMachine_NodeClose : public StateMachineTestBase
{
protected:
    TestStateMachine_NodeClose() : 
    closeCompletedEvent_(make_shared<ManualResetEvent>()),
    wasCloseStarted_(false)
    {
        Test.UTContext.Config.GracefulReplicaShutdownMaxDurationEntry.Test_SetValue(TimeSpan::FromMinutes(5));
        Test.UTContext.Config.PerNodeMinimumIntervalBetweenMessageToFMEntry.Test_SetValue(TimeSpan::Zero);
        Test.UTContext.Config.MinimumIntervalBetweenRAPMessageRetryEntry.Test_SetValue(TimeSpan::Zero);
        Test.UTContext.Config.PerReplicaMinimumIntervalBetweenMessageToFMEntry.Test_SetValue(TimeSpan::Zero);
        Test.UTContext.Clock.SetManualMode();
    }

    ~TestStateMachine_NodeClose()
    {
        if (wasCloseStarted_)
        {
            Drain();

            if (!CheckCloseCompletion())
            {
                Test.UTContext.RA.CloseReplicasAsyncOperation->Cancel();
            }

            VerifyCloseCompleted();
        }
    }

    void AddOpenFT()
    {
        Test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    }

    void AddDroppingFT()
    {
        Test.AddFT(ShortName, L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
    }

    void ProcessReplicaCloseReply(bool isDrop)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(ShortName, wformatString("000/411 [N/P RD U 1:1] Success {0}", isDrop ? L"D" : L"-"));
    }

    void AppHostDown()
    {
        Test.ProcessAppHostClosedAndDrain(ShortName);
    }

    void FireCloseCompletionCheckTimer()
    {
        VerifyCloseIsStuck();

        TimeoutHelper helper(IsDebuggerPresent() ? TimeSpan::FromSeconds(90) : TimeSpan::FromSeconds(5));
        
        MultipleReplicaCloseAsyncOperationSPtr op;

        for (;;)
        {
            Verify::IsTrueLogOnError(!helper.IsExpired, L"Timeout waiting for close");
            op = Test.RA.CloseReplicasAsyncOperation;
            if (op != nullptr)
            {
                break;
            }
            else
            {
                SwitchToThread();
            }
        }
        
        TestLog::WriteInfo(L"Firing close timer");
        auto closeCompletionCheckOp = op->CloseCompletionCheckAsyncOperation;
        closeCompletionCheckOp->Test_StartCompletionCheck(closeCompletionCheckOp);

        Drain();
    }

    void AddDownFT()
    {
        Test.AddFT(ShortName, L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }

    void DumpFT()
    {
        Test.DumpFT(ShortName);
    }

    void StartCloseNode()
    {
        StartCloseNode(true);
    }

    void StartCloseNode(bool waitForJobItem)
    {
        TestLog::WriteInfo(L"Start Close Node");
        Verify::IsTrueLogOnError(!wasCloseStarted_, L"Closing multiple times");

        ManualResetEvent closeStartedEvent;

        if (waitForJobItem)
        {
            Test.UTContext.ThreadpoolStubObj.ItemEnqueuedCallback = [&closeStartedEvent]() { closeStartedEvent.Set(); };
        }

        closeCompletedEvent_->Reset();
        wasCloseStarted_ = true;

        // Start the close in a bg thread
        auto copy = closeCompletedEvent_;
        Common::Threadpool::Post([this, copy]()
        {
            Test.UTContext.Close(); 

            TestLog::WriteInfo(L"Close Completed");
            copy->Set();
        });

        if (waitForJobItem)
        {
            closeStartedEvent.Wait();
            Test.DrainJobQueues();
            Test.UTContext.ThreadpoolStubObj.ItemEnqueuedCallback = nullptr;
        }
    }

    bool CheckCloseCompletion()
    {
        Verify::IsTrueLogOnError(wasCloseStarted_, L"Cannot wait for close if not started");

        auto result = closeCompletedEvent_->WaitOne(50);
        TestLog::WriteInfo(wformatString("CheckCloseCompletion: {0}", result));
        return result;
    }

    void VerifyCloseCompleted()
    {
        auto result = CheckCloseCompletion();
        Verify::IsTrue(result, L"Node Close has not completed");

        TestLog::WriteInfo(L"Close Completion Verified");
    }
    
    void VerifyCloseIsStuck()
    {
        Verify::IsTrueLogOnError(wasCloseStarted_, L"Cannot wait for close if not started");
        Verify::IsTrue(!closeCompletedEvent_->IsSet(), L"Close is stuck");
    }

    void DisableFeature()
    {
        Test.UTContext.Config.GracefulReplicaShutdownMaxDurationEntry.Test_SetValue(TimeSpan::Zero);
    }

    void FireReplicaCloseRetryTimer()
    {
        Test.ResetAll();

        Test.StateItemHelpers.ReplicaCloseRetryHelper.RequestWork();

        Test.DrainJobQueues();
    }

    void FireReplicaDownRetryTimer()
    {
        FireFMMessageRetryTimer();
    }

    void FireReplicaDroppedRetryTimer()
    {
        FireFMMessageRetryTimer();
    }

    void FireFMMessageRetryTimer()
    {
        Test.ResetAll();

        Test.StateItemHelpers.FMMessageRetryHelperObj.Request(*FailoverManagerId::Fm);

        Test.DrainJobQueues();
    }

    void VerifyReplicaCloseMessage()
    {
        Test.ValidateRAPMessage<MessageType::ReplicaClose>(ShortName);
    }

    template<MessageType::Enum Message>
    void VerifyFMMessage()
    {
        Test.ValidateFMMessage<Message>();
    }

    void ResetAll()
    {
        Test.ResetAll();
    }

    void VerifyFMMessageStage(FMMessageStage::Enum expected)
    {
        Verify::AreEqual(expected, Test.GetFT(ShortName).FMMessageStage, L"Expected message stage");
    }

private:
    bool wasCloseStarted_;
    shared_ptr<ManualResetEvent> closeCompletedEvent_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_NodeCloseSuite, TestStateMachine_NodeClose)

BOOST_AUTO_TEST_CASE(ReplicaCloseCausesCloseToWait)
{
    AddOpenFT();

    StartCloseNode();

    VerifyCloseIsStuck();

    DumpFT();

    ProcessReplicaCloseReply(false);

    FireCloseCompletionCheckTimer();

    DumpFT();

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(ClosingReplicasContinueToClose)
{
    AddDroppingFT();

    StartCloseNode();

    VerifyCloseIsStuck();

    ProcessReplicaCloseReply(true);

    FireCloseCompletionCheckTimer();

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringClose)
{
    AddOpenFT();

    StartCloseNode();

    VerifyCloseIsStuck();

    AppHostDown();

    FireCloseCompletionCheckTimer();

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(ReplicaCloseTimerFires)
{
    AddOpenFT();

    StartCloseNode();

    DumpFT();

    FireReplicaCloseRetryTimer();

    VerifyReplicaCloseMessage();
}

BOOST_AUTO_TEST_CASE(DropContinuesDuringClose)
{
    AddDroppingFT();

    StartCloseNode();

    VerifyCloseIsStuck();

    ProcessReplicaCloseReply(true);

    FireCloseCompletionCheckTimer();

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(ReplicaDownIsSentIfHostCrashesForDroppingFT)
{
    AddDroppingFT();

    StartCloseNode();

    ResetAll();

    AppHostDown();

    FireCloseCompletionCheckTimer();

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(ReplicaDownIsSentAndReplyIsProcessed)
{
    AddOpenFT();

    StartCloseNode();

    ResetAll();

    AppHostDown();

    DumpFT();

    VerifyFMMessage<MessageType::ReplicaUp>();

    FireReplicaDownRetryTimer();

    VerifyFMMessage<MessageType::ReplicaUp>();

    ResetAll();

    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");

    DumpFT();

    VerifyFMMessageStage(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(ReplicaDroppedIsSentAndReplyIsProcessed)
{
    AddDroppingFT();

    StartCloseNode();

    ProcessReplicaCloseReply(true);

    DumpFT();

    VerifyFMMessage<MessageType::ReplicaUp>();
    
    FireReplicaDroppedRetryTimer();

    VerifyFMMessage<MessageType::ReplicaUp>();

    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false | {SP1 000/000/411 [N/N/N DD D 1:1]}");

    DumpFT();

    VerifyFMMessageStage(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(ReadWriteStatusRevokedNotificationIsProcessed)
{
    AddOpenFT();

    StartCloseNode();

    Test.ProcessRAPMessageAndDrain<MessageType::ReadWriteStatusRevokedNotification>(ShortName, L"000/411 [N/P RD U 1:1]");

    VerifyFMMessageStage(FMMessageStage::ReplicaDown);
}

BOOST_AUTO_TEST_CASE(HostsAreTermianted)
{
    AddOpenFT();

    StartCloseNode();

    Test.UTContext.Clock.AdvanceTime(Test.UTContext.Config.GracefulReplicaShutdownMaxDuration + TimeSpan::FromSeconds(1));

    FireCloseCompletionCheckTimer();

    Test.ValidateTerminateCall(ShortName);
}

BOOST_AUTO_TEST_CASE(CompletesAfterTwiceTheDuration)
{
    // Set the config to a very low value
    AddOpenFT();

    Test.UTContext.Config.GracefulReplicaShutdownMaxDurationEntry.Test_SetValue(TimeSpan::FromMilliseconds(5));
    StartCloseNode();

    Sleep(10);

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(WithConfigSetToZeroNoCloseIsSent)
{
    DisableFeature();

    AddOpenFT();

    StartCloseNode(false);

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_CASE(WithNoReplicasToCloseCloseIsNoOp)
{
    AddDownFT();

    StartCloseNode(true);

    VerifyCloseCompleted();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
