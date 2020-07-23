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
        // Represents a Begin Transaction Operation log record. It is only applicable for transactions (And not atomic/atomic-redo operations)
        // This log record is created on the first operation of a transaction.
        // If the transaction consists of only 1 operation (followed by a commit), the field "IsSingleOperationTransaction" represents if this is a committed tx.
        //
        class BeginTransactionOperationLogRecord final 
            : public TransactionLogRecord
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED(BeginTransactionOperationLogRecord)

        public:

            //
            // Used during recovery
            //
            static BeginTransactionOperationLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            static BeginTransactionOperationLogRecord::SPtr Create(
                __in TxnReplicator::TransactionBase & transactionBase,
                __in bool isSingleOperationTransaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            virtual bool get_IsLatencySensitiveRecord() const override
            {
                return isSingleOperationTransaction_;
            }

            virtual std::wstring ToString() const override;

            //
            // Returns TRUE If the transaction consists of only 1 operation (followed by a commit)
            //
            __declspec(property(get = get_IsSingleOperationTransaction)) bool IsSingleOperationTransaction;
            bool get_IsSingleOperationTransaction() const
            {
                return isSingleOperationTransaction_;
            }

            __declspec(property(put = set_OperationContext)) TxnReplicator::OperationContext::CSPtr OperationContextValue;
            void set_OperationContext(__in TxnReplicator::OperationContext const & value)
            {
                operationContext_ = &value;
            }

            __declspec(property(get = get_RecordEpoch, put = set_RecordEpoch)) TxnReplicator::Epoch & RecordEpoch;
            TxnReplicator::Epoch const & get_RecordEpoch() const
            {
                return recordEpoch_;
            }

            void set_RecordEpoch(TxnReplicator::Epoch const & value)
            {
                recordEpoch_ = value;
            }

            __declspec(property(get = get_Metadata, put = set_MetaData)) Utilities::OperationData::CSPtr Metadata;
            Utilities::OperationData::CSPtr get_Metadata() const
            {
                return metaData_;
            }
            void set_MetaData(__in Utilities::OperationData const & value)
            {
                metaData_ = &value;
            }

            __declspec(property(get = get_Undo)) Utilities::OperationData::CSPtr Undo;
            Utilities::OperationData::CSPtr get_Undo() const
            {
                return undo_;
            }

            __declspec(property(get = get_Redo)) Utilities::OperationData::CSPtr Redo;
            Utilities::OperationData::CSPtr get_Redo() const
            {
                return redo_;
            }

            TxnReplicator::OperationContext::CSPtr ResetOperationContext();

            bool Test_Equals(__in LogRecord const & other) const override;

        private:

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

            BeginTransactionOperationLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            BeginTransactionOperationLogRecord(
                __in TxnReplicator::TransactionBase & transactionBase,
                __in bool isSingleOperationTransaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            bool isSingleOperationTransaction_;
            Utilities::OperationData::CSPtr metaData_;
            Utilities::OperationData::CSPtr undo_;
            Utilities::OperationData::CSPtr redo_;
            TxnReplicator::OperationContext::CSPtr operationContext_;
            TxnReplicator::Epoch recordEpoch_;
            Utilities::OperationData::SPtr replicatedData_;
        };
    }
}
