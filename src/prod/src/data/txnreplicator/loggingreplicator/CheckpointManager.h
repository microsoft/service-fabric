// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // Co-ordinates checkpoints with the state manager and also manages the lifetime of the checkpoints
        // Interacts with log truncation manager to checkpoint, index and truncate head when needed
        // Finally, also ensures barriers are added when requested by the secondary or primary operation processor
        //
        class CheckpointManager final
            : public ICheckpointManager
            , public KObject<CheckpointManager>
            , public KShared<CheckpointManager>
            , public KWeakRefType<CheckpointManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(CheckpointManager);
            K_SHARED_INTERFACE_IMP(ICheckpointManager);
            K_WEAKREF_INTERFACE_IMP(ICheckpointManager, CheckpointManager);

        public:

            static CheckpointManager::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in ILogTruncationManager & logtruncationManager,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TransactionMap & transactionMap,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in KAllocator & allocator);

            static ULONG CalculateTruncationTimerDurationMs(
                __in Common::DateTime lastPeriodicCheckpointTime,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in PeriodicCheckpointTruncationState::Enum const & state);

            __declspec(property(get = get_LastStableLsn)) LONG64 LastStableLsn;
            LONG64 get_LastStableLsn() const override
            {
                LONG64 lastStableLsn = 0;

                K_LOCK_BLOCK(thisLock_)
                {
                    lastStableLsn = lastStableLsn_;
                }

                return lastStableLsn;
            }

            __declspec(property(get = get_CompletedRecordsProcessor, put = set_CompletedRecordsProcessor)) ICompletedRecordsProcessor::SPtr CompletedRecordsProcessor;
            ICompletedRecordsProcessor::SPtr get_CompletedRecordsProcessor()
            {
                ICompletedRecordsProcessor::SPtr lock = completedRecordsProcessor_->TryGetTarget();
                return lock;
            }
            void set_CompletedRecordsProcessor(ICompletedRecordsProcessor & processor)
            {
                NTSTATUS status = processor.GetWeakIfRef(completedRecordsProcessor_);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to get weak ref to completed records processor in CheckpointManager");
            }

            __declspec(property(get = get_LastPeriodicCheckpointTime, put = set_LastPeriodicCheckpointTime)) Common::DateTime LastPeriodicCheckpointTime;
            Common::DateTime get_LastPeriodicCheckpointTime() const
            {
                return lastPeriodicCheckpointTime_;
            }

            __declspec(property(get = get_LastPeriodicTruncationTime, put = set_LastPeriodicTruncationTime)) Common::DateTime LastPeriodicTruncationTime;
            Common::DateTime get_LastPeriodicTruncationTime() const
            {
                return lastPeriodicTruncationTime_;
            }

            //
            // Invoked when the replica is about to close
            // Marks the state to "Aborted" if the record has not yet been applied. If the record is already applied, the close waits for the pending checkpoint to complete 
            //
            // Refer to checkpoint states for different possible states of the checkpoint.
            //
            void AbortPendingCheckpoint();

            //
            // Invoked when the replica is about to close
            // Marks the state to "Aborted" if the record has not yet been applied. If the record is already applied, the close waits for the pending truncation to complete 
            //
            void AbortPendingLogHeadTruncation();

            //
            // Backup consistency locks around logging EndCheckpoint, calling CompleteCheckpoint and logging CompleteCheckpoint.
            // Locking around endcheckpoint allows efficient way of making sure the low water mark does not get truncated without blocking all truncates.
            // Locking around completeCheckpoint ensures that all checkpoint files being backed up belong to the same checkpoint.
            //
            ktl::Awaitable<bool> AcquireBackupAndCopyConsistencyLockAsync(
                __in KStringView const locker,
                __in KDuration const & timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<bool> AcquireBackupAndCopyConsistencyLockAsync(__in KStringView const locker);

            //
            // Protects backup, preparecheckpoint and performcheckpoint
            //
            ktl::Awaitable<bool> AcquireStateManagerApiLockAsync(
                __in KStringView const locker,
                __in KDuration const & timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> CancelFirstCheckpointOnIdleDueToIncompleteCopy(
                __in LogRecordLib::BeginCheckpointLogRecord & checkpointRecord,
                __in LONG64 copiedUptoLsn
            );

            ktl::Awaitable<void> CompleteFirstCheckpointOnIdleAndRenameLog(
                __in LogRecordLib::BeginCheckpointLogRecord & checkpointRecord,
                __in LONG64 copiedUptoLsn
            );

            void AppendBarrierOnSecondary(__in LogRecordLib::BarrierLogRecord & barrierRecord);

            //
            // Calls prepare, waits for the lsn to become stable and marks the log record as applied if
            // record was successfully flushed and prepared.
            //
            ktl::Awaitable<void> ApplyCheckpointIfPermittedAsync(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS flushError) override;

            void ApplyLogHeadTruncationIfPermitted(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS flushError) override;

            //
            // If a pending checkpoint or log truncation is taking too long, secondary stops pumping additional operations until the pending operation completes
            // This is to ensure the log does not grow out of bounds
            //
            ktl::Awaitable<void> BlockSecondaryPumpIfNeeded();

            //
            // Invoked during full copy on an idle replica
            //
            void FirstBeginCheckpointOnIdleSecondary();

            //
            // Invoked by the primary copy stream class to retreive a log records enumerator during copy
            // It needs to go through the checkpoint manager as a checkpoint could alter the state of the log records to be copied
            //
            LogRecordLib::CopyModeFlag::Enum GetLogRecordsToCopy(
                __in LogRecordLib::ProgressVector & targetProgressVector,
                __in TxnReplicator::Epoch const & targetLogHeadEpoch,
                __in LONG64 targetLogHeadLsn,
                __in LONG64 targetLogTailLsn,
                __in LONG64 lastRecoveredAtomicRedoOperationLsn,
                __in LONG64 targetReplicaId,
                __out LONG64 & sourceStartingLsn,
                __out LONG64 & targetStartingLsn,
                __out Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr & logRecordsToCopy,
                __out LogRecordLib::BeginCheckpointLogRecord::SPtr & completedBeginCheckpointRecord) override;

            //
            // Uses the truncation manager to decide to index or truncate the log
            //
            void InsertPhysicalRecordsIfNecessary();

            void InsertPhysicalRecordsIfNecessary(
                __out bool & addedIndex,
                __out bool & addedTruncateHead);

            void InsertPhysicalRecordsIfNecessaryOnSecondary(
                __in LONG64 copiedUptoLsn,
                __in DrainingStream::Enum drainingStream,
                __in LogRecordLib::LogicalLogRecord & record);

            //
            // Uses the truncation manager to decide to checkpoint based on the thresholds.
            // Additionally, this is invoked by the ReplicatedLogManager when it has a group commit(barrier) record appended
            //
            LogRecordLib::BeginCheckpointLogRecord::SPtr CheckpointIfNecessary(__in bool isPrimary);

            void ReleaseBackupAndCopyConsistencyLock(__in KStringView const & lockReleaser) override;

            void ReleaseStateManagerApiLock(__in KStringView const & lockReleaser) override;

            NTSTATUS ReplicateBarrier(__out LogRecordLib::BarrierLogRecord::SPtr & result);

            //
            // Used by the primary to request a new group commit after a commit transaction
            //
            void RequestGroupCommit();

            void ResetStableLsn(__in LONG64 lsn);

            void Reuse();

            NTSTATUS ErrorIfThrottled(__in LogRecordLib::LogicalLogRecord const & record) noexcept;

            // Resume the periodic checkpoint/truncation lifecycle as necessary
            void Recover(
                __in Data::LogRecordLib::BeginCheckpointLogRecord::SPtr & lastCompletedBeginCheckpointRecord,
                __in LONG64 recoveredTruncationTime);

            void CancelPeriodicCheckpointTimer();

        private:

            //
            // ArmGroupCommit method uses this work item to schedule a group commit on a different thread
            //
            class GroupCommitWorkItem
                : public KObject<GroupCommitWorkItem>
                , public KShared<GroupCommitWorkItem>
                , public KThreadPool::WorkItem
            {
                K_FORCE_SHARED(GroupCommitWorkItem)

            public:
                GroupCommitWorkItem(__in CheckpointManager & parent);

                void Execute();

            private:

                CheckpointManager::SPtr parent_;
            };

            //
            // If an old transaction(s) is/are blocking checkpoints, this work item is trigerred by the checkpoint logic
            // to abort old transactions
            //
            class AbortOldTransactionsWorkItem
                : public KObject<AbortOldTransactionsWorkItem>
                , public KShared<AbortOldTransactionsWorkItem>
                , public KThreadPool::WorkItem
                , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
            {
                K_FORCE_SHARED(AbortOldTransactionsWorkItem)

            public:
                AbortOldTransactionsWorkItem(
                    __in CheckpointManager & parent,
                    __in KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> && abortTxList,
                    __in LONG64 checkpointPreventedAtLsn);

                void Execute();

            private:

                LONG64 checkpointPreventedAtLsn_;
                KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> abortTxList_;
            };

            CheckpointManager(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in ILogTruncationManager & logtruncationManager,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TransactionMap & transactionMap,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            void AbortOldTransactions(
                __in KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> && abortTxList,
                __in LONG64 checkpointPreventedAtLsn);

            ktl::Awaitable<bool> AcquireStateManagerApiLockAsync(__in KStringView const locker);

            void ApplyCheckpointOrLogHeadTruncationIfNecessaryCallerHoldsLock();

            ktl::Task ArmGroupCommitCallerHoldsLock();

            ktl::Task ArmGroupCommitTimerCallerHoldsLock(__in ULONG delayInMilliseconds);

            void CheckpointApply(__in NTSTATUS flushError);

            ktl::Awaitable<NTSTATUS> CompleteStateProviderCheckpointAsync(__in LogRecordLib::BeginCheckpointLogRecord & checkpointRecord);

            ktl::Task GroupCommit();

            bool IndexIfNecessary();

            ktl::Task InitiateCheckpoint(
                __in bool isPrimary,
                __in bool isFirstCheckpointOnFullCopy,
                __out LogRecordLib::BeginCheckpointLogRecord::SPtr & beginCheckpoint);

            void LogHeadTruncationApply(
                __in LogRecordLib::TruncateHeadLogRecord & truncateHeadRecord,
                __in NTSTATUS flushError);

            ktl::Awaitable<void> LogHeadTruncationProcessAsync();

            ktl::Task PerformCheckpointAsync(__in NTSTATUS processingError);

            ktl::Awaitable<NTSTATUS> PerformStateProviderCheckpointAsync(__in LogRecordLib::BeginCheckpointLogRecord & checkpointRecord);

            ktl::Awaitable<NTSTATUS> CompleteCheckpointAndRenameIfNeeded(
                __in LogRecordLib::BeginCheckpointLogRecord & checkpointRecord,
                __in bool renameCopyLog);

            void ProcessBarrierRecord(
                __in LogRecordLib::BarrierLogRecord & barrierRecord,
                __in NTSTATUS errorCode,
                __in bool isPrimary);

            ktl::Task ProcessBarrierRecordOnSecondaryAsync(__in LogRecordLib::BarrierLogRecord & barrierRecord);

            void ProcessError(
                __in LogRecordLib::LogRecord & record,
                __in KStringView const & messagePrefix,
                __in NTSTATUS errorCode);

            void ProcessStableLsnCallerHoldsLock(
                __in LONG64 stableLsn,
                __in LogRecordLib::BarrierLogRecord & barrierRecord);

            NTSTATUS ReplicateBarrier(__in LogRecordLib::BarrierLogRecord & record);

            ktl::Task ScheduleProcessingOnPrimary(__in LogRecordLib::BarrierLogRecord & record);

            ktl::Task TruncateHeadIfNecessary(__out bool addedTruncateHead);

            void InvokeLogicalRecordProcessedCallback(__in LogRecordLib::LogicalLogRecord & logicalRecord);

            void InvokePhysicalRecordProcessedCallback(__in LogRecordLib::PhysicalLogRecord & physicalRecord);

            void OnCompletePendingLogHeadTruncation(__in Data::LogRecordLib::TruncationState::Enum truncationState);
 
            void OnCompletePendingCheckpoint(
                __in NTSTATUS status,
                __in Data::LogRecordLib::CheckpointState::Enum checkpointState);

            void TimerCallback();

            void StartPeriodicCheckpointTimer();

            TransactionMap::SPtr const transactionMap_;
            ILogTruncationManager::SPtr const logTruncationManager_;
            ReplicatedLogManager::SPtr const replicatedLogManager_;
            IBackupManager::SPtr const backupManager_;
            RecoveredOrCopiedCheckpointState::SPtr const recoveredOrCopiedCheckpointState_;
            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;
            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

            KWeakRef<ICompletedRecordsProcessor>::SPtr completedRecordsProcessor_;

            // Ensures that the last End Checkpoint and Complete Checkpoint records in the full backup log correspond to State Provider checkpoints that were backed up.
            Utilities::ReaderWriterAsyncLock::SPtr backupAndCopyConsistencyLock_;

            KSpinLock headTruncationLock_;
            mutable KSpinLock thisLock_;

            KWeakRef<TxnReplicator::IStateProviderManager>::SPtr stateManager_;
            ReplicatedLogManager::IsGoodLogHeadCandidateCalculator goodLogHeadCalculator_;

            // Protects backup, preparecheckpoint and performcheckpoint
            Utilities::ReaderWriterAsyncLock::SPtr stateManagerApiLock_;

            ULONG groupCommitDelay_;
            bool groupCommitNeeded_;
            ktl::AwaitableCompletionSource<void>::SPtr groupCommitTask_;
            LONG64 lastStableLsn_;

            // Timer to fire at configured interval and initiate checkpoint/truncation
            Common::TimerSPtr checkpointTimerSPtr_;

            // Fields used to maintain periodic checkpoint + truncation state machine
            Common::DateTime lastPeriodicCheckpointTime_;
            Common::DateTime lastPeriodicTruncationTime_;
        };
    }
}
