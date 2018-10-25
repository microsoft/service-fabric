// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    internal abstract class LogHeadRecord : PhysicalLogRecord
    {
        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(ulong) + (4*sizeof(long));

        private Epoch logHeadEpoch;

        private LogicalSequenceNumber logHeadLsn;

        private PhysicalSequenceNumber logHeadPsn;

        // The following fields are not persisted
        private IndexingLogRecord logHeadRecord;

        private ulong logHeadRecordOffset;

        internal LogHeadRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            this.logHeadEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.logHeadLsn = LogicalSequenceNumber.InvalidLsn;
            this.logHeadPsn = PhysicalSequenceNumber.InvalidPsn;
            this.logHeadRecordOffset = InvalidPhysicalRecordOffset;
            this.logHeadRecord = IndexingLogRecord.InvalidIndexingLogRecord;
        }

        internal LogHeadRecord(
            LogRecordType recordType,
            IndexingLogRecord logHeadRecord,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord lastLinkedPhysicalRecord)
            : base(recordType, lsn, lastLinkedPhysicalRecord)
        {
            this.logHeadEpoch = logHeadRecord.CurrentEpoch;
            this.logHeadLsn = logHeadRecord.Lsn;
            this.logHeadPsn = logHeadRecord.Psn;
            this.logHeadRecordOffset = InvalidPhysicalRecordOffset;
            this.logHeadRecord = logHeadRecord;

            this.UpdateApproximateDiskSize();
        }

        internal IndexingLogRecord HeadRecord
        {
            get { return this.logHeadRecord; }

            set { this.logHeadRecord = value; }
        }

        internal Epoch LogHeadEpoch
        {
            get { return this.logHeadEpoch; }
        }

        internal LogicalSequenceNumber LogHeadLsn
        {
            get { return this.logHeadLsn; }
        }

        internal PhysicalSequenceNumber LogHeadPsn
        {
            get { return this.logHeadPsn; }
        }

        internal ulong LogHeadRecordOffset
        {
            get { return this.logHeadRecordOffset; }
        }

        internal ulong LogHeadRecordPosition
        {
            get { return this.RecordPosition - this.logHeadRecordOffset; }
        }

        internal override bool FreePreviousLinksLowerThanPsn(PhysicalSequenceNumber newHeadPsn)
        {
            bool ret = base.FreePreviousLinksLowerThanPsn(newHeadPsn);

            if (this.logHeadRecord != null &&
                this.logHeadRecord.Psn < newHeadPsn)
            {
                Utility.Assert(
                    this.logHeadRecordOffset != InvalidPhysicalRecordOffset,
                    "this.logHeadRecordOffset must be valid");

                this.logHeadRecord = 
                    (this.logHeadRecordOffset == 0) ? 
                        null : 
                        IndexingLogRecord.InvalidIndexingLogRecord;

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

            var dataLossNumber = br.ReadInt64();
            var configurationNumber = br.ReadInt64();
            this.logHeadEpoch = new Epoch(dataLossNumber, configurationNumber);
            this.logHeadLsn = new LogicalSequenceNumber(br.ReadInt64());
            this.logHeadPsn = new PhysicalSequenceNumber(br.ReadInt64());
            if (isPhysicalRead == true)
            {
                this.logHeadRecordOffset = br.ReadUInt64();
            }

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
            return;
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);
            var startingPos = bw.BaseStream.Position;
            bw.BaseStream.Position += sizeof(int);

            bw.Write(this.logHeadEpoch.DataLossNumber);
            bw.Write(this.logHeadEpoch.ConfigurationNumber);
            bw.Write(this.logHeadLsn.LSN);
            bw.Write(this.logHeadPsn.PSN);

            if (isPhysicalWrite == true)
            {
                if (this.logHeadRecordOffset == InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
                {
                    Utility.Assert(
                        this.logHeadRecord != null,
                        "this.logHeadRecord != null");
                    Utility.Assert(
                        this.logHeadRecord.RecordPosition != InvalidRecordPosition,
                        "this.logHeadRecord.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                    Utility.Assert(
                        this.RecordPosition != InvalidRecordPosition,
                        "this.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                    this.logHeadRecordOffset = this.RecordPosition - this.logHeadRecord.RecordPosition;
                }

                bw.Write(this.logHeadRecordOffset);
            }

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPos));
            bw.BaseStream.Position = startingPos;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPos, bw));
            return;
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}