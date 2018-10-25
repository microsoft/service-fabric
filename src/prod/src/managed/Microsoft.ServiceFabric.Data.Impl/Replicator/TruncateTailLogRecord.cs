// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    internal sealed class TruncateTailLogRecord : PhysicalLogRecord
    {
        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int);

        internal TruncateTailLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.TruncateTail, "recordType == LogRecordType.TruncateTail");
        }

        internal TruncateTailLogRecord(LogicalSequenceNumber lsn, PhysicalLogRecord lastLinkedPhysicalRecord)
            : base(LogRecordType.TruncateTail, lsn, lastLinkedPhysicalRecord)
        {
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);
            this.ReadPrivate(br, isPhysicalRead);

            return;
        }

        protected override void ReadLogical(OperationData operationData, ref int index)
        {
            base.ReadLogical(operationData, ref index);

            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                this.ReadPrivate(br, false);
            }

            return;
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);
            var startingPosition = bw.BaseStream.Position;

            // Metadata Size
            bw.BaseStream.Position += sizeof(int);

            // Future fields

            // End of metadata.
            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPosition));
            bw.BaseStream.Position = startingPosition;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPosition, bw));
        }

        private void ReadPrivate(BinaryReader br, bool isPhysicalRead)
        {
            // Read Metadata size.
            var logicalStartingPosition = br.BaseStream.Position;
            var logicalSizeOfSection = br.ReadInt32();
            var logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

            // Read Metadata fields (future)

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(logicalEndPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = logicalEndPosition;

            this.UpdateApproximateDiskSize();
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}