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
    wstring const FT1 = L"SP1";
    wstring const FT2 = L"SP2";    
}

class TestMultipleReplicaCloseAsyncOperation : public StateMachineTestBase
{
protected:
    ~TestMultipleReplicaCloseAsyncOperation()
    {
        if (operation_ != nullptr)
        {
            VerifyState(true);
        }
    }

    void SetupAndStart(
        initializer_list<wstring> const & ftsToClose)
    {
        SetupAndStart(ftsToClose, true, [](MultipleReplicaCloseAsyncOperation::Parameters) {});
    }

    void SetupAndStart(
        initializer_list<wstring> const & ftsToClose, 
        bool shouldDrain,
        function<void (MultipleReplicaCloseAsyncOperation::Parameters&)> modifier)
    {
        TestLog::WriteInfo(L"SetupAndStart");

        // SetupAndStart the parameters
        MultipleReplicaCloseAsyncOperation::Parameters parameters;
        parameters.Clock = Test.UTContext.RA.ClockSPtr;
        parameters.RA = &Test.UTContext.RA;
        parameters.ActivityId = L"a";
        parameters.FTsToClose = Test.GetFailoverUnitEntries(ftsToClose);
        parameters.CloseMode = ReplicaCloseMode::Close;
        parameters.MaxWaitBeforeTerminationEntry = nullptr;
        parameters.MonitoringIntervalEntry = &Test.UTContext.Config.BuildReplicaTimeLimitEntry;
        parameters.Callback = [](std::wstring const &, EntityEntryBaseList const & , ReconfigurationAgent &) { };

        if (modifier != nullptr)
        {
            modifier(parameters);
        }

        auto op = AsyncOperation::CreateAndStart<MultipleReplicaCloseAsyncOperation>(
            move(parameters),
            [](Common::AsyncOperationSPtr const &) {},
            Test.RA.Root.CreateAsyncOperationRoot());

        operation_ = dynamic_pointer_cast<MultipleReplicaCloseAsyncOperation>(op);

        if (shouldDrain)
        {
            Drain();
        }
    }

    void AddOpenFT(wstring const & shortName)
    {
        Test.AddFT(shortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    }

    void AddDownFT(wstring const & shortName)
    {
        Test.AddFT(shortName, L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }

    void AppHostDown(wstring const & shortName)
    {
        Test.ProcessAppHostClosedAndDrain(shortName);
    }

    void VerifyState(bool isCompleted)
    {
        Verify::AreEqual(isCompleted, operation_->IsCompleted, L"IsCompleted");
        Verify::AreEqual(ErrorCodeValue::Success, operation_->Error.ReadValue(), L"Error");
    }

    void FireTimer()
    {
        auto op = operation_->CloseCompletionCheckAsyncOperation;
        Verify::IsTrue(op != nullptr, L"Close completion check not started");
        op->Test_StartCompletionCheck(op);

        Drain();
    }

    void Cancel()
    {
        operation_->Cancel();
    }

private:
    MultipleReplicaCloseAsyncOperationSPtr operation_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestMultipleReplicaCloseAsyncOperationSuite, TestMultipleReplicaCloseAsyncOperation)

BOOST_AUTO_TEST_CASE(OperationIsCompletedIfNoFTs)
{
    SetupAndStart({});

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(CancelTestwithNoft)
{
    SetupAndStart({});

    Cancel();

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(OperationIsCompletedIfAllFTsAreAlreadyDown)
{
    AddDownFT(FT1);

    AddDownFT(FT2);

    SetupAndStart({FT1, FT2});

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(CancelAfterCompletion)
{
    AddDownFT(FT1);

    AddDownFT(FT2);

    SetupAndStart({ FT1, FT2 });

    Cancel();

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(OperationOnlyConsidersFTList)
{
    AddDownFT(FT1);

    AddOpenFT(FT2);

    SetupAndStart({ FT1 });

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(OperationClosesReplicasAndWaitsForThemToClose)
{
    AddOpenFT(FT1);

    SetupAndStart({ FT1 });

    Verify::AreEqual(ReplicaCloseMode::Close, Test.GetFT(FT1).CloseMode, L"CloseMode");

    VerifyState(false);

    AppHostDown(FT1);

    FireTimer();

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(CancelBeforeCloseJobItemExecute)
{
    AddOpenFT(FT1);

    SetupAndStart({ FT1 }, false, nullptr);

    Cancel();

    VerifyState(false);

    Drain();

    VerifyState(true);
}

BOOST_AUTO_TEST_CASE(CancelWhileCloseCompletionCheckIsRunningCancelsAfterCloseCompletionCheckIsDone)
{
    AddOpenFT(FT1);

    SetupAndStart({ FT1 });

    Cancel();

    VerifyState(true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
