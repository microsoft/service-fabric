// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    internal enum TruncationState : int
    {
        Invalid = 0,

        Ready = 1,

        Applied = 2,

        Completed = 3,

        Faulted = 4,

        Aborted = 5,
    }

    internal sealed class TruncateHeadLogRecord : LogHeadRecord
    {
        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(bool) + sizeof(long);

        private bool isStable;

        // This state is not persisted
        private TruncationState truncationState;

        private long periodicTruncationTimeTicks;

        internal TruncateHeadLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.TruncateHead, "recordType == LogRecordType.TruncateHead");
            this.truncationState = TruncationState.Invalid;
            this.isStable = false;
            this.periodicTruncationTimeTicks = 0;
        }

        internal TruncateHeadLogRecord(
            IndexingLogRecord logHeadRecord,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord lastLinkedPhysicalRecord,
            bool isStable,
            long periodicTruncationTimeTicks)
            : base(LogRecordType.TruncateHead, logHeadRecord, lsn, lastLinkedPhysicalRecord)
        {
            this.truncationState = TruncationState.Invalid;
            this.isStable = isStable;
            this.periodicTruncationTimeTicks = periodicTruncationTimeTicks;
            this.UpdateApproximateDiskSize();
        }

        internal bool IsStable
        {
            get { return this.isStable; }
        }

        internal TruncationState TruncationState
        {
            get { return this.truncationState; }

            set
            {
                Utility.Assert(this.truncationState < value, "this.truncationState < value");
                this.truncationState = value;
            }
        }

        internal long PeriodicTruncationTimestampTicks
        {
            get
            {
                return this.periodicTruncationTimeTicks;
            }
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            this.isStable = br.ReadBoolean();

            if (br.BaseStream.Position < endPosition)
            {
                this.periodicTruncationTimeTicks = br.ReadInt64();
            }

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);
            var startingPosition = bw.BaseStream.Position;
            bw.BaseStream.Position += sizeof(int);

            bw.Write(this.isStable);
            bw.Write(this.periodicTruncationTimeTicks);

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPosition));
            bw.BaseStream.Position = startingPosition;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPosition, bw));
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}