// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTTX_TAG 'nxTT'

const UINT TestTransaction::BufferContentMaxValue = 5;

TestTransaction::TestOperationContext::TestOperationContext()
{
}

TestTransaction::TestOperationContext::~TestOperationContext()
{
}

TestTransaction::TestOperationContext::CSPtr TestTransaction::TestOperationContext::Create(__in KAllocator & allocator)
{
    TestOperationContext * pointer = _new(TESTTX_TAG, allocator) TestOperationContext();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TestOperationContext::CSPtr(pointer);
}

TestTransaction::TestTransaction(
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in bool isSingleOperationTx,
    __in NTSTATUS flushError,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , logRecords_(GetThisAllocator())
    , transaction_(_new(TESTTX_TAG, GetThisAllocator())Transaction(TransactionBase::CreateTransactionId(), false))
    , invalidLogRecords_(&invalidLogRecords)
    , isSingleOperationTx_(isSingleOperationTx)
    , flushError_(flushError)
{
    CODING_ERROR_ASSERT(transaction_ != nullptr);

    OperationData::SPtr operationData = GenerateOperationData(bufferCount, operationSize, GetThisAllocator());
    TestOperationContext::CSPtr context = TestOperationContext::Create(GetThisAllocator());

    BeginTransactionOperationLogRecord::SPtr beginTxLog = BeginTransactionOperationLogRecord::Create(
        *transaction_,
        isSingleOperationTx_,
        operationData.RawPtr(),
        operationData.RawPtr(),
        operationData.RawPtr(),
        context.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    beginTxLog->ParentTransactionRecord = nullptr;
    beginTxLog->CompletedFlush(flushError_);

    logRecords_.Append(beginTxLog.RawPtr());
}

TestTransaction::TestTransaction(
    __in OperationData const * const meta,
    __in OperationData const * const undo,
    __in OperationData const * const redo,
    __in bool isSingleOperationTx,
    __in NTSTATUS flushError,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , logRecords_(GetThisAllocator())
    , transaction_(_new(TESTTX_TAG, GetThisAllocator())Transaction(TransactionBase::CreateTransactionId(), false))
    , invalidLogRecords_(&invalidLogRecords)
    , isSingleOperationTx_(isSingleOperationTx)
    , flushError_(flushError)
{
    CODING_ERROR_ASSERT(transaction_ != nullptr);

    TestOperationContext::CSPtr context = TestOperationContext::Create(GetThisAllocator());

    BeginTransactionOperationLogRecord::SPtr beginTxLog = BeginTransactionOperationLogRecord::Create(
        *transaction_,
        isSingleOperationTx_,
        meta,
        undo,
        redo,
        context.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    beginTxLog->ParentTransactionRecord = nullptr;
    beginTxLog->CompletedFlush(flushError_);

    logRecords_.Append(beginTxLog.RawPtr());
}


TestTransaction::~TestTransaction()
{
}

TestTransaction::SPtr TestTransaction::Create(
    __in InvalidLogRecords & invalidLogRecords,
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in bool isSingleOperationTx,
    __in NTSTATUS flushError,
    __in KAllocator & allocator)
{
    TestTransaction * tx = _new(TESTTX_TAG, allocator)TestTransaction(bufferCount, operationSize, isSingleOperationTx, flushError, invalidLogRecords);
    CODING_ERROR_ASSERT(tx != nullptr);

    return TestTransaction::SPtr(tx);
}

TestTransaction::SPtr TestTransaction::Create(
    __in InvalidLogRecords & invalidLogRecords,
    __in OperationData const * const meta,
    __in OperationData const * const undo,
    __in OperationData const * const redo,
    __in bool isSingleOperationTx,
    __in NTSTATUS flushError,
    __in KAllocator & allocator)
{
    TestTransaction * tx = _new(TESTTX_TAG, allocator)TestTransaction(meta, undo, redo, isSingleOperationTx, flushError, invalidLogRecords);
    CODING_ERROR_ASSERT(tx != nullptr);

    return TestTransaction::SPtr(tx);
}

OperationData::SPtr TestTransaction::GenerateOperationData(
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in KAllocator & allocator)
{
    OperationData::SPtr operationData = OperationData::Create(allocator);

    for (ULONG i = 0; i < bufferCount; i++)
    {
        KBuffer::SPtr buffer;
        NTSTATUS status = KBuffer::Create(
            operationSize,
            buffer,
            allocator);

        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        char * bufferContent = (char *) buffer->GetBuffer();

        for (ULONG index = 0; index < operationSize; index++)
        {
            *(bufferContent + index) = (char)i % BufferContentMaxValue;
        }

        operationData->Append(*buffer);
    }

    return operationData;
}

void TestTransaction::AddOperation(
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in NTSTATUS flushError)
{
    CODING_ERROR_ASSERT(!isSingleOperationTx_);
    CODING_ERROR_ASSERT(logRecords_[logRecords_.Count() - 1]->RecordType != LogRecordType::Enum::EndTransaction);

    OperationData::SPtr operationData = GenerateOperationData(bufferCount, operationSize, GetThisAllocator());
    TestOperationContext::CSPtr context = TestOperationContext::Create(GetThisAllocator());

    OperationLogRecord::SPtr operationLog = OperationLogRecord::Create(
        *transaction_,
        operationData.RawPtr(),
        operationData.RawPtr(),
        operationData.RawPtr(),
        context.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());
    
    if (NT_SUCCESS(flushError_) && !NT_SUCCESS(flushError))
    {
        flushError_ = flushError;
    }

    operationLog->CompletedFlush(flushError_);
    operationLog->ParentTransactionRecord = logRecords_[logRecords_.Count() - 1].RawPtr();
    logRecords_[logRecords_.Count() - 1]->ChildTransactionRecord = *operationLog;

    logRecords_.Append(operationLog.RawPtr());
}

void TestTransaction::AddOperation(
    __in OperationData const * const metaData,
    __in OperationData const * const undo,
    __in OperationData const * const redo,
    __in NTSTATUS flushError)
{
    CODING_ERROR_ASSERT(!isSingleOperationTx_);
    CODING_ERROR_ASSERT(logRecords_[logRecords_.Count() - 1]->RecordType != LogRecordType::Enum::EndTransaction);

    TestOperationContext::CSPtr context = TestOperationContext::Create(GetThisAllocator());

    OperationLogRecord::SPtr operationLog = OperationLogRecord::Create(
        *transaction_,
        metaData,
        undo,
        redo,
        context.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    if (NT_SUCCESS(flushError_) && !NT_SUCCESS(flushError))
    {
        flushError_ = flushError;
    }

    operationLog->CompletedFlush(flushError_);
    operationLog->ParentTransactionRecord = logRecords_[logRecords_.Count() - 1].RawPtr();
    logRecords_[logRecords_.Count() - 1]->ChildTransactionRecord = *operationLog;

    logRecords_.Append(operationLog.RawPtr());
}

void TestTransaction::Commit(bool isCommitted)
{
    CODING_ERROR_ASSERT(!isSingleOperationTx_);
    CODING_ERROR_ASSERT(logRecords_[logRecords_.Count() - 1]->RecordType != LogRecordType::Enum::EndTransaction);

    EndTransactionLogRecord::SPtr endTxLog = EndTransactionLogRecord::Create(
        *transaction_,
        isCommitted,
        true,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    endTxLog->CompletedFlush(flushError_);
    endTxLog->ParentTransactionRecord = logRecords_[logRecords_.Count() - 1].RawPtr();
    logRecords_[logRecords_.Count() - 1]->ChildTransactionRecord = *endTxLog;

    logRecords_.Append(endTxLog.RawPtr());
}
