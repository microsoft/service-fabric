// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Data.Log;
    using System.Globalization;
    using System.IO;
    using System.Runtime.CompilerServices;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    internal class PhysicalLogWriter : IDisposable
    {
        private const int MovingAverageHistory = 10;

        private static readonly TaskCompletionSource<object> FlushPendingTcs;

        private static readonly TaskCompletionSource<object> FlushPermittedTcs;

        private readonly List<long> avgRunningLatencyMilliseconds = new List<long>(MovingAverageHistory);

        private readonly List<long> avgWriteSpeedBytesPerSecond = new List<long>(MovingAverageHistory);

        private readonly AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter;

        // Flushing related datastructures
        private readonly object flushLock = new object();

        private readonly IncomingBytesRateCounterWriter incomingBytesRateCounterWriter;

        private readonly LogFlushBytesRateCounterWriter logFlushBytesRateCounterWriter;

        private readonly AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter;

        private readonly AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter;

        private readonly long maxWriteCacheSizeInBytes;

        private readonly bool recomputeRecordOffsets;

        private List<LogRecord> bufferedRecords;

        private long bufferedRecordsBytes;

        private PhysicalLogWriterCallbackManager callbackManager;

        private Exception closedException;

        // Log tail positions
        private ulong currentLogTailPosition;

        private PhysicalSequenceNumber currentLogTailPsn;

        private LogRecord currentLogTailRecord;

        private TaskCompletionSource<object> exitTcs;

        private List<LogRecord> flushingRecords;

        private TaskCompletionSource<object> flushNotificationTcs;

        private bool isCallbackManagerReset;

        private bool isDisposed;

        private PhysicalLogRecord lastPhysicalRecord;

        private Exception loggingException;

        private ILogicalLog logicalLogStream;

        private List<LogRecord> pendingFlushRecords;

        private long pendingFlushRecordsBytes;

        private List<TaskCompletionSource<object>> pendingFlushTasks;

        private long runningLatencySumMs = 0;

        private ITracer tracer;

        private long writeSpeedBytesPerSecondSum = 0;

        private BinaryWriter recordWriter;

        static PhysicalLogWriter()
        {
            FlushPermittedTcs = new TaskCompletionSource<object>();
            FlushPermittedTcs.SetResult(null);

            FlushPendingTcs = new TaskCompletionSource<object>();
            FlushPendingTcs.SetException(new InvalidOperationException("Awaited on a flush pending TCS"));
        }

        internal PhysicalLogWriter(
            ILogicalLog logicalLogStream,
            PhysicalLogWriterCallbackManager callbackManager,
            ITracer tracer,
            int maxWriteCacheSizeInMB,
            IncomingBytesRateCounterWriter incomingBytesRateCounterWriter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounterWriter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter,
            bool recomputeRecordOffsets)
        {
            Utility.Assert(callbackManager != null, "PhysicalLogWriter cannot accept null callback managers");

            this.maxWriteCacheSizeInBytes = maxWriteCacheSizeInMB*1024*1024;
            this.Init(tracer, null);
            this.logicalLogStream = logicalLogStream;
            this.callbackManager = callbackManager;
            this.currentLogTailPosition = (ulong) logicalLogStream.WritePosition;
            this.currentLogTailRecord = LogRecord.InvalidLogRecord;
            this.currentLogTailPsn = new PhysicalSequenceNumber(-1);
            this.lastPhysicalRecord = null;
            this.incomingBytesRateCounterWriter = incomingBytesRateCounterWriter;
            this.logFlushBytesRateCounterWriter = logFlushBytesRateCounterWriter;
            this.bytesPerFlushCounterWriter = bytesPerFlushCounterWriter;
            this.avgFlushLatencyCounterWriter = avgFlushLatencyCounterWriter;
            this.avgSerializationLatencyCounterWriter = avgSerializationLatencyCounterWriter;
            this.recordWriter = new BinaryWriter(new MemoryStream());
            this.recomputeRecordOffsets = recomputeRecordOffsets;
        }

        internal PhysicalLogWriter(
            ILogicalLog logicalLogStream,
            LogRecord tailRecord,
            PhysicalLogWriterCallbackManager callbackManager,
            Exception closedException,
            ITracer tracer,
            int maxWriteCacheSizeInMB,
            IncomingBytesRateCounterWriter incomingBytesRateCounterWriter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounterWriter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter)
        {
            Utility.Assert(callbackManager != null, "PhysicalLogWriter cannot accept null callback managers");

            this.incomingBytesRateCounterWriter = incomingBytesRateCounterWriter;
            this.logFlushBytesRateCounterWriter = logFlushBytesRateCounterWriter;
            this.bytesPerFlushCounterWriter = bytesPerFlushCounterWriter;
            this.avgFlushLatencyCounterWriter = avgFlushLatencyCounterWriter;
            this.avgSerializationLatencyCounterWriter = avgSerializationLatencyCounterWriter;

            this.maxWriteCacheSizeInBytes = maxWriteCacheSizeInMB*1024*1024;
            this.Init(tracer, closedException);
            this.logicalLogStream = logicalLogStream;
            this.callbackManager = callbackManager;
            this.SetTailRecord(tailRecord);
            Utility.Assert(
                this.currentLogTailPosition == (ulong) logicalLogStream.WritePosition,
                "this.currentLogTailPosition:{0} should be equal to logicalLogStream.WritePosition:{1}",
                this.currentLogTailPosition, logicalLogStream.WritePosition);
            this.recordWriter = new BinaryWriter(new MemoryStream());
            this.recomputeRecordOffsets = false;
        }

        ~PhysicalLogWriter()
        {
            this.Dispose(false);
        }

        public long BufferedRecordsBytes
        {
            get { return Interlocked.Read(ref this.bufferedRecordsBytes); }
        }

        public long PendingFlushBytes
        {
            get { return Interlocked.Read(ref this.pendingFlushRecordsBytes); }
        }

        internal PhysicalLogWriterCallbackManager CallbackManager
        {
            get { return this.callbackManager; }
        }

        internal Exception ClosedException
        {
            get { return this.closedException; }
        }

        internal PhysicalLogRecord CurrentLastPhysicalRecord
        {
            get
            {
                Utility.Assert(
                    this.lastPhysicalRecord != PhysicalLogRecord.InvalidPhysicalLogRecord,
                    "this.lastPhysicalRecord != PhysicalLogRecord.InvalidPhysicalLogRecord");
                return this.lastPhysicalRecord;
            }
        }

        internal PhysicalSequenceNumber CurrentLogTailPsn
        {
            get { return this.currentLogTailPsn; }
        }

        internal LogRecord CurrentLogTailRecord
        {
            get { return this.currentLogTailRecord; }
        }

        internal TaskCompletionSource<object> ExitTcs
        {
            get { return this.exitTcs; }

            set { this.exitTcs = value; }
        }

        internal bool IsCompletelyFlushed
        {
            get
            {
                return (this.bufferedRecords == null) && (this.pendingFlushRecords == null)
                       && (this.flushingRecords == null);
            }
        }

        internal bool ResetCallbackManager
        {
            get { return this.isCallbackManagerReset; }

            set { this.isCallbackManagerReset = value; }
        }

        internal bool ShouldThrottleWrites
        {
            get { return Interlocked.Read(ref this.pendingFlushRecordsBytes) >= this.maxWriteCacheSizeInBytes; }
        }

        internal bool RecomputeOffsets
        {
            get { return this.recomputeRecordOffsets;  }
        }

        public void Dispose()
        {
            this.Dispose(true);
        }

        internal async Task FlushAsync(string flushInitiator)
        {
            var isFlushTask = false;
            var isFlushing = false;
            TaskCompletionSource<object> tcs = null;
            LogRecord flushWaitRecord;

            lock (this.flushLock)
            {
                if (this.flushingRecords == null)
                {
                    this.flushingRecords = this.bufferedRecords;
                    if (this.flushingRecords != null)
                    {
                        if (this.exitTcs != null)
                        {
                            this.exitTcs.SetResult(null);
                            Thread.Sleep(Timeout.InfiniteTimeSpan);
                        }

                        this.bufferedRecords = null;

#if DEBUG
                        var cachedPendingFlushRecordsBytes = Interlocked.Read(ref this.pendingFlushRecordsBytes);
                        Utility.Assert(
                            cachedPendingFlushRecordsBytes == 0,
                            "PendingFlushRecordBytes is non-zero : {0}",
                            cachedPendingFlushRecordsBytes);
#endif
                        var originalValue = Interlocked.Exchange(
                            ref this.pendingFlushRecordsBytes,
                            this.bufferedRecordsBytes);
                        Interlocked.Exchange(ref this.bufferedRecordsBytes, 0);
                        isFlushing = true;
                        isFlushTask = this.loggingException == null;
                        flushWaitRecord = this.flushingRecords[this.flushingRecords.Count - 1];
                    }
                    else
                    {
                        flushWaitRecord = this.currentLogTailRecord;
                    }
                }
                else
                {
                    tcs = new TaskCompletionSource<object>();
                    if (this.pendingFlushRecords != null)
                    {
                        if (this.bufferedRecords != null)
                        {
                            this.pendingFlushRecords.AddRange(this.bufferedRecords);
                        }

                        Utility.Assert(this.pendingFlushTasks != null, "this.pendingFlushTasks != null");
                    }
                    else
                    {
                        this.pendingFlushRecords = this.bufferedRecords;
                        if (this.pendingFlushTasks == null)
                        {
                            this.pendingFlushTasks = new List<TaskCompletionSource<object>>();
                        }
                    }

#if DEBUG
                    var cachedPendingFlushRecordsBytes = Interlocked.Read(ref this.pendingFlushRecordsBytes);
                    Utility.Assert(
                        cachedPendingFlushRecordsBytes > 0,
                        "PendingFlushRecordBytes is not greater than zero : {0}",
                        cachedPendingFlushRecordsBytes);
#endif

                    this.bufferedRecords = null;
                    var pendingFlushSize = Interlocked.Add(
                        ref this.pendingFlushRecordsBytes,
                        this.bufferedRecordsBytes);
                    if (pendingFlushSize > this.maxWriteCacheSizeInBytes)
                    {
                        var trace = string.Format(
                            CultureInfo.InvariantCulture,
                            "PhysicalLogWriter:Pending Flush Size {0} greater than MaxWriteCacheSize {1}. Throttling writes",
                            pendingFlushSize,
                            this.maxWriteCacheSizeInBytes);

                        FabricEvents.Events.Warning(this.tracer.Type, trace);
                    }

                    Interlocked.Exchange(ref this.bufferedRecordsBytes, 0);
                    this.pendingFlushTasks.Add(tcs);
                    if (this.pendingFlushRecords != null)
                    {
                        flushWaitRecord = this.pendingFlushRecords[this.pendingFlushRecords.Count - 1];
                    }
                    else
                    {
                        flushWaitRecord = this.flushingRecords[this.flushingRecords.Count - 1];
                    }
                }
            }

            if (tcs != null)
            {
                await tcs.Task.ConfigureAwait(false);
            }
            else if (isFlushing == true)
            {
                if (isFlushTask == true)
                {
                    var initiatingTcs = new TaskCompletionSource<object>();
                    Task.Factory.StartNew<Task>(this.FlushTask, initiatingTcs).IgnoreExceptionVoid();

                    await initiatingTcs.Task.ConfigureAwait(false);
                }
                else
                {
                    this.FailedFlushTask(new List<TaskCompletionSource<object>>());
                }
            }
            else
            {
                return;
            }

            var latestFlushedRecord = this.currentLogTailRecord;

            FabricEvents.Events.PhysicalLogWriterFlushAsync(
                this.tracer.Type,
                flushInitiator + " initiated flush. Waited Record Type: " + flushWaitRecord.RecordType,
                flushWaitRecord.Lsn.LSN,
                flushWaitRecord.Psn.PSN,
                flushWaitRecord.RecordPosition,
                latestFlushedRecord.RecordType.ToString(),
                latestFlushedRecord.Lsn.LSN,
                latestFlushedRecord.Psn.PSN,
                latestFlushedRecord.RecordPosition);

            return;
        }

        internal Task GetFlushCompletionTask()
        {
            TaskCompletionSource<object> createdTcs = null;
            var notificationTcs = this.flushNotificationTcs;
            do
            {
                if (notificationTcs == FlushPermittedTcs)
                {
                    break;
                }

                if (createdTcs == null)
                {
                    createdTcs = new TaskCompletionSource<object>();
                }

                notificationTcs =
                    Interlocked.CompareExchange<TaskCompletionSource<object>>(
                        ref this.flushNotificationTcs,
                        createdTcs,
                        FlushPendingTcs);
                if (notificationTcs == FlushPendingTcs)
                {
                    notificationTcs = createdTcs;
                    break;
                }
            } while (true);

            return notificationTcs.Task;
        }

        /// <summary>
        /// Inserts a record to the buffer where it waits to be flushed in batches.
        /// A successful run will set following properties in the record.
        ///     record.Psn and record.PreviousPhysicalLogRecord
        /// </summary>
        /// <param name="record"></param>
        /// <returns>The pending size of the buffered records</returns>
        internal long InsertBufferedRecord(LogRecord record)
        {
            Utility.Assert(LogRecord.IsInvalidRecord(record) == false, "LogRecord.IsInvalidRecord(record) == false");

            var physicalRecord = record as PhysicalLogRecord;
            lock (this.flushLock)
            {
                if (this.closedException == null)
                {
                    // No record can be inserted after 'RemovingState' Information log record
                    if (this.lastPhysicalRecord != null
                        && this.lastPhysicalRecord.RecordType == LogRecordType.Information)
                    {
                        var lastInformationRecord = this.lastPhysicalRecord as InformationLogRecord;
                        Utility.Assert(
                            lastInformationRecord.InformationEvent != InformationEvent.RemovingState,
                            "No record should be inserted after 'RemovingState'. Current Record = {0}",
                            record);
                    }

                    // Update record and tail
                    ++this.currentLogTailPsn;
                    record.Psn = this.currentLogTailPsn;
                    this.incomingBytesRateCounterWriter.IncrementBy(record.ApproximateSizeOnDisk);

                    record.PreviousPhysicalRecord = this.lastPhysicalRecord;
                    if (physicalRecord != null)
                    {
                        if (this.lastPhysicalRecord != null)
                        {
                            this.lastPhysicalRecord.NextPhysicalRecord = physicalRecord;
                        }

                        this.lastPhysicalRecord = physicalRecord;
                    }

                    // Buffer record
                    if (this.bufferedRecords == null)
                    {
                        this.bufferedRecords = new List<LogRecord>();
                    }

                    this.bufferedRecords.Add(record);
                    var newBufferedRecordsBytes = Interlocked.Add(
                        ref this.bufferedRecordsBytes,
                        record.ApproximateSizeOnDisk);

                    return newBufferedRecordsBytes;
                }
            }

            var failedRecord = new LoggedRecords(record, this.closedException);
            this.ProcessFlushedRecords(failedRecord);

            return Interlocked.Read(ref this.bufferedRecordsBytes);
        }

        internal async Task PrepareForExit()
        {
            var tcs = new TaskCompletionSource<object>();
            lock (this.flushLock)
            {
                this.exitTcs = tcs;

                if (this.flushingRecords == null)
                {
                    return;
                }
            }

            await tcs.Task.ConfigureAwait(false);

            return;
        }

        internal void PrepareToClose()
        {
            lock (this.flushLock)
            {
                this.closedException = new FabricObjectClosedException(
                    "Log is closing or closed",
                    FabricErrorCode.ObjectClosed);
            }

            return;
        }

        internal void TruncateLogHead(ulong headRecordPosition)
        {
            if (this.logicalLogStream != null)
            {
                Utility.Assert(headRecordPosition <= long.MaxValue, "headRecordPosition <= long.MaxValue");
                lock (this.flushLock)
                {
                    // Fire off background/best effort truncate head
                    this.logicalLogStream.TruncateHead((long) headRecordPosition);
                }
            }

            return;
        }

        internal async Task TruncateLogTail(LogRecord newTailRecord)
        {
            this.callbackManager.FlushedPsn = newTailRecord.Psn + 1;
            this.SetTailRecord(newTailRecord);
            await this.logicalLogStream.TruncateTail((long) this.currentLogTailPosition, CancellationToken.None).ConfigureAwait(false);

            return;
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing == true)
            {
                if (this.isDisposed == false)
                {
                    Utility.Assert(this.IsCompletelyFlushed == true, "IsCompletelyFlushed == true");
                    if ((this.isCallbackManagerReset == false) && (this.callbackManager != null))
                    {
                        this.callbackManager.Dispose();
                    }

                    if (this.recordWriter != null)
                    {
                        this.recordWriter.BaseStream.Dispose();
                        this.recordWriter.Dispose();
                    }

                    GC.SuppressFinalize(this);
                    this.isDisposed = true;
                }
            }

            return;
        }

        private void FailedFlushTask(List<TaskCompletionSource<object>> flushingTasks)
        {
            Utility.Assert(this.loggingException != null, "this.loggingException != null");

            var isFlushTask = true;
            do
            {
                // GopalK: The order of the following instructions is very important.
                // It is important to process flushed records first before 
                // waking up flush waiters
                var flushedTasks = flushingTasks;
                var flushedRecords = new LoggedRecords(this.flushingRecords, this.loggingException);
                this.ProcessFlushedRecords(flushedRecords);

                isFlushTask = this.ProcessFlushCompletion(flushedRecords, ref flushedTasks, out flushingTasks);

                this.WakeupFlushWaiters(flushedTasks);
            } while (isFlushTask == true);

            return;
        }

        private TaskCompletionSource<object> FlushCompleted()
        {
            var notificationTcs = this.flushNotificationTcs;
            do
            {
                var currentTcs = notificationTcs;
                notificationTcs =
                    Interlocked.CompareExchange<TaskCompletionSource<object>>(
                        ref this.flushNotificationTcs,
                        FlushPermittedTcs,
                        currentTcs);
                if (notificationTcs == currentTcs)
                {
                    break;
                }
            } while (true);

            if (notificationTcs != FlushPendingTcs)
            {
                notificationTcs.SetResult(null);
            }

            return null;
        }

        private async Task FlushTask(object initiatingTcs)
        {
            var flushingTasks = new List<TaskCompletionSource<object>>();
            flushingTasks.Add((TaskCompletionSource<object>) initiatingTcs);

            var isFlushTask = true;
            var flushWatch = Stopwatch.StartNew();
            var serializationWatch = Stopwatch.StartNew();
            do
            {
                TaskCompletionSource<object> notificationTcs;

                try
                {
                    Utility.Assert(this.loggingException == null, "this.loggingException == null");

                    var numberOfLatencySensitiveRecords = 0;
                    ulong numberOfBytes = 0;
                    var startingPsn = this.flushingRecords[0].Psn;

                    FabricEvents.Events.PhysicalLogWriterFlushStart(
                        this.tracer.Type,
                        this.flushingRecords.Count,
                        startingPsn.PSN);

                    // Enable group commit as we are about to issue IO
                    // Operations above this line must not throw.
                    notificationTcs = Interlocked.Exchange<TaskCompletionSource<object>>(
                        ref this.flushNotificationTcs,
                        FlushPendingTcs);

                    flushWatch.Reset();
                    serializationWatch.Reset();

                    foreach (var record in this.flushingRecords)
                    {
                        serializationWatch.Start();
                        var operationData = this.WriteRecord(record, numberOfBytes);
#if DEBUG
                        ReplicatedLogManager.ValidateOperationData(operationData, "Log Write for " + record.RecordType);
#endif
                        var logicalRecord = record as LogicalLogRecord;
                        if (logicalRecord != null && logicalRecord.IsLatencySensitiveRecord == true)
                        {
                            ++numberOfLatencySensitiveRecords;
                        }

                        serializationWatch.Stop();

                        this.avgSerializationLatencyCounterWriter.IncrementBy(serializationWatch);

                        flushWatch.Start();
                        foreach (var arraySegment in operationData)
                        {
                            numberOfBytes += (ulong) arraySegment.Count;
                            await
                                this.logicalLogStream.AppendAsync(
                                    arraySegment.Array,
                                    arraySegment.Offset,
                                    arraySegment.Count,
                                    CancellationToken.None).ConfigureAwait(false);
                        }
                        flushWatch.Stop();
                    }

                    Utility.Assert(notificationTcs == FlushPermittedTcs, "NotificationTcs == FlushPermittedTcs");

                    flushWatch.Start();
                    await this.logicalLogStream.FlushWithMarkerAsync(CancellationToken.None).ConfigureAwait(false);
                    flushWatch.Stop();

                    this.UpdateWriteStats(flushWatch, numberOfBytes);

                    var lastWrittenTailRecord = this.flushingRecords[this.flushingRecords.Count - 1];

                    // Save the new log tail position and no need to interlock as single 
                    // instruction 64-bit writes are atomic on 64-bit machines and there
                    // can only be one thread here
                    this.currentLogTailPosition += numberOfBytes;
                    this.currentLogTailRecord = lastWrittenTailRecord;
                    this.bytesPerFlushCounterWriter.IncrementBy((long) numberOfBytes);

                    if (flushWatch.ElapsedMilliseconds > 3000)
                    {
                        FabricEvents.Events.PhysicalLogWriterFlushEndWarning(
                            this.tracer.Type,
                            numberOfBytes,
                            this.flushingRecords.Count,
                            numberOfLatencySensitiveRecords,
                            flushWatch.ElapsedMilliseconds,
                            serializationWatch.ElapsedMilliseconds,
                            (double) this.writeSpeedBytesPerSecondSum/MovingAverageHistory,
                            (double) this.runningLatencySumMs/MovingAverageHistory,
                            this.logicalLogStream.WritePosition - (long)numberOfBytes);
                    }
                    else 
                    {
                        FabricEvents.Events.PhysicalLogWriterFlushEnd(
                            this.tracer.Type,
                            numberOfBytes,
                            this.flushingRecords.Count,
                            numberOfLatencySensitiveRecords,
                            flushWatch.ElapsedMilliseconds,
                            serializationWatch.ElapsedMilliseconds,
                            (double) this.writeSpeedBytesPerSecondSum/MovingAverageHistory,
                            (double) this.runningLatencySumMs/MovingAverageHistory,
                            this.logicalLogStream.WritePosition - (long)numberOfBytes);
                    }
                }
                catch (Exception e)
                {
                    this.loggingException = e;
                }
                finally
                {
                    try
                    {
                        // Disable group commit as there is no pending IO
                        notificationTcs = this.FlushCompleted();

                        // GopalK: The order of the following instructions is very important.
                        // It is important to process flushed records first before 
                        // waking up flush waiters
                        var flushedTasks = flushingTasks;
                        var flushedRecords = new LoggedRecords(this.flushingRecords, this.loggingException);
                        this.ProcessFlushedRecords(flushedRecords);

                        isFlushTask = this.ProcessFlushCompletion(flushedRecords, ref flushedTasks, out flushingTasks);

                        this.WakeupFlushWaiters(flushedTasks);
                        if ((isFlushTask == true) && (this.loggingException != null))
                        {
                            this.FailedFlushTask(flushingTasks);
                            isFlushTask = false;
                        }
                    }
                    catch (Exception e)
                    {
                        int innerHResult = 0;
                        var exception = Utility.FlattenException(e, out innerHResult);
                        var exceptionMessage =
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "PhysicalLogWriter::FlushTask() - Hit an exception, Type: {0}, Message: {1}, HResult: 0x{2:X8}, Stack Trace: {3}",
                                exception.GetType().ToString(),
                                exception.Message,
                                exception.HResult != 0 ? exception.HResult : innerHResult,
                                exception.StackTrace);
                        FabricEvents.Events.Warning(this.tracer.Type, exceptionMessage);

                        Utility.Assert(false, exceptionMessage);
                        throw;
                    }
                }
            } while (isFlushTask == true);

            return;
        }

        private void Init(ITracer tracer, Exception closedException)
        {
            Utility.Assert(this.maxWriteCacheSizeInBytes > 0, "MaxWriteCacheSizeInBytes must be greater than 0");
            this.isCallbackManagerReset = false;
            this.tracer = tracer;
            this.flushNotificationTcs = FlushPermittedTcs;
            this.isDisposed = false;
            this.closedException = closedException;
            this.exitTcs = null;

            this.bufferedRecords = null;
            this.bufferedRecordsBytes = 0;
            this.pendingFlushRecordsBytes = 0;

            this.flushingRecords = null;
            this.pendingFlushRecords = null;
            this.pendingFlushTasks = null;
            this.loggingException = null;

            for (var i = 0; i < MovingAverageHistory; i++)
            {
                this.avgRunningLatencyMilliseconds.Add(0);
                this.avgWriteSpeedBytesPerSecond.Add(0);
            }
        }

        private bool ProcessFlushCompletion(
            LoggedRecords flushedRecords,
            ref List<TaskCompletionSource<object>> flushedTasks,
            out List<TaskCompletionSource<object>> flushingTasks)
        {
            Utility.Assert(flushedTasks != null, "flushedTasks != null");

            bool isFlushTask;
            lock (this.flushLock)
            {
                foreach (var record in flushedRecords.Records)
                {
                    // Subtract the approximate size on disk for the records that got flushed
                    var result = Interlocked.Add(ref this.pendingFlushRecordsBytes, -record.ApproximateSizeOnDisk);
#if DEBUG
                    Utility.Assert(
                        result >= 0,
                        "Subtraction of ApproximateSizeOnDisk {0} for record Lsn: {1} yielded negative value for pending flush records {2}",
                        record.ApproximateSizeOnDisk, record.Lsn.LSN, result);
#endif
                }

                if (this.exitTcs != null)
                {
                    this.exitTcs.SetResult(null);

                    Thread.Sleep(Timeout.InfiniteTimeSpan);
                }

                this.flushingRecords = this.pendingFlushRecords;
                if (this.flushingRecords == null)
                {
                    if (this.pendingFlushTasks != null)
                    {
                        flushedTasks.AddRange(this.pendingFlushTasks);
                        this.pendingFlushTasks = null;
                    }

                    flushingTasks = null;
                    isFlushTask = false;
                }
                else
                {
                    flushingTasks = this.pendingFlushTasks;
                    this.pendingFlushRecords = null;
                    this.pendingFlushTasks = null;

                    isFlushTask = true;
                }
            }

            return isFlushTask;
        }

        private void ProcessFlushedRecords(LoggedRecords flushedRecords)
        {
            if (this.callbackManager != null)
            {
                this.callbackManager.ProcessFlushedRecords(flushedRecords);
            }

            return;
        }

        private void SetTailRecord(LogRecord tailRecord)
        {
            this.currentLogTailPosition = tailRecord.RecordPosition + tailRecord.RecordSize;
            this.currentLogTailRecord = tailRecord;
            this.currentLogTailPsn = tailRecord.Psn;
            var physicalRecord = tailRecord as PhysicalLogRecord;
            this.lastPhysicalRecord = (physicalRecord == null) ? tailRecord.PreviousPhysicalRecord : physicalRecord;
            Utility.Assert(
                this.lastPhysicalRecord != PhysicalLogRecord.InvalidPhysicalLogRecord,
                "this.lastPhysicalRecord ({0}) != PhysicalLogRecord.InvalidPhysicalLogRecord",
                this.lastPhysicalRecord);

            return;
        }

        private void UpdateWriteStats(Stopwatch watch, ulong size)
        {
            var bytesWritten = size;
            this.logFlushBytesRateCounterWriter.IncrementBy((long) bytesWritten);
            this.avgFlushLatencyCounterWriter.IncrementBy(watch);

            var localAvgWriteSpeed = (long) ((bytesWritten*10000*1000)/(ulong) (watch.ElapsedTicks + 1));
            var localAvgRunnigLatencyMs = watch.ElapsedMilliseconds;

            this.runningLatencySumMs -= this.avgRunningLatencyMilliseconds[0];
            this.avgRunningLatencyMilliseconds.RemoveAt(0);
            this.avgRunningLatencyMilliseconds.Add(localAvgRunnigLatencyMs);
            this.runningLatencySumMs += localAvgRunnigLatencyMs;

            this.writeSpeedBytesPerSecondSum -= this.avgWriteSpeedBytesPerSecond[0];
            this.avgWriteSpeedBytesPerSecond.RemoveAt(0);
            this.avgWriteSpeedBytesPerSecond.Add(localAvgWriteSpeed);
            this.writeSpeedBytesPerSecondSum += localAvgWriteSpeed;
        }

        private void WakeupFlushWaiters(List<TaskCompletionSource<object>> waitingTasks)
        {
            if (waitingTasks != null)
            {
                foreach (var tcs in waitingTasks)
                {
                    tcs.SetResult(null);
                }
            }

            return;
        }

        private OperationData WriteRecord(LogRecord record, ulong bytesWritten)
        {
            // GopalK: The order of the following instructions is very important
            Utility.Assert(
                this.currentLogTailPosition + bytesWritten == (ulong) this.logicalLogStream.WritePosition,
                "this.currentLogTailPosition == (ulong) this.logicalLogStream.WritePosition");

            // Update record position
            var recordPosition = this.currentLogTailPosition + bytesWritten;
            record.RecordPosition = recordPosition;

            // Write record
            return LogRecord.WriteRecord(record, this.recordWriter, true, true, this.recomputeRecordOffsets);
        }
    }
}