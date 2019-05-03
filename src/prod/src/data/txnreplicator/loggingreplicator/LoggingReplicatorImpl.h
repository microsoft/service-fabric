// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class LoggingReplicatorImpl
            : public KObject<LoggingReplicatorImpl>
            , public TxnReplicator::IVersionProvider
            , public KShared<LoggingReplicatorImpl>
            , public TxnReplicator::ILoggingReplicator
            , public IFlushCallbackProcessor
            , public ICompletedRecordsProcessor
            , public TxnReplicator::IBackupRestoreProvider
            , public KWeakRefType<LoggingReplicatorImpl>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(LoggingReplicatorImpl)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(ITransactionManager)
            K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
            K_SHARED_INTERFACE_IMP(IFlushCallbackProcessor)
            K_SHARED_INTERFACE_IMP(ICompletedRecordsProcessor)
            K_SHARED_INTERFACE_IMP(ILoggingReplicator)
            K_SHARED_INTERFACE_IMP(IVersionManager)
            K_SHARED_INTERFACE_IMP(IVersionProvider)
            K_SHARED_INTERFACE_IMP(IBackupRestoreProvider)
            K_WEAKREF_INTERFACE_IMP(ILoggingReplicator, LoggingReplicatorImpl)
            K_WEAKREF_INTERFACE_IMP(ITransactionManager, LoggingReplicatorImpl)
            K_WEAKREF_INTERFACE_IMP(IFlushCallbackProcessor, LoggingReplicatorImpl)
            K_WEAKREF_INTERFACE_IMP(ICompletedRecordsProcessor, LoggingReplicatorImpl)
            K_WEAKREF_INTERFACE_IMP(IVersionProvider, LoggingReplicatorImpl)
            K_WEAKREF_INTERFACE_IMP(IBackupRestoreProvider, LoggingReplicatorImpl)

        public:

            static LoggingReplicatorImpl::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders const & runtimeFolders,
                __in Data::Utilities::IStatefulPartition & partition,
                __in IStateReplicator & stateReplicator,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in KString const & logDirectory,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
                __in Data::Log::LogManager & dataLogManager,
                __in TxnReplicator::IDataLossHandler & dataLossHandler,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in bool hasPersistedState,
                __in KAllocator& allocator,
                __out IStateProvider::SPtr & stateProvider);

            bool get_HasPersistedState() const override;

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __out TxnReplicator::RecoveryInformation & recoveryInformation) noexcept override;

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in_opt TxnReplicator::IBackupFolderInfo const * const backupFolderInfo,
                __out TxnReplicator::RecoveryInformation & recoveryInformation) noexcept override;

            ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(__in TxnReplicator::RecoveryInformation const & recoveryInfo) noexcept override;

            ktl::Awaitable<NTSTATUS> BecomeActiveSecondaryAsync(__in StateManagerBecomeSecondaryDelegate stateManagerBecomeSecondaryDelegate) noexcept override;

            ktl::Awaitable<NTSTATUS> BecomeIdleSecondaryAsync() noexcept override;

            ktl::Awaitable<NTSTATUS> BecomeNoneAsync() noexcept override;

            ktl::Awaitable<NTSTATUS> BecomePrimaryAsync(bool isForRestore) noexcept override;

            ktl::Awaitable<NTSTATUS> DeleteLogAsync() noexcept override;

            NTSTATUS GetSafeLsnToRemoveStateProvider(__out LONG64 & lsn) noexcept override;

            ktl::Awaitable<NTSTATUS> CloseAsync() noexcept override;

            ktl::Awaitable<NTSTATUS> PrepareForRestoreAsync() noexcept override;

            void Dispose() override;
             
            bool IsReadable() noexcept override;

            NTSTATUS GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept override;

            NTSTATUS GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept override;

            NTSTATUS GetCurrentEpoch(__out FABRIC_EPOCH & epoch) noexcept override;

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

            NTSTATUS GetVersion(__out LONG64 & version) const noexcept override;
            
#pragma region IVersionManager: used to forward calls in to Version Manager from TransactionalReplicator
            ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
                __in LONG64 checkpointLsnToBeRemoved,
                __in LONG64 nextCheckpointLsn) noexcept override;

            NTSTATUS TryRemoveVersion(
                __in LONG64 stateProviderId,
                __in LONG64 commitLsn,
                __in LONG64 nextCommitLsn,
                __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept override;
#pragma endregion 

            __declspec(property(get = get_Role)) FABRIC_REPLICA_ROLE Role;
            FABRIC_REPLICA_ROLE get_Role() const override
            {
                return roleContextDrainState_->ReplicaRole;
            }

            NTSTATUS CreateTransaction(__out KSharedPtr<TxnReplicator::Transaction> & transaction) noexcept override
            {
                transaction = nullptr;

                ASSERT_IFNOT(
                    false,
                    "{0}: LoggingReplicatorImpl::CreateTransaction() should never be called",
                    TraceId);

                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS CreateAtomicOperation(__out KSharedPtr<TxnReplicator::AtomicOperation> & atomicOperation) noexcept override
            {
                atomicOperation = nullptr;

                ASSERT_IFNOT(
                    false,
                    "{0}: LoggingReplicatorImpl::CreateAtomicOperation() should never be called",
                    TraceId);

                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS CreateAtomicRedoOperation(KSharedPtr<TxnReplicator::AtomicRedoOperation> & atomicRedoOperation) noexcept override
            {
                atomicRedoOperation = nullptr;

                ASSERT_IFNOT(
                    false,
                    "{0}: LoggingReplicatorImpl::CreateAtomicRedoOperation() should never be called",
                    TraceId);

                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS SingleOperationTransactionAbortUnlock(
                __in LONG64 stateProviderId,
                __in TxnReplicator::OperationContext const & operationContext) noexcept override
            {
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);

                ASSERT_IFNOT(
                    false,
                    "{0}: LoggingReplicatorImpl::SingleOperationTransactionAbortUnlock() should never be called",
                    TraceId);

                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS ErrorIfStateProviderIsNotRegistered(
                __in LONG64 stateProviderId,
                __in LONG64 transactionId) noexcept override
            {
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(transactionId);
                ASSERT_IFNOT(
                    false,
                    "{0}: LoggingReplicatorImpl::ErrorIfStateProviderIsNotRegistered() should never be called",
                    TraceId);

                return STATUS_NOT_IMPLEMENTED;
            }

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

            void Reuse();

            ktl::Awaitable<NTSTATUS> OnDataLossAsync(__out bool & wasDataLost) noexcept;

            ktl::Awaitable<NTSTATUS> UpdateEpochAsync(
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 lastLsnInPreviousEpoch) noexcept;

            NTSTATUS GetCopyContext(__out TxnReplicator::IOperationDataStream::SPtr & result) noexcept;

            NTSTATUS GetCopyState(
                __in LONG64 uptoLsn,
                __in TxnReplicator::OperationDataStream & copyContext,
                __out TxnReplicator::IOperationDataStream::SPtr & result) noexcept;

            void Test_SetTestHookContext(TestHooks::TestHookContext const &);

            NTSTATUS Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
                __out LONG64 & lastPeriodicCheckpointTimeTicks,
                __out LONG64 & lastPeriodicTruncationTimeTicks) noexcept override;

        public: // Notification APIs
            NTSTATUS RegisterTransactionChangeHandler(
                __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

            NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        public: // Test Support

            NTSTATUS Test_RequestCheckpointAfterNextTransaction() noexcept override;

        private: 

            //
            // Indicates state of the primary 
            //
            typedef enum PrimaryElectionStatus
            {
                //
                // Replica that is not a primary.
                // 
                None = 0,

                //
                // Primary that is being demoted in favor of promoting an active secondary in configuration set.
                //
                SwappingOut = 1,

                //
                // Replica that retained the primary status across a failover.
                // Will only start executing when Reliability introduces reconfiguration free partition wide restarts.
                //
                Retained = 2,

                //
                // Replica that has been elected to be the new Primary (common case).
                //
                Elected = 3,

                //
                // Replica that has established it self as the new Primary
                // This means that it has started to replicate and log a Barrier log record.
                //
                Established = 4,
            }PrimaryElectionStatus;

            LoggingReplicatorImpl(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders const & runtimeFolders,
                __in Data::Utilities::IStatefulPartition & partition,
                __in IStateReplicator & stateReplicator,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords_,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in KString const & logDirectory,
                __in LogManager & logManager,
                __in RoleContextDrainState & roleContextDrainState,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TxnReplicator::IInternalVersionManager & versionManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
                __in TxnReplicator::IDataLossHandler & dataLossHandler,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in bool hasPersistedState,
                __out IStateProvider::SPtr & stateProvider);
            
            NTSTATUS ErrorIfNoWriteStatus() const noexcept;

            //
            // This method ensures that the first ever replicated record on a new primary is a barrier
            //
            ktl::Awaitable<NTSTATUS> ReplicateBarrierIfPrimaryStatusIsBelowEstablishedAsync() noexcept;

            NTSTATUS ReplicateBarrierIfPrimaryStatusIsBelowEstablished() noexcept;

            ktl::Awaitable<void> AwaitProcessingBarrierRecord(LogRecordLib::BarrierLogRecord & barrierLogRecord) noexcept;

            ktl::Awaitable<FABRIC_REPLICA_ROLE> PrepareForReplicaRoleTransitionAsync(__in FABRIC_REPLICA_ROLE newRole) noexcept;

            ktl::Awaitable<void> QuiesceReplicaActivityAsync() noexcept;

            ktl::Task AbortPendingTransactionsOrAddBarrierOnBecomingPrimary(
                __in KSharedArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr>::SPtr abortingTransactions,
                __in Utilities::ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<void>> & completingSource,
                __in Utilities::ThreadSafeSPtrCache<ktl::CancellationTokenSource> & cts) noexcept;

            ktl::Awaitable<NTSTATUS> CancelTheAbortingOfTransactionsIfNecessaryAsync() noexcept override;

            void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override;

            void ProcessedLogicalRecord(__in LogRecordLib::LogicalLogRecord & logicalRecord) noexcept override;

            void ProcessedPhysicalRecord(__in LogRecordLib::PhysicalLogRecord & physicalRecord) noexcept override;

            ktl::Awaitable<NTSTATUS> PerformUpdateEpochAsync(
                __in TxnReplicator::Epoch const & epoch,
                __in FABRIC_SEQUENCE_NUMBER previousEpochLastLsn) noexcept override;

            void PrepareToTransitionOutOfPrimary(__in bool isSwapping) noexcept;

            ktl::Awaitable<NTSTATUS> WaitForLogFlushUptoLsn(__in LONG64 uptoLsn) noexcept;

            void WakeupWaitingStreams(
                __in LogRecordLib::LogRecord & mostRecentFlushedRecord,
                __in NTSTATUS flushError) noexcept;

            void CreateAllManagers();

            void CreateOperationProcessor();

            bool hasPersistedState_ = true;

            TxnReplicator::IRuntimeFolders::CSPtr runtimeFolders_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            TxnReplicator::SLInternalSettingsSPtr const ktlLoggerSharedLogConfig_;
            Data::Utilities::IStatefulPartition::SPtr const partition_;
            TxnReplicator::IStateProviderManager::SPtr const stateManager_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;
            KString::CSPtr const logDirectory_;
            LogManager::SPtr const logManager_;
            RoleContextDrainState::SPtr const roleContextDrainState_;
            RecoveredOrCopiedCheckpointState::SPtr const recoveredOrCopiedCheckpointState_;
            ReplicatedLogManager::SPtr const replicatedLogManager_;
            TxnReplicator::IInternalVersionManager::SPtr const versionManager_;
            LogRecordLib::CopyStageBuffers::SPtr const copyStageBuffers_;
            PhysicalLogWriterCallbackManager::SPtr const logFlushCallbackManager_;
            Data::LoggingReplicator::IStateReplicator::SPtr const iStateReplicator_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

            LogTruncationManager::SPtr logTruncationManager_;
            CheckpointManager::SPtr checkpointManager_;
            LogRecordsDispatcher::SPtr logRecordsDispatcher_;
            BackupManager::SPtr backupManager_;
            OperationProcessor::SPtr operationProcessor_;
            TransactionManager::SPtr transactionManager_;
            RecoveryManager::SPtr recoveryManager_;
            SecondaryDrainManager::SPtr secondaryDrainManager_;
            IStateProvider::SPtr stateProvider_;
            TxnReplicator::IDataLossHandler::SPtr dataLossHandler_;
            TxnReplicator::ITransactionalReplicator & iTransactionalReplicator_;

            bool isDisposed_;
            
            // This is volatile even though it is protected by lock.
            // This is because we check its value outside the lock in the hot path and acquire the lock only if needed
            volatile PrimaryElectionStatus primaryStatus_; 

            KSpinLock flushWaitLock_;
            KSpinLock primaryStatusLock_;
            KSpinLock changeHandlerLock_;

            Utilities::ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<void>> primaryTransactionsAbortingTask_;
            Utilities::ThreadSafeSPtrCache<ktl::CancellationTokenSource> primaryTransactionsAbortingCts_;
            TxnReplicator::ITransactionChangeHandler::SPtr changeHandlerCache_;

            typedef struct WaitingFlushStreamsContext
            {
                ktl::AwaitableCompletionSource<NTSTATUS>::SPtr Tcs;
                LONG64 Lsn;
            }WaitingFlushStreamsContext;

            KArray<WaitingFlushStreamsContext> waitingStreams_;

            // Performance counters
            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;

            // Performance counter related stopwatches
            Common::Stopwatch logRecoveryWatch_;
            Common::Stopwatch stateManagerRecoveryWatch_;
        };
    }
}
