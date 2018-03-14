// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

Transaction::Transaction(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(transactionId, isPrimaryTransaction)
    , isReplicatorAbortingBadTransaction_(false)
    , visibilityLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , firstOperation_()
    , transactionManagerForUnregistering_()
    , commitLatencyWatch_()
{
}

Transaction::Transaction(
    __in ITransactionManager & replicator,
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(replicator, transactionId, isPrimaryTransaction)
    , isReplicatorAbortingBadTransaction_(false)
    , visibilityLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , firstOperation_()
    , transactionManagerForUnregistering_()
    , commitLatencyWatch_()
{
    NTSTATUS status = replicator.GetWeakIfRef(transactionManagerForUnregistering_);
    ASSERT_IFNOT(NT_SUCCESS(status) == true, "Could not get reference to replicator object: {0}", status);
}

Transaction::~Transaction()
{ 
}

Transaction::FirstOperation::FirstOperation(
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext)
    : metaData_(metaData)
    , undo_(undo)
    , redo_(redo)
    , context_(operationContext)
    , stateProviderId_(stateProviderId)
{
}

Transaction::FirstOperation::~FirstOperation()
{
}

Transaction::SPtr Transaction::CreateTransaction(
    __in ITransactionManager & replicator,
    __in KAllocator & allocator)
{
    LONG64 id = TransactionBase::CreateTransactionId();

    bool isPrimaryTx = replicator.Role == FABRIC_REPLICA_ROLE_PRIMARY ?
        true :
        false;

    Transaction* pointer = _new(TRANSACTION_TAG, allocator) Transaction(
        replicator, 
        id, 
        isPrimaryTx);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return Transaction::SPtr(pointer);
}

Transaction::FirstOperation::CSPtr Transaction::FirstOperation::Create(
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const context,
    __in KAllocator & allocator)
{
    Transaction::FirstOperation* pointer = _new(TRANSACTION_TAG, allocator) Transaction::FirstOperation(
        metaData,
        undo,
        redo,
        stateProviderId,
        context);
    
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return Transaction::FirstOperation::CSPtr(pointer);;
}

void Transaction::Dispose()
{
    if (IsDisposed)
    {
        return;
    }

    ASSERT_IFNOT(
        IsValidTransaction == true,
        "IsValidTransaction == true");

    AbortDuringDispose();

    __super::Dispose();
}

NTSTATUS Transaction::AddOperation(
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    auto callback = [&]() {
        return PrivateAddOperation(metaData, undo, redo, stateProviderId, operationContext);
    };

    return StateMachine->OnAddOperation(callback);
}

NTSTATUS Transaction::Abort() noexcept
{
    NTSTATUS status = StateMachine->OnUserBeginAbort();

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Awaitable<void> task = PrivateAbortTask();
    ToTask(task);

    return status;
}

void Transaction::AbortBadTransaction() noexcept
{
    //Ignore any errors as this is best effort as user can be attempting to abort/commit/add operation
    NTSTATUS status = StateMachine->OnSystemInternallyAbort();

    if (!NT_SUCCESS(status))
    {
        return;
    }

    Awaitable<void> task = PrivateAbortTask();
    ToTask(task);
}

NTSTATUS Transaction::AbortDuringDispose() noexcept
{
    auto callback = [&]() {
        if (visibilityLsn_ != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            ITransactionManager::SPtr txMgr;
            NTSTATUS status = GetTransactionManager(txMgr);

            if (!NT_SUCCESS(status))
            {
                return status;
            }

            return txMgr->UnRegister(visibilityLsn_);
        }

        return STATUS_SUCCESS;
    };

    NTSTATUS status = StateMachine->OnUserDisposeBeginAbort(callback);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Awaitable<void> task = PrivateAbortTask();
    ToTask(task);

    return status;
}

Awaitable<NTSTATUS> Transaction::GetVisibilitySequenceNumberAsync(__out FABRIC_SEQUENCE_NUMBER & vsn) noexcept
{
    NTSTATUS status = StateMachine->OnBeginRead();

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await PrivateGetVisibilitySequenceNumberAsync(vsn);

    StateMachine->OnEndRead();

    co_return status;
}

Awaitable<NTSTATUS> Transaction::CommitAsync() noexcept
{
    NTSTATUS status = StateMachine->OnBeginCommit();

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await PrivateCommitAsync(Common::TimeSpan::MaxValue, CancellationToken::None);

    co_return status;
}

Awaitable<NTSTATUS> Transaction::CommitAsync(
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = StateMachine->OnBeginCommit();

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await PrivateCommitAsync(timeout, cancellationToken);

    co_return status;
}

NTSTATUS Transaction::PrivateAddOperation(
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    NTSTATUS result = ErrorIfNotPrimaryTransaction(IsPrimaryTransaction);

    if (!NT_SUCCESS(result))
    {
        return result;
    }

    ITransactionManager::SPtr txMgr;
    result = GetTransactionManager(txMgr);

    if (!NT_SUCCESS(result))
    {
        return result;
    }

    if (IsWriteTransaction == false)
    {
        result = txMgr->ErrorIfStateProviderIsNotRegistered(
            stateProviderId,
            TransactionId);

        if (!NT_SUCCESS(result))
        {
            return result;
        }

        IsWriteTransaction = true;
        firstOperation_ = FirstOperation::Create(
            metaData,
            undo,
            redo,
            stateProviderId,
            operationContext,
            GetThisAllocator());

        return STATUS_SUCCESS;
    }

    if (firstOperation_ != nullptr)
    {
        result = txMgr->BeginTransaction(
            *this,
            firstOperation_->Metadata.RawPtr(),
            firstOperation_->Undo.RawPtr(),
            firstOperation_->Redo.RawPtr(),
            firstOperation_->StateProdiverId,
            firstOperation_->Context.RawPtr());

        if (!NT_SUCCESS(result))
        {
            return result;
        }

        firstOperation_.Reset();
    }

    return txMgr->AddOperation(
        *this,
        metaData,
        undo,
        redo,
        stateProviderId,
        operationContext);
}

Awaitable<void> Transaction::PrivateAbortTask() noexcept
{
    KCoShared$ApiEntry()

    if (IsAtomicOperation == false && IsWriteTransaction == true)
    {
        co_await AbortTransactionAsync();
        co_return;
    }

    StateMachine->OnAbortSuccessful();
    co_return;
}

Awaitable<NTSTATUS> Transaction::AbortTransactionAsync() noexcept
{
    ASSERT_IFNOT(
        IsAtomicOperation == false && IsWriteTransaction == true,
        "Incompatible atomic and write xacts: {0} {1}", IsAtomicOperation, IsWriteTransaction);

    ITransactionManager::SPtr txMgr;
    NTSTATUS status = GetTransactionManager(txMgr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    if (firstOperation_ != nullptr)
    {
        if (firstOperation_->Context != nullptr)
        {
            status = txMgr->SingleOperationTransactionAbortUnlock(
                firstOperation_->StateProdiverId,
                *firstOperation_->Context);

            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "SingleOperationTransactionAbortUnlock is not expected to throw an exception for transaction {0}",
                this->TransactionId);
        }

        firstOperation_.Reset();
        StateMachine->OnAbortSuccessful();
        co_return status;
    }

    do
    {
        LONG64 lsn = 0;
        status = co_await txMgr->AbortTransactionAsync(*this, lsn);

        if (NT_SUCCESS(status))
        {
            StateMachine->OnAbortSuccessful();
            co_return status;
        }

        bool isRetriable = IsRetriableNTSTATUS(status);

        if (!isRetriable)
        {
            // Do not throw exception here as the caller might not be waiting
            // This is because abort is sync from the user's public API
            StateMachine->OnAbortFaulted();
            co_return status;
        }

        status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TRANSACTION_TAG, RetryDelayMilliseconds, nullptr);
        TESTASSERT_IFNOT(NT_SUCCESS(status), "Failed to create a timer during abort delay");

        RetryDelayMilliseconds *= 2;
    } while (true);

    co_return status;
}

Awaitable<NTSTATUS> Transaction::PrivateGetVisibilitySequenceNumberAsync(__out LONG64 & lsn) noexcept
{
    ITransactionManager::SPtr txMgr;
    NTSTATUS status = GetTransactionManager(txMgr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    if (visibilityLsn_ == FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        status = co_await txMgr->RegisterAsync(visibilityLsn_);
    }

    lsn = visibilityLsn_;
    co_return status;
}

Awaitable<NTSTATUS> Transaction::PrivateCommitAsync(
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ITransactionManager::SPtr txMgr;
    NTSTATUS status = GetTransactionManager(txMgr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    if (IsAtomicOperation == false && IsWriteTransaction == true)
    {
        commitLatencyWatch_.Start();

        do
        {
            LONG64 lsn;

            if (firstOperation_ != nullptr)
            {
                status = co_await txMgr->BeginTransactionAsync(
                    *this,
                    firstOperation_->Metadata.RawPtr(),
                    firstOperation_->Undo.RawPtr(),
                    firstOperation_->Redo.RawPtr(),
                    firstOperation_->StateProdiverId,
                    firstOperation_->Context.RawPtr(),
                    lsn);
            }
            else
            {
                status = co_await txMgr->CommitTransactionAsync(*this, lsn);
            }

            if (NT_SUCCESS(status))
            {
                StateMachine->OnCommitSuccessful();
                co_return status;
            }

            bool isRetriable = IsRetriableNTSTATUS(status);

            if (!isRetriable)
            {
                StateMachine->OnCommitFaulted();
                co_return status;
            }

            bool isCancellationRequested = cancellationToken.IsCancellationRequested;
            if (isCancellationRequested ||
                commitLatencyWatch_.Elapsed > timeout)
            {
                StateMachine->OnCommitTimedoutOrCancelled();

                // Reset the commit latency watch in case user retries
                commitLatencyWatch_.Stop();
                commitLatencyWatch_.Reset();

                if (isCancellationRequested)
                {
                    co_return STATUS_CANCELLED;
                }

                co_return SF_STATUS_TIMEOUT;
            }

            status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TRANSACTION_TAG, RetryDelayMilliseconds, nullptr);
            TESTASSERT_IFNOT(NT_SUCCESS(status), "Failed to create a timer during commit delay");

            RetryDelayMilliseconds *= 2;
        } while (true);
    }

    // A readonly transaction or atomic operation is considered committed
    StateMachine->OnCommitSuccessful();

    co_return status;
}
