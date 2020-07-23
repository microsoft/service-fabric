// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class MockLoggingReplicator : 
        public KAsyncServiceBase, 
        public KWeakRefType<MockLoggingReplicator>,
        public TxnReplicator::ILoggingReplicator
    {
        K_FORCE_SHARED(MockLoggingReplicator)
        K_SHARED_INTERFACE_IMP(IDisposable)
        K_SHARED_INTERFACE_IMP(ITransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
        K_SHARED_INTERFACE_IMP(ILoggingReplicator)
        K_SHARED_INTERFACE_IMP(IVersionManager)
        K_WEAKREF_INTERFACE_IMP(ILoggingReplicator, MockLoggingReplicator)
        K_WEAKREF_INTERFACE_IMP(ITransactionManager, MockLoggingReplicator)

    public:
        static SPtr Create(__in bool hasPersistedState, __in KAllocator & allocator);

    public:
        void Initialize(__in TxnReplicator::IStateProviderManager & stateManager);

    public: // ITransactionManager APIs
        bool IsReadable() noexcept override;
        void SetReadable(bool value);

        void Dispose() override;

        NTSTATUS GetLastStableSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER & stableLSN) noexcept override;

        NTSTATUS GetLastCommittedSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept override;

        NTSTATUS GetCurrentEpoch(
            __out FABRIC_EPOCH & epoch) noexcept override;

        __declspec(property(get = get_Role, put = put_Role)) FABRIC_REPLICA_ROLE Role;
        FABRIC_REPLICA_ROLE get_Role() const
        {
            return role_;
        }

        void put_Role(FABRIC_REPLICA_ROLE newRole)
        {
            role_ = newRole;
        }

        __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
        virtual bool get_HasPersistedState() const override
        {
            return hasPersistedState_;
        }

        ktl::Awaitable<NTSTATUS> OpenAsync(
            __out TxnReplicator::RecoveryInformation & recoveryInformation) noexcept override;

        ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(
            TxnReplicator::RecoveryInformation const & recoveryInfo) noexcept override;

        ktl::Awaitable<NTSTATUS> BecomeActiveSecondaryAsync(
            StateManagerBecomeSecondaryDelegate stateManagerBecomeSecondaryDelegate) noexcept override;

        ktl::Awaitable<NTSTATUS> BecomeIdleSecondaryAsync() noexcept override;

        ktl::Awaitable<NTSTATUS> BecomeNoneAsync() noexcept override;

        ktl::Awaitable<NTSTATUS> BecomePrimaryAsync(bool isForRestore) noexcept override;

        ktl::Awaitable<NTSTATUS> DeleteLogAsync() noexcept override;

        ktl::Awaitable<NTSTATUS> CloseAsync() noexcept override;

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __out TxnReplicator::BackupInfo & backupInfo) noexcept override;

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken,
            __out TxnReplicator::BackupInfo & backupInfo) noexcept override;

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder) noexcept override;

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder,
            __in FABRIC_RESTORE_POLICY restorePolicy,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        NTSTATUS GetSafeLsnToRemoveStateProvider(
            __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept override;

        // Create Transaction
        NTSTATUS CreateTransaction(
            __out TxnReplicator::Transaction::SPtr & result) noexcept override;
        NTSTATUS CreateAtomicOperation(
            __out TxnReplicator::AtomicOperation::SPtr & result) noexcept override;
        NTSTATUS CreateAtomicRedoOperation(
            __out TxnReplicator::AtomicRedoOperation::SPtr & result) noexcept override;

        NTSTATUS SingleOperationTransactionAbortUnlock(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in TxnReplicator::OperationContext const & operationContext) noexcept override;

        NTSTATUS ErrorIfStateProviderIsNotRegistered(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in LONG64 transactionId) noexcept override;

        // Transaction operations
        NTSTATUS BeginTransaction(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

        ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;

        NTSTATUS AddOperation(
            __in TxnReplicator::Transaction & transaction,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in TxnReplicator::AtomicOperation & atomicOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;

        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in_opt TxnReplicator::OperationContext const * const operationContext,
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;

        ktl::Awaitable<NTSTATUS> CommitTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;

        ktl::Awaitable<NTSTATUS> AbortTransactionAsync(
            __in TxnReplicator::Transaction & transaction,
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;

        ktl::Awaitable<NTSTATUS> RegisterAsync(
            __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept override;
       
        NTSTATUS UnRegister(
            __in FABRIC_SEQUENCE_NUMBER visibilityVersionNumber) noexcept override;

    public: // IVersionManager
        ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
            __in FABRIC_SEQUENCE_NUMBER checkpointLsnToBeRemoved,
            __in FABRIC_SEQUENCE_NUMBER nextCheckpointLsn) noexcept override;

        NTSTATUS TryRemoveVersion(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in FABRIC_SEQUENCE_NUMBER commitLsn,
            __in FABRIC_SEQUENCE_NUMBER nextCommitLsn,
            __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept override;

    public: // Test methods
        ktl::Awaitable<LONG64> PrepareCheckpointAsync();
        ktl::Awaitable<void> PerformCheckpointAsync(__in ktl::CancellationToken const & cancellationToken);
        ktl::Awaitable<void> CompleteCheckpointAsync(__in ktl::CancellationToken const & cancellationToken);

        void SetReplicationBlocking(__in bool blockReplication);
        void SetSafeLSN(__in FABRIC_SEQUENCE_NUMBER safeLSN);
    
    public: // Notification APIs
        NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;


    private:
        NTSTATUS InjectFaultIfNecessary() noexcept;

    private:
        MockLoggingReplicator(__in bool hasPersistedState);

    private:
        FABRIC_REPLICA_ROLE role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
        volatile LONG64 lastLSN_ = 0;
        volatile FABRIC_SEQUENCE_NUMBER safeLSN_ = 0;
        bool isReadable_ = false;
        bool hasPersistedState_ = true;

    private:
        Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestReplicatedOperation>::SPtr>::SPtr inflightTransactionMapSPtr_;
        Data::Utilities::ReaderWriterAsyncLock::SPtr readerWriterLockSPtr_;
        KWeakRef<TxnReplicator::IStateProviderManager>::SPtr stateManagerWRef_;

    private:
        KString::CSPtr backupFolderPath_;

    private:
        bool isReplicationBlocked_ = false;
    };
}
