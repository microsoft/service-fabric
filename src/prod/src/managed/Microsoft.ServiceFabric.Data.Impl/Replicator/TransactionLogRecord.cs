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

    internal class TransactionLogRecord : LogicalLogRecord
    {
        internal static readonly TransactionLogRecord BarrierMarkerLogRecord = new TransactionLogRecord();

        internal static readonly TransactionLogRecord InvalidTransactionLogRecord = new TransactionLogRecord();

        private const int SizeOnWireIncrement = sizeof(int) + sizeof(long);

        private static readonly uint ApproximateDiskSpaceUsed = sizeof(int) + sizeof(int) + sizeof(long) + sizeof(ulong);

        // The following is a logical field
        private TransactionBase transaction;

        private WeakReference<TransactionLogRecord> childTransactionRecord;

        private bool isEnlistedTransaction;

        private ArraySegment<byte> replicatedData;

        // The following fields are not persisted
        private TransactionLogRecord parentTransactionRecord;

        private ulong parentTransactionRecordOffset;

        internal TransactionLogRecord(LogRecordType recordType, ulong recordPosition, long lsn)
            : base(recordType, recordPosition, lsn)
        {
            this.transaction = TransactionBase.InvalidTransaction;
            this.parentTransactionRecordOffset = InvalidLogicalRecordOffset;

            this.parentTransactionRecord = InvalidTransactionLogRecord;
            this.childTransactionRecord = new WeakReference<TransactionLogRecord>(InvalidTransactionLogRecord);
            this.isEnlistedTransaction = false;
        }

        internal TransactionLogRecord(
            LogRecordType recordType,
            TransactionBase transaction,
            TransactionLogRecord parentLogicalRecord)
            : base(recordType)
        {
            this.transaction = transaction;
            this.parentTransactionRecordOffset = InvalidLogicalRecordOffset;

            this.parentTransactionRecord = parentLogicalRecord;
            this.childTransactionRecord = new WeakReference<TransactionLogRecord>(InvalidTransactionLogRecord);
            this.isEnlistedTransaction = false;

            this.UpdateApproximateDiskSize();
        }

        protected TransactionLogRecord()
            : base()
        {
            this.transaction = TransactionBase.InvalidTransaction;
            this.parentTransactionRecordOffset = InvalidLogicalRecordOffset;

            this.parentTransactionRecord = InvalidTransactionLogRecord;
            this.childTransactionRecord = new WeakReference<TransactionLogRecord>(InvalidTransactionLogRecord);
            this.isEnlistedTransaction = false;
        }

        internal TransactionLogRecord ChildTransactionRecord
        {
            get
            {
                if (this.childTransactionRecord == null)
                {
                    return null;
                }

                TransactionLogRecord result = null;
                bool exists = this.childTransactionRecord.TryGetTarget(out result);

#if (!DEBUG)
                Utility.Assert(
                    exists,
                    "Child Transaction for tx Id: {0} is not present. Lsn: {1}",
                    this.transaction.Id,
                    this.Lsn);
#endif

                return result;
            }

            set { this.childTransactionRecord = new WeakReference<TransactionLogRecord>(value); }
        }

        internal bool IsEnlistedTransaction
        {
            get { return this.isEnlistedTransaction; }

            set { this.isEnlistedTransaction = value; }
        }

        internal TransactionLogRecord ParentTransactionRecord
        {
            get { return this.parentTransactionRecord; }

            set { this.parentTransactionRecord = value; }
        }

        internal ulong ParentTransactionRecordOffset
        {
            get { return this.parentTransactionRecordOffset; }
        }

        internal TransactionBase Transaction
        {
            get { return this.transaction; }

            set { this.transaction = value; }
        }

        protected int CalculateWireSize(OperationData data)
        {
            // Number of Array segments for data.
            var size = sizeof(int);

            if (data == null)
            {
                return size;
            }

            foreach (var item in data)
            {
                size += sizeof(int);
            }

            return size;
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
            var startingPosition = bw.BaseStream.Position;

            if (this.replicatedData.Array == null)
            {
                bw.BaseStream.Position += sizeof(int);

                bw.Write(this.transaction.Id);

                var endPosition = bw.BaseStream.Position;
                var sizeOfSection = checked((int) (endPosition - startingPosition));
                bw.BaseStream.Position = startingPosition;
                bw.Write(sizeOfSection);
                bw.BaseStream.Position = endPosition;

                this.replicatedData = CreateArraySegment(startingPosition, bw);
            }

            operationData.Add(this.replicatedData);

            if (isPhysicalWrite)
            {
                // Physical Metadata section.
                var physicalStartPosition = bw.BaseStream.Position;
                bw.BaseStream.Position += sizeof(int);

                startingPosition = bw.BaseStream.Position;

                if (this.parentTransactionRecordOffset == InvalidLogicalRecordOffset || forceRecomputeOffsets == true)
                {
                    Utility.Assert(
                        this.parentTransactionRecord != InvalidLogicalLogRecord || forceRecomputeOffsets == true,
                        "(this.parentTransactionRecord != LogicalLogRecord.InvalidLogicalLogRecord)={0}, forceRecomputeOffsets={1}",
                        this.parentTransactionRecord != InvalidLogicalLogRecord,
                        forceRecomputeOffsets);

                    if (this.parentTransactionRecord == null)
                    {
                        this.parentTransactionRecordOffset = 0;
                    }
                    else
                    {
                        Utility.Assert(
                            this.parentTransactionRecord.RecordPosition != InvalidRecordPosition,
                            "his.parentTransactionRecord.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                        Utility.Assert(
                            this.RecordPosition != InvalidRecordPosition,
                            "this.RecordPosition != LogRecord.INVALID_RECORD_POSITION");
                        this.parentTransactionRecordOffset = this.RecordPosition
                                                             - this.parentTransactionRecord.RecordPosition;
                    }
                }
                bw.Write(this.parentTransactionRecordOffset);

                // End Physical Metadata section
                var physicalEndPosition = bw.BaseStream.Position;
                var physicalSizeOfSection = checked((int) (physicalEndPosition - physicalStartPosition));
                bw.BaseStream.Position = physicalStartPosition;
                bw.Write(physicalSizeOfSection);
                bw.BaseStream.Position = physicalEndPosition;

                operationData.Add(CreateArraySegment(physicalStartPosition, bw));
            }

            return;
        }

        private void ReadPrivate(BinaryReader br, bool isPhysicalRead)
        {
            var logicalStartingPosition = br.BaseStream.Position;
            var logicalSizeOfSection = br.ReadInt32();
            var logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

            this.transaction = TransactionBase.CreateTransaction(br.ReadInt64(), true);

            // Jump to the end of the section ignoring fields that are not understood.
            Utility.Assert(logicalEndPosition >= br.BaseStream.Position, "Could not have read more than section size.");
            br.BaseStream.Position = logicalEndPosition;

            if (isPhysicalRead)
            {
                // Metadata section.
                var physicalStartPosition = br.BaseStream.Position;
                var physicalSectionSize = br.ReadInt32();
                var physicalEndPosition = physicalStartPosition + physicalSectionSize;

                this.parentTransactionRecordOffset = br.ReadUInt64();
                if (this.parentTransactionRecordOffset == 0)
                {
                    this.parentTransactionRecord = null;
                }

                // Jump to the end of the section ignoring fields that are not understood.
                Utility.Assert(physicalEndPosition >= br.BaseStream.Position, "Could not have read more than section size.");
                br.BaseStream.Position = physicalEndPosition;
            }

            this.UpdateApproximateDiskSize();
        }

        private void UpdateApproximateDiskSize()
        {
            this.ApproximateSizeOnDisk += ApproximateDiskSpaceUsed;
        }
    }
}