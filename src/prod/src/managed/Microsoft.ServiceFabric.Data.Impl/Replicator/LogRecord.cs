// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Naming conventions used for log contents from http://en.wikipedia.org/wiki/Transaction_log 
    /// </summary>
    internal enum LogRecordType : uint
    {
        Invalid = 0,

        // Logical log records
        BeginTransaction = 1,

        Operation = 2,

        EndTransaction = 3,

        Barrier = 4,

        UpdateEpoch = 5,

        Backup = 6,

        // Physical log records
        BeginCheckpoint = 7,

        EndCheckpoint = 8,

        Indexing = 9,

        TruncateHead = 10,

        TruncateTail = 11,

        Information = 12,

        CompleteCheckpoint = 13,
    }

    internal class LogRecord : IEquatable<LogRecord>
    {
        internal const ulong InvalidPhysicalRecordOffset = ulong.MaxValue;

        internal const uint InvalidRecordLength = uint.MaxValue;

        internal const ulong InvalidRecordPosition = ulong.MaxValue;

        internal static readonly LogRecord InvalidLogRecord = new LogRecord();

        private static readonly uint ApproximateDiskSpaceUsed = (2*sizeof(int)) + (2*sizeof(long)) + sizeof(int) + sizeof(ulong);

        private CompletionTask appliedTask;

        private CompletionTask flushedTask;

        private CompletionTask invalidCompletionTask = new CompletionTask();

        private LogicalSequenceNumber lsn;

        private PhysicalLogRecord previousPhysicalRecord;

        private ulong previousPhysicalRecordOffset;

        private CompletionTask processedTask;

        // The following are physical fields
        private PhysicalSequenceNumber psn;

        // The following are logical fields
        private uint recordLength;

        // The following fields are not persisted
        private ulong recordPosition;

        private LogRecordType recordType;

        protected LogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
        {
            this.recordType = recordType;
            this.lsn = new LogicalSequenceNumber(lsn);
            this.psn = PhysicalSequenceNumber.InvalidPsn;
            this.previousPhysicalRecordOffset = InvalidPhysicalRecordOffset;

            this.recordPosition = recordPosition;
            this.recordLength = InvalidRecordLength;
            this.previousPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;
            this.flushedTask = new CompletionTask();
            this.appliedTask = new CompletionTask();
            this.processedTask = new CompletionTask();

            this.ApproximateSizeOnDisk = 0;
        }

        protected LogRecord()
        {
            this.recordType = LogRecordType.Invalid;
            this.lsn = LogicalSequenceNumber.InvalidLsn;
            this.psn = PhysicalSequenceNumber.InvalidPsn;
            this.previousPhysicalRecordOffset = InvalidPhysicalRecordOffset;

            this.recordPosition = InvalidRecordPosition;
            this.recordLength = InvalidRecordLength;
            this.previousPhysicalRecord = null;
            this.flushedTask = this.invalidCompletionTask;
            this.appliedTask = this.invalidCompletionTask;
            this.processedTask = this.invalidCompletionTask;

            this.ApproximateSizeOnDisk = 0;
        }

        protected LogRecord(LogRecordType recordType)
        {
            this.recordType = recordType;
            this.lsn = LogicalSequenceNumber.InvalidLsn;
            this.psn = PhysicalSequenceNumber.InvalidPsn;
            this.previousPhysicalRecordOffset = InvalidPhysicalRecordOffset;

            this.recordLength = InvalidRecordLength;
            this.recordPosition = InvalidRecordPosition;
            this.previousPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;
            this.flushedTask = new CompletionTask();
            this.appliedTask = new CompletionTask();
            this.processedTask = new CompletionTask();

            this.UpdateApproximateDiskSize();
        }

        public uint ApproximateSizeOnDisk { get; protected set; }

        /// <summary>
        /// This is read by the LogRecordsDispatcher to set BarrierTask.
        /// </summary>
        internal CompletionTask AppliedTask
        {
            get { return this.appliedTask; }
        }

        internal Exception FlushException
        {
            get { return this.flushedTask.CompletedException; }
        }

        internal bool IsApplied
        {
            get { return this.appliedTask.IsCompleted; }
        }

        internal bool IsFlushed
        {
            get { return this.flushedTask.IsCompleted; }
        }

        internal bool IsProcessed
        {
            get { return this.processedTask.IsCompleted; }
        }

        internal LogicalSequenceNumber Lsn
        {
            get { return this.lsn; }

            set { this.lsn = value; }
        }

        internal PhysicalLogRecord PreviousPhysicalRecord
        {
            get { return this.previousPhysicalRecord; }

            set
            {
                Utility.Assert(
                    (this.previousPhysicalRecord == PhysicalLogRecord.InvalidPhysicalLogRecord)
                    || (this.previousPhysicalRecord == value),
                    "(this.previousPhysicalRecord == PhysicalLogRecord.InvalidPhysicalLogRecord) ||" + "(this.previousPhysicalRecord == value)");

                this.previousPhysicalRecord = value;
            }
        }

        internal ulong PreviousPhysicalRecordOffset
        {
            get { return this.previousPhysicalRecordOffset; }
        }

        internal PhysicalSequenceNumber Psn
        {
            get { return this.psn; }

            set { this.psn = value; }
        }

        internal uint RecordLength
        {
            get { return this.recordLength; }

            set
            {
                if (this.recordLength == InvalidRecordLength)
                {
                    this.recordLength = value;
                    return;
                }

                // Currently we use the same encoding everywhere
                Utility.Assert(
                    this.recordLength == value,
                    "this.recordLength ({0}) == value ({1})",
                    this.recordLength, value);
            }
        }

        /// <summary>
        /// Position of the record in byte
        /// </summary>
        internal ulong RecordPosition
        {
            get { return this.recordPosition; }

            set
            {
                Utility.Assert(
                    this.recordPosition == InvalidRecordPosition,
                    "this.recordPosition == LogRecord.INVALID_RECORD_POSITION");

                this.recordPosition = value;
            }
        }

        internal uint RecordSize
        {
            get
            {
                Utility.Assert(this.recordLength > 0, "this.recordLength > 0");
                return this.recordLength + (2*sizeof(uint));
            }
        }

        internal LogRecordType RecordType
        {
            get { return this.recordType; }
        }

        /// <summary>
        /// Checks equality.
        /// </summary>
        /// <param name="other">Other LogRecord.</param>
        /// <returns>True or false</returns>
        public virtual bool Equals(LogRecord other)
        {
            // Assumption: If two records have the same lsn and psn, they must be the same record.
            // RecordType here is a double check.
            return this.recordType == other.recordType && this.lsn == other.lsn && this.psn == other.psn;
        }

        /// <summary>
        /// Overriding the object Equals.
        /// </summary>
        /// <param name="obj"></param>
        /// <returns>Boolean indicating whether the two objects are same.</returns>
        public override bool Equals(object obj)
        {
            var other = obj as LogRecord;

            if (other == null)
            {
                return false;
            }

            return this.Equals(other);
        }

        /// <summary>
        /// Gets the hash code.
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            var result = this.recordType.GetHashCode() ^ this.lsn.GetHashCode() ^ this.psn.GetHashCode();

            return result;
        }

        internal static LogRecord FromOperationData(OperationData operationData)
        {
            return ReadFromOperationData(operationData);
        }

        internal static bool IsInvalidRecord(LogRecord record)
        {
            return (record != null) && (record.recordType == LogRecordType.Invalid);
        }

        /// <summary>
        /// Read the next record in the stream asynchronously.
        /// </summary>
        /// <param name="stream">Stream to read from.</param>
        /// <param name="isPhysicalRead">Is this a physical read.</param>
        /// <param name="useInvalidRecordPosition">Should record be created with invalid record position.</param>
        /// <param name="setRecordLength">Should the record length be set.</param>
        /// <returns>
        /// Task that represents the asynchronous stream read.
        /// Result is the log record.
        /// </returns>
        internal static async Task<LogRecord> ReadNextRecordAsync(
            Stream stream,
            bool isPhysicalRead = true,
            bool useInvalidRecordPosition = false,
            bool setRecordLength = true)
        {
            var recordPosition = (ulong) stream.Position;

            // Read next record length
            var buffer = new byte[sizeof(uint)];
            await Utility.ReadAsync(stream, buffer).ConfigureAwait(false);
            uint recordLength = 0;
            using (var br = new BinaryReader(new MemoryStream(buffer)))
            {
                recordLength = br.ReadUInt32();
            }

            // Read record bytes
            buffer = new byte[recordLength + sizeof(uint)];
            await Utility.ReadAsync(stream, buffer).ConfigureAwait(false);

            // Convert record bytes into record
            using (var br = new BinaryReader(new MemoryStream(buffer)))
            {
                if (true == useInvalidRecordPosition)
                {
                    recordPosition = InvalidRecordPosition;
                }

                return ReadRecordWithHeaders(br, recordPosition, isPhysicalRead, setRecordLength);
            }
        }

        internal static async Task<LogRecord> ReadPreviousRecordAsync(Stream stream)
        {
            // Record previous record length
            var readPosition = ((ulong) stream.Position) - sizeof(uint);
            stream.Position = (long) readPosition;
            var buffer = new byte[sizeof(uint)];
            await Utility.ReadAsync(stream, buffer).ConfigureAwait(false);

            uint recordLength = 0;
            using (var br = new BinaryReader(new MemoryStream(buffer)))
            {
                recordLength = br.ReadUInt32();
            }

            // Read record bytes
            readPosition = readPosition - recordLength;
            stream.Position = (long) readPosition;
            buffer = new byte[recordLength + sizeof(uint)];
            await Utility.ReadAsync(stream, buffer).ConfigureAwait(false);

            // Convert record bytes into record
            var recordPosition = readPosition - sizeof(uint);

            using (var br = new BinaryReader(new MemoryStream(buffer)))
            {
                return ReadRecordWithHeaders(br, recordPosition);
            }
        }

        internal static OperationData WriteRecord(
            LogRecord record,
            BinaryWriter bw,
            bool isPhysicalWrite = true,
            bool setRecordLength = true,
            bool forceRecomputeOffsets = false)
        {
            // NOTE:- The binary writer is not where the real data is written
            // It is only passed in to avoid each log record from creating its own writer

            // The real data of the log record is returned in the operation data
            // As a result, reset the position of the writer before starting to write

            bw.BaseStream.Position = 0;

            var operationData = new OperationData();
            record.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);
            uint recordLength = 0;

            foreach (var data in operationData)
            {
                recordLength += (uint) data.Count;
            }

            if (setRecordLength)
            {
                record.RecordLength = recordLength;
            }

            var mm = bw.BaseStream as MemoryStream;
            var startingPosition = bw.BaseStream.Position;
            bw.Write(recordLength);
            var arraySegment = new ArraySegment<byte>(
                mm.GetBuffer(),
                (int) startingPosition,
                (int) mm.Position - (int) startingPosition);

            // Append and prepend record length in a fixed encoding of sizeof(int) bytes
            operationData.Add(arraySegment);
            operationData.Insert(0, arraySegment);

            return operationData;
        }

        internal Task AwaitApply()
        {
            return this.appliedTask.AwaitCompletion();
        }

        internal async Task AwaitFlush()
        {
            await this.flushedTask.AwaitCompletion().ConfigureAwait(false);

            return;
        }

        internal async Task AwaitProcessing()
        {
            await this.processedTask.AwaitCompletion().ConfigureAwait(false);

            return;
        }

        internal void CompletedApply(Exception applyException)
        {
            this.appliedTask.CompleteAwaiters(applyException);

            return;
        }

        internal void CompletedFlush(Exception flushException)
        {
            this.flushedTask.CompleteAwaiters(flushException);

            return;
        }

        internal void CompletedProcessing(Exception processingException)
        {
            this.processedTask.CompleteAwaiters(processingException);

            return;
        }

        internal virtual bool FreePreviousLinksLowerThanPsn(PhysicalSequenceNumber newHeadPsn)
        {
            if (this.previousPhysicalRecord != null &&
                this.previousPhysicalRecord.Psn < newHeadPsn)
            {
                Utility.Assert(
                    this.previousPhysicalRecordOffset != InvalidPhysicalRecordOffset,
                    "FreePreviousLinksLowerThanPsn: PreviousPhysicalRecordOffset is invalid. Record LSN is {0}",
                    this.lsn.LSN);

                this.previousPhysicalRecord = (this.previousPhysicalRecordOffset == 0)
                    ? null
                    : PhysicalLogRecord.InvalidPhysicalLogRecord;
            }

            return false;
        }

        protected internal static ArraySegment<byte> CreateArraySegment(long startingPosition, BinaryWriter bw)
        {
            var mm = bw.BaseStream as MemoryStream;

            int startingPos = checked ((int) startingPosition);
            int count = checked ((int) mm.Position - startingPos);

            var data = new ArraySegment<byte>(
                mm.GetBuffer(),
                startingPos,
                count);

            return data;
        }

        protected static ArraySegment<byte> IncrementIndexAndGetArraySegmentAt(
            OperationData operationData,
            ref int index)
        {
            index += 1;
            return operationData[index];
        }

        protected static MemoryStream IncrementIndexAndGetMemoryStreamAt(OperationData operationData, ref int index)
        {
            index += 1;
            return new MemoryStream(operationData[index].Array, operationData[index].Offset, operationData[index].Count);
        }

        protected virtual void OnCompletedFlushing()
        {
            return;
        }

        protected virtual void Read(BinaryReader br, bool isPhysical)
        {
            if (isPhysical)
            {
                // Metadata section.
                var startingPosition = br.BaseStream.Position;
                var sizeOfSection = br.ReadInt32();
                var endPosition = startingPosition + sizeOfSection;

                // Read Physical metadata section
                this.psn = new PhysicalSequenceNumber(br.ReadInt64());
                this.previousPhysicalRecordOffset = br.ReadUInt64();

                // Jump to the end of the section ignoring fields that are not understood.
                Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
                br.BaseStream.Position = endPosition;

                if (this.previousPhysicalRecordOffset == 0)
                {
                    this.previousPhysicalRecord = null;
                }
            }
        }

        protected virtual void ReadLogical(OperationData operationData, ref int index)
        {
        }

        protected virtual void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            // NOTE - LogicalData optimization is NOT done here unlike other log records because the lsn is not yet assigned at this point.
            // Logical Metadata section.
            var logicalStartingPosition = bw.BaseStream.Position;
            bw.BaseStream.Position += sizeof(int);

            // Logical Metadata fields
            bw.Write((uint) this.recordType);
            bw.Write(this.lsn.LSN);

            // End Logical Metadata section
            var logicalEndPosition = bw.BaseStream.Position;

            var logicalSizeOfSection = checked((int) (logicalEndPosition - logicalStartingPosition));
            bw.BaseStream.Position = logicalStartingPosition;
            bw.Write(logicalSizeOfSection);
            bw.BaseStream.Position = logicalEndPosition;

            operationData.Add(CreateArraySegment(logicalStartingPosition, bw));

            if (isPhysicalWrite)
            {
                // Physical Metadata section.
                var physicalStartPosition = bw.BaseStream.Position;
                bw.BaseStream.Position += sizeof(int);

                bw.Write(this.psn.PSN);

                Utility.Assert(
                    this.recordPosition != InvalidRecordPosition,
                    "this.recordPosition != LogRecord.INVALID_RECORD_POSITION");

                if (this.previousPhysicalRecordOffset == InvalidPhysicalRecordOffset)
                {
                    Utility.Assert(
                        this.previousPhysicalRecord != PhysicalLogRecord.InvalidPhysicalLogRecord,
                        "this.previousPhysicalRecord != PhysicalLogRecord.InvalidPhysicalLogRecord");

                    if (this.previousPhysicalRecord == null)
                    {
                        this.previousPhysicalRecordOffset = 0;
                    }
                    else
                    {
                        Utility.Assert(
                            this.previousPhysicalRecord.recordPosition != InvalidRecordPosition,
                            "this.previousPhysicalRecord.recordPosition != LogRecord.INVALID_RECORD_POSITION");

                        Utility.Assert(
                            this.recordPosition != InvalidRecordPosition,
                            "this.recordPosition != LogRecord.INVALID_RECORD_POSITION");

                        this.previousPhysicalRecordOffset = this.recordPosition
                                                            - this.previousPhysicalRecord.recordPosition;
                    }
                }
                else
                {
                    Utility.CodingError(
                        "Record written twice. Record Type:{0} RecordPosition={1}",
                        this.recordType,
                        this.recordPosition);
                }

                bw.Write(this.previousPhysicalRecordOffset);

                // End Logical Metadata section
                var physicalEndPosition = bw.BaseStream.Position;
                var physicalSizeOfSection = checked((int) (physicalEndPosition - physicalStartPosition));
                bw.BaseStream.Position = physicalStartPosition;
                bw.Write(physicalSizeOfSection);
                bw.BaseStream.Position = physicalEndPosition;

                operationData.Add(CreateArraySegment(physicalStartPosition, bw));
            }

            return;
        }

        private static LogRecord ReadFromOperationData(OperationData operationData)
        {
            LogRecord record;
            long lsn;
            const ulong RecordPosition = InvalidRecordPosition;
            LogRecordType recordType;
            var index = -1;

            using (var reader = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                // Logical metadata section.
                var startingPosition = reader.BaseStream.Position;
                var sizeOfSection = reader.ReadInt32();
                var endPosition = startingPosition + sizeOfSection;

                // Logical metadata read.
                recordType = (LogRecordType) reader.ReadUInt32();
                lsn = reader.ReadInt64();

                // Jump to the end of the section ignoring fields that are not understood.
                Utility.Assert(endPosition >= reader.BaseStream.Position, "Could not have read more than section size.");
                reader.BaseStream.Position = endPosition;
            }

            switch (recordType)
            {
                case LogRecordType.BeginTransaction:
                    record = new BeginTransactionOperationLogRecord(recordType, RecordPosition, lsn);
                    break;

                case LogRecordType.Operation:
                    record = new OperationLogRecord(recordType, RecordPosition, lsn);
                    break;

                case LogRecordType.EndTransaction:
                    record = new EndTransactionLogRecord(recordType, RecordPosition, lsn);
                    break;

                case LogRecordType.Barrier:
                    record = new BarrierLogRecord(recordType, RecordPosition, lsn);
                    break;

                case LogRecordType.UpdateEpoch:
                    record = new UpdateEpochLogRecord(recordType, RecordPosition, lsn);
                    break;

                case LogRecordType.Backup:
                    record = new BackupLogRecord(recordType, RecordPosition, lsn);
                    break;

                default:
                    Utility.CodingError(
                        "Unexpected record type received during replication/copy processing {0}",
                        recordType);
                    return null;
            }

            record.ReadLogical(operationData, ref index);
            return record;
        }

        private static LogRecord ReadRecord(BinaryReader br, ulong recordPosition, bool isPhysicalRead)
        {
            LogRecord record;
            var lsn = LogicalSequenceNumber.InvalidLsn.LSN;
            LogRecordType recordType;

            // Metadata section.
            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            // Read Logical Metadata
            recordType = (LogRecordType) br.ReadUInt32();

            switch (recordType)
            {
                case LogRecordType.BeginTransaction:
                    record = new BeginTransactionOperationLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.Operation:
                    record = new OperationLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.EndTransaction:
                    record = new EndTransactionLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.Barrier:
                    record = new BarrierLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.UpdateEpoch:
                    record = new UpdateEpochLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.Backup:
                    record = new BackupLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.BeginCheckpoint:
                    record = new BeginCheckpointLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.EndCheckpoint:
                    record = new EndCheckpointLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.Indexing:
                    record = new IndexingLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.TruncateHead:
                    record = new TruncateHeadLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.TruncateTail:
                    record = new TruncateTailLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.Information:
                    record = new InformationLogRecord(recordType, recordPosition, lsn);
                    break;

                case LogRecordType.CompleteCheckpoint:
                    record = new CompleteCheckpointLogRecord(recordType, recordPosition, lsn);
                    break;

                default:
                    Utility.CodingError("Unexpected record type {0}", recordType);
                    return null;
            }

            record.lsn = new LogicalSequenceNumber(br.ReadInt64());

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            record.Read(br, isPhysicalRead);

            return record;
        }

        /// <summary>
        /// Reads the record from the binary reader.
        /// </summary>
        /// <param name="br">Binary Reader to read the record from.</param>
        /// <param name="recordPosition">Position of the log record.</param>
        /// <param name="isPhysicalRead">Is this a physical read.</param>
        /// <param name="setRecordLength">Should the record length be set.</param>
        /// <returns>
        /// Reads the log record from binary record stream.
        /// </returns>
        private static LogRecord ReadRecordWithHeaders(
            BinaryReader br,
            ulong recordPosition,
            bool isPhysicalRead = true,
            bool setRecordLength = true)
        {
            var record = ReadRecord(br, recordPosition, isPhysicalRead);

            // Read record length that is fixed encoded into sizeof(int) bytes
            var recordLength = br.ReadUInt32();
            Utility.Assert(
                recordLength == (((MemoryStream) br.BaseStream).Length - sizeof(int)),
                "recordLength == ((((MemoryStream) br.BaseStream).Length) - sizeof(int))");

            if (setRecordLength)
            {
                record.RecordLength = recordLength;
            }

            return record;
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Too many perf counter classes")
    ]
    internal class CompletionTask
    {
        private static readonly TaskCompletionSource<object> CompletedTcs;

        private static readonly TaskCompletionSource<object> InvalidTcs;

        private TaskCompletionSource<object> completionTcs;

        private TaskCompletionSource<object> waiterTcs;

        static CompletionTask()
        {
            InvalidTcs = new TaskCompletionSource<object>();
            InvalidTcs.SetException(new InvalidOperationException("Awaited on an invalid TCS"));

            CompletedTcs = new TaskCompletionSource<object>();
            CompletedTcs.SetResult(null);
        }

        internal CompletionTask()
        {
            this.completionTcs = InvalidTcs;
            this.waiterTcs = InvalidTcs;
        }

        internal Exception CompletedException
        {
            get
            {
                var currentTcs = this.completionTcs;
                if (currentTcs == InvalidTcs)
                {
                    return null;
                }

                return currentTcs.Task.Exception;
            }
        }

        internal bool IsCompleted
        {
            get
            {
                var currentTcs = this.completionTcs;
                if (currentTcs == InvalidTcs)
                {
                    return false;
                }

                return currentTcs.Task.IsCompleted;
            }
        }

        internal Task AwaitCompletion()
        {
            if (this.completionTcs == InvalidTcs)
            {
                var newTcs = new TaskCompletionSource<object>();
                Interlocked.CompareExchange<TaskCompletionSource<object>>(ref this.completionTcs, newTcs, InvalidTcs);
            }

            if (this.completionTcs.Task.IsCompleted)
            {
                return this.completionTcs.Task;
            }

            var isWaiterCreator = false;
            if (this.waiterTcs == InvalidTcs)
            {
                var newTcs = new TaskCompletionSource<object>();
                var previousWaiterTcs =
                    Interlocked.CompareExchange<TaskCompletionSource<object>>(ref this.waiterTcs, newTcs, InvalidTcs);

                if (previousWaiterTcs == InvalidTcs)
                {
                    isWaiterCreator = true;
                }
            }

            if (isWaiterCreator == false)
            {
                return this.waiterTcs.Task;
            }

            // Note: This is a workaround for the .Result in TStore CreateEnumerable.
            // Should be reverted once that is fixed.
            this.completionTcs.Task.ContinueWith(
                task => Task.Run(
                    () =>
                    {
                        if (task.IsFaulted)
                        {
                            this.waiterTcs.SetException(task.Exception);
                        }
                        else if (task.IsCanceled)
                        {
                            // This is for future proofing.
                            this.waiterTcs.SetCanceled();
                        }
                        else
                        {
                            this.waiterTcs.SetResult(null);
                        }
                    }),
                TaskContinuationOptions.ExecuteSynchronously);

            return this.waiterTcs.Task;
        }

        internal bool CompleteAwaiters(Exception exception)
        {
            var currentTcs = this.completionTcs;
            var isWaiterPresent = currentTcs != InvalidTcs;
            if (isWaiterPresent == false)
            {
                var newTcs = (exception == null)
                    ? CompletedTcs
                    : new TaskCompletionSource<object>();
                currentTcs = Interlocked.CompareExchange<TaskCompletionSource<object>>(
                    ref this.completionTcs,
                    newTcs,
                    InvalidTcs);
                isWaiterPresent = currentTcs != InvalidTcs;
                if (isWaiterPresent == false)
                {
                    if (exception == null)
                    {
                        return false;
                    }

                    currentTcs = newTcs;
                }
            }

            Utility.Assert(currentTcs != CompletedTcs, "currentTcs != CompletedTcs");
            Utility.Assert(currentTcs != InvalidTcs, "currentTcs != InvalidTcs");

            if (exception == null)
            {
                currentTcs.SetResult(null);
            }
            else
            {
                // If there are no waiters listening and we need to set an exception, lets observe it to not generate un-observed task exceptions when this object is GC'd
                if (isWaiterPresent == false)
                {
                    currentTcs.Task.IgnoreExceptionVoid();
                }

                currentTcs.SetException(exception);
            }

            return isWaiterPresent;
        }
    }
}