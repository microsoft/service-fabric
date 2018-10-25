// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal enum CheckpointState : int
    {
        Invalid = 0,

        Ready = 1,

        Applied = 2,

        Completed = 3,

        Faulted = 4,

        Aborted = 5,
    }

    internal sealed class BeginCheckpointLogRecord : PhysicalLogRecord
    {
        internal static readonly BeginCheckpointLogRecord InvalidBeginCheckpointLogRecord =
            new BeginCheckpointLogRecord();

        // BackupLogRecord counts for the count.
        private static readonly uint ApproximateDiskSpaceUsed = BackupLogRecord.ApproximateDiskSpaceUsed + (4*sizeof(long)) + sizeof(ulong);

        /// <summary>
        /// Id of the backup.
        /// </summary>
        private Guid backupId;

        /// <summary>
        /// Number of log records that has been backed up.
        /// </summary>
        /// <remarks>Used for deciding whether incremental backup can be allowed.</remarks>
        private uint backupLogRecordCount;

        /// <summary>
        /// Current log backupLogSize of the backup log in KB.
        /// </summary>
        private uint backupLogSize;

        private CheckpointState checkpointState;

        private Epoch epoch;

        // The following fields are not persisted
        private BeginTransactionOperationLogRecord earliestPendingTransaction;

        private ulong earliestPendingTransactionOffset;

        /// <summary>
        /// Epoch of the highest log record that is backed up.
        /// </summary>
        /// <remarks>This will be used to decide whether incremental backup is possible.</remarks>
        private Epoch highestBackedUpEpoch;

        /// <summary>
        /// Highest logical sequence number that has been backed up.
        /// </summary>
        /// <remarks>This will be used to decide whether incremental backup is possible.</remarks>
        private LogicalSequenceNumber highestBackedUpLsn;

        private LogicalSequenceNumber lastStableLsn;

        private ProgressVector progressVector;
        
        // Phase1 for first checkpoint on full copy on idle is when prepare and perform checkpoint are completed.
        // Phase2 is when copy pump drains the last lsn from the v1 repl and invokes complete checkpoint along with log rename
        private TaskCompletionSource<object> firstCheckpointPhase1CompletionTcs = new TaskCompletionSource<object>();

        // Once the earliest pending tx chain is released due to a truncation of the head, we mark this flag
        // Anyone trying to access this later can get incorrect results and should hence assert
        private long earliestPendingTransactionInvalidated;

        /// <summary>
        /// Timestamp of the last periodic checkpoint. 
        /// </summary>
        private long lastPeriodicCheckpointTimeTicks;

        /// <summary>
        /// Timestamp of the last periodic truncation
        /// </summary>
        private long lastPeriodicTruncationTimeTicks;

        /// <summary>
        /// Initializes a new instance of the BeginCheckpointLogRecord class.
        /// </summary>
        /// <param name="recordType">The record type for double checking.</param>
        /// <param name="recordPosition">The record position.</param>
        /// <param name="lsn"></param>
        /// <remarks>Called on the read path to initialize the log record who will read the binary reader.</remarks>
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal BeginCheckpointLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(
                recordType == LogRecordType.BeginCheckpoint,
                "Record type is expected to be begin checkpoint  but the record type is : {0}",
                recordType);
            this.IsFirstCheckpointOnFullCopy = false;
            this.progressVector = null;
            this.earliestPendingTransactionOffset = LogicalLogRecord.InvalidLogicalRecordOffset;
            this.earliestPendingTransaction = BeginTransactionOperationLogRecord.InvalidBeginTransactionLogRecord;
            this.checkpointState = CheckpointState.Invalid;
            this.lastStableLsn = LogicalSequenceNumber.InvalidLsn;
            this.epoch = LogicalSequenceNumber.InvalidEpoch;

            // Initializes the backup information to invalid.
            this.backupId = BackupLogRecord.InvalidBackupId;
            this.highestBackedUpEpoch = new Epoch(
                LogicalSequenceNumber.InvalidLsn.LSN,
                LogicalSequenceNumber.InvalidLsn.LSN);
            this.highestBackedUpLsn = LogicalSequenceNumber.InvalidLsn;

            // Uint.MaxValue is used to indicate invalid. 4,294,967,295 log records, 4.294967295 TB. 
            this.backupLogRecordCount = uint.MaxValue;
            this.backupLogSize = uint.MaxValue;

            this.earliestPendingTransactionInvalidated = 0;

            this.lastPeriodicCheckpointTimeTicks = 0;
            this.lastPeriodicTruncationTimeTicks = 0;
        }

        /// <summary>
        /// Initializes a new instance of the BeginCheckpointLogRecord class.
        /// </summary>
        /// <remarks>Called when the replicator decides to checkpoint.</remarks>
        internal BeginCheckpointLogRecord(
            bool isFirstCheckpointOnFullCopy,
            ProgressVector progressVector,
            BeginTransactionOperationLogRecord earliestPendingTransaction,
            Epoch headEpoch,
            Epoch epoch,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord lastLinkedPhysicalRecord,
            BackupLogRecord lastCompletedBackupLogRecord,
            uint progressVectorMaxEntries, 
            long periodicCheckpointTimeTicks,
            long periodicTruncationTimeTicks)
            : base(LogRecordType.BeginCheckpoint, lsn, lastLinkedPhysicalRecord)
        {
            this.IsFirstCheckpointOnFullCopy = isFirstCheckpointOnFullCopy;
            this.progressVector = ProgressVector.Clone(progressVector, progressVectorMaxEntries, lastCompletedBackupLogRecord.HighestBackedUpEpoch, headEpoch);

            this.earliestPendingTransactionOffset = LogicalLogRecord.InvalidLogicalRecordOffset;
            this.earliestPendingTransaction = earliestPendingTransaction;
            this.checkpointState = CheckpointState.Invalid;
            this.lastStableLsn = LogicalSequenceNumber.InvalidLsn;
            this.epoch = (earliestPendingTransaction != null) ? earliestPendingTransaction.RecordEpoch : epoch;

            // Initialize backup log record fields.
            this.highestBackedUpEpoch = lastCompletedBackupLogRecord.HighestBackedUpEpoch;
            this.highestBackedUpLsn = lastCompletedBackupLogRecord.HighestBackedUpLsn;

            this.backupLogRecordCount = lastCompletedBackupLogRecord.BackupLogRecordCount;
            this.backupLogSize = lastCompletedBackupLogRecord.BackupLogSizeInKB;

            this.earliestPendingTransactionInvalidated = 0;

            this.lastPeriodicCheckpointTimeTicks = periodicCheckpointTimeTicks;
            this.lastPeriodicTruncationTimeTicks = periodicTruncationTimeTicks;
            this.UpdateApproximateDiskSize();
        }

        /// <summary>
        /// Initializes a new instance of the BeginCheckpointLogRecord class.
        /// </summary>
        /// <remarks>Only used for generating invalid BeginCheckpointLogRecord.</remarks>
        private BeginCheckpointLogRecord()
        {
            this.IsFirstCheckpointOnFullCopy = false;
            this.progressVector = null;
            this.earliestPendingTransactionOffset = LogicalLogRecord.InvalidLogicalRecordOffset;
            this.earliestPendingTransaction = BeginTransactionOperationLogRecord.InvalidBeginTransactionLogRecord;
            this.checkpointState = CheckpointState.Invalid;
            this.lastStableLsn = LogicalSequenceNumber.InvalidLsn;
            this.epoch = LogicalSequenceNumber.InvalidEpoch;

            // Initializes the backup information to invalid.
            this.highestBackedUpEpoch = new Epoch(
                LogicalSequenceNumber.InvalidLsn.LSN,
                LogicalSequenceNumber.InvalidLsn.LSN);
            this.highestBackedUpLsn = LogicalSequenceNumber.InvalidLsn;

            // Uint.MaxValue is used to indicate invalid. 4,294,967,295 log records, 4.294967295 TB. 
            this.backupLogRecordCount = uint.MaxValue;
            this.backupLogSize = uint.MaxValue;

            this.earliestPendingTransactionInvalidated = 0;

            this.lastPeriodicCheckpointTimeTicks = 0;
            this.lastPeriodicTruncationTimeTicks = 0;
        }

        /// <summary>
        /// Initializes a new instance of the BeginCheckpointLogRecord class.
        /// </summary>
        /// <param name="dummy">Used to indicate that this is not an Invalid BeginCheckpointLogRecord.</param>
        private BeginCheckpointLogRecord(bool dummy)
            : base(LogRecordType.BeginCheckpoint, LogicalSequenceNumber.ZeroLsn, null)
        {
            this.IsFirstCheckpointOnFullCopy = false;
            this.progressVector = ProgressVector.Clone(ProgressVector.ZeroProgressVector, 0, LogicalSequenceNumber.ZeroEpoch, LogicalSequenceNumber.ZeroEpoch);
            this.earliestPendingTransactionOffset = 0;
            this.earliestPendingTransaction = null;
            this.checkpointState = CheckpointState.Completed;
            this.lastStableLsn = LogicalSequenceNumber.ZeroLsn;
            this.epoch = LogicalSequenceNumber.ZeroEpoch;

            // Indicates that a full backup has not been taken yet.
            this.highestBackedUpEpoch = LogicalSequenceNumber.ZeroEpoch;
            this.highestBackedUpLsn = LogicalSequenceNumber.ZeroLsn;

            // Indicates that the current backup stream has zero logs and hence 0 KB size.
            this.backupLogRecordCount = (uint) 0;
            this.backupLogSize = (uint) 0;

            this.earliestPendingTransactionInvalidated = 0;
            this.lastPeriodicCheckpointTimeTicks = DateTime.Now.Ticks;
            this.lastPeriodicTruncationTimeTicks = this.lastPeriodicCheckpointTimeTicks;
        }

        /// <summary>
        /// Indicates if this is the first checkpoint on a full copy
        /// </summary>
        public bool IsFirstCheckpointOnFullCopy
        {
            get; private set;
        }

        /// <summary>
        /// Number of log records that has been backed up.
        /// </summary>
        /// <remarks>Used for deciding whether incremental backup can be allowed.</remarks>
        public uint BackupLogRecordCount
        {
            get { return this.backupLogRecordCount; }
        }

        /// <summary>
        /// Current log backupLogSize of the backup log in KB.
        /// </summary>
        public uint BackupLogSize
        {
            get { return this.backupLogSize; }
        }

        /// <summary>
        /// Epoch of the highest log record that is backed up.
        /// </summary>
        /// <remarks>This will be used to decide whether incremental backup is possible.</remarks>
        public Epoch HighestBackedUpEpoch
        {
            get { return this.highestBackedUpEpoch; }
        }

        /// <summary>
        /// Gets the highest logical sequence number that has been backed up.
        /// </summary>
        public LogicalSequenceNumber HighestBackedUpLsn
        {
            get { return this.highestBackedUpLsn; }
        }

        /// <summary>
        /// Epoch of the highest log record that is backed up.
        /// </summary>
        /// <remarks>This will be used to decide whether incremental backup is possible.</remarks>
        public Guid LastBackupId
        {
            get { return this.backupId; }
        }

        /// <summary>
        /// Last periodic checkpoint timestamp in tick format
        /// </summary>
        public long PeriodicCheckpointTimeTicks
        {
            get { return this.lastPeriodicCheckpointTimeTicks; }
        }

        /// <summary>
        /// Last periodic truncation timestamp in tick format
        /// </summary>
        public long PeriodicTruncationTimeTicks
        {
            get { return this.lastPeriodicTruncationTimeTicks; }
        }

        internal override bool FreePreviousLinksLowerThanPsn(PhysicalSequenceNumber newHeadPsn)
        {
            bool ret = base.FreePreviousLinksLowerThanPsn(newHeadPsn);

            if (this.earliestPendingTransaction != null &&
                this.earliestPendingTransaction.Psn < newHeadPsn)
            {
                Utility.Assert(
                    this.earliestPendingTransactionOffset != LogicalLogRecord.InvalidLogicalRecordOffset,
                    "FreePreviousLinksLowerThanPsn: Earliest pending transaction offset cannot be invalid for checkpoint record lsn: {0}, psn: {1}",
                    Lsn,
                    Psn);

                this.earliestPendingTransaction = BeginTransactionOperationLogRecord.InvalidBeginTransactionLogRecord;
                Interlocked.Increment(ref this.earliestPendingTransactionInvalidated);
                return true;
            }

            return ret;
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal CheckpointState CheckpointState
        {
            get { return this.checkpointState; }

            set
            {
                Utility.Assert(
                    this.checkpointState < value,
                    "New value of checkpoint state should be greater than the existing checkpoint state. Existing checkpoint stats is {0}, new value is {1}",
                    this.checkpointState, value);
                this.checkpointState = value;
            }
        }

        internal Epoch Epoch
        {
            get { return this.epoch; }
        }

        internal BeginTransactionOperationLogRecord EarliestPendingTransaction
        {
            get
            {
                Utility.Assert(
                   Interlocked.Read(ref this.earliestPendingTransactionInvalidated) == 0,
                   "Cannot access earliest pending tx lsn for checkpoint record as it is invalidated lsn: {0} psn: {1}",
                   Lsn.LSN,
                   Psn.PSN);

                return this.earliestPendingTransaction;
            }

            set
            {
                Utility.Assert(
                    IsInvalidRecord(this.earliestPendingTransaction) == true,
                    "LogRecord.IsInvalidRecord(this.earliestPendingTransaction) == true");

                this.earliestPendingTransaction = value;
                if (this.earliestPendingTransaction != null)
                {
                    this.earliestPendingTransaction.RecordEpoch = this.epoch;
                }
            }
        }

        internal LogicalSequenceNumber EarliestPendingTransactionLsn
        {
            get
            {
                Utility.Assert(
                      Interlocked.Read(ref this.earliestPendingTransactionInvalidated) == 0,
                      "Cannot access earliest pending tx lsn for checkpoint record as it is invalidated lsn: {0} psn: {1}",
                      Lsn.LSN,
                      Psn.PSN);

                if ((this.earliestPendingTransaction == null) || IsInvalidRecord(this.earliestPendingTransaction))
                {
                    return this.Lsn;
                }

                return this.earliestPendingTransaction.Lsn;
            }
        }

        internal ulong EarliestPendingTransactionOffset
        {
            get { return this.earliestPendingTransactionOffset; }
        }

        internal ulong EarliestPendingTransactionRecordPosition
        {
            get { return this.RecordPosition - this.earliestPendingTransactionOffset; }
        }

        internal LogicalSequenceNumber LastStableLsn
        {
            get { return this.lastStableLsn; }

            set { this.lastStableLsn = value; }
        }

        internal ProgressVector ProgressVector
        {
            get { return this.progressVector; }
        }

        internal async Task AwaitCompletionPhase1FirstCheckpointOnFullCopy(string type)
        {
            Utility.Assert(this.IsFirstCheckpointOnFullCopy == true, "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy");

            FabricEvents.Events.Checkpoint(
                type,
                "AwaitCompletionPhase1FirstCheckpointOnFullCopy_Start",
                (int)CheckpointState,
                Lsn.LSN,
                Psn.PSN,
                RecordPosition,
                EarliestPendingTransactionLsn.LSN);

            await firstCheckpointPhase1CompletionTcs.Task.ConfigureAwait(false);

            FabricEvents.Events.Checkpoint(
                type,
                "AwaitCompletionPhase1FirstCheckpointOnFullCopy_End",
                (int)CheckpointState,
                Lsn.LSN,
                Psn.PSN,
                RecordPosition,
                EarliestPendingTransactionLsn.LSN);
        }

        internal void SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(Exception e, string type)
        {
            Utility.Assert(
                this.IsFirstCheckpointOnFullCopy == true, 
                "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy or if exception is null");
            
            FabricEvents.Events.Checkpoint(
                type,
                "SignalExceptionForPhase1OfFirstCheckpointOnFullCopy",
                (int)CheckpointState,
                Lsn.LSN,
                Psn.PSN,
                RecordPosition,
                EarliestPendingTransactionLsn.LSN);

            if (e != null)
            {
                firstCheckpointPhase1CompletionTcs.SetException(e);
            }
            else
            {
                firstCheckpointPhase1CompletionTcs.SetException(new Exception("Encountered exception during phase 1 of full copy checkpoint"));
            }
        }

        internal void SignalCompletionPhase1FirstCheckpointOnFullCopy(string type)
        {
            Utility.Assert(
                this.IsFirstCheckpointOnFullCopy == true, 
                "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy");

            FabricEvents.Events.Checkpoint(
                type,
                "SignalCompletionPhase1FirstCheckpointOnFullCopy",
                (int)CheckpointState,
                Lsn.LSN,
                Psn.PSN,
                RecordPosition,
                EarliestPendingTransactionLsn.LSN);

            firstCheckpointPhase1CompletionTcs.SetResult(null);
        }

        internal static BeginCheckpointLogRecord CreateZeroBeginCheckpointLogRecord()
        {
            return new BeginCheckpointLogRecord(true);
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            this.progressVector = new ProgressVector();
            this.progressVector.Read(br, isPhysicalRead);
            this.earliestPendingTransactionOffset = br.ReadUInt64();
            this.epoch = new Epoch(br.ReadInt64(), br.ReadInt64());

            // Read the backup information
            // Note that if you change this part, backup log record also must be changed.
            this.backupId = new Guid(br.ReadBytes(Constants.SizeOfGuidBytes));

            this.highestBackedUpEpoch = new Epoch(br.ReadInt64(), br.ReadInt64());
            this.highestBackedUpLsn = new LogicalSequenceNumber(br.ReadInt64());

            this.backupLogRecordCount = br.ReadUInt32();
            this.backupLogSize = br.ReadUInt32();

            // Conditionally read periodicCheckpointTimeTicks_
            // Ensures compatibility with versions prior to addition of timestamp field
            if (br.BaseStream.Position < endPosition)
            {
                this.lastPeriodicCheckpointTimeTicks = br.ReadInt64();
                this.lastPeriodicTruncationTimeTicks = br.ReadInt64();
            }

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);

            var startingPos = bw.BaseStream.Position;

            // Leave room for size
            bw.BaseStream.Position += sizeof(int);

            // By now, all the log records before this begin checkpoint must have been serialized
            if (this.earliestPendingTransactionOffset == LogicalLogRecord.InvalidLogicalRecordOffset || forceRecomputeOffsets == true)
            {
                Utility.Assert(
                    this.earliestPendingTransaction
                    != BeginTransactionOperationLogRecord.InvalidBeginTransactionLogRecord,
                    "Earliest pending transaction is not expected to be invalid");

                if (this.earliestPendingTransaction == null)
                {
                    this.earliestPendingTransactionOffset = 0;
                }
                else
                {
                    this.earliestPendingTransactionOffset = this.RecordPosition
                                                            - this.earliestPendingTransaction.RecordPosition;
                }
            }

            Utility.Assert(this.progressVector != null, "this.progressVector != null");
            this.progressVector.Write(bw);
            bw.Write(this.earliestPendingTransactionOffset);
            Utility.Assert(
                this.epoch != LogicalSequenceNumber.InvalidEpoch,
                "this.copyEpoch != LogicalSequenceNumber.InvalidEpoch");
            bw.Write(this.epoch.DataLossNumber);
            bw.Write(this.epoch.ConfigurationNumber);

            // Write the backup information.
            // Note that if you change this part, backup log record also must be changed.
            Utility.Assert(
                this.highestBackedUpEpoch.DataLossNumber != LogicalSequenceNumber.InvalidLsn.LSN,
                "this.backupLogSize != uint.MaxValue");
            Utility.Assert(
                this.highestBackedUpEpoch.ConfigurationNumber != LogicalSequenceNumber.InvalidLsn.LSN,
                "this.backupLogSize != uint.MaxValue");
            Utility.Assert(
                this.highestBackedUpLsn != LogicalSequenceNumber.InvalidLsn,
                "this.backupLogSize != uint.MaxValue");
            Utility.Assert(this.backupLogRecordCount != uint.MaxValue, "this.backupLogSize != uint.MaxValue");
            Utility.Assert(this.backupLogSize != uint.MaxValue, "this.backupLogSize != uint.MaxValue");

            bw.Write(this.backupId.ToByteArray());
            bw.Write(this.highestBackedUpEpoch.DataLossNumber);
            bw.Write(this.highestBackedUpEpoch.ConfigurationNumber);
            bw.Write(this.highestBackedUpLsn.LSN);

            bw.Write(this.backupLogRecordCount);
            bw.Write(this.backupLogSize);

            bw.Write(this.lastPeriodicCheckpointTimeTicks);
            bw.Write(this.lastPeriodicTruncationTimeTicks);

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPos));
            bw.BaseStream.Position = startingPos;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPos, bw));
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
            this.ApproximateSizeOnDisk += (uint) this.progressVector.Length*sizeof(long)*6;
        }
    }
}