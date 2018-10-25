// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    internal class IndexingLogRecord : PhysicalLogRecord
    {
        internal static readonly IndexingLogRecord InvalidIndexingLogRecord = new IndexingLogRecord();

        private const uint ApproximateDiskSpaceUsed = sizeof(int) + 2*sizeof(long);

        private Epoch currentEpoch;

        internal IndexingLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.Indexing, "recordType == LogRecordType.Indexing");
            this.currentEpoch = LogicalSequenceNumber.InvalidEpoch;
        }

        internal IndexingLogRecord(
            Epoch currentEpoch,
            LogicalSequenceNumber lsn,
            PhysicalLogRecord linkedPhysicalRecord)
            : base(LogRecordType.Indexing, lsn, linkedPhysicalRecord)
        {
            this.currentEpoch = currentEpoch;
            this.UpdateApproximateDiskSize();
        }

        //
        // Invoked when this indexing record is chosen as the new head 
        // Returns the number of physical links we jumped ahead to free previous backward references and how many actually free'd links older than the head
        //
        // This also implies we cannot have backward references from logical records to other records
        // If this requirement comes up, we must be able to nagivate through the logical records 1 by 1 to free the links
        //
        public void OnTruncateHead(out int freeLinkCallCount, out int freeLinkCallTrueCount)
        {
            freeLinkCallCount = 0;
            freeLinkCallTrueCount = 0;

            // Start from the current record (new head) all the way to the latest physical record free-ing previous links if they point to earlier than the new head
            // Without doing so, we slowly start leaking physical links
            // Refer to V2ReplicatorTests/unittest/Integration/PhysicalRecordLeakTest.cs

            PhysicalLogRecord currentPhysicalRecord = this;

            do
            {
                freeLinkCallCount += 1;
                if (currentPhysicalRecord.FreePreviousLinksLowerThanPsn(Psn))
                {
                    freeLinkCallTrueCount += 1;
                }

                currentPhysicalRecord = currentPhysicalRecord.NextPhysicalRecord;

                // We reach the end of the chain
                if (currentPhysicalRecord.Psn == PhysicalSequenceNumber.InvalidPsn)
                {
                    break;
                }

            } while (true);
        }

        private IndexingLogRecord()
            : base()
        {
            this.currentEpoch = LogicalSequenceNumber.InvalidEpoch;
        }

        internal Epoch CurrentEpoch
        {
            get { return this.currentEpoch; }
        }

        internal static IndexingLogRecord CreateZeroIndexingLogRecord()
        {
            return new IndexingLogRecord(LogicalSequenceNumber.ZeroEpoch, LogicalSequenceNumber.ZeroLsn, null);
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            var dataLossNumber = br.ReadInt64();
            var configurationNumber = br.ReadInt64();
            this.currentEpoch = new Epoch(dataLossNumber, configurationNumber);

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

            bw.Write(this.currentEpoch.DataLossNumber);
            bw.Write(this.currentEpoch.ConfigurationNumber);

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