// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestTransactionManager final
        : public TxnReplicator::ITransactionManager
        , public KObject<TestTransactionManager>
        , public KShared<TestTransactionManager>
        , public KWeakRefType<TestTransactionManager>
    {
        K_FORCE_SHARED(TestTransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
        K_SHARED_INTERFACE_IMP(ITransactionManager)
        K_WEAKREF_INTERFACE_IMP(ITransactionManager, TestTransactionManager)

    public:

        static SPtr TestTransactionManager::Create(
            __in Data::LoggingReplicator::TransactionManager & innerTransactionManager,
            __in KAllocator & allocator);

        FABRIC_REPLICA_ROLE get_Role() const override;

        NTSTATUS CreateTransaction(__out KSharedPtr<TxnReplicator::Transaction> & transaction) noexcept override;
        
        NTSTATUS CreateAtomicOperation(__out KSharedPtr<TxnReplicator::AtomicOperation> & atomicOperation) noexcept override;
        
        NTSTATUS CreateAtomicRedoOperation(KSharedPtr<TxnReplicator::AtomicRedoOperation> & atomicRedoOperation) noexcept override;
        
        NTSTATUS SingleOperationTransactionAbortUnlock(
            __in LONG64 stateProviderId,
            __in TxnReplicator::OperationContext const & operationContext) noexcept override;
        
        NTSTATUS ErrorIfStateProviderIsNotRegistered(
            __in LONG64 stateProviderId,
            __in LONG64 transactionId) noexcept override;
        
        // Transaction operations
        NTSTATUS BeginTransaction(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

        ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept override;

        NTSTATUS AddOperation(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in TxnReplicator::AtomicOperation & atomicOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept override;

        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out LONG64 & lsn) noexcept override;

        ktl::Awaitable<NTSTATUS> CommitTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __out LONG64 & commitLsn) noexcept override;

        ktl::Awaitable<NTSTATUS> AbortTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __out LONG64 & abortLsn) noexcept override;

        ktl::Awaitable<NTSTATUS> RegisterAsync(__out LONG64 & lsn) noexcept override;

        NTSTATUS UnRegister(LONG64 visibilityVersionNumber) noexcept override;

    private: 

        TestTransactionManager(__in Data::LoggingReplicator::TransactionManager & innerTransctionManager);

        Data::LoggingReplicator::TransactionManager::SPtr innerTransactionManager_;
    };
}
