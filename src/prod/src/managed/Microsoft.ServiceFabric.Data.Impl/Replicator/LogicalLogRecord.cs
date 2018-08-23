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
    using System.Threading.Tasks;

    internal class LogicalLogRecord : LogRecord
    {
        internal const ulong InvalidLogicalRecordOffset = ulong.MaxValue;
        private const int SizeOnWire = sizeof(int) + sizeof(int) + sizeof(uint) + sizeof(long);

        internal static readonly LogicalLogRecord InvalidLogicalLogRecord = new LogicalLogRecord();
        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int);

        private Task<long> replicatorTask;
        private ArraySegment<byte> replicatedData;
        private BinaryWriter replicatedDataWriter;

        internal LogicalLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            this.replicatorTask = null;
        }

        internal LogicalLogRecord(LogRecordType recordType)
            : base(recordType)
        {
            this.replicatorTask = null;
        }

        protected LogicalLogRecord()
            : base()
        {
            this.replicatorTask = null;
        }

        internal virtual bool IsLatencySensitiveRecord
        {
            get { return false; }
        }

        internal bool IsReplicated
        {
            get
            {
                if (this.replicatorTask != null)
                {
                    return this.replicatorTask.IsCompleted;
                }

                return true;
            }
        }

        internal Task<long> ReplicatorTask
        {
            get { return this.replicatorTask; }

            set { this.replicatorTask = value; }
        }

        internal async Task AwaitReplication()
        {
            if (this.replicatorTask != null)
            {
                await this.replicatorTask.ConfigureAwait(false);
            }

            return;
        }

        internal virtual void InvalidateReplicatedDataBuffers(SynchronizedBufferPool<BinaryWriter> pool)
        {
            if (pool != null && this.replicatedDataWriter != null)
            {
                pool.Return(this.replicatedDataWriter);
            }

            this.replicatedDataWriter = null;
            this.replicatedData = new ArraySegment<byte>();
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal OperationData ToOperationData(BinaryWriter bw)
        {
            this.replicatedDataWriter = bw;
            this.replicatedDataWriter.BaseStream.Position = 0;
            var operationData = new OperationData();
            this.Write(this.replicatedDataWriter, operationData, false, false);
            return operationData;
        }

        protected virtual int GetSizeOnWire()
        {
            return SizeOnWire;
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

            if (this.replicatedData.Array == null)
            {
                // Metadata Size
                bw.BaseStream.Position += sizeof(int);

                // Future fields

                // End of metadata.
                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPosition));
                bw.BaseStream.Position = startingPosition;
                bw.Write(sizeOfSection);
                bw.BaseStream.Position = endPosition;

                this.replicatedData = CreateArraySegment(startingPosition, bw);
            }

            operationData.Add(this.replicatedData);
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