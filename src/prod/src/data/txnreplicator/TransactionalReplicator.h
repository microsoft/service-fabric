// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class TransactionalReplicator final
        : public KAsyncServiceBase
        , public KWeakRefType<TransactionalReplicator>
        , public ITransactionalReplicator
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(TransactionalReplicator)

        K_SHARED_INTERFACE_IMP(IStateProviderMap)
        K_SHARED_INTERFACE_IMP(ITransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
        K_SHARED_INTERFACE_IMP(IVersionManager)
        K_SHARED_INTERFACE_IMP(ITransactionalReplicator)
        K_WEAKREF_INTERFACE_IMP(ITransactionManager, TransactionalReplicator)
        K_WEAKREF_INTERFACE_IMP(ITransactionalReplicator, TransactionalReplicator)

    public:

        static TransactionalReplicator::SPtr Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition,
            __in Data::LoggingReplicator::IStateReplicator & stateReplicator,
            __in Data::StateManager::IStateProvider2Factory & stateProviderFactory,
            __in TRInternalSettingsSPtr && transactionalReplicatorConfig,
            __in SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
            __in Data::Log::LogManager & logManager,
            __in IDataLossHandler & dataLossHandler,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in bool hasPersistedState,
            __in KAllocator& allocator);

        __declspec(property(get = get_Config)) TRInternalSettingsSPtr const & Config;
        TRInternalSettingsSPtr const & get_Config() const { return transactionalReplicatorConfig_; }

        __declspec(property(get = get_SharedLogConfig)) SLInternalSettingsSPtr const & SLConfig;
        SLInternalSettingsSPtr const & get_SharedLogConfig() const { return ktlLoggerSharedLogConfig_; }

        Data::LoggingReplicator::IStateProvider::SPtr GetStateProvider()
        {
            return stateProvider_;
        }

        NTSTATUS RegisterTransactionChangeHandler(
            __in ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        NTSTATUS RegisterStateManagerChangeHandler(
            __in IStateManagerChangeHandler & stateManagerChangeHandler) noexcept override;

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

        void Test_SetTestHookContext(TestHooks::TestHookContext const &) override;
        TestHooks::TestHookContext const & Test_GetTestHookContext() override;

        ktl::Awaitable<NTSTATUS> OpenAsync();

        ktl::Awaitable<NTSTATUS> CloseAsync();

        ktl::Awaitable<NTSTATUS> ChangeRoleAsync(FABRIC_REPLICA_ROLE newRole);

        void Abort();

        bool get_IsReadable() const noexcept override;

        bool get_HasPersistedState() const noexcept override;

        Data::Utilities::IStatefulPartition::SPtr get_StatefulPartition() const override;

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
            __out TryRemoveVersionResult::SPtr & result) noexcept override;

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

        NTSTATUS Get(
            __in KUriView const & stateProviderName,
            __out IStateProvider2::SPtr & stateProvider) noexcept override;

        ktl::Awaitable<NTSTATUS> AddAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProvider,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        ktl::Awaitable<NTSTATUS> RemoveAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        NTSTATUS CreateEnumerator(
            __in bool parentsOnly,
            __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept override;

        ktl::Awaitable<NTSTATUS> GetOrAddAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProviderTypeName,
            __out IStateProvider2::SPtr & stateProvider,
            __out bool & alreadyExist,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept override;

        NTSTATUS Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
            __out LONG64 & lastPeriodicCheckpointTimeTicks,
            __out LONG64 & lastPeriodicTruncationTimeTicks) noexcept override;

        //
        // Open async operation
        //
        class AsyncOpenContext
            : public Ktl::Com::FabricAsyncContextBase
        {
            K_FORCE_SHARED(AsyncOpenContext)

        public:

            NTSTATUS StartOpen(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);
        };

        NTSTATUS CreateAsyncOpenContext(__out AsyncOpenContext::SPtr & asyncContext);

        //
        // ChangeRole async operation
        //
        class AsyncChangeRoleContext
            : public Ktl::Com::FabricAsyncContextBase
        {
            K_FORCE_SHARED(AsyncChangeRoleContext)

        public:

            NTSTATUS StartChangeRole(
                __in FABRIC_EPOCH const & epoch,
                __in FABRIC_REPLICA_ROLE role,
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);
        };

        NTSTATUS CreateAsyncChangeRoleContext(__out AsyncChangeRoleContext::SPtr & asyncContext);

        class AsyncCloseContext 
            : public Ktl::Com::FabricAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCloseContext)

        public:

            NTSTATUS StartClose(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);
        };

        NTSTATUS CreateAsyncCloseContext(__out AsyncCloseContext::SPtr & asyncContext);

    public: // Test Support
        NTSTATUS Test_RequestCheckpointAfterNextTransaction() noexcept override;

    protected:
        void OnServiceOpen() override;
        void OnServiceClose() override;

    private:

        TransactionalReplicator(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition,
            __in Data::LoggingReplicator::IStateReplicator & stateReplicator,
            __in Data::StateManager::IStateProvider2Factory & stateProviderFactory,
            __in TxnReplicator::TRInternalSettingsSPtr && transactionalReplicatorConfig,
            __in TxnReplicator::SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
            __in Data::Log::LogManager & logManager,
            __in IDataLossHandler & dataLossHandler,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in bool hasPersistedState);

        ktl::Task OnServiceOpenAsync() noexcept;
        ktl::Task OnServiceCloseAsync() noexcept;

        ktl::Awaitable<NTSTATUS> InvokeStateManagerChangeRoleDelegate(FABRIC_REPLICA_ROLE newRole, ktl::CancellationToken cancellationToken) noexcept;

        TRInternalSettingsSPtr const transactionalReplicatorConfig_;
        SLInternalSettingsSPtr const ktlLoggerSharedLogConfig_;
        IRuntimeFolders::SPtr const runtimeFolders_;
        Data::Utilities::IStatefulPartition::SPtr const partition_;
        Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

        ILoggingReplicator::SPtr loggingReplicator_;
        bool loggingReplicatorRecovered_;

        Data::LoggingReplicator::IStateProvider::SPtr stateProvider_;

        Data::StateManager::IStateManager::SPtr stateManager_;
        bool stateManagerOpened_;

        TRPerformanceCountersSPtr perfCounters_;

        // Used for fault injection from FabricTest
        //
        TestHooks::TestHookContext testHookContext_;
    };
}
