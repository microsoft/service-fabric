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

namespace
{
    wstring const FT1 = L"SP1";
    wstring const FT2 = L"SP2";
    wstring const FT3 = L"SV1";
    ReplicaCloseMode const DefaultCloseMode(ReplicaCloseModeName::Close);
}

namespace CompletionState
{
    enum Enum
    {
        None = 0x00,
        Complete = 0x01,
        TimerArmed = 0x02,

        CompleteAndTimerNotArmed = 0x01,
        IncompleteAndTimerNotArmed = 0x00,
        IncompleteAndTimerArmed = 0x02
    };

    CompletionState::Enum operator|(CompletionState::Enum left, CompletionState::Enum right)
    {
        return static_cast<CompletionState::Enum>(static_cast<int>(left) | static_cast<int>(right));
    }
}

class TestMultipleReplicaCloseCompletionCheckAsyncOperation : public StateMachineTestBase
{
protected:
    ~TestMultipleReplicaCloseCompletionCheckAsyncOperation()
    {
        if (operation_ != nullptr)
        {
            bool hasTimer = operation_->Test_Timer.get() != nullptr;
            Verify::IsTrue(!hasTimer, L"Cannot have a timer -> not cleaned up");

            Verify::IsTrue(operation_->IsCompleted, L"Op must have completed");
            Verify::IsTrue(operation_->Test_IsInternalCompleted, L"Internal complete should be true");
        }
    }

    void VerifyCompletion(CompletionState::Enum expected)
    {
        bool isCompleted = (expected & CompletionState::Complete) != 0;
        bool isTimerArmed = (expected & CompletionState::TimerArmed) != 0;
        Verify::AreEqual(isCompleted, operation_->IsCompleted, L"IsCompleted");

        bool isTimerNull = operation_->Test_Timer == nullptr;
        Verify::AreEqual(isTimerArmed, !isTimerNull, L"Timer Null");

        Verify::AreEqual(ErrorCodeValue::Success, operation_->Error.ReadValue(), L"Error");
    }

    void VerifyCallback(int countExpected, initializer_list<wstring> const& shortNames)
    {
        Verify::AreEqual(countExpected, callbackCount_, L"Callback count");

        auto expected = ConvertShortNamesToPartitionIds(shortNames);
        Verify::VectorStrictAsString(expected, callback_);
    }

    void VerifyTerminateCalls(initializer_list<wstring> const & shortNames)
    {
        if (shortNames.size() == 0)
        {
            Test.ValidateNoTerminateCalls();
        }
        else
        {
            Test.ValidateTerminateCall(shortNames);
        }
    }

    void SetupAndStart(TimeSpan terminateTimeout, initializer_list<wstring> const & ftNamesToClose)
    {
        SetupAndStart(terminateTimeout, ftNamesToClose, nullptr);
    }

    void SetupAndStart(TimeSpan terminateDuration, initializer_list<wstring> const & ftNamesToClose, function<void (MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters&)> modifier)
    {
        TestLog::WriteInfo(L"SetupAndStart");        
        startTime_ = Test.UTContext.Clock.SetManualMode();
        callbackCount_ = 0;
        callback_.clear();

        Test.UTContext.Config.AdminStateTraceInterval = terminateDuration;

        // SetupAndStart the parameters
        MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters;
        parameters.Clock = Test.UTContext.RA.ClockSPtr;
        parameters.MaxWaitBeforeTerminationEntry = terminateDuration == TimeSpan::MaxValue ? nullptr : &Test.UTContext.Config.AdminStateTraceIntervalEntry;
        parameters.ActivityId = L"a";
        parameters.CloseMode = DefaultCloseMode;
        parameters.FTsToClose = ConvertShortNamesToPartitionIds(ftNamesToClose);
        parameters.MonitoringIntervalEntry = &Test.UTContext.Config.BuildReplicaTimeLimitEntry;
        parameters.Callback = [this](std::wstring const & activityId, EntityEntryBaseList const & v, ReconfigurationAgent &)
        {
            Verify::AreEqual(L"a", activityId, L"ActivityId");
            callbackCount_++;
            callback_ = v;
        };
        parameters.RA = &Test.UTContext.RA;

        if (modifier != nullptr)
        {
            modifier(parameters);
        }

        auto op = AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(
            move(parameters),
            [](Common::AsyncOperationSPtr const &) {},
            Test.RA.Root.CreateAsyncOperationRoot());

        operation_ = dynamic_pointer_cast<MultipleReplicaCloseCompletionCheckAsyncOperation>(op);
    }

    StopwatchTime GetTimeAfterStart(int seconds) const
    {
        return startTime_ + TimeSpan::FromSeconds(seconds);
    }

    void AddNullFT(wstring const & shortName)
    {
        Test.AddInsertedFT(shortName);
    }

    void AddClosingFT(wstring const & shortName)
    {
        Test.AddFT(shortName, L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");
    }

    void AddClosedFT(wstring const & shortName)
    {
        Test.AddFT(shortName, L"C None 000/000/411 1:1 -");
    }

    void AddDownFT(wstring const & shortName)
    {
        Test.AddFT(shortName, L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }
    
    void CloseFT(wstring const & shortName)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(shortName, L"000/411 [N/N RD U 1:1] Success -");
    }

    void CloseAndDrain(initializer_list<wstring> const & shortNames)
    {
        for (auto const & it : shortNames)
        {
            CloseFT(it);
        }

        Test.ResetAll();
    }

    void Cancel()
    {
        TestLog::WriteInfo(L"Cancel");
        operation_->Cancel();
    }

    void CancelAndDrain()
    {
        Cancel();
        DrainJobQueues();
    }

    void MultipleCancel()
    {
        Cancel();
        Cancel();
    }

    void DrainJobQueues()
    {
        Test.DrainJobQueues();
        Test.ResetAll();
    }

    void FireTimer(int seconds)
    {
        Test.UTContext.Clock.AdvanceTimeBySeconds(seconds);

        operation_->Test_StartCompletionCheck(operation_);
    }

    void FireTimerAndDrain(int seconds)
    {
        FireTimer(seconds);

        DrainJobQueues();
    }

    EntityEntryBaseList ConvertShortNamesToPartitionIds(initializer_list<wstring> const & shortNames)
    {
        return Test.GetFailoverUnitEntries(shortNames);
    }

    MultipleReplicaCloseCompletionCheckAsyncOperationSPtr operation_;
    StopwatchTime startTime_;

    EntityEntryBaseList callback_;
    int callbackCount_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestMultipleReplicaCloseCompletionCheckAsyncOperationSuite, TestMultipleReplicaCloseCompletionCheckAsyncOperation)

BOOST_AUTO_TEST_CASE(Start_NoReplicasToClose_IsNoOp)
{
    SetupAndStart(TimeSpan::MaxValue, {});

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Start_AlreadyClosedReplicas_IsNoOp)
{
    AddClosedFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Start_NotClosedReplicas_DoesNotComplete_BeforeSchedule)
{
    AddClosingFT({ FT1 });

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    VerifyCompletion(CompletionState::IncompleteAndTimerNotArmed);

    CancelAndDrain();
}

BOOST_AUTO_TEST_CASE(Start_NotClosedReplicas_DoesNotComplete_AfterSchedule)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    VerifyCompletion(CompletionState::IncompleteAndTimerArmed);

    Cancel();
}

BOOST_AUTO_TEST_CASE(Callback_NotCalled_BeforeSchedule)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    VerifyCallback(0, {});

    CancelAndDrain();
}

BOOST_AUTO_TEST_CASE(Callback_NotCalled_IfCancelledAfterSchedule)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    Cancel();

    VerifyCallback(0, {});

    DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(Callback_NotCalled_IfCompleted)
{
    AddClosedFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    VerifyCallback(0, {});
}

BOOST_AUTO_TEST_CASE(Callback_Called_IfStillPending)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    VerifyCallback(1, {FT1});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Callback_Called_IfStillPendingMultipleTimes)
{
    AddClosingFT(FT1);
    AddClosingFT(FT2);

    SetupAndStart(TimeSpan::MaxValue, { FT1, FT2 });
    DrainJobQueues();

    CloseFT(FT1);

    FireTimerAndDrain(2);

    VerifyCallback(2, { FT2 });

    Cancel();
}

BOOST_AUTO_TEST_CASE(ReplicaCompletionReducesNumberOfJobItems)
{
    AddClosingFT(FT1);
    AddClosedFT(FT2);

    SetupAndStart(TimeSpan::MaxValue, { FT1, FT2 });
    DrainJobQueues();

    auto & threadpool = Test.UTContext.ThreadpoolStubObj;
    int current = threadpool.FTQueue.TotalEnqueuedCount;

    FireTimer(2);

    // At this point only FT2 should be enqueued
    Verify::AreEqual(current + 1, threadpool.FTQueue.TotalEnqueuedCount, L"One work item should be enqueued");
    CancelAndDrain();
}

BOOST_AUTO_TEST_CASE(Timer_IfReplicasClose_Completes)
{
    AddClosingFT(FT1);
    AddClosingFT(FT2);

    SetupAndStart(TimeSpan::MaxValue, { FT1, FT2 });

    DrainJobQueues();

    CloseAndDrain({ FT1, FT2 });

    FireTimerAndDrain(4);

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Timer_IfReplicasNotClose_Completes)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    FireTimerAndDrain(4);

    VerifyCompletion(CompletionState::IncompleteAndTimerArmed);

    Cancel();
}

BOOST_AUTO_TEST_CASE(Cancel_AfterNoOpComplete_IsNoOp)
{
    SetupAndStart(TimeSpan::MaxValue, {});

    Cancel();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(MultipleCancel_AfterNoOpComplete_IsNoOp)
{
    SetupAndStart(TimeSpan::MaxValue, {});

    MultipleCancel();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Cancel_DuringScheduledCloseCheck_Completes)
{
    AddClosedFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    Cancel();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);

    DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(Cancel_WaitingForCheck_Completes)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    DrainJobQueues();

    Cancel();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Callback_AfterCancel_DoesNotArmTimer)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::MaxValue, { FT1 });

    Cancel();

    DrainJobQueues();

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_CASE(Terminate_BeforeInterval_DoesNotTerminate)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1 });

    FireTimerAndDrain(1);

    VerifyTerminateCalls({});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Terminate_AfterInterval_IssuesTerminateCall)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1 });

    FireTimerAndDrain(3);

    VerifyTerminateCalls({FT1});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Terminate_AfterInterval_SkipsFTWithoutSTR)
{
    AddDownFT(FT1);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1 }, [](MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters & parameters) { parameters.CloseMode = ReplicaCloseMode::Close; });

    FireTimerAndDrain(3);

    VerifyTerminateCalls({});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Terminate_AfterInterval_DoesNotTerminateTwice)
{
    AddClosingFT(FT1);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1 });

    FireTimerAndDrain(3);

    Test.UTContext.Hosting.Reset();

    FireTimerAndDrain(3);

    VerifyTerminateCalls({});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Terminate_AfterInterval_OnlyCalledForOpenFT)
{
    AddClosingFT(FT1);
    AddClosedFT(FT2);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1, FT2 });

    FireTimerAndDrain(3);

    VerifyTerminateCalls({FT1});

    Cancel();
}

BOOST_AUTO_TEST_CASE(Terminate_AfterInterval_OnlyCalledOncePerServicePackage)
{
    // FT1 and FT3 are in the same package
    AddClosingFT(FT1);
    AddClosingFT(FT3);

    SetupAndStart(TimeSpan::FromSeconds(2), { FT1, FT3 });

    FireTimerAndDrain(3);

    VerifyTerminateCalls({ FT1 });

    Cancel();
}

BOOST_AUTO_TEST_CASE(NullFTIsConsideredClosed)
{
    AddNullFT(FT1);

    SetupAndStart(
        TimeSpan::MaxValue, 
        { FT1 },
        [](MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters & parameters) { parameters.CloseMode = ReplicaCloseMode::Drop; });

    FireTimerAndDrain(6);

    VerifyCompletion(CompletionState::CompleteAndTimerNotArmed);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
