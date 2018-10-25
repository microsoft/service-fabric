// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Log;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    /// <summary>
    /// Data used to initialize the LogManager
    /// </summary>
    public class LogManagerInitialization
    {
        /// <summary>
        /// Data used to initialize the LogManager
        /// </summary>
        /// <param name="path">Specifies the path for the location of the dedicated logs</param>
        /// <param name="directory">Specifies the path to the application work directory</param>
        /// <param name="partition">Specifies the partitionId for the replica</param>
        /// <param name="replica">Specifies the replicaId for the replica</param>
        public LogManagerInitialization(string path, string directory, Guid partition, long replica)
        {
            dedicatedLogPath = path;
            workDirectory = directory;
            partitionId = partition;
            replicaId = replica;
        }

        /// <summary>
        /// Dedicated log path
        /// </summary>
        public string dedicatedLogPath;

        /// <summary>
        /// Application work directory
        /// </summary>
        public string workDirectory;

        /// <summary>
        /// Partition id
        /// </summary>
        public Guid partitionId;

        /// <summary>
        /// Replica id
        /// </summary>
        public long replicaId;
    };

    internal abstract class LogManager : IDisposable
    {
        public const int ReadAheadBytesForSequentialStream = 1024*1024;

        protected const string BackupSuffix = "_Backup";

        protected const string CopySuffix = "_Copy";

        protected readonly AvgBytesPerFlushCounterWriter BytesPerFlushCounterWriter;

        protected readonly IncomingBytesRateCounterWriter IncomingBytesRateCounterWriter;

        protected readonly LogFlushBytesRateCounterWriter LogFlushBytesRateCounterWriter;

        protected readonly AvgFlushLatencyCounterWriter AvgFlushLatencyCounterWriter;

        protected readonly AvgSerializationLatencyCounterWriter AvgSerializationLatencyCounterWriter;

        private const int DefaultWriteCacheSizeMb = 300;

        private readonly object readersLock = new object();

        // Naming info of the log file
        private string baseLogFileAlias;

        private string currentLogFileAlias;

        // Disposition status
        private bool isDisposed;

        private string logFileDirectoryPath;

        private ulong logHeadRecordPosition;

        // Underlying LogicalLog OM components
        private ILogicalLog logicalLog;

        private int maxWriteCacheSizeInMB;

        // Log writer and reader
        private PhysicalLogWriter physicalLogWriter;

        private ITracer tracer;

        // Log cannot be truncated past this position
        private LogReaderRange earliestLogReader;

        private PhysicalLogWriterCallbackManager emptyCallbackManager;

        // Log Readers range
        private List<LogReaderRange> logReaderRanges;

        private PendingLogHeadTruncationContext pendingLogHeadTruncationContext;

        protected LogManager(
            string baseLogFileAlias,
            IncomingBytesRateCounterWriter incomingBytesRateCounter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter, 
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter)
        {
            this.BaseLogFileAlias = baseLogFileAlias;
            this.CurrentLogFileAlias = this.BaseLogFileAlias;
            this.LogicalLog = null;
            this.IsDisposed = false;
            this.LogHeadRecordPosition = 0;
            this.PhysicalLogWriter = null;
            this.logReaderRanges = new List<LogReaderRange>();
            this.earliestLogReader = LogReaderRange.DefaultLogReaderRange;
            this.pendingLogHeadTruncationContext = null;
            this.IncomingBytesRateCounterWriter = incomingBytesRateCounter;
            this.LogFlushBytesRateCounterWriter = logFlushBytesRateCounter;
            this.AvgFlushLatencyCounterWriter = avgFlushLatencyCounterWriter;
            this.AvgSerializationLatencyCounterWriter = avgSerializationLatencyCounterWriter;
            this.BytesPerFlushCounterWriter = bytesPerFlushCounterWriter;
        }

        ~LogManager()
        {
            this.DisposeInternal(false);
        }

        internal enum LogReaderType : int
        {
            Default = 0,

            Recovery = 1,

            PartialCopy = 2,

            FullCopy = 3,

            Backup = 4
        }

        public long BufferedBytes
        {
            get { return this.PhysicalLogWriter.BufferedRecordsBytes; }
        }

        public long PendingFlushBytes
        {
            get { return this.PhysicalLogWriter.PendingFlushBytes; }
        }

        internal PhysicalLogRecord CurrentLastPhysicalRecord
        {
            get { return this.PhysicalLogWriter.CurrentLastPhysicalRecord; }
        }

        internal ulong CurrentLogTailPosition
        {
            get { return this.PhysicalLogWriter.CurrentLogTailRecord.RecordPosition; }
        }

        internal LogRecord CurrentLogTailRecord
        {
            get { return this.PhysicalLogWriter.CurrentLogTailRecord; }
        }

        internal bool IsClosing
        {
            get
            {
                var closedException = this.PhysicalLogWriter.ClosedException;
                return closedException != null;
            }
        }

        internal bool IsCompletelyFlushed
        {
            get { return this.PhysicalLogWriter.IsCompletelyFlushed; }
        }

        internal long Length
        {
            get { return this.LogicalLog.Length; }
        }

        internal bool ShouldThrottleWrites
        {
            get { return this.PhysicalLogWriter.ShouldThrottleWrites; }
        }

        protected string BaseLogFileAlias
        {
            get { return this.baseLogFileAlias; }

            set { this.baseLogFileAlias = value; }
        }

        protected string CurrentLogFileAlias
        {
            get { return this.currentLogFileAlias; }

            set { this.currentLogFileAlias = value; }
        }

        protected bool IsDisposed
        {
            get { return this.isDisposed; }

            set { this.isDisposed = value; }
        }

        protected string LogFileDirectoryPath
        {
            get { return this.logFileDirectoryPath; }

            set { this.logFileDirectoryPath = value; }
        }

        protected ulong LogHeadRecordPosition
        {
            get { return this.logHeadRecordPosition; }

            set { this.logHeadRecordPosition = value; }
        }

        protected ILogicalLog LogicalLog
        {
            get { return this.logicalLog; }

            set { this.logicalLog = value; }
        }

        protected int MaxWriteCacheSizeInMB
        {
            get { return this.maxWriteCacheSizeInMB; }

            set { this.maxWriteCacheSizeInMB = value; }
        }

        public PhysicalLogWriter PhysicalLogWriter
        {
            get { return this.physicalLogWriter; }

            protected set { this.physicalLogWriter = value; }
        }

        protected ITracer Tracer
        {
            get { return this.tracer; }

            set { this.tracer = value; }
        }

        public void Dispose()
        {
            this.DisposeInternal(true);
        }

        internal bool AddLogReader(
            long startingLsn,
            ulong startingRecordPosition,
            ulong endingRecordPosition,
            string readerName,
            LogReaderType readerType)
        {
            Utility.Assert(
                (startingRecordPosition == 0) || (this.LogHeadRecordPosition <= startingRecordPosition),
                "(startingRecordPosition == 0) || (this.CurrentLogHeadPosition <= startingRecordPosition), start record position :{0}. LogHeadRecordPosition: {1}",
                startingRecordPosition, this.LogHeadRecordPosition);

            FabricEvents.Events.LogManager(
                this.Tracer.Type,
                "AddLogReader: Adding Log Reader with StartingRecordPosition: " + startingRecordPosition + " EndingRecordPosition: " + endingRecordPosition +
                " ReaderName: " + readerName);

            lock (this.readersLock)
            {
                if (this.LogHeadRecordPosition > startingRecordPosition)
                {
                    return false;
                }

                int i;
                LogReaderRange readerRange;
                for (i = 0; i < this.logReaderRanges.Count; i++)
                {
                    readerRange = this.logReaderRanges[i];
                    if (startingRecordPosition < readerRange.StartingRecordPosition)
                    {
                        break;
                    }

                    if (readerRange.StartingRecordPosition == startingRecordPosition)
                    {
                        readerRange.AddRef();

                        // If a fullcopy reader comes in at the same position of a non-full copy reader, over-write the type as full-copy reader
                        // have significance when determining safe LSN to delete state providers
                        if (readerType == LogReaderType.FullCopy)
                        {
                            readerRange.ReaderType = LogReaderType.FullCopy;
                        }

                        return true;
                    }
                }

                readerRange = new LogReaderRange(startingLsn, startingRecordPosition, readerName, readerType);
                this.logReaderRanges.Insert(i, readerRange);
                if (i == 0)
                {
                    this.earliestLogReader = this.logReaderRanges[0];
                }
            }

            Utility.Assert(
                (endingRecordPosition == long.MaxValue) || (this.CurrentLogTailPosition >= endingRecordPosition),
                @"(endingRecordPosition == Int64.MaxValue) || (this.CurrentLogTailPosition >= endingRecordPosition), EndingRecordPosition: {0}",
                endingRecordPosition);

            return true;
        }

        internal abstract Task<IndexingLogRecord> CreateCopyLogAsync(
            Epoch startingEpoch,
            LogicalSequenceNumber startingLsn);

        internal Stream CreateReaderStream()
        {
            // * TODO: Optimization: Old code established FileOptions.SequentialScan
            return this.LogicalLog.CreateReadStream(0);
        }

        internal async Task FlushAsync(string flushInitiator)
        {
            await this.PhysicalLogWriter.FlushAsync(flushInitiator).ConfigureAwait(false);
        }

        /// <summary>
        /// Earliest pending Full copy starting Lsn
        /// </summary>
        /// <returns>
        /// If there is no FullCopy reader, returns long.MaxValue
        /// </returns>
        internal long GetEarliestFullCopyStartingLsn()
        {
            var earliestFullCopyStartingLsn = long.MaxValue;

            lock (this.readersLock)
            {
                foreach (var readerRange in this.logReaderRanges)
                {
                    if (readerRange.ReaderType == LogReaderType.FullCopy
                        && readerRange.FullCopyStartingLsn < earliestFullCopyStartingLsn)
                    {
                        earliestFullCopyStartingLsn = readerRange.FullCopyStartingLsn;

                        // The first reader need not be the earliest as the list is NOT sorted by starting LSN of the enumeration
                        // So continue searching for the smallest value
                    }
                }
            }

            return earliestFullCopyStartingLsn;
        }

        internal string GetEarliestLogReaderName(out LogReaderType readerType)
        {
            lock (this.readersLock)
            {
                readerType = this.earliestLogReader.ReaderType;
                return this.earliestLogReader.ReaderName;
            }
        }

        internal Task GetFlushCompletionTask()
        {
            return this.PhysicalLogWriter.GetFlushCompletionTask();
        }

        /// <summary>
        /// Gets a log reader capable of reading records between the specified positions
        /// </summary>
        internal PhysicalLogReader GetPhysicalLogReader(
            ulong startingRecordPosition,
            ulong endingRecordPosition,
            long startingLsn,
            string readerName,
            LogReaderType readerType)
        {
            PhysicalLogReader logReader;
            do
            {
                logReader = new PhysicalLogReader(
                    this,
                    startingRecordPosition,
                    endingRecordPosition,
                    startingLsn,
                    readerName,
                    readerType);
                if (logReader.IsValid)
                {
                    break;
                }

                logReader.Dispose();
            } while (true);

            return logReader;
        }

        internal virtual Task InitializeAsync(ITracer tracer, LogManagerInitialization init, TransactionalReplicatorSettings settings)
        {
            this.Tracer = tracer;
            this.LogFileDirectoryPath = init.dedicatedLogPath;
            this.MaxWriteCacheSizeInMB = DefaultWriteCacheSizeMb;
            this.emptyCallbackManager = new PhysicalLogWriterCallbackManager(this.EmptyFlushedRecordsCallback, tracer);

            return Task.FromResult(0);
        }

        /// <summary>
        /// Creates or finds the log stream.
        /// If being created either initializes the log with default log records or records from backup log.
        /// </summary>
        /// <param name="openMode">Open mode of the replica.</param>
        /// <returns>Task that represents the asynchronous open operation.</returns>
        internal async Task<PhysicalLogReader> OpenAsync(ReplicaOpenMode openMode)
        {
            // TODO: Anurag: do we plumb c.token up?
            this.LogicalLog = await this.CreateLogFileAsync(openMode == ReplicaOpenMode.New, CancellationToken.None).ConfigureAwait(false);

            var logLogLength = this.LogicalLog.Length;
            if (logLogLength <= sizeof(int))
            {
                // No usable content in the log
                if (this.LogicalLog.WritePosition > 0)
                {
                    await this.LogicalLog.TruncateTail(0, CancellationToken.None).ConfigureAwait(false);

                    // Remove all contents and reset write cursor back to 0
                    Utility.Assert(this.LogicalLog.Length == 0, "this.logicalLog.Length == 0");
                    Utility.Assert(this.LogicalLog.WritePosition == 0, "this.logicalLog.WritePosition == 0");
                }

                using (
                    var logWriter = new PhysicalLogWriter(
                        this.LogicalLog,
                        this.emptyCallbackManager,
                        this.Tracer,
                        this.MaxWriteCacheSizeInMB,
                        this.IncomingBytesRateCounterWriter,
                        this.LogFlushBytesRateCounterWriter,
                        this.BytesPerFlushCounterWriter,
                        this.AvgFlushLatencyCounterWriter,
                        this.AvgSerializationLatencyCounterWriter,
                        false))
                {
                    var zeroIndexRecord = IndexingLogRecord.CreateZeroIndexingLogRecord();
                    logWriter.InsertBufferedRecord(zeroIndexRecord);
                    logWriter.InsertBufferedRecord(UpdateEpochLogRecord.CreateZeroUpdateEpochLogRecord());
                    var zeroBeginCheckpointRecord =
                        BeginCheckpointLogRecord.CreateZeroBeginCheckpointLogRecord();
                    logWriter.InsertBufferedRecord(zeroBeginCheckpointRecord);
                    logWriter.InsertBufferedRecord(BarrierLogRecord.CreateOneBarrierLogRecord());
                    var oneEndCheckpointRecord =
                        EndCheckpointLogRecord.CreateOneEndCheckpointLogRecord(
                            zeroBeginCheckpointRecord,
                            zeroIndexRecord);
                    logWriter.InsertBufferedRecord(oneEndCheckpointRecord);
                    var endCompleteCheckpointRecord =
                        new CompleteCheckpointLogRecord(
                            LogicalSequenceNumber.OneLsn,
                            zeroIndexRecord,
                            oneEndCheckpointRecord);
                    logWriter.InsertBufferedRecord(endCompleteCheckpointRecord);
                    await logWriter.FlushAsync("OpenAsync").ConfigureAwait(false);

                    // This additional await is required to ensure the log record was indeed flushed.
                    // Without this, the flushasync could succeed, but the log record flush could have failed due to a write error
                    await endCompleteCheckpointRecord.AwaitFlush().ConfigureAwait(false);
                }
            }

            return new PhysicalLogReader(this);
        }

        internal async Task<PhysicalLogReader> OpenWithRestoreFileAsync(
            ReplicaOpenMode openMode,
            FabricPerformanceCounterSetInstance perfCounterInstance,
            IList<string> backupLogFilePathList,
            int flushEveryNKB)
        {
            this.LogicalLog = await this.CreateLogFileAsync(openMode == ReplicaOpenMode.New, CancellationToken.None).ConfigureAwait(false);
            
            // No usable content in the log
            if (this.LogicalLog.WritePosition > 0)
            {
                await this.LogicalLog.TruncateTail(0, CancellationToken.None).ConfigureAwait(false);

                // Remove all contents and reset write cursor back to 0
                Utility.Assert(
                    this.LogicalLog.Length == 0 && this.LogicalLog.WritePosition == 0,
                    "{0}: this.logicalLog.Length: {1} this.logicalLog.WritePosition: {2}",
                    this.tracer.Type,
                    this.LogicalLog.Length,
                    this.LogicalLog.WritePosition);
            }

            using (PhysicalLogWriter logWriter = new PhysicalLogWriter(
                    this.LogicalLog,
                    this.emptyCallbackManager,
                    this.Tracer,
                    this.MaxWriteCacheSizeInMB,
                    this.IncomingBytesRateCounterWriter,
                    this.LogFlushBytesRateCounterWriter,
                    this.BytesPerFlushCounterWriter,
                    this.AvgFlushLatencyCounterWriter,
                    this.AvgSerializationLatencyCounterWriter,
                    true))
            {
                LogRecord record = null;
                LogRecordsMap logRecordsMap = null;
                long bufferedRecordsSizeBytes = -1;
                long backupRecordIndex = 0;

                foreach (string backupLogFilePath in backupLogFilePathList)
                {
                    BackupLogFile backupLogFile = await BackupLogFile.OpenAsync(
                        backupLogFilePath, 
                        CancellationToken.None).ConfigureAwait(false);

                    using (var backupLogEnumerator = backupLogFile.GetAsyncEnumerator())
                    {
                        if (logRecordsMap == null)
                        {
                            bool hasFirstRecord = await backupLogEnumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false);
                            Utility.Assert(hasFirstRecord, "{0}: Backup must include at least six records.", this.tracer.Type);

                            // If the log is being restored.
                            // First record must be a indexing log record. Flush it.
                            LogRecord firstRecordFromBackupLog = backupLogEnumerator.Current;

                            Utility.Assert(
                                null != firstRecordFromBackupLog,
                                "{0}: BackupLogEnumerator will never return null",
                                this.tracer.Type);
                            Utility.Assert(
                                false == LogRecord.IsInvalidRecord(firstRecordFromBackupLog),
                                "{0}: First record read from the backup log cannot be invalid",
                                this.tracer.Type);

                            IndexingLogRecord logHead = firstRecordFromBackupLog as IndexingLogRecord;

                            Utility.Assert(
                                null != logHead,
                                "{0}: First record read from the backup log must be indexing log record: Type: {1} LSN: {2} PSN: {3} Position: {4}",
                                this.tracer.Type,
                                firstRecordFromBackupLog.RecordType,
                                firstRecordFromBackupLog.Lsn.LSN,
                                firstRecordFromBackupLog.Psn.PSN,
                                firstRecordFromBackupLog.RecordPosition);

                            logRecordsMap = new LogRecordsMap(logHead, this.Tracer);

                            logRecordsMap.ProcessLogRecord(logHead);

                            bufferedRecordsSizeBytes = logWriter.InsertBufferedRecord(logHead);

                            // Note that logHead.PreviousPhysicalRecord is an InvalidLogRecord.
                            backupRecordIndex++;
                            FabricEvents.Events.RestoreOperationAsync(
                                this.Tracer.Type,
                                logHead.RecordType.ToString(),
                                backupRecordIndex,
                                logHead.Lsn.LSN,
                                logHead.Psn.PSN,
                                long.MaxValue);
                        }

                        while (await backupLogEnumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                        {
                            record = backupLogEnumerator.Current;

                            logRecordsMap.ProcessLogRecord(record);

                            // Note: Function inserts a record to the buffer where it waits to be flushed and return 
                            // the new size of the whole buffer.
                            bufferedRecordsSizeBytes = logWriter.InsertBufferedRecord(record);

                            backupRecordIndex++;
                            FabricEvents.Events.RestoreOperationAsync(
                                this.Tracer.Type,
                                record.RecordType.ToString(),
                                backupRecordIndex,
                                record.Lsn.LSN,
                                record.Psn.PSN,
                                record.PreviousPhysicalRecord.Psn.PSN);

                            // TODO: Use a backup config for this flush size determination
                            if (bufferedRecordsSizeBytes > flushEveryNKB*1024)
                            {
                                string intermediateFlushMessage = string.Format(
                                        CultureInfo.InvariantCulture,
                                        "LogManager: OpenWithRestoreFile (Restore) Intermediate Flush Size: {0} bytes, Index: {1}",
                                        bufferedRecordsSizeBytes,
                                        backupRecordIndex);

                                await logWriter.FlushAsync(intermediateFlushMessage).ConfigureAwait(false);

                                // This additional await is required to ensure the log record was indeed flushed.
                                // Without this, the flushasync could succeed, but the log record flush could have failed due to a write error
                                await record.AwaitFlush().ConfigureAwait(false);

                                bufferedRecordsSizeBytes = 0;
                            }
                        }
                    }
                }

                // Note: Move the last flush for remaining items out of the loop to avoid unnecessary flush in the 
                // case of each iteration has a small number of un-flushed log records.
                if (bufferedRecordsSizeBytes > 0)
                {
                    string flushMessage = string.Format(
                            CultureInfo.InvariantCulture,
                            "LogManager: OpenWithRestoreFile (Restore)");
                    await logWriter.FlushAsync(flushMessage).ConfigureAwait(false);

                    // This additional await is required to ensure the log record was indeed flushed.
                    // Without this, the flushasync could succeed, but the log record flush could have failed due to a write error
                    await record.AwaitFlush().ConfigureAwait(false);
                }
            }

            return new PhysicalLogReader(this);
        }

        internal async Task PerformLogTailTruncationAsync(LogRecord newTailRecord)
        {
            try
            {
                await this.PhysicalLogWriter.TruncateLogTail(newTailRecord).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "LogManager::PerformLogTailTruncationAsync",
                    this.Tracer,
                    "truncating log tail",
                    e);
            }
        }

        internal void PrepareToClose()
        {
            // It is possible the physicallogwriter is null if there was an incomplete CR->None during which we crashed and the next open skipped recovery due to this
            // It is only at the end of recovery that we set the physical log writer to a valid object
            if (this.PhysicalLogWriter == null)
            {
                return;
            }

            this.PhysicalLogWriter.PrepareToClose();
        }

        internal virtual async Task CloseLogAsync(CancellationToken cancellationToken)
        {
            var snapLogicalLog = this.logicalLog;
            if (snapLogicalLog != null)
            {
                this.logicalLog = null;
                await snapLogicalLog.CloseAsync(cancellationToken).ConfigureAwait(false);
                snapLogicalLog.Dispose();
            }
        }

        internal abstract Task DeleteLogAsync();

        internal void PrepareToLog(LogRecord tailRecord, PhysicalLogWriterCallbackManager callbackManager)
        {
            this.PhysicalLogWriter = new PhysicalLogWriter(
                this.LogicalLog,
                tailRecord,
                callbackManager,
                null,
                this.Tracer,
                this.MaxWriteCacheSizeInMB,
                this.IncomingBytesRateCounterWriter,
                this.LogFlushBytesRateCounterWriter,
                this.BytesPerFlushCounterWriter,
                this.AvgFlushLatencyCounterWriter,
                this.AvgSerializationLatencyCounterWriter);
        }

        internal Task ProcessLogHeadTruncationAsync(TruncateHeadLogRecord truncateHeadRecord)
        {
            var tcs = new TaskCompletionSource<object>();
            var context = new PendingLogHeadTruncationContext(
                tcs,
                truncateHeadRecord);

            lock (this.readersLock)
            {
                if (this.earliestLogReader.StartingRecordPosition < truncateHeadRecord.HeadRecord.RecordPosition)
                {
                    Utility.Assert(
                        this.pendingLogHeadTruncationContext == null,
                        "Pending Log Head Truncation should be null");

                    this.pendingLogHeadTruncationContext = context;

                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "ProcessLogHeadTruncationAsync: Could not truncate log head immediately due to reader: " + this.earliestLogReader.ReaderName);

                    return tcs.Task;
                }
                else
                {
                    this.LogHeadRecordPosition = truncateHeadRecord.HeadRecord.RecordPosition;

                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "ProcessLogHeadTruncationAsync:  Initiating log head truncation to position: " + this.LogHeadRecordPosition);

                    Task.Factory.StartNew(this.PerformLogHeadTruncation, context).IgnoreExceptionVoid();
                    return tcs.Task;
                }
            }
        }

        internal void RemoveLogReader(ulong startingRecordPosition)
        {
            FabricEvents.Events.LogManager(
                this.Tracer.Type,
                "RemoveLogReader: Removing Log Reader with StartingRecordPosition: " + startingRecordPosition);

            lock (this.readersLock)
            {
                LogReaderRange readerRange;
                for (var i = 0; i < this.logReaderRanges.Count; i++)
                {
                    readerRange = this.logReaderRanges[i];
                    if (readerRange.StartingRecordPosition == startingRecordPosition)
                    {
                        if (readerRange.Release() > 0)
                        {
                            return;
                        }

                        this.logReaderRanges.RemoveAt(i);
                        if (i == 0)
                        {
                            this.earliestLogReader = (this.logReaderRanges.Count > 0)
                                ? this.logReaderRanges[0]
                                : LogReaderRange.DefaultLogReaderRange;
                        }

                        // check if removing this reader could help in completing the pending log head truncation 
                        if (this.pendingLogHeadTruncationContext != null
                            && this.earliestLogReader.StartingRecordPosition
                            >= this.pendingLogHeadTruncationContext.ProposedPosition)
                        {
                            this.LogHeadRecordPosition = this.pendingLogHeadTruncationContext.ProposedPosition;
                            FabricEvents.Events.LogManager(
                                this.Tracer.Type,
                                "RemoveLogReader: Initiating PerformLogHeadTruncation head position: " + this.LogHeadRecordPosition);

                            var localpendingLogHeadTruncationContext = this.pendingLogHeadTruncationContext;
                            Task.Factory.StartNew(this.PerformLogHeadTruncation, localpendingLogHeadTruncationContext)
                                .IgnoreExceptionVoid();
                            this.pendingLogHeadTruncationContext = null;
                        }

                        return;
                    }
                }
            }

            Utility.CodingError("Code should never have come here: RemoveLogReader");
        }

        internal abstract Task RenameCopyLogAtomicallyAsync();

        internal void SetLogHeadRecordPosition(ulong newPosition)
        {
            this.LogHeadRecordPosition = newPosition;
        }

        internal void SetSequentialAccessReadSize(Stream logStream, int sequentialAccessReadSize)
        {
            this.LogicalLog.SetSequentialAccessReadSize(logStream, sequentialAccessReadSize);
        }

        internal async Task WaitForPendingRecordsToFlush()
        {
            await this.FlushAsync("WaitForPendingFlushToComplete").ConfigureAwait(false);
        }

        protected async Task CloseCurrentLogAsync()
        {
            Utility.Assert(this.logReaderRanges.Count == 0, "this.logReaderRanges.Count == 0");

            this.PhysicalLogWriter.Dispose();
            FabricEvents.Events.LogManager(this.Tracer.Type, "CloseCurrentLog: Alias: " + this.CurrentLogFileAlias);

            var log = this.LogicalLog;
            this.LogicalLog = null;
            await log.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        protected abstract Task<ILogicalLog> CreateLogFileAsync(bool createNew, CancellationToken cancellationToken);

        protected virtual void DisposeInternal(bool isDisposing)
        {
            if (isDisposing)
            {
                if (this.IsDisposed == false)
                {
                    if (this.PhysicalLogWriter != null)
                    {
                        this.PhysicalLogWriter.Dispose();
                    }

                    if (this.LogicalLog != null)
                    {
                        this.LogicalLog.Dispose();
                    }

                    this.IsDisposed = true;
                    GC.SuppressFinalize(this);
                }
            }
        }

        private void EmptyFlushedRecordsCallback(LoggedRecords loggedRecords)
        {
            var records = loggedRecords.Records;
            var exception = loggedRecords.Exception;
            foreach (var record in records)
            {
                if (exception == null)
                {
                    FabricEvents.Events.LogManagerNoOpRecordsCallback(
                        this.Tracer.Type,
                        record.RecordType.ToString(),
                        record.Lsn.LSN,
                        record.Psn.PSN,
                        record.RecordPosition);
                }
                else
                {
                    FabricEvents.Events.LogManagerNoOpRecordsCallbackFailed(
                        this.Tracer.Type,
                        record.RecordType.ToString(),
                        record.Lsn.LSN,
                        record.Psn.PSN,
                        record.RecordPosition);
                }

                record.CompletedFlush(exception);
            }
        }

        private void PerformLogHeadTruncation(object context)
        {
            var truncationContext = (PendingLogHeadTruncationContext) context;

            int freeBackwardLinkCallCount;
            int freeBackwardLinkSucceededCallCount;

            truncationContext.NewHeadRecord.OnTruncateHead(
                out freeBackwardLinkCallCount,
                out freeBackwardLinkSucceededCallCount);

            FabricEvents.Events.ProcessLogHeadTruncationDone(
                this.tracer.Type,
                truncationContext.NewHeadRecord.Lsn.LSN,
                truncationContext.NewHeadRecord.Psn.PSN,
                truncationContext.NewHeadRecord.RecordPosition,
                freeBackwardLinkCallCount,
                freeBackwardLinkSucceededCallCount);

            try
            {
                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    "PerformLogHeadTruncation:  Invoking TruncateLogHead API at position: " + truncationContext.ProposedPosition);

                this.PhysicalLogWriter.TruncateLogHead(truncationContext.ProposedPosition);
                truncationContext.Tcs.SetResult(null);
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "LogManager::PerformLogHeadTruncation",
                    this.Tracer,
                    "truncating log head",
                    e);
                truncationContext.Tcs.SetResult(e);
            }
        }

        private class LogReaderRange
        {
            internal static readonly LogReaderRange DefaultLogReaderRange = new LogReaderRange(
                long.MaxValue,
                long.MaxValue,
                Constants.DefaultEarliestLogReader,
                LogReaderType.Default);

            private readonly long fullCopyStartingLsn;

            private readonly ulong startingRecordPosition;

            private LogReaderType logReaderType;

            private int referenceCount;

            internal LogReaderRange(
                long fullCopyStartingLsn,
                ulong startingRecordPosition,
                string readerName,
                LogReaderType readerType)
            {
                this.startingRecordPosition = startingRecordPosition;
                this.referenceCount = 1;
                this.ReaderName = readerName;
                this.logReaderType = readerType;
                this.fullCopyStartingLsn = fullCopyStartingLsn;
            }

            internal long FullCopyStartingLsn
            {
                get { return this.fullCopyStartingLsn; }
            }

            internal string ReaderName { get; private set; }

            internal LogReaderType ReaderType
            {
                get { return this.logReaderType; }

                set { this.logReaderType = value; }
            }

            internal ulong StartingRecordPosition
            {
                get { return this.startingRecordPosition; }
            }

            internal void AddRef()
            {
                Utility.Assert(
                    this.startingRecordPosition != long.MaxValue,
                    "this.startingRecordPosition != Int64.MaxValue");

                ++this.referenceCount;
            }

            internal int Release()
            {
                Utility.Assert(
                    this.startingRecordPosition != long.MaxValue,
                    "this.startingRecordPosition != Int64.MaxValue");

                --this.referenceCount;

                return this.referenceCount;
            }
        }

        private class PendingLogHeadTruncationContext
        {
            private readonly TruncateHeadLogRecord truncateHeadLogRecord;

            public PendingLogHeadTruncationContext(TaskCompletionSource<object> tcs, TruncateHeadLogRecord newHeadRecord)
            {
                this.Tcs = tcs;
                this.truncateHeadLogRecord = newHeadRecord;
            }

            public IndexingLogRecord NewHeadRecord
            {
                get
                {
                    return this.truncateHeadLogRecord.HeadRecord;
                }
            }

            public ulong ProposedPosition {
                get
                {
                    return this.truncateHeadLogRecord.HeadRecord.RecordPosition;
                }
            }

            public TaskCompletionSource<object> Tcs { get; private set; }
        }
    }
}
