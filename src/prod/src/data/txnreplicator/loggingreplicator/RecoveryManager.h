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
        // Recovery Manager is used during recovery to read the log and restore
        // the in memory state of the replicator to the same state as what it would have been
        // before the replica going down
        //
        class RecoveryManager final
            : public KObject<RecoveryManager>
            , public KShared<RecoveryManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(RecoveryManager)
            
        public:

            static RecoveryManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogManager & logManager,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            __declspec(property(get = get_RecoveryError)) NTSTATUS RecoveryError;
            NTSTATUS get_RecoveryError()
            {
                return recoveryError_;
            }

            __declspec(property(get = get_RecoveredLsn)) LONG64 RecoveredLsn;
            LONG64 get_RecoveredLsn()
            {
                return recoveredLsn_;
            }

            __declspec(property(get = get_LastRecoveredAtomicRedoOperationLsn)) LONG64 LastRecoveredAtomicRedoOperationLsn;
            LONG64 get_LastRecoveredAtomicRedoOperationLsn()
            {
                return lastRecoveredAtomicRedoOperationLsn_;
            }

            __declspec(property(get = get_IsRemovingStateAfterOpen)) bool IsRemovingStateAfterOpen;
            bool get_IsRemovingStateAfterOpen()
            {
                return isRemovingStateAfterOpen_;
            }

            __declspec(property(get = get_RecoveredLastEndCheckpointRecord)) LogRecordLib::EndCheckpointLogRecord::SPtr RecoveredLastEndCheckpointRecord;
            LogRecordLib::EndCheckpointLogRecord::SPtr get_RecoveredLastEndCheckpointRecord()
            {
                return recoveredLastEndCheckpointRecord_;
            }

            __declspec(property(get = get_RecoveryLogsReader)) LogRecordLib::RecoveryPhysicalLogReader::SPtr RecoveryLogsReader;
            LogRecordLib::RecoveryPhysicalLogReader::SPtr get_RecoveryLogsReader()
            {
                return recoveryLogReader_;
            }
    
            ktl::Awaitable<TxnReplicator::RecoveryInformation> OpenAsync(
                __in bool shouldLocalStateBeRemoved,
                __in LogRecordLib::RecoveryPhysicalLogReader & recoveryReader,
                __in bool isRestoring);

            ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in OperationProcessor & operationProcessor,
                __in CheckpointManager & checkpointManager,
                __in TransactionManager & transactionManager,
                __in LogRecordsDispatcher & logRecordsDispatcher,
                __in ReplicatedLogManager & replicatedLogManager,
                __in LONG64 readAheadSize,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & config);

            void OnTailTruncation(__in LONG64 newTailLsn);

            void Reuse();

            void DisposeRecoveryReadStreamIfNeeded();

        private:

            RecoveryManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogManager & logManager,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            LogManager::SPtr const logManager_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;

            NTSTATUS recoveryError_;
            bool logCompleteCheckpointAfterRecovery_;
            PhysicalLogWriterCallbackManager::SPtr callbackManager_;
            LONG64 recoveredLsn_;
            bool isRemovingStateAfterOpen_;
            LONG64 lastRecoveredAtomicRedoOperationLsn_;

            LogRecordLib::LogRecord::SPtr tailRecordAtStart_;
            LogRecordLib::EndCheckpointLogRecord::SPtr recoveredLastEndCheckpointRecord_;
            LogRecordLib::RecoveryPhysicalLogReader::SPtr recoveryLogReader_;
        };
    }
}
