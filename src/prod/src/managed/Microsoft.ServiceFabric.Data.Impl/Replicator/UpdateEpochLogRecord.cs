// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;

    internal sealed class UpdateEpochLogRecord : LogicalLogRecord
    {
        private const int SizeOnWireIncrement = sizeof(int) + (4*sizeof(long));

        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + (4*sizeof(long));

        private Epoch epoch;

        private long primaryReplicaId;

        private DateTime timestamp;

        internal UpdateEpochLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.UpdateEpoch, "recordType == LogRecordType.UpdateEpoch");
            this.epoch = LogicalSequenceNumber.InvalidEpoch;
            this.primaryReplicaId = ProgressVectorEntry.InvalidReplicaId;
            this.timestamp = DateTime.MinValue;
        }

        internal UpdateEpochLogRecord(Epoch epoch, long primaryReplicaId)
            : base(LogRecordType.UpdateEpoch)
        {
            this.epoch = epoch;
            this.primaryReplicaId = primaryReplicaId;
            this.timestamp = DateTime.UtcNow;

            this.UpdateApproximateDiskSize();
        }

        private UpdateEpochLogRecord()
            : this(LogicalSequenceNumber.ZeroEpoch, ProgressVectorEntry.UniversalReplicaId)
        {
            this.Lsn = LogicalSequenceNumber.ZeroLsn;
        }

        internal Epoch Epoch
        {
            get { return this.epoch; }
        }

        internal long PrimaryReplicaId
        {
            get { return this.primaryReplicaId; }
        }

        internal DateTime Timestamp
        {
            get { return this.timestamp; }
        }

        internal static UpdateEpochLogRecord CreateZeroUpdateEpochLogRecord()
        {
            return new UpdateEpochLogRecord();
        }

        protected override int GetSizeOnWire()
        {
            return base.GetSizeOnWire() + SizeOnWireIncrement;
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

            var startingPosition = bw.BaseStream.Position;
            bw.BaseStream.Position += sizeof(int);

            bw.Write(this.epoch.DataLossNumber);
            bw.Write(this.epoch.ConfigurationNumber);
            bw.Write(this.primaryReplicaId);
            bw.Write(this.timestamp.ToBinary());

            var endPosition = bw.BaseStream.Position;
            var sizeOfSection = checked((int) (endPosition - startingPosition));
            bw.BaseStream.Position = startingPosition;
            bw.Write(sizeOfSection);
            bw.BaseStream.Position = endPosition;

            operationData.Add(CreateArraySegment(startingPosition, bw));

            return;
        }

        private void ReadPrivate(BinaryReader br, bool isPhysicalRead)
        {
            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            var dataLossNumber = br.ReadInt64();
            var configurationNumber = br.ReadInt64();
            this.epoch = new Epoch(dataLossNumber, configurationNumber);
            this.primaryReplicaId = br.ReadInt64();
            this.timestamp = DateTime.FromBinary(br.ReadInt64());

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.UpdateApproximateDiskSize();
            return;
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}