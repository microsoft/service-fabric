// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestTransactionManager
        : public KObject<TestTransactionManager>
        , public KShared<TestTransactionManager>
        , public KWeakRefType<TestTransactionManager>
        , public TxnReplicator::ITransactionalReplicator
    {
        K_FORCE_SHARED(TestTransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionalReplicator)
        K_SHARED_INTERFACE_IMP(IStateProviderMap)
        K_SHARED_INTERFACE_IMP(ITransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
        K_SHARED_INTERFACE_IMP(IVersionManager)
        K_WEAKREF_INTERFACE_IMP(ITransactionManager, TestTransactionManager)
        K_WEAKREF_INTERFACE_IMP(ITransactionalReplicator, TestTransactionManager)

    public:
        static SPtr TestTransactionManager::Create(__in KAllocator & allocator);

    public:
        __declspec(property(get = get_StateManager)) Data::StateManager::IStateManager::SPtr StateManager;
        __declspec(noinline)
        Data::StateManager::IStateManager::SPtr get_StateManager() const
        {
            return stateManagerSPtr_.RawPtr();
        }

        __declspec(property(get = get_Role)) FABRIC_REPLICA_ROLE Role;
        FABRIC_REPLICA_ROLE get_Role() const
        {
            return role_;
        }

        bool get_IsReadable() const override
        {
            throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
        }

        __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const override
        {
            return loggingReplicatorSPtr_->HasPersistedState;
        }

    public: // ITransactionalReplicator interface.
        Data::Utilities::IStatefulPartition::SPtr get_StatefulPartition() const override
        {
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS GetLastStableSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER & stableLSN) noexcept override
        {
            stableLSN = FABRIC_INVALID_SEQUENCE_NUMBER;

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS GetLastCommittedSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept override
        {
            return loggingReplicatorSPtr_->GetLastCommittedSequenceNumber(commitLSN);
        }

        NTSTATUS GetCurrentEpoch(
         __out FABRIC_EPOCH & epoch) noexcept override
        {
            UNREFERENCED_PARAMETER(epoch);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        NTSTATUS RegisterStateManagerChangeHandler(
            __in TxnReplicator::IStateManagerChangeHandler & stateManagerChangeHandler) noexcept override;

        NTSTATUS UnRegisterStateManagerChangeHandler() noexcept override;

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

    public: // IStateProviderMap interface.
        NTSTATUS Get(
            __in KUriView const & stateProviderName,
            __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

        ktl::Awaitable<NTSTATUS> AddAsync(
            __in TxnReplicator::Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProvider,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        ktl::Awaitable<NTSTATUS> RemoveAsync(
            __in TxnReplicator::Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        NTSTATUS CreateEnumerator(
            __in bool parentsOnly,
            __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept override;

        ktl::Awaitable<NTSTATUS> GetOrAddAsync(
            __in TxnReplicator::Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProviderTypeName,
            __out TxnReplicator::IStateProvider2::SPtr & stateProvider,
            __out bool & alreadyExist,
            __in_opt Data::Utilities::OperationData const * const initializationParameters,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

    public: // IVersionManager
        ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
            __in FABRIC_SEQUENCE_NUMBER checkpointLsnToBeRemoved,
            __in FABRIC_SEQUENCE_NUMBER nextCheckpointLsn) noexcept override;

        NTSTATUS TryRemoveVersion(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in FABRIC_SEQUENCE_NUMBER commitLsn,
            __in FABRIC_SEQUENCE_NUMBER nextCommitLsn,
            __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept override;

    public:
        void Initialize(
            __in MockLoggingReplicator & loggingReplicator,
            __in Data::StateManager::StateManager & stateManager);

    public:
        ktl::Awaitable<void> OpenAsync(
            __in ktl::CancellationToken & cancellationToken,
            __in_opt bool completeCheckpoint = false,
            __in_opt bool cleanupRestore = false);

        ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE role, 
            __in ktl::CancellationToken & cancellationToken);
        ktl::Awaitable<void> CloseAsync(__in ktl::CancellationToken & cancellationToken);

    public:      
        NTSTATUS CreateTransaction(
            __out TxnReplicator::Transaction::SPtr & txn) noexcept override;
        NTSTATUS CreateAtomicOperation(
            __out TxnReplicator::AtomicOperation::SPtr & txn) noexcept override;
        NTSTATUS CreateAtomicRedoOperation(
            __out TxnReplicator::AtomicRedoOperation::SPtr & txn) noexcept override;

        // TODO: Rethink
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
        
    public: // Test methods
        TxnReplicator::Transaction::SPtr CreateTransaction();
        TxnReplicator::AtomicOperation::SPtr CreateAtomicOperation();
        TxnReplicator::AtomicRedoOperation::SPtr CreateAtomicRedoOperation();

        ktl::Awaitable<LONG64> CheckpointAsync(__in ktl::CancellationToken const & cancellationToken);

        ktl::Awaitable<LONG64> PrepareCheckpointAsync();
        ktl::Awaitable<void> PerformCheckpointAsync(__in ktl::CancellationToken const & cancellationToken);
        ktl::Awaitable<void> CompleteCheckpointAsync(__in ktl::CancellationToken const & cancellationToken);

        void SetReplicationBlocking(__in bool blockReplication);

        void SetSafeLSN(__in FABRIC_SEQUENCE_NUMBER safeLSN);

        ktl::Awaitable<void> CopyAsync(
            __in TxnReplicator::OperationDataStream & operationDataStream, 
            __in ktl::CancellationToken const & cancellationToken);

        ktl::Awaitable<void> CopyAsync(
            __in KArray<Data::Utilities::OperationData::SPtr> operationDataArray,
            __in ktl::CancellationToken const & cancellationToken);

    private:
        Data::StateManager::StateManager::SPtr stateManagerSPtr_;
        MockLoggingReplicator::SPtr loggingReplicatorSPtr_;

    private:
        FABRIC_REPLICA_ROLE role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    };
}
