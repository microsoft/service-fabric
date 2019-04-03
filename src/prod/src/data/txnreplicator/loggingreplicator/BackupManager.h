// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class BackupManager final
            : public KObject<BackupManager>
            , public KShared<BackupManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
            , public IBackupManager
        {
            friend class LoggingReplicatorTests::BackupManagerTests;

            K_FORCE_SHARED(BackupManager);
            K_SHARED_INTERFACE_IMP(IBackupManager);

        public:
            static KStringView const FullMetadataFileName;
            static KStringView const IncrementalMetadataFileName;

            // Parent folder name
            static KStringView const BackupFolderName;

            // State manager folder name under parent
            static KStringView const StateManagerBackupFolderName;

            // Replicator folder name under parent.
            static KStringView const ReplicatorBackupFolderName;

            // Replicator log name under replicator folder.
            static KStringView const ReplicatorBackupLogName;

        public:
            static BackupManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KString const & workFolder,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IReplicatedLogManager & replicatedLogManager,
                __in ILogManager & logManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

        public:
            void Initialize(
                __in TxnReplicator::IBackupRestoreProvider & backupRestoreProvider,
                __in ICheckpointManager & checkpointManager,
                __in IOperationProcessor & operationProcessor);

        public: // IBackupManager
            void SetLastCompletedBackupLogRecord(
                __in LogRecordLib::BackupLogRecord const & backupLogRecord) override;

            LogRecordLib::BackupLogRecord::CSPtr GetLastCompletedBackupLogRecord() const override;

            void UndoLastCompletedBackupLogRecord() override;

            bool ShouldCleanState() const override;

            void StateCleaningComplete() override;

            bool IsLogUnavailableDueToRestore() const noexcept override;

            ktl::Awaitable<TxnReplicator::BackupInfo> BackupAsync(
                __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler) override;

            ktl::Awaitable<TxnReplicator::BackupInfo> BackupAsync(
                __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> RestoreAsync(
                __in KString const & backupFolder) override;

            ktl::Awaitable<void> RestoreAsync(
                __in KString const & backupFolder,
                __in FABRIC_RESTORE_POLICY restorePolicy,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            void EnableBackup() noexcept override;
            ktl::Awaitable<void> DisableBackupAndDrainAsync() override;

            void EnableRestore() noexcept override;
            void DisableRestore() noexcept override;

            NTSTATUS DeletePersistedState() noexcept override;

        private:
            enum LockRequester
            {
                Backup,
                Restore
            };

        private:
            static KString::CSPtr CreateBackupFolderPath(
                __in KString const & workFolder,
                __in KGuid const & partitionId,
                __in LONG64 replicaId,
                __in KAllocator & allocator);

            static KString::CSPtr CreateRestoreFolderPath(
                __in KString const & workFolder,
                __in KGuid const & partitionId,
                __in LONG64 replicaId,
                __in KAllocator & allocator);

        private:
            Common::Guid GenerateBackupId();

            ktl::Awaitable<void> PrepareBackupFolderAsync(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::Guid backupId);

            ktl::Awaitable<Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr> BackupStateManagerAndGetReplicatorLogsToBeBackedUpAsync(
                __in Common::Guid backupId,
                __in KString const & stateManagerBackupFolderPath,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr> GetIncrementalLogRecordsAsync(
                __in Common::Guid backupId,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken);

            Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr GetLogRecordsCallerHoldsLock(
                __in LogRecordLib::LogRecord const & indexingLogRecord,
                __in KString const & lockTakerName);

            Data::LogRecordLib::IndexingLogRecord::CSPtr GetIndexingLogRecordForFullBackup(
                __in IReplicatedLogManager & replicatedLogManager);

            ktl::Awaitable<ReplicatorBackup> BackupReplicatorAsync(
                __in Common::Guid backupId, 
                __in FABRIC_BACKUP_OPTION backupOption,
                __in KString const & replicatorBackupFolder,
                __in Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr> & logRecordsAsyncEnumerator,
                __in ktl::CancellationToken const & cancellationToken);

            LogRecordLib::LogRecord::CSPtr FindFirstLogRecordForIncrementalBackup(
                __in FABRIC_SEQUENCE_NUMBER highestBackedUpLSN,
                __in Common::Guid const & backupId);

            ktl::Awaitable<NTSTATUS> WriteBackupMetadataFileAsync(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::Guid backupId,
                __in Common::Guid parentBackupId,
                __in ReplicatorBackup const & replicatorBackup,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CallBackupCallbackAsync(
                __in Common::Guid const & backupId,
                __in TxnReplicator::BackupInfo const & backupInfo,
                __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ReplicateBackupLogRecordAsync(
                __in Common::Guid const & backupId,
                __in ReplicatorBackup const & replicatorBackup) noexcept;

            ktl::Awaitable<NTSTATUS> ReplicateAndAwaitBarrierLogRecordAsync(
                __in Common::Guid const & backupId) noexcept;

            ktl::Awaitable<NTSTATUS> UnlockOnceApplyAndReplicationCompletesAsync(
                __in LogRecordLib::BackupLogRecord & backupLogRecord) noexcept;

            ktl::Awaitable<NTSTATUS> RecoverRestoreDataAsync(
                __in TxnReplicator::IBackupRestoreProvider & loggingReplicator,
                __in Common::Guid & restoreId,
                __in BackupFolderInfo const & backupFolderInfo) noexcept;

            virtual ktl::Awaitable<NTSTATUS> FinishRestoreAsync(
                __in TxnReplicator::IBackupRestoreProvider & loggingReplicator,
                __in TxnReplicator::Epoch & dataLossEpoch) noexcept;

        private: // Restore Token methods
            bool CheckIfRestoreTokenExists();

            ktl::Awaitable<void> CreateRestoreTokenAsync(
                __in Common::Guid const & id);

            void DeleteRestoreToken(
                __in Common::Guid const & id);

        private: // Locks
            ktl::Awaitable<void> AcquireEntryLock(
                __in Common::Guid const & id,
                __in Common::TimeSpan timeout,
                __in LockRequester requester);
            void ReleaseEntryLock();

        private: // Drain related operations
            NTSTATUS StartDrainableBackupPhase() noexcept;
            void CompleteDrainableBackupPhase() noexcept;

        private: // ThrowIf
            void ThrowIfFullBackupIsMissing(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::Guid const & backupId);

            void ThrowIfAccumulatedIncrementalBackupLogsIsTooLarge(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::Guid const & backupId,
                __in ReplicatorBackup const & replicatorBackup);

            void ThrowIfOnDataLossIsNotInflight();

            void ThrowIfInvalidState(
                __in Common::Guid const & restoreId);

            void ThrowIfNotSafe(
                __in FABRIC_RESTORE_POLICY restorePolicy,
                __in BackupFolderInfo const & backupFolderInfo,
                __in Common::Guid const & restoreId);

        private: // Helper functions
            TxnReplicator::IBackupRestoreProvider::SPtr  GetLoggingReplicator();
            ICheckpointManager::SPtr GetCheckpointManager();
            IOperationProcessor::SPtr GetOperationProcessor();

            void TraceException(
                __in Common::Guid const & backupId,
                __in std::wstring const & message,
                __in ktl::Exception & exception) const;

        private:
            BackupManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KString const & workFolder,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IReplicatedLogManager & replicatedLogManager,
                __in ILogManager & logManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

        private:
            static LONG32 const StartingBackoffInMilliSeconds = 500;
            static LONG32 const MaxRetryCount = 7;
            static KStringView const RestoreFolderName;
            static KStringView const RestoreTokenName;

        private: // Initializer list const.
            TxnReplicator::IStateProviderManager::SPtr const stateManagerSPtr_;
            IReplicatedLogManager::SPtr const replicatedLogManagerSPtr_;
            ILogManager::SPtr const logManagerSPtr_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecordsSPtr_;
            KString::CSPtr const backupFolderCSPtr_;
            KString::CSPtr const lrBackupFolderCSPtr_;
            KString::CSPtr const smBackupFolderCSPtr_;
            KString::CSPtr const fullMetadataFilePathCSPtr_;
            KString::CSPtr const incrementalMetadataFilePathCSPtr_;
            KString::CSPtr const restoreTokenFolderPathCSPtr_;
            KString::CSPtr const restoreTokenFilePathCSPtr_;
            KString::CSPtr const replicatorLogBackupFilePathCSPtr_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

        private: // Members for drainable backup logic.
            KSpinLock backupDrainLock_;
            bool isBackupDrainablePhaseAllowed_ = false;
            bool isBackupPhaseInflight_ = false;
            bool isLogUnavailableDueToRestore_ = false;
            ktl::AwaitableCompletionSource<void>::SPtr drainBackupCallBackAcsSPtr_;

        private: 
            volatile bool isOnDataLossInProgress_ = false;

        private: // Set once.
            KWeakRef<TxnReplicator::IBackupRestoreProvider>::SPtr backupRestoreProviderWRef_;
            KWeakRef<ICheckpointManager>::SPtr checkpointManagerWRef_;
            KWeakRef<IOperationProcessor>::SPtr operationProcessorWRef_;

            // Note: change SpinLock to ReaderWriterAsyncLock for the reason that coroutine will cause thread change thus
            // we can not guarantee acquire lock and release lock in the same thread.
            Data::Utilities::ReaderWriterAsyncLock::SPtr backupAndRestoreLock_;
            
        private:
            /// <summary>
            /// The last completed backup log record.
            /// </summary>
            Utilities::ThreadSafeCSPtrCache<LogRecordLib::BackupLogRecord> lastCompletedBackupLogRecordThreadSafeCache_;
           
        private:
            /// <summary>
            /// Test variable that causes the DeleteRestoreToken call to no-op instead of deleting the token. 
            /// </summary>
            bool testFlagNoOpDeleteRestoreToken_ = false;

            /// <summary>
            /// Test variable that causes the DeleteRestoreToken call to no-op instead of deleting the token. 
            /// </summary>
            bool testFlagFailRestoreData_ = false;
        };
    }
}
