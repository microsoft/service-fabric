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
        // Responsible for processing the operations of a transaction and other replicator operations
        // Interacts with the state provider
        //
        class OperationProcessor final
            : public Data::LoggingReplicator::IOperationProcessor
            , public KObject<OperationProcessor>
            , public KShared<OperationProcessor>
            , public KWeakRefType<OperationProcessor>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(OperationProcessor);
            K_SHARED_INTERFACE_IMP(IOperationProcessor);
            K_WEAKREF_INTERFACE_IMP(IOperationProcessor, OperationProcessor);

        public:

            static OperationProcessor::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in RoleContextDrainState & roleContextDrainState,
                __in TxnReplicator::ILoggingReplicatorToVersionManager & versionManager,
                __in ICheckpointManager & checkpointManager,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in KAllocator & allocator);

            __declspec(property(get = get_LastAppliedBarrierRecord)) LogRecordLib::LogRecord::CSPtr LastAppliedBarrierRecord;
            LogRecordLib::LogRecord::CSPtr get_LastAppliedBarrierRecord() const
            {
                return lastAppliedBarrierRecord_.Get();
            }

            __declspec(property(get = get_ServiceError)) NTSTATUS ServiceError;
            NTSTATUS get_ServiceError() const
            {
                return serviceError_.load();
            }

            __declspec(property(get = get_LogError)) NTSTATUS LogError;
            NTSTATUS get_LogError() const
            {
                return logError_.load();
            }

            __declspec(property(get = get_ChangeHandler)) TxnReplicator::ITransactionChangeHandler::SPtr ChangeHandler;
            TxnReplicator::ITransactionChangeHandler::SPtr get_ChangeHandler() const
            {
                return changeHandlerCache_.Get();
            }

            void Reuse();
            
            void TruncateTail(__in LogRecordLib::LogRecord & currentRecord);

            RecordProcessingMode::Enum IdentifyProcessingModeForRecord(__in LogRecordLib::LogRecord const & record) override;

            ktl::Awaitable<void> ImmediatelyProcessRecordAsync(
                __in LogRecordLib::LogRecord & logRecord,
                __in NTSTATUS flushErrorCode,
                __in RecordProcessingMode::Enum processingMode) override;

            void PrepareToProcessLogicalRecord() override;

            void PrepareToProcessPhysicalRecord() override;

            ktl::Awaitable<void> ProcessLoggedRecordAsync(__in LogRecordLib::LogRecord & logRecord) override;

            void UpdateDispatchingBarrierTask(__in TxnReplicator::CompletionTask & barrierTask) override;

            //
            // Callback for logical log record processing completion
            //
            void ProcessedLogicalRecord(__in LogRecordLib::LogicalLogRecord & logicalRecord);

            //
            // Callback for physical log record processing completion
            //
            void ProcessedPhysicalRecord(__in LogRecordLib::PhysicalLogRecord & physicalRecord);

            //
            // Should NOT be invoked along with WaitForAllRecordsProcessingAsync
            //
            // Waits for all logical records to be processed
            //
            ktl::Awaitable<void> WaitForLogicalRecordsProcessingAsync();

            //
            // Should NOT be invoked along with WaitForAllRecordsProcessingAsync
            //
            // Waits for all physical records to be processed
            //
            ktl::Awaitable<void> WaitForPhysicalRecordsProcessingAsync();

            //
            // Should NOT be invoked along with either of the above 2 methods
            //
            // Waits for all records to be processed
            //
            ktl::Awaitable<void> WaitForAllRecordsProcessingAsync();

            void ProcessLogError(
                __in KStringView const & component,
                __in LogRecordLib::LogRecord const & record,
                __in NTSTATUS error);

            NTSTATUS ProcessServiceError(
                __in KStringView const & component,
                __in LogRecordLib::LogRecord const & record,
                __in NTSTATUS error);

            void Unlock(__in LogRecordLib::LogicalLogRecord & record) override;

            // Notification APIs
            NTSTATUS RegisterTransactionChangeHandler(
                __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

            NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        private:

            OperationProcessor(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in RoleContextDrainState & roleContextDrainState,
                __in TxnReplicator::ILoggingReplicatorToVersionManager & versionManager,
                __in ICheckpointManager & checkpointManager,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator);

            ktl::Awaitable<void> ApplyCallback(__in LogRecordLib::LogRecord & record) noexcept;

            void FireCommitNotification(__in TxnReplicator::TransactionBase const & transaction);

            bool ProcessError(
                __in KStringView const & component,
                __in LogRecordLib::LogRecord const & record,
                __in NTSTATUS errorCode);

            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;
            ICheckpointManager::SPtr const checkpointManager_;
            TxnReplicator::ILoggingReplicatorToVersionManager::SPtr const versionManager_;
            RoleContextDrainState::SPtr const roleContextDrainState_;
            RecoveredOrCopiedCheckpointState::SPtr const recoveredOrCopiedCheckpointState_;
            KWeakRef<TxnReplicator::IStateProviderManager>::SPtr stateManager_;
            IBackupManager::SPtr const backupManager_;

            Common::atomic_long numberOfLogicalRecordsBeingProcessed_;
            Common::atomic_long numberOfPhysicalRecordsBeingProcessed_;
            ktl::AwaitableCompletionSource<LogRecordLib::PhysicalLogRecord::SPtr>::SPtr physicalRecordsProcessingTcs_;
            ktl::AwaitableCompletionSource<LogRecordLib::LogicalLogRecord::SPtr>::SPtr logicalRecordsProcessingTcs_;
            Common::atomic_long serviceError_;
            Common::atomic_long logError_;
            Utilities::ThreadSafeCSPtrCache<LogRecordLib::LogRecord> lastAppliedBarrierRecord_;
            Utilities::ThreadSafeSPtrCache<TxnReplicator::ITransactionChangeHandler> changeHandlerCache_;

            // Pointer to a configuration object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            bool const enableSecondaryCommitApplyAcknowledgement_;

            TxnReplicator::ITransactionalReplicator * transactionalReplicator_;
        };
    }
}
