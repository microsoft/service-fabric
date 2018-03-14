// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

AtomicOperation::AtomicOperation(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(transactionId, isPrimaryTransaction)
{
}

AtomicOperation::AtomicOperation(
    __in ITransactionManager & replicator,
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(replicator, transactionId, isPrimaryTransaction)
{
}

AtomicOperation::~AtomicOperation()
{
}

AtomicOperation::SPtr AtomicOperation::CreateAtomicOperation(
    __in ITransactionManager & replicator,
    __in KAllocator & allocator)
{
    LONG64 id = TransactionBase::CreateTransactionId();

    bool isPrimaryTx = replicator.Role == FABRIC_REPLICA_ROLE_PRIMARY ?
        true :
        false;

    AtomicOperation* pointer = _new(TRANSACTION_TAG, allocator) AtomicOperation(
        replicator, 
        -id, 
        isPrimaryTx);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return AtomicOperation::SPtr(pointer);
}

AtomicOperation::SPtr AtomicOperation::CreateAtomicOperation(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction,
    __in KAllocator & allocator)
{
    AtomicOperation* pointer = _new(TRANSACTION_TAG, allocator) AtomicOperation(
        transactionId,
        isPrimaryTransaction);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return AtomicOperation::SPtr(pointer);
}

Awaitable<NTSTATUS> AtomicOperation::AddOperationAsync(
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    NTSTATUS status = ErrorIfNotPrimaryTransaction(IsPrimaryTransaction);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    ITransactionManager::SPtr txMgr;
    status = GetTransactionManager(txMgr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = StateMachine->OnBeginAtomicOperationAddAsync();

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    LONG64 lsn;
    status = co_await txMgr->AddOperationAsync(
        *this,
        metaData,
        undo,
        redo,
        stateProviderId,
        operationContext,
        lsn);

    if (NT_SUCCESS(status))
    {
        StateMachine->OnAtomicOperationSuccess();
        co_return status;
    }

    if (IsRetriableNTSTATUS(status))
    {
        StateMachine->OnAtomicOperationRetry();
    }
    else
    {
        StateMachine->OnAtomicOperationFaulted();
    }

    co_return status;
}
