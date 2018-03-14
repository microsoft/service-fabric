// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestTransaction final 
        : public KObject<TestTransaction>
        , public KShared<TestTransaction>
    {
        K_FORCE_SHARED(TestTransaction)

    public:

        class TestOperationContext
            : public TxnReplicator::OperationContext
        {
            K_FORCE_SHARED(TestOperationContext)

        public:
            static TestOperationContext::CSPtr Create(__in KAllocator & allocator);
        };

        static const UINT BufferContentMaxValue;

        static TestTransaction::SPtr Create(
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords,
            __in Data::Utilities::OperationData const * const metaData,
            __in Data::Utilities::OperationData const * const undo,
            __in Data::Utilities::OperationData const * const redo,
            __in bool isSingleOperationTx,
            __in NTSTATUS flushError,
            __in KAllocator & allocator);

        static TestTransaction::SPtr Create(
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords,
            __in ULONG bufferCount,
            __in ULONG operationSize,
            __in bool isSingleOperationTx,
            __in NTSTATUS flushError,
            __in KAllocator & allocator);

        static Data::Utilities::OperationData::SPtr GenerateOperationData(
            __in ULONG bufferCount,
            __in ULONG operationSize,
            __in KAllocator & allocator);

        __declspec(property(get = get_LogRecords)) KArray<Data::LogRecordLib::TransactionLogRecord::SPtr> & LogRecords;
        KArray<Data::LogRecordLib::TransactionLogRecord::SPtr> const & get_LogRecords() const
        {
            return logRecords_;
        }

        __declspec(property(get = get_Tx)) TxnReplicator::Transaction::SPtr Tx;
        TxnReplicator::Transaction::SPtr get_Tx() const
        {
            return transaction_;
        }

        // Contents of the add operation buffer is equal to the index of the operation in the transaction
        void AddOperation(
            __in ULONG bufferCount = 1,
            __in ULONG operationSize = 1,
            __in NTSTATUS flushError = STATUS_SUCCESS);

        // Add an OperationLogRecord initialized with operation data objects for metadata/undo/redo
        void AddOperation(
            __in Data::Utilities::OperationData const * const metaData,
            __in Data::Utilities::OperationData const * const undo,
            __in Data::Utilities::OperationData const * const redo,
            __in NTSTATUS flushError = STATUS_SUCCESS);

        void Commit(__in bool isCommitted);

    private:

        TestTransaction(
            __in ULONG bufferCount,
            __in ULONG operationSize,
            __in bool isSingleOperationTx,
            __in NTSTATUS flushError,
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords);

        // Create test transaction initialized with operation data objects for metadata/undo/redo
        // Metadata objects are used to create the BeginTransactionLogRecord
        TestTransaction(
            __in Data::Utilities::OperationData const * const metaData,
            __in Data::Utilities::OperationData const * const undo,
            __in Data::Utilities::OperationData const * const redo,
            __in bool isSingleOperationTx,
            __in NTSTATUS flushError,
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords);
        
        TxnReplicator::Transaction::SPtr transaction_;
        KArray<Data::LogRecordLib::TransactionLogRecord::SPtr> logRecords_;
        Data::LogRecordLib::InvalidLogRecords::SPtr invalidLogRecords_;
        bool const isSingleOperationTx_;
        NTSTATUS flushError_;
    };
}
