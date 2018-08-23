// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.IO;

    internal sealed class EndCheckpointLogRecord : LogHeadRecord
    {
        private const uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(long) + sizeof(ulong);

        // The following fields are not persisted
        private BeginCheckpointLogRecord lastCompletedBeginCheckpointRecord;

        private ulong lastCompletedBeginCheckpointRecordOffset;

        private LogicalSequenceNumber lastStableLsn;

        internal EndCheckpointLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.EndCheckpoint, "recordType == LogRecordType.EndCheckpoint");
            this.lastCompletedBeginCheckpointRecordOffset = InvalidPhysicalRecordOffset;
            this.lastCompletedBeginCheckpointRecord = BeginCheckpointLogRecord.InvalidBeginCheckpointLogRecord;
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal EndCheckpointLogRecord(
            BeginCheckpointLogRecord lastCompletedBeginCheckpointRecord,
            IndexingLogRecord logHeadRecord,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord linkedPhysicalRecord)
            : base(LogRecordType.EndCheckpoint, logHeadRecord, lsn, linkedPhysicalRecord)
        {
            this.lastCompletedBeginCheckpointRecordOffset = InvalidPhysicalRecordOffset;
            this.lastStableLsn = lastCompletedBeginCheckpointRecord.LastStableLsn;
            this.lastCompletedBeginCheckpointRecord = lastCompletedBeginCheckpointRecord;
            Utility.Assert(
                this.lastStableLsn >= this.lastCompletedBeginCheckpointRecord.Lsn,
                "this.lastStableLsn >= this.lastCompletedBeginCheckpointRecord.LastLogicalSequenceNumber. Last stable lsn is : {0} and last completed checkpoint record is {1}",
                this.lastStableLsn.LSN, this.lastCompletedBeginCheckpointRecord.Lsn.LSN);

            this.UpdateApproximateDiskSize();
        }

        private EndCheckpointLogRecord(
            BeginCheckpointLogRecord lastCompletedBeginCheckpointRecord,
            IndexingLogRecord logHeadRecord)
            : this(lastCompletedBeginCheckpointRecord, logHeadRecord, LogicalSequenceNumber.OneLsn, null)
        {
        }

        internal BeginCheckpointLogRecord LastCompletedBeginCheckpointRecord
        {
            get { return this.lastCompletedBeginCheckpointRecord; }

            set { this.lastCompletedBeginCheckpointRecord = value; }
        }

        internal ulong LastCompletedBeginCheckpointRecordOffset
        {
            get { return this.lastCompletedBeginCheckpointRecordOffset; }
        }

        internal LogicalSequenceNumber LastStableLsn
        {
            get { return this.lastStableLsn; }
        }

        internal static EndCheckpointLogRecord CreateOneEndCheckpointLogRecord(
            BeginCheckpointLogRecord lastCompletedBeginCheckpointRecord,
            IndexingLogRecord logHeadRecord)
        {
            return new EndCheckpointLogRecord(lastCompletedBeginCheckpointRecord, logHeadRecord);
        }

        internal override bool FreePreviousLinksLowerThanPsn(PhysicalSequenceNumber newHeadPsn)
        {
            bool ret = base.FreePreviousLinksLowerThanPsn(newHeadPsn);

            if ((this.lastCompletedBeginCheckpointRecord != null) &&
                (this.lastCompletedBeginCheckpointRecord.Psn < newHeadPsn))
            {
                Utility.Assert(
                    this.lastCompletedBeginCheckpointRecordOffset != InvalidPhysicalRecordOffset,
                    "this.lastCompletedBeginCheckpointRecordOffset != PhysicalLogRecord.INVALID_PHYSICAL_RECORD_OFFSET");

                this.lastCompletedBeginCheckpointRecord = this.lastCompletedBeginCheckpointRecordOffset == 0
                    ? null
                    : BeginCheckpointLogRecord.InvalidBeginCheckpointLogRecord;

                return true;
            }

            return ret;
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            this.lastStableLsn = new LogicalSequenceNumber(br.ReadInt64());

            if (isPhysicalRead)
            {
                this.lastCompletedBeginCheckpointRecordOffset = br.ReadUInt64();
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
            bw.BaseStream.Position += sizeof(int);

            bw.Write(this.lastStableLsn.LSN);
            if (isPhysicalWrite == true)
            {
                if (this.lastCompletedBeginCheckpointRecordOffset == InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
                {
                    Utility.Assert(
                        this.lastCompletedBeginCheckpointRecord != null || forceRecomputeOffsets,
                        "(this.lastCompletedBeginCheckpointRecord != null)={0} || forceRecomputeOffsets={1}",
                        this.lastCompletedBeginCheckpointRecord != null,
                        forceRecomputeOffsets);

                    if (this.lastCompletedBeginCheckpointRecord != null)
                    {
                        if (this.lastCompletedBeginCheckpointRecord.RecordPosition != InvalidRecordPosition)
                        {
                            Utility.Assert(
                                this.RecordPosition != InvalidPhysicalRecordOffset,
                                "this.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                            this.lastCompletedBeginCheckpointRecordOffset = this.RecordPosition
                                                                            - this.lastCompletedBeginCheckpointRecord
                                                                                .RecordPosition;
                        }
                        else
                        {
                            this.lastCompletedBeginCheckpointRecordOffset = InvalidPhysicalRecordOffset;
                        }
                    }
                }

                bw.Write(this.lastCompletedBeginCheckpointRecordOffset);
            }

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
        }
    }
}