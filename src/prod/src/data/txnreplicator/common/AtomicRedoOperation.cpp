// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

AtomicRedoOperation::AtomicRedoOperation(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(transactionId, isPrimaryTransaction)
{
}

AtomicRedoOperation::AtomicRedoOperation(
    __in ITransactionManager & replicator,
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction)
    : TransactionBase(replicator, transactionId, isPrimaryTransaction)
{
}

AtomicRedoOperation::~AtomicRedoOperation()
{
}

AtomicRedoOperation::SPtr AtomicRedoOperation::CreateAtomicRedoOperation(
    __in ITransactionManager & replicator,
    __in KAllocator & allocator)
{
    LONG64 id = TransactionBase::CreateTransactionId();

    bool isPrimaryTx = replicator.Role == FABRIC_REPLICA_ROLE_PRIMARY ?
        true :
        false;

    AtomicRedoOperation* pointer = _new(TRANSACTION_TAG, allocator) AtomicRedoOperation(
        replicator, 
        -id, 
        isPrimaryTx);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return AtomicRedoOperation::SPtr(pointer);
}

AtomicRedoOperation::SPtr AtomicRedoOperation::CreateAtomicRedoOperation(
    __in LONG64 transactionId,
    __in bool isPrimaryTransaction,
    __in KAllocator & allocator)
{
    ASSERT_IFNOT(transactionId < 0, "Invalid xact id creating atomic redo op: {0}", transactionId);

    AtomicRedoOperation* pointer = _new(TRANSACTION_TAG, allocator) AtomicRedoOperation(
        transactionId, 
        isPrimaryTransaction);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return AtomicRedoOperation::SPtr(pointer);
}

Awaitable<NTSTATUS> AtomicRedoOperation::AddOperationAsync(
    __in_opt OperationData const * const metaData,
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
        redo,
        stateProviderId,
        operationContext,
        lsn);

    if (NT_SUCCESS(status))
    {
        StateMachine->OnAtomicOperationSuccess();

        co_return status;
    }
    
    if(IsRetriableNTSTATUS(status))
    {
        StateMachine->OnAtomicOperationRetry();
    }
    else
    {
        StateMachine->OnAtomicOperationFaulted();
    }

    co_return status;
}
