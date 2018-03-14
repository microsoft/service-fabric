// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("StateMachine");

    ReplicatedStore::StateMachine::StateMachine(
        Store::PartitionedReplicaId const & partitionedReplicaId)
        : PartitionedReplicaTraceComponent(partitionedReplicaId)
        , state_(Store::ReplicatedStoreState::Created) 
        , transactionCount_(0)
    { 
    }

    ErrorCode ReplicatedStore::StateMachine::StartTransactionEvent(__out ReplicatedStoreState::Enum & stateResult)
    {
        ErrorCode errorResult;

        ProcessEvent(
            ReplicatedStoreEvent::StartTransaction, 
            [&errorResult, &stateResult](ErrorCode const & error, ReplicatedStoreState::Enum state)
            {
                errorResult = error;
                stateResult = state;
            });

        return errorResult;
    }

    void ReplicatedStore::StateMachine::PostOpenEvent(StateMachineCallback const & callback, ComponentRootSPtr const & root)
    {
        Threadpool::Post([this, root, callback]{ ProcessEvent(ReplicatedStoreEvent::Open, callback); });
    }

    void ReplicatedStore::StateMachine::PostChangeRoleEvent(
        ::FABRIC_REPLICA_ROLE newRole, 
        StateMachineCallback const & callback,
        ComponentRootSPtr const & root)
    {
        ReplicatedStoreEvent::Enum event;
        switch (newRole)
        {
            case ::FABRIC_REPLICA_ROLE_PRIMARY:
                event = ReplicatedStoreEvent::ChangePrimary;
                break;

            case ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            case ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
                event = ReplicatedStoreEvent::ChangeSecondary;
                break;

            default:
                Assert::CodingError(
                    "{0} unrecognized ::FABRIC_REPLICA_ROLE = {1}", 
                    this->TraceId, 
                    static_cast<int>(newRole));
        }

        Threadpool::Post([this, root, event, callback]{ ProcessEvent(event, callback); });
    }

    void ReplicatedStore::StateMachine::PostCloseEvent(StateMachineCallback const & callback, ComponentRootSPtr const & root)
    {
        Threadpool::Post([this, root, callback]{ ProcessEvent(ReplicatedStoreEvent::Close, callback); });
    }

    void ReplicatedStore::StateMachine::FinishTransactionEvent(StateMachineCallback const & callback)
    {
        ProcessEvent(ReplicatedStoreEvent::FinishTransaction, callback);
    }

    void ReplicatedStore::StateMachine::SecondaryPumpClosedEvent(StateMachineCallback const & callback)
    {
        ProcessEvent(ReplicatedStoreEvent::SecondaryPumpClosed, callback);
    }

    void ReplicatedStore::StateMachine::ProcessEvent(ReplicatedStoreEvent::Enum event, StateMachineCallback const & callback)
    {
        ReplicatedStoreEventSource::Trace->QueueEvent(this->PartitionedReplicaId, event);

        {
            AcquireExclusiveLock lock(stateLock_);

            ReplicatedStoreEventSource::Trace->ProcessEvent(this->PartitionedReplicaId, event);

            ErrorCode error;

            switch (event)
            {
                case ReplicatedStoreEvent::Open:
                    error = ProcessOpenEvent();
                    break;

                case ReplicatedStoreEvent::ChangePrimary:
                    error = ProcessChangePrimary();
                    break;

                case ReplicatedStoreEvent::ChangeSecondary:
                    error = ProcessChangeSecondary();
                    break;

                case ReplicatedStoreEvent::StartTransaction:
                    error = ProcessStartTransactionEvent();
                    break;

                case ReplicatedStoreEvent::FinishTransaction:
                    error = ProcessFinishTransactionEvent();
                    break;

                case ReplicatedStoreEvent::SecondaryPumpClosed:
                    error = ProcessSecondaryPumpClosedEvent();
                    break;

                case ReplicatedStoreEvent::Close:
                    error = ProcessCloseEvent();
                    break;

                default:
                    Assert::CodingError(
                        "{0} unrecognized ReplicatedStoreEvent {1}", 
                        this->TraceId, 
                        event);
            }

            // Intentionally call state machine callback under state lock
            // to allow additional processing before the next event is handled
            //
            callback(error, state_);
        }
    }

    bool ReplicatedStore::StateMachine::IsClosed()
    {
        AcquireExclusiveLock lock(stateLock_);

        return (state_ == ReplicatedStoreState::Closed);
    }

    void ReplicatedStore::StateMachine::Abort()
    {
        WriteNoise(
            TraceComponent, 
            "{0} aborting",
            this->TraceId);

        AcquireExclusiveLock lock(stateLock_);

        ChangeState(ReplicatedStoreState::Closed);
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessOpenEvent()
    {
        switch (state_)
        {
            case ReplicatedStoreState::Created:
                ChangeState(ReplicatedStoreState::Opened);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::Closed:
                return ErrorCodeValue::ObjectClosed;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::Open);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessChangePrimary()
    {
        switch (state_)
        {
            case ReplicatedStoreState::Opened:
            case ReplicatedStoreState::SecondaryPassive:
                ChangeState(ReplicatedStoreState::PrimaryPassive);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::SecondaryActive:
                ChangeState(ReplicatedStoreState::SecondaryChangePending);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::Closed:
                return ErrorCodeValue::ObjectClosed;

            case ReplicatedStoreState::PrimaryPassive:
                return ErrorCodeValue::Success;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::ChangePrimary);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessChangeSecondary()
    {
        switch (state_)
        {
            case ReplicatedStoreState::Opened:
            case ReplicatedStoreState::PrimaryPassive:
            case ReplicatedStoreState::SecondaryPassive:
                ChangeState(ReplicatedStoreState::SecondaryActive);
                // Intentional fall-through to return success

            case ReplicatedStoreState::SecondaryActive:
                // Intentional no-op (change secondary Idle->Active)
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::PrimaryActive:
                ChangeState(ReplicatedStoreState::PrimaryChangePending);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::Closed:
                return ErrorCodeValue::ObjectClosed;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::ChangeSecondary);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessStartTransactionEvent()
    {
        switch (state_)
        {
            case ReplicatedStoreState::PrimaryPassive:
                ChangeState(ReplicatedStoreState::PrimaryActive);
                // intentional fall-through to increment transaction count
                
            case ReplicatedStoreState::PrimaryActive:
                ++transactionCount_;
                
                ReplicatedStoreEventSource::Trace->IncrementTransaction(this->PartitionedReplicaId, transactionCount_);

                return ErrorCodeValue::Success;

            case ReplicatedStoreState::Created:
            case ReplicatedStoreState::Opened:
            case ReplicatedStoreState::PrimaryClosePending:
            case ReplicatedStoreState::SecondaryPassive:
            case ReplicatedStoreState::SecondaryActive:
            case ReplicatedStoreState::SecondaryClosePending:
                return ErrorCodeValue::NotPrimary;

            case ReplicatedStoreState::PrimaryChangePending:
            case ReplicatedStoreState::SecondaryChangePending:
                return ErrorCodeValue::ReconfigurationPending;

            case ReplicatedStoreState::Closed:
                return ErrorCodeValue::ObjectClosed;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::StartTransaction);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessFinishTransactionEvent()
    {
        switch (state_)
        {
            case ReplicatedStoreState::PrimaryActive:
                if (DecrementTransactionCount() == 0)
                {
                    ChangeState(ReplicatedStoreState::PrimaryPassive);
                }
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::PrimaryChangePending:
                if (DecrementTransactionCount() == 0)
                {
                    ChangeState(ReplicatedStoreState::SecondaryActive);
                }
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::PrimaryClosePending:
                if (DecrementTransactionCount() == 0)
                {
                    ChangeState(ReplicatedStoreState::Closed);
                }
                return ErrorCodeValue::Success;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::FinishTransaction);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessSecondaryPumpClosedEvent()
    {
        switch (state_)
        {
            case ReplicatedStoreState::SecondaryActive:
                ChangeState(ReplicatedStoreState::SecondaryPassive);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::SecondaryChangePending:
                ChangeState(ReplicatedStoreState::PrimaryPassive);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::SecondaryClosePending:
                ChangeState(ReplicatedStoreState::Closed);
                return ErrorCodeValue::Success;
            case ReplicatedStoreState::Closed:
                return ErrorCodeValue::Success;
            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::SecondaryPumpClosed);
        }
    }

    ErrorCode ReplicatedStore::StateMachine::ProcessCloseEvent()
    {
        switch (state_)
        {
            case ReplicatedStoreState::Opened:
            case ReplicatedStoreState::PrimaryPassive:
            case ReplicatedStoreState::SecondaryPassive:
                ChangeState(ReplicatedStoreState::Closed);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::PrimaryActive:
            case ReplicatedStoreState::PrimaryChangePending:
                ChangeState(ReplicatedStoreState::PrimaryClosePending);
                return ErrorCodeValue::Success;

            case ReplicatedStoreState::SecondaryActive:
            case ReplicatedStoreState::SecondaryChangePending:
                ChangeState(ReplicatedStoreState::SecondaryClosePending);
                return ErrorCodeValue::Success;

            // Abort
            case ReplicatedStoreState::PrimaryClosePending:
            case ReplicatedStoreState::SecondaryClosePending:
            case ReplicatedStoreState::Closed: 
                return ErrorCodeValue::Success;

            default:
                TEST_ASSERT_INVALID_STATE_CHANGE(ReplicatedStoreEvent::Close);
        }
    }

    void ReplicatedStore::StateMachine::ChangeState(ReplicatedStoreState::Enum newState)
    {
        if ((state_ == ReplicatedStoreState::PrimaryPassive && newState == ReplicatedStoreState::PrimaryActive) ||
            (state_ == ReplicatedStoreState::PrimaryActive && newState == ReplicatedStoreState::PrimaryPassive) ||
            (state_ == ReplicatedStoreState::SecondaryPassive && newState == ReplicatedStoreState::SecondaryActive) ||
            (state_ == ReplicatedStoreState::SecondaryActive && newState == ReplicatedStoreState::SecondaryPassive))
        {
            // State changes occur for creating/releasing the first/last transaction and starting/stopping the secondary
            // pump. Do not trace state changes at Info level for these events as they are too noisy
            //
            ReplicatedStoreEventSource::Trace->StateChangeNoise(this->PartitionedReplicaId, state_, newState);
        }
        else
        {
            ReplicatedStoreEventSource::Trace->StateChangeInfo(this->PartitionedReplicaId, state_, newState);
        }

        state_ = newState;
    }

    LONG ReplicatedStore::StateMachine::DecrementTransactionCount()
    {
        --transactionCount_;

        if (transactionCount_ < 0)
        {
            Assert::CodingError(
                "{0} mismatched transaction decrement: count = {1}",
                this->TraceId, 
                transactionCount_);
        }

        ReplicatedStoreEventSource::Trace->DecrementTransaction(this->PartitionedReplicaId, transactionCount_);

        return transactionCount_;
    }
}
