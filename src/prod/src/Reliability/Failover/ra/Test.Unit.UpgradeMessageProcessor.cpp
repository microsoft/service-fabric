// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;

namespace
{
    const wstring App1Name(L"foo");
}

class CancelUpgradeStub : public ICancelUpgradeContext
{
public:
    CancelUpgradeStub(uint64 instanceId) 
    : ICancelUpgradeContext(instanceId),
        sendReplyCount_(0)
    {
    }

    __declspec(property(get=get_SendReplyCount)) int SendReplyCount;
    int get_SendReplyCount() const { return sendReplyCount_; }

    void SendReply()
    {
        sendReplyCount_++;
    }

private:
    int sendReplyCount_;
};

class TestUpgradeMessageProcessor
{
protected:
    TestUpgradeMessageProcessor() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestUpgradeMessageProcessor() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);
       
    void CancelWithInstanceIdAfterCancelSendsReplyTestHelper(uint64 initialInstanceId, uint64 newInstanceId);
    void UpgradeDropTestHelper(uint64 upgradeInstanceId, uint64 cancelInstanceId);
    void UpgradeProcessedAfterCancelTestHelper(uint64 upgradeInstanceId, uint64 cancelInstanceId);        
    void CancelWhenUpgradeIsQueuedIsDroppedTestHelper(uint64 initialUpgradeId, uint64 queuedUpgradeId, uint64 cancelUpgradeId);
    void CancelWithHigherInstanceSendsReplyTestHelper(UpgradeStateMachineSPtr const & initialUpgrade);
    UpgradeStateMachineSPtr CreateStateMachineWithSingleAction(
        uint64 instanceId, 
        UpgradeStateDescriptionUPtr && state,
        UpgradeStubBase::EnterStateFunctionPtr enterFunction, 
        UpgradeStubBase::ResendReplyFunctionPtr resendReply = nullptr);

    UpgradeStateMachineSPtr CreateStateMachineWithSingleAction(
        uint64 instanceId, 
        UpgradeStubBase::EnterStateFunctionPtr enterFunction, 
        UpgradeStubBase::ResendReplyFunctionPtr resendReply = nullptr);

    UpgradeStateMachineSPtr CreateStateMachineWithSingleSyncActionThatGoesToCompleted(uint64 instanceId);
    UpgradeStateMachineSPtr CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(uint64 instanceId);
    ICancelUpgradeContextSPtr CreateCancelUpgradeContext(uint64 instanceId);

    void VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(UpgradeStateMachine* expectedSM);
    void VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(UpgradeStateMachine* expectedSM, UpgradeStateMachine * queuedSM);
    void VerifyUpgradeMapContainsSingleApplicationWithCancellation(ICancelUpgradeContext * cancelContext);
    void VerifyUpgradeMapContainsSingleApplicationWithQueuedCancellation(UpgradeStateMachine* expectedSM, ICancelUpgradeContext * cancelContext);

    void VerifyUpgradeMapContainsSingleApplication(UpgradeStateMachine * expectedSM, UpgradeStateMachine* queuedSM, ICancelUpgradeContext * cancelContext);

    void VerifyUpgradeMapIsEmpty();

    void VerifyUpgradeWasNotStartedAndClosed(UpgradeStateMachineSPtr const & upgrade);
    void VerifyUpgradeWasCompletedAndClosed(UpgradeStateMachineSPtr const & upgrade);
    void VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::Enum state, UpgradeStateMachineSPtr const & upgrade);
    void VerifyUpgradeIsInExpectedStateAndOpen(UpgradeStateName::Enum state, UpgradeStateMachineSPtr const & upgrade);

    void ProcessUpgradeAndAssertHelper(wstring const & appId, UpgradeStateMachineSPtr const & upgrade, bool expected);
    void ProcessUpgradeAndAssertNotStarted(wstring const & appId, UpgradeStateMachineSPtr const & upgrade);
    void ProcessUpgradeAndAssertStarted(wstring const & appId, UpgradeStateMachineSPtr const & upgrade);

    UpgradeStubBase & GetStub(UpgradeStateMachineSPtr const & sm)
    {
        return dynamic_cast<UpgradeStubBase&>(sm->Upgrade);
    }

    void VerifyStateMachineReceivedNoRollbackSnapshot(UpgradeStateMachineSPtr const & sm)
    {
        Verify::IsTrue(GetStub(sm).RollbackSnapshotPassedAtStart == nullptr, L"RollbackSnapshotAtStart: IsNull");
    }

    void VerifyStateMachineReceivedCorrectRollbackSnapshot(UpgradeStateMachineSPtr const & sm, UpgradeStateName::Enum expectedState)
    {
        auto snapshot = GetStub(sm).RollbackSnapshotPassedAtStart;
        if (snapshot == nullptr)
        {
            Verify::Fail(L"Expected snapshot");
            return;
        }

        Verify::AreEqualLogOnError(expectedState, snapshot->State, L"Snapshot: Expected State");
        Verify::AreEqualLogOnError(UpgradeStubBase::DefaultSnapshotWereReplicasClosed, snapshot->WereReplicasClosed, L"Snapshot: WereReplicasClosed");
    }
        
    void VerifySendReplyExecutedOnce(ICancelUpgradeContext & context)
    {
        Verify::AreEqual(1, dynamic_cast<CancelUpgradeStub&>(context).SendReplyCount, L"SendReplyExecutedOnce");
    }

    void VerifySendReplyNotExecuted(ICancelUpgradeContext & context)
    {
        Verify::AreEqual(0, dynamic_cast<CancelUpgradeStub&>(context).SendReplyCount, L"SendReplyNotExecuted");
    }

    void VerifySendReplyExecutedOnce(ICancelUpgradeContextSPtr const & context)
    {
        VerifySendReplyExecutedOnce(*context);
    }

    void VerifySendReplyNotExecuted(ICancelUpgradeContextSPtr const & context)
    {
        VerifySendReplyNotExecuted(*context);
    }

    void VerifyIsInProgress(wstring const & appName, bool expected)
    {
        Verify::AreEqual(expected, upgradeProcessor_->IsUpgrading(appName), L"IsUpgrading");
    }

    void Close()
    {
        upgradeProcessor_->Close();
    }

    ICancelUpgradeContextSPtr CreateAndProcessCancelUpgradeContext(wstring const & appId, uint64 instanceId);

    ScenarioTestUPtr scenarioTest_;
    UpgradeMessageProcessorUPtr upgradeProcessor_;
        
};

UpgradeStateMachineSPtr TestUpgradeMessageProcessor::CreateStateMachineWithSingleAction(
    uint64 instanceId,
    UpgradeStateDescriptionUPtr && state,
    UpgradeStubBase::EnterStateFunctionPtr enterFunction,
    UpgradeStubBase::ResendReplyFunctionPtr resendReply)
{
    UpgradeStubBase * upgrade = new UpgradeStubBase(scenarioTest_->RA, instanceId, move(state));
    upgrade->EnterStateFunction = enterFunction;
    upgrade->ResendReplyFunction = resendReply;

    return make_shared<UpgradeStateMachine>(IUpgradeUPtr(upgrade), scenarioTest_->RA);
}

UpgradeStateMachineSPtr TestUpgradeMessageProcessor::CreateStateMachineWithSingleAction(
    uint64 instanceId, 
    UpgradeStubBase::EnterStateFunctionPtr enterFunction, 
    UpgradeStubBase::ResendReplyFunctionPtr resendReply)
{
    return CreateStateMachineWithSingleAction(
        instanceId,
        CreateState(UpgradeStateName::Test1),
        enterFunction,
        resendReply);
}

bool TestUpgradeMessageProcessor::TestSetup()
{
	scenarioTest_ = ScenarioTest::Create();
	upgradeProcessor_ = make_unique<UpgradeMessageProcessor>(scenarioTest_->RA);
	return true;
}

bool TestUpgradeMessageProcessor::TestCleanup()
{
	if (upgradeProcessor_ != nullptr)
	{
		Close();
		upgradeProcessor_.reset();
	}

	if (scenarioTest_ != nullptr)
	{
		scenarioTest_->Cleanup();
		scenarioTest_.reset();
	}

	return true;
}

ICancelUpgradeContextSPtr TestUpgradeMessageProcessor::CreateAndProcessCancelUpgradeContext(wstring const & appId, uint64 instanceId)
{
	auto ptr = CreateCancelUpgradeContext(instanceId);

	upgradeProcessor_->ProcessCancelUpgradeMessage(appId, ptr);

	return ptr;
}

void TestUpgradeMessageProcessor::ProcessUpgradeAndAssertHelper(wstring const & appId, UpgradeStateMachineSPtr const & upgrade, bool expected)
{
	bool rv = upgradeProcessor_->ProcessUpgradeMessage(appId, upgrade);
	Verify::AreEqual(expected, rv, L"UpgradeStarted");
}

void TestUpgradeMessageProcessor::ProcessUpgradeAndAssertNotStarted(wstring const & appId, UpgradeStateMachineSPtr const & upgrade)
{
	ProcessUpgradeAndAssertHelper(appId, upgrade, false);
	VerifyUpgradeWasNotStartedAndClosed(upgrade);
}

void TestUpgradeMessageProcessor::ProcessUpgradeAndAssertStarted(wstring const & appId, UpgradeStateMachineSPtr const & upgrade)
{
	ProcessUpgradeAndAssertHelper(appId, upgrade, true);
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(UpgradeStateMachine* expectedSM)
{
	VerifyUpgradeMapContainsSingleApplication(expectedSM, nullptr, nullptr);
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(UpgradeStateMachine* expectedSM, UpgradeStateMachine * queuedSM)
{
	VerifyUpgradeMapContainsSingleApplication(expectedSM, queuedSM, nullptr);
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapContainsSingleApplicationWithCancellation(ICancelUpgradeContext * cancelContext)
{
	VerifyUpgradeMapContainsSingleApplication(nullptr, nullptr, cancelContext);
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapContainsSingleApplicationWithQueuedCancellation(UpgradeStateMachine * expectedSM, ICancelUpgradeContext * cancelContext)
{
	VerifyUpgradeMapContainsSingleApplication(expectedSM, nullptr, cancelContext);
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapContainsSingleApplication(UpgradeStateMachine * expectedSM, UpgradeStateMachine* queuedSM, ICancelUpgradeContext * cancelContext)
{
	Verify::AreEqual(1u, upgradeProcessor_->Test_UpgradeMap.size(), L"VerifyUpgradeMapContainsSingleApplication: UpgradeMapSize");
	Verify::AreEqual(App1Name, upgradeProcessor_->Test_UpgradeMap.cbegin()->first, L"VerifyUpgradeMapContainsSingleApplication: AppName");
	Verify::AreEqual(expectedSM, upgradeProcessor_->Test_UpgradeMap.cbegin()->second.Current.get(), L"VerifyUpgradeMapContainsSingleApplication: Expected SM");
	Verify::AreEqual(queuedSM, upgradeProcessor_->Test_UpgradeMap.cbegin()->second.Queued.get(), L"VerifyUpgradeMapContainsSingleApplication: Queued SM");
	Verify::AreEqual(cancelContext, upgradeProcessor_->Test_UpgradeMap.cbegin()->second.CancellationContext.get(), L"VerifyUpgradeMapContainsSingleApplication: Cancel");
}

void TestUpgradeMessageProcessor::VerifyUpgradeMapIsEmpty()
{
	Verify::IsTrue(upgradeProcessor_->Test_UpgradeMap.empty());
}

void TestUpgradeMessageProcessor::VerifyUpgradeIsInExpectedStateAndOpen(UpgradeStateName::Enum state, UpgradeStateMachineSPtr const & upgrade)
{
	Verify::AreEqual(state, upgrade->CurrentState);
	Verify::IsTrue(!upgrade->IsClosed);
}

void TestUpgradeMessageProcessor::VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::Enum state, UpgradeStateMachineSPtr const & upgrade)
{
	Verify::AreEqual(state, upgrade->CurrentState);
	Verify::IsTrue(upgrade->IsClosed);
}

void TestUpgradeMessageProcessor::VerifyUpgradeWasNotStartedAndClosed(UpgradeStateMachineSPtr const & upgrade)
{
	VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::None, upgrade);
}

void TestUpgradeMessageProcessor::VerifyUpgradeWasCompletedAndClosed(UpgradeStateMachineSPtr const & upgrade)
{
	VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::Completed, upgrade);
}

ICancelUpgradeContextSPtr TestUpgradeMessageProcessor::CreateCancelUpgradeContext(uint64 instanceId)
{
	return make_shared<CancelUpgradeStub>(instanceId);
}

UpgradeStateMachineSPtr TestUpgradeMessageProcessor::CreateStateMachineWithSingleSyncActionThatGoesToCompleted(uint64 instanceId)
{
	return CreateStateMachineWithSingleAction(instanceId, [](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
	{
		return UpgradeStateName::Completed;
	});
}

UpgradeStateMachineSPtr TestUpgradeMessageProcessor::CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(uint64 instanceId)
{
	return CreateStateMachineWithSingleAction(instanceId, [](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
	{
		return UpgradeStateName::Invalid;
	});
}

void TestUpgradeMessageProcessor::CancelWithInstanceIdAfterCancelSendsReplyTestHelper(uint64 initialInstanceId, uint64 newInstanceId)
{
	auto initial = CreateAndProcessCancelUpgradeContext(App1Name, initialInstanceId);
	auto next = CreateAndProcessCancelUpgradeContext(App1Name, newInstanceId);

	VerifyUpgradeMapContainsSingleApplicationWithCancellation(next.get());
	VerifySendReplyExecutedOnce(next);
}

void TestUpgradeMessageProcessor::UpgradeDropTestHelper(uint64 upgradeInstanceId, uint64 cancelInstanceId)
{
	auto cancelRequest = CreateAndProcessCancelUpgradeContext(App1Name, cancelInstanceId);

	auto upgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(upgradeInstanceId);

	ProcessUpgradeAndAssertNotStarted(App1Name, upgrade);

	VerifyUpgradeMapContainsSingleApplicationWithCancellation(cancelRequest.get());
}

void TestUpgradeMessageProcessor::UpgradeProcessedAfterCancelTestHelper(uint64 upgradeInstanceId, uint64 cancelInstanceId)
{
	auto cancelRequest = CreateAndProcessCancelUpgradeContext(App1Name, cancelInstanceId);

	auto upgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(upgradeInstanceId);

	ProcessUpgradeAndAssertStarted(App1Name, upgrade);

	VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(upgrade.get());
}

void TestUpgradeMessageProcessor::CancelWhenUpgradeIsQueuedIsDroppedTestHelper(uint64 initialUpgradeId, uint64 queuedUpgradeId, uint64 cancelUpgradeId)
{
	// this upgrade will be stuck
	auto initialUpgrade = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(initialUpgradeId);

	auto queuedUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(queuedUpgradeId);

	ProcessUpgradeAndAssertStarted(App1Name, initialUpgrade);
	ProcessUpgradeAndAssertStarted(App1Name, queuedUpgrade);

	// At this time queued Upgrade is queued
	auto cancelRequest = CreateAndProcessCancelUpgradeContext(App1Name, cancelUpgradeId);

	VerifyUpgradeIsInExpectedStateAndOpen(UpgradeStateName::Test1, initialUpgrade);
	VerifyUpgradeIsInExpectedStateAndOpen(UpgradeStateName::None, queuedUpgrade);

	VerifySendReplyNotExecuted(cancelRequest);

	VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(initialUpgrade.get(), queuedUpgrade.get());
}

void TestUpgradeMessageProcessor::CancelWithHigherInstanceSendsReplyTestHelper(UpgradeStateMachineSPtr const & initialUpgrade)
{
	uint64 cancelInstance = initialUpgrade->InstanceId + 1;

	ProcessUpgradeAndAssertStarted(App1Name, initialUpgrade);

	auto finalState = initialUpgrade->CurrentState;

	// Now process a cancel with higher instance
	auto cancelContext = CreateAndProcessCancelUpgradeContext(App1Name, cancelInstance);

	// Verify the reply was sent
	VerifySendReplyExecutedOnce(cancelContext);

	// Verify that the upgrade processor contains a cancelled state
	VerifyUpgradeMapContainsSingleApplicationWithCancellation(cancelContext.get());
	VerifyUpgradeIsInExpectedStateAndClosed(finalState, initialUpgrade);
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestUpgradeMessageProcessorSuite,TestUpgradeMessageProcessor)

BOOST_AUTO_TEST_CASE(IsUpgrading_NotFoundEntry)
{
    VerifyIsInProgress(App1Name, false);
}

BOOST_AUTO_TEST_CASE(IsUpgrading_Completed)
{
    auto sm = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(1);

    upgradeProcessor_->ProcessUpgradeMessage(App1Name, sm);

    VerifyIsInProgress(App1Name, false);
}

BOOST_AUTO_TEST_CASE(IsUpgrading_AfterCancel)
{
    CreateAndProcessCancelUpgradeContext(App1Name, 1);

    VerifyIsInProgress(App1Name, false);
}

BOOST_AUTO_TEST_CASE(IsUpgrading_InProgress)
{
    AsyncStateActionCompleteCallback callback;
    auto upgrade = CreateStateMachineWithSingleAction(1, [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
    {
        callback = inner;
        return UpgradeStateName::Invalid;
    });

    ProcessUpgradeAndAssertStarted(App1Name, upgrade);

    VerifyIsInProgress(App1Name, true);
}

BOOST_AUTO_TEST_CASE(IsUpgrading_InProgress_Closed)
{
    auto upgrade = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(1);

    ProcessUpgradeAndAssertStarted(App1Name, upgrade);

    Close();

    VerifyIsInProgress(App1Name, false);
}

BOOST_AUTO_TEST_CASE(NewUpgradesAreAddedAndStarted)
{
    auto sm = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(1);

    upgradeProcessor_->ProcessUpgradeMessage(App1Name, sm);

    Verify::AreEqual(UpgradeStateName::Completed, sm->CurrentState);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(sm.get());
    VerifyStateMachineReceivedNoRollbackSnapshot(sm);
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithSmallerInstanceIdIsIgnored)
{
    auto temp = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(2);
    auto initialUpgrade = temp;
        
    // Start the first upgrade
    upgradeProcessor_->ProcessUpgradeMessage(App1Name, initialUpgrade);

    // Now another upgrade comes for the same app but with less instance id
    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(1);

    // send that upgrade to the processor
    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade);

    // Validate that the first upgrade is still executing
    Verify::AreEqual(UpgradeStateName::Test1, initialUpgrade->CurrentState);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(initialUpgrade.get());
}

BOOST_AUTO_TEST_CASE(CallingProcessUpgradeAfterCloseIsNoOpAndStateMachineIsClosed)
{
    auto sm = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(1);

    Close();

    ProcessUpgradeAndAssertNotStarted(App1Name, sm);

    VerifyUpgradeMapIsEmpty();
}

BOOST_AUTO_TEST_CASE(CloseClosesAllStateMachines)
{
    auto sm = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(1);
        
    // Start this upgrade
    upgradeProcessor_->ProcessUpgradeMessage(App1Name, sm);

    // at this point the only reference to the upgrade is in the upgrade message processor object itself
    // assert that the reference is still valid
    Verify::AreEqual(UpgradeStateName::Test1, sm->CurrentState);

    // close the processor
    Close();

    // validate that the upgrade sm has been transitioned to closed
    Verify::AreEqual(UpgradeStateName::Test1, sm->CurrentState);
    Verify::IsTrue(sm->IsClosed);

    VerifyUpgradeMapIsEmpty();
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithEqualInstanceIdCausesReplyToBeSentIfComplete)
{
    bool oldUpgradeReplySent = false;
    bool newUpgradeReplySent = false;

    auto sm = CreateStateMachineWithSingleAction(1,
                                                    [] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Completed; },
                                                    [&] () { oldUpgradeReplySent = true; });

    // Try to start a new upgrade with the same instance id
    // This should cause the message to be sent for the earlier upgrade
    auto newUpgrade = CreateStateMachineWithSingleAction(1,
                                                            [] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Completed; },
                                                            [&] () { newUpgradeReplySent = true; });

    // Start this upgrade
    ProcessUpgradeAndAssertStarted(App1Name, sm);

    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade);

    // the new upgrade should be closed
    Verify::AreEqual(UpgradeStateName::None, newUpgrade->CurrentState);
    Verify::IsTrue(newUpgrade->IsClosed);

    // The send reply action should have been executed
    Verify::IsTrue(newUpgradeReplySent, L"new upgrade reply sent");
    Verify::IsTrue(!oldUpgradeReplySent, L"old upgrade reply sent");

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(sm.get());
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithEqualInstanceIdDoesNotSendReplyIfIncomplete)
{
    bool oldUpgradeReplySent = false;
    bool newUpgradeReplySent = false;

    auto sm = CreateStateMachineWithSingleAction(1,
                                                    [] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Invalid; },
                                                    [&] () { oldUpgradeReplySent = true; });

    auto newUpgrade = CreateStateMachineWithSingleAction(1,
                                                            [] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Completed; },
                                                            [&] () { newUpgradeReplySent = true; });

    // Start this upgrade
    ProcessUpgradeAndAssertStarted(App1Name, sm);

    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade);

    // the new upgrade should be closed
    Verify::AreEqual(UpgradeStateName::None, newUpgrade->CurrentState);
    Verify::IsTrue(newUpgrade->IsClosed);

    // The send reply action should have been executed
    Verify::IsTrue(!newUpgradeReplySent, L"new upgrade reply sent");
    Verify::IsTrue(!oldUpgradeReplySent, L"old upgrade reply sent");

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(sm.get());
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithGreaterInstanceIdAndWithOldUpgradeCompleteStartsNewUpgradeAndIsNotARollback)
{
    auto oldUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(1);
    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);
    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);

    VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::Completed, oldUpgrade);

    // new upgrade should have been started (in this case completed because it is a single sync action)
    Verify::AreEqual(UpgradeStateName::Completed, newUpgrade->CurrentState);
    Verify::IsTrue(!newUpgrade->IsClosed);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(newUpgrade.get());

    VerifyStateMachineReceivedNoRollbackSnapshot(newUpgrade);
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithGreaterInstanceIdAndWithOldUpgradeImmediatelyCancellableCancelsOldAndStartsNewUpgrade)
{
    // cancellable because the old upgrade is in an async state where it can be immediately cancelled
    // this also tests that it gets closed properly
    auto state = CreateState(UpgradeStateName::Test1, UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);
        
    AsyncStateActionCompleteCallback callback;
    auto oldUpgrade = CreateStateMachineWithSingleAction(
        1, 
        move(state),
        [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
        {
            callback = inner;
            return UpgradeStateName::Invalid;
        });
                
    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    // now process the new upgrade with a greater instance id
    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);

    // the old upgrade should be closed
    VerifyUpgradeIsInExpectedStateAndClosed(UpgradeStateName::Test1, oldUpgrade);
        
    // new upgrade should have been started (in this case completed because it is a single sync action)
    Verify::AreEqual(UpgradeStateName::Completed, newUpgrade->CurrentState);
    Verify::IsTrue(!newUpgrade->IsClosed);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(newUpgrade.get());

    // Close the upgrade state machine
    TestCleanup();

    // Callback should be fine
    callback(UpgradeStateName::Completed);

    VerifyStateMachineReceivedCorrectRollbackSnapshot(newUpgrade, UpgradeStateName::Test1);
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithGreaterInstanceIdAndWithOldUpgradeNonCancellableIsQueuedAndResumed)
{
    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
    auto state = CreateState(UpgradeStateName::Test1, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);
    auto oldUpgrade = CreateStateMachineWithSingleAction(1, move(state), [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                callback = inner;
                                                                return UpgradeStateName::Invalid;
                                                            });

    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    // At this point the upgrade has been started - oldUpgrade

    // now process the new upgrade with a greater instance id
    // since the old upgrade is currently executing a callback should be queued for it
    // this should return true
    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);
    VerifyIsInProgress(App1Name, true);

    // the old upgrade should still be going on
    Verify::AreEqual(UpgradeStateName::Test1, oldUpgrade->CurrentState);
    Verify::IsTrue(oldUpgrade->IsCancelling);
        
    // new upgrade should not have been started yet
    Verify::AreEqual(UpgradeStateName::None, newUpgrade->CurrentState);
    Verify::IsTrue(!newUpgrade->IsClosed);
    Verify::IsTrue(!newUpgrade->IsCancelling);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(oldUpgrade.get(), newUpgrade.get());

    // Complete the state transition in the old upgrade
    callback(UpgradeStateName::Test2);

    // at this point the old upgrade should be closed
    // the new upgrade should have been started
    Verify::AreEqual(UpgradeStateName::Test1, oldUpgrade->CurrentState);
    Verify::IsTrue(oldUpgrade->IsClosed);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(newUpgrade.get());

    VerifyStateMachineReceivedCorrectRollbackSnapshot(newUpgrade, UpgradeStateName::Test1);

    VerifyIsInProgress(App1Name, false);
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithGreaterInstanceIdAndWithOldUpgradeNoRollbackAllowedIsDropped)
{
    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
    auto state = CreateState(UpgradeStateName::Test1, UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback);
    auto oldUpgrade = CreateStateMachineWithSingleAction(1, move(state), [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                callback = inner;
                                                                return UpgradeStateName::Invalid;
                                                            });

    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    // At this point the upgrade has been started - oldUpgrade

    // now process the new upgrade with a greater instance id
    // The old upgrade is in a state where it does not support rollback
    // The incoming upgrade should be dropped
    // It will be automatically retried by the FM
    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade);

    // the old upgrade should still be going on
    Verify::AreEqual(UpgradeStateName::Test1, oldUpgrade->CurrentState);

    callback(UpgradeStateName::Completed);
}

BOOST_AUTO_TEST_CASE(QueuedUpgradesAreClosedWhenStateMachineIsClosed)
{
    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
    auto oldUpgrade = CreateStateMachineWithSingleAction(1, [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                callback = inner;
                                                                return UpgradeStateName::Invalid;
                                                            });

    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);

    // Close the upgrade message processor
    Close();

    // Verify that the upgrades are closed
    VerifyUpgradeMapIsEmpty();

    Verify::IsTrue(oldUpgrade->IsClosed);
    Verify::IsTrue(newUpgrade->IsClosed);
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithEqualToQueuedInstanceIdIsIgnored)
{
    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
    auto oldUpgrade = CreateStateMachineWithSingleAction(1, [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                callback = inner;
                                                                return UpgradeStateName::Invalid;
                                                            });

    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);
    auto newUpgrade2 = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    // old upgrade will be started
    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    // new upgrade will be started
    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);

    // retry of queued upgrade will be ignored
    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade2);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(oldUpgrade.get(), newUpgrade.get());
}

BOOST_AUTO_TEST_CASE(UpgradeMessageWithGreaterThenCurrentButLessThanQueuedIsIgnored)
{
    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
    auto oldUpgrade = CreateStateMachineWithSingleAction(1, [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                callback = inner;
                                                                return UpgradeStateName::Invalid;
                                                            });

    // note: new upgrade 2 comes in with instance id 2 and the queued upgrade has instance id 3. 
    // newupgrade2 is a stale message
    auto newUpgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(3);
    auto newUpgrade2 = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    // old upgrade will be started
    ProcessUpgradeAndAssertStarted(App1Name, oldUpgrade);

    // new upgrade will be started
    ProcessUpgradeAndAssertStarted(App1Name, newUpgrade);

    // retry of queued upgrade will be ignored
    ProcessUpgradeAndAssertNotStarted(App1Name, newUpgrade2);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellationWithQueuedUpgrade(oldUpgrade.get(), newUpgrade.get());
}

BOOST_AUTO_TEST_CASE(MultipleUpgradesCanBeStartedAtTheSameTime)
{
    auto u1 = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(1);
    auto u2 = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(1);

    ProcessUpgradeAndAssertStarted(L"app1", u1);
    ProcessUpgradeAndAssertStarted(L"app2", u2);

    Verify::AreEqual(UpgradeStateName::Test1, u1->CurrentState);
    Verify::AreEqual(UpgradeStateName::Test1, u2->CurrentState);
}

BOOST_AUTO_TEST_CASE(CancelWithNoUpgradeSendsCancelReply)
{
    auto ptr = CreateAndProcessCancelUpgradeContext(App1Name, 1);

    VerifyUpgradeMapContainsSingleApplicationWithCancellation(ptr.get());

    VerifySendReplyExecutedOnce(ptr);
}

BOOST_AUTO_TEST_CASE(CancelWithLowerInstanceIdIsIgnoredAfterCancel)
{
    // initial with seq 4
    auto initial = CreateAndProcessCancelUpgradeContext(App1Name, 4);

    // another cancel message with seq 3
    auto ptr = CreateAndProcessCancelUpgradeContext(App1Name, 3);

    VerifyUpgradeMapContainsSingleApplicationWithCancellation(initial.get());
    VerifySendReplyNotExecuted(ptr);
}

BOOST_AUTO_TEST_CASE(CancelWithLowerInstanceIdIsIgnoredAfterUpgrade)
{
    // initial with seq 4
    auto u1 = CreateStateMachineWithSingleAsyncActionThatGoesToCompleted(4);
    ProcessUpgradeAndAssertStarted(App1Name, u1);

    // another cancel message with seq 3
    auto ptr = CreateAndProcessCancelUpgradeContext(App1Name, 3);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(u1.get());
    VerifySendReplyNotExecuted(ptr);
}


BOOST_AUTO_TEST_CASE(CancelWithEqualInstanceIdAfterCancelSendsReply)
{
    uint64 initial = 1;
    uint64 next = 1;
    CancelWithInstanceIdAfterCancelSendsReplyTestHelper(initial, next);
}

BOOST_AUTO_TEST_CASE(CancelWithHigherInstanceIdAfterCancelSendsReply)
{
    uint64 initial = 1;
    uint64 next = 2;
    CancelWithInstanceIdAfterCancelSendsReplyTestHelper(initial, next);
}

BOOST_AUTO_TEST_CASE(CancelWithEqualInstanceIdAfterCompletedUpgradeSendsCancelReply)
{
    auto upgrade = CreateStateMachineWithSingleSyncActionThatGoesToCompleted(2);

    ProcessUpgradeAndAssertStarted(App1Name, upgrade);

    auto cancelRequest = CreateAndProcessCancelUpgradeContext(App1Name, 2);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(upgrade.get());
    VerifySendReplyExecutedOnce(cancelRequest);
}

BOOST_AUTO_TEST_CASE(CancelWithEqualInstanceIdWithUpgradeNonCancellableSendsCancelReply)
{
    AsyncStateActionCompleteCallback callback;
    auto initialUpgrade = CreateStateMachineWithSingleAction(2, [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
                                                                {
                                                                    callback = inner;
                                                                    return UpgradeStateName::Invalid;
                                                                });

    // After this initialUpgrade has been started and is running in the bg
    ProcessUpgradeAndAssertStarted(App1Name, initialUpgrade);

    // Now a cancel request arrives
    auto cancelContext = CreateAndProcessCancelUpgradeContext(App1Name, 2);

    // The cancel message must not be sent at this time
    VerifySendReplyExecutedOnce(cancelContext);

    // The old upgrade should be in progress
    Verify::AreEqual(UpgradeStateName::Test1, initialUpgrade->CurrentState);
    Verify::IsTrue(!initialUpgrade->IsCancelling);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(initialUpgrade.get());

    // Complete the state transition in the old upgrade
    callback(UpgradeStateName::Completed);

    // at this point the old upgrade should be closed
    VerifyUpgradeIsInExpectedStateAndOpen(UpgradeStateName::Completed, initialUpgrade);
}

BOOST_AUTO_TEST_CASE(CancelWithHigherInstanceIdAfterCompletedUpgradeSendsCancelReply)
{
    int sendReplyCount = 0;

    auto upgrade = CreateStateMachineWithSingleAction(
        2, 
        [] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Completed; },
        [&] () { ++sendReplyCount; });

    ProcessUpgradeAndAssertStarted(App1Name, upgrade);

    auto cancelRequest = CreateAndProcessCancelUpgradeContext(App1Name, 3);

    VerifyUpgradeMapContainsSingleApplicationWithCancellation(cancelRequest.get());

    Verify::AreEqual(0, sendReplyCount);
    VerifySendReplyExecutedOnce(cancelRequest);
}


BOOST_AUTO_TEST_CASE(UpgradeWithLowerInstanceIdIsIgnoredAfterCancel)
{
    uint64 cancelInstanceId = 2;
    uint64 upgradeInstanceId = 1;
    UpgradeDropTestHelper(upgradeInstanceId, cancelInstanceId);
}


BOOST_AUTO_TEST_CASE(UpgradeWithEqualInstanceIdIsProcessedAfterCancel)
{
    uint64 cancelInstanceId = 1;
    uint64 upgradeInstanceId = 1;
    UpgradeProcessedAfterCancelTestHelper(upgradeInstanceId, cancelInstanceId);
}

BOOST_AUTO_TEST_CASE(UpgradeWithHigherInstanceIdIsProcessedAfterCancel)
{
    uint64 cancelInstanceId = 1;
    uint64 upgradeInstanceId =2;
    UpgradeProcessedAfterCancelTestHelper(upgradeInstanceId, cancelInstanceId);
}


BOOST_AUTO_TEST_CASE(CancelWithInstanceIdLessThanInitialIsDroppedAfterQueuedUpgrade)
{
    uint64 initial = 2, queued = 4, cancel = 1;
    CancelWhenUpgradeIsQueuedIsDroppedTestHelper(initial, queued, cancel);
}

BOOST_AUTO_TEST_CASE(CancelWithInstanceIdEqualToInitialIsDroppedAfterQueuedUpgrade)
{
    uint64 initial = 2, queued = 4, cancel = 2;
    CancelWhenUpgradeIsQueuedIsDroppedTestHelper(initial, queued, cancel);
}

BOOST_AUTO_TEST_CASE(CancelWithInstanceIdGreaterThanInitialAndLessThanQueuedIsDroppedAfterQueuedUpgrade)
{
    uint64 initial = 2, queued = 4, cancel = 3;
    CancelWhenUpgradeIsQueuedIsDroppedTestHelper(initial, queued, cancel);
}

BOOST_AUTO_TEST_CASE(CancelWithInstanceIdEqualToQueuedIsDroppedAfterQueuedUpgrade)
{
    uint64 initial = 2, queued = 4, cancel = 4;
    CancelWhenUpgradeIsQueuedIsDroppedTestHelper(initial, queued, cancel);
}

BOOST_AUTO_TEST_CASE(CancelWithInstanceIdGreaterThanQueuedIsDroppedAfterQueuedUpgrade)
{
    uint64 initial = 2, queued = 4, cancel = 5;
    CancelWhenUpgradeIsQueuedIsDroppedTestHelper(initial, queued, cancel);
}


BOOST_AUTO_TEST_CASE(CancelWithHigherInstanceIdWithUpgradeCompleteIsSentImmediately)
{
    uint64 const upgradeInstance = 2;
    CancelWithHigherInstanceSendsReplyTestHelper(CreateStateMachineWithSingleSyncActionThatGoesToCompleted(upgradeInstance));
}

BOOST_AUTO_TEST_CASE(CancelWithHigherInstanceIdWithUpgradeInAnCancellableStateIsSentImmediately)
{
    uint64 const upgradeInstance = 2;

    AsyncStateActionCompleteCallback callback;
    auto state = CreateState(UpgradeStateName::Test1, UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);
    auto initialUpgrade = CreateStateMachineWithSingleAction(
        upgradeInstance,
        move(state),
        [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner) 
        { 
            callback = inner;
            return UpgradeStateName::Invalid; 
        });

    CancelWithHigherInstanceSendsReplyTestHelper(initialUpgrade);
        
    // complete the callback later on
    callback(UpgradeStateName::Completed);
}

BOOST_AUTO_TEST_CASE(CancelWithHigherInstanceIdWithUpgradeNotCancellableIsDropped)
{
    uint64 upgradeInstance = 1, cancelInstance = 2;

    // Setup an upgrade which is in an async state
    AsyncStateActionCompleteCallback callback;
        
    auto state = CreateState(UpgradeStateName::Test1, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);
    auto initialUpgrade = CreateStateMachineWithSingleAction(
        upgradeInstance, 
        move(state),
        [&] (UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
        {
            callback = inner;
            return UpgradeStateName::Invalid;
        });

    // After this initialUpgrade has been started and is running in the bg
    ProcessUpgradeAndAssertStarted(App1Name, initialUpgrade);

    // Now a cancel request arrives
    auto cancelContext = CreateAndProcessCancelUpgradeContext(App1Name, cancelInstance);

    // The cancel message must not be sent at this time
    VerifySendReplyNotExecuted(cancelContext);

    // The old upgrade should be in progress
    Verify::AreEqual(UpgradeStateName::Test1, initialUpgrade->CurrentState);
        
    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(initialUpgrade.get());

    // Complete the state transition in the old upgrade
    callback(UpgradeStateName::Completed);

    // cancel should not be sent
    VerifySendReplyNotExecuted(cancelContext);

    VerifyUpgradeMapContainsSingleApplicationWithNoCancellation(initialUpgrade.get());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
