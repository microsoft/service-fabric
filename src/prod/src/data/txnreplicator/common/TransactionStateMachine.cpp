// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;

TransactionStateMachine::SPtr TransactionStateMachine::Create(
    __in LONG64 transactionId,
    __in KAllocator & allocator)
{
    TransactionStateMachine * pointer = _new(TRANSACTION_TAG, allocator) TransactionStateMachine(transactionId);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TransactionStateMachine::SPtr(pointer);
}

TransactionStateMachine::TransactionStateMachine(__in LONG64 transactionId)
    : KObject()
    , KShared()
    , txId_(transactionId)
    , state_(TransactionState::Enum::Active)
    , stateLock_()
    , abortReason_(AbortReason::Enum::Invalid)
{
}

TransactionStateMachine::~TransactionStateMachine()
{
}

NTSTATUS TransactionStateMachine::OnBeginAtomicOperationAddAsync() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        if (state_ != TransactionState::Enum::Active)
        {
            NTSTATUS status = ValidateStateCallerHoldsLock();

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        state_ = TransactionState::Enum::Committing;
    }

    return STATUS_SUCCESS;
}

void TransactionStateMachine::OnAtomicOperationSuccess() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transcation state during OnAtomicOperationSuccess must be Committing. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Committed;
    }
}

void TransactionStateMachine::OnAtomicOperationFaulted() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transcation state during OnAtomicOperationFaulted must be Committing. It is {0} for transcation id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Faulted;
    }
}

void TransactionStateMachine::OnAtomicOperationRetry() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transaction state during OnAtomicOperationRetry must be Committing. It is {0} for transaction id {1}",
            state_,
            txId_);

        TransactionState::Enum::Active;
    }
}

NTSTATUS TransactionStateMachine::OnBeginRead() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        if (state_ != TransactionState::Enum::Active)
        {
            NTSTATUS status = ValidateStateCallerHoldsLock();

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        state_ = TransactionState::Enum::Reading;
    }

    return STATUS_SUCCESS;
}

void TransactionStateMachine::OnEndRead() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Reading,
            "Transaction state during OnEndRead must be Reading. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Active;
    }
}

NTSTATUS TransactionStateMachine::OnBeginCommit() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        if (state_ != TransactionState::Enum::Active)
        {
            NTSTATUS status = ValidateStateCallerHoldsLock();

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        state_ = TransactionState::Enum::Committing;
    }

    return STATUS_SUCCESS;
}

void TransactionStateMachine::OnCommitTimedoutOrCancelled() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transaction state during OnCommitTimedoutOrCancelled must be Committing. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Active;
    }
}

void TransactionStateMachine::OnCommitFaulted() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transaction state during OnCommitFaulted must be Committing. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Faulted;
    }
}

void TransactionStateMachine::OnCommitSuccessful() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Committing,
            "Transaction state during OnCommitSuccessful must be Committing. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Committed;
    }
}

NTSTATUS TransactionStateMachine::OnUserBeginAbort() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        if (state_ != TransactionState::Enum::Active)
        {
            NTSTATUS status = ValidateStateCallerHoldsLock();

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        state_ = TransactionState::Enum::Aborting;
        abortReason_ = AbortReason::Enum::UserAborted;
    }

    return STATUS_SUCCESS;
}

NTSTATUS TransactionStateMachine::OnSystemInternallyAbort() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        if (state_ != TransactionState::Enum::Active)
        {
            NTSTATUS status = ValidateStateCallerHoldsLock();

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        state_ = TransactionState::Enum::Aborting;
        abortReason_ = AbortReason::Enum::SystemAborted;
    }

    return STATUS_SUCCESS;
}

void TransactionStateMachine::OnAbortFaulted() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Aborting,
            "Transaction state during OnAbortFaulted must be Aborting. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Faulted;
    }
}

void TransactionStateMachine::OnAbortSuccessful() noexcept
{
    K_LOCK_BLOCK(stateLock_)
    {
        ASSERT_IFNOT(
            state_ == TransactionState::Enum::Aborting,
            "Transaction state during OnAbortSuccessful must be Aborting. It is {0} for transaction id {1}",
            state_,
            txId_);

        state_ = TransactionState::Enum::Aborted;
    }
}

NTSTATUS TransactionStateMachine::ValidateStateCallerHoldsLock() noexcept
{
    switch (state_)
    {
    case TransactionState::Enum::Active:
        ASSERT_IFNOT(false, "Transaction is active. Cannot throw exception");
        break;
    case TransactionState::Enum::Reading:
        return SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED;
    case TransactionState::Enum::Committing:
    case TransactionState::Enum::Committed:
        return SF_STATUS_TRANSACTION_NOT_ACTIVE;
    case TransactionState::Enum::Aborting:
    case TransactionState::Enum::Aborted:
    case TransactionState::Enum::Faulted:
        switch (abortReason_)
        {
        case AbortReason::Enum::Invalid:
        case AbortReason::Enum::UserAborted:
        case AbortReason::Enum::UserDisposed:
            return SF_STATUS_TRANSACTION_NOT_ACTIVE;
        case AbortReason::Enum::SystemAborted:
            return SF_STATUS_TRANSACTION_ABORTED; 
        }
        ASSERT_IFNOT(false, "Unreachable codepath was reached.");
        break;
    }

    return STATUS_SUCCESS;
}
