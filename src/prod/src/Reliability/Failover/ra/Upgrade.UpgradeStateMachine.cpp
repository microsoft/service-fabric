// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Infrastructure;

namespace 
{
    namespace UpgradeLifeCycle
    {
        StringLiteral const Started("Started");
        StringLiteral const Closed("Closed");        
        StringLiteral const Completed("Completed");
        StringLiteral const Cancelling("Cancelling");
    }
};

UpgradeStateMachine::UpgradeStateMachine(IUpgradeUPtr && upgrade, ReconfigurationAgent & ra)
: upgrade_(move(upgrade)),
  ra_(ra),
  currentState_(UpgradeStateName::None),
  lifeCycleState_(LifeCycleState::Open)
{
    ASSERT_IF(upgrade_ == nullptr, "Upgrade cannot be null");
}
     
void UpgradeStateMachine::Start(RollbackSnapshotUPtr && rollbackSnapshot)
{
    {
        AcquireReadLock grab(lock_);
        ASSERT_IF(currentState_ != UpgradeStateName::None, "Invalid state at start {0} for upgrade {1}", currentState_, ActivityId);
    }

    auto root = ra_.Root.CreateComponentRoot();
    auto thisSPtr = shared_from_this();
    timer_ = Timer::Create(TimerTagDefault, [this, root, thisSPtr](TimerSPtr const &) { this->OnTimer(); }, true);

    upgrade_->TraceLifeCycle(UpgradeLifeCycle::Started);

    auto startState = upgrade_->GetStartState(move(rollbackSnapshot));    
    ASSERT_IF(startState == UpgradeStateName::None || startState == UpgradeStateName::Completed, "Invalid start state");

    ProcessNextState(startState);
}

RollbackSnapshotUPtr UpgradeStateMachine::CreateRollbackSnapshot()
{
    AcquireWriteLock grab(lock_);
    ASSERT_IF(!IsClosed, "Upgrade should be closed before calling CreateRollback");

    return upgrade_->CreateRollbackSnapshot(currentState_);
}

void UpgradeStateMachine::Close()
{
    AsyncOperationSPtr asyncOpToCancel;
    UpgradeStateName::Enum state;

    {
        AcquireWriteLock grab(lock_);
        CleanupTimer();
        MarkClosed();
        callback_ = nullptr;
        state = currentState_;
        swap(asyncOpToCancel, asyncOp_);
    }

    CancelOperationIfNotNull(state, asyncOpToCancel);
}

UpgradeCancelResult::Enum UpgradeStateMachine::TryCancelUpgrade(UpgradeCancelMode::Enum cancelMode, CancelCompletionCallback callback)
{
    AsyncOperationSPtr asyncOpToCancel;
    UpgradeStateName::Enum state;
    UpgradeCancelResult::Enum result;

    {
        AcquireWriteLock grab(lock_);

        state = currentState_;

        // Cancellation is no-op if already completed or closed
        if (currentState_ == UpgradeStateName::Completed || IsClosed)
        {
            return UpgradeCancelResult::Success;
        }

        // The current SM has not been started. Mark as closed and complete
        if (currentState_ == UpgradeStateName::None)
        {
            MarkClosed();
            return UpgradeCancelResult::Success;
        }

        result = DecideCancelAction(cancelMode);
        UpdateStateForCancelAction(result, callback, asyncOpToCancel);
    }

    // If an async op had to be cancelled do it outside the lock
    CancelOperationIfNotNull(state, asyncOpToCancel);

    return result;
}

void UpgradeStateMachine::UpdateStateForCancelAction(
    UpgradeCancelResult::Enum action, 
    CancelCompletionCallback callback,
    Common::AsyncOperationSPtr & operationToCancel)
{
    switch (action)
    {
    case UpgradeCancelResult::NotAllowed:
        break;

    case UpgradeCancelResult::Queued:
        ASSERT_IF(GetCurrentState().IsTimer || GetCurrentState().IsAsyncApiState, "Currently not allowed - update CIT if this changes");
        ASSERT_IF(callback == nullptr, "Cannot have null callback");
        callback_ = callback;
        lifeCycleState_ = LifeCycleState::Cancelling;
        upgrade_->TraceLifeCycle(UpgradeLifeCycle::Cancelling);
        break;

    case UpgradeCancelResult::Success:
        if (GetCurrentState().IsTimer)
        {
            CleanupTimer();
        }
        else if (GetCurrentState().IsAsyncApiState)
        {
            // copy so that cancel can be performed outside lock
            operationToCancel = asyncOp_;
        }

        MarkClosed();
        break;
    }
}

UpgradeCancelResult::Enum UpgradeStateMachine::DecideCancelAction(UpgradeCancelMode::Enum cancelMode)
{
    UpgradeStateDescription const & currentStateObj = GetCurrentState();

    bool const isCancellationSupported = currentStateObj.CancelBehaviorType == UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback;
    bool const isRollbackSupported = currentStateObj.CancelBehaviorType != UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback;
    bool const isRollbackDeferred = currentStateObj.CancelBehaviorType == UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback;
    
    if (cancelMode == UpgradeCancelMode::Cancel && !isCancellationSupported)
    {
        // Request was to cancel the upgrade and cancellation is not supported
        return UpgradeCancelResult::NotAllowed;
    }
    else if (cancelMode == UpgradeCancelMode::Rollback && !isRollbackSupported)
    {
        return UpgradeCancelResult::NotAllowed;
    }
    else if (cancelMode == UpgradeCancelMode::Cancel && isCancellationSupported)
    {
        return UpgradeCancelResult::Success;
    }
    else if (cancelMode == UpgradeCancelMode::Rollback && !isRollbackDeferred)
    {
        // Rollback is requested and the rollback does not need to wait for the current state to finish
        return UpgradeCancelResult::Success;
    }
    else if (cancelMode == UpgradeCancelMode::Rollback && isRollbackDeferred)
    {
        // Rollback is requested and the rollback must wait until the current state completes
        return UpgradeCancelResult::Queued;
    }
    else
    {
        Assert::CodingError("unsupported mode");
    }
}

void UpgradeStateMachine::SendReply()
{
    {
        // SendReply is always invoked on either
        // a StateMachine created for processing a duplicate message
        // or a statemachine that has been completed and then a cancel upgrade message was received for that same instance
        AcquireReadLock grab(lock_);
        ASSERT_IF(currentState_ != UpgradeStateName::None && currentState_ != UpgradeStateName::Completed, "upgrade should either be not started or completed");
        ASSERT_IF(IsClosed, "upgrade cannot be closed yet");
    }

    upgrade_->SendReply();
}

void UpgradeStateMachine::OnTimer()
{
    UpgradeStateName::Enum nextState;
    
    {
        AcquireReadLock grab(lock_);

        ASSERT_IF(currentState_ == UpgradeStateName::None || currentState_ == UpgradeStateName::Completed, "State cannot be none or completed");
        AssertIfCancelling();
        
        if (IsClosed)
        {
            return;
        }

        UpgradeStateDescription const & currentStateObj = GetCurrentState();
        ASSERT_IF(!currentStateObj.IsTimer, "Current state must be a timer");
        nextState = currentStateObj.TargetStateOnTimer;

        RAEventSource::Events->UpgradeStateMachineOnTimer(ra_.NodeInstanceIdStr, upgrade_->GetActivityId(), currentState_, nextState);
    }

    ProcessNextState(nextState);
}

void UpgradeStateMachine::ProcessNextState(UpgradeStateName::Enum state)
{
    do
    {
        CancelCompletionCallback callback = nullptr;
        ASSERT_IF(state == UpgradeStateName::Invalid || state == UpgradeStateName::None, "Being asked to enter invalid state {0}", state);

        {
            AcquireWriteLock grab(lock_); 
            RAEventSource::Events->UpgradeStateMachineEnterState(ra_.NodeInstanceIdStr, ActivityId, currentState_, state);

            if (IsClosed)
            {
                return;
            }
            else if (IsCancelling)
            {
                // Store callback and invoke outside lock
                // The upgrade has been cancelled and this needs to be propagated
                MarkClosed();
                callback  = callback_;
                callback_ = nullptr;
            }
            else
            {
                currentState_ = state;

                if (currentState_ == UpgradeStateName::Completed)
                {
                    ra_.PerfCounters.OnUpgradeCompleted();

                    upgrade_->TraceLifeCycle(UpgradeLifeCycle::Completed);
                    return;
                }
            }
        }

        if (callback != nullptr)
        {
            // the upgrade was cancelled and the callback was stored
            // invoke the callback and return
            callback(shared_from_this());
            return;
        }

        // Do the actual work 
        // There may be a racing cancel along with invoking the begin
        // if so then invoke cancel on the async op as required
        AsyncOperationSPtr opToCancel;
        UpgradeStateName::Enum cancelState = state;
        state = EnterState(state, opToCancel);

        CancelOperationIfNotNull(cancelState, opToCancel);

    } while (state != UpgradeStateName::Invalid);
}

UpgradeStateName::Enum UpgradeStateMachine::EnterState(
    UpgradeStateName::Enum state,
    Common::AsyncOperationSPtr & operationToCancel)
{
    UpgradeStateDescription const & stateObject = GetState(state);
    if (stateObject.IsTimer)
    {
        return EnterTimerState(stateObject, operationToCancel);
    }
    else if (stateObject.IsAsyncApiState)
    {
        return EnterAsyncApiState(stateObject, operationToCancel);
    }
    else
    {
        return EnterNormalState(stateObject, operationToCancel);
    }
}

UpgradeStateName::Enum UpgradeStateMachine::EnterTimerState(
    UpgradeStateDescription const & stateObject,
    Common::AsyncOperationSPtr & )
{
    AcquireWriteLock grab(lock_);

    AssertIfCancelling();
    if (!IsClosed)
    {
        timer_->Change(stateObject.GetRetryInterval(ra_.Config));
    }

    return UpgradeStateName::Invalid;
}

UpgradeStateName::Enum UpgradeStateMachine::EnterAsyncApiState(
    UpgradeStateDescription const & stateObject,
    Common::AsyncOperationSPtr & operationToCancel)
{
    auto state = stateObject.Name;

    auto copy = shared_from_this();
    auto op = upgrade_->EnterAsyncOperationState(
        state,
        [this, state, copy](AsyncOperationSPtr const & inner)
    {
        if (!inner->CompletedSynchronously)
        {
            // Async invoke, start processing next state 
            auto nextState = FinishAsyncApi(state, inner);
            ProcessNextState(nextState);
        }
    });

    upgrade_->TraceAsyncOperationStart(state, *op);

    if (op->CompletedSynchronously)
    {
        // Sync invoke
        return FinishAsyncApi(state, op);
    }
    else
    {
        AcquireWriteLock grab(lock_);

        AssertIfCancelling(); // queued cancellation is not allowed
        if (IsClosed)
        {
            // There was a cancel request while the begin was in progress
            // That cancel has not been propagated to the async operation yet
            operationToCancel = op;
        }

        // Set the async op - this will be used if the SM is closed or Cancelled
        // while the async api is in progress
        asyncOp_ = op;

        // Async execution - do not continue on this thread
        // the callback will run and continue the state machine
        return UpgradeStateName::Invalid;
    }
}

UpgradeStateName::Enum UpgradeStateMachine::EnterNormalState(
    UpgradeStateDescription const & stateObject,
    Common::AsyncOperationSPtr & )
{
    auto copy = shared_from_this();
    return upgrade_->EnterState(
        stateObject.Name, 
        [this, copy](UpgradeStateName::Enum nextState) 
        { 
            ProcessNextState(nextState); 
        });
}

UpgradeStateName::Enum UpgradeStateMachine::FinishAsyncApi(
    UpgradeStateName::Enum state, 
    Common::AsyncOperationSPtr const & operation)
{
    {
        AcquireReadLock grab(lock_);

        ASSERT_IF(state != currentState_, "State transition during async api invoke");
    }

    upgrade_->TraceAsyncOperationEnd(state, *operation, operation->Error);

    return upgrade_->ExitAsyncOperationState(state, operation);
}

void UpgradeStateMachine::CancelOperationIfNotNull(
    UpgradeStateName::Enum state, 
    AsyncOperationSPtr const & op)
{
    if (op != nullptr)
    {
        upgrade_->TraceAsyncOperationCancel(state, *op);
        op->Cancel();
    }
}

void UpgradeStateMachine::CleanupTimer()
{
    if (timer_)
    {
        timer_->Cancel();
        timer_.reset();
    }
}

UpgradeStateDescription const & UpgradeStateMachine::GetState(UpgradeStateName::Enum state)
{
    return upgrade_->GetStateDescription(state);
}

UpgradeStateDescription const & UpgradeStateMachine::GetCurrentState()
{
    return GetState(currentState_);
}

void UpgradeStateMachine::MarkClosed()
{
    lifeCycleState_ = LifeCycleState::Closed;
    upgrade_->TraceLifeCycle(UpgradeLifeCycle::Closed);
}

void UpgradeStateMachine::AssertIfCancelling() const
{
    ASSERT_IF(IsCancelling, "Queued cancellation not supported");
}
