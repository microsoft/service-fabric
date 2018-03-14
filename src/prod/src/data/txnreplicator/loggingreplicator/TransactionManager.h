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
        // Manages transaction state 
        //
        class TransactionManager final
            : public KObject<TransactionManager>
            , public KShared<TransactionManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(TransactionManager)

        public:
 
            static TransactionManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState const & recoveredOrCopiedCheckpointLsn,
                __in TransactionMap & transactionMap,
                __in CheckpointManager & checkpointManager,
                __in ReplicatedLogManager & replicatedLogManager,
                __in OperationProcessor & operationProcessor,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in KAllocator & allocator);

            __declspec(property(get = get_TransactionMap)) TransactionMap::SPtr TransactionMapObject;
            TransactionMap::SPtr get_TransactionMap() const
            {
                // Never re-assigned after construction. Safe to read
                return transactionMap_;
            }

            void CreateTransactionState(
                __in TxnReplicator::Epoch const & tailEpoch,
                __in LogRecordLib::TransactionLogRecord & record) noexcept;

            ktl::Awaitable<NTSTATUS> AbortTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __out LONG64 & abortLsn) noexcept;

            NTSTATUS AddOperation(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept;

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicOperation & atomicOperation,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept;

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept;

            NTSTATUS BeginTransaction(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept;

            ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept;

            ktl::Awaitable<NTSTATUS> CommitTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __out LONG64 & commitLsn) noexcept;

            ktl::Awaitable<NTSTATUS> EndTransactionAsync(
                __in TxnReplicator::TransactionBase & transaction,
                __in bool isCommitted,
                __in bool isThisReplicaTransaction,
                __out LogRecordLib::EndTransactionLogRecord::SPtr & result) noexcept;

        private:

            // Interval at which to update the configuration values
            static const Common::TimeSpan ConfigUpdateIntervalInSeconds;

            TransactionManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState const & recoveredOrCopiedCheckpointLsn,
                __in TransactionMap & transactionMap,
                __in CheckpointManager & checkpointManager,
                __in ReplicatedLogManager & replicatedLogManager,
                __in OperationProcessor & operationProcessor,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters);

            NTSTATUS InitiateLogicalRecordProcessingOnPrimary(__in LogRecordLib::LogicalLogRecord & record) noexcept;

            ktl::Task ProcessLogicalRecordOnPrimary(__in LogRecordLib::TransactionLogRecord & record);

            ktl::Awaitable<NTSTATUS> ProcessLogicalRecordOnPrimaryAsync(__in LogRecordLib::TransactionLogRecord & record) noexcept;

            NTSTATUS ReplicateAndLogLogicalRecord(__in LogRecordLib::TransactionLogRecord & record) noexcept;

            void InvokeUnlockCallback(__in LogRecordLib::TransactionLogRecord & record) noexcept;

            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            // Refers to the last update of class values from the shared config object
            Common::Stopwatch configUpdateStopWatch_;

            RecoveredOrCopiedCheckpointState::CSPtr recoveredOrCopiedCheckpointState_;
            TransactionMap::SPtr transactionMap_;
            CheckpointManager::SPtr checkpointManager_;
            ReplicatedLogManager::SPtr replicatedLogManager_;
            OperationProcessor::SPtr operationProcessor_;
            LogRecordLib::InvalidLogRecords::SPtr invalidLogRecords_;

            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;
        };
    }
}
