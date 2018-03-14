// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface ITransactionManager 
        : public ITransactionVersionManager
    {
        K_SHARED_INTERFACE(ITransactionManager)
        K_WEAKREF_INTERFACE(ITransactionManager)

    public:
        __declspec(property(get = get_Role)) FABRIC_REPLICA_ROLE Role;
        virtual FABRIC_REPLICA_ROLE get_Role() const = 0;

        // Create Transaction
        virtual NTSTATUS CreateTransaction(__out KSharedPtr<Transaction> & transaction) noexcept = 0;
        virtual NTSTATUS CreateAtomicOperation(__out KSharedPtr<AtomicOperation> & atomicOperation) noexcept = 0;
        virtual NTSTATUS CreateAtomicRedoOperation(KSharedPtr<AtomicRedoOperation> & atomicRedoOperation) noexcept = 0;

        virtual NTSTATUS SingleOperationTransactionAbortUnlock(
            __in LONG64 stateProviderId,
            __in OperationContext const & operationContext) noexcept = 0;

        virtual NTSTATUS ErrorIfStateProviderIsNotRegistered(
            __in LONG64 stateProviderId,
            __in LONG64 transactionId) noexcept = 0;

        virtual NTSTATUS BeginTransaction(
            __in Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
            __in Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept = 0;

        virtual NTSTATUS AddOperation(
            __in Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in AtomicOperation & atomicOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in AtomicRedoOperation & atomicRedoOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> CommitTransactionAsync(
            __in Transaction & transaction,
            __out LONG64 & commitLsn) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> AbortTransactionAsync(
            __in Transaction & transaction,
            __out LONG64 & abortLsn) noexcept = 0;
    };
}
