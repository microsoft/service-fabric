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
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeCancelResult
            {
            	void WriteToTextWriter(TextWriter & w, UpgradeCancelResult::Enum result)
            	{
            		switch (result)
            		{
            		case UpgradeCancelResult::NotAllowed:
            			w << "NotAllowed";
            			break;
            
            		case UpgradeCancelResult::Queued:
            			w << "Queued";
            			break;
            
            		case UpgradeCancelResult::Success:
            			w << "Success";
            			break;
            
            		default:
            			Assert::CodingError("unknown cancel result {0}", static_cast<int>(result));
            			break;
            		}
            	}
            }
        }
    }
}

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeStateCancelBehaviorType
            {

            	void WriteToTextWriter(TextWriter & w, Enum result)
            	{
            		switch (result)
            		{
            		case UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback:
            			w << "CancellableWithImmediateRollback";
            			break;
            
            		case UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback:
            			w << "NonCancellableWithDeferredRollback";
            			break;
            
            		case UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback:
            			w << "Success";
            			break;
            
            		case UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback:
            			w << "NonCancellableWithNoRollback";
            			break;
            
            		default:
            			Assert::CodingError("unknown cancel result {0}", static_cast<int>(result));
            			break;
            		}
            	}
            }
        }
    }
}

namespace ReliabilityUnitTest
{
	namespace OperationName
	{
		enum Enum
		{
			Close = 0,
			Cancel = 1,
			Rollback = 2,
		};

		void WriteToTextWriter(TextWriter & w, Enum e)
		{
			switch (e)
			{
			case Close: w << "Close"; return;
			case Cancel: w << "Cancel"; return;
			case Rollback: w << "Rollback"; return;
			}
		}
	}

	namespace OperationResult
	{
		enum Enum
		{
			Closed,
			NotAllowed,
			Queued,
			Success
		};

		void WriteToTextWriter(TextWriter & w, Enum e)
		{
			switch (e)
			{
			case Closed: w << "Closed"; return;
			case NotAllowed: w << "NotAllowed"; return;
			case Queued: w << "Queued"; return;
			case Success: w << "Success"; return;
			}
		}
	}

	class TestUpgradeStateMachine
	{
	protected:
		TestUpgradeStateMachine() { BOOST_REQUIRE(TestSetup()); }
		TEST_METHOD_SETUP(TestSetup);
		~TestUpgradeStateMachine() { BOOST_REQUIRE(TestCleanup()); }
		TEST_METHOD_CLEANUP(TestCleanup);

#pragma region Execution

#pragma endregion

#pragma region Tests before SM is started

		void ActionBeforeStartTransitionsToClosedTestHelper(OperationName::Enum operation);
		void ActionBeforeStartReturnValueTestHelper(
			OperationName::Enum operation,
			OperationResult::Enum expected);
#pragma endregion

#pragma region Tests for after SM is complete

		UpgradeStateMachineSPtr CreateStateMachineThatGoesToCompletedAndInvokeStart();
		void ActionAfterCompleteIsNoOpTestHelper(
			OperationName::Enum operation);
		void ActionAfterCompleteReturnValueTestHelper(
			OperationName::Enum operation,
			OperationResult::Enum expected);
#pragma endregion

#pragma region Tests for action during a state execution - return values

		void ActionDuringExecutionReturnValueTestHelper(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			OperationResult::Enum expected);

#pragma endregion

#pragma region Tests for action during a sync state execution

		void ActionDuringSyncStateTestHelper(
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			OperationName::Enum operation,
			bool isCloseCompleted,
			bool expectCallback);

		void CloseAfterCancelDuringSyncStateExecutionTestHelper(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType);

#pragma endregion

#pragma region Tests for action during async state execution

		void ActionDuringAsyncStateTestHelper(
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			OperationName::Enum operation,
			bool isCloseCompleted,
			bool expectCallback);

		void CloseAfterCancelDuringAsyncStateExecutionTestHelper(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType);
#pragma endregion

#pragma region Tests for action during timer state execution
		void ActionDuringTimerTestHelper(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			bool isClosed);

		// TEST_METHOD(QueuedCancellation_TimerState_InvokesCallback); NOT SUPPORTED
#pragma endregion

#pragma region Tests for action during Async Api state execution

		void ActionDuringAsyncApiTestHelper(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			bool isAccepted);

		// TEST_METHOD(QueuedCancellation_AsyncApiState_InvokesCallback); NOT SUPPORTED
#pragma endregion

#pragma region Tests for action during Begin being invoked

		void ActionDuringBeginOfAsyncApiTestHelper_CompleteSynchronously(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			bool isAccepted);

		// TEST_METHOD(QueuedCancellation_AsyncApiState_CompleteSynchronously_InvokesCancel); NOT SUPPORTED

		void ActionDuringBeginOfAsyncApiTestHelper_CompleteAsynchronously(
			OperationName::Enum operation,
			UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
			bool isAccepted);

		// TEST_METHOD(QueuedCancellation_AsyncApiState_CompleteAsynchronously_InvokesCancel); NOT SUPPORTED

#pragma endregion

		void SendReplyTestHelper(bool startStateMachine, bool expectedReturnValue);
		OperationResult::Enum ExecuteOperation(
			UpgradeStateMachine & stateMachine,
			OperationName::Enum operation)
		{
			if (operation == OperationName::Close)
			{
				stateMachine.Close();
				return OperationResult::Closed;
			}

			UpgradeCancelMode::Enum mode = operation == OperationName::Rollback ? UpgradeCancelMode::Rollback : UpgradeCancelMode::Cancel;

			auto result = InvokeTryCancel(mode);
			switch (result)
			{
			case UpgradeCancelResult::NotAllowed:
				return OperationResult::NotAllowed;
			case UpgradeCancelResult::Queued:
				return OperationResult::Queued;
			case UpgradeCancelResult::Success:
				return OperationResult::Success;
			default:
				Assert::CodingError("unknown");
			};
		}

		void ExecuteOperationAndVerify(
			UpgradeStateMachine & stateMachine,
			OperationName::Enum operation,
			OperationResult::Enum expected)
		{
			auto actual = ExecuteOperation(stateMachine, operation);

			Verify::AreEqual(expected, actual, wformatString("Operation {0}", operation));
		}

		void VerifyStateMachine(UpgradeStateMachine const & stateMachine, UpgradeStateName::Enum expectedState, bool expectedClose)
		{
			Verify::AreEqual(expectedClose, stateMachine.IsClosed, L"StateMachine: IsClosed");
			Verify::AreEqual(expectedState, stateMachine.CurrentState, L"StateMachine: CurrentState");
		}

		void VerifyStateMachineIsClosedAndInState(UpgradeStateMachine const & stateMachine, UpgradeStateName::Enum expectedState)
		{
			VerifyStateMachine(stateMachine, expectedState, true);
		}

		void VerifyStateMachineIsNotClosedAndInState(UpgradeStateMachine const & stateMachine, UpgradeStateName::Enum expectedState)
		{
			VerifyStateMachine(stateMachine, expectedState, false);
		}

		void VerifyCancelCallbackInvoked()
		{
			Verify::AreEqual(1, callbackInvokeCount_, L"CancelCallbackInvoke");
		}

		void VerifyCancelCallbackNotInvoked()
		{
			Verify::AreEqual(0, callbackInvokeCount_, L"CancelCallbackInvoke");
		}

		void VerifyCancelCallback(bool isExpected)
		{
			if (isExpected)
			{
				VerifyCancelCallbackInvoked();
			}
			else
			{
				VerifyCancelCallbackNotInvoked();
			}
		}

		void InvokeStart(UpgradeStateMachine & stateMachine)
		{
			stateMachine.Start(RollbackSnapshotUPtr());
		}

		UpgradeCancelResult::Enum InvokeTryCancel(UpgradeCancelMode::Enum cancelMode, UpgradeStateMachine::CancelCompletionCallback callback = nullptr)
		{
			return stateMachine_->TryCancelUpgrade(cancelMode, [=](UpgradeStateMachineSPtr const & inner)
			{
				Verify::IsTrue(inner->IsClosed, L"sm must be closed");
				callbackInvokeCount_++;
				if (callback != nullptr)
				{
					callback(inner);
				}
			});
		}

		// Setup a state machine for testing core features
		// This will not use a send reply action
		UpgradeStateMachineSPtr TestSetup(
			UpgradeStateDescriptionUPtr && state1,
			UpgradeStateDescriptionUPtr && state2,
			UpgradeStateDescriptionUPtr && state3,
			UpgradeStubBase::EnterStateFunctionPtr enterState);

		UpgradeStateMachineSPtr TestSetup(
			UpgradeStateDescriptionUPtr && state1,
			UpgradeStubBase::EnterStateFunctionPtr enterState);

		UpgradeStateMachineSPtr TestSetup(
			UpgradeStateDescriptionUPtr && state1,
			UpgradeStateDescriptionUPtr && state2,
			UpgradeStateDescriptionUPtr && state3);

		UpgradeStateMachineSPtr TestSetup(
			UpgradeStateDescriptionUPtr && state1);

		void SetEnterStateFunctionPtr(
			UpgradeStubBase::EnterStateFunctionPtr enterState);

		void SetResendReplyFunctionPtr(
			UpgradeStubBase::ResendReplyFunctionPtr resend);

		void SetExitAsyncOperationStateFunctionPtr(
			UpgradeStubBase::ExitAsyncOperationStateFunctionPtr exitAsyncOperationState);

		void SetEnterAsyncOperationStateFunctionPtr(
			UpgradeStubBase::EnterAsyncOperationStateFunctionPtr enterAsyncOperationState);

		UnitTestContextUPtr utContext_;
		UpgradeStateMachineSPtr stateMachine_;
		UpgradeStubBase * upgrade_;

		int callbackInvokeCount_;
	};

	bool TestUpgradeStateMachine::TestSetup()
	{
		upgrade_ = nullptr;

		callbackInvokeCount_ = 0;

		if (utContext_ == nullptr)
		{
			utContext_ = UnitTestContext::Create();
		}

		return true;
	}

	bool TestUpgradeStateMachine::TestCleanup()
	{
		if (stateMachine_)
		{
			stateMachine_->Close();
			stateMachine_.reset();
		}

		if (utContext_)
		{
			utContext_->Cleanup();
			utContext_.reset();
		}

		upgrade_ = nullptr;

		return true;
	}

	UpgradeStateMachineSPtr TestUpgradeStateMachine::TestSetup(
		UpgradeStateDescriptionUPtr && state1,
		UpgradeStubBase::EnterStateFunctionPtr enterState)
	{
		return TestSetup(move(state1), nullptr, nullptr, enterState);
	}

	UpgradeStateMachineSPtr TestUpgradeStateMachine::TestSetup(
		UpgradeStateDescriptionUPtr && state1,
		UpgradeStateDescriptionUPtr && state2,
		UpgradeStateDescriptionUPtr && state3,
		UpgradeStubBase::EnterStateFunctionPtr enterState)
	{
		upgrade_ = new UpgradeStubBase(utContext_->RA, 1, move(state1), move(state2), move(state3));

		SetEnterStateFunctionPtr(enterState);

		stateMachine_ = make_shared<UpgradeStateMachine>(IUpgradeUPtr(upgrade_), utContext_->RA);
		return stateMachine_;
	}

	UpgradeStateMachineSPtr TestUpgradeStateMachine::TestSetup(
		UpgradeStateDescriptionUPtr && state1,
		UpgradeStateDescriptionUPtr && state2,
		UpgradeStateDescriptionUPtr && state3)
	{
		return TestSetup(move(state1), move(state2), move(state3), nullptr);
	}

	UpgradeStateMachineSPtr TestUpgradeStateMachine::TestSetup(
		UpgradeStateDescriptionUPtr && state1)
	{
		return TestSetup(move(state1), nullptr);
	}
	
	void TestUpgradeStateMachine::SetEnterStateFunctionPtr(
		UpgradeStubBase::EnterStateFunctionPtr enterState)
	{
		upgrade_->EnterStateFunction = enterState;
	}

	void TestUpgradeStateMachine::SetResendReplyFunctionPtr(
		UpgradeStubBase::ResendReplyFunctionPtr resend)
	{
		upgrade_->ResendReplyFunction = resend;
	}

	void TestUpgradeStateMachine::SetExitAsyncOperationStateFunctionPtr(
		UpgradeStubBase::ExitAsyncOperationStateFunctionPtr exitAsyncOperationState)
	{
		upgrade_->ExitAsyncOperationStateFunction = exitAsyncOperationState;
	}

	void TestUpgradeStateMachine::SetEnterAsyncOperationStateFunctionPtr(
		UpgradeStubBase::EnterAsyncOperationStateFunctionPtr enterAsyncOperationState)
	{
		upgrade_->EnterAsyncOperationStateFunction = enterAsyncOperationState;
	}
	
	void TestUpgradeStateMachine::ActionAfterCompleteIsNoOpTestHelper(
		OperationName::Enum operation)
	{
		auto stateMachine = CreateStateMachineThatGoesToCompletedAndInvokeStart();

		ExecuteOperation(*stateMachine, operation);

		if (operation == OperationName::Close)
		{
			VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::Completed);
		}
		else
		{
			VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Completed);
		}

		VerifyCancelCallbackNotInvoked();
	}

	void TestUpgradeStateMachine::ActionAfterCompleteReturnValueTestHelper(
		OperationName::Enum operation,
		OperationResult::Enum expected)
	{
		auto stateMachine = CreateStateMachineThatGoesToCompletedAndInvokeStart();

		ExecuteOperationAndVerify(*stateMachine, operation, expected);
	}
	void TestUpgradeStateMachine::ActionDuringExecutionReturnValueTestHelper(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		OperationResult::Enum expected)
	{
		TestSetup();

		OperationResult::Enum actual;
		auto state = CreateState(UpgradeStateName::Test1, cancelBehaviorType);
		UpgradeStateMachineSPtr sm = TestSetup(move(state), [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
		{
			actual = ExecuteOperation(*sm, operation);
			return UpgradeStateName::Completed;
		});

		InvokeStart(*sm);

		Verify::AreEqualLogOnError(expected, actual, wformatString("ReturnValue for op {0} with behavior type {1}", operation, cancelBehaviorType));

		TestCleanup();
	}

	void TestUpgradeStateMachine::ActionDuringSyncStateTestHelper(
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		OperationName::Enum operation,
		bool isCloseCompleted,
		bool expectCallback)
	{
		auto state = CreateState(UpgradeStateName::Test1, cancelBehaviorType);
		UpgradeStateMachineSPtr stateMachine = TestSetup(move(state), nullptr, nullptr,
			[&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
		{
			ExecuteOperation(*stateMachine, operation);
			return UpgradeStateName::Completed;
		});

		InvokeStart(*stateMachine);

		VerifyStateMachine(*stateMachine, isCloseCompleted ? UpgradeStateName::Test1 : UpgradeStateName::Completed, isCloseCompleted);
		VerifyCancelCallback(expectCallback);
	}

	void TestUpgradeStateMachine::CloseAfterCancelDuringSyncStateExecutionTestHelper(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)
	{
		auto state = CreateState(UpgradeStateName::Test1, cancelBehaviorType);
		UpgradeStateMachineSPtr stateMachine = TestSetup(move(state), nullptr, nullptr,
			[&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
		{
			ExecuteOperation(*stateMachine, operation);
			ExecuteOperation(*stateMachine, OperationName::Close);
			return UpgradeStateName::Completed;
		});

		InvokeStart(*stateMachine);

		VerifyStateMachine(*stateMachine, UpgradeStateName::Test1, true);
		VerifyCancelCallback(false);
	}

	void TestUpgradeStateMachine::ActionDuringAsyncStateTestHelper(
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		OperationName::Enum operation,
		bool isCloseCompleted,
		bool expectCallback)
	{
		AsyncStateActionCompleteCallback callback;
		auto state = CreateState(UpgradeStateName::Test1, cancelBehaviorType);
		UpgradeStateMachineSPtr stateMachine = TestSetup(move(state),
			[&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
		{
			callback = inner;
			return UpgradeStateName::Invalid;
		});

		InvokeStart(*stateMachine);

		ExecuteOperation(*stateMachine, operation);

		VerifyCancelCallback(false); // Execution is not complete: Cancel cannot execute

		// complete the async state machine
		callback(UpgradeStateName::Completed);

		VerifyStateMachine(*stateMachine, isCloseCompleted ? UpgradeStateName::Test1 : UpgradeStateName::Completed, isCloseCompleted);
		VerifyCancelCallback(expectCallback);
	}

	void TestUpgradeStateMachine::CloseAfterCancelDuringAsyncStateExecutionTestHelper(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)
	{
		AsyncStateActionCompleteCallback callback;
		auto state = CreateState(UpgradeStateName::Test1, cancelBehaviorType);
		UpgradeStateMachineSPtr stateMachine = TestSetup(move(state),
			[&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback inner)
		{
			callback = inner;
			return UpgradeStateName::Invalid;
		});

		InvokeStart(*stateMachine);

		ExecuteOperation(*stateMachine, operation);
		ExecuteOperation(*stateMachine, OperationName::Close);

		VerifyStateMachine(*stateMachine, UpgradeStateName::Test1, true);
		VerifyCancelCallback(false); // Execution is not complete: Cancel cannot execute

		// complete the async state machine
		callback(UpgradeStateName::Completed);

		VerifyStateMachine(*stateMachine, UpgradeStateName::Test1, true);
		VerifyCancelCallback(false); // Execution is not complete: Cancel cannot execute
	}

	void TestUpgradeStateMachine::ActionDuringTimerTestHelper(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		bool isClosed)
	{
		const TimeSpan retryInterval = TimeSpan::FromMilliseconds(5000);

		auto retryState = make_unique<UpgradeStateDescription>(
			UpgradeStateName::Test1,
			retryInterval,
			UpgradeStateName::Completed,
			cancelBehaviorType);

		auto stateMachine = TestSetup(move(retryState));
		InvokeStart(*stateMachine);

		// perform action
		ExecuteOperation(*stateMachine, operation);

		if (isClosed)
		{
			Verify::IsTrue(nullptr == stateMachine_->TimerObj);
			VerifyStateMachineIsClosedAndInState(*stateMachine_, UpgradeStateName::Test1);
		}
		else
		{
			Verify::IsTrue(nullptr != stateMachine_->TimerObj);
			VerifyStateMachineIsNotClosedAndInState(*stateMachine_, UpgradeStateName::Test1);
		}

		VerifyCancelCallbackNotInvoked();
	}

	void TestUpgradeStateMachine::ActionDuringAsyncApiTestHelper(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		bool isAccepted)
	{
		auto stateMachine = TestSetup(CreateAsyncApiState(UpgradeStateName::Test1, cancelBehaviorType));

		SetExitAsyncOperationStateFunctionPtr([](UpgradeStateName::Enum, AsyncOperationSPtr const &) { return UpgradeStateName::Completed; });
		upgrade_->AsyncOperationStateAsyncApi.SetCompleteAsynchronously();

		InvokeStart(*stateMachine);

		ExecuteOperation(*stateMachine, operation);

		upgrade_->AsyncOperationStateAsyncApi.VerifyCancelRequested(isAccepted);

		if (isAccepted)
		{
			VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::Test1);
		}
		else
		{
			VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Test1);
		}

		upgrade_->AsyncOperationStateAsyncApi.FinishOperationWithSuccess(nullptr);
	}

	void TestUpgradeStateMachine::ActionDuringBeginOfAsyncApiTestHelper_CompleteSynchronously(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		bool isAccepted)
	{
		auto stateMachine = TestSetup(CreateAsyncApiState(UpgradeStateName::Test1, cancelBehaviorType));

		SetEnterAsyncOperationStateFunctionPtr([&](UpgradeStateName::Enum)
		{
			ExecuteOperation(*stateMachine, operation);
		});

		SetExitAsyncOperationStateFunctionPtr([](UpgradeStateName::Enum, AsyncOperationSPtr const &) { return UpgradeStateName::Completed; });
		upgrade_->AsyncOperationStateAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);

		InvokeStart(*stateMachine);

		upgrade_->AsyncOperationStateAsyncApi.VerifyCancelRequested(false);

		if (isAccepted)
		{
			VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::Test1);
		}
		else
		{
			VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Completed);
		}
	}

	void TestUpgradeStateMachine::ActionDuringBeginOfAsyncApiTestHelper_CompleteAsynchronously(
		OperationName::Enum operation,
		UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
		bool isAccepted)
	{
		auto stateMachine = TestSetup(CreateAsyncApiState(UpgradeStateName::Test1, cancelBehaviorType));

		SetEnterAsyncOperationStateFunctionPtr([&](UpgradeStateName::Enum)
		{
			ExecuteOperation(*stateMachine, operation);
		});

		SetExitAsyncOperationStateFunctionPtr([](UpgradeStateName::Enum, AsyncOperationSPtr const &) { return UpgradeStateName::Completed; });
		upgrade_->AsyncOperationStateAsyncApi.SetCompleteAsynchronously();

		InvokeStart(*stateMachine);

		upgrade_->AsyncOperationStateAsyncApi.VerifyCancelRequested(isAccepted);

		if (isAccepted)
		{
			VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::Test1);
		}
		else
		{
			VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Test1);
		}

		upgrade_->AsyncOperationStateAsyncApi.FinishOperationWithSuccess(nullptr);
	}

	void TestUpgradeStateMachine::ActionBeforeStartReturnValueTestHelper(
		OperationName::Enum operation,
		OperationResult::Enum expected)
	{
		auto stateMachine = TestSetup(
			CreateState(UpgradeStateName::Test1),
			[](UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Invalid; });

		ExecuteOperationAndVerify(*stateMachine, operation, expected);
	}

	UpgradeStateMachineSPtr TestUpgradeStateMachine::CreateStateMachineThatGoesToCompletedAndInvokeStart()
	{
		UpgradeStateMachineSPtr stateMachine = TestSetup(
			CreateState(UpgradeStateName::Test1),
			[&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
		{
			return UpgradeStateName::Completed;
		});

		InvokeStart(*stateMachine);

		return stateMachine;
	}

	void TestUpgradeStateMachine::SendReplyTestHelper(bool startStateMachine, bool expectedReturnValue)
	{
		auto state = CreateState(UpgradeStateName::Test1);

		stateMachine_ = TestSetup(move(state),
			[](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
		{
			return UpgradeStateName::Completed;
		});

		if (startStateMachine)
		{
			InvokeStart(*stateMachine_);
		}

		bool actual = stateMachine_->IsCompleted;
		Verify::AreEqual(expectedReturnValue, actual);
	}

	void TestUpgradeStateMachine::ActionBeforeStartTransitionsToClosedTestHelper(
		OperationName::Enum operation)
	{
		auto stateMachine = TestSetup(
			CreateState(UpgradeStateName::Test1),
			[](UpgradeStateName::Enum, AsyncStateActionCompleteCallback) { return UpgradeStateName::Invalid; });

		ExecuteOperation(*stateMachine, operation);

		VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::None);
		VerifyCancelCallbackNotInvoked();
	}

	class AsyncStateInvokeTestHelper
	{
	public:
		AsyncStateInvokeTestHelper()
			: actual_(UpgradeStateName::Invalid)
		{
		}

		UpgradeStubBase::ExitAsyncOperationStateFunctionPtr GetFunction()
		{
			UpgradeStubBase::ExitAsyncOperationStateFunctionPtr rv = [this](UpgradeStateName::Enum state, AsyncOperationSPtr const & inner) -> UpgradeStateName::Enum
			{
				actual_ = state;
				actualOperation_ = inner;
				return UpgradeStateName::Completed;
			};

			return rv;
		}

		void Verify(UpgradeStubBase & upgrade)
		{
			Verify::AreEqual(UpgradeStateName::Test1, actual_, L"State");
			Verify::AreEqual(upgrade.AsyncOperationStateAsyncApi.Current.get(), actualOperation_.get(), L"Operation");
		}

	private:
		AsyncOperationSPtr actualOperation_;
		UpgradeStateName::Enum actual_;
	};

    BOOST_AUTO_TEST_SUITE(Unit)

    BOOST_FIXTURE_TEST_SUITE(TestUpgradeStateMachineSuite, TestUpgradeStateMachine)

    BOOST_AUTO_TEST_CASE(MultiStepSyncStateMachineStepExecution)
    {
        vector<UpgradeStateName::Enum> steps;

        // Setup a single step sync state machine 
        auto state1 = CreateState(UpgradeStateName::Test1);
        auto state2 = CreateState(UpgradeStateName::Test2);

        UpgradeStateMachineSPtr stateMachine = TestSetup(move(state1), move(state2), nullptr,
                                                            [&](UpgradeStateName::Enum state, AsyncStateActionCompleteCallback)
                                                            {
                                                                steps.push_back(state);
                                                                if (state == UpgradeStateName::Test1)
                                                                {
                                                                    return UpgradeStateName::Test2;
                                                                }
                                                                else if (state == UpgradeStateName::Test2)
                                                                {
                                                                    return UpgradeStateName::Completed;
                                                                }
                                                                else
                                                                {
                                                                    Assert::CodingError("Invalid state");
                                                                }
                                                            });

        InvokeStart(*stateMachine);

        Verify::AreEqual(UpgradeStateName::Completed, stateMachine->CurrentState, L"Current state");
        Verify::Vector(steps, UpgradeStateName::Test1, UpgradeStateName::Test2);
    }

    BOOST_AUTO_TEST_CASE(MultiStepMixedStateMachineStepExecution)
    {
        // Start state = Test1 which completes synchronously to Test2
        // Test2 = starts async and stores the callback
        // Test completes async callback to FabricUpgrade_UpgradeFailed
        // Which is also async and stores the callback

        vector<UpgradeStateName::Enum> steps;
        AsyncStateActionCompleteCallback callback;

        auto state1 = CreateState(UpgradeStateName::Test1);
        auto state2 = CreateState(UpgradeStateName::Test2);
        auto state3 = CreateState(UpgradeStateName::FabricUpgrade_UpgradeFailed);

        UpgradeStateMachineSPtr stateMachine = TestSetup(move(state1), move(state2), move(state3),
                                                            [&](UpgradeStateName::Enum state, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                steps.push_back(state);
                                                                if (state == UpgradeStateName::Test1)
                                                                {
                                                                    return UpgradeStateName::Test2;
                                                                }
                                                                else
                                                                {
                                                                    // Async
                                                                    callback = inner;
                                                                    return UpgradeStateName::Invalid;
                                                                }
                                                            });

        InvokeStart(*stateMachine);

        // it should be stopped at state Test2
        Verify::AreEqual(UpgradeStateName::Test2, stateMachine->CurrentState, L"Current state");

        // resume to UpgradeFailed
        callback(UpgradeStateName::FabricUpgrade_UpgradeFailed);

        // when this returns we should have gone back to Test2
        Verify::AreEqual(UpgradeStateName::FabricUpgrade_UpgradeFailed, stateMachine->CurrentState, L"Current state");

        // now complete
        callback(UpgradeStateName::Completed);

        Verify::Vector(steps, UpgradeStateName::Test1, UpgradeStateName::Test2, UpgradeStateName::FabricUpgrade_UpgradeFailed);
    }

    BOOST_AUTO_TEST_CASE(AsyncInvocationClonesSharedPtr)
    {
        bool executed = false;
        AsyncStateActionCompleteCallback callback;

        auto state1 = CreateState(UpgradeStateName::Test1);
        auto state2 = CreateState(UpgradeStateName::Test2);

        UpgradeStateMachine* stateMachinePtr = nullptr;
        UpgradeStateMachineSPtr stateMachine = TestSetup(move(state1), move(state2), nullptr,
                                                            [&](UpgradeStateName::Enum state, AsyncStateActionCompleteCallback inner)
                                                            {
                                                                if (state == UpgradeStateName::Test1)
                                                                {
                                                                    callback = inner;
                                                                    return UpgradeStateName::Invalid;
                                                                }
                                                                else
                                                                {
                                                                    executed = true;

                                                                    // Need to close the state machine over here
                                                                    // the outer test will release its references
                                                                    // this also validates that the state machine is cloned
                                                                    // other wise this will AV
                                                                    stateMachinePtr->Close();
                                                                    return UpgradeStateName::Completed;

                                                                }
                                                            });

        stateMachinePtr = stateMachine.get();

        InvokeStart(*stateMachine);

        // at this point there should be two references to the shared_ptr of the state machine
        // one is stateMachine and another was created when the callback was invoked
        // this test will validate that if the original (stateMachine) is deleted 
        // the execution can still continue when the asyn ccallback returns
        stateMachine.reset();
        stateMachine_.reset();

        // now resume back to Test1
        callback(UpgradeStateName::Test2);

        // action2 should have executed
        Verify::IsTrue(executed, L"Action2 did not execute");
    }

    BOOST_AUTO_TEST_CASE(AsyncApiStateClonesSharedPtr)
    {
        bool executed = false;
        AsyncStateActionCompleteCallback callback;

        auto state1 = CreateAsyncApiState(UpgradeStateName::Test1);
        auto state2 = CreateState(UpgradeStateName::Test2);

        UpgradeStateMachine* stateMachinePtr = nullptr;
        UpgradeStateMachineSPtr stateMachine = TestSetup(move(state1), move(state2), nullptr);

        upgrade_->AsyncOperationStateAsyncApi.SetCompleteAsynchronously();

        SetExitAsyncOperationStateFunctionPtr([](UpgradeStateName::Enum, AsyncOperationSPtr const &)
            {
                return UpgradeStateName::Test2;
            });

        SetEnterStateFunctionPtr([&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
            {
                executed = true;
                stateMachinePtr->Close(); // Need to close or the state machine will fail because the timer is not disposed
                return UpgradeStateName::Completed;
            });

        stateMachinePtr = stateMachine.get();

        InvokeStart(*stateMachine);

        // at this point there should be two references to the shared_ptr of the state machine
        // one is stateMachine and another was created when the callback was invoked
        // this test will validate that if the original (stateMachine) is deleted 
        // the execution can still continue when the asyn ccallback returns
        stateMachine.reset();
        stateMachine_.reset();

        // now complete the async op
        upgrade_->AsyncOperationStateAsyncApi.FinishOperationWithSuccess(nullptr);

        // The second state should have executed
        Verify::IsTrue(executed, L"State should have executed");
    }

    BOOST_AUTO_TEST_CASE(AsyncOperationAsyncTestExecution)
    {
        AsyncStateInvokeTestHelper testHelper;

        auto stateMachine = TestSetup(CreateAsyncApiState(UpgradeStateName::Test1));

        SetExitAsyncOperationStateFunctionPtr(testHelper.GetFunction());

        upgrade_->AsyncOperationStateAsyncApi.SetCompleteAsynchronously();

        InvokeStart(*stateMachine);

        Verify::AreEqual(UpgradeStateName::Test1, stateMachine->CurrentState, L"Current state");

        upgrade_->AsyncOperationStateAsyncApi.FinishOperationWithSuccess(nullptr);

        VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Completed);
        testHelper.Verify(*upgrade_);
    }

    BOOST_AUTO_TEST_CASE(AsyncOperationStateTransition)
    {
        // Start state = async op state test1 complete asynchronously
        // Next state = async state that stores callback
        // next state = async API state that completes

        AsyncStateActionCompleteCallback callback;
        auto stateMachine = TestSetup(
            CreateAsyncApiState(UpgradeStateName::Test1),
            CreateAsyncApiState(UpgradeStateName::Test2),
            CreateState(UpgradeStateName::ApplicationUpgrade_Analyze),
            [&](UpgradeStateName::Enum s, AsyncStateActionCompleteCallback cb)
            {
                Verify::AreEqualLogOnError(s, UpgradeStateName::ApplicationUpgrade_Analyze);
                callback = cb;
                return UpgradeStateName::Invalid;
            });

        SetExitAsyncOperationStateFunctionPtr([](UpgradeStateName::Enum s, AsyncOperationSPtr const&)
            {
                switch (s)
                {
                case UpgradeStateName::Test1:
                    return UpgradeStateName::Test2;
                case UpgradeStateName::Test2:
                    return UpgradeStateName::ApplicationUpgrade_Analyze;
                default:
                    Verify::Fail(L"Unexpected state");
                    return UpgradeStateName::Invalid;
                }
            });

        upgrade_->AsyncOperationStateAsyncApi.SetCompleteAsynchronously();

        InvokeStart(*stateMachine);

        // Upgrade should be stuck in test1
        VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Test1);

        // Set the expectation for the next state
        upgrade_->AsyncOperationStateAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);

        // Complete Test1
        upgrade_->AsyncOperationStateAsyncApi.FinishOperationWithSuccess(nullptr);

        // The upgrade should have executed test2 and transitioned to success
        VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::ApplicationUpgrade_Analyze);

        // Complete the cb
        callback(UpgradeStateName::Completed);

        VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Completed);
    }

    BOOST_AUTO_TEST_CASE(AsyncOperationSyncTestExecution)
    {
        AsyncStateInvokeTestHelper testHelper;

        auto stateMachine = TestSetup(CreateAsyncApiState(UpgradeStateName::Test1));

        SetExitAsyncOperationStateFunctionPtr(testHelper.GetFunction());

        upgrade_->AsyncOperationStateAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);

        InvokeStart(*stateMachine);

        VerifyStateMachineIsNotClosedAndInState(*stateMachine, UpgradeStateName::Completed);

        testHelper.Verify(*upgrade_);
    }

    BOOST_AUTO_TEST_CASE(RollbackBeforeStartTransitionsToClosed)
    {
        ActionBeforeStartTransitionsToClosedTestHelper(OperationName::Rollback);
    }

    BOOST_AUTO_TEST_CASE(CancellingBeforeStartTransitionsToClosed)
    {
        ActionBeforeStartTransitionsToClosedTestHelper(OperationName::Cancel);
    }

    BOOST_AUTO_TEST_CASE(ClosingBeforeStartTransitionsToClosed)
    {
        ActionBeforeStartTransitionsToClosedTestHelper(OperationName::Close);
    }
	
    BOOST_AUTO_TEST_CASE(RollbackBeforeStartReturnsSuccess)
    {
        ActionBeforeStartReturnValueTestHelper(OperationName::Rollback, OperationResult::Success);
    }

    BOOST_AUTO_TEST_CASE(CancelBeforeStartReturnsSuccess)
    {
        ActionBeforeStartReturnValueTestHelper(OperationName::Cancel, OperationResult::Success);
    }

    BOOST_AUTO_TEST_CASE(StartForClosedStateMachineIsNoOp)
    {
        bool executed = false;

        UpgradeStateMachineSPtr stateMachine = TestSetup(
            CreateState(UpgradeStateName::Test1),
            [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
            {
                executed = true;
                return UpgradeStateName::Completed;
            });

        ExecuteOperation(*stateMachine, OperationName::Close);

        InvokeStart(*stateMachine);

        VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::None);
        VERIFY_IS_FALSE(executed);
        VerifyCancelCallbackNotInvoked();
    }

    BOOST_AUTO_TEST_CASE(RollbackAfterCompleteIsNoOpAndIsNotClosed)
    {
        ActionAfterCompleteIsNoOpTestHelper(OperationName::Rollback);
    }

    BOOST_AUTO_TEST_CASE(CancelAfterCompleteIsNoOpAndIsNotClosed)
    {
        ActionAfterCompleteIsNoOpTestHelper(OperationName::Cancel);
    }

    BOOST_AUTO_TEST_CASE(CloseAfterCompleteIsNoOp)
    {
        ActionAfterCompleteIsNoOpTestHelper(OperationName::Close);
    }

    BOOST_AUTO_TEST_CASE(RollbackAfterCompleteReturnsSuccess)
    {
        ActionAfterCompleteReturnValueTestHelper(OperationName::Rollback, OperationResult::Success);
    }

    BOOST_AUTO_TEST_CASE(CancelAfterCompleteReturnsSuccess)
    {
        ActionAfterCompleteReturnValueTestHelper(OperationName::Cancel, OperationResult::Success);
    }

    BOOST_AUTO_TEST_CASE(RollbackOrCancelReturnValueTests)
    {
        vector<tuple<OperationName::Enum, UpgradeStateCancelBehaviorType::Enum, OperationResult::Enum>> tests;

        tests.push_back(make_tuple(OperationName::Rollback, UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback, OperationResult::Success));
        tests.push_back(make_tuple(OperationName::Rollback, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback, OperationResult::Queued));
        tests.push_back(make_tuple(OperationName::Rollback, UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback, OperationResult::Success));
        tests.push_back(make_tuple(OperationName::Rollback, UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback, OperationResult::NotAllowed));
        tests.push_back(make_tuple(OperationName::Cancel, UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback, OperationResult::Success));
        tests.push_back(make_tuple(OperationName::Cancel, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback, OperationResult::NotAllowed));
        tests.push_back(make_tuple(OperationName::Cancel, UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback, OperationResult::NotAllowed));
        tests.push_back(make_tuple(OperationName::Cancel, UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback, OperationResult::NotAllowed));

        for (auto i : tests)
        {
            ActionDuringExecutionReturnValueTestHelper(get<0>(i), get<1>(i), get<2>(i));
        }
    }

    BOOST_AUTO_TEST_CASE(NotAllowedCancel_SyncState_IsNoOp)
    {
        ActionDuringSyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback,
            OperationName::Rollback,
            false,
            false);
    }

    BOOST_AUTO_TEST_CASE(QueuedCancellation_SyncState_InvokesCallback)
    {
        ActionDuringSyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            OperationName::Rollback,
            true,
            true);
    }

    BOOST_AUTO_TEST_CASE(ImmediateCancellation_SyncState_ClosesStateMachine)
    {
        ActionDuringSyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
            OperationName::Rollback,
            true,
            false);
    }

    BOOST_AUTO_TEST_CASE(Close_SyncState_ClosesStateMachine)
    {
        ActionDuringSyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            OperationName::Close,
            true,
            false);
    }

    BOOST_AUTO_TEST_CASE(CloseOfCancelledStateMachine_SyncState)
    {
        CloseAfterCancelDuringSyncStateExecutionTestHelper(
            OperationName::Rollback,
            UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);
    }

    BOOST_AUTO_TEST_CASE(CloseOfCancellingStateMachine_SyncState)
    {
        CloseAfterCancelDuringSyncStateExecutionTestHelper(
            OperationName::Rollback,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);
    }

    BOOST_AUTO_TEST_CASE(NotAllowedCancel_AsyncState_IsNoOp)
    {
        ActionDuringAsyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback,
            OperationName::Rollback,
            false,
            false);
    }

    BOOST_AUTO_TEST_CASE(QueuedCancellation_AsyncState_InvokesCallback)
    {
        ActionDuringAsyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            OperationName::Rollback,
            true,
            true);
    }

    BOOST_AUTO_TEST_CASE(ImmediateCancellation_AsyncState_ClosesStateMachine)
    {
        ActionDuringAsyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
            OperationName::Rollback,
            true,
            false);
    }

    BOOST_AUTO_TEST_CASE(Close_AsyncState_ClosesStateMachine)
    {
        ActionDuringAsyncStateTestHelper(
            UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback,
            OperationName::Close,
            true,
            false);
    }

    BOOST_AUTO_TEST_CASE(CloseOfCancelledStateMachine_AsyncState)
    {
        CloseAfterCancelDuringAsyncStateExecutionTestHelper(
            OperationName::Rollback,
            UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);
    }

    BOOST_AUTO_TEST_CASE(CloseOfCancellingStateMachine_AsyncState)
    {
        CloseAfterCancelDuringAsyncStateExecutionTestHelper(
            OperationName::Rollback,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);
    }


    BOOST_AUTO_TEST_CASE(NotAllowedCancel_TimerState_IsNoOp)
    {
        ActionDuringTimerTestHelper(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
            false);
    }

    BOOST_AUTO_TEST_CASE(ImmediateCancellation_TimerState_ClosesStateMachine)
    {
        ActionDuringTimerTestHelper(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(Close_TimerState_ClosesStateMachine)
    {
        ActionDuringTimerTestHelper(
            OperationName::Close,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(NotAllowedCancel_AsyncApiState_IsNoOp)
    {
        ActionDuringAsyncApiTestHelper(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            false);
    }

    // void TestUpgradeStateMachine::QueuedCancellation_TimerState_InvokesCallback); NOT SUPPORTED
    BOOST_AUTO_TEST_CASE(ImmediateCancellation_AsyncApiState_InvokesCancel)
    {
        ActionDuringAsyncApiTestHelper(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(Close_AsyncApiState_InvokesCancel)
    {
        ActionDuringAsyncApiTestHelper(
            OperationName::Close,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }


    BOOST_AUTO_TEST_CASE(NotAllowedCancel_AsyncApiBegin_CompleteSynchronously_IsNoOp)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteSynchronously(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            false);
    }

    BOOST_AUTO_TEST_CASE(ImmediateCancellation_AsyncApiBegin_CompleteSynchronously_ClosesSM)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteSynchronously(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(Close_AsyncApiBegin_CompleteSynchronously_ClosesSM)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteSynchronously(
            OperationName::Close,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(NotAllowedCancel_AsyncApiBegin_CompleteAsynchronously_IsNoOp)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteAsynchronously(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            false);
    }

    BOOST_AUTO_TEST_CASE(ImmediateCancellation_AsyncApiBegin_CompleteAsynchronously_ClosesSM)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteAsynchronously(
            OperationName::Cancel,
            UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(Close_AsyncApiBegin_CompleteAsynchronously_ClosesSM)
    {
        ActionDuringBeginOfAsyncApiTestHelper_CompleteAsynchronously(
            OperationName::Close,
            UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback,
            true);
    }

    BOOST_AUTO_TEST_CASE(EnteringATimerStateCausesARetry)
    {
        const TimeSpan retryInterval = TimeSpan::FromMilliseconds(50);
        int64 startTS;
        int64 executionTS;

        auto state1 = CreateState(UpgradeStateName::Test2);

        auto state2 = make_unique<UpgradeStateDescription>(UpgradeStateName::Test1, retryInterval, UpgradeStateName::Test2, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);

        auto stateMachine = TestSetup(move(state2), move(state1), nullptr,
                                        [&](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
                                        {
                                            executionTS = Stopwatch::GetTimestamp();
                                            return UpgradeStateName::Completed;
                                        });

        // start the state machine
        startTS = Stopwatch::GetTimestamp();
        InvokeStart(*stateMachine);

        // wait for it to complete
        BusyWaitUntil([&]() { return stateMachine->CurrentState == UpgradeStateName::Completed; });

        Verify::IsTrue(executionTS > startTS);
    }

    BOOST_AUTO_TEST_CASE(InvokingTimerOnCloseIsNoOp)
    {
        const TimeSpan retryInterval = TimeSpan::FromMilliseconds(5000);

        auto retryState = make_unique<UpgradeStateDescription>(UpgradeStateName::Test1, retryInterval, UpgradeStateName::Completed, UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);

        auto stateMachine = TestSetup(move(retryState));
        InvokeStart(*stateMachine);

        // perform action
        stateMachine->Close();

        // invoke the timer - there should be no crash etc
        // we should still be in closed
        stateMachine->OnTimer();

        VerifyStateMachineIsClosedAndInState(*stateMachine, UpgradeStateName::Test1);
    }

    BOOST_AUTO_TEST_CASE(IsCompletedReturnsFalseIfNotCompleted)
    {
        SendReplyTestHelper(false, false);
    }

    BOOST_AUTO_TEST_CASE(IsCompletedReturnsTrueIfCompleted)
    {
        SendReplyTestHelper(true, true);
    }

    BOOST_AUTO_TEST_CASE(IsCompletedReturnsFalseIfClosed)
    {
        auto state = CreateState(UpgradeStateName::Test1);

        stateMachine_ = TestSetup(move(state),
                                    [](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
                                    {
                                        return UpgradeStateName::Completed;
                                    });

        InvokeStart(*stateMachine_);
        stateMachine_->Close();

        bool actual = stateMachine_->IsCompleted;
        Verify::AreEqual(false, actual);
    }

    BOOST_AUTO_TEST_CASE(SendReplyInvokesSendReplyOnUpgrade)
    {
        bool executed = false;
        auto state = CreateState(UpgradeStateName::Test1);

        stateMachine_ = TestSetup(move(state),
                                    [](UpgradeStateName::Enum, AsyncStateActionCompleteCallback)
                                    {
                                        return UpgradeStateName::Completed;
                                    });

        SetResendReplyFunctionPtr([&]() { executed = true; });

        stateMachine_->SendReply();
        Verify::AreEqual(true, executed);
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}
