// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Common;
using namespace std;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;

namespace
{
    const bool FMSource = false;
    const bool FMMSource = true;

    FailoverUnitId GetFTId(bool isFmm)
    {
        if (isFmm)
        {
            return FailoverUnitId(Reliability::Constants::FMServiceGuid);
        }
        else
        {
            return FailoverUnitId(Guid(L"0D72FA69-728D-4794-8AEA-0343FC3213A3"));
        }
    }
}

class TestNodeActivationState
{
protected:
    void SanityTestHelper(bool isFmm);
    TestNodeActivationState() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD(StaleDeactivateDoesNotStartNewCheck);
    TEST_METHOD(StaleActivateDoesNotCancelExistingCheck);

    TEST_METHOD_SETUP(TestSetup);
    ~TestNodeActivationState() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void Deactivate(bool isFmm, int64 sequenceNumber)
    {
        bool expectedReturnValue = false;
        NodeActivationState::Enum expectedIsActivated = NodeActivationState::Deactivated;
        int64 expectedSequenceNumber = 0;

        ChangeActivationState(isFmm, NodeActivationState::Deactivated, sequenceNumber, expectedReturnValue, expectedIsActivated, expectedSequenceNumber, true);
    }

    void Activate(bool isFmm, int64 sequenceNumber)
    {
        bool expectedReturnValue = false;
        NodeActivationState::Enum expectedIsActivated = NodeActivationState::Deactivated;
        int64 expectedSequenceNumber = 0;

        ChangeActivationState(isFmm, NodeActivationState::Activated, sequenceNumber, expectedReturnValue, expectedIsActivated, expectedSequenceNumber, true);
    }

    void ValidateAsyncOperation(bool isFmm, bool isNull, bool isRunning)
    {
        auto op = GetOperation(isFmm);

        Verify::AreEqualLogOnError(isNull, op == nullptr, L"AsyncOperation is null");

        if (op != nullptr)
        {
            Verify::AreEqualLogOnError(isRunning, !op->IsCompleted, L"IsRunning");
        }
    }

    void StartCloseMonitoring(bool isFmm, int64 sequenceNumber)
    {
        EntityEntryBaseList v;
        v.push_back(testContext_->GetLFUMEntry(L"SP1"));
        state_->StartReplicaCloseOperation(L"a", isFmm, sequenceNumber, move(v));
    }

    void CancelCloseMonitoring(bool isFmm, int64 sequenceNumber)
    {
        state_->CancelReplicaCloseOperation(L"a", isFmm, sequenceNumber);
    }

    void Deactivate(bool isFmm, int64 sequenceNumber, bool expectedReturnValue, NodeActivationState::Enum expectedIsActivated, int64 expectedSequenceNumber)
    {
        ChangeActivationState(isFmm, NodeActivationState::Deactivated, sequenceNumber, expectedReturnValue, expectedIsActivated, expectedSequenceNumber, false);
    }

    void Activate(bool isFmm, int64 sequenceNumber, bool expectedReturnValue, NodeActivationState::Enum expectedIsActivated, int64 expectedSequenceNumber)
    {
        ChangeActivationState(isFmm, NodeActivationState::Activated, sequenceNumber, expectedReturnValue, expectedIsActivated, expectedSequenceNumber, false);
    }

    AsyncOperationSPtr GetOperation(bool isFmm)
    {
        return state_->Test_GetReplicaCloseMonitorAsyncOperation(isFmm);
    }

    void ChangeActivationState(bool isFmm, NodeActivationState::Enum state, int64 sequenceNumber, bool expectedReturnValue, NodeActivationState::Enum expectedState, int64 expectedSequenceNumber, bool skipVerify);

    void ValidateActivated(bool isFmm, int64 currentSequenceNumber);
    void ValidateDeactivated(bool isFmm, int64 currentSequenceNumber);

    ScenarioTestUPtr testContext_;
    NodeActivationStateUPtr state_;
};

bool TestNodeActivationState::TestSetup()
{
    testContext_ = ScenarioTest::Create();
    testContext_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    state_ = make_unique<NodeActivationState>(testContext_->RA);
    return true;
}

void VerifyOp(AsyncOperationSPtr const & op)
{
    if (op == nullptr)
    {
        return;
    }

    auto casted = static_pointer_cast<MultipleReplicaCloseCompletionCheckAsyncOperation>(op);
    Verify::IsTrue(casted->Test_IsInternalCompleted, L"Async op not completed");
}

bool TestNodeActivationState::TestCleanup()
{
    state_->Close();

    VerifyOp(state_->Test_GetReplicaCloseMonitorAsyncOperation(true));
    VerifyOp(state_->Test_GetReplicaCloseMonitorAsyncOperation(false));

    state_.reset();
    return testContext_->Cleanup();
}

void TestNodeActivationState::ChangeActivationState(bool isFmm, NodeActivationState::Enum state, int64 sequenceNumber, bool expectedReturnValue, NodeActivationState::Enum expectedState, int64 expectedSequenceNumber, bool skipVerify)
{
    NodeActivationState::Enum actualState; int64 actualSequenceNumber;
    auto rv = state_->TryChangeActivationStatus(isFmm, state, sequenceNumber, actualState, actualSequenceNumber);

    if (!skipVerify)
    {
        VERIFY_ARE_EQUAL(expectedReturnValue, rv);
        VERIFY_ARE_EQUAL(expectedState, actualState);
        VERIFY_ARE_EQUAL(expectedSequenceNumber, actualSequenceNumber);
    }
}

void TestNodeActivationState::ValidateActivated(bool isFmm, int64 expectedCurrentSequenceNumber)
{
    VERIFY_IS_TRUE(state_->IsCurrent(GetFTId(isFmm), expectedCurrentSequenceNumber));
    VERIFY_IS_TRUE(state_->IsActivated(isFmm));
    VERIFY_IS_TRUE(state_->IsActivated(GetFTId(isFmm)));
}

void TestNodeActivationState::ValidateDeactivated(bool isFmm, int64 expectedCurrentSequenceNumber)
{
    VERIFY_IS_TRUE(state_->IsCurrent(GetFTId(isFmm), expectedCurrentSequenceNumber));
    VERIFY_IS_FALSE(state_->IsActivated(isFmm));
    VERIFY_IS_FALSE(state_->IsActivated(GetFTId(isFmm)));
}

void TestNodeActivationState::SanityTestHelper(bool isFmm)
{
    // Deactivation with higher seq num
    Deactivate(isFmm, 1, true, NodeActivationState::Deactivated, 1);
    ValidateDeactivated(isFmm, 1);

    // Activation with higher sequence number
    Activate(isFmm, 2, true, NodeActivationState::Activated, 2);
    ValidateActivated(isFmm, 2);
        
    // Activate again with higher sequence number
    Activate(isFmm, 3, true, NodeActivationState::Activated, 3);
    ValidateActivated(isFmm, 3);

    // activate with 2 should not reduce seq num
    Activate(isFmm, 2, false, NodeActivationState::Activated, 3);
    ValidateActivated(isFmm, 3);

    // Activate with 3 is also stale
    Activate(isFmm, 3, false, NodeActivationState::Activated, 3);
    ValidateActivated(isFmm, 3);

    // Deactivate with 2 should not work
    Deactivate(isFmm, 2, false, NodeActivationState::Activated, 3);
    ValidateActivated(isFmm, 3);

    // Deactivate with 4 should work
    Deactivate(isFmm, 4, true, NodeActivationState::Deactivated, 4);
    ValidateDeactivated(isFmm, 4);

    // Deactivate again with 4 is stale
    Deactivate(isFmm, 4, false, NodeActivationState::Deactivated, 4);
    ValidateDeactivated(isFmm, 4);

    // Deactivate with 3 should not work
    Deactivate(isFmm, 3, false, NodeActivationState::Deactivated, 4);
    ValidateDeactivated(isFmm, 4);
}

BOOST_FIXTURE_TEST_SUITE(TestNodeActivationStateSuite, TestNodeActivationState)

BOOST_AUTO_TEST_CASE(NodeIsInitiallyActivated)
{
	bool currentState; int64 currentSeqNum;
	state_->Test_GetStatus(false, currentState, currentSeqNum);

	VERIFY_ARE_EQUAL(Reliability::Constants::InvalidNodeActivationSequenceNumber, currentSeqNum);
	VERIFY_IS_TRUE(currentState);

	state_->Test_GetStatus(true, currentState, currentSeqNum);

	VERIFY_ARE_EQUAL(Reliability::Constants::InvalidNodeActivationSequenceNumber, currentSeqNum);
	VERIFY_IS_TRUE(currentState);
}

BOOST_AUTO_TEST_CASE(SanityTest_FM)
{
	SanityTestHelper(false);
}

BOOST_AUTO_TEST_CASE(SanityTest_FMM)
{
	SanityTestHelper(true);
}

BOOST_AUTO_TEST_CASE(MultipleDeactivateCauseNewWork)
{
    Deactivate(false, 1, true, NodeActivationState::Deactivated, 1);
    Deactivate(false, 2, true, NodeActivationState::Deactivated, 2);
}

BOOST_AUTO_TEST_CASE(MultipleActivateCauseNewWork)
{
    Deactivate(false, 1, true, NodeActivationState::Deactivated, 1);
    Activate(false, 2, true, NodeActivationState::Activated, 2);
    Activate(false, 3, true, NodeActivationState::Activated, 3);
}

BOOST_AUTO_TEST_CASE(StaleDeactivateDoesNotStartNewCheck)
{
    bool isFmm = true;

    Deactivate(isFmm, 4);
    StartCloseMonitoring(isFmm, 4);
    
    auto op = GetOperation(isFmm);

    StartCloseMonitoring(isFmm, 3);

    ValidateAsyncOperation(isFmm, false, true);

    Verify::AreEqual(op.get(), GetOperation(isFmm).get(), L"AsyncOp must not change");

    testContext_->DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(StaleActivateDoesNotCancelExistingCheck)
{
    bool isFmm = true;

    Deactivate(isFmm, 4);
    StartCloseMonitoring(isFmm, 4);

    Activate(isFmm, 3);
    CancelCloseMonitoring(isFmm, 3);

    ValidateAsyncOperation(isFmm, false, true);

    testContext_->DrainJobQueues();
}

BOOST_AUTO_TEST_SUITE_END()
