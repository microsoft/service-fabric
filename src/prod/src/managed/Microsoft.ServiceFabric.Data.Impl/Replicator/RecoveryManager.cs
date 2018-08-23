// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RecoveryManager
    {
        private ITracer tracer;
        private Exception recoveryException;
        private LogRecord tailRecordAtStart;
        private bool logCompleteCheckpointAfterRecovery;
        private readonly Action<LoggedRecords> flushedRecordsCallback;

        public RecoveryManager(Action<LoggedRecords> flushedRecordsCallback)
        {
            this.flushedRecordsCallback = flushedRecordsCallback;
            this.Reuse();
        }

        public Exception RecoveryException
        {
            get; private set;
        }

        public LogicalSequenceNumber RecoveredLsn
        {
            get; private set;
        }

        public bool IsRemovingStateAfterOpen
        {
            get; private set;
        }

        public long LastRecoveredAtomicRedoOperationLsn
        {
            get; private set;
        }

        public EndCheckpointLogRecord RecoveredLastEndCheckpointRecord
        {
            get; private set;
        }

        public PhysicalLogReader RecoveryLogsReader
        {
            get; private set;
        }

        public async Task<RecoveryInformation> OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings replicatorSettings,
            ITracer tracer,
            LogManager logManager,
            bool shouldLocalStateBeRemoved,
            PhysicalLogReader recoveryReader,
            bool isRestoring)
        {
            this.RecoveryLogsReader = recoveryReader;
            this.tracer = tracer;

            PhysicalLogRecord recoveredLinkedPhysicalRecord;
            TruncateHeadLogRecord recoveredLastTruncateHeadLogRecord = null;
            CompleteCheckpointLogRecord recoveredCompleteCheckpointRecord = null;
            this.logCompleteCheckpointAfterRecovery = true;

            this.tailRecordAtStart = await this.RecoveryLogsReader.SeekToLastRecord().ConfigureAwait(false);

            var logUsage = logManager.Length;
            var trace = "OpenAsync: Log Usage: " + logUsage + " Recovered Replica Tail Record Type: "
                    + tailRecordAtStart.RecordType + " LSN: " + tailRecordAtStart.Lsn.LSN + " PSN: "
                    + tailRecordAtStart.Psn.PSN + " Position: " + tailRecordAtStart.RecordPosition;

            FabricEvents.Events.Api(tracer.Type, trace);

            var lastPhysicalRecord = tailRecordAtStart as PhysicalLogRecord;
            if (lastPhysicalRecord == null)
            {
                lastPhysicalRecord = await this.RecoveryLogsReader.GetPreviousPhysicalRecord(tailRecordAtStart).ConfigureAwait(false);
            }

            trace = "OpenAsync: Recovered last physical record Type: " + lastPhysicalRecord.RecordType + " LSN: "
                    + lastPhysicalRecord.Lsn.LSN + " PSN: " + lastPhysicalRecord.Psn.PSN + " Position: "
                    + lastPhysicalRecord.RecordPosition;

            FabricEvents.Events.Api(tracer.Type, trace);

            if (lastPhysicalRecord.RecordType == LogRecordType.Information)
            {
                var removingStateRecord = lastPhysicalRecord as InformationLogRecord;
                if (removingStateRecord.InformationEvent == InformationEvent.RemovingState)
                {
                    Utility.Assert(
                        lastPhysicalRecord == tailRecordAtStart,
                        "Last Physical Record {0} should be same as tail record at start {1}",
                        lastPhysicalRecord, tailRecordAtStart);

                    FabricEvents.Events.Api(tracer.Type, "OpenAsync: Skipping Recovery due to pending RemovingState");

                    this.IsRemovingStateAfterOpen = true;

                    return new RecoveryInformation(shouldLocalStateBeRemoved);
                }
            }

            if (lastPhysicalRecord.RecordType == LogRecordType.TruncateHead)
            {
                recoveredLastTruncateHeadLogRecord = lastPhysicalRecord as TruncateHeadLogRecord;
            }
            else if (lastPhysicalRecord.RecordType == LogRecordType.EndCheckpoint)
            {
                this.RecoveredLastEndCheckpointRecord = lastPhysicalRecord as EndCheckpointLogRecord;
            }
            else if (lastPhysicalRecord.RecordType == LogRecordType.CompleteCheckpoint)
            {
                recoveredCompleteCheckpointRecord = lastPhysicalRecord as CompleteCheckpointLogRecord;
            }
            else
            {
                recoveredLinkedPhysicalRecord = await this.RecoveryLogsReader.GetLinkedPhysicalRecord(lastPhysicalRecord).ConfigureAwait(false);
                Utility.Assert(
                    (recoveredLinkedPhysicalRecord != null) &&
                    ((recoveredLinkedPhysicalRecord.RecordType == LogRecordType.TruncateHead)
                    || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.EndCheckpoint)
                    || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.CompleteCheckpoint)),
                    "Record type should be truncate head or end checkpoint or complete checkpoint. Record type is : {0}",
                    recoveredLinkedPhysicalRecord.RecordType);

                trace = "OpenAsync: RecoveredLinkedPhysicalRecord: " + recoveredLinkedPhysicalRecord.RecordType
                        + " LSN: " + recoveredLinkedPhysicalRecord.Lsn.LSN + " PSN: "
                        + recoveredLinkedPhysicalRecord.Psn.PSN + " Position: "
                        + recoveredLinkedPhysicalRecord.RecordPosition;

                FabricEvents.Events.Api(tracer.Type, trace);

                if (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.TruncateHead)
                {
                    recoveredLastTruncateHeadLogRecord = recoveredLinkedPhysicalRecord as TruncateHeadLogRecord;
                }
                else if (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.EndCheckpoint)
                {
                    this.RecoveredLastEndCheckpointRecord = recoveredLinkedPhysicalRecord as EndCheckpointLogRecord;
                }
                else
                {
                    recoveredCompleteCheckpointRecord = recoveredLinkedPhysicalRecord as CompleteCheckpointLogRecord;
                }
            }

            ulong logHeadPosition = 0;
            long logHeadLsn = 0;

            if (recoveredLastTruncateHeadLogRecord != null)
            {
                logHeadPosition = recoveredLastTruncateHeadLogRecord.LogHeadRecordPosition;
                logHeadLsn = recoveredLastTruncateHeadLogRecord.LogHeadLsn.LSN;
                trace = "OpenAsync: Recovered last truncate head record Type: "
                        + recoveredLastTruncateHeadLogRecord.RecordType + " LSN: "
                        + recoveredLastTruncateHeadLogRecord.Lsn.LSN + " PSN: "
                        + recoveredLastTruncateHeadLogRecord.Psn.PSN + " Position: "
                        + recoveredLastTruncateHeadLogRecord.RecordPosition + " LogHeadLSN: "
                        + recoveredLastTruncateHeadLogRecord.LogHeadLsn.LSN + " LogHeadPosition: "
                        + recoveredLastTruncateHeadLogRecord.LogHeadRecordPosition;

                FabricEvents.Events.Api(tracer.Type, trace);

                recoveredLinkedPhysicalRecord = recoveredLastTruncateHeadLogRecord;
                do
                {
                    recoveredLinkedPhysicalRecord = await this.RecoveryLogsReader.GetLinkedPhysicalRecord(recoveredLinkedPhysicalRecord).ConfigureAwait(false);
                    Utility.Assert(
                        (recoveredLinkedPhysicalRecord != null) &&
                        ((recoveredLinkedPhysicalRecord.RecordType == LogRecordType.TruncateHead)
                        || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.EndCheckpoint)
                        || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.CompleteCheckpoint)),
                        "Record type should be truncate head or end checkpoint or complete checkpoint. Record type is : {0}",
                        recoveredLinkedPhysicalRecord.RecordType);

                    this.RecoveredLastEndCheckpointRecord = recoveredLinkedPhysicalRecord as EndCheckpointLogRecord;

                    if (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.CompleteCheckpoint)
                    {
                        recoveredCompleteCheckpointRecord = recoveredLinkedPhysicalRecord as CompleteCheckpointLogRecord;
                    }
                } while (this.RecoveredLastEndCheckpointRecord == null);
            }
            else if (recoveredCompleteCheckpointRecord != null)
            {
                logHeadPosition = recoveredCompleteCheckpointRecord.LogHeadRecordPosition;
                logHeadLsn = recoveredCompleteCheckpointRecord.LogHeadLsn.LSN;

                trace = "OpenAsync: Recovered last complete checkpoint record Type: "
                        + recoveredCompleteCheckpointRecord.RecordType + " LSN: "
                        + recoveredCompleteCheckpointRecord.Lsn.LSN + " PSN: "
                        + recoveredCompleteCheckpointRecord.Psn.PSN + " Position: "
                        + recoveredCompleteCheckpointRecord.RecordPosition + " LogHeadLsn: "
                        + recoveredCompleteCheckpointRecord.LogHeadLsn.LSN + " LogHeadPosition: "
                        + recoveredCompleteCheckpointRecord.LogHeadRecordPosition;

                FabricEvents.Events.Api(tracer.Type, trace);
                recoveredLinkedPhysicalRecord = recoveredCompleteCheckpointRecord;

                do
                {
                    recoveredLinkedPhysicalRecord = await this.RecoveryLogsReader.GetLinkedPhysicalRecord(recoveredLinkedPhysicalRecord).ConfigureAwait(false);

                    Utility.Assert(
                        (recoveredLinkedPhysicalRecord != null) &&
                        ((recoveredLinkedPhysicalRecord.RecordType == LogRecordType.TruncateHead)
                        || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.EndCheckpoint)
                        || (recoveredLinkedPhysicalRecord.RecordType == LogRecordType.CompleteCheckpoint)),
                        "Record type should be truncate head or end checkpoint or complete checkpoint. Record type is : {0}",
                        recoveredLinkedPhysicalRecord.RecordType);

                    this.RecoveredLastEndCheckpointRecord = recoveredLinkedPhysicalRecord as EndCheckpointLogRecord;
                } while (this.RecoveredLastEndCheckpointRecord == null);
            }
            else
            {
                logHeadPosition = this.RecoveredLastEndCheckpointRecord.LogHeadRecordPosition;
                logHeadLsn = this.RecoveredLastEndCheckpointRecord.LogHeadLsn.LSN;
            }

            trace = "OpenAsync: Recovered last end checkpoint record Type: "
                    + this.RecoveredLastEndCheckpointRecord.RecordType + " LSN: " + this.RecoveredLastEndCheckpointRecord.Lsn.LSN
                    + " PSN: " + this.RecoveredLastEndCheckpointRecord.Psn.PSN + " Position: "
                    + this.RecoveredLastEndCheckpointRecord.RecordPosition + " LogHeadLSN: "
                    + this.RecoveredLastEndCheckpointRecord.LogHeadLsn.LSN + " LogHeadPosition: "
                    + this.RecoveredLastEndCheckpointRecord.LogHeadRecordPosition;

            FabricEvents.Events.Api(tracer.Type, trace);
            if (recoveredCompleteCheckpointRecord != null)
            {
                this.logCompleteCheckpointAfterRecovery = false;

                // This is fine since both the begin and end records guarantee all the SP's have been renamed
                trace = "OpenAsync: " + " LogHeadPosition: " + this.RecoveredLastEndCheckpointRecord.LogHeadRecordPosition
                        + Environment.NewLine + " LogHeadLSN: " + this.RecoveredLastEndCheckpointRecord.LogHeadLsn.LSN
                        + Environment.NewLine + "           CompleteCheckpoint record found with LSN: "
                        + recoveredCompleteCheckpointRecord.Lsn.LSN + " PSN: "
                        + recoveredCompleteCheckpointRecord.Psn.PSN + " Position: "
                        + recoveredCompleteCheckpointRecord.RecordPosition;

                FabricEvents.Events.Api(tracer.Type, trace);
            }
            else
            {
                this.logCompleteCheckpointAfterRecovery = true;

                trace = "OpenAsync: " + " LogHeadPosition: " + this.RecoveredLastEndCheckpointRecord.LogHeadRecordPosition
                        + Environment.NewLine + " LogHeadLSN: " + this.RecoveredLastEndCheckpointRecord.LogHeadLsn.LSN
                        + Environment.NewLine + "           CompleteCheckpoint record missing";

                FabricEvents.Events.Api(tracer.Type, trace);
            }

            // For restore cases, logHeadPosition must be 0.
            if (isRestoring == true)
            {
                Utility.Assert(
                    logHeadPosition == 0,
                    "Full backup: LogHead Position ({0}) must be 0",
                    logHeadPosition);
            }

            this.RecoveryLogsReader.MoveStartingRecordPosition(
                logHeadLsn,
                logHeadPosition,
                "recovery",
                LogManager.LogReaderType.Recovery);

            logManager.SetLogHeadRecordPosition(logHeadPosition);

            return new RecoveryInformation(
                this.logCompleteCheckpointAfterRecovery,
                shouldLocalStateBeRemoved);
        }

        public async Task PerformRecoveryAsync(
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn,
            OperationProcessor recordsProcessor,
            CheckpointManager checkpointManager,
            TransactionManager transactionManager,
            LogRecordsDispatcher logRecordsDispatcher,
            ReplicatedLogManager replicatedLogManager)
        {
            var lastPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;
            LogicalSequenceNumber lsn;
            var recoveredRecords = new List<LogRecord>();

            var recoveredLastCompletedBeginCheckpointRecord = await this.RecoveryLogsReader.GetLastCompletedBeginCheckpointRecord(this.RecoveredLastEndCheckpointRecord).ConfigureAwait(false);
            var recoveredLastCompletedBeginCheckpointRecordPosition = recoveredLastCompletedBeginCheckpointRecord.RecordPosition;
            var recoveryStartingPosition = recoveredLastCompletedBeginCheckpointRecordPosition - recoveredLastCompletedBeginCheckpointRecord.EarliestPendingTransactionOffset;

            // Set the recovered lsn before performing recovery as it impacts unlock callback logic during recovery
            recoveredOrCopiedCheckpointLsn.Update(recoveredLastCompletedBeginCheckpointRecord.Lsn);

            var trace =
                string.Format(
                    CultureInfo.InvariantCulture,
                    "PerformRecoveryAsync: " + Environment.NewLine
                    + "      Recovered last completed begin checkpoint record." + Environment.NewLine
                    + "      Type: {0} LSN: {1} PSN:{2} Position: {3}" + Environment.NewLine
                    + "      Recovery Starting Position: {4} Recovery Starting Epoch: {5},{6}" + Environment.NewLine
                    + "      LogCompleteCheckpointAfterRecovery: {7}" + "      Recovered ProgressVector: {8}"
                    + Environment.NewLine,
                    recoveredLastCompletedBeginCheckpointRecord.RecordType,
                    recoveredLastCompletedBeginCheckpointRecord.Lsn.LSN,
                    recoveredLastCompletedBeginCheckpointRecord.Psn.PSN,
                    recoveredLastCompletedBeginCheckpointRecord.RecordPosition,
                    recoveryStartingPosition,
                    recoveredLastCompletedBeginCheckpointRecord.Epoch.DataLossNumber,
                    recoveredLastCompletedBeginCheckpointRecord.Epoch.ConfigurationNumber,
                    this.logCompleteCheckpointAfterRecovery,
                    recoveredLastCompletedBeginCheckpointRecord.ProgressVector.ToString(Constants.ProgressVectorMaxStringSizeInKb));

            FabricEvents.Events.Api(tracer.Type, trace);

            LogRecord lastRecoverableRecord = null;

            replicatedLogManager.LogManager.SetSequentialAccessReadSize(
                this.RecoveryLogsReader.ReadStream,
                LogManager.ReadAheadBytesForSequentialStream);

            // Updated to the recovered value in LogRecordsMap during successful recovery
            long lastPeriodicTruncationTimeTicks = 0;

            using (var records = new LogRecords(this.RecoveryLogsReader.ReadStream, recoveryStartingPosition, this.tailRecordAtStart.RecordPosition))
            {
                var hasMoved = await records.MoveNextAsync(CancellationToken.None).ConfigureAwait(false);
                Utility.Assert(hasMoved == true, "hasMoved == true");

                lsn = records.Current.Lsn;
                if (recoveredLastCompletedBeginCheckpointRecord.EarliestPendingTransactionOffset != 0)
                {
                    lsn -= 1;
                }

                var logRecordsMap = new LogRecordsMap(
                    lsn,
                    transactionManager.TransactionsMap,
                    recoveredLastCompletedBeginCheckpointRecord.Epoch,
                    checkpointManager.LastStableLsn,
                    recoveredLastCompletedBeginCheckpointRecord.ProgressVector,
                    recoveredLastCompletedBeginCheckpointRecord,
                    this.tracer,
                    this.RecoveredLastEndCheckpointRecord);

                do
                {
                    var isRecoverableRecord = true;
                    var record = records.Current;
                    record.CompletedFlush(null);

                    Utility.Assert(
                        record.PreviousPhysicalRecord == logRecordsMap.LastPhysicalRecord,
                        "record.PreviousPhysicalRecord == lastPhysicalRecord");

                    logRecordsMap.ProcessLogRecord(record, out isRecoverableRecord);

                    if (isRecoverableRecord == true)
                    {
                        recoveredRecords.Add(record);
                        lastRecoverableRecord = record;

                        if (LoggingReplicator.IsBarrierRecord(record) &&
                            recoveredRecords.Count >= ReplicatorDynamicConstants.ParallelRecoveryBatchSize)
                        {
                            logRecordsDispatcher.DispatchLoggedRecords(new LoggedRecords(recoveredRecords, null));
                            recoveredRecords = new List<LogRecord>();
                            this.recoveryException = recordsProcessor.ServiceException;
                            
                            if (this.recoveryException == null)
                            {
                                this.recoveryException = recordsProcessor.LogException;
                            }

                            if (this.recoveryException != null)
                            {
                                // If there was an apply or unlock failure, report fault does not help during recovery because the replica is not opened yet.
                                // The only solution here is to throw during OpenAsync
                                
                                FabricEvents.Events.Api(
                                    tracer.Type,
                                    "PerformRecoveryAsync: RecoveryFailed");

                                await lastRecoverableRecord.AwaitProcessing().ConfigureAwait(false);
                                await recordsProcessor.WaitForAllRecordsProcessingAsync().ConfigureAwait(false);

                                throw this.recoveryException;
                            }
                        }
                    }
                    else
                    {
                        Utility.Assert(LoggingReplicator.IsBarrierRecord(record) == false, "IsBarrierRecord(record) == false");
                        record.CompletedApply(null);
                        record.CompletedProcessing(null);
                    }

                    hasMoved = await records.MoveNextAsync(CancellationToken.None).ConfigureAwait(false);
                } while (hasMoved == true);

                this.LastRecoveredAtomicRedoOperationLsn = logRecordsMap.LastRecoveredAtomicRedoOperationLsn;
                checkpointManager.ResetStableLsn(logRecordsMap.LastStableLsn);
                lastPhysicalRecord = logRecordsMap.LastPhysicalRecord;
                lsn = logRecordsMap.LastLogicalSequenceNumber;

                if (recoveredRecords.Count > 0)
                {
                    logRecordsDispatcher.DispatchLoggedRecords(new LoggedRecords(recoveredRecords, null));
                }

                var tailRecord = records.Current;

                FabricEvents.Events.Api(
                    tracer.Type,
                    "PerformRecoveryAsync: Current tail record Type: " + tailRecord.RecordType + " LSN: "
                    + tailRecord.Lsn.LSN + " PSN: " + tailRecord.Psn.PSN + " Position: " + tailRecord.RecordPosition);

                Utility.Assert(
                    lastPhysicalRecord == records.LastPhysicalRecord,
                    "lastPhysicalRecord == records.LastPhysicalRecord");
                Utility.Assert(
                    (tailRecord == lastPhysicalRecord) || (tailRecord.PreviousPhysicalRecord == lastPhysicalRecord),
                    "(tailRecord == lastPhysicalRecord) || "
                    + "(tailRecord.PreviousPhysicalRecord == lastPhysicalRecord), " + "LastPhysicalRecord PSN: {0}",
                    lastPhysicalRecord.Psn.PSN);
                Utility.Assert(
                    logRecordsMap.LastCompletedEndCheckpointRecord != null,
                    "this.lastCompletedEndCheckpointRecord != null");
                Utility.Assert(
                    logRecordsMap.LastInProgressCheckpointRecord == null,
                    "this.lastInProgressCheckpointRecord == null");
                Utility.Assert(logRecordsMap.LastLinkedPhysicalRecord != null, "this.lastLinkedPhysicalRecord != nul");
                Utility.Assert(lsn == tailRecord.Lsn, "lsn == tailRecord.LastLogicalSequenceNumber");

                // Disable read ahead as indexing physical records will read the log backwards
                replicatedLogManager.LogManager.SetSequentialAccessReadSize(this.RecoveryLogsReader.ReadStream, 0);

                await this.RecoveryLogsReader.IndexPhysicalRecords(lastPhysicalRecord).ConfigureAwait(false);
                var callbackManager = new PhysicalLogWriterCallbackManager(this.flushedRecordsCallback);
                callbackManager.FlushedPsn = tailRecord.Psn + 1;
                replicatedLogManager.LogManager.PrepareToLog(tailRecord, callbackManager);

                replicatedLogManager.Reuse(
                    recoveredLastCompletedBeginCheckpointRecord.ProgressVector,
                    logRecordsMap.LastCompletedEndCheckpointRecord,
                    logRecordsMap.LastInProgressCheckpointRecord,
                    logRecordsMap.LastLinkedPhysicalRecord,
                    replicatedLogManager.LastInformationRecord,
                    (IndexingLogRecord)this.RecoveryLogsReader.StartingRecord,
                    logRecordsMap.CurrentLogTailEpoch,
                    lsn);

                this.RecoveredLsn = lsn;

                // GopalK: The order of the following statements is significant
                if (this.logCompleteCheckpointAfterRecovery)
                {
                    replicatedLogManager.CompleteCheckpoint();
                }

                replicatedLogManager.Information(InformationEvent.Recovered);

                await replicatedLogManager.LogManager.FlushAsync("PerformRecovery").ConfigureAwait(false);
                await lastRecoverableRecord.AwaitProcessing().ConfigureAwait(false);
                await recordsProcessor.WaitForAllRecordsProcessingAsync().ConfigureAwait(false);

                lastPeriodicTruncationTimeTicks = logRecordsMap.LastPeriodicTruncationTimeTicks;
            }

            this.recoveryException = recordsProcessor.ServiceException;
            if (this.recoveryException == null)
            {
                this.recoveryException = recordsProcessor.LogException;
            }

            if (this.recoveryException != null)
            {
                FabricEvents.Events.Api(
                    tracer.Type,
                    "PerformRecoveryAsync: RecoveryFailed");

                // If there was an apply or unlock failure, report fault does not help during recovery because the replica is not opened yet.
                // The only solution here is to throw during OpenAsync
                throw this.recoveryException;
            }

            checkpointManager.Recover(
                recoveredLastCompletedBeginCheckpointRecord,
                lastPeriodicTruncationTimeTicks);
        }

        public void OnTailTruncation(LogicalSequenceNumber newTailLsn)
        {
            this.RecoveredLsn = newTailLsn;
        }

        public void Reuse()
        {
            this.logCompleteCheckpointAfterRecovery = true;
            this.IsRemovingStateAfterOpen = false;
            this.RecoveredLsn = LogicalSequenceNumber.InvalidLsn;
            this.RecoveryException = null;
            this.RecoveredLastEndCheckpointRecord = null;
            this.tailRecordAtStart = null;
            this.LastRecoveredAtomicRedoOperationLsn = 0;
        }

        public void DisposeRecoveryReadStreamIfNeeded()
        {
            if (this.RecoveryLogsReader != null)
            {
                this.RecoveryLogsReader.Dispose();
                this.RecoveryLogsReader = null;
            }
        }
    }
}
