// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    internal enum InformationEvent : int
    {
        Invalid = 0,

        Recovered = 1,

        CopyFinished = 2,

        ReplicationFinished = 3,

        Closed = 4,

        PrimarySwap = 5,

        RestoredFromBackup = 6,

        RemovingState = 7,

        // This indicates that change role to none was called and no new records are inserted after this
    }

    internal sealed class InformationLogRecord : PhysicalLogRecord
    {
        internal static readonly InformationLogRecord InvalidInformationLogRecord = new InformationLogRecord();

        private const uint ApproximateDiskSpaceUsed = +sizeof(int) + sizeof(int);
        private InformationEvent informationEvent;

        // The following fields are not persisted
        private bool isBarrierRecord;

        internal InformationLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.Information, "recordType == LogRecordType.Information");
            this.informationEvent = InformationEvent.Invalid;
        }

        internal InformationLogRecord(
            LogicalSequenceNumber lsn,
            PhysicalLogRecord linkedPhysicalRecord,
            InformationEvent informationEvent)
            : base(LogRecordType.Information, lsn, linkedPhysicalRecord)
        {
            this.Initialize(informationEvent);
            this.UpdateApproximateDiskSize();
        }

        private InformationLogRecord()
            : base()
        {
            this.informationEvent = InformationEvent.Invalid;
            this.isBarrierRecord = false;
        }

        internal InformationEvent InformationEvent
        {
            get { return this.informationEvent; }
        }

        internal bool IsBarrierRecord
        {
            get { return this.isBarrierRecord; }
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            base.Read(br, isPhysicalRead);

            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            this.Initialize((InformationEvent) br.ReadInt32());

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

            bw.Write((int) this.informationEvent);

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPos));
            bw.BaseStream.Position = startingPos;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPos, bw));
        }

        private void Initialize(InformationEvent infoEvent)
        {
            this.informationEvent = infoEvent;

            // All information records are currently barrier records
            this.isBarrierRecord = true;
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}