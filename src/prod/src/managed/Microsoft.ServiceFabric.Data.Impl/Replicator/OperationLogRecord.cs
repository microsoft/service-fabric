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

    internal sealed class OperationLogRecord : TransactionLogRecord
    {
        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + (3*sizeof(int)) + sizeof(bool);

        private static readonly int NullOperationDataCode = -1;

        // The below fields are persisted
        private bool isRedoOnly;

        private List<ArraySegment<byte>> replicatedData;

        private OperationData metaData;

        private object operationContext;

        private OperationData redo;

        private OperationData undo;

        internal OperationLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            Utility.Assert(recordType == LogRecordType.Operation, "recordType == LogRecordType.Operation");
            this.metaData = null;
            this.redo = null;
            this.undo = null;
            this.operationContext = null;
            this.isRedoOnly = false;
        }

        internal OperationLogRecord(
            TransactionBase transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
            : base(LogRecordType.Operation, transaction, InvalidTransactionLogRecord)
        {
            this.metaData = metaData;
            this.undo = undo;
            this.redo = redo;
            this.operationContext = operationContext;
            this.isRedoOnly = false;

            this.UpdateApproximateDiskSize();
        }

        internal OperationLogRecord(
            TransactionBase transaction,
            OperationData metaData,
            OperationData redo,
            object operationContext)
            : base(LogRecordType.Operation, transaction, InvalidTransactionLogRecord)
        {
            this.metaData = metaData;
            this.undo = null;
            this.redo = redo;
            this.operationContext = operationContext;
            this.isRedoOnly = true;

            this.UpdateApproximateDiskSize();
        }

        internal override bool IsLatencySensitiveRecord
        {
            get { return this.Transaction.IsAtomicOperation; }
        }

        internal bool IsOperationContextPresent
        {
            get { return this.operationContext != null; }
        }

        internal bool IsRedoOnly
        {
            get { return this.isRedoOnly; }
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

        internal OperationData Redo
        {
            get { return this.redo; }
        }

        internal OperationData Undo
        {
            get { return this.undo; }
        }

        internal static OperationData ReadOperationData(OperationData operationData, ref int index)
        {
            int count;
            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                count = br.ReadInt32();
            }

            if (count == NullOperationDataCode)
            {
                return null;
            }

            var operationData2 = new OperationData();
            for (var i = 0; i < count; i++)
            {
                operationData2.Add(ReadBytes(operationData, ref index));
            }

            return operationData2;
        }

        internal static OperationData ReadOperationData(BinaryReader br)
        {
            var count = br.ReadInt32();
            if (count == NullOperationDataCode)
            {
                return null;
            }

            var operationData = new OperationData();
            for (var i = 0; i < count; i++)
            {
                operationData.Add(ReadBytes(br));
            }

            return operationData;
        }

        internal static void WriteOperationData(
            OperationData bytes,
            BinaryWriter bw,
            ref List<ArraySegment<byte>> writtenSegments)
        {
            var startingPos = bw.BaseStream.Position;

            if (bytes == null)
            {
                bw.Write(NullOperationDataCode);
                writtenSegments.Add(CreateArraySegment(startingPos, bw));
                return;
            }

            bw.Write(bytes.Count);
            writtenSegments.Add(CreateArraySegment(startingPos, bw));

            foreach (var arraySegment in bytes)
            {
                startingPos = bw.BaseStream.Position;
                bw.Write(arraySegment.Count);
                writtenSegments.Add(CreateArraySegment(startingPos, bw));

                // NOTE: Include the array segment only if it has some data. Otherwise, V1 replicator fails replication
                if (arraySegment.Count > 0)
                {
                    writtenSegments.Add(arraySegment);
                }
            }
        }

        internal object ResetOperationContext()
        {
            var context = this.operationContext;
            if (context != null)
            {
                // context = Interlocked.CompareExchange(ref this.operationContext, null, context);
                this.operationContext = null;
            }

            return context;
        }

        protected override int GetSizeOnWire()
        {
            var size = base.GetSizeOnWire() + sizeof(int) + this.CalculateWireSize(this.metaData) + this.CalculateWireSize(this.redo)
                       + sizeof(bool);

            if (this.isRedoOnly == false)
            {
                size += this.CalculateWireSize(this.undo);
            }

            return size;
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

            // Read Metadata fields
            this.isRedoOnly = br.ReadBoolean();

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = endPosition;

            if (this.isRedoOnly)
            {
                this.Transaction = AtomicRedoOperation.CreateAtomicRedoOperation(this.Transaction.Id, true);
            }

            this.metaData = ReadOperationData(br);
            this.redo = ReadOperationData(br);

            if (!this.isRedoOnly)
            {
                this.undo = ReadOperationData(br);
            }

            this.UpdateApproximateDiskSize();
            return;
        }

        protected override void ReadLogical(OperationData operationData, ref int index)
        {
            base.ReadLogical(operationData, ref index);

            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                // Metadata section.
                var startingPosition = br.BaseStream.Position;
                var sizeOfSection = br.ReadInt32();
                var endPosition = startingPosition + sizeOfSection;

                // Read Metadata fields
                this.isRedoOnly = br.ReadBoolean();

                // Jump to the end of the section ignoring fields that are not understood.
                Utility.Assert(endPosition >= br.BaseStream.Position, "Could not have read more than section size.");
                br.BaseStream.Position = endPosition;
            }

            if (this.isRedoOnly)
            {
                this.Transaction = AtomicRedoOperation.CreateAtomicRedoOperation(this.Transaction.Id, true);
            }

            this.metaData = ReadOperationData(operationData, ref index);
            this.redo = ReadOperationData(operationData, ref index);

            if (!this.isRedoOnly)
            {
                this.undo = ReadOperationData(operationData, ref index);
            }

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

                // Write Metadata fields
                bw.Write(this.isRedoOnly);

                // End Metadata section
                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPos));
                bw.BaseStream.Position = startingPos;
                bw.Write(sizeOfSection);
                bw.BaseStream.Position = endPosition;

                this.replicatedData.Add(CreateArraySegment(startingPos, bw));
                WriteOperationData(this.metaData, bw, ref this.replicatedData);
                WriteOperationData(this.redo, bw, ref this.replicatedData);

                if (!this.isRedoOnly)
                {
                    WriteOperationData(this.undo, bw, ref this.replicatedData);
                }
            }

            foreach (var segment in this.replicatedData)
            {
                operationData.Add(segment);
            }

            return;
        }

        private static ArraySegment<byte> ReadBytes(OperationData operationData, ref int index)
        {
            int count;
            using (var br = new BinaryReader(IncrementIndexAndGetMemoryStreamAt(operationData, ref index)))
            {
                count = br.ReadInt32();
            }

            if (count == 0)
            {
                return new ArraySegment<byte>(new byte[0]);
            }

            return IncrementIndexAndGetArraySegmentAt(operationData, ref index);
        }

        private static ArraySegment<byte> ReadBytes(BinaryReader br)
        {
            var count = br.ReadInt32();
            var array = br.ReadBytes(count);

            return new ArraySegment<byte>(array);
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