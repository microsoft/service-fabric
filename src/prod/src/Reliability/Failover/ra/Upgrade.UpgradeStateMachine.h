// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            
            // The upgrade state machine
            // It always starts in state None and moves to the start state
            // It continues executing the state machine until a transition to completed is seen
            // It supports cancellation - cancellation will transition 
            // the state machine to Closed either immediately if the current state is a timer state
            // or after the current state completes execution 
            // If the current state is executing (i.e. async/sync execution) 
            // then after the execution of the current state completes the cancel completion callback will be executed
            // and the state machine will be closed
            // Cancellation will either Close the state machine or invoke the cancel completion callback once cancellation is completed
            // Close will transition to the Closed state 
            // SendReply will invoke the ResendReply method on the IUpgrade object once the upgrade is complete
            class UpgradeStateMachine : public std::enable_shared_from_this<UpgradeStateMachine>
            {
                DENY_COPY(UpgradeStateMachine);

            public:
                typedef std::function<void (std::shared_ptr<UpgradeStateMachine> const &)> CancelCompletionCallback;

                UpgradeStateMachine(
                    IUpgradeUPtr && upgrade,
                    ReconfigurationAgent & ra);

                __declspec(property(get=get_CurrentState)) UpgradeStateName::Enum CurrentState;
                UpgradeStateName::Enum get_CurrentState() const 
                { 
                    Common::AcquireReadLock grab(lock_);
                    return currentState_; 
                }

                __declspec(property(get=get_Timer)) Common::TimerSPtr const & TimerObj;
                Common::TimerSPtr const & get_Timer() const { return timer_; }

                __declspec(property(get=get_IsClosed)) bool IsClosed;
                bool get_IsClosed() const {  return lifeCycleState_ == LifeCycleState::Closed;}

                __declspec(property(get=get_IsCancelling)) bool IsCancelling;
                bool get_IsCancelling() const { return lifeCycleState_ == LifeCycleState::Cancelling; }

                __declspec(property(get = get_Test_AsyncOp)) Common::AsyncOperationSPtr Test_AsyncOp;
                Common::AsyncOperationSPtr get_Test_AsyncOp() const { return asyncOp_;; }

                __declspec(property(get=get_ActivityId)) std::wstring const & ActivityId;
                std::wstring const & get_ActivityId() const { return upgrade_->GetActivityId(); }

                __declspec(property(get=get_InstanceId)) uint64 InstanceId;
                uint64 get_InstanceId() const { return upgrade_->InstanceId; }

                __declspec(property(get=get_Upgrade)) IUpgrade& Upgrade;
                IUpgrade& get_Upgrade() { return *upgrade_; }

                void Start(RollbackSnapshotUPtr && rollbackSnapshot);

                // Close will transition the object to Closed once the current state completes
                // If the current state is a retry state it will disarm the timer                
                void Close();

                // This method should be called if a newer upgrade has been received or a cancel message is received
                // It will return true if the current state is None and transition to Closed
                // It will return true if the current state is Completed and transition to Closed
                // It will return true if the current state is Closed
                // For Cancellations: the current upgrade state must be a state at which cancellation is allowed
                // otherwise the request will return NotAllowed
                // For Rollback: if the current upgrade state supports cancellation then it is allowed immediately and return is success
                // otherwise the callback will be executed once the current state completes and return is queued
                // if rollback is not permitted then the return value is NotAllowed
                // In general:
                // If the current state is a retry state then it will disarm the retry timer and transition to Closed 
                // If the current state is either a sync or async state and cancellable it will transition to Closed 
                // If the current state is either a sync or async state and not cancellable it will execute the callback once the current state completes
                UpgradeCancelResult::Enum TryCancelUpgrade(UpgradeCancelMode::Enum cancelMode, CancelCompletionCallback callback);

                __declspec(property(get=get_IsCompleted)) bool IsCompleted;
                bool get_IsCompleted() 
                { 
                    Common::AcquireReadLock grab(lock_);
                    return currentState_ == UpgradeStateName::Completed && !IsClosed;
                }

                RollbackSnapshotUPtr CreateRollbackSnapshot();

                void SendReply();

                // INternal method - exposed for easier unit testing
                // DO NOT CALL THIS
                void OnTimer();

                static UpgradeStateMachineSPtr Create(IUpgradeUPtr && upgrade, ReconfigurationAgent & ra)
                {
                    return std::make_shared<UpgradeStateMachine>(std::move(upgrade), ra);
                }

            private:
                UpgradeCancelResult::Enum DecideCancelAction(UpgradeCancelMode::Enum cancelMode);
                
                void UpdateStateForCancelAction(
                    UpgradeCancelResult::Enum action, 
                    CancelCompletionCallback callback,
                    Common::AsyncOperationSPtr & operationToCancel);

                UpgradeStateDescription const & GetState(UpgradeStateName::Enum state);

                UpgradeStateDescription const & GetCurrentState();

                void ProcessNextState(UpgradeStateName::Enum state);

                UpgradeStateName::Enum EnterState(
                    UpgradeStateName::Enum state,
                    Common::AsyncOperationSPtr & operationToCancel);

                UpgradeStateName::Enum EnterTimerState(
                    UpgradeStateDescription const & stateObj,
                    Common::AsyncOperationSPtr & operationToCancel);

                UpgradeStateName::Enum EnterAsyncApiState(
                    UpgradeStateDescription const &,
                    Common::AsyncOperationSPtr & operationToCancel);

                UpgradeStateName::Enum EnterNormalState(
                    UpgradeStateDescription const &,
                    Common::AsyncOperationSPtr & operationToCancel);

                void CancelOperationIfNotNull(
                    UpgradeStateName::Enum,
                    Common::AsyncOperationSPtr const &);

                void CleanupTimer();

                void MarkClosed();

                void AssertIfCancelling() const;

                UpgradeStateName::Enum FinishAsyncApi(UpgradeStateName::Enum currentState, Common::AsyncOperationSPtr const &);

                ReconfigurationAgent & ra_;
                Common::RwLock mutable lock_;
                Common::TimerSPtr timer_;
                UpgradeStateName::Enum currentState_;
                IUpgradeUPtr upgrade_;

                class LifeCycleState
                {
                public:
                    enum Enum
                    {
                        Open = 0,
                        Closed = 1,
                        Cancelling = 2,
                    };
                };

                LifeCycleState::Enum lifeCycleState_;
                CancelCompletionCallback callback_;
                Common::AsyncOperationSPtr asyncOp_;
            };
        };
    }
}

