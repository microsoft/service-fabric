// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;

    internal class LoggingReplicatorCopyStream : IOperationDataStream, IDisposable
    {
        public const int CopyMetadataVersion = 1;

        public const int CopyStateMetadataVersion = 1;

        // 2 * sizeof(int) + sizeof(long)
        private const int CopyMetadataByteSize = 16;

        // sizeof(int) + 6 * sizeof(long) 
        private const int CopyStateMetadataByteSizeNotIncludingProgressVector = 52;

        private static readonly ArraySegment<byte> CopyFalseProgressOperation;

        private static readonly ArraySegment<byte> CopyLogOperation;

        private static readonly ArraySegment<byte> CopyProgressVectorOperation;

        private static readonly ArraySegment<byte> CopyStateOperation;

        private readonly IOperationDataStream copyContext;

        private readonly ReplicatedLogManager replicatedLogManager;

        private readonly CheckpointManager checkpointManager;

        private readonly IStateManager stateManager;

        private readonly long replicaId;

        private Func<LogicalSequenceNumber, Task> waitForLogFlushUptoLsn;

        private BeginCheckpointLogRecord beginCheckpointRecord;

        private int copiedRecordNumber;

        private CopyStage copyStage;

        private IOperationDataStream copyStateStream;

        private LogicalSequenceNumber currentTargetLsn;

        private bool isDisposed;

        private long latestRecoveredAtomicRedoOperationLsn;

        private IAsyncEnumerator<LogRecord> logRecordsToCopy;

        private LogicalSequenceNumber sourceStartingLsn;

        private Epoch targetLogHeadEpoch;

        private LogicalSequenceNumber targetLogHeadLsn;

        private ProgressVector targetProgressVector;

        private long targetReplicaId;

        private LogicalSequenceNumber targetStartingLsn;

        private LogicalSequenceNumber uptoLsn;

        private BinaryWriter bw;

        private ITracer tracer;

        static LoggingReplicatorCopyStream()
        {
            using (var stream = new MemoryStream())
            {
                using (var bw = new BinaryWriter(stream))
                {
                    bw.Write((int) CopyStage.CopyState);
                    CopyStateOperation = new ArraySegment<byte>(stream.ToArray());

                    stream.Position = 0;
                    bw.Write((int) CopyStage.CopyProgressVector);
                    CopyProgressVectorOperation = new ArraySegment<byte>(stream.ToArray());

                    stream.Position = 0;
                    bw.Write((int) CopyStage.CopyFalseProgress);
                    CopyFalseProgressOperation = new ArraySegment<byte>(stream.ToArray());

                    stream.Position = 0;
                    bw.Write((int) CopyStage.CopyLog);
                    CopyLogOperation = new ArraySegment<byte>(stream.ToArray());
                }
            }
        }

        internal LoggingReplicatorCopyStream(
            ReplicatedLogManager replicatedLogManager,            
            IStateManager stateManager,
            CheckpointManager checkpointManager,
            Func<LogicalSequenceNumber, Task> waitForLogFlushUptoLsn,
            long replicaId,
            LogicalSequenceNumber uptoLsn,
            IOperationDataStream copyContext,
            ITracer tracer)
        {
            this.stateManager = stateManager;
            this.checkpointManager = checkpointManager;
            this.replicatedLogManager = replicatedLogManager;
            this.replicaId = replicaId;
            this.waitForLogFlushUptoLsn = waitForLogFlushUptoLsn;
            this.uptoLsn = uptoLsn;
            this.copyContext = copyContext;
            this.targetReplicaId = 0;
            this.targetProgressVector = null;
            this.targetLogHeadEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.targetLogHeadLsn = LogicalSequenceNumber.InvalidLsn;
            this.currentTargetLsn = LogicalSequenceNumber.InvalidLsn;
            this.copyStage = CopyStage.CopyMetadata;
            this.copyStateStream = null;
            this.copiedRecordNumber = 0;
            this.sourceStartingLsn = LogicalSequenceNumber.InvalidLsn;
            this.targetStartingLsn = LogicalSequenceNumber.InvalidLsn;
            this.logRecordsToCopy = null;
            this.beginCheckpointRecord = null;
            this.bw = new BinaryWriter(new MemoryStream());
            this.isDisposed = false;
            this.tracer = tracer;
        }

        public void Dispose()
        {
            this.PrivateDispose(true);
        }

        public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            try
            {
                return GetNextAsyncSafe(cancellationToken);
            }
            catch (Exception)
            {
                if (this.copyStateStream != null)
                {
                    var disposable = this.copyStateStream as IDisposable;
                    if (disposable != null)
                    {
                        disposable.Dispose();
                    }
                }

                if (this.logRecordsToCopy != null)
                {
                    var disposable = this.logRecordsToCopy as IDisposable;
                    if (disposable != null)
                    {
                        disposable.Dispose();
                    }
                }

                throw;
            }
        }

        private async Task<OperationData> GetNextAsyncSafe(CancellationToken cancellationToken)
        {
            OperationData data;

            if (this.copyStage == CopyStage.CopyMetadata)
            {
                data = await this.copyContext.GetNextAsync(CancellationToken.None).ConfigureAwait(false);
                Utility.Assert(data.Count == 1, "data.Count == 1");

                var bytes = data[0].Array;

                using (var br = new BinaryReader(new MemoryStream(bytes, data[0].Offset, data[0].Count)))
                {
                    this.targetReplicaId = br.ReadInt64();
                    this.targetProgressVector = new ProgressVector();
                    this.targetProgressVector.Read(br, false);
                    var targetLogHeadDatalossNumber = br.ReadInt64();
                    var targetLogHeadConfigurationNumber = br.ReadInt64();
                    this.targetLogHeadEpoch = new Epoch(targetLogHeadDatalossNumber, targetLogHeadConfigurationNumber);
                    this.targetLogHeadLsn = new LogicalSequenceNumber(br.ReadInt64());
                    this.currentTargetLsn = new LogicalSequenceNumber(br.ReadInt64());
                    this.latestRecoveredAtomicRedoOperationLsn = br.ReadInt64();
                }

                var message =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Target Replica Copy Context: {0}" + Environment.NewLine
                        + "     TargetLogHeadEpoch: {1},{2} TargetLogHeadLsn: {3} TargetLogTailEpoch: {4},{5} TargetLogTailLSN: {6}"
                        + Environment.NewLine
                        + "     SourceLogHeadEpoch: {7},{8} SourceLogHeadLsn: {9} SourceLogTailEpoch: {10},{11} SourceLogTailLSN: {12}"
                        + Environment.NewLine + "     Target ProgressVector: {13}" + Environment.NewLine
                        + "     Source ProgressVector: {14}" + Environment.NewLine
                        + "     LastRecoveredAtomicRedoOperationLsn: {15}",
                        this.targetReplicaId,
                        this.targetLogHeadEpoch.DataLossNumber,
                        this.targetLogHeadEpoch.ConfigurationNumber,
                        this.targetLogHeadLsn.LSN,
                        this.targetProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber,
                        this.targetProgressVector.LastProgressVectorEntry.Epoch.ConfigurationNumber,
                        this.currentTargetLsn.LSN,
                        this.replicatedLogManager.CurrentLogHeadRecord.CurrentEpoch.DataLossNumber,
                        this.replicatedLogManager.CurrentLogHeadRecord.CurrentEpoch.ConfigurationNumber,
                        this.replicatedLogManager.CurrentLogHeadRecord.Lsn.LSN,
                        this.replicatedLogManager.CurrentLogTailEpoch.DataLossNumber,
                        this.replicatedLogManager.CurrentLogTailEpoch.ConfigurationNumber,
                        this.replicatedLogManager.CurrentLogTailLsn.LSN,
                        this.targetProgressVector.ToString(Constants.ProgressVectorMaxStringSizeInKb / 2),
                        this.replicatedLogManager.ProgressVector.ToString(Constants.ProgressVectorMaxStringSizeInKb / 2),
                        this.latestRecoveredAtomicRedoOperationLsn);

                FabricEvents.Events.CopyStreamMetadata(tracer.Type, message);

                await this.waitForLogFlushUptoLsn(this.uptoLsn).ConfigureAwait(false);

                CopyMode copyMode;
                await this.checkpointManager.AcquireBackupAndCopyConsistencyLockAsync("Copy").ConfigureAwait(false);
                try
                {
                    copyMode = this.checkpointManager.GetLogRecordsToCopy(
                        this.targetProgressVector,
                        this.targetLogHeadEpoch,
                        this.targetLogHeadLsn,
                        this.currentTargetLsn,
                        this.latestRecoveredAtomicRedoOperationLsn,
                        this.targetReplicaId,
                        out this.sourceStartingLsn,
                        out this.targetStartingLsn,
                        out this.logRecordsToCopy,
                        out this.beginCheckpointRecord);
                }
                finally
                {
                    this.checkpointManager.ReleaseBackupAndCopyConsistencyLock("Copy");
                }

                CopyStage copyStageToWrite;
                if (copyMode == CopyMode.None)
                {
                    this.copyStage = CopyStage.CopyNone;
                    copyStageToWrite = CopyStage.CopyNone;
                }
                else if (copyMode == CopyMode.Full)
                {
                    this.copyStage = CopyStage.CopyState;
                    copyStageToWrite = CopyStage.CopyState;
                    this.copyStateStream = this.stateManager.GetCurrentState();
                }
                else if ((copyMode & CopyMode.FalseProgress) != 0)
                {
                    Utility.Assert(
                        this.sourceStartingLsn <= this.targetStartingLsn,
                        "this.sourceStartingLsn ({0}) <= this.targetStartingLsn ({1})",
                        this.sourceStartingLsn, this.targetStartingLsn);

                    this.copyStage = CopyStage.CopyFalseProgress;
                    copyStageToWrite = CopyStage.CopyFalseProgress;
                }
                else
                {
                    this.copyStage = CopyStage.CopyScanToStartingLsn;
                    copyStageToWrite = CopyStage.CopyLog;
                }

                return this.GetCopyMetadata(copyStageToWrite);
            }

            if (this.copyStage == CopyStage.CopyState)
            {
                data = await this.copyStateStream.GetNextAsync(cancellationToken).ConfigureAwait(false);
                if (data != null)
                {
                    data.Add(CopyStateOperation);

                    FabricEvents.Events.CopyStreamGetNextNoise(
                        tracer.Type,
                        "Copying State Operation: " + this.copiedRecordNumber);

                    ++this.copiedRecordNumber;
                    return data;
                }

                var disposableCopyStateStream = this.copyStateStream as IDisposable;
                if (disposableCopyStateStream != null)
                {
                    disposableCopyStateStream.Dispose();
                }

                if (this.uptoLsn < beginCheckpointRecord.LastStableLsn)
                {
                    // Ensure we copy records up to the stable LSN of the begin checkpoint record as part of the copy stream
                    // so that the idle can initiate a checkpoint and apply it as part of the copy pump itself

                    // Not doing the above will mean that the idle can get promoted to active even before the checkpoint is completed
                    // uptolsn is inclusive LSN
                    // BUG: RDBug #9269022:Replicator must rename copy log before changing role to active secondary during full builds
                    this.uptoLsn = beginCheckpointRecord.LastStableLsn;

                    var trace = "FullCopy: Copying Logs from " + this.sourceStartingLsn.LSN
                                + " to BC.LastStableLsn: " + beginCheckpointRecord.LastStableLsn.LSN + " as the upto lsn: " + this.uptoLsn + " was smaller than the checkpoint stable lsn";

                    FabricEvents.Events.CopyStreamGetNext(tracer.Type, trace);
                }

                data = this.GetCopyStateMetadata();

                this.copyStage = CopyStage.CopyLog;
                this.copiedRecordNumber = 0;

                return data;
            }

            if (this.copyStage == CopyStage.CopyFalseProgress)
            {
                this.copyStage = CopyStage.CopyScanToStartingLsn;

                this.bw.BaseStream.Position = 0;
                this.bw.Write(this.sourceStartingLsn.LSN);
                var stream = this.bw.BaseStream as MemoryStream;
                data = new OperationData(new ArraySegment<byte>( stream.GetBuffer(), 0, (int) stream.Position));
                data.Add(CopyFalseProgressOperation);

                var trace = "Copying False Progress. SourceStartingLSN: " + this.sourceStartingLsn.LSN
                            + " TargetStartingLSN: " + this.targetStartingLsn.LSN;

                FabricEvents.Events.CopyStreamGetNext(tracer.Type, trace);
                return data;
            }

            LogRecord record;
            if (this.copyStage == CopyStage.CopyScanToStartingLsn)
            {
                var startingLsn = (this.sourceStartingLsn < this.targetStartingLsn)
                    ? this.sourceStartingLsn
                    : this.targetStartingLsn;

                do
                {
                    var hasMoved = await this.logRecordsToCopy.MoveNextAsync(cancellationToken).ConfigureAwait(false);
                    if (hasMoved == false)
                    {
                        goto Finish;
                    }

                    record = this.logRecordsToCopy.Current;
                    Utility.Assert(record.Lsn <= startingLsn, "record.Lsn <= startingLsn");
                } while (record.Lsn < startingLsn);

                // The log stream is positioned at the end of startingLsn at this point. The enumerator Current is pointing to the "startingLsn"
                this.copyStage = CopyStage.CopyLog;
            }

            if (this.copyStage == CopyStage.CopyLog)
            {
                do
                {
                    // This will start copying after the startingLsn record as expected
                    var hasMoved = await this.logRecordsToCopy.MoveNextAsync(cancellationToken).ConfigureAwait(false);
                    if (hasMoved == false)
                    {
                        goto Finish;
                    }

                    record = this.logRecordsToCopy.Current;
                } while (record is PhysicalLogRecord);

                if (record.Lsn > this.uptoLsn)
                {
                    goto Finish;
                }

                var logicalLogRecord = record as LogicalLogRecord;

                Utility.Assert(logicalLogRecord != null, "Must be a logical log record");

                data = logicalLogRecord.ToOperationData(this.bw);
                data.Add(CopyLogOperation);

                var trace = "Copying log record. LogRecordNumber: " + this.copiedRecordNumber + "Record Type: "
                            + record.RecordType + " LSN: " + record.Lsn.LSN;

                FabricEvents.Events.CopyStreamGetNextNoise(tracer.Type, trace);
                ++this.copiedRecordNumber;
                return data;
            }

            Finish:
            if (this.logRecordsToCopy != null)
            {
                this.logRecordsToCopy.Dispose();
            }

            FabricEvents.Events.CopyStreamFinished(tracer.Type, this.copiedRecordNumber);

            return null;
        }

        private OperationData GetCopyMetadata(CopyStage copyStageToWrite)
        {
            OperationData result;

            using (var stream = new MemoryStream(CopyMetadataByteSize))
            {
                using (var bw = new BinaryWriter(stream))
                {
                    // First 4 bytes are reserved for the version of the metadata.
                    bw.Write(CopyMetadataVersion);

                    bw.Write((int) copyStageToWrite);
                    bw.Write(this.replicaId);

                    result = new OperationData(new ArraySegment<byte>(stream.GetBuffer(), 0, (int) stream.Position));
                }
            }

            var trace = "Copying Metadata. CopyStage: " + this.copyStage + " SourceStartingLSN: "
                        + this.sourceStartingLsn.LSN + " TargetStartingLSN: " + this.targetStartingLsn.LSN;

            FabricEvents.Events.CopyStreamGetNext(tracer.Type, trace);
            return result;
        }

        /// <summary>
        /// Returns the metadata for the copied state.
        /// </summary>
        /// <returns></returns>
        /// <remarks>
        /// Contains 
        /// - Version of the copy metadata state.
        /// - Progress ProgressVectorEntry of the copied checkpoint.
        /// - Copied Checkpoints Epoch
        /// - Sequence number of the first operation in the log to be copied.
        /// - Sequence number of the copied checkpoints sequence number.
        /// - UptoSequence number for the copy provided by the volatile replicator.
        /// - Highest possible the state providers could have copied.
        /// </remarks>
        private OperationData GetCopyStateMetadata()
        {
            var expectedSize = this.beginCheckpointRecord.ProgressVector.ByteCount;
            expectedSize += CopyStateMetadataByteSizeNotIncludingProgressVector;

            OperationData result;

            using (var stream = new MemoryStream(expectedSize))
            {
                using (var bw = new BinaryWriter(stream))
                {
                    // First 4 bytes are reserved for the version of the metadata.
                    bw.Write(CopyStateMetadataVersion);

                    // TODO: Ask for the size to avoid expanding the memoryStream.
                    this.beginCheckpointRecord.ProgressVector.Write(bw);

                    var startingEpoch = this.beginCheckpointRecord.Epoch;
                    bw.Write(startingEpoch.DataLossNumber);
                    bw.Write(startingEpoch.ConfigurationNumber);

                    bw.Write(this.sourceStartingLsn.LSN);
                    bw.Write(this.beginCheckpointRecord.Lsn.LSN);
                    bw.Write(this.uptoLsn.LSN);
                    bw.Write(this.replicatedLogManager.CurrentLogTailLsn.LSN);

                    Utility.Assert(
                        expectedSize == stream.Position,
                        "Position mismatch. Expected {0} Position {1}",
                        expectedSize, stream.Position);

                    result = new OperationData(new ArraySegment<byte>(stream.GetBuffer(), 0, (int) stream.Position));
                    result.Add(CopyProgressVectorOperation);
                }
            }

            var trace = "Copying Log Preamble. SourceStartingLSN: " + this.sourceStartingLsn.LSN
                        + " BeginCheckpointLSN: " + this.beginCheckpointRecord.Lsn.LSN + " StateRecordsCoipied: "
                        + this.copiedRecordNumber + " CurrentTailSequenceNumber: "
                        + this.replicatedLogManager.CurrentLogTailLsn.LSN;

            FabricEvents.Events.CopyStreamGetNext(tracer.Type, trace);
            return result;
        }

        private void PrivateDispose(bool isDisposing)
        {
            if (isDisposing == true)
            {
                if (this.isDisposed == false)
                {
                    var disposableCopyStateStream = this.copyStateStream as IDisposable;
                    if (disposableCopyStateStream != null)
                    {
                        disposableCopyStateStream.Dispose();
                    }

                    if (this.logRecordsToCopy != null)
                    {
                        this.logRecordsToCopy.Dispose();
                    }
                    
                    this.bw.Dispose();

                    GC.SuppressFinalize(this);
                    this.isDisposed = true;
                }
            }
        }
    }
}
