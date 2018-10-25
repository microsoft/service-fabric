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

    internal sealed class BarrierLogRecord : LogicalLogRecord
    {
        private const uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(long);

        private const int SizeOnWireIncrement = sizeof(int) + sizeof(long);

        private LogicalSequenceNumber lastStableLsn;

        private ArraySegment<byte> replicatedData;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal BarrierLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(
                recordType == LogRecordType.Barrier,
                "Expected record type is barrier record, but the actual record type is : {0}",
                recordType);
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal BarrierLogRecord(LogicalSequenceNumber lastStableLsn)
            : base(LogRecordType.Barrier)
        {
            Utility.Assert(
                lastStableLsn != LogicalSequenceNumber.InvalidLsn,
                "lastStableLsn is expected to be invalid but the last stable lsn has value : {0}",
                lastStableLsn.LSN);
            this.lastStableLsn = lastStableLsn;
            this.UpdateApproximateDiskSize();
        }

        private BarrierLogRecord(LogicalSequenceNumber lsn, LogicalSequenceNumber lastStableLsn)
            : base(LogRecordType.Barrier)
        {
            this.Lsn = lsn;
            this.lastStableLsn = lastStableLsn;
        }

        internal LogicalSequenceNumber LastStableLsn
        {
            get { return this.lastStableLsn; }
        }

        internal static BarrierLogRecord CreateOneBarrierLogRecord()
        {
            return new BarrierLogRecord(LogicalSequenceNumber.OneLsn, LogicalSequenceNumber.ZeroLsn);
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

                bw.Write(this.lastStableLsn.LSN);

                // Write size at the beginning.
                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked ((int) (endPosition - startingPos));
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

            this.lastStableLsn = new LogicalSequenceNumber(br.ReadInt64());

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