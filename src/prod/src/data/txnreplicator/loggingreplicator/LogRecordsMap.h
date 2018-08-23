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
        // Class used for linking log records.
        //
        class LogRecordsMap final
            : public KObject<LogRecordsMap>
            , public KShared<LogRecordsMap>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(LogRecordsMap)
            
        public:

            typedef enum Mode
            {
                Recovery,
                Restore,
            }Mode;

            static LogRecordsMap::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogRecordLib::IndexingLogRecord & indexingLogRecord,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            static LogRecordsMap::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LONG64 startingLogicalSequenceNumber,
                __in TransactionMap & transactionMap,
                __in TxnReplicator::Epoch const & currentLogTailEpoch,
                __in LONG64 lastStableLsn,
                __in LogRecordLib::ProgressVector & progressVector,
                __in LogRecordLib::BeginCheckpointLogRecord & recoveredLastCompletedBeginCheckpointRecord,
                __in LogRecordLib::EndCheckpointLogRecord & recoveredLastEndCheckpointRecord,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

             __declspec(property(get = get_CurrentLogTailEpoch)) TxnReplicator::Epoch CurrentLogTailEpoch;
            TxnReplicator::Epoch get_CurrentLogTailEpoch()
            {
                return currentLogTailEpoch_;
            }

            __declspec(property(get = get_LastCompletedEndCheckpointRecord)) LogRecordLib::EndCheckpointLogRecord::SPtr LastCompletedEndCheckpointRecord;
            LogRecordLib::EndCheckpointLogRecord::SPtr get_LastCompletedEndCheckpointRecord()
            {
                return lastCompletedEndCheckpointRecord_;
            }

             __declspec(property(get = get_LastInProgressCheckpointRecord)) LogRecordLib::BeginCheckpointLogRecord::SPtr LastInProgressCheckpointRecord;
            LogRecordLib::BeginCheckpointLogRecord::SPtr get_LastInProgressCheckpointRecord()
            {
                return lastInProgressCheckpointRecord_;
            }

            __declspec(property(get = get_LastLinkedPhysicalRecord)) LogRecordLib::PhysicalLogRecord::SPtr LastLinkedPhysicalRecord;
            LogRecordLib::PhysicalLogRecord::SPtr get_LastLinkedPhysicalRecord()
            {
                return lastLinkedPhysicalRecord_;
            }

             __declspec(property(get = get_LastPhysicalRecord)) LogRecordLib::PhysicalLogRecord::SPtr LastPhysicalRecord;
            LogRecordLib::PhysicalLogRecord::SPtr get_LastPhysicalRecord()
            {
                return lastPhysicalRecord_;
            }

            __declspec(property(get = get_LastLogicalSequenceNumber)) LONG64 LastLogicalSequenceNumber;
            LONG64 get_LastLogicalSequenceNumber()
            {
                return lastLogicalSequenceNumber_;
            }

            __declspec(property(get = get_LastRecoveredAtomicRedoOperationLsn)) LONG64 LastRecoveredAtomicRedoOperationLsn;
            LONG64 get_LastRecoveredAtomicRedoOperationLsn()
            {
                return lastRecoveredAtomicRedoOperationLsn_;
            }

            __declspec(property(get = get_LastStableLsn)) LONG64 LastStableLsn;
            LONG64 get_LastStableLsn()
            {
                return lastStableLsn_;
            }

            __declspec(property(get = get_ProgressVector)) LogRecordLib::ProgressVector::SPtr ProgressVectorValue;
            LogRecordLib::ProgressVector::SPtr get_ProgressVector()
            {
                return progressVector_;
            }

            __declspec(property(get = get_TransactionMap)) TransactionMap::SPtr TransactionMapValue;
            TransactionMap::SPtr get_TransactionMap()
            {
                return transactionMap_;
            }

            __declspec(property(get = get_LastPeriodicTruncationTime)) LONG64 LastPeriodicTruncationTimeTicks;
            LONG64 get_LastPeriodicTruncationTime() const
            {
                return lastPeriodicTruncationTimeTicks_;
            }

            void ProcessLogRecord(__in LogRecordLib::LogRecord & logRecord);

            void ProcessLogRecord(
                __in LogRecordLib::LogRecord & logRecord,
                __out bool & isRecoverableRecord);

        private:

            LogRecordsMap(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogRecordLib::IndexingLogRecord & indexingLogRecord,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            LogRecordsMap(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LONG64 startingLogicalSequenceNumber,
                __in TransactionMap & transactionMap,
                __in TxnReplicator::Epoch const & currentLogTailEpoch,
                __in LONG64 lastStableLsn,
                __in LogRecordLib::ProgressVector & progressVector,
                __in LogRecordLib::BeginCheckpointLogRecord & recoveredLastCompletedBeginCheckpointRecord,
                __in LogRecordLib::EndCheckpointLogRecord & recoveredLastEndCheckpointRecord,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            void ProcessLogRecord(__in LogRecordLib::PhysicalLogRecord & physicalLogRecord);

            void ProcessLogRecord(
                __in LogRecordLib::BeginTransactionOperationLogRecord & beginTransactionRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::OperationLogRecord & operationLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::EndTransactionLogRecord & endTransactionLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::BarrierLogRecord & barrierLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::BackupLogRecord & backupLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::BeginCheckpointLogRecord & beginCheckpointLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::EndCheckpointLogRecord & endCheckpointLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::CompleteCheckPointLogRecord & completeCheckpointLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::TruncateHeadLogRecord & truncateHeadLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::IndexingLogRecord & indexingLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::InformationLogRecord & informationLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::TruncateTailLogRecord & truncateTailLogRecord,
                __out bool & isRecoverableRecord);

            void ProcessLogRecord(
                __in LogRecordLib::UpdateEpochLogRecord & updateEpochLogRecord,
                __out bool & isRecoverableRecord);

            LogRecordLib::IndexingLogRecord::SPtr const firstIndexingLogRecord_;
            Mode const mode_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;

            LogRecordLib::BeginCheckpointLogRecord::SPtr recoveredLastCompletedBeginCheckpointRecord_;
            LogRecordLib::EndCheckpointLogRecord::SPtr recoveredLastEndCheckpointRecord_;

            LogRecordLib::EndCheckpointLogRecord::SPtr lastCompletedEndCheckpointRecord_;
            LogRecordLib::BeginCheckpointLogRecord::SPtr lastInProgressCheckpointRecord_;
            LogRecordLib::PhysicalLogRecord::SPtr lastLinkedPhysicalRecord_;
            LogRecordLib::PhysicalLogRecord::SPtr lastPhysicalRecord_;

            LONG64 lastLogicalSequenceNumber_;
            LONG64 lastRecoveredAtomicRedoOperationLsn_;
            LONG64 lastStableLsn_;

            TxnReplicator::Epoch currentLogTailEpoch_;
            LogRecordLib::ProgressVector::SPtr progressVector_;
            TransactionMap::SPtr transactionMap_;

            LONG64 lastPeriodicTruncationTimeTicks_;
        };
    }
}
