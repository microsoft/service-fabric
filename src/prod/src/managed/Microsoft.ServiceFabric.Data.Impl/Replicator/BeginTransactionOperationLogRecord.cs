// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;

    internal sealed class BeginTransactionOperationLogRecord : TransactionLogRecord
    {
        internal static readonly BeginTransactionOperationLogRecord InvalidBeginTransactionLogRecord =
            new BeginTransactionOperationLogRecord();

        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + (3*sizeof(int)) + sizeof(bool);

        // These fields are persisted
        private bool isSingleOperationTransaction;

        private List<ArraySegment<byte>> replicatedData;

        private OperationData metaData;

        private object operationContext;

        private Epoch recordEpoch;

        private OperationData redo;

        private OperationData undo;

        internal BeginTransactionOperationLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            this.isSingleOperationTransaction = false;
            this.ParentTransactionRecord = null;
            Utility.Assert(recordType == LogRecordType.BeginTransaction, "recordType == LogRecordType.BeginTransaction");
            this.recordEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.metaData = null;
            this.redo = null;
            this.undo = null;
            this.operationContext = null;
        }

        internal BeginTransactionOperationLogRecord(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            bool isSingleOperationTransaction)
            : base(LogRecordType.BeginTransaction, transaction, null)
        {
            this.isSingleOperationTransaction = isSingleOperationTransaction;
            this.recordEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.metaData = metaData;
            this.undo = undo;
            this.redo = redo;
            this.operationContext = operationContext;

            this.UpdateApproximateDiskSize();
        }

        private BeginTransactionOperationLogRecord()
            : base()
        {
            this.isSingleOperationTransaction = false;
            this.recordEpoch = LogicalSequenceNumber.InvalidEpoch;
            this.redo = null;
            this.undo = null;
            this.metaData = null;
            this.operationContext = null;
        }

        internal override bool IsLatencySensitiveRecord
        {
            get { return this.IsSingleOperationTransaction; }
        }

        internal bool IsSingleOperationTransaction
        {
            get { return this.isSingleOperationTransaction; }
        }

        internal OperationData MetaData
        {
            get { return this.metaData; }

            set { this.metaData = value; }
        }

        internal object OperationContext
        {
            set
            {
                // todo preethas:  With name being added to operation context, needs value comparison and not reference. Removing it for now.
                // Utility.Assert((this.operationContext == null) || (this.operationContext == value));
                this.operationContext = value;
            }
        }

        internal Epoch RecordEpoch
        {
            get { return this.recordEpoch; }

            set { this.recordEpoch = value; }
        }

        internal OperationData Redo
        {
            get { return this.redo; }
        }

        internal OperationData Undo
        {
            get { return this.undo; }
        }

        internal object ResetOperationContext()
        {
            var context = this.operationContext;
            if (context != null)
            {
                this.operationContext = null;
            }

            return context;
        }

        protected override int GetSizeOnWire()
        {
            return base.GetSizeOnWire() + sizeof(int) + this.CalculateWireSize(this.metaData) + this.CalculateWireSize(this.redo)
                   + this.CalculateWireSize(this.undo) + sizeof(bool);
        }

        internal override void InvalidateReplicatedDataBuffers(SynchronizedBufferPool<BinaryWriter> pool)
        {
            base.InvalidateReplicatedDataBuffers(pool);
            this.replicatedData = null;
        }

        protected override void Read(BinaryReader br, bool isPhysicalRead)
        {
            // IF THIS METHOD CHANGES, READLOGICAL MUST CHANGE ACCORDINGLY AS WE ARE ADDING MULTIPLE ARRAYSEGMENTS HERE
            // THIS IS TRUE FOR ANY REDO+UNDO TYPE RECORDS
            base.Read(br, isPhysicalRead);

            // Metadata section.
            var startingPosition = br.BaseStream.Position;
            var sizeOfSection = br.ReadInt32();
            var endPosition = startingPosition + sizeOfSection;

            // Read single operation optimiation flag.
            this.isSingleOperationTransaction = br.ReadBoolean();

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            this.metaData = OperationLogRecord.ReadOperationData(br);
            this.redo = OperationLogRecord.ReadOperationData(br);
            this.undo = OperationLogRecord.ReadOperationData(br);

            this.UpdateApproximateDiskSize();
        }

        protected override void ReadLogical(OperationData operationData, ref int index)
        {
            base.ReadLogical(operationData, ref index);

            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                // Read metadata section
                var startingPosition = br.BaseStream.Position;
                var sizeOfSection = br.ReadInt32();
                var endPosition = startingPosition + sizeOfSection;

                // Read metadata fields
                this.isSingleOperationTransaction = br.ReadBoolean();

                // Jump to the end of the section ignoring fields that are not understood.
                Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
                br.BaseStream.Position = endPosition;
            }

            this.metaData = OperationLogRecord.ReadOperationData(operationData, ref index);
            this.redo = OperationLogRecord.ReadOperationData(operationData, ref index);
            this.undo = OperationLogRecord.ReadOperationData(operationData, ref index);

            this.UpdateApproximateDiskSize();
        }

        protected override void Write(BinaryWriter bw, OperationData operationData, bool isPhysicalWrite, bool forceRecomputeOffsets)
        {
            base.Write(bw, operationData, isPhysicalWrite, forceRecomputeOffsets);

            if (this.replicatedData == null)
            {
                this.replicatedData = new List<ArraySegment<byte>>();

                // Metadata section.
                var startingPos = bw.BaseStream.Position;
                bw.BaseStream.Position += sizeof(int);

                // Metadata fields.
                bw.Write(this.isSingleOperationTransaction);

                // End Metadata section
                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPos));
                bw.BaseStream.Position = startingPos;
                bw.Write(sizeOfSection);
                bw.BaseStream.Position = endPosition;

                this.replicatedData.Add(CreateArraySegment(startingPos, bw));

                OperationLogRecord.WriteOperationData(this.metaData, bw, ref this.replicatedData);
                OperationLogRecord.WriteOperationData(this.redo, bw, ref this.replicatedData);
                OperationLogRecord.WriteOperationData(this.undo, bw, ref this.replicatedData);
            }

            foreach (var segment in this.replicatedData)
            {
                operationData.Add(segment);
            }
        }

        private void UpdateApproximateDiskSize()
        {
            if (this.metaData != null)
            {
                foreach (var buffer in this.metaData)
                {
                    this.ApproximateSizeOnDisk += (uint) buffer.Count + sizeof(int);
                }
            }

            if (this.undo != null)
            {
                foreach (var buffer in this.undo)
                {
                    this.ApproximateSizeOnDisk += (uint) buffer.Count + sizeof(int);
                }
            }

            if (this.redo != null)
            {
                foreach (var buffer in this.redo)
                {
                    this.ApproximateSizeOnDisk += (uint) buffer.Count + sizeof(int);
                }
            }

            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}