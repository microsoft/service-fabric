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

    internal sealed class EndTransactionLogRecord : TransactionLogRecord
    {
        private const uint ApproximateDiskSpaceUsed = sizeof(bool);

        private const int SizeOnWireIncrement = sizeof(int) + sizeof(bool);

        private readonly bool isThisReplicaTransaction;

        // This is the only field persisted
        private bool isCommitted;

        private ArraySegment<byte> replicatedData;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal EndTransactionLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(
                recordType == LogRecordType.EndTransaction,
                "LogRecordType {0} != EndTransaction",
                recordType);
            this.ChildTransactionRecord = null;
            this.isCommitted = false;
            this.isThisReplicaTransaction = false;
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        internal EndTransactionLogRecord(TransactionBase transaction, bool isCommitted, bool isThisReplicaTransaction)
            : base(LogRecordType.EndTransaction, transaction, InvalidTransactionLogRecord)
        {
            Utility.Assert(
                (isThisReplicaTransaction == true) || (isCommitted == false),
                "This should be a replica transaction or it should not be committed. IsThisReplicaTransaction : {0}, IsCommited: {1} ",
                isThisReplicaTransaction, isCommitted);
            this.ChildTransactionRecord = null;
            this.isCommitted = isCommitted;
            this.isThisReplicaTransaction = isThisReplicaTransaction;
            this.UpdateApproximateDiskSize();
        }

        internal bool IsCommitted
        {
            get { return this.isCommitted; }

            set { this.isCommitted = value; }
        }

        internal override bool IsLatencySensitiveRecord
        {
            get { return true; }
        }

        internal bool IsThisReplicaTransaction
        {
            get { return this.isThisReplicaTransaction; }
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
            return;
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
                bw.BaseStream.Position += sizeof(int);

                bw.Write(this.isCommitted);

                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPos));
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

            this.isCommitted = br.ReadBoolean();

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