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
        // Represents the operations of a transaction or a single atomic/atomic-redo operation
        //
        class OperationLogRecord final 
            : public TransactionLogRecord
        {
            K_FORCE_SHARED(OperationLogRecord)

        public:

            static OperationLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            static OperationLogRecord::SPtr Create(
                __in TxnReplicator::TransactionBase & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            static OperationLogRecord::SPtr Create(
                __in TxnReplicator::TransactionBase & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in KAllocator & allocator);

            virtual bool get_IsLatencySensitiveRecord() const override
            {
                return BaseTransaction.IsAtomicOperation;
            }

            virtual std::wstring ToString() const override;

            //
            // TRUE if this is a redo only operation, which implies it can never be undone
            //
            __declspec(property(get = get_IsRedoOnly)) bool IsRedoOnly;
            bool get_IsRedoOnly() const
            {
                return isRedoOnly_;
            }

            __declspec(property(get = get_IsOperationContextNull)) bool IsOperationContextNull;
            bool get_IsOperationContextNull() const
            {
                return (operationContext_.RawPtr() == nullptr);
            }

            __declspec(property(put = set_OperationContext)) TxnReplicator::OperationContext::CSPtr OperationContextValue;
            void set_OperationContext(__in TxnReplicator::OperationContext const & value)
            {
                operationContext_ = &value;
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

            OperationLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            OperationLogRecord(
                __in TxnReplicator::TransactionBase & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            OperationLogRecord(
                __in TxnReplicator::TransactionBase & transaction,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            bool isRedoOnly_;
            Utilities::OperationData::CSPtr metaData_;
            Utilities::OperationData::CSPtr undo_;
            Utilities::OperationData::CSPtr redo_;
            TxnReplicator::OperationContext::CSPtr operationContext_;

            Utilities::OperationData::CSPtr replicatedData_;
        };
    }
}
