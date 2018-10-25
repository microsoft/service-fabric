// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        #define DECLARE_LR_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
        #define LR_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::LR, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        class EventSource
        {
        public:
            DECLARE_LR_STRUCTURED_TRACE(Exception, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, int);
            DECLARE_LR_STRUCTURED_TRACE(ExceptionInfo, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, int);
            DECLARE_LR_STRUCTURED_TRACE(ExceptionError, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, int);
            DECLARE_LR_STRUCTURED_TRACE(Ctor, Common::Guid, LONG64, std::wstring, uintptr_t);
            DECLARE_LR_STRUCTURED_TRACE(Dtor, Common::Guid, LONG64, std::wstring, uintptr_t);

            // CheckpointManager
            DECLARE_LR_STRUCTURED_TRACE(CheckpointManagerAcquireLock, Common::Guid, LONG64, Common::WStringLiteral, Common::TimeSpan);
            DECLARE_LR_STRUCTURED_TRACE(CopyOrBuildReplica, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(Checkpoint, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::CheckpointState::Trace, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AbortOldTransactions, Common::Guid, LONG64, ULONG32, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AbortOldTransactionsDone, Common::Guid, LONG64, ULONG32, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(PrimaryFullCopy, Common::Guid, LONG64, LogRecordLib::FullCopyReason::Trace, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(PrimaryCopy, Common::Guid, LONG64, LONG64, ULONGLONG, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(TruncateHead, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::TruncationState::Trace, LONG64, LONG64, ULONGLONG, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(ProcessBarrierRecord, Common::Guid, LONG64, DrainingStream::Trace, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(PeriodicCheckpointAndTruncation, Common::Guid, LONG64, PeriodicCheckpointTruncationState::Trace, Common::DateTime, Common::DateTime);

            // CopyStream
            DECLARE_LR_STRUCTURED_TRACE(CopyStream, Common::Guid, LONG64, std::wstring, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamGetNextState, Common::Guid, LONG64, LONG64, ULONG32);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamGetNextLogs, Common::Guid, LONG64, LONG64, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamGetNextFalseProgress, Common::Guid, LONG64, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamGetNextLogRecord, Common::Guid, LONG64, LONG64, LONG64, LONG64, Common::TimeSpan, Common::TimeSpan);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamFinished, Common::Guid, LONG64, LONG64, ULONG32);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamMetadata, Common::Guid, LONG64, LONG64, LogRecordLib::CopyStage::Trace, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopyStreamStateMetadata, Common::Guid, LONG64, LONG64, LogRecordLib::CopyStage::Trace, LONG64, LONG64, ULONG32, LONG64);

            // KLogManager
            DECLARE_LR_STRUCTURED_TRACE(KLMCreateCopyLog, Common::Guid, LONG64, Common::WStringLiteral, Common::WStringLiteral, Common::Guid, int);
            DECLARE_LR_STRUCTURED_TRACE(KLMDeleteLogFileStart, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(KLMDeleteLogFileFailed, Common::Guid, LONG64, std::wstring, Common::WStringLiteral, int);
            DECLARE_LR_STRUCTURED_TRACE(KLMError, Common::Guid, LONG64, std::wstring, Common::Guid, Common::WStringLiteral, int);
            DECLARE_LR_STRUCTURED_TRACE(KLMOpenLogicalLog, Common::Guid, LONG64, Common::WStringLiteral, Common::Guid, int);
            DECLARE_LR_STRUCTURED_TRACE(KLMRenameCopyLogError, Common::Guid, LONG64, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, int);
            DECLARE_LR_STRUCTURED_TRACE(KLMRenameCopyLog, Common::Guid, LONG64, ULONGLONG, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(KLMLogFileNotFound, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(KLMCreateLog, Common::Guid, LONG64, Common::WStringLiteral, Common::Guid, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(KLMMappedDedicatedLogPathForContainer, Common::Guid, LONG64, Common::WStringLiteral, Common::WStringLiteral);

            // LoggingReplicatorImpl
            DECLARE_LR_STRUCTURED_TRACE(Open, Common::Guid, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(Recover, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(Close, Common::Guid, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(BecomeSecondary, Common::Guid, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(BecomeNone, Common::Guid, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(BecomePrimary, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(BecomePrimaryAbortTransaction, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(BecomePrimaryAbortTransactionStop, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DeleteLog, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(FlushedRecordsCallback, Common::Guid, LONG64, LONG64, std::vector<FlushedRecordInfo>);
            DECLARE_LR_STRUCTURED_TRACE(CreateAllManagers, Common::Guid, LONG64);

            DECLARE_LR_STRUCTURED_TRACE(UpdateEpoch, Common::Guid, LONG64, LONG64, LONG64, LONG64, int, DrainingStream::Trace);
            DECLARE_LR_STRUCTURED_TRACE(GetCopyContext, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(GetCopyState, Common::Guid, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(QuiesceActivity, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG, bool);
            DECLARE_LR_STRUCTURED_TRACE(TransitionOutOfPrimaryUnlock, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(OnDataLossAsync, Common::Guid, LONG64, std::wstring, int);
            DECLARE_LR_STRUCTURED_TRACE(ProgressVectorValidationTelemetry, Common::Guid, LONG64, std::wstring, std::wstring, ULONG32, std::wstring, std::wstring, ULONG32, std::wstring);

            // LogManager
            DECLARE_LR_STRUCTURED_TRACE(ProcessLogHeadTruncationWaiting, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(ProcessLogHeadTruncationDone, Common::Guid, LONG64, LONG64, LONG64, ULONG64, ULONG32, ULONG32);
            DECLARE_LR_STRUCTURED_TRACE(AddLogReader, Common::Guid, LONG64, ULONGLONG, ULONGLONG, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(RemoveLogReader, Common::Guid, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RemoveLogReaderInitiateTruncation, Common::Guid, LONG64, ULONGLONG);

            // LogRecordsDispatcher
            DECLARE_LR_STRUCTURED_TRACE(DispatcherConcurrentRecords, Common::Guid, LONG64, ULONG32, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DispatcherBarrier, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DispatcherPauseAtBarrier, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DispatcherContinue, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DispatcherDrainStart, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DispatcherDrainFinish, Common::Guid, LONG64, LONG64, LONG64, int);

            // LogTruncationManager
            DECLARE_LR_STRUCTURED_TRACE(DrainBlocked, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DrainContinue, Common::Guid, LONG64, ULONGLONG, LONG64);

            // OperationProcessor
            DECLARE_LR_STRUCTURED_TRACE(OPRecordProcessedImmediately, Common::Guid, LONG64, DrainingStream::Trace, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(OPRecordProcessedImmediatelyInfo, Common::Guid, LONG64, DrainingStream::Trace, LONG64, LogRecordLib::InformationEvent::Trace);
            DECLARE_LR_STRUCTURED_TRACE(OPApplyCallbackTransactionRecord, Common::Guid, LONG64, DrainingStream::Trace, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(OPWaitForRecordsProcessing, Common::Guid, LONG64, Common::StringLiteral, int);
            DECLARE_LR_STRUCTURED_TRACE(OPWaitForRecordsProcessingDone, Common::Guid, LONG64, Common::StringLiteral, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);

            // TruncateTailManager
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailSingleOperationTransactionRecord, Common::Guid, LONG64, Common::StringLiteral, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailTransactionRecord, Common::Guid, LONG64, Common::StringLiteral, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailAtomicOperation, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailOperationRecord, Common::Guid, LONG64, Common::StringLiteral, LONG64, LONG64, ULONGLONG, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailBarrier, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailBackup, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailUpdateEpoch, Common::Guid, LONG64, LONG64, LONG64, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTailDone, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);

            // PhysicalLogWriter
            DECLARE_LR_STRUCTURED_TRACE(FlushAsync, Common::Guid, LONG64, Common::WStringLiteral, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(FlushStart, Common::Guid, LONG64, ULONG32, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(FlushEnd, Common::Guid, LONG64, ULONG32, ULONG32, LONG64, LONG64, double, double, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(FlushEndWarning, Common::Guid, LONG64, ULONG32, ULONG32, LONG64, LONG64, double, double, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(PendingFlushWarning, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(FlushInvoke, Common::Guid, LONG64, Common::WStringLiteral);

            // FileLogManager
            DECLARE_LR_STRUCTURED_TRACE(FileLogManagerDeleteLogFailed, Common::Guid, LONG64, Common::ErrorCode, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(FileLogManagerDeleteFile, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(FileLogManagerDeleteFileFailed, Common::Guid, LONG64, Common::WStringLiteral, Common::ErrorCode);
            DECLARE_LR_STRUCTURED_TRACE(FileLogManagerRenameCopyLogAtomically, Common::Guid, LONG64, ULONGLONG, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(FileLogManagerRenameCopyLogAtomicallyException, Common::Guid, LONG64, int, Common::StringLiteral);

            // RecoveryManager
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncLogUsage, Common::Guid, LONG64, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncLastPhysicalRecord, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncSkippingRecovery, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLinkedPhysicalRecord, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredTruncateHeadRecord, Common::Guid, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLastCCRecord, Common::Guid, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredCCRecordInfo, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncCCRecordNotFound, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLastECRecord, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMPerformRecoveryAsync, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG, ULONGLONG, LONG64, LONG64, bool, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(RMPerformRecoveryAsyncTailRecordType, Common::Guid, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(RMRecoveryFailed, Common::Guid, LONG64, int, int, LogRecordLib::LogRecordType::Trace, LONG64, LONG64);

            // TransactionManager
            DECLARE_LR_STRUCTURED_TRACE(AcceptSingleOperationTransaction, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AcceptBeginTransaction, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AcceptAtomicOperation, Common::Guid, LONG64, LONG64, LONG64, bool);
            DECLARE_LR_STRUCTURED_TRACE(AcceptOperation, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AcceptEndTransaction, Common::Guid, LONG64, LONG64, LONG64, bool);

            // ReplicatedLogManager
            DECLARE_LR_STRUCTURED_TRACE(UpdateEpochRecord, Common::Guid, LONG64, LONG64, LONG64, LONG64, ULONG32);
            DECLARE_LR_STRUCTURED_TRACE(AcceptBarrier, Common::Guid, LONG64, LONG64);

            // SecondaryDrainManager 
            DECLARE_LR_STRUCTURED_TRACE(EnableSecondaryCommitApplyAcknowledgement, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopiedUpdateEpoch, Common::Guid, LONG64, LONG64, LONG64, LONG64, int);
            DECLARE_LR_STRUCTURED_TRACE(CopyOrBuildReplicaStatus, Common::Guid, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopyOrBuildReplicaAborted, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(CopyOrBuildReplicaStarted, Common::Guid, LONG64, LONG64, LONG64, LONG64, LONG64, LONG64, LONG64, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(UpdateEpochRecordDueToFullCopy, Common::Guid, LONG64, LONG64, LONG64, LONG64, int);
            DECLARE_LR_STRUCTURED_TRACE(CopyAckedProgress, Common::Guid, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DrainCopyReceive, Common::Guid, LONG64, LONG64, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DrainCopyStreamCompleted, Common::Guid, LONG64, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(DrainReplicationReceive, Common::Guid, LONG64, LONG64, LogRecordLib::LogRecordType::Trace, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(AwaitFlushingReplicatedRecordTask, Common::Guid, LONG64, LONG64, LONG64, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DrainStart, Common::Guid, LONG64, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(DrainStateNoise, Common::Guid, LONG64, std::wstring, LONG64);
            DECLARE_LR_STRUCTURED_TRACE(DrainCompleted, Common::Guid, LONG64, std::wstring, std::wstring, LONG64, std::wstring, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONGLONG);
            DECLARE_LR_STRUCTURED_TRACE(ThrottlingWrites, Common::Guid, LONG64, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(TruncateTail, Common::Guid, LONG64, LONG64);

            // BackupManager : Backup
            DECLARE_LR_STRUCTURED_TRACE(IBM_BackupAsyncStart, Common::Guid, LONG64, Common::Guid, Common::Guid, int, bool, LONG64, LONG64, LONG64, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(IBM_BackupAsync, Common::Guid, LONG64, Common::Guid, Common::Guid, LONG64, LONG64, LONG64, int64, int64, int64, int64, int64, int64, int64);
            DECLARE_LR_STRUCTURED_TRACE(IBM_BackupAsyncFinish, Common::Guid, LONG64, Common::Guid, Common::Guid, LONG64, LONG64, LONG64, int64, int64, int64, int64, int64, int64, int64);
            DECLARE_LR_STRUCTURED_TRACE(IBM_BackupAsyncFinishError, Common::Guid, LONG64, Common::Guid, std::wstring, int, std::wstring);

            // Backup Manager : Restore
            DECLARE_LR_STRUCTURED_TRACE(IBM_RestoreAsyncStart, Common::Guid, LONG64, Common::Guid, LONG64, LONG64, LONG64, ULONGLONG, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(IBM_RestoreAsync, Common::Guid, LONG64, Common::Guid, int64, int64, int64, int64);
            DECLARE_LR_STRUCTURED_TRACE(IBM_RestoreAsyncFinish, Common::Guid, LONG64, Common::Guid, int64, int64, int64, int64);
            DECLARE_LR_STRUCTURED_TRACE(IBM_RestoreAsyncFinishError, Common::Guid, LONG64, Common::Guid, int, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(RestoreTokenCreated, Common::Guid, LONG64, Common::Guid, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(RestoreTokenDeleted, Common::Guid, LONG64, Common::Guid, Common::WStringLiteral);
            DECLARE_LR_STRUCTURED_TRACE(RestoreRecord, Common::Guid, LONG64, LONG64, FABRIC_SEQUENCE_NUMBER, LONG64, LONG64);

            // BackupManager : Other
            DECLARE_LR_STRUCTURED_TRACE(BackupRestoreWarning, Common::Guid, LONG64, bool, Common::Guid, std::wstring);
            DECLARE_LR_STRUCTURED_TRACE(UndoBackupLogRecord, Common::Guid, LONG64, LONG64, LONG64, ULONG64, LONG64, LONG64, LONG64);

            // BackupFolderInfo
            DECLARE_LR_STRUCTURED_TRACE(BackupFolderWarning, Common::Guid, LONG64, Common::WStringLiteral, std::wstring, int);

            // PhysicalLogWriterCallbackManager
            DECLARE_LR_STRUCTURED_TRACE(NullCallbackProcessor, Common::Guid, LONG64, LONG64, LONG64);

            // FlushedRecordInfo. Note: Accepts contextSequenceId
            DECLARE_LR_STRUCTURED_TRACE(FlushedRecordInfo, uint16, LogRecordLib::LogRecordType::Trace, LONG64, LONG64, ULONG64);

            EventSource() :
                // Structured trace reserved areas.
                //  1. TraceType: [0, 3]
                //  2. General: [4, 10]
                //  2. CheckpointManager: [11, 20]
                //  3. CopyStream: [21, 30]
                //  4. KLogManager: [31, 40]
                //  5. LoggingReplicatorImpl: [41, 65]
                //  6. LogManager: [66, 75]
                //  7. LogRecordsDispatcher: [76, 85]
                //  8. LogTruncationManager: [86, 90]
                //  9. OperationProcessor: [91, 100]
                // 10. TruncateTailManager: [101, 110]
                // 11. PhysicalLogWriter: [111, 120]
                // 12. FileLogManager: [121, 130]
                // 13. RecoveryManager: [130, 140]
                // 14. TransactionManager: [141, 150]
                // 15. ReplicatedLogManager: [151, 160]
                // 16. SecondaryDrainManager: [161, 185]
                // 17. BackupManager: [186, 205]
                // 18. BackupFolderInfo: [206, 210] 
                // 19. PhysicalLogWriterCallbackManager [216, 220]
                //
                // Note: TraceId starts from 4, this is because 0-3 has already been defined as trace type: Info, Warning, Error, Noise.
                // To move the partition id to the type section, format like DSM.MetadataManagerLockForRead@{PartitionId}, the first parameter PartitionId should mark as "id".
                LR_STRUCTURED_TRACE(Exception, 4, Warning, "{1}: Message: {2} LogRecordType: {3} Lsn: {4} Psn: {5} NTSTATUS: {6:x} \r\n", "id", "ReplicaId", "message", "recordtype", "lsn", "psn", "status"),
                LR_STRUCTURED_TRACE(ExceptionInfo, 5, Info, "{1}: Message: {2} LogRecordType: {3} Lsn: {4} Psn: {5} NTSTATUS: {6:x} \r\n", "id", "ReplicaId", "message", "recordtype", "lsn", "psn", "status"),
                LR_STRUCTURED_TRACE(ExceptionError, 6, Error, "{1}: Message: {2} LogRecordType: {3} Lsn: {4} Psn: {5} NTSTATUS: {6:x} \r\n", "id", "ReplicaId", "message", "recordtype", "lsn", "psn", "status"),
                LR_STRUCTURED_TRACE(Ctor, 7, Info, "{1}: {2} this={3}", "id", "replicaid", "className", "this"),
                LR_STRUCTURED_TRACE(Dtor, 8, Info, "{1}: {2} this={3}", "id", "replicaid", "className", "this"),

                // CheckpointManager
                LR_STRUCTURED_TRACE(CheckpointManagerAcquireLock, 11, Noise, "{1}: LockName: {2} AcquireTime: {3}", "id", "ReplicaId", "lock", "time"),
                LR_STRUCTURED_TRACE(CopyOrBuildReplica, 12, Info, "{1}: Copied Replica is now consistent", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(Checkpoint, 13, Info, "{1}: Action: {2} State: {3} LSN: {4} PSN: {5} Position: {6} EarliestPendingTxLSN: {7}", "id", "ReplicaId", "reason", "checkpointState", "lsn", "psn", "position", "earliest"),
                LR_STRUCTURED_TRACE(AbortOldTransactions, 14, Info, "{1}: Scheduling to abort {2} transactions First Lsn: {3} Last Lsn: {4} CheckpointPreventedAtLsn: {5}", "id", "ReplicaId", "num", "first", "last", "prevented"),
                LR_STRUCTURED_TRACE(AbortOldTransactionsDone, 15, Info, "{1}: Executed abort {2} transactions First Lsn: {3} Last Lsn: {4} CheckpointPreventedAtLsn: {5}", "id", "ReplicaId", "num", "first", "last", "prevented"),
                LR_STRUCTURED_TRACE(PrimaryFullCopy, 16, Info, "{1}: Primary full copy due to {2} for target replica {3}", "id", "ReplicaId", "reason", "target"),
                LR_STRUCTURED_TRACE(PrimaryCopy, 17, Info, "{1}: GetLogRecordsToCopy for target replica {2} from {3}->{4}", "id", "ReplicaId", "target", "start", "end"),
                LR_STRUCTURED_TRACE(TruncateHead, 18, Info, "{1}: Action: {2} State: {3} LSN: {4} PSN: {5} Position: {6} LogHeadLSN: {7} LogHeadPSN: {8} LogHeadPos: {9}", "id", "ReplicaId", "reason", "checkpointState", "lsn", "psn", "position", "logheadlsn", "logheadpsn", "logheadpos"),
                LR_STRUCTURED_TRACE(ProcessBarrierRecord, 19, Info, "{1}: Draining Stream: {2} PSN: {3} RecordStableLSN: {4} CurrentStableLSN: {5}", "id", "ReplicaId", "drainingstream", "psn", "recordstablelsn", "currentstablelsn"),
                LR_STRUCTURED_TRACE(PeriodicCheckpointAndTruncation, 20, Info, "{1}: State: {2}. Last Periodic Checkpoint : {3}. Last Periodic Truncation : {4}", "id", "ReplicaId", "periodicCheckpointTruncationState", "lastPeriodicCheckpointTime", "lastPeriodicTruncationTime"),

                // CopyStream
                LR_STRUCTURED_TRACE(CopyStream, 21, Info, "{1}: \r\n {2} \r\n {3}", "id", "ReplicaId", "source", "target"),
                LR_STRUCTURED_TRACE(CopyStreamGetNextState, 22, Noise, "{1}: TargetReplica: {2} Copying State Operation: {3}", "id", "ReplicaId", "targetId", "num"),
                LR_STRUCTURED_TRACE(CopyStreamGetNextLogs, 23, Info, "{1}: TargetReplica: {2} StartingLsn: {3} BC.LastStableLsn: {4} UptoLsn: {5}", "id", "ReplicaId", "targetId", "starting", "bc", "upto"),
                LR_STRUCTURED_TRACE(CopyStreamGetNextFalseProgress, 24, Info, "{1}: TargetReplica: {2} SourceStartingLsn: {3} TargetStartingLsn: {4}", "id", "ReplicaId", "targetId", "sourceS", "targetS"),
                LR_STRUCTURED_TRACE(CopyStreamGetNextLogRecord, 25, Info, "{1}: TargetReplica: {2} Copying LSNs: {3}-{4} Read and Serialization Time: {5} GetNextLatency: {6}", "id", "ReplicaId", "targetId", "startLsn", "endLsn", "prepT", "latency"),
                LR_STRUCTURED_TRACE(CopyStreamFinished, 26, Info, "{1}: TargetReplica: {2} Copy Operations Sent: {3}", "id", "ReplicaId", "targetId", "num"),
                LR_STRUCTURED_TRACE(CopyStreamMetadata, 27, Info, "{1}: TargetReplica: {2} CopyStage: {3} SourceStartingLsn: {4} TargetStartingLsn: {5}", "id", "ReplicaId", "targetId", "stage", "sourceS", "targetS"),
                LR_STRUCTURED_TRACE(CopyStreamStateMetadata, 28, Info, "{1}: TargetReplica: {2} CopyStage: {3} SourceStartingLsn: {4} BeginCheckpointLsn: {5} StateRecordsCopied: {6} TailLsn: {7}", "id", "ReplicaId", "targetId", "stage", "sourceS", "BCLsn", "staterecords", "tail"),

                // KLogManager
                LR_STRUCTURED_TRACE(KLMCreateCopyLog, 31, Info, "{1}: {2} Alias: {3} Guid: {4} NTSTATUS: {5:x}", "id", "ReplicaId", "message", "alias", "guid", "status"),
                LR_STRUCTURED_TRACE(KLMDeleteLogFileStart, 32, Info, "{1}: Alias: {2}", "id", "ReplicaId", "alias"),
                LR_STRUCTURED_TRACE(KLMDeleteLogFileFailed, 33, Info, "{1}: Operation: {2} Alias: {3} NTSTATUS: {4:x}", "id", "ReplicaId", "operation", "alias", "status"),
                LR_STRUCTURED_TRACE(KLMError, 34, Error, "{1}: Message: {2} Guid: {3} Alias: {4} NTSTATUS: {5:x}", "id", "ReplicaId", "message", "guid", "alias", "status"),
                LR_STRUCTURED_TRACE(KLMOpenLogicalLog, 35, Info, "{1}: Alias: {2} Guid: {3} NTSTATUS: {4:x}", "id", "ReplicaId", "alias", "guid", "status"),
                LR_STRUCTURED_TRACE(KLMRenameCopyLogError, 36, Error, "{1}: CurrentAlias: {2} BaseAlias: {3} BackupAlias: {4} NTSTATUS: {5:x}", "id", "ReplicaId", "current", "base", "backup", "status"),
                LR_STRUCTURED_TRACE(KLMRenameCopyLog, 37, Info, "{1}: Renamed Copy Log. Head Record Position: {2}. TailRecordType: {3} TailLsn: {4} TailPsn: {5} TailPosition: {6}", "id", "ReplicaId", "headpos", "tailtype", "taillsn", "tailpsn", "tailpos"),
                LR_STRUCTURED_TRACE(KLMLogFileNotFound, 38, Warning, "{1}: CreateLogFileAsync Expected to find the log file, but not found", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(KLMCreateLog, 39, Info, "{1}: Path: {2} StreamId: {3} Alias: {4}", "id", "ReplicaId", "path", "guid", "alias"),
                LR_STRUCTURED_TRACE(KLMMappedDedicatedLogPathForContainer, 40, Info, "{1}: From {2} To: {3}", "id", "ReplicaId", "fromPath", "toPath"),

                // LoggingReplicatorImpl
                LR_STRUCTURED_TRACE(BecomeSecondary, 41, Info, "{1}: IsActiveSecondary: {2}", "id", "ReplicaId", "IsActive"),
                LR_STRUCTURED_TRACE(BecomeNone, 42, Info, "{1}: IsRemovingStateAfterOpen: {2}", "id", "ReplicaId", "IsRemovingStateAfterOpen"),
                LR_STRUCTURED_TRACE(BecomePrimary, 43, Info, "{1}", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(BecomePrimaryAbortTransaction, 44, Info, "{1}: Attempting to abort transaction RecordType: {2} Lsn: {3} Psn: {4} Position: {5} TxId: {6}", "id", "ReplicaId", "recordtype", "lsn", "psn", "position", "txid"),
                LR_STRUCTURED_TRACE(BecomePrimaryAbortTransactionStop, 45, Info, "{1}: Stop aborting transaction RecordType: {2} Lsn: {3} Psn: {4} Position: {5} TxId: {6}", "id", "ReplicaId", "recordtype", "lsn", "psn", "position", "txid"),
                LR_STRUCTURED_TRACE(DeleteLog, 46, Info, "{1}", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(UpdateEpoch, 48, Info, "{1}: {2},{3:x} Lsn: {4} Role: {5} DrainingStream: {6}", "id", "ReplicaId", "dl", "con", "lsn", "role", "stream"),
                LR_STRUCTURED_TRACE(GetCopyContext, 49, Info, "{1}", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(GetCopyState, 50, Info, "{1}: UptoLsn: {2}", "id", "ReplicaId", "lsn"),
                LR_STRUCTURED_TRACE(QuiesceActivity, 51, Info, "{1}: RecordType: {2} Lsn: {3} Psn: {4} Position: {5} Done: {6}", "id", "ReplicaId","recordtype", "lsn", "psn", "pos", "done"),
                LR_STRUCTURED_TRACE(TransitionOutOfPrimaryUnlock, 52, Info, "{1}: RecordType: {2} Lsn: {3} Psn: {4} Position: {5} TxId: {6}", "id", "ReplicaId", "recordtype", "lsn", "psn", "position", "txid"),
                LR_STRUCTURED_TRACE(OnDataLossAsync, 53, Error, "{1}: Message: {2} NTSTATUS: {3}", "id", "ReplicaId", "message", "status"),
                LR_STRUCTURED_TRACE(Open, 54, Info, "{1}: ShouldLocalStateBeRemoved: {2}", "id", "ReplicaId", "ShouldLocalStateBeRemoved"),
                LR_STRUCTURED_TRACE(Recover, 55, Info, "{1}", "id", "ReplicaId"),
                LR_STRUCTURED_TRACE(Close, 56, Info, "{1}: IsRemovingStateAfterOpen: {2}", "id", "ReplicaId", "IsRemovingStateAfterOpen"),
                LR_STRUCTURED_TRACE(ProgressVectorValidationTelemetry, 57, Warning, "{1}: Starting full copy, ProgressVector validation failed. \r\nError Message: {2}\r\nSourceProgressVector: {3}\r\nSourceIndex: {4}.\r\nSourceProgressVectorEntry: {5} \r\nTargetProgressVector: {6}\r\nTargetIndex: {7}.\r\nTargetProgressVectorEntry: {8}", "id", "ReplicaId", "errorMessage", "sourceVector", "sourceIndex", "sourceVectorEntry", "targetVector", "targetIndex", "targetVectorEntry"),
                LR_STRUCTURED_TRACE(FlushedRecordsCallback, 58, Info, "{1}: Status: {2} \r\nFlushed Records:{3}", "id", "ReplicaId", "status", "flushedRecordsInfoVector"),
                LR_STRUCTURED_TRACE(CreateAllManagers, 59, Info, "{1}", "id", "ReplicaId"),

                // LogManager
                LR_STRUCTURED_TRACE(ProcessLogHeadTruncationWaiting, 66, Info, "{1}: Waiting for reader: {2} to complete", "id", "ReplicaId", "reader"),
                LR_STRUCTURED_TRACE(ProcessLogHeadTruncationDone, 67, Info, "{1}: Head Truncation done at Lsn: {2} Psn: {3} Position: {4}. Invoked {5} freelink calls with {6} links older than the new log head Psn", "id", "replicaid", "lsn", "psn", "pos", "call1", "call2"),
                LR_STRUCTURED_TRACE(AddLogReader, 68, Info, "{1}: StartingPosition: {2} EndingPosition: {3} Reader: {4}", "id", "ReplicaId", "startingpos", "endingpos", "reader"),
                LR_STRUCTURED_TRACE(RemoveLogReader, 69, Info, "{1}: Position: {2}", "id", "ReplicaId", "startingpos"),
                LR_STRUCTURED_TRACE(RemoveLogReaderInitiateTruncation, 70, Info, "{1}: Removing Log Reader at Position: {2} triggerred truncation", "id", "ReplicaId", "position"),

                // LogRecordsDispatcher
                LR_STRUCTURED_TRACE(DispatcherConcurrentRecords, 76, Info, "{1}: Pending processing {2} records from Psn: {3} to {4}", "id", "ReplicaId", "count", "start", "end"),
                LR_STRUCTURED_TRACE(DispatcherBarrier, 77, Noise, "{1}: Updating Last Dispatching barrier Lsn: {2} Psn: {3}", "id", "ReplicaId", "lsn", "psn"),
                LR_STRUCTURED_TRACE(DispatcherPauseAtBarrier, 78, Info, "{1}: Lsn: {2} Psn: {3}", "id", "ReplicaId", "lsn", "psn"),
                LR_STRUCTURED_TRACE(DispatcherContinue, 79, Info, "{1}: Lsn: {2} Psn: {3}", "id", "ReplicaId", "lsn", "psn"),
                LR_STRUCTURED_TRACE(DispatcherDrainStart, 80, Info, "{1}: Lsn: {2} Psn: {3}", "id", "ReplicaId", "lsn", "psn"),
                LR_STRUCTURED_TRACE(DispatcherDrainFinish, 81, Info, "{1}: Lsn: {2} Psn: {3} NTSTATUS: {4:x}", "id", "ReplicaId", "lsn", "psn", "status"),

                // LogTruncationManager
                LR_STRUCTURED_TRACE(DrainBlocked, 85, Warning, "{1}: Until Pending OperationType: {2} with Lsn: {3} completes. Bytes from log head: {4} LogUsage: {5}", "id", "ReplicaId", "operationtype", "lsn", "bytes", "usage"),
                LR_STRUCTURED_TRACE(DrainContinue, 86, Warning, "{1}: Bytes from log head: {2} LogUsage: {3}", "id", "ReplicaId", "bytes", "usage"),

                // OperationProcessor
                LR_STRUCTURED_TRACE(OPRecordProcessedImmediately, 91, Info, "{1}: DrainingStream: {2} Psn: {3}", "id", "ReplicaId", "stream", "psn"),
                LR_STRUCTURED_TRACE(OPRecordProcessedImmediatelyInfo, 92, Info, "{1}: DrainingStream: {2} Psn: {3} InformationEvent: {4}", "id", "ReplicaId", "stream", "psn", "info"),
                LR_STRUCTURED_TRACE(OPApplyCallbackTransactionRecord, 93, Noise, "{1}: DrainingStream: {2} RecordType: {3} Lsn: {4} Psn: {5} RecordPosition: {6}, TxId: {7} IsSingleOpTx: {8}", "id", "ReplicaId", "stream", "recordtype", "lsn", "psn", "pos", "txid", "issingleop"),
                LR_STRUCTURED_TRACE(OPWaitForRecordsProcessing, 94, Info, "{1}: Type: {2} OutstandingCount: {3}", "id", "ReplicaId", "waittype", "count"),
                LR_STRUCTURED_TRACE(OPWaitForRecordsProcessingDone, 95, Info, "{1}: Type: {2} RecordType: {3} Lsn: {4} Psn: {5} RecordPosition: {6}", "id", "ReplicaId", "waittype", "recordtype", "lsn", "psn", "pos"),

                // TruncateTailManager
                LR_STRUCTURED_TRACE(TruncateTailSingleOperationTransactionRecord, 101, Info, "{1}: Transaction:{2}. LSN: {3} PSN: {4} Position: {5} Tx: {6}", "id", "replicaid", "operation", "lsn", "psn", "pos", "transactionid"),
                LR_STRUCTURED_TRACE(TruncateTailTransactionRecord, 102, Info, "{1}: Operation:{2}. LSN: {3} PSN: {4} Position: {5} Tx: {6}", "id", "replicaid", "operation", "lsn", "psn", "pos", "transactionid"),
                LR_STRUCTURED_TRACE(TruncateTailAtomicOperation, 103, Info, "{1}: Atomic Operation Undone. LSN: {2} PSN: {3} Position: {4} Tx: {5}", "id", "replicaid", "lsn", "psn", "pos", "transactionid"),
                LR_STRUCTURED_TRACE(TruncateTailOperationRecord, 104, Info, "{1}: Operation:{2}. LSN: {3} PSN: {4} Position: {5} Tx: {6}", "id", "replicaid", "operation", "lsn", "psn", "pos", "transactionid"),
                LR_STRUCTURED_TRACE(TruncateTailBarrier, 105, Info, "{1}: Barrier Deleted. LSN: {2} PSN: {3} Position: {4}", "id", "replicaid", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(TruncateTailBackup, 106, Info, "{1}: Backup Deleted. LSN: {2} PSN: {3} Position: {4}", "id", "replicaid", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(TruncateTailUpdateEpoch, 107, Info, "{1}: UpdateEpoch Deleted. Epoch:({2},{3:x}) LSN: {4} PSN: {5} Position: {6}", "id", "replicaid", "epoch.datalossversion", "epoch.configurationversion", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(TruncateTailDone, 108, Info, "{1}: Tail truncated to record Type: {2} LSN: {3} PSN: {4} Position: {5}", "id", "replicaid", "recordtype", "lsn", "psn", "pos"),

                // PhysicalLogWriter
                LR_STRUCTURED_TRACE(FlushAsync, 111, Noise, "{1}: {2} initiated flush. Waited RecordType: {3} LSN: {4} PSN: {5} Position: {6}", "id", "replicaid", "initiator", "waitrecordttype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(FlushStart, 112, Noise, "{1}: Flush Start for {2} records from PSN: {3} at write position {4}", "id", "replicaid", "flushingrecords", "psn", "pos"),
                LR_STRUCTURED_TRACE(FlushEnd, 113, Noise, "{1}: Flush Ended. Bytes: {2} LSR: {3} FlushTime(ms): {4} SerializationTime(ms): {5} Avg. Byte/sec: {6} Avg. Latency Milliseconds: {7}. WritePosition: {8}", "id", "replicaid", "numberofbytes", "latencysensitiverecords", "flushms", "serializationms", "avg bytes/sec", "avg latency ms", "pos"),
                LR_STRUCTURED_TRACE(FlushEndWarning, 114, Info, "{1}: Flush Ended. Bytes: {2} LSR: {3} FlushTime(ms): {4} SerializationTime(ms): {5} Avg. Byte/sec: {6} Avg. Latency Milliseconds: {7}. WritePosition: {8}", "id","replicaid", "numberofbytes", "latencysensitiverecords", "flushms", "serializationms", "avg bytes/sec", "avg latency ms", "pos"),
                LR_STRUCTURED_TRACE(PendingFlushWarning, 115, Warning, "{1}: Pending Flush Size {2} greater than MaxWriteCacheSize {3}. Throttling writes", "id", "replicaid", "pendingflushbytes", "maxwritecachesizeinbytes"),
                LR_STRUCTURED_TRACE(FlushInvoke, 116, Noise, "{1}:{2} invoking flush", "id", "replicaid", "initiator"),

                // FileLogManager
                LR_STRUCTURED_TRACE(FileLogManagerDeleteLogFailed, 121, Warning, "{1}: CreateCopyLog: Delete logical log failed with EC: {2} for file {3}", "id", "replicaid", "errorcode", "filename"),
                LR_STRUCTURED_TRACE(FileLogManagerDeleteFile, 122, Info, "{1}: DeleteLog: Try delete log file {2}", "id", "replicaid", "filename"),
                LR_STRUCTURED_TRACE(FileLogManagerDeleteFileFailed, 123, Warning, "{1}: DeleteLogFileAsync: Delete logical log failed with exception for file {2}. EC: {3}", "id", "replicaid", "filename", "errorcode"),
                LR_STRUCTURED_TRACE(FileLogManagerRenameCopyLogAtomically, 124, Info, "{1}: Renamed log. Head record Position {2}. Tail Record Type: {3} LSN: {4} PSN: {5} Position:{6}", "id", "replicaid", "headrecordpos", "tailrecordtype", "lsn", "psn", "tailerecordpos"),
                LR_STRUCTURED_TRACE(FileLogManagerRenameCopyLogAtomicallyException, 125, Error, "{1}: Status: {2:x} \r\n {3}", "id", "replicaid", "errorcode", "stacktrace"),

                // RecoveryManager
                LR_STRUCTURED_TRACE(RMRecoveryFailed, 129, Info, "{1}: ApplyError: {2:x} LogError: {3:x} Awaiting LastRecoverableRecord Type: {4} Lsn:{5} Psn:{6}", "id", "replicaid", "applyerror", "logerror", "recordType", "lsn", "psn"),
                LR_STRUCTURED_TRACE(RMOpenAsyncLogUsage, 130, Info, "{1}: LogUsage: {2} tail record type: {3} lsn: {4} psn: {5} position: {6}", "id", "replicaid", "logusage", "recordtype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncLastPhysicalRecord, 131, Info, "{1}: Recovered Last Physical Record type: {2} lsn: {3} psn: {4} position: {5}", "id", "replicaid", "recordtype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncSkippingRecovery, 132, Info, "{1}: Skipping recovery due to pending remove state", "id", "replicaid"),
                LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLinkedPhysicalRecord, 133, Info, "{1}: RecoveredLinkedPhysicalRecord Type: {2} Lsn: {3} Psn: {4} Position: {5}", "id", "replicaid", "recordtype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredTruncateHeadRecord, 134, Info, "{1}: Recovered last Truncate Head record. Head Lsn: {2} Head Position: {3}", "id", "replicaid", "headlsn", "headpos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLastCCRecord, 135, Info, "{1}: Recovered last CompleteCheckpoint record. Head Lsn: {2} Head Position: {3}", "id", "replicaid", "headlsn", "headpos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredCCRecordInfo, 136, Info, "{1}: CompleteCheckpoint recovered Lsn: {2} Psn: {3} Position: {4}. Head Lsn: {5} Head Position: {6}", "id", "replicaid", "lsn", "psn", "pos", "headlsn", "headpos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncRecoveredLastECRecord, 137, Info, "{1}: Recovered last end checkpoint record Lsn: {2} Psn: {3} Position: {4}. Head Lsn: {5} Head Position: {6}", "id", "replicaid", "lsn", "psn", "pos", "headlsn", "headpos"),
                LR_STRUCTURED_TRACE(RMOpenAsyncCCRecordNotFound, 138, Info, "{1}: CompleteCheckpoint record not found", "id", "replicaid"),
                LR_STRUCTURED_TRACE(RMPerformRecoveryAsync, 139, Info, "{1}: Recovered Completed Begin Checkpoint Lsn: {2} Psn: {3} Position: {4} \r\n Recovery starting position: {5} Epoch: ({6}:{7:x}) LogCompleteCheckpoint: {8} \r\n Last Completed Begin Checkpoint ProgressVector: {9}","id", "replicaid", "lsn", "psn", "pos", "recoverystartingpos", "datalossversion", "configversion", "islogcomplete", "progressvectorstring"),
                LR_STRUCTURED_TRACE(RMPerformRecoveryAsyncTailRecordType, 140, Info, "{1}: Current Tail Record Type: {2} Lsn: {3} Psn: {4} Position: {5}", "id", "replicaid", "recordttype", "lsn", "psn", "pos"),

                // TransactionManager
                LR_STRUCTURED_TRACE(AcceptSingleOperationTransaction, 141, Info, "{1}: TxId: {2} Lsn: {3}", "id", "replicaid", "transactionid", "lsn"),
                LR_STRUCTURED_TRACE(AcceptBeginTransaction, 142, Info, "{1}: TxId: {2} Lsn: {3}", "id", "replicaid", "transactionid", "lsn"),
                LR_STRUCTURED_TRACE(AcceptAtomicOperation, 143, Info, "{1}: TxId: {2} Lsn: {3} IsRedoOnly: {4}", "id", "replicaid", "transactionid", "lsn", "isredoonly"),
                LR_STRUCTURED_TRACE(AcceptOperation, 144, Info, "{1}: TxId: {2} Lsn: {3}", "id", "replicaid", "transactionid", "lsn"),
                LR_STRUCTURED_TRACE(AcceptEndTransaction, 145, Info, "{1}: TxId: {2} Lsn: {3} IsCommitted: {4}", "id", "replicaid", "transactionid", "lsn", "iscommitted"),

                // ReplicatedLogManager
                LR_STRUCTURED_TRACE(UpdateEpochRecord, 151, Info, "{1}: Epoch ({2},{3:x}) Lsn: {4} Role: {5}", "id", "replicaid", "datalossversion", "configurationversion", "lsn", "replicarole"),
                LR_STRUCTURED_TRACE(AcceptBarrier, 152, Info, "{1}: Lsn: {2}", "id", "replicaid", "lsn"),

                // SecondaryDrainManager
                LR_STRUCTURED_TRACE(EnableSecondaryCommitApplyAcknowledgement, 161, Info, "{1}: EnableSecondaryCommitApplyAcknowledgement is enabled. This is an internal unsupported config. It should only used with extreme care.", "id", "replicaid"),
                LR_STRUCTURED_TRACE(CopiedUpdateEpoch, 162, Info, "{1}: UpdateEpoch: ({2},{3:x}) LSN: {4} ReplicaRole: {5}", "id", "replicaid", "datalossversion", "configurationversion", "lsn", "replicarole"),
                LR_STRUCTURED_TRACE(CopyOrBuildReplicaStatus, 163, Info, "{1}: Status : {2}, Primary Replica Id: {3}", "id", "replicaid", "message", "primaryreplicaid"),
                LR_STRUCTURED_TRACE(CopyOrBuildReplicaAborted, 164, Info, "{1}:Returning null as copy pump has been aborted", "id", "replicaid"),
                LR_STRUCTURED_TRACE(CopyOrBuildReplicaStarted, 165, Info, "{1}:Copy started. Starting LSN {2} StartingEpoch: ({3},{4:x}) CheckpoinLSN: {5} UptoLSN: {6} Highest Checkpoint LSN: {7} \r\n Copied ProgressVector: {8}", "id", "replicaid", "startinglsn", "startingdatalossversion", "startingconfigurationversion", "checkpointlsn", "uptolsn", "higheststateproviderlsn", "progressvector"),
                LR_STRUCTURED_TRACE(UpdateEpochRecordDueToFullCopy, 166, Info, "{1}:Inserting UpdateEpochLogRecord - Epoch:({2},{3:x}) LSN: {4} ReplicaRole: {5}", "id", "replicaid", "datalossversion", "configurationversion", "lsn", "replicarole"),
                LR_STRUCTURED_TRACE(CopyAckedProgress, 167, Info, "{1}:CopyOrBuildReplica acknowledged ProgressVectorEntry operation", "id", "replicaid"),
                LR_STRUCTURED_TRACE(DrainCopyReceive, 169, Noise, "{1}: Copy Operation: {2}. LSN {3} to {4}. AcksRemaining: {5}", "id", "replicaid", "copiedoperationnumber", "copystartlsn", "copyendlsn", "acksremaining"),
                LR_STRUCTURED_TRACE(DrainCopyStreamCompleted, 170, Info, "{1}: DrainCopyStream completed. Number of operations: {2}. Last record Type: {3} Last LSN: {4} PSN: {5} Position: {6}", "id", "replicaid", "numberofcopiedoperations", "recordtype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(DrainReplicationReceive, 171, Noise, "{1}: DrainReplicationStream Received Replication record: {2} RecordType: {3} LSN: {4} AcksRemaining: {5}", "id", "replicaid", "numberofreplicatedrecords", "recordtype", "lsn", "acksremaining"),
                LR_STRUCTURED_TRACE(AwaitFlushingReplicatedRecordTask, 172, Noise, "{1}: AckedLSN: {2} AcksPending: {3} BytesPending: {4}", "id", "replicaid", "ackedlsn", "ackspending", "bytespending"),
                LR_STRUCTURED_TRACE(DrainStart, 173, Info, "{1}: {2}", "id", "replicaid", "streamname"),
                LR_STRUCTURED_TRACE(DrainStateNoise, 174, Info, "{1}: {2} state record {3}", "id", "replicaid", "state", "staterecordnumber"),
                LR_STRUCTURED_TRACE(DrainCompleted, 175, Info, "{1}: Drain {2} completed. Number of {3}: {4}. Status: {5}. Last Record Type: {6} Last LSN: {7} PSN: {8} Position: {9}", "id", "replicaid", "streamname", "drainunits", "numberofunits", "drainstatus", "recordtype", "lsn", "psn", "pos"),
                LR_STRUCTURED_TRACE(ThrottlingWrites, 176, Warning, "{1}: {2}", "id", "replicaid", "message"),
                LR_STRUCTURED_TRACE(TruncateTail, 177, Warning, "{1}: Tail is being truncated to {2}", "id", "replicaid", "taillsn"),

                // BackupManager
                LR_STRUCTURED_TRACE(
                    IBM_BackupAsyncStart, 186, Info,
                    "{1}: IBM.BackupAsync: BackupId: {2} ParentBackupId: {3} Option: {4} EnableIBAR: {5} Epoch: ({6},{7}) LSN: {8} \r\n Folder: {9} \r\n Backup Steps: \r\n 1. Argument validation and prepare backup \r\n 2. StateManager backup \r\n 3. Get log records and backup \r\n 4. Write backup metadata \r\n 5. Ensure backed up data is stabilized \r\n 6. Call the user callback \r\n 7. Replicate backup log record",
                    "id", "replicaid", "backupid", "parentbackupid", "option", "ibar", "configurationversion", "datalossversion", "highestbackeduplsn", "folder"),
                LR_STRUCTURED_TRACE(
                    IBM_BackupAsync, 187, Info, 
                    "{1}: IBM.BackupAsync: BackupId: {2} ParentBackupId: {3} Epoch: ({4},{5}) LSN: {6} \r\n Step Duration: {7}ms, {8}ms, {9}ms, {10}ms, {11}ms, {12}ms, {13}ms", 
                    "id", "replicaid", "backupid", "parentbackupid", "configurationversion", "datalossversion", "highestbackeduplsn", "t1", "t2", "t3", "t4", "t5", "t6", "t7"),
                LR_STRUCTURED_TRACE(
                    IBM_BackupAsyncFinish, 188, Info, 
                    "{1}: IBM.BackupAsync: BackupId: {2} ParentBackupId: {3} Epoch: ({4},{5}) LSN: {6} \r\n Step Duration: {7}ms, {8}ms, {9}ms, {10}ms, {11}ms, {12}ms, {13}ms",
                    "id", "replicaid", "backupid", "parentbackupid", "configurationversion", "datalossversion", "highestbackeduplsn", "t1", "t2", "t3", "t4", "t5", "t6", "t7"),
                LR_STRUCTURED_TRACE(
                    IBM_BackupAsyncFinishError, 189, Warning, 
                    "{1}: BackupId: {2}. Failed with {3} {4}. {5}", 
                    "id", "replicaid", "backupid", "errortype", "error", "message"),
                LR_STRUCTURED_TRACE(
                    IBM_RestoreAsyncStart, 190, Info, 
                    "{1}: RestoreId: {2} Epoch: ({3},{4}) LSN: {5} Number of Backups: {6} \r\n Folder: {7} \r\n Restore Steps: \r\n 1. Argument validation and prepare for restore \r\n 2. StateManager restore \r\n 3. Replicator restore \r\n 4. Reset the replica role and primary state", 
                    "id", "replicaid", "restoreid", "configurationversion", "datalossversion", "highestbackeduplsn", "numofbackups", "directory"),
                LR_STRUCTURED_TRACE(
                    IBM_RestoreAsync, 191, Info,
                    "{1}: RestoreId: {2} \r\n Step Duration: {3}ms, {4}ms, {5}ms, {6}ms",
                    "id", "replicaid", "restoreid", "t1", "t2", "t3", "t4"),
                LR_STRUCTURED_TRACE(
                    IBM_RestoreAsyncFinish, 192, Info, 
                    "{1}: RestoreId: {2} \r\n Step Duration: {3}ms, {4}ms, {5}ms, {6}ms",
                    "id", "replicaid", "restoreid", "t1", "t2", "t3", "t4"),
                LR_STRUCTURED_TRACE(
                    IBM_RestoreAsyncFinishError, 193, Warning, 
                    "{1}: RestoreId: {2} Status: {3} Message: {4}", 
                    "id", "replicaid", "restoreid", "status", "message"),
                LR_STRUCTURED_TRACE(
                    RestoreTokenCreated, 194, Info, 
                    "{1}: RestoreId: {2}. Restore Token created {3}", 
                    "id", "replicaid", "restoreid", "restoretokenpath"), 
                LR_STRUCTURED_TRACE(
                    RestoreTokenDeleted, 195, Info, 
                    "{1}: RestoreId: {2}. Restore Token deleted {3}", 
                    "id", "replicaid", "restoreid", "restoretokenpath"),
                LR_STRUCTURED_TRACE(
                    RestoreRecord, 196, Noise,
                    "{1}: Inserted record from backup. Type: {2} LSN: {3} PSN: {4} Previous PSN: {5}",
                    "id", "replicaId", "recordType", "lsn", "psn", "previousPSN"),

                LR_STRUCTURED_TRACE(BackupRestoreWarning, 197, Warning,
                    "{1}: Backup Manager: IsBackup: {2} Id: {3} Message: {4}",
                    "id", "replicaid", "isBackup", "backupOrRestoreId", "message"),

                LR_STRUCTURED_TRACE(UndoBackupLogRecord, 198, Info,
                    "{1}: Backup Manager: UndoLastCompletedBackupLogRecord: LSN: {2} PSN: {3} RecordPosition: {4} HighestBackedupEpoch: ({6},{5}) HighestBackedupLsn: {7}.",
                    "id", "replicaid", "lsn", "psn", "position", "datalossversion", "configurationversion", "highestbackeduplsn"),

                // BackupFolderInfo
                LR_STRUCTURED_TRACE(BackupFolderWarning, 206, Error, "{1}: {2} Folder: {3}. Status: {4:x}", "id", "replicaid", "message", "folder", "status"),

                // PhysicalLogWriterCallbackManager
                LR_STRUCTURED_TRACE(NullCallbackProcessor, 216, Info, "{1}: PhysicalLogWriterCallbackManager Could not process log records from Psn: {2}-{3} as the flush callback processor is null", "id", "replicaid", "firstrecordpsn", "lastrecordpsn"),

                // FlushedRecordInfo
                LR_STRUCTURED_TRACE(FlushedRecordInfo, 217, Info, "\r\nType:{1} Lsn:{2} Psn:{3} Pos:{4} ", "contextSequenceId", "recordType","lsn", "psn", "position")
            {
            }

            static Common::Global<EventSource> Events;
        }; 
    } 
}
