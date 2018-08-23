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
using namespace Reliability::ReconfigurationAgentComponent::Node;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestNodeDeactivationState
{
protected:
    TestNodeDeactivationState()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestNodeDeactivationState()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void Start(NodeDeactivationInfo const & newInfo)
    {
        state_->TryStartChange(L"", newInfo);
    }

    void StartAndVerifyState(NodeDeactivationInfo const & newInfo, NodeDeactivationInfo const & expected)
    {
        state_->TryStartChange(L"", newInfo);
        NodeDeactivationInfo actual = state_->DeactivationInfo;

        Verify::AreEqualLogOnError(expected.IsActivated, actual.IsActivated, L"IsActivated");
        Verify::AreEqualLogOnError(expected.SequenceNumber, actual.SequenceNumber, L"SequenceNumber");
        Verify::AreEqualLogOnError(state_->IsActivated, expected.IsActivated, L"IsActivated Property");
    }

    void StartAndVerifyReturnValue(NodeDeactivationInfo const & newInfo, bool expected)
    {
        auto actual = state_->TryStartChange(L"", newInfo);

        Verify::AreEqualLogOnError(expected, actual, L"Return Value from TryStart");
    }

    AsyncOperationSPtr StartAndFinish(NodeDeactivationInfo const & activationInfo)
    {
        Start(activationInfo);
        return Finish(activationInfo);
    }

    AsyncOperationSPtr Finish(NodeDeactivationInfo const & activationInfo)
    {
        state_->FinishChange(L"A", activationInfo, EntityEntryBaseList());
        return state_->Test_GetAsyncOp();
    }

    void StartFinishAndVerify(NodeDeactivationInfo const & activationInfo, AsyncOperationSPtr prevOp, bool expectPrevIsNull, bool expectCurrentIsNull)
    {
        Start(activationInfo);
        FinishAndVerify(activationInfo, prevOp, expectPrevIsNull, expectCurrentIsNull);
    }

    void FinishAndVerify(NodeDeactivationInfo const & activationInfo, AsyncOperationSPtr prevOp, bool expectPrevIsNull, bool expectCurrentIsNull)
    {
        auto current = Finish(activationInfo);

        if (expectPrevIsNull)
        {
            Verify::IsTrueLogOnError(prevOp == nullptr, L"Previous is null");
        }
        else
        {
            VERIFY_IS_NOT_NULL(prevOp);
            Verify::IsTrueLogOnError(prevOp->IsCancelRequested, L"Prev op cancelled");
        }

        Verify::AreEqualLogOnError(expectCurrentIsNull, current == nullptr, L"current is null");
    }

    void Close()
    {
        state_->Close();
    }

    void SetNodeUpAckNotProcessed()
    {
        utContext_->RA.StateObj.Test_SetNodeUpAckFromFMProcessed(false);
    }

private:
    NodeDeactivationStateUPtr state_;
    UnitTestContextUPtr utContext_;
};

bool TestNodeDeactivationState::TestSetup()
{
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::StubJobQueueManager | UnitTestContext::Option::TestAssertEnabled);
    utContext_->RA.StateObj.Test_SetNodeUpAckFromFMProcessed(true);
    state_ = make_unique<NodeDeactivationState>(*FailoverManagerId::Fm, utContext_->RA);
    return true;
}

bool TestNodeDeactivationState::TestCleanup()
{
    Close();
    return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestNodeDeactivationStateSuite, TestNodeDeactivationState)

BOOST_AUTO_TEST_CASE(Start_HigherInstanceIdActivates)
{
    StartAndVerifyState(NodeDeactivationInfo(true, 3), NodeDeactivationInfo(true, 3));
}

BOOST_AUTO_TEST_CASE(Start_SameInstanceIdDoesNotActivate)
{
    Start(NodeDeactivationInfo(true, 3));

    StartAndVerifyState(NodeDeactivationInfo(true, 3), NodeDeactivationInfo(true, 3));
}

BOOST_AUTO_TEST_CASE(Start_LowerInstanceIdDoesNotChange)
{
    Start(NodeDeactivationInfo(true, 3));

    StartAndVerifyState(NodeDeactivationInfo(true, 2), NodeDeactivationInfo(true, 3));
}

BOOST_AUTO_TEST_CASE(Start_MultipleActivationWithHigherInstanceId)
{
    Start(NodeDeactivationInfo(true, 3));

    StartAndVerifyState(NodeDeactivationInfo(false, 4), NodeDeactivationInfo(false, 4));
}

BOOST_AUTO_TEST_CASE(Start_HigherInstanceReturnsTrue_DifferentIntent)
{
    Start(NodeDeactivationInfo(true, 4));

    StartAndVerifyReturnValue(NodeDeactivationInfo(false, 5), true);
}

BOOST_AUTO_TEST_CASE(Start_HigherInstanceReturnsTrue_SameIntent)
{
    Start(NodeDeactivationInfo(true, 4));

    StartAndVerifyReturnValue(NodeDeactivationInfo(true, 5), true);
}

BOOST_AUTO_TEST_CASE(Start_EqualInstanceReturnsFalseIfPreviouslyDeactivatedScenario1)
{
    Start(NodeDeactivationInfo(false, 4));

    StartAndVerifyReturnValue(NodeDeactivationInfo(true, 4), false);
}

BOOST_AUTO_TEST_CASE(Start_EqualInstanceReturnsFalseIfPreviouslyDeactivatedScenario2)
{
    Start(NodeDeactivationInfo(false, 4));

    StartAndVerifyReturnValue(NodeDeactivationInfo(false, 4), false);
}

BOOST_AUTO_TEST_CASE(Start_LowerInstanceReturnsFalse)
{
    Start(NodeDeactivationInfo(true, 4));

    StartAndVerifyReturnValue(NodeDeactivationInfo(true, 3), false);
}

BOOST_AUTO_TEST_CASE(Start_HigherInstanceButWithNodeUpAckNotProcessedUpdatesState)
{
    SetNodeUpAckNotProcessed();

    auto value = NodeDeactivationInfo(true, 100);
    StartAndVerifyState(value, value);
}

BOOST_AUTO_TEST_CASE(Start_MultipleCallsWithNodeUpAckNotProcessed)
{
    // consider node up ack for fmm arrives with true, 4. then node up ack for fm true, 2. true, 2 will also try to update which should fail
    SetNodeUpAckNotProcessed();

    auto initial = NodeDeactivationInfo(true, 4);
    Start(initial);

    StartAndVerifyState(NodeDeactivationInfo(true, 2), initial);
}

BOOST_AUTO_TEST_CASE(Start_NodeUpAckNotProcessedReturnsFalse)
{
    SetNodeUpAckNotProcessed();

    StartAndVerifyReturnValue(NodeDeactivationInfo(true, 4), false);
}

BOOST_AUTO_TEST_CASE(Finish_Deactivated_Deactivate_MatchingInstance_StartsCloseOperation)
{
    auto firstOp = StartAndFinish(NodeDeactivationInfo(false, 3));

    StartFinishAndVerify(NodeDeactivationInfo(false, 5), firstOp, false, false);
}

BOOST_AUTO_TEST_CASE(Finish_Activated_Deactivate_MatchingInstance_StartsCloseOperation)
{
    auto firstOp = StartAndFinish(NodeDeactivationInfo(true, 3));

    StartFinishAndVerify(NodeDeactivationInfo(false, 5), firstOp, true, false);
}

BOOST_AUTO_TEST_CASE(Finish_Deactivated_Activate_MatchingInstance_CancelClose)
{
    auto firstOp = StartAndFinish(NodeDeactivationInfo(false, 3));

    StartFinishAndVerify(NodeDeactivationInfo(true, 5), firstOp, false, true);
}

BOOST_AUTO_TEST_CASE(Finish_Activated_Activate_MatchingInstance_IsNoOp)
{
    auto firstOp = StartAndFinish(NodeDeactivationInfo(true, 3));

    StartFinishAndVerify(NodeDeactivationInfo(true, 5), firstOp, true, true);
}

BOOST_AUTO_TEST_CASE(Finish_LowerInstance_Activate_Ignored)
{
    auto firstOp = StartAndFinish(NodeDeactivationInfo(false, 3));

    auto current = Finish(NodeDeactivationInfo(true, 2));
    VERIFY_ARE_EQUAL(firstOp, current);

    Verify::IsTrueLogOnError(!current->IsCancelRequested, L"Should not be cancelled by lower");
}

BOOST_AUTO_TEST_CASE(Finish_LowerInstance_Deactivate_Ignored)
{
    StartAndFinish(NodeDeactivationInfo(true, 3));

    auto current = Finish(NodeDeactivationInfo(false, 2));
    Verify::IsTrueLogOnError(current == nullptr, L"should not have started async op");
}

BOOST_AUTO_TEST_CASE(Close_CancelsOperation)
{
    auto op = StartAndFinish(NodeDeactivationInfo(false, 4));

    Close();

    Verify::IsTrueLogOnError(op->IsCancelRequested, L"Is cancelled");
}

BOOST_AUTO_TEST_CASE(Close_WhenActivated)
{
    StartAndFinish(NodeDeactivationInfo(true, 4));

    Close();
}

BOOST_AUTO_TEST_CASE(Close_AfterActivateStart_ButBeforeFinish)
{
    Start(NodeDeactivationInfo(true, 1));

    Close();
}

BOOST_AUTO_TEST_CASE(Close_AfterDeactivateStart_ButBeforeFinish)
{
    Start(NodeDeactivationInfo(false, 1));

    Close();
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
