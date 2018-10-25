// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.IO;

    internal class PhysicalLogRecord : LogRecord
    {
        internal static readonly PhysicalLogRecord InvalidPhysicalLogRecord = new PhysicalLogRecord();

        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(ulong) + sizeof(long);

        // The following fields are not persisted
        private PhysicalLogRecord linkedPhysicalRecord;

        private ulong linkedPhysicalRecordOffset;

        private WeakReference<PhysicalLogRecord> nextPhysicalRecord;

        protected PhysicalLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            this.linkedPhysicalRecordOffset = InvalidPhysicalRecordOffset;
            this.linkedPhysicalRecord = InvalidPhysicalLogRecord;
            this.nextPhysicalRecord = new WeakReference<PhysicalLogRecord>(InvalidPhysicalLogRecord);
        }

        protected PhysicalLogRecord(
            LogRecordType recordType,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord linkedPhysicalRecord)
            : base(recordType)
        {
            this.Lsn = lsn;
            if (linkedPhysicalRecord != null)
            {
                this.linkedPhysicalRecordOffset = InvalidPhysicalRecordOffset;
                this.linkedPhysicalRecord = linkedPhysicalRecord;
            }
            else
            {
                this.linkedPhysicalRecordOffset = 0;
                this.linkedPhysicalRecord = null;
            }

            this.nextPhysicalRecord = new WeakReference<PhysicalLogRecord>(InvalidPhysicalLogRecord);

            this.UpdateApproximateDiskSize();
        }

        protected PhysicalLogRecord()
            : base()
        {
            this.linkedPhysicalRecordOffset = InvalidPhysicalRecordOffset;
            this.linkedPhysicalRecord = null;
            this.nextPhysicalRecord = null;
        }

        internal PhysicalLogRecord LinkedPhysicalRecord
        {
            get { return this.linkedPhysicalRecord; }

            set { this.linkedPhysicalRecord = value; }
        }

        internal ulong LinkedPhysicalRecordOffset
        {
            get { return this.linkedPhysicalRecordOffset; }

            // Test only set.
            set { this.linkedPhysicalRecordOffset = value; }
        }

        internal PhysicalLogRecord NextPhysicalRecord
        {
            get
            {
                if (this.nextPhysicalRecord == null)
                {
                    return null;
                }

                PhysicalLogRecord result = null;
                bool exists = this.nextPhysicalRecord.TryGetTarget(out result);

#if (!DEBUG)
                Utility.Assert(
                    exists,
                    "NextPhysicalRecord for Lsn: {0} is not present",
                    this.Lsn);
#endif

                return result;
            }

            set { this.nextPhysicalRecord = new WeakReference<PhysicalLogRecord>(value); }
        }
        
        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            this.linkedPhysicalRecordOffset = br.ReadUInt64();

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
            return;
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);
            var startingPosition = bw.BaseStream.Position;
            bw.BaseStream.Position += sizeof(int);

            if (this.linkedPhysicalRecordOffset == InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
            {
                Utility.Assert(
                    this.linkedPhysicalRecord != null || forceRecomputeOffsets == true,
                    "(this.linkedPhysicalRecord != null)={0}, forceRecomputeOffsets={1}",
                    this.linkedPhysicalRecord != null,
                    forceRecomputeOffsets);

                if (this.linkedPhysicalRecord == null)
                {
                    this.linkedPhysicalRecordOffset = 0;
                }
                else
                {
                    Utility.Assert(
                        this.linkedPhysicalRecord.RecordPosition != InvalidRecordPosition,
                        "this.linkedPhysicalRecord.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                    Utility.Assert(
                        this.RecordPosition != InvalidRecordPosition,
                        "this.RecordPosition != LogRecord.INVALID_RECORD_POSITION");

                    this.linkedPhysicalRecordOffset = this.RecordPosition - this.linkedPhysicalRecord.RecordPosition;
                }
            }

            bw.Write(this.linkedPhysicalRecordOffset);

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPosition));
            bw.BaseStream.Position = startingPosition;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPosition, bw));
            return;
        }

        internal override bool FreePreviousLinksLowerThanPsn(PhysicalSequenceNumber newHeadPsn)
        {
            bool ret = base.FreePreviousLinksLowerThanPsn(newHeadPsn);

            if (this.linkedPhysicalRecord != null &&
                this.linkedPhysicalRecord.Psn < newHeadPsn)
            {
                Utility.Assert(
                    this.linkedPhysicalRecordOffset != InvalidPhysicalRecordOffset,
                    "FreePreviousLinksLowerThanPsn: PreviousPhysicalRecordOffset is invalid. Record LSN is {0}");

                this.linkedPhysicalRecord = (this.linkedPhysicalRecordOffset == 0)
                    ? null
                    : PhysicalLogRecord.InvalidPhysicalLogRecord;

                return true;
            }

            return ret;
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}