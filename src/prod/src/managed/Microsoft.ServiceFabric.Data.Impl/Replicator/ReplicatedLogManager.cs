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
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics.CodeAnalysis;

    internal class ReplicatedLogManager : IReplicatedLogManager
    {
        private const int PrimaryWarningExceptionTraceIntervalMs = 5000;
        private readonly object lsnOrderingLock = new object();
        private readonly RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn;

        // If there has been no replication in the past 1 minute, clear out all the memory streams to reduce memory footprint of replicator
        [SuppressMessage(FxCop.Category.Reliability, "CA2000:DisposeObjectsBeforeLosingScope", Justification = "Caller is responsible for disposing.")]
        private readonly SynchronizedBufferPool<BinaryWriter> replicationSerializationBinaryWritersPool = new SynchronizedBufferPool<BinaryWriter>(() => new BinaryWriter(new MemoryStream()), Constants.MaxReplicationWriteMemoryStreamsBufferPoolItemCount);

        private IStateReplicator fabricReplicator;
        private Stopwatch lastWarningExceptionTraceOnPrimary;
        private List<LogicalLogRecord> lsnOrderingQueue = new List<LogicalLogRecord>();
        private TaskCompletionSource<object> lsnOrderingTcs;
        private long operationAcceptedCount;
        private Timer replicationSerializationBinaryWritersPoolClearTimer;
        private Func<bool, BeginCheckpointLogRecord> appendCheckpointCallback;
        private ITracer tracer;

        public ReplicatedLogManager(
            LogManager logManager,
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn)
        {
            this.LogManager = logManager;
            this.recoveredOrCopiedCheckpointLsn = recoveredOrCopiedCheckpointLsn;
            Reuse();
        }

        public IndexingLogRecord CurrentLogHeadRecord { get; private set; }

        public Epoch CurrentLogTailEpoch
        {
            get; private set;
        }

        public LogicalSequenceNumber CurrentLogTailLsn
        {
            get; private set;
        }

        public BeginCheckpointLogRecord LastCompletedBeginCheckpointRecord
        {
            get
            {
                if (this.LastCompletedEndCheckpointRecord == null)
                {
                    return null;
                }

                return this.LastCompletedEndCheckpointRecord.LastCompletedBeginCheckpointRecord;
            }
        }

        public EndCheckpointLogRecord LastCompletedEndCheckpointRecord { get; private set; }

        public IndexingLogRecord LastIndexRecord { get; private set; }

        public InformationLogRecord LastInformationRecord
        {
            get; private set;
        }

        public BeginCheckpointLogRecord LastInProgressCheckpointRecord { get; private set; }

        public TruncateHeadLogRecord LastInProgressTruncateHeadRecord { get; private set; }

        // Either last completed checkpoint or latest log head truncation record
        public PhysicalLogRecord LastLinkedPhysicalRecord
        {
            get; private set;
        }

        public LogManager LogManager
        {
            get; private set;
        }

        public ProgressVector ProgressVector
        {
            get; private set;
        }

        public SynchronizedBufferPool<BinaryWriter> ReplicationSerializationBinaryWritersPool
        {
            get; private set;
        }

        public RoleContextDrainState RoleContextDrainState
        {
            get; private set;
        }

#if DEBUG
        /// <summary>
        /// This method is to ensure that the V2 replicator does not pass in-valid data to either the V1 or the logger
        /// If the user passes in NULL, the V2 replicator does the right conversions
        /// </summary>
        /// <param name="data"></param>
        /// <param name="context"></param>
        public static void ValidateOperationData(OperationData data, string context)
        {
            for (var i = 0; i < data.Count; i++)
            {
                Utility.Assert(
                    data[i].Array != null && data[i].Count > 0,
                    "Invalid Operation Data as segment {0} is either null or has count = {1}. Context = {2}",
                    i, data[i].Count, context);
            }
        }
#endif

        public void AppendBarrier(
            BarrierLogRecord barrierLogRecord,
            bool isPrimary)
        {
            if (isPrimary == true)
            {
                ReplicateAndLog(barrierLogRecord, null);
                return;
            }

            AppendWithoutReplication(
                barrierLogRecord,
                null);
        }

        public long AppendWithoutReplication(
            LogicalLogRecord record,
            TransactionManager transactionsManager)
        {
            return Append(record, transactionsManager, false);
        }

        public async Task AwaitLsnOrderingTaskOnPrimaryAsync()
        {
            // On the primary, since replication happens outside the lsnOrderingLock and logging happens within,
            // there could be pending replication operations that must be awaited to get logged before going ahead with
            // the close or change role or update epoch
            Task lsnOrderingTask = null;

            lock (this.lsnOrderingLock)
            {
                if (this.lsnOrderingTcs != null)
                {
                    lsnOrderingTask = this.lsnOrderingTcs.Task;
                }
                else if (this.operationAcceptedCount > 0 || this.lsnOrderingQueue.Count > 0)
                {
                    this.lsnOrderingTcs = new TaskCompletionSource<object>();
                    lsnOrderingTask = this.lsnOrderingTcs.Task;
                }
            }

            if (lsnOrderingTask != null)
            {
                await lsnOrderingTask.ConfigureAwait(false);
            }
        }

        ///
        /// Append complete checkpoint record
        ///
        public void CompleteCheckpoint()
        {
            lock (lsnOrderingLock)
            {
                var record = new CompleteCheckpointLogRecord(
                    this.CurrentLogTailLsn,
                    this.CurrentLogHeadRecord,
                    this.LastLinkedPhysicalRecord);
                this.LastLinkedPhysicalRecord = record;
                this.LogManager.PhysicalLogWriter.InsertBufferedRecord(record);
            }
        }

        /// <summary>
        /// Append end checkpoint record
        /// </summary>
        public void EndCheckpoint(BeginCheckpointLogRecord lastInProgressCheckpointRecord)
        {
            lock (lsnOrderingLock)
            {
                EndCheckpointCallerHoldsLock(lastInProgressCheckpointRecord);
            }
        }

        public async Task FlushEndCheckpointAsync()
        {
            lock (lsnOrderingLock)
            {
                this.EndCheckpointCallerHoldsLock(this.LastInProgressCheckpointRecord);
            }

            this.LogManager.FlushAsync("FlushEndCheckpointAsync").IgnoreExceptionVoid();
            await this.LastCompletedEndCheckpointRecord.AwaitFlush().ConfigureAwait(false);
        }

        async Task IReplicatedLogManager.FlushAsync(string initiator)
        {
            await this.LogManager.FlushAsync(initiator).ConfigureAwait(false);
        }

        public async Task FlushInformationRecordAsync(
            InformationEvent type,
            bool closeLog,
            string flushInitiator)
        {
            lock (lsnOrderingLock)
            {
                this.Information(type);
                if (closeLog)
                {
                    this.LogManager.PrepareToClose();
                }
            }

            await this.LogManager.FlushAsync(flushInitiator).ConfigureAwait(false);
        }

        public LogRecord GetEarliestRecord()
        {
            lock (lsnOrderingLock)
            {
                return GetEarliestRecordCallerHoldsLock();
            }
        }

        /// <summary>
        /// Append indexing record
        /// </summary>
        public void Index()
        {
            lock (lsnOrderingLock)
            {
                this.LastIndexRecord = new IndexingLogRecord(
                    this.CurrentLogTailEpoch,
                    this.CurrentLogTailLsn,
                    this.LastLinkedPhysicalRecord);
                this.LogManager.PhysicalLogWriter.InsertBufferedRecord(this.LastIndexRecord);
            }
        }

        /// <summary>
        /// Append information record
        /// </summary>
        public void Information(InformationEvent informationEvent)
        {
            lock (lsnOrderingLock)
            {
                this.LastInformationRecord = new InformationLogRecord(
                    this.CurrentLogTailLsn,
                    this.LastLinkedPhysicalRecord,
                    informationEvent);

                this.LogManager.PhysicalLogWriter.InsertBufferedRecord(this.LastInformationRecord);
            }
        }

        public void OnCompletedPendingCheckpoint()
        {
            this.LastInProgressCheckpointRecord = null;
        }

        public void OnCompletedPendingLogHeadTruncation()
        {
            this.LastInProgressTruncateHeadRecord = null;
        }

        public void OnTruncateTailOfLastLinkedPhysicalRecord()
        {
            this.LastLinkedPhysicalRecord = this.LastLinkedPhysicalRecord.LinkedPhysicalRecord;
        }

        public void Open(
            ITracer tracer,
            Func<bool, BeginCheckpointLogRecord> appendCheckpointCallback,
            IndexingLogRecord currentHead,
            RoleContextDrainState roleContextDrainState,
            IStateReplicator fabricReplicator)
        {
            this.appendCheckpointCallback = appendCheckpointCallback;
            this.RoleContextDrainState = roleContextDrainState;
            this.fabricReplicator = fabricReplicator;
            this.tracer = tracer;
            this.CurrentLogHeadRecord = currentHead;
            this.replicationSerializationBinaryWritersPoolClearTimer.Change(Constants.ReplicationWriteMemoryStreamsBufferPoolCleanupMilliseconds, Timeout.Infinite);
        }

        public long ReplicateAndLog(
            LogicalLogRecord record,
            TransactionManager transactionsManager)
        {
            Replicate(record);
            return Append(record, transactionsManager, isPrimary: true);
        }

        public void Reuse()
        {
            this.ProgressVector = null;
            this.LastLinkedPhysicalRecord = null;
            this.LastInformationRecord = InformationLogRecord.InvalidInformationLogRecord;
            this.CurrentLogTailEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.CurrentLogTailLsn = LogicalSequenceNumber.InvalidLsn;
            this.lastWarningExceptionTraceOnPrimary = Stopwatch.StartNew();
            this.lsnOrderingTcs = null;
            this.operationAcceptedCount = 0;
            this.lsnOrderingQueue = new List<LogicalLogRecord>();
            this.replicationSerializationBinaryWritersPoolClearTimer = new Timer(this.OnReplicationSerializationBinaryWritersPoolCleanupTimer, null, Timeout.Infinite, Timeout.Infinite);
        }

        public void Reuse(
            ProgressVector progressVector,
            EndCheckpointLogRecord lastCompletedEndCheckpointRecord,
            BeginCheckpointLogRecord lastInProgressBeginCheckpointRecord,
            PhysicalLogRecord lastLinkedPhysicalRecord,
            InformationLogRecord lastInformationRecord,
            IndexingLogRecord currentLogHeadRecord,
            Epoch tailEpoch,
            LogicalSequenceNumber tailLsn)
        {
            this.LastInProgressCheckpointRecord = lastInProgressBeginCheckpointRecord;
            Utility.Assert(this.LastInProgressCheckpointRecord == null, "ReInitialize of ReplicatedLogManager must have null in progress checkpoint");
            this.LastInProgressTruncateHeadRecord = null;
            this.ProgressVector = progressVector;
            this.LastCompletedEndCheckpointRecord = lastCompletedEndCheckpointRecord;
            this.LastLinkedPhysicalRecord = lastLinkedPhysicalRecord;
            this.CurrentLogHeadRecord = currentLogHeadRecord;
            this.LastInformationRecord = lastInformationRecord;
            this.CurrentLogTailEpoch = tailEpoch;
            this.CurrentLogTailLsn = tailLsn;
        }

        public void SetTailEpoch(Epoch epoch)
        {
            this.CurrentLogTailEpoch = epoch;
        }

        public void SetTailLsn(LogicalSequenceNumber lsn)
        {
            this.CurrentLogTailLsn = lsn;
        }

        public void Insert_BeginCheckpoint(BeginCheckpointLogRecord record)
        {
            Utility.Assert(this.LastInProgressCheckpointRecord == null, "Inprogress checkpoint must be null");
            this.LastInProgressCheckpointRecord = record;
            this.LogManager.PhysicalLogWriter.InsertBufferedRecord(record);
        }

        public void Test_TruncateHead(
            TruncateHeadLogRecord record,
            IndexingLogRecord headRecord)
        {
            this.LastLinkedPhysicalRecord = record;
            this.CurrentLogHeadRecord = headRecord;
            this.LastInProgressTruncateHeadRecord = record;
            this.LogManager.PhysicalLogWriter.InsertBufferedRecord(record);
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        public void ThrowReplicationException(LogicalLogRecord record, Exception e)
        {
            int innerHResult = 0;
            var flattenedException = Utility.FlattenException(e, out innerHResult);

            Exception notPrimaryException = flattenedException as FabricNotPrimaryException;
            var transientException = flattenedException as FabricTransientException;
            var cancelException = flattenedException as OperationCanceledException;
            var closedException = flattenedException as FabricObjectClosedException;
            var tooLargeException = flattenedException as FabricReplicationOperationTooLargeException;

            var isExpected = notPrimaryException != null || transientException != null || cancelException != null
                             || closedException != null || tooLargeException != null;

            var message =
                string.Format(
                    CultureInfo.InvariantCulture,
                    "ProcessReplicationException" + Environment.NewLine
                    + "      Replication exception. Type: {0} Message: {1} HResult: 0x{2:X8}" + Environment.NewLine
                    + "      Log record. Type: {3} LSN: {4}" + Environment.NewLine,
                    flattenedException.GetType().ToString(),
                    flattenedException.Message,
                    flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                    record.RecordType,
                    record.Lsn.LSN);

            if (isExpected)
            {
                if (this.lastWarningExceptionTraceOnPrimary.ElapsedMilliseconds > PrimaryWarningExceptionTraceIntervalMs)
                {
                    this.lastWarningExceptionTraceOnPrimary.Restart();
                    FabricEvents.Events.Warning(this.tracer.Type, message);
                }
            }
            else if (!this.RoleContextDrainState.IsClosing)
            {
                FabricEvents.Events.Error(this.tracer.Type, message);

                this.RoleContextDrainState.ReportPartitionFault();
            }

            if (closedException != null)
            {
                throw new FabricNotPrimaryException();
            }
            else
            {
                throw flattenedException;
            }
        }
        /// <summary>
        /// Do a check if our log matches truncate head criteria, and append truncate head record
        /// </summary>
        public TruncateHeadLogRecord TruncateHead(
            bool isStable,
            long lastPeriodicTruncationTimeTicks,
            Func<IndexingLogRecord, bool> isGoodLogHeadCandidateCalculator)
        {
            lock (lsnOrderingLock)
            {
                var earliestRecord = this.GetEarliestRecordCallerHoldsLock();

                IndexingLogRecord previousIndexingRecord = null;
                PhysicalLogRecord previousPhysicalRecord = this.LastCompletedBeginCheckpointRecord;

                TruncateHeadLogRecord record = null;

                do
                {
                    // Search for the last Indexing Log Record
                    do
                    {
                        previousPhysicalRecord = previousPhysicalRecord.PreviousPhysicalRecord;
                    } while (!(previousPhysicalRecord is IndexingLogRecord));
                    previousIndexingRecord = previousPhysicalRecord as IndexingLogRecord;

                    // This index record is not before the ealiest pending transaction record
                    if (previousIndexingRecord.RecordPosition >= earliestRecord.RecordPosition)
                    {
                        continue;
                    }

                    // We reached log head, so do not continue to look for last index log record
                    if (previousIndexingRecord == this.CurrentLogHeadRecord)
                    {
                        return null;
                    }

                    if (isGoodLogHeadCandidateCalculator(previousIndexingRecord))
                    {
                        break;
                    }
                } while (true);

                record = new TruncateHeadLogRecord(
                    previousIndexingRecord,
                    this.CurrentLogTailLsn,
                    this.LastLinkedPhysicalRecord,
                    isStable,
                    lastPeriodicTruncationTimeTicks);

                Test_TruncateHead(record, previousIndexingRecord);

                return record;
            }
        }

        public void TruncateTail(LogicalSequenceNumber tailLsn)
        {
            lock (lsnOrderingLock)
            {
                var record = new TruncateTailLogRecord(tailLsn, this.LastLinkedPhysicalRecord);
                this.LogManager.PhysicalLogWriter.InsertBufferedRecord(record);
            }
        }

        public void UpdateEpochRecord(UpdateEpochLogRecord record)
        {
            Utility.Assert(
                this.CurrentLogTailLsn == record.Lsn,
                "this.currentLogTailLsn == record.LastLogicalSequenceNumber");

            FabricEvents.Events.UpdateEpoch(
                this.tracer.Type,
                "UpdateEpochRecord",
                record.Epoch.DataLossNumber,
                record.Epoch.ConfigurationNumber,
                record.Lsn.LSN,
                RoleContextDrainState.ReplicaRole.ToString());

            this.AppendWithoutReplication(
                record,
                null);

            this.ProgressVector.Add(new ProgressVectorEntry(record));
            this.SetTailEpoch(record.Epoch);
        }

        private void AddCheckpointIfNeeded(bool isPrimary)
        {
            var record = this.appendCheckpointCallback(isPrimary);

            if (record == null)
            {
                return;
            }

            Insert_BeginCheckpoint(record);
        }

        private long Append(
            LogicalLogRecord record,
            TransactionManager transactionsManager,
            bool isPrimary)
        {
            long bufferedBytes = 0;
            TransactionLogRecord txLogRecord = record as TransactionLogRecord;
            TaskCompletionSource<object> lsnOrderingTcsToSignal = null;

            if (txLogRecord != null)
            {
                Utility.Assert(transactionsManager != null, "transactionsManager should not be null in AppendOnPrimary");
                transactionsManager.CreateTransactionState(this.CurrentLogTailEpoch, txLogRecord);
            }

            lock (this.lsnOrderingLock)
            {
                bufferedBytes = LsnOrderingInsert(record, isPrimary, out lsnOrderingTcsToSignal);
            }

            if (lsnOrderingTcsToSignal != null)
            {
                // Do this outside the lock above and also after setting the this.lsnorderingtcs to NULL as this set result can end up invoking a completion to run on the same thread,
                // which ends up invoking another call to Append. This happened in a unit test on the UpdateEpoch code path on primary
                bool result = lsnOrderingTcsToSignal.TrySetResult(null);
                Utility.Assert(result == true, "ReplicatedLogManager: failed to set result on lsn ordering tcs");
            }

            return bufferedBytes;
        }

        /// <summary>
        /// Append end checkpoint record
        /// </summary>
        private void EndCheckpointCallerHoldsLock(BeginCheckpointLogRecord lastInProgressCheckpointRecord)
        {
            Utility.Assert(lastInProgressCheckpointRecord != null, "this.lastInProgressCheckpointRecord != null");
            this.LastCompletedEndCheckpointRecord =
                new EndCheckpointLogRecord(
                    lastInProgressCheckpointRecord,
                    this.CurrentLogHeadRecord,
                    this.CurrentLogTailLsn,
                    this.LastLinkedPhysicalRecord);
            this.LastLinkedPhysicalRecord = this.LastCompletedEndCheckpointRecord;
            this.LogManager.PhysicalLogWriter.InsertBufferedRecord(this.LastCompletedEndCheckpointRecord);
        }

        private LogRecord GetEarliestRecordCallerHoldsLock()
        {
            LogRecord earliestRecord = this.LastCompletedBeginCheckpointRecord;

            if (this.LastCompletedBeginCheckpointRecord.EarliestPendingTransaction != null)
            {
                earliestRecord = this.LastCompletedBeginCheckpointRecord.EarliestPendingTransaction;
            }

            Utility.Assert(
                earliestRecord != null,
                "Earliest record should not be null");

            return earliestRecord;
        }

        private long InsertBufferedRecordCallerHoldsLock(
            LogicalLogRecord record,
            bool isPrimary,
            out long pendingCount)
        {
            pendingCount = -1;
            var result = LogManager.PhysicalLogWriter.InsertBufferedRecord(record);

            UpdateEpochLogRecord updateEpochRecord = record as UpdateEpochLogRecord;

            Utility.Assert(
                (this.CurrentLogTailLsn + 1) == record.Lsn || (updateEpochRecord != null && updateEpochRecord.Lsn == this.CurrentLogTailLsn),
                "(this.currentLogTailLsn + 1) {0} == record.LastLogicalSequenceNumber {1}. RecordType {2}",
                this.CurrentLogTailLsn + 1,
                record.Lsn,
                record.RecordType);

            if (updateEpochRecord == null)
            {
                // Increment tail lsn only on appending a logical record that is NOT an update epoch
                ++this.CurrentLogTailLsn;
            }

            if (isPrimary)
            {
                pendingCount = OnOperationLogInitiationCallerHoldsLock();
            }

            var barrierRecord = record as BarrierLogRecord;
            if (barrierRecord != null)
            {
                this.OnBarrierBufferedCallerHoldsLock(barrierRecord, isPrimary);
            }

            return result;
        }

        private long LsnOrderingInsert(
            LogicalLogRecord record,
            bool isPrimary,
            out TaskCompletionSource<object> lsnOrderingTcsToSignal)
        {
            var recordLsn = record.Lsn;
            long pendingCount = -1;
            var i = 0;
            lsnOrderingTcsToSignal = null;

            // First every update epoch record on full copy secondary has UpdateEpoch(0,0)
            Utility.Assert(
                recordLsn > LogicalSequenceNumber.ZeroLsn || record.RecordType == LogRecordType.UpdateEpoch,
                "recordLsn > LogicalSequenceNumber.ZeroLsn || record.RecordType == LogRecordType.UpdateEpoch");

            if (recordLsn > (this.CurrentLogTailLsn + 1))
            {
                for (i = this.lsnOrderingQueue.Count - 1; i >= 0; i--)
                {
                    if (recordLsn > this.lsnOrderingQueue[i].Lsn)
                    {
                        this.lsnOrderingQueue.Insert(i + 1, record);
                        break;
                    }
                }

                if (i == -1)
                {
                    this.lsnOrderingQueue.Insert(0, record);
                }

                return -1;
            }

            var result = InsertBufferedRecordCallerHoldsLock(record, isPrimary, out pendingCount);

            for (i = 0; i < this.lsnOrderingQueue.Count; i++)
            {
                recordLsn = this.lsnOrderingQueue[i].Lsn;
                if (recordLsn > (this.CurrentLogTailLsn + 1))
                {
                    break;
                }

                record = this.lsnOrderingQueue[i];
                result = InsertBufferedRecordCallerHoldsLock(record, isPrimary, out pendingCount);
            }

            this.lsnOrderingQueue.RemoveRange(0, i);

            if (pendingCount == 0 &&                  	    
  	            this.lsnOrderingQueue.Count == 0 && 
                this.lsnOrderingTcs != null)
  	        {
  	            lsnOrderingTcsToSignal = this.lsnOrderingTcs;
  	            this.lsnOrderingTcs = null;
  	        }  	        

            return result;
        }

        private void OnBarrierBufferedCallerHoldsLock(
            BarrierLogRecord record,
            bool isPrimary)
        {
            FabricEvents.Events.AcceptBarrier(this.tracer.Type, record.Lsn.LSN);
            AddCheckpointIfNeeded(isPrimary);
            this.LogManager.FlushAsync("BarrierRecordAppended").IgnoreExceptionVoid();
        }

        private void OnOperationAcceptance()
        {
            Interlocked.Increment(ref this.operationAcceptedCount);
        }

        private long OnOperationLogInitiationCallerHoldsLock()
        {
            var pendingCount = Interlocked.Decrement(ref this.operationAcceptedCount);
            Utility.Assert(pendingCount >= 0, "pendingCount must be >=0. It is {0}", pendingCount);
            
            return pendingCount;
        }

        private void OnOperationAcceptanceException()
        {
            var pendingCount = Interlocked.Decrement(ref this.operationAcceptedCount);
            Utility.Assert(pendingCount >= 0, "pendingCount must be >=0. It is {0}", pendingCount);
            TaskCompletionSource<object> localTcs = null;

            if (pendingCount == 0)
            {
                lock (this.lsnOrderingLock)
                {
                    if (this.lsnOrderingQueue.Count == 0 && this.lsnOrderingTcs != null)
                    {
                        localTcs = this.lsnOrderingTcs;
                        this.lsnOrderingTcs = null;
                    }
                }
            }

            if (localTcs != null)
            {
                // Do this outside the lock above and also after setting the this.lsnorderingtcs to NULL as this set result can end up invoking a completion to run on the same thread,
                // which ends up invoking another call to Append. This happened in a unit test on the UpdateEpoch code path on primary
                bool result = localTcs.TrySetResult(null);
                Utility.Assert(result == true, "ReplicatedLogManager: failed to set result on lsn ordering tcs");
            }
        }

        private void OnReplicationSerializationBinaryWritersPoolCleanupTimer(object sender)
        {
            this.replicationSerializationBinaryWritersPool.OnClear();
        }

        private void Replicate(LogicalLogRecord record)
        {
            Utility.Assert(
                record.Lsn <= LogicalSequenceNumber.ZeroLsn && record.RecordType != LogRecordType.UpdateEpoch,
                "Record Lsn not <=0 during Replicate or record is update epoch. It is {0} for type {1}",
                record.Lsn, record.RecordType);

            if (RoleContextDrainState.ReplicaRole != ReplicaRole.Primary ||
                RoleContextDrainState.IsClosing)
            {
                throw new FabricNotPrimaryException();
            }

            // 1. The binary writer is taken before replication
            // 2. It is released immediately if replication failed (see catch block)
            // 3. If replication is accepted, it is released after the log record is flushed in flushedrecordscallback
            //      This is because the same memory buffers are re-used to write to disk as well to avoid double serialization of logical data

            var bw = this.replicationSerializationBinaryWritersPool.Take();
            var operationData = record.ToOperationData(bw);

#if DEBUG
            ValidateOperationData(operationData, "Replicate data " + record.RecordType);
#endif

            long sequenceNumber;
            try
            {
                this.OnOperationAcceptance();

                // Use the local variable as the member can be set to null
                var replicatorTask = this.fabricReplicator.ReplicateAsync(
                    operationData,
                    CancellationToken.None,
                    out sequenceNumber);
                record.ReplicatorTask = replicatorTask;
                record.Lsn = new LogicalSequenceNumber(sequenceNumber);
                if (sequenceNumber == 0)
                {
                    Utility.Assert(replicatorTask.IsFaulted == true, "replicatorTask.IsFaulted == true");
                    record.InvalidateReplicatedDataBuffers(this.replicationSerializationBinaryWritersPool);
                    this.ThrowReplicationException(record, replicatorTask.Exception);
                }
            }
            catch (Exception e)
            {
                record.InvalidateReplicatedDataBuffers(this.replicationSerializationBinaryWritersPool);
                OnOperationAcceptanceException();

                this.ThrowReplicationException(record, e);
            }
        }
    }
}