// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define MOCKREPLICATOR_TAG 'prCM' 

namespace TStoreTests
{
    class MockTransactionalReplicator :
        public KAsyncServiceBase,
        public KWeakRefType<MockTransactionalReplicator>,
        public TxnReplicator::ITransactionalReplicator,
        public TxnReplicator::IVersionProvider
    {
        K_FORCE_SHARED(MockTransactionalReplicator)
            K_SHARED_INTERFACE_IMP(ITransactionalReplicator)
            K_SHARED_INTERFACE_IMP(IStateProviderMap)
            K_SHARED_INTERFACE_IMP(ITransactionManager)
            K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
            K_SHARED_INTERFACE_IMP(IVersionManager)
            K_SHARED_INTERFACE_IMP(IVersionProvider)
            K_WEAKREF_INTERFACE_IMP(ITransactionManager, MockTransactionalReplicator)
            K_WEAKREF_INTERFACE_IMP(ITransactionalReplicator, MockTransactionalReplicator)
            K_WEAKREF_INTERFACE_IMP(IVersionProvider, MockTransactionalReplicator)

    public:

        static NTSTATUS Create(
            __in KAllocator & allocator,
            __in bool hasPersistedState,
            __out MockTransactionalReplicator::SPtr& result);

        //ITransactionManager APIs
        bool IsReadable();

        __declspec(property(get = get_HasPersistedState)) bool HasPeristedState;
        bool get_HasPersistedState() const override
        {
            return hasPersistedState_;
        }

        LONG64 IncrementAndGetCommitSequenceNumber();

        bool TryGetStateProvider(
            __in LONG64 stateProviderId,
            __out TxnReplicator::IStateProvider2::SPtr& result);

        ktl::Awaitable<void> AddAsync(
            __in TxnReplicator::Transaction& transaction,
            __in LONG64 stateProviderId,
            __in TxnReplicator::IStateProvider2& stateProvider,
            __in_opt Data::Utilities::OperationData const * const initializationParameters);

        void RegisterStateProvider(
            __in KUriView const&  stateProviderName,
            __in TxnReplicator::IStateProvider2& stateProvider);

        bool get_IsReadable() const override;

        void SetReadable(bool value);

        // TODO: Remove
        __declspec(property(get = get_Role, put = put_Role)) FABRIC_REPLICA_ROLE Role;
        FABRIC_REPLICA_ROLE get_Role() const
        {
            return role_;
        }

        void put_Role(FABRIC_REPLICA_ROLE newRole)
        {
            role_ = newRole;
        }

        __declspec(property(get = get_DelayAddOperationAndThrow, put = put_DelayAddOperationAndThrow)) bool DelayAddOperationAndThrow;
        bool get_DelayAddOperationAndThrow() const
        {
            return delayAddOperationAndThrow_;
        }

        void put_DelayAddOperationAndThrow(bool delay)
        {
            delayAddOperationAndThrow_ = delay;
        }

        __declspec(property(get = get_ShouldSynchPrepareAndApply, put = set_ShouldSynchPrepareAndApply)) bool ShouldSynchronizePrepareAndApply;
        bool get_ShouldSynchPrepareAndApply() const
        {
            return shouldSynchPrepareAndApply_;
        }

        void set_ShouldSynchPrepareAndApply(bool enable)
        {
            shouldSynchPrepareAndApply_ = enable;
        }

        __declspec(property(get = get_Timeout, put = put_Timeout)) ULONG Timeout;
        ULONG get_Timeout() const
        {
            return timeout_;
        }

        ULONG put_Timeout(ULONG timeout)
        {
            timeout_ = timeout;
        }

        __declspec(property(get = get_PrepareApplyLock)) Data::Utilities::ReaderWriterAsyncLock::SPtr PrepareApplyLockSPtr;
        Data::Utilities::ReaderWriterAsyncLock::SPtr get_PrepareApplyLock()
        {
            return prepareApplyLockSPtr_;
        }

        ktl::Awaitable<NTSTATUS> WaitForTimeoutAndThrowAsync();

        void ClearSecondary();

        ktl::Awaitable<void> OpenAsync();

        ktl::Awaitable<void> SimulateFalseProgressAsync(__in TxnReplicator::TransactionBase& tx);

        ktl::Awaitable<void> SimulateSecondaryFalseProgressAsync(__in TxnReplicator::TransactionBase& tx);

        ktl::Awaitable<void> RemoveStateAndCloseAsync(bool doNotRemoveState = false);

        NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        NTSTATUS RegisterStateManagerChangeHandler(
            __in TxnReplicator::IStateManagerChangeHandler & stateManagerChangeHandler) noexcept override;

        NTSTATUS UnRegisterStateManagerChangeHandler() noexcept override;

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __out TxnReplicator::BackupInfo & result) noexcept override;

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken,
            __out TxnReplicator::BackupInfo & result) noexcept override;

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder) noexcept override;

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder,
            __in FABRIC_RESTORE_POLICY restorePolicy,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

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
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        NTSTATUS GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept override;

        NTSTATUS GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept override;

        NTSTATUS GetCurrentEpoch(__out FABRIC_EPOCH & epoch) noexcept override;

        ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
            LONG64 checkpointLsnToBeRemoved,
            LONG64 nextCheckpointLsn) noexcept override;

        NTSTATUS TryRemoveVersion(
            LONG64 stateProviderId,
            LONG64 commitLsn,
            LONG64 nextCommitLsn,
            __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept override;

        NTSTATUS GetVersion(__out LONG64 & version) const noexcept override;

        template<typename T>
        bool TryAddStateSerializer(Data::StateManager::IStateSerializer<T>& stateSerializer)
        {
            //todo: use a dictionary instead?

            //return this.serializationManager.TryAddStateSerializer<T>(stateSerializer);
            return true;
        }

        template<typename T>
        bool TryAddStateSerializer(KUriView const & name, Data::StateManager::IStateSerializer<T>& stateSerializer)
        {
            //return this.serializationManager.TryAddStateSerializer<T>(name, stateSerializer);
            return true;
        }

        template<typename T>
        Data::StateManager::IStateSerializer<T> GetStateSerializer(KUriView & name)
        {
            //return this.serializationManager.GetStateSerializer<T>(name);
            throw Exception(STATUS_NOT_IMPLEMENTED);
        }

        Data::Utilities::IStatefulPartition::SPtr get_StatefulPartition() const override;
        void SetWriteStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus);
        void SetReadStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus);

        __declspec(property(get = get_CommitSequenceNumber, put = set_CommitSequenceNumber)) LONG64 CommitSequenceNumber;
        LONG64 get_CommitSequenceNumber()
        {
            return lastLSN_;
        }

        void set_CommitSequenceNumber(LONG64 commitSequenceNumber)
        {
            lastLSN_ = commitSequenceNumber;
        }

        void Initialize(__in Data::Utilities::PartitionedReplicaId const & traceId);

        void RegisterSecondary(__in MockTransactionalReplicator & replicator);

        void MockTransactionalReplicator::UpdateLSN(__in LONG64 lsn);
        void MockTransactionalReplicator::UpdateSecondaryLSN();

    private:
        MockTransactionalReplicator(__in bool hasPersistedState);

        void AddTransaction(
            __in TxnReplicator::Transaction & transaction,
            __in LONG64 stateProviderId,
            __in TestTransactionContext operation);

        ktl::Awaitable<void> CommitAsync(
            __in TxnReplicator::Transaction & tx);

        void ReplicateTransaction(
            __in TxnReplicator::Transaction & tx,
            __in LONG64 stateProviderId,
            __in TestTransactionContext operation);

        ktl::Awaitable<void> ReplicateCommitAsync(
            __in TxnReplicator::Transaction & tx);

        FABRIC_REPLICA_ROLE role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
        volatile LONG64 lastLSN_ = 0;
        bool isReadable_ = false;

        bool hasPersistedState_;

        Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestTransactionContext>::SPtr>::SPtr inflightTransactionMapSPtr_;
        Data::Utilities::ReaderWriterAsyncLock::SPtr readerWriterLockSPtr_;

        Data::Utilities::ConcurrentDictionary<LONG64, TxnReplicator::IStateProvider2::SPtr>::SPtr stateProvidersSPtr_;
        TxnReplicator::IStateProvider2::SPtr stateProvider_ = nullptr;

        KSharedArray<MockTransactionalReplicator::SPtr>::SPtr secondariesReplicatorSPtr_;

        Data::TestCommon::TestStatefulServicePartition::SPtr partition_ = nullptr;

        TxnReplicator::IVersionManager::SPtr versionManager_;

        bool delayAddOperationAndThrow_;

        ULONG timeout_;

        Data::Utilities::ReaderWriterAsyncLock::SPtr prepareApplyLockSPtr_;
        bool shouldSynchPrepareAndApply_ = true;
    };
}
