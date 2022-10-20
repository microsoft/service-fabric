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
        // A base class for all transaction records: BeginTx, EndTx and Operation
        //
        class TransactionLogRecord
            : public LogicalLogRecord
            , public KWeakRefType<TransactionLogRecord>
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED_WITH_INHERITANCE(TransactionLogRecord)

        public:

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_IsEnlistedTransaction, put = set_IsEnlistedTransaction)) bool IsEnlistedTransaction;
            bool get_IsEnlistedTransaction() const
            {
                return isEnlistedTransaction_;
            }

            void set_IsEnlistedTransaction(__in bool value)
            {
                isEnlistedTransaction_ = value;
            }

            __declspec(property(get = get_childTransactionRecord, put = set_childTransactionRecord)) TransactionLogRecord::SPtr ChildTransactionRecord;
            TransactionLogRecord::SPtr get_childTransactionRecord() const
            {
                TransactionLogRecord::SPtr strongRef = childTransactionRecord_->TryGetTarget();
                return strongRef;
            }

            void set_childTransactionRecord(__in TransactionLogRecord & value)
            {
                childTransactionRecord_ = value.GetWeakRef();
            }

            __declspec(property(get = get_parentTransactionRecord, put = set_parentTransactionRecord)) TransactionLogRecord::SPtr ParentTransactionRecord;
            TransactionLogRecord::SPtr get_parentTransactionRecord() const
            {
                return parentTransactionRecord_;
            }

            void set_parentTransactionRecord(__in_opt TransactionLogRecord * const value)
            {
                parentTransactionRecord_ = value;
            }

            __declspec(property(get = get_parentTransactionRecordOffset)) ULONG64 ParentTransactionRecordOffset;
            ULONG64 get_parentTransactionRecordOffset() const
            {
                return parentTransactionRecordOffset_;
            }

            __declspec(property(get = get_TransactionBase)) TxnReplicator::TransactionBase & BaseTransaction;
            TxnReplicator::TransactionBase & get_TransactionBase() const
            {
                return *transactionBase_;
            }

            LONG64 get_TransactionId() const
            {
                return transactionBase_->get_TransactionId();
            }

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            static ULONG CalculateDiskWriteSize(__in_opt Utilities::OperationData const * const data);
            static ULONG CalculateWireSize(__in_opt Utilities::OperationData const * const data);

            TransactionLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog);

            TransactionLogRecord(
                __in LogRecordType::Enum recordType,
                __in TxnReplicator::TransactionBase & transactionBase,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in TransactionLogRecord & invalidTransactionLog,
                __in_opt TransactionLogRecord * parentTransactionRecord);

            virtual void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            virtual void ReadLogical(
                __in Utilities::OperationData const & operationData,
                __inout INT & index) override;

            virtual void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

            virtual ULONG GetSizeOnWire() const override;

            TxnReplicator::TransactionBase::SPtr transactionBase_;
            TransactionLogRecord::SPtr parentTransactionRecord_;

        private:

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            void FreeParentChainProactively();

            static const ULONG DiskSpaceUsed;
            static const ULONG SizeOnWireIncrement;

            void UpdateApproximateDiskSize();

            bool isEnlistedTransaction_;

            // Forward references are always Weak Ref's to avoid circular chains
            KWeakRef<TransactionLogRecord>::SPtr childTransactionRecord_;
            ULONG64 parentTransactionRecordOffset_;

            KBuffer::SPtr replicatedData_;
        };
    }
}
