// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace LoggingReplicatorTests;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

#define TESTTRANSACTIONMANAGER_TAG 'MxTT'

TestTransactionManager::TestTransactionManager(__in TransactionManager & innerTransactionManager)
    : KObject()
    , KShared()
    , innerTransactionManager_(&innerTransactionManager)
{
}

TestTransactionManager::~TestTransactionManager()
{
}

TestTransactionManager::SPtr TestTransactionManager::Create(
    __in TransactionManager & innerTransactionManager,
    __in KAllocator& allocator)
{
    TestTransactionManager * pointer = _new(TESTTRANSACTIONMANAGER_TAG, allocator) TestTransactionManager(innerTransactionManager);
    if (pointer == nullptr)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return TestTransactionManager::SPtr(pointer);
}

FABRIC_REPLICA_ROLE TestTransactionManager::get_Role() const
{
    return FABRIC_REPLICA_ROLE_PRIMARY;
}

NTSTATUS TestTransactionManager::CreateTransaction(KSharedPtr<Transaction> & result) noexcept
{
    result = Transaction::CreateTransaction(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::CreateAtomicOperation(KSharedPtr<AtomicOperation> & result) noexcept
{
    result = AtomicOperation::CreateAtomicOperation(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::CreateAtomicRedoOperation(KSharedPtr<AtomicRedoOperation> & result) noexcept
{
    result = AtomicRedoOperation::CreateAtomicRedoOperation(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::SingleOperationTransactionAbortUnlock(
    __in LONG64 stateProviderId,
    __in OperationContext const& operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(operationContext);
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::ErrorIfStateProviderIsNotRegistered(
    __in LONG64 stateProviderId,
    __in LONG64 transactionId) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(transactionId);
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::BeginTransaction(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    return innerTransactionManager_->BeginTransaction(
        transaction,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> TestTransactionManager::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & lsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    co_return co_await innerTransactionManager_->BeginTransactionAsync(
        transaction, 
        metaData, 
        undo, 
        redo, 
        operationContext,
        lsn);
}

NTSTATUS TestTransactionManager::AddOperation(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    return innerTransactionManager_->AddOperation(
        transaction, 
        metaData, 
        undo, 
        redo, 
        operationContext);
}

Awaitable<NTSTATUS> TestTransactionManager::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & lsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
 
    co_return co_await innerTransactionManager_->AddOperationAsync(
        atomicOperation, 
        metaData, 
        undo, 
        redo, 
        operationContext,
        lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & lsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    co_return co_await innerTransactionManager_->AddOperationAsync(
        atomicRedoOperation, 
        metaData, 
        redo, 
        operationContext,
        lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::CommitTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & commitLsn) noexcept
{
    co_return co_await innerTransactionManager_->CommitTransactionAsync(transaction, commitLsn);
}

Awaitable<NTSTATUS> TestTransactionManager::AbortTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & abortLsn) noexcept
{
    co_return co_await innerTransactionManager_->AbortTransactionAsync(transaction, abortLsn);
}

Awaitable<NTSTATUS> TestTransactionManager::RegisterAsync(__out LONG64 & lsn) noexcept
{
    lsn = -1;
    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS TestTransactionManager::UnRegister(LONG64 visibilityVersionNumber) noexcept
{
    UNREFERENCED_PARAMETER(visibilityVersionNumber);
    return STATUS_NOT_IMPLEMENTED;
}
