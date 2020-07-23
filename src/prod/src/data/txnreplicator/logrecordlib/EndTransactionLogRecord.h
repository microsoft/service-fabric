// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        // 
        // Represents a commit/abort transaction operation
        //
        class EndTransactionLogRecord final 
            : public TransactionLogRecord
        {
            K_FORCE_SHARED(EndTransactionLogRecord)

        public:

            //
            // Used during recovery
            //
            static EndTransactionLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            static EndTransactionLogRecord::SPtr Create(
                __in TxnReplicator::TransactionBase & transaction,
                __in bool isCommitted,
                __in bool isThisReplicaTransaction,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            virtual bool get_IsLatencySensitiveRecord() const override
            {
                return true;
            }

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_IsCommitted)) bool IsCommitted;
            bool get_IsCommitted() const
            {
                return isCommitted_;
            }

            __declspec(property(get = get_IsThisReplicaTransaction)) bool IsThisReplicaTransaction;
            bool get_IsThisReplicaTransaction() const
            {
                return isThisReplicaTransaction_;
            }

            bool Test_Equals(__in LogRecord const & other) const override;

        private:
            static const ULONG DiskSpaceUsed;
            static const ULONG SizeOnWireIncrement;

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            void ReadLogical(
                __in Utilities::OperationData const & operationData,
                __inout INT & index) override;

            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

            ULONG GetSizeOnWire() const override;

            EndTransactionLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            EndTransactionLogRecord(
                __in TxnReplicator::TransactionBase & transaction,
                __in bool isCommitted,
                __in bool isThisReplicaTransaction,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            void UpdateApproximateDiskSize();

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            bool const isThisReplicaTransaction_;
            bool isCommitted_;
            KBuffer::SPtr replicatedData_;
        };
    }
}
