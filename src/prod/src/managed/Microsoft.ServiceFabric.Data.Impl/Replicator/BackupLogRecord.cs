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

    /// <summary>
    /// Backup log record.
    /// Contains information on the last completed backup.
    /// </summary>
    internal sealed class BackupLogRecord : LogicalLogRecord
    {
        /// <summary>
        /// Approximate disk usage of BackupLogRecord.
        /// </summary>
        /// <remarks>Guid, 1 Epoch, 1 logical sequence number, 2 unsigned integers.</remarks>
        public const uint ApproximateDiskSpaceUsed = sizeof(int) + Constants.SizeOfGuidBytes + (2*sizeof(long)) + sizeof(long) + (2*sizeof(uint));

        /// <summary>
        /// Invalid backup id.
        /// </summary>
        public static readonly Guid InvalidBackupId = Guid.Empty;

        /// <summary>
        /// Approximate wire usage of BackupLogRecord.
        /// </summary>
        /// <remarks>size, Guid, 1 Epoch, 1 logical sequence number, 2 unsigned integers.</remarks>
        private const int SizeOnWireIncrement = sizeof(int) + Constants.SizeOfGuidBytes + (3*sizeof(long)) + (2*sizeof(uint));

        /// <summary>
        /// Creates a zero backup log record indicating that no backup has been taken.
        /// </summary>
        private static readonly BackupLogRecord PrivateZeroBackupLogRecord = new BackupLogRecord(
            InvalidBackupId,
            LogicalSequenceNumber.ZeroEpoch,
            LogicalSequenceNumber.ZeroLsn,
            0,
            0);

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
        /// Current log BackupLogSizeInKB of the backup log in KB.
        /// </summary>
        private uint backupLogSize;

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

        /// <summary>
        /// Cache of the logical serialization.
        /// </summary>
        private ArraySegment<byte> replicatedData;

        /// <summary>
        /// Initializes a new instance of the BackupLogRecord class.
        /// </summary>
        /// <param name="recordType">The record type for double checking.</param>
        /// <param name="recordPosition">The record position.</param>
        /// <param name="lsn"></param>
        /// <remarks>Called on the read path to initialize the log record who will read the binary reader.</remarks>
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal BackupLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(
                recordType == LogRecordType.Backup,
                "recordType ({0}) == LogRecordType.Backup",
                recordType);
        }

        /// <summary>
        /// Initializes a new instance of the BackupLogRecord class.
        /// </summary>
        /// <param name="highestBackedUpEpoch"></param>
        /// <param name="highestBackedUpLsn">Highest logical sequence number that has been backed up.</param>
        /// <param name="backupId"></param>
        /// <param name="backupLogRecordCount"></param>
        /// <param name="backupLogSize"></param>
        /// <remarks>Called on Primary.</remarks>
        internal BackupLogRecord(
            Guid backupId,
            Epoch highestBackedUpEpoch,
            LogicalSequenceNumber highestBackedUpLsn,
            uint backupLogRecordCount,
            uint backupLogSize)
            : base(LogRecordType.Backup)
        {
            // Initialize backup log record fields.
            this.backupId = backupId;
            this.highestBackedUpEpoch = highestBackedUpEpoch;
            this.highestBackedUpLsn = highestBackedUpLsn;

            this.backupLogRecordCount = backupLogRecordCount;
            this.backupLogSize = backupLogSize;

            this.UpdateApproximateDiskSize();
        }

        public Guid BackupId
        {
            get { return this.backupId; }
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
        /// Current log BackupLogSizeInKB of the backup log in KB.
        /// </summary>
        public uint BackupLogSizeInKB
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

        public bool IsZeroBackupLogRecord()
        {
            if (this.backupId != InvalidBackupId)
            {
                return false;
            }

            Utility.Assert(
                this.highestBackedUpEpoch.CompareTo(LogicalSequenceNumber.ZeroEpoch) == 0,
                "If ZeroBackupLogRecord, epoch must be ZeroEpoch. {0}", 
                this.highestBackedUpEpoch);

            Utility.Assert(
                this.highestBackedUpLsn == LogicalSequenceNumber.ZeroLsn,
                "If ZeroBackupLogRecord, highest backed up LSN must be ZeroLsn. {0}",
                this.highestBackedUpLsn);

            Utility.Assert(
                this.backupLogRecordCount == 0,
                "If ZeroBackupLogRecord, backupLogRecordCount must be ZeroLsn. {0}",
                this.backupLogRecordCount);

            Utility.Assert(
                this.backupLogSize == 0,
                "If ZeroBackupLogRecord, backupLogSizemust be ZeroLsn. {0}",
                this.backupLogSize);

            return true;
        }

        internal static BackupLogRecord ZeroBackupLogRecord
        {
            get { return PrivateZeroBackupLogRecord; }
        }

        protected override int GetSizeOnWire()
        {
            return base.GetSizeOnWire() + SizeOnWireIncrement;
        }

        internal override void InvalidateReplicatedDataBuffers(SynchronizedBufferPool<BinaryWriter> pool)
        {
            base.InvalidateReplicatedDataBuffers(pool);
            this.replicatedData = new ArraySegment<byte>();
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);
            this.ReadPrivate(br, isPhysicalRead);
        }

        protected override void ReadLogical(OperationData operationData, ref int index)
        {
            base.ReadLogical(operationData, ref index);

            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                this.ReadPrivate(br, false);
            }
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);

            if (this.replicatedData.Array == null)
            {
                var startingPos = bw.BaseStream.Position;

                // Leave room for size
                bw.BaseStream.Position += sizeof(int);

                // Note that if you change this part, begin checkpoint log record also must be changed.
                bw.Write(this.backupId.ToByteArray());
                bw.Write(this.highestBackedUpEpoch.DataLossNumber);
                bw.Write(this.highestBackedUpEpoch.ConfigurationNumber);
                bw.Write(this.highestBackedUpLsn.LSN);

                bw.Write(this.backupLogRecordCount);
                bw.Write(this.BackupLogSizeInKB);

                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPos));
                bw.BaseStream.Position = startingPos;
                bw.Write(sizeOfSection);
                bw.BaseStream.Position = endPosition;

                this.replicatedData = CreateArraySegment(startingPos, bw);
            }

            operationData.Add(this.replicatedData);
        }

        private void ReadPrivate(BinaryReader br, bool isPhysicalRead)
        {
            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            // Note that if you change this part, begin checkpoint log record also must be changed.
            this.backupId = new Guid(br.ReadBytes(Constants.SizeOfGuidBytes));

            this.highestBackedUpEpoch = new Epoch(br.ReadInt64(), br.ReadInt64());
            this.highestBackedUpLsn = new LogicalSequenceNumber(br.ReadInt64());

            this.backupLogRecordCount = br.ReadUInt32();
            this.backupLogSize = br.ReadUInt32();

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}