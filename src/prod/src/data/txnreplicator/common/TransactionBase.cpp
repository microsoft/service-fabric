// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;

ULONG const TransactionBase::MaxRetryDelayMilliseconds = 4096;
LONG64 TransactionBase::lastTransactionId_ = KNt::GetSystemTime();
KSpinLock TransactionBase::transactionIdLock_;

TransactionBase::TransactionBase(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : KObject()
    , KShared()
    , transactionId_(transactionId)
    , isPrimaryTransaction_(isPrimaryTransaction)
    , lockContexts_(GetThisAllocator())
    , transactionManager_()
    , isWriteTransaction_(false)
    , retryDelay_(32)
    , commitLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , transactionManagerLock_()
    , state_(TransactionState::Enum::Active)
    , stateMachine_(TransactionStateMachine::Create(transactionId_, GetThisAllocator()).RawPtr())
    , isDisposed_(false)
{
    THROW_ON_CONSTRUCTOR_FAILURE(lockContexts_);
}

TransactionBase::TransactionBase(
    __in ITransactionManager & replicator,
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : KObject()
    , KShared()
    , transactionId_(transactionId)
    , isPrimaryTransaction_(isPrimaryTransaction)
    , lockContexts_(GetThisAllocator())
    , transactionManager_()
    , isWriteTransaction_(false)
    , retryDelay_(32)
    , commitLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , transactionManagerLock_()
    , state_(TransactionState::Enum::Active)
    , stateMachine_(TransactionStateMachine::Create(transactionId_, GetThisAllocator()).RawPtr())
    , isDisposed_(false)
{
    THROW_ON_CONSTRUCTOR_FAILURE(lockContexts_);

    NTSTATUS status = replicator.GetWeakIfRef(transactionManager_);
    ASSERT_IFNOT(NT_SUCCESS(status), "Could not get reference to replicator object: {0}", status);
}

TransactionBase::~TransactionBase()
{
    if (isPrimaryTransaction_ == true)
    {
        TESTASSERT_IFNOT(isDisposed_, "Transaction {0} not disposed on primary", transactionId_);
    }
    
    if (!isDisposed_)
    {
        for (ULONG i = 0; i < lockContexts_.Count(); i++)
        {
            if (lockContexts_[i]->IsTrackingContext() == false)
            {
                lockContexts_[i]->Unlock();
            }
        }
    }
}

LONG64 TransactionBase::CreateTransactionId()
{
    LONG64 transactionId = KNt::GetSystemTime();

    K_LOCK_BLOCK(transactionIdLock_)
    {
        if (transactionId > lastTransactionId_)
        {
            lastTransactionId_ = transactionId;
        }
        else
        {
            ++lastTransactionId_;
            transactionId = lastTransactionId_;
        }
    }

    return transactionId;
}

TransactionBase::SPtr TransactionBase::CreateTransaction(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction,
    __in KAllocator & allocator)
{
    LONG64 positiveTransactionId;
    TransactionBase::SPtr transaction;

    if (transactionId >= 0)
    {
        positiveTransactionId = transactionId;
        transaction = _new(TRANSACTION_TAG, allocator) Transaction(transactionId, isPrimaryTransaction);
        THROW_ON_ALLOCATION_FAILURE(transaction);
    }
    else
    {
        positiveTransactionId = -transactionId;
        transaction = _new(TRANSACTION_TAG, allocator) AtomicOperation(transactionId, isPrimaryTransaction);
        THROW_ON_ALLOCATION_FAILURE(transaction);
    }

    K_LOCK_BLOCK(transactionIdLock_)
    {
        if (positiveTransactionId > lastTransactionId_)
        {
            lastTransactionId_ = positiveTransactionId;
        }
    }

    return transaction;
}

TransactionState::Enum TransactionBase::get_State()
{
    return stateMachine_.Get()->State;
}

TransactionStateMachine::SPtr TransactionBase::get_StateMachine()
{
    return stateMachine_.Get();
}

NTSTATUS TransactionBase::GetTransactionManager(ITransactionManager::SPtr & result)
{
    ITransactionManager::SPtr local;

    K_LOCK_BLOCK(transactionManagerLock_)
    {
        if (transactionManager_ != nullptr)
        {
            local = transactionManager_->TryGetTarget();
        }
    }

    if (local == nullptr)
    {
        return SF_STATUS_OBJECT_CLOSED;
    }

    result = local;
    return STATUS_SUCCESS;
}

void TransactionBase::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    ASSERT_IFNOT(IsValidTransaction, "Transaction {0} not valid during dispose", transactionId_);

    for (ULONG i = 0; i < lockContexts_.Count(); i++)
    {
        if (lockContexts_[i]->IsTrackingContext() == false)
        {
            lockContexts_[i]->Unlock();
        }
    }
    
    isDisposed_ = true;
}

void TransactionBase::set_CommitSequenceNumber(FABRIC_SEQUENCE_NUMBER value)
{
    ASSERT_IFNOT(
        commitLsn_ == FABRIC_INVALID_SEQUENCE_NUMBER,
        "Commit sequence number can only be set once, current value: {0}", commitLsn_);
    
    ASSERT_IFNOT(value > 0, "Commit sequence number must be positive: {0}", value);

    commitLsn_ = value;
}

bool TransactionBase::IsRetriableException(Exception const & e)
{
    return IsRetriableNTSTATUS(e.GetStatus());
}

bool TransactionBase::IsRetriableNTSTATUS(
    __in NTSTATUS status) noexcept
{
    if (status == SF_STATUS_NO_WRITE_QUORUM ||
        status == SF_STATUS_RECONFIGURATION_PENDING ||
        status == SF_STATUS_REPLICATION_QUEUE_FULL ||
        status == SF_STATUS_SERVICE_TOO_BUSY)
    {
        return true;
    }

    return false;
}

bool TransactionBase::operator==(TransactionBase const &other) const
{
    return
        this->transactionId_ == other.transactionId_ &&
        this->isPrimaryTransaction_ == other.isPrimaryTransaction_;
}

bool TransactionBase::operator>(TransactionBase const &other) const
{
    return this->transactionId_ > other.transactionId_;
}

bool TransactionBase::operator>=(TransactionBase const &other) const
{
    return this->transactionId_ > other.transactionId_;
}

bool TransactionBase::operator<(TransactionBase const &other) const
{
    return !(*this >= other);
}

bool TransactionBase::operator<=(TransactionBase const &other) const
{
    return !(*this > other);
}

bool TransactionBase::operator!=(TransactionBase const &other) const
{
    return !(*this == other);
}

NTSTATUS TransactionBase::AddLockContext(
    __in LockContext & lockContext) noexcept
{
    return lockContexts_.Append(&lockContext);
}

NTSTATUS TransactionBase::ErrorIfNotPrimaryTransaction(bool isPrimaryTransaction)
{
    if (isPrimaryTransaction == false)
    {
        return SF_STATUS_TRANSACTION_NOT_ACTIVE;
    }

    return STATUS_SUCCESS;
}
