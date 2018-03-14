// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // <summary>
    // Transaction State machine
    // 
    // The accepted state transitions are:
    // 
    // Active  ->Reading       ->Active
    // 
    // Active  ->Committing    ->Committed     (Happy path)
    // 
    //                         ->Active        (Can only happen for atomic or atomicredo operations)
    //                     
    //                         ->Faulted       (When commit fails due to terminal exception like object closed or not primary)
    //
    //                         ->Active        (When commit is timed out or cancelled)
    //                         
    //                     
    // Active  ->Aborting      ->Aborted       (Happy path for abort)
    //       
    //                         ->Faulted       (When abort fails due to terminal exception like object closed or not primary)
    //                         
    // </summary>
    class TransactionStateMachine
        : public KObject<TransactionStateMachine>
        , public KShared<TransactionStateMachine>
    {
        K_FORCE_SHARED(TransactionStateMachine)

    public:

        static TransactionStateMachine::SPtr Create(
            __in LONG64 transactionId,
            __in KAllocator & allocator);

        __declspec(property(get = get_State)) TransactionState::Enum State;
        TransactionState::Enum get_State()
        {
            TransactionState::Enum result = TransactionState::Aborted;
            K_LOCK_BLOCK(stateLock_)
            {
                result = state_;
            }

            return result;
        }

        __declspec(property(get = get_AbortedReason)) AbortReason::Enum AbortReason;
        AbortReason::Enum get_AbortedReason()
        {
            AbortReason::Enum result = AbortReason::Invalid;
            K_LOCK_BLOCK(stateLock_)
            {
                result = abortReason_;
            }

            return result;
        }
        
        template<typename AddOperationAction>
        NTSTATUS OnAddOperation(__in AddOperationAction addOperationAction) noexcept
        {
            NTSTATUS status = STATUS_SUCCESS;

            K_LOCK_BLOCK(stateLock_)
            {
                if (state_ != TransactionState::Enum::Active)
                {
                    status = ValidateStateCallerHoldsLock();

                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }

                status = addOperationAction();
            }

            return status;
        }

        template<typename UnregisterAction>
        NTSTATUS OnUserDisposeBeginAbort(__in UnregisterAction unregisterAction) noexcept
        {
            NTSTATUS status = STATUS_SUCCESS;

            K_LOCK_BLOCK(stateLock_)
            {
                ASSERT_IFNOT(
                    state_ != TransactionState::Enum::Committing && state_ != TransactionState::Enum::Reading,
                    "Transaction state during OnUserDisposeBeginAbort cannot be Committing or Reading for transaction id {0}. It is {1}",
                    txId_,
                    state_);

                unregisterAction(); //do the unregister first

                if (state_ != TransactionState::Enum::Active)
                {
                    status = ValidateStateCallerHoldsLock();

                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }

                state_ = TransactionState::Enum::Aborting;
                abortReason_ = AbortReason::Enum::UserDisposed;
            }

            return status;
        }

        NTSTATUS OnBeginAtomicOperationAddAsync() noexcept;
        void OnAtomicOperationSuccess() noexcept;
        void OnAtomicOperationFaulted() noexcept;
        void OnAtomicOperationRetry() noexcept;
        NTSTATUS OnBeginRead() noexcept;
        void OnEndRead() noexcept;
        NTSTATUS OnBeginCommit() noexcept;
        void OnCommitTimedoutOrCancelled() noexcept;
        void OnCommitFaulted() noexcept;
        void OnCommitSuccessful() noexcept;
        NTSTATUS OnUserBeginAbort() noexcept;

        NTSTATUS OnSystemInternallyAbort() noexcept;
        void OnAbortFaulted() noexcept;
        void OnAbortSuccessful() noexcept;

    private:

        TransactionStateMachine(__in LONG64 transactionId);

        NTSTATUS ValidateStateCallerHoldsLock() noexcept;

        LONG64 const txId_;
        KSpinLock stateLock_;
        TransactionState::Enum state_;
        AbortReason::Enum abortReason_;
    };
}
