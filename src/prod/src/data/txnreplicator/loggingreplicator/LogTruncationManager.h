// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class LogTruncationManager final
            : public ILogTruncationManager
            , public KObject<LogTruncationManager>
            , public KShared<LogTruncationManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(LogTruncationManager)
            K_SHARED_INTERFACE_IMP(ILogTruncationManager)

        public:
            static LogTruncationManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager const & replicatedLogManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            ktl::Awaitable<void> BlockSecondaryPumpIfNeeded(__in LONG64 lastStableLsn) override;

            KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> GetOldTransactions(__in TransactionMap & transactionMap) override;

            bool IsGoodLogHeadCandidate(__in LogRecordLib::IndexingLogRecord & proposedLogHead);

            ReplicatedLogManager::IsGoodLogHeadCandidateCalculator GetGoodLogHeadCandidateCalculator() override;

            bool ShouldBlockOperationsOnPrimary() override;

            bool ShouldCheckpointOnPrimary(
                __in TransactionMap & transactionMap,
                __out KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & abortTxList) override;

            bool ShouldCheckpointOnSecondary(__in TransactionMap & transactionMap) override;

            bool ShouldIndex() override;

            bool ShouldTruncateHead() override;

            void ForceCheckpoint();

            void OnCheckpointCompleted(
                __in NTSTATUS status,
                __in Data::LogRecordLib::CheckpointState::Enum checkpointState,
                __in bool isRecoveredCheckpoint) override;

            void OnTruncationCompleted() override;

            void InitiatePeriodicCheckpoint() override;

            __declspec(property(get = get_PeriodicTruncationState)) PeriodicCheckpointTruncationState::Enum PeriodicCheckpointTruncationState;
            PeriodicCheckpointTruncationState::Enum get_PeriodicTruncationState() const override
            {
                return periodicCheckpointTruncationState_;
            }

            void Recover(
                __in LONG64 recoveredCheckpointTime,
                __in LONG64 recoveredTruncationTime) override;

        private:
            
            static const ULONG ThrottleAfterPendingCheckpointCount;

            // If an old transaction blocks checkpoint, it gets aborted.
            // The transaction record offset must be older by this factor of the current log usage
            static const ULONG AbortOldTxFactor;
            static const ULONG MBtoBytesMultiplier;

            // Factor used for deciding whether it is worth truncating.
            static const ULONG MinTruncationFactor;
            static const ULONG NumberOfIndexRecordsPerCheckpoint;

            // Interval at which to update the configuration values
            static const Common::TimeSpan ConfigUpdateIntervalInSeconds;

            LogTruncationManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager const & replicatedLogManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            ULONG64 GetBytesUsed(__in LogRecordLib::LogRecord const & startingRecord);

            ULONG64 GetCurrentTailPosition();

            //
            // Gets the min truncation amount threshold in bytes.
            //
            ULONG64 GetMinTruncationAmountInBytes(__in ULONG64 checkpointThresholdInBytes);

            //
            // Gets the throttle threshold in bytes.
            // 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / DefaultMinLogDivider needs to be used.
            //
            ULONG64 GetThrottleThresholdInBytes(
                __in int64 throttleThresholdFactor,
                __in ULONG64 checkpointThresholdInBytes,
                __in ULONG64 minLogSizeInBytes);

            bool ShouldCheckpoint(
                __in TransactionMap & transactionMap,
                __out KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & abortTxList);

            // Updates the configuration values if the set duration has passed
            void RefreshConfigurationValues(bool forceRefresh = false);

            // Initiates periodic checkpoint if the configured interval has passed
            bool ShouldInitiatePeriodicCheckpoint();

            // Initiates periodic truncation if a periodic checkpoint has completed
            bool ShouldInitiatePeriodicTruncation();

            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            // Refers to the last update of class values from the shared config object
            Common::Stopwatch configUpdateStopWatch_;

            // We try to checkpoint after this size of non-checkpointed data
            ULONG64 checkpointIntervalBytes_;

            // Minimum size of the log we would like to keep.
            ULONG64 minLogSizeInBytes_;

            // We try to truncate the log when it reaches this usage
            ULONG64 truncationThresholdInBytes_;

            // Size of the log at which throttling will be initiated.
            ULONG64 throttleAtLogUsageBytes_;

            // Minimum size of the log we would like to keep.
            ULONG64 minTruncationAmountInBytes_;

            // We try to insert an indexing log record after this size of log since the last index log record.
            ULONG64 indexIntervalBytes_;

            // Threshold above which in-flight transactions will start getting aborted.
            ULONG64 txnAbortThresholdInBytes_;

            ReplicatedLogManager::CSPtr replicatedLogManager_;

            Common::atomic_bool forceCheckpoint_;

            PeriodicCheckpointTruncationState::Enum periodicCheckpointTruncationState_;
        };
    }
}
