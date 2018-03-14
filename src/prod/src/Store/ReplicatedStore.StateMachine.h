// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    // 
    // State Machine
    //
    // States:
    //   Created:  Replica not yet opened.
    //   Opened:   Role=none replica.
    //   1Passive: Primary with no active calls.
    //   1Active:  Active primary, i.e. at least one active transaction.
    //   1AChange: Active primary with a pending role change.
    //   1AClose:  Active primary with a pending close.
    //   2Passive: Secondary that has fully drained replicator.
    //   2Active:  Active secondary, i.e. processing op or waiting for GetOperation.
    //   2AChange: Active secondary with a pending role change.
    //   2AClose:  Active secondary with a pending close.
    //   Closed:   Replica closed.
    //
    // Events:
    //   Open:       BeginOpen call.
    //   Change1:    BeginChangeRole (Primary) call.
    //   Change2:    BeginChangeRole (Secondary) call.
    //   +Tx:        CreateTransaction call.
    //   -Tx:        Transaction completed rollback or commit.
    //   NullOp:     replicator_->GetOperation returned NULL.
    //   Close:      BeginClose call.
    //
    // Errors:
    //   NotPrimary: The replicated store is not in the primary role
    //   ReconfigurationPending: A role transition is pending
    //   ObjectClosed: State machine is already in the closed or close pending state
    //
    // Notes:
    //   *: The state machine keeps an internal count of transactions. 
    //      The 1Active->1Passive/2Active/Closed transition only occurs when this count reaches 0. 
    //      Otherwise, the state remains unchanged.
    //
    //
    //          | Open   | Change1     | Change2     | +Tx      | -Tx       | NullOp    | Close
    // --------------------------------------------------------------------------------------------
    // Created  | Opened | x           | x           | error    | x         | x         | x
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // Opened   | x      | 1Passive    | 2Active     | error    | x         | x         | Closed
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // 1Passive | x      | x           | 2Active     | 1Active  | x         | x         | Closed
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // 1Active  | x      | x           | 1AChange    | 1Active  | 1Active   | x         | 1AClose
    //          |        |             |             |          | 1Passive* |           |
    // --------------------------------------------------------------------------------------------
    // 1AChange | x      | x           | x           | error    | 1AChange  | x         | 1AClose
    //          |        |             |             |          | 2Active*  |           |
    // --------------------------------------------------------------------------------------------
    // 1AClose  | x      | x           | x           | error    | 1AClose   | x         | 1AClose
    //          |        |             |             |          | Closed*   |           |
    // --------------------------------------------------------------------------------------------
    // 2Passive | x      | 1Passive    | 2Active     | error    | x         | x         | Closed
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // 2Active  | x      | 2AChange    | 2Active     | error    | x         | 2Passive  | 2AClose
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // 2AChange | x      | x           | x           | error    | x         | 1Passive  | 2AClose
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // 2AClose  | x      | x           | x           | error    | x         | Closed    | 2AClose
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    // Closed   | error  | error       | error       | error    | x         | x         | Closed
    //          |        |             |             |          |           |           |
    // --------------------------------------------------------------------------------------------
    //
    
#define TEST_ASSERT_INVALID_STATE_CHANGE( ReplicatedStoreEvent ) \
    { \
        TRACE_ERROR_AND_TESTASSERT( \
            TraceComponent, \
            "{0} invalid state change: state = {1} event = {2}", \
            this->TraceId, \
            state_, \
            ReplicatedStoreEvent); \
        return ErrorCodeValue::InvalidState; \
    } \

    class ReplicatedStore::StateMachine
        : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        typedef std::function<void(Common::ErrorCode const &, ReplicatedStoreState::Enum)> StateMachineCallback; 

        StateMachine(Store::PartitionedReplicaId const &);

        __declspec(property(get=get_TransactionCount)) LONG TransactionCount;
        LONG get_TransactionCount() const { return transactionCount_; }

        __declspec(property(get=get_StateLock)) Common::ExclusiveLock & StateLock;
        Common::ExclusiveLock & get_StateLock() { return stateLock_; }

        bool IsClosed();

        void Abort();
        Common::ErrorCode StartTransactionEvent(__out ReplicatedStoreState::Enum &);
        void PostOpenEvent(StateMachineCallback const &, Common::ComponentRootSPtr const &);
        void PostChangeRoleEvent(::FABRIC_REPLICA_ROLE, StateMachineCallback const &, Common::ComponentRootSPtr const &);
        void PostCloseEvent(StateMachineCallback const &, Common::ComponentRootSPtr const &);
        void FinishTransactionEvent(StateMachineCallback const &);
        void SecondaryPumpClosedEvent(StateMachineCallback const &);

    private:

        void ProcessEvent(ReplicatedStoreEvent::Enum, StateMachineCallback const &);
        Common::ErrorCode ProcessOpenEvent();
        Common::ErrorCode ProcessChangePrimary();
        Common::ErrorCode ProcessChangeSecondary();
        Common::ErrorCode ProcessStartTransactionEvent();
        Common::ErrorCode ProcessFinishTransactionEvent();
        Common::ErrorCode ProcessSecondaryPumpClosedEvent();
        Common::ErrorCode ProcessCloseEvent();

        void ChangeState(Store::ReplicatedStoreState::Enum);
        LONG DecrementTransactionCount();

        Store::ReplicatedStoreState::Enum state_;
        LONG transactionCount_;
        RWLOCK(StoreStateMachine, stateLock_);
    };
}
